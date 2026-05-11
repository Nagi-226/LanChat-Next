# v1.0.5 -> v1.1.0 Migration Notes

v1.0.5 is the final feature-bearing legacy Qt checkpoint. From v1.1.0 onward,
new work should happen beside the legacy code, not inside it, unless fixing a
blocking bug.

## Keep

- `protocol/message_types.h`: framework-agnostic message ids and field names.
- Legacy JSON shapes documented in `docs/LEGACY_API.md`.
- SQLite table intent for users, private messages, channels, members, and history.
- Interaction behavior: login, online list, private chat, channel join/leave, channel chat.

## Replace In v1.1.x-v1.2.x

- `QTcpServer`, `QTcpSocket`, `QThread`: replace with C++17 `asio` server.
- Qt SQL access: replace with SQLiteCpp or a thin prepared-statement DAO.
- Legacy frame codec: keep the 4-byte big-endian frame format, reimplement without Qt.
- `MsgBuilder` Qt wrapper: replace with pure C++ JSON serialization.

## Replace Later

- `QWidget` and `.ui` client screens: replace with Tauri v2 + React 18 + TypeScript.
- QRC avatar resources: replace with file-system assets and client cache.
- Legacy in-memory online/channel session maps: replace with explicit session/channel services.

## Drop

- `cJSON.*` and `CJsonObject.*` once no archived demo needs them.
- `.pro.user` files and historical Qt Creator build folders.
- Legacy UI-only prototypes under `legacy/qq-chat-prototype` after visual references are extracted.

## v1.1.0 Entry Criteria

- Legacy server/client code remains buildable in a Qt 5.15 environment.
- `docs/LEGACY_API.md` matches the on-wire protocol.
- New `server-next/` and `client-next/` skeletons compile independently.
- No new feature work is added to legacy Qt modules after this checkpoint.

