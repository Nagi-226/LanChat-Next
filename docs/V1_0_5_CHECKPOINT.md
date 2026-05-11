# v1.0.5 Legacy Checkpoint

Date: 2026-05-11

## Feasibility Review

- v1.0.1 is feasible as an archive baseline: original Qt projects remain under `legacy/` for reference.
- v1.0.2 is directionally feasible, but required correction: the repo referenced `nlohmann/json.hpp` without vendoring or installing it. The legacy `MsgBuilder` now uses Qt JSON to keep the Qt checkpoint buildable; the pure C++ JSON layer should be introduced with the asio server.
- The long-term migration plan remains valid: protocol first, then network/data replacement, then Tauri/React UI, then v2.0.0 zero Qt.

## Implemented In This Checkpoint

- v1.0.3 network hardening:
  - 4-byte big-endian length-prefixed JSON frames.
  - UTF-8 socket encoding.
  - Legacy unframed single-JSON fallback for manual tests.
  - Server `server.log` logging.
  - Client reconnect with exponential backoff from 1s to 30s.
  - Invalid message type handling with an error frame.
- v1.0.4 channel completion:
  - Persistent `channels`, `channel_members`, and `channel_messages` tables.
  - Create/search/join/leave routes backed by SQLite channel metadata.
  - Private offline messages are replayed at login and marked read.
- v1.0.5 seal:
  - `docs/LEGACY_API.md`
  - `docs/MIGRATION_NOTES.md`
  - `build_legacy.bat`
  - `tests/legacy_protocol_smoke.py`

## Audit Decision

Claude Code sync on 2026-05-11 accepted v1.0.3-v1.0.5 as code-delivered and non-blocking for v1.1.0.

No Qt 5.15 GUI joint test will be performed because the final product target is zero Qt. Legacy Qt code is now treated as a migration reference and design-codification layer, not a release artifact.

Deferred `[~]` items move forward:

- Heartbeat / sticky-packet integration test -> v1.2.0 asio server integration tests.
- Group offline replay -> v1.2.0 new server data layer.
- GTest coverage -> v1.2.0 C++17 + CMake + GTest.

## Known Remaining Risks

- Group offline replay is persisted but not yet pushed as unread channel history on login.
- `legacy/qq-chat-prototype` and other archived demos still contain cJSON because they are preserved as historical references.
- Legacy password hashing is salted SHA-256, not bcrypt; the plan should upgrade this in the new server data layer.
