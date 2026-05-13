# v1.1.x Stabilization For v1.2.x

Date: 2026-05-12

This note records the fixes that make the current v1.1.x baseline safe enough to start v1.2.x server-core work.

## Fixed blockers

- Per-session frame buffering: `AsyncSession` now owns its read accumulator, so half frames from one client cannot pollute another client's decoder state.
- Partial socket writes: `AsyncSession` tracks `write_offset_` and keeps writing until the queued frame is fully flushed.
- Tauri TCP manager: the client splits `TcpStream` into read/write halves so a pending read no longer holds the same mutex needed for sends.
- Minimal protocol smoke responses: the server now returns heartbeat ack, register return, login success, and an echo-style receive message for early UI/network smoke testing.
- Encoding cleanup: visible React text and C++ vendor comments were normalized to avoid mojibake and MSVC C4819 warnings.

## Added verification

- `tests/server_multi_client_smoke.mjs` starts the server, opens two clients, interleaves one client's half-frame with another client's heartbeat, and confirms both sessions receive correct heartbeat acknowledgements.
- `scripts/verify_v1_1_5.ps1` now runs the multi-client smoke test after CMake/CTest.

## Verified locally

- `powershell -ExecutionPolicy Bypass -File scripts\verify_v1_1_5.ps1`
- MSVC configure/build/test with `Visual Studio 17 2022` and `ctest -C Debug`

## Still deferred to v1.2.x

- Replace mini-asio with the selected real async networking dependency or formally accept mini-asio as project-owned code.
- Add real session pool, router, repositories, SQLite schema, authentication, heartbeat timeout sweep, and persistence tests.
- Replace smoke auth responses with repository-backed registration/login.
