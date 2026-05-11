# Next Steps - v1.1.3 Hardening

Date: 2026-05-11

v1.1.0 has started and delivered the first new-stack skeleton. The next milestone should be `v1.1.3`, focused on making the skeleton reproducibly buildable and replacing temporary fallbacks with the planned libraries.

## Environment Status

Available:

- CMake 3.29
- Node.js 23.9
- npm
- Rust / Cargo
- MinGW `g++`

Missing from PATH:

- `ninja`
- `mingw32-make`
- `nmake`
- MSVC `cl`

Impact: direct `g++` smoke builds work, but CMake cannot configure a generator until a build driver is installed or a VS Developer Prompt is used.

## v1.1.3 Tasks

1. Build driver setup
   - Install or expose one of `ninja`, `mingw32-make`, or `nmake`.
   - Re-run `cmake -S . -B build/server-next`.
   - Build `lanchat_server_next` through CMake rather than direct `g++`.

2. Server dependency hardening
   - Vendor or install standalone `asio.hpp`.
   - Add `spdlog`.
   - Add `SQLiteCpp`.
   - Choose the bcrypt-compatible password dependency.
   - Pin dependency versions in a documented location.

3. Server networking upgrade
   - Replace the temporary WinSock/POSIX listener fallback in `src/server/src/TcpServer.cpp`.
   - Implement asio async accept/read/write.
   - Keep port `12346` and the 4-byte big-endian frame format.
   - Add heartbeat timeout scaffolding.

4. Test bootstrap
   - Add a CMake test target.
   - Add unit tests for `FrameCodec`.
   - Add a Python or C++ integration smoke test for heartbeat and invalid frame handling.

5. Client bootstrap
   - Install npm dependencies in `src/client`.
   - Verify `npm run build`.
   - Verify `npm run tauri:dev`.
   - Keep UI polish ownership with Cursor/Gemini unless explicitly reassigned.

## v1.2.0 Carry-Forward

- Formal heartbeat / sticky-packet integration tests.
- Group offline replay in the new server data layer.
- C++ GTest coverage for protocol, frame codec, routing, and data access.
- SQLite schema migration from legacy tables to new server tables.

