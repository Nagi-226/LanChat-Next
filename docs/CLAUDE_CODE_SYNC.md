# Claude Code Sync - v1.0.x Audit And v1.1.0 Handoff

Date: 2026-05-11

## v1.0.3-v1.0.5 Audit Decision

Claude Code side confirmed the v1.0.3-v1.0.5 code delivery passes for the purpose of migration, and v1.1.0 is allowed to proceed.

Decision: do not install Qt 5.15 for legacy GUI joint testing.

Reasons:

- The v2.0.0 target is zero Qt.
- The value of v1.0.x legacy work is to codify design intent and migration seams, not to ship a runnable Qt application.
- Protocol correctness is already covered by `tests/legacy_protocol_smoke.py`.
- The core migration-relevant logic is present: length-prefixed frames, channel persistence, and offline message replay.

## Deferred Items

The previous `[~]` items are not blockers:

- Heartbeat / sticky-packet integration testing: formalized in the v1.2.0 asio server integration suite.
- Group offline replay: completed in v1.2.0 with the new server data layer.
- GTest unit tests: implemented in v1.2.0 against C++17 + CMake + GTest, not the legacy Qt build.

## Accepted Deviations

- `cJSON -> QJsonDocument` in legacy Qt is accepted. `nlohmann/json` belongs in the new C++ server layer.
- Python smoke test replacing legacy GTest is accepted because there is no Qt toolchain and the legacy GUI is not a release target.

## v1.1.0 Current Status

Codex has already started v1.1.0 and delivered the first blueprint skeleton:

- `protocol/message_types.ts`
- `protocol/schemas/*.schema.json` for all 34 message types
- `src/server/` C++17 server-next skeleton
- `src/client/` Tauri v2 + React 18 + TypeScript skeleton
- `docs/V1_1_0_BLUEPRINT.md`
- `docs/CODEX_SKILLS_MCP.md`

Validation completed:

- 34 schema files present.
- C++ and TypeScript enum endpoints aligned at `SystemBroadcast = 33`.
- server-next direct `g++` smoke build succeeds.
- server-next heartbeat runtime smoke succeeds on port `12346`.
- `cargo metadata --no-deps` succeeds for the Tauri manifest.
- npm package metadata/scripts are readable.

Known environment gaps:

- CMake configure needs a make program such as `ninja`, `mingw32-make`, or `nmake`.
- Tauri dev needs npm/cargo dependency installation.

