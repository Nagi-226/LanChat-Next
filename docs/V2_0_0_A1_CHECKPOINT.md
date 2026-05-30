# v2.0.0-a1 Checkpoint — Real Asio Transport

> Date: 2026-05-30

## Scope Delivered

- Replaced the server transport include path from project-owned `mini_asio.hpp` to vendored standalone Asio.
- Added `lanchat/server/Net.h` as the single transport alias point for server networking code.
- Migrated `AsyncSession` and `TcpServer` to real `asio::io_context`, `asio::ip::tcp::socket`, `asio::ip::tcp::acceptor`, `asio::steady_timer`, and `asio::buffer`.
- Added `asio_transport` CTest coverage to ensure v2.0.0-a1 builds against real standalone Asio, not mini-asio.
- Added `scripts/verify_v2_0_0_a1.ps1`, which raises the load gate to 500 clients / 50 messages by default.
- Updated `scripts/load_smoke.mjs` to keep long-running load clients alive with heartbeats during slow auth bootstrap.

## Dependency

- Standalone Asio is vendored under `src/server/vendor/asio/include`.
- License: Boost Software License 1.0, copied to `src/server/vendor/asio/LICENSE_1_0.txt`.

## Verification

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_v2_0_0_a1.ps1
```

Target gate:
- Client Vitest, production build, protocol audit, and Tauri `cargo check`.
- Server Release build and CTest, including `asio_transport`.
- Protocol E2E smoke.
- 500-client / 50-message load smoke.

Latest full gate result: PASS on 2026-05-30.

Observed metrics:
- Vitest: 2 files / 5 tests passed.
- Protocol audit: 51 schemas, TypeScript enum, and C++ enum aligned.
- CTest: 7/7 tests passed, including `asio_transport`.
- Protocol E2E smoke passed.
- 500-client / 50-message load smoke passed in ~214.4s.
- Vite JS raw chunk warning remains at ~524.10 kB; gzip size is ~172.10 kB.

## Remaining v2.0.0 Blockers

- `v2.0.0-a2`: mini-spdlog -> official spdlog.
- `v2.0.0-a3`: mini-json -> nlohmann/json.
- `v2.0.0-a4`: JSON document DB -> normalized schema.
- `v2.0.0-a5`: bcrypt offload to worker threads. The 500-client load shows auth bootstrap is still slow and should be treated as the next event-loop responsiveness risk.
- `v2.0.0-a7+`: desktop Playwright E2E expansion.
