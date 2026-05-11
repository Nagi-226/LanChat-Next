# v1.1.0 Blueprint Checkpoint

Date: 2026-05-11

## Goal

Start the new architecture beside the legacy Qt code without deleting or blocking it.

- Legacy Qt server remains on port `12345`.
- New server-next listens on port `12346`.
- New client-next is a Tauri v2 + React + TypeScript skeleton.
- `protocol/` is now shared by C++, TypeScript, JSON Schema, and Rust codec work.

## Delivered

- `protocol/message_types.ts`: TypeScript enum and shared client-side protocol types aligned with `protocol/message_types.h`.
- `protocol/schemas/*.schema.json`: 34 draft JSON Schema files, one per message type.
- `src/server/`: C++17 CMake server skeleton with length-prefixed frame codec and a minimal TCP listener.
- `src/client/`: Tauri/React/Rust skeleton with protocol frame utilities and TCP command placeholders.
- `CMakeLists.txt`: root build entry for server-next.

## Technical Note

The server skeleton has a WinSock/POSIX listener fallback so it can compile immediately on this machine. `CMakeLists.txt` detects standalone `asio.hpp` if it exists, but the full async asio implementation is intentionally scheduled for v1.1.3 hardening after the dependency is vendored or installed.

## Remaining v1.1.0 Validation

- Install npm/cargo dependencies, then run `npm run tauri:dev` under `src/client`.
- Run legacy Qt server on `12345` and server-next on `12346` simultaneously.
- Replace schema placeholders with strict per-message required fields in v1.1.4.

## Next Step Decision

v1.1.0 is considered started and materially delivered as a blueprint skeleton. The next practical milestone is `v1.1.3`:

- Install or configure a build driver (`ninja`, `mingw32-make`, or VS Developer Prompt `nmake`).
- Vendor or install standalone `asio.hpp`, `spdlog`, `SQLiteCpp`, and bcrypt-compatible hashing dependency.
- Replace the temporary WinSock/POSIX server fallback with the planned asio async listener.
- Add a minimal CMake-driven test target for `FrameCodec`.
- Install client dependencies and verify `npm run tauri:dev`.

## Local Validation

- `protocol/schemas`: 34 schema files present.
- `protocol/message_types.ts` and `protocol/message_types.h`: terminal enum value `SystemBroadcast = 33` aligned.
- `g++` direct smoke build succeeded: `build/manual/lanchat_server_next_smoke.exe`.
- server-next runtime smoke succeeded on `127.0.0.1:12346`: heartbeat `{ "type": 20 }` returned `{ "type": 21, "status": "ok" }`.
- `cargo metadata --no-deps` succeeded for `src/client/src-tauri/Cargo.toml`.
- `npm --prefix src/client pkg get ...` succeeded, confirming package metadata/scripts are readable.

## Environment Gaps

- CMake configure with `MinGW Makefiles` did not run because no make program (`mingw32-make`, `ninja`, or `nmake`) is installed in PATH.
- Tauri dev was not launched because npm/cargo dependencies are not installed and network is restricted.
