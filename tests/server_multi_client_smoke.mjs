import net from 'node:net';
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const exe = process.argv[2];
const port = Number(process.argv[3] ?? 12347);
if (!exe) {
  console.error('usage: node tests/server_multi_client_smoke.mjs <server-exe> [port]');
  process.exit(2);
}

function frame(json) {
  const body = Buffer.from(json, 'utf8');
  const out = Buffer.alloc(4 + body.length);
  out.writeUInt32BE(body.length, 0);
  body.copy(out, 4);
  return out;
}

function collect(name, socket, replies) {
  let buf = Buffer.alloc(0);
  socket.on('data', (chunk) => {
    buf = Buffer.concat([buf, chunk]);
    while (buf.length >= 4) {
      const len = buf.readUInt32BE(0);
      if (buf.length < 4 + len) break;
      replies[name].push(buf.subarray(4, 4 + len).toString('utf8'));
      buf = buf.subarray(4 + len);
    }
  });
}

function wait(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function connectClient(name, replies) {
  const socket = net.createConnection({ host: '127.0.0.1', port });
  replies[name] = [];
  collect(name, socket, replies);
  return new Promise((resolve, reject) => {
    socket.once('connect', () => resolve(socket)).once('error', reject);
  });
}

function idFrom(reply, label) {
  const match = reply?.match(/"id":(\d+)/);
  if (!match) {
    throw new Error(`${label} did not return an id`);
  }
  return Number(match[1]);
}

async function run() {
  const dataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lanchat-server-smoke-'));
  const server = spawn(exe, ['--port', String(port), '--data', dataDir], {
    stdio: 'ignore',
    windowsHide: true,
  });
  await wait(500);

  const replies = {};
  const a = await connectClient('a', replies);
  const b = await connectClient('b', replies);

  const partialHeartbeat = frame('{"type":20}');
  a.write(partialHeartbeat.subarray(0, 2));
  await wait(40);
  b.write(frame('{"type":20}'));
  await wait(40);
  a.write(partialHeartbeat.subarray(2));
  await wait(40);

  a.write(frame('{"type":0,"nickname":"Alice","password":"pw"}'));
  b.write(frame('{"type":0,"nickname":"Bob","password":"pw"}'));
  await wait(500);
  const aliceId = idFrom(replies.a.find((r) => r.includes('"type":1')), 'Alice registration');
  const bobId = idFrom(replies.b.find((r) => r.includes('"type":1')), 'Bob registration');

  a.write(frame(`{"type":2,"id":${aliceId},"password":"pw"}`));
  b.write(frame(`{"type":2,"id":${bobId},"password":"pw"}`));
  await wait(500);

  a.write(frame(`{"type":5,"fromId":${aliceId},"toId":${bobId},"msg":"hello-bob"}`));
  await wait(300);

  a.write(frame(`{"type":9,"hostId":${aliceId},"name":"General"}`));
  await wait(300);
  const groupReply = replies.a.find((r) => r.includes('"type":10'));
  const groupMatch = groupReply?.match(/"groupId":(\d+)/);
  if (!groupMatch) {
    throw new Error('group creation did not return a groupId');
  }
  const groupId = Number(groupMatch[1]);
  b.write(frame(`{"type":13,"id":${bobId},"groupId":${groupId}}`));
  await wait(300);
  a.write(frame(`{"type":16,"fromId":${aliceId},"groupId":${groupId},"msg":"hello-group"}`));
  await wait(300);

  b.destroy();
  await wait(500);
  a.write(frame(`{"type":5,"fromId":${aliceId},"toId":${bobId},"msg":"offline-bob"}`));
  await wait(300);

  const c = await connectClient('c', replies);
  c.write(frame(`{"type":2,"id":${bobId},"password":"pw"}`));
  await wait(500);

  a.destroy();
  c.destroy();
  server.kill();
  await new Promise((resolve) => {
    server.once('exit', resolve);
    setTimeout(resolve, 1000);
  });

  const ok = replies.a.some((r) => r.includes('"type":21'))
    && replies.b.some((r) => r.includes('"type":21'))
    && replies.a.some((r) => r.includes('"type":3'))
    && replies.b.some((r) => r.includes('"type":3'))
    && replies.b.some((r) => r.includes('hello-bob'))
    && replies.b.some((r) => r.includes('"type":14'))
    && replies.b.some((r) => r.includes('hello-group'))
    && replies.c.some((r) => r.includes('"type":22') && r.includes('offline-bob'));

  console.log(JSON.stringify(replies));
  fs.rmSync(dataDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 100 });
  if (!ok) {
    throw new Error('server multi-client routing smoke failed');
  }
}

run().catch((err) => {
  console.error(err.message);
  process.exit(1);
});
