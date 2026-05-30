import { spawn } from 'node:child_process';
import fs from 'node:fs';
import net from 'node:net';
import os from 'node:os';
import path from 'node:path';

const root = path.resolve(import.meta.dirname, '..');
const serverExe = process.argv[2] || path.join(root, 'build', 'server-release', 'src', 'server', 'Release', 'lanchat_server_next.exe');
const port = 12446;
const dataDir = path.join(os.tmpdir(), `lanchat-e2e-${Date.now()}`);

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
  const waiters = [];

  socket.on('data', (chunk) => {
    buffer = Buffer.concat([buffer, chunk]);
    while (buffer.length >= 4) {
      const len = buffer.readUInt32BE(0);
      if (buffer.length < 4 + len) break;
      const payload = JSON.parse(buffer.subarray(4, 4 + len).toString('utf8'));
      buffer = buffer.subarray(4 + len);
      const waiter = waiters.shift();
      if (waiter) waiter(payload);
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
    next() {
      return new Promise((resolve) => waiters.push(resolve));
    },
    close() {
      socket.end();
    },
  };
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

async function main() {
  if (!fs.existsSync(serverExe)) {
    throw new Error(`server executable not found: ${serverExe}`);
  }
  fs.rmSync(dataDir, { recursive: true, force: true });
  const server = spawn(serverExe, ['--port', String(port), '--data', dataDir], {
    cwd: root,
    stdio: ['ignore', 'pipe', 'pipe'],
  });
  try {
    await new Promise((resolve) => setTimeout(resolve, 600));
    const client = createClient();
    await client.ready;

    client.send({ type: 50, version: 2, min_version: 1, features: ['e2e_smoke'] });
    const hello = await client.next();
    if (hello.type !== 50 || hello.status !== 'ok') {
      throw new Error(`protocol hello failed: ${JSON.stringify(hello)}`);
    }

    client.send({ type: 50, version: 3, min_version: 3, features: ['future_client'] });
    const incompatible = await client.next();
    if (incompatible.type !== 50 || incompatible.status !== 'error') {
      throw new Error(`incompatible protocol was not rejected: ${JSON.stringify(incompatible)}`);
    }

    client.send({ type: 0, nickname: `e2e-${Date.now()}`, password: 'secret' });
    const registered = await client.next();
    if (registered.type !== 1 || registered.status !== 'ok' || !registered.id) {
      throw new Error(`register failed: ${JSON.stringify(registered)}`);
    }

    client.send({ type: 2, id: registered.id, password: 'secret' });
    const login = await client.next();
    if (login.type !== 3 || login.id !== registered.id) {
      throw new Error(`login failed: ${JSON.stringify(login)}`);
    }

    client.close();
    console.log('protocol E2E smoke PASS');
  } finally {
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
