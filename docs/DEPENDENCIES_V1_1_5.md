# LanChat-Next Dependency Pins (v1.1.5)

This file records the dependency decisions required by the v1.1.3 hardening step and used by the v1.1.5 checkpoint scripts.

## Native server

| Dependency | Pinned version | Status | Notes |
| --- | --- | --- | --- |
| C++ standard | C++17 | Active | Required by `src/server/CMakeLists.txt`. |
| CMake | 3.28+ | Active | Root project minimum is 3.28. |
| standalone asio | 1.30.2 | Deferred install/vendor | `find_path(ASIO_INCLUDE_DIR asio.hpp)` is wired; current offline workspace keeps the WinSock/POSIX listener until the header is available. |
| spdlog | 1.14.1 | Deferred install/vendor | Pinned for v1.2 logging integration. |
| SQLiteCpp | 3.3.2 | Deferred install/vendor | Pinned for v1.2 repository layer. |
| bcrypt-compatible hashing | bcrypt 1.1 style API | Deferred selection | Final choice remains open until the v1.2 user repository is implemented. |

## Client

| Dependency | Pinned range | Status | Notes |
| --- | --- | --- | --- |
| Tauri | 2.x | Active | Rust and npm packages use Tauri v2. |
| React | 18.2.x | Active | UI skeleton baseline. |
| Vite | 5.x | Active | Build script: `npm run build`. |
| TypeScript | 5.x | Active | Strict TS config. |
| Zustand | 4.5.x | Active | UI state skeleton. |

## Build driver

CMake requires one of `ninja`, `mingw32-make`, `nmake`, or another configured generator build tool on PATH. The v1.1.5 verification script auto-detects these tools and fails with a clear message if none is available.
