import net from 'node:net';
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const exe = process.argv[2];
const port = Number(process.argv[3] ?? 12348);
const clients = Number(process.argv[4] ?? 50);
const durationMs = Number(process.argv[5] ?? 10000);

if (!exe) {
  console.error('usage: node tests/server_v1_2_5_load.mjs <server-exe> [port] [clients] [durationMs]');
  process.exit(2);
}

function frame(payload) {
  const json = typeof payload === 'string' ? payload : JSON.stringify(payload);
  const body = Buffer.from(json, 'utf8');
  const out = Buffer.alloc(4 + body.length);
  out.writeUInt32BE(body.length, 0);
  body.copy(out, 4);
  return out;
}

function wait(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

class Client {
  constructor(name) {
    this.name = name;
    this.socket = null;
    this.buffer = Buffer.alloc(0);
    this.messages = [];
    this.waiters = [];
    this.id = 0;
  }

  async connect(port) {
    this.socket = net.createConnection({ host: '127.0.0.1', port });
    this.socket.on('data', (chunk) => this.onData(chunk));
    await new Promise((resolve, reject) => {
      this.socket.once('connect', resolve).once('error', reject);
    });
  }

  onData(chunk) {
    this.buffer = Buffer.concat([this.buffer, chunk]);
    while (this.buffer.length >= 4) {
      const len = this.buffer.readUInt32BE(0);
      if (this.buffer.length < 4 + len) break;
      const raw = this.buffer.subarray(4, 4 + len).toString('utf8');
      let parsed = null;
      try { parsed = JSON.parse(raw); } catch { parsed = { raw }; }
      this.messages.push(parsed);
      this.buffer = this.buffer.subarray(4 + len);
    }
    this.flushWaiters();
  }

  flushWaiters() {
    for (const waiter of [...this.waiters]) {
      const found = this.messages.find(waiter.predicate);
      if (found) {
        clearTimeout(waiter.timer);
        this.waiters = this.waiters.filter((w) => w !== waiter);
        waiter.resolve(found);
      }
    }
  }

  send(payload) {
    this.socket.write(frame(payload));
  }

  waitFor(predicate, timeoutMs = 5000) {
    const found = this.messages.find(predicate);
    if (found) return Promise.resolve(found);
    return new Promise((resolve, reject) => {
      const waiter = { predicate, resolve, reject, timer: null };
      waiter.timer = setTimeout(() => {
        this.waiters = this.waiters.filter((w) => w !== waiter);
        reject(new Error(`${this.name} timed out waiting for message`));
      }, timeoutMs);
      this.waiters.push(waiter);
    });
  }

  destroy() {
    if (this.socket) this.socket.destroy();
  }
}

async function registerAndLogin(client, index) {
  const nickname = `LoadUser${index}_${Date.now()}`;
  client.send({ type: 0, nickname, password: 'pw' });
  const registered = await client.waitFor((m) => m.type === 1 && m.status === 'ok');
  client.id = registered.id;
  client.send({ type: 2, id: client.id, password: 'pw' });
  await client.waitFor((m) => m.type === 3 && m.id === client.id);
}

async function run() {
  const dataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lanchat-v125-load-'));
  const server = spawn(exe, ['--port', String(port), '--data', dataDir], {
    stdio: 'ignore',
    windowsHide: true,
  });
  await wait(500);

  const all = [];
  for (let i = 0; i < clients; i += 1) {
    const c = new Client(`c${i}`);
    all.push(c);
  }

  await Promise.all(all.map((c) => c.connect(port)));
  all.forEach((c) => c.send({ type: 20 }));
  await Promise.all(all.map((c) => c.waitFor((m) => m.type === 21)));

  for (let i = 0; i < all.length; i += 25) {
    await Promise.all(all.slice(i, i + 25).map((c, offset) => registerAndLogin(c, i + offset)));
  }

  const p2pToken = `p2p-${Date.now()}`;
  const p2pStart = performance.now();
  all[0].send({ type: 5, fromId: all[0].id, toId: all[1].id, msg: p2pToken });
  const p2pReceived = await all[1].waitFor((m) => m.type === 6 && m.msg === p2pToken);
  const p2pLatencyMs = performance.now() - p2pStart;

  const groupSize = Math.min(10, clients - 1);
  all[0].send({ type: 9, hostId: all[0].id, name: `LoadGeneral${Date.now()}` });
  const group = await all[0].waitFor((m) => m.type === 10 && m.groupId);
  for (let i = 1; i <= groupSize; i += 1) {
    all[i].send({ type: 13, id: all[i].id, groupId: group.groupId });
  }
  await Promise.all(all.slice(1, groupSize + 1).map((c) => c.waitFor((m) => m.type === 14 && m.groupId === group.groupId)));
  const groupToken = `group-${Date.now()}`;
  all[0].send({ type: 16, fromId: all[0].id, groupId: group.groupId, msg: groupToken });
  await Promise.all(all.slice(1, groupSize + 1).map((c) => c.waitFor((m) => m.type === 17 && m.msg === groupToken)));

  const offlineTargetId = all[2].id;
  all[2].destroy();
  await wait(300);
  const offlineToken = `offline-${Date.now()}`;
  all[0].send({ type: 5, fromId: all[0].id, toId: offlineTargetId, msg: offlineToken });
  await wait(300);
  const reconnected = new Client('offline-reconnect');
  await reconnected.connect(port);
  reconnected.send({ type: 2, id: offlineTargetId, password: 'pw' });
  await reconnected.waitFor((m) => m.type === 3 && m.id === offlineTargetId);
  await reconnected.waitFor((m) => m.type === 22 && JSON.stringify(m).includes(offlineToken));

  all[1].send({ type: 24, id: all[1].id, toId: all[0].id, msg_id: p2pReceived.msg_id, limit: 10 });
  const history = await all[1].waitFor((m) => m.type === 25 && JSON.stringify(m).includes(p2pToken));
  const readReceiptOk = JSON.stringify(history).includes('"read":true');

  all[0].send({ type: 29, request_id: 'load-search', ai_type: 'search', msg: p2pToken });
  await all[0].waitFor((m) => m.type === 30 && m.request_id === 'load-search' && JSON.stringify(m).includes(p2pToken));

  let sent = 0;
  let delivered = 0;
  const end = Date.now() + durationMs;
  while (Date.now() < end) {
    const fromIdx = sent % clients;
    const toIdx = (fromIdx + 1) % clients;
    if (fromIdx === 2 || toIdx === 2) { sent += 1; continue; }
    const token = `load-${sent}-${Date.now()}`;
    all[fromIdx].send({ type: 5, fromId: all[fromIdx].id, toId: all[toIdx].id, msg: token });
    try {
      await all[toIdx].waitFor((m) => m.type === 6 && m.msg === token, 2000);
      delivered += 1;
    } catch {}
    sent += 1;
  }

  const metrics = {
    clients,
    durationMs,
    p2pLatencyMs: Number(p2pLatencyMs.toFixed(3)),
    p2pUnder10ms: p2pLatencyMs < 10,
    groupDelivered: groupSize,
    offlineDelivered: true,
    readReceiptOk,
    searchOk: true,
    loadMessagesSent: sent,
    loadMessagesDelivered: delivered,
  };

  all.forEach((c) => c.destroy());
  reconnected.destroy();
  server.kill();
  await new Promise((resolve) => {
    server.once('exit', resolve);
    setTimeout(resolve, 1000);
  });
  fs.rmSync(dataDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 100 });

  console.log(JSON.stringify(metrics));
  if (!metrics.offlineDelivered || !metrics.readReceiptOk || metrics.groupDelivered !== groupSize || delivered === 0) {
    throw new Error('v1.2.5 load smoke failed acceptance checks');
  }
}

run().catch((err) => {
  console.error(err.stack || err.message);
  process.exit(1);
});
