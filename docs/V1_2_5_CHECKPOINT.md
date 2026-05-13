# v1.2.5 Accelerated Checkpoint

Date: 2026-05-13

This checkpoint validates the first v1.2.x server-core slice: session routing, presence, repository-backed message persistence, group flow, offline delivery, and a 200-client smoke target.

## Completed

- Added server-core modules: `SessionPool`, `PresenceManager`, `MessageRouter`, `MessageRepository`, and `ChannelRepository`.
- Routed heartbeat, register, login, logout, private message, group message, group create/search/join/leave, history, read receipt, profile update, and AI placeholder requests through `MessageRouter`.
- Added persistence-oriented in-memory repositories for messages and groups on top of the existing `Database` facade.
- Added `tests/server_v1_2_5_load.mjs` for latency, group delivery, offline delivery, read receipt, search, and load-smoke coverage.
- Added `scripts/verify_v1_2_5.ps1` to run protocol checks, CMake/CTest, routing smoke, 50-client acceptance smoke, 200-client checkpoint smoke, Vite build, and Cargo check.

## Verification

Latest local run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_v1_2_5.ps1 -LoadDurationMs 1500
```

Result: passed.

Key smoke metrics from the successful run:

- 50-client smoke: P2P latency 1.578 ms, group delivery 10/10, offline delivery ok, read receipt ok, search ok.
- 200-client smoke: P2P latency 2.261 ms, group delivery 10/10, offline delivery ok, read receipt ok, search ok.
- Frontend: `npm run build` passed.
- Rust bridge: `cargo metadata` and `cargo check` passed.

## Follow-ups

- Run the same checkpoint with the default 5000 ms load duration before tagging.
- Decide whether to keep `mini-asio` as project-owned infrastructure or replace it with pinned standalone asio.
- Replace SHA-256 password hashing with the selected bcrypt-compatible implementation before production hardening.
- Run an interactive desktop E2E pass with `npm run tauri dev` for connect -> login -> send -> receive -> theme toggle.
