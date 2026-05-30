import { spawn } from 'node:child_process';
import fs from 'node:fs';
import net from 'node:net';
import os from 'node:os';
import path from 'node:path';

const root = path.resolve(import.meta.dirname, '..');

function argValue(name, fallback) {
  const index = process.argv.indexOf(name);
  if (index >= 0 && process.argv[index + 1]) return process.argv[index + 1];
  return fallback;
}

const serverExe = argValue('--server', path.join(root, 'build', 'server-release', 'src', 'server', 'Release', 'lanchat_server_next.exe'));
const clients = Number(argValue('--clients', '50'));
const messages = Number(argValue('--messages', '20'));
const port = Number(argValue('--port', '12447'));
const dataDir = path.join(os.tmpdir(), `lanchat-load-${Date.now()}`);

function encode(message) {
  const body = Buffer.from(JSON.stringify(message), 'utf8');
  const frame = Buffer.allocUnsafe(4 + body.length);
  frame.writeUInt32BE(body.length, 0);
  body.copy(frame, 4);
  return frame;
}

function createClient() {
  const socket = net.createConnection({ host: '127.0.0.1', port });
  let buffer = Buffer.alloc(0);
  let heartbeatTimer = null;
  const inbox = [];
  const waiters = [];

  function dispatch(payload) {
    const waiterIndex = waiters.findIndex((waiter) => waiter.predicate(payload));
    if (waiterIndex >= 0) {
      const [waiter] = waiters.splice(waiterIndex, 1);
      clearTimeout(waiter.timer);
      waiter.resolve(payload);
      return;
    }
    inbox.push(payload);
  }

  socket.on('data', (chunk) => {
    buffer = Buffer.concat([buffer, chunk]);
    while (buffer.length >= 4) {
      const len = buffer.readUInt32BE(0);
      if (buffer.length < 4 + len) break;
      const payload = JSON.parse(buffer.subarray(4, 4 + len).toString('utf8'));
      buffer = buffer.subarray(4 + len);
      dispatch(payload);
    }
  });

  return {
    ready: new Promise((resolve, reject) => {
      socket.once('connect', resolve);
      socket.once('error', reject);
    }),
    send(message) {
      socket.write(encode(message));
    },
    startHeartbeat() {
      if (heartbeatTimer) return;
      heartbeatTimer = setInterval(() => {
        socket.write(encode({ type: 20 }));
      }, 20000);
    },
    waitFor(predicate, label, timeoutMs = 8000) {
      const index = inbox.findIndex(predicate);
      if (index >= 0) {
        const [payload] = inbox.splice(index, 1);
        return Promise.resolve(payload);
      }
      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          const waiterIndex = waiters.findIndex((waiter) => waiter.timer === timer);
          if (waiterIndex >= 0) waiters.splice(waiterIndex, 1);
          reject(new Error(`timeout waiting for ${label}`));
        }, timeoutMs);
        waiters.push({ predicate, resolve, timer });
      });
    },
    close() {
      if (heartbeatTimer) clearInterval(heartbeatTimer);
      socket.end();
    },
  };
}

async function waitForServer() {
  const deadline = Date.now() + 8000;
  while (Date.now() < deadline) {
    try {
      const probe = net.createConnection({ host: '127.0.0.1', port });
      await new Promise((resolve, reject) => {
        probe.once('connect', resolve);
        probe.once('error', reject);
      });
      probe.end();
      return;
    } catch {
      await new Promise((resolve) => setTimeout(resolve, 150));
    }
  }
  throw new Error(`server did not listen on ${port}`);
}

function stopProcess(child) {
  if (child.exitCode !== null || child.signalCode !== null) {
    return Promise.resolve();
  }
  return new Promise((resolve) => {
    child.once('exit', resolve);
    child.kill();
  });
}

async function bootstrapClient(index) {
  const client = createClient();
  await client.ready;

  client.send({ type: 50, version: 2, min_version: 1, features: ['load_smoke'] });
  const hello = await client.waitFor((msg) => msg.type === 50, `protocol hello ${index}`);
  if (hello.status !== 'ok') throw new Error(`protocol hello failed for ${index}: ${JSON.stringify(hello)}`);

  client.send({ type: 0, nickname: `load-${Date.now()}-${index}`, password: 'secret' });
  const registered = await client.waitFor((msg) => msg.type === 1, `register ${index}`);
  if (registered.status !== 'ok' || !registered.id) throw new Error(`register failed for ${index}: ${JSON.stringify(registered)}`);

  client.send({ type: 2, id: registered.id, password: 'secret' });
  const login = await client.waitFor((msg) => msg.type === 3 && msg.id === registered.id, `login ${index}`);
  if (login.id !== registered.id) throw new Error(`login failed for ${index}: ${JSON.stringify(login)}`);
  client.startHeartbeat();

  return { client, userId: registered.id };
}

async function main() {
  if (!fs.existsSync(serverExe)) {
    throw new Error(`server executable not found: ${serverExe}`);
  }
  if (!Number.isInteger(clients) || clients < 2) {
    throw new Error('--clients must be an integer >= 2');
  }

  fs.rmSync(dataDir, { recursive: true, force: true });
  const server = spawn(serverExe, ['--port', String(port), '--data', dataDir], {
    cwd: root,
    stdio: ['ignore', 'pipe', 'pipe'],
  });

  const startedAt = Date.now();
  const sessions = [];
  try {
    await waitForServer();
    for (let start = 0; start < clients; start += 10) {
      const batch = await Promise.all(
        Array.from({ length: Math.min(10, clients - start) }, (_, offset) => bootstrapClient(start + offset)),
      );
      sessions.push(...batch);
    }

    for (let index = 0; index < messages; index += 1) {
      const sender = sessions[index % sessions.length];
      const receiver = sessions[(index + 1) % sessions.length];
      const text = `load-smoke-${index}`;
      sender.client.send({ type: 5, fromId: sender.userId, toId: receiver.userId, msg: text });
      await receiver.client.waitFor(
        (msg) => msg.type === 6 && msg.fromId === sender.userId && msg.msg === text,
        `message ${index}`,
      );
    }

    const durationMs = Date.now() - startedAt;
    console.log(`load smoke PASS: clients=${clients} messages=${messages} duration_ms=${durationMs}`);
  } finally {
    for (const session of sessions) session.client.close();
    await stopProcess(server);
    for (let attempt = 0; attempt < 5; attempt += 1) {
      try {
        fs.rmSync(dataDir, { recursive: true, force: true });
        break;
      } catch {
        await new Promise((resolve) => setTimeout(resolve, 200));
      }
    }
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
