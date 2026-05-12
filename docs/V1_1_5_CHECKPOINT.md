# v1.1.5 Checkpoint

Date: 2026-05-12

## Scope delivered

- v1.1.3 hardening baseline: dependency pins documented in `docs/DEPENDENCIES_V1_1_5.md`; CMake test target added for the server frame codec.
- v1.1.4 protocol completion: `protocol/protocol_definitions.json` is now the source of truth for 34 message types, strict JSON Schemas, and generated TypeScript protocol types.
- v1.1.5 checkpoint automation: `scripts/verify_v1_1_5.ps1` runs protocol audit, CMake configure/build/test, Vite build, and Cargo metadata/check from one command.

## Verification command

```powershell
./scripts/verify_v1_1_5.ps1
```

If frontend dependencies are intentionally not installed yet, use this reduced local audit:

```powershell
./scripts/verify_v1_1_5.ps1 -AllowMissingFrontendDeps -SkipCargoCheck
```

## Local verification

`./scripts/verify_v1_1_5.ps1` passes on this machine:

- Protocol generation check and 34-schema audit pass.
- CMake config/build uses Ninja and CTest; `frame_codec` passes.
- Multi-client TCP smoke test passes, including interleaved half-frame decoding.
- `npm run build` completes the Vite production build.
- `cargo metadata` and `cargo check` complete for the Tauri shell.

## Carry-forward

- Replace the WinSock/POSIX listener fallback with standalone-asio async networking once `asio.hpp` is vendored or installed.
- Vendor/install spdlog, SQLiteCpp, and the final bcrypt-compatible password library before the v1.2 data and routing layer work.
