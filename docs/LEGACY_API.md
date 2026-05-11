# LanChat Legacy API (v1.0.5)

This document freezes the legacy Qt protocol used by `legacy/original-server` and
`legacy/original-client` before the v1.1.0 new-stack migration starts.

## Transport

- TCP port: `12345` (`legacy/original-*/Publicdef.h`)
- Encoding: UTF-8
- Frame: 4-byte unsigned big-endian length + compact JSON body
- Compatibility: the decoder still accepts a single unframed JSON object for old manual smoke tests

Example framed body:

```json
{"type":2,"id":100000,"password":"admin123"}
```

## Message Types

| Type | Name | Direction | Purpose |
|---:|---|---|---|
| 0 | registerUser | Client -> Server | Register a new account |
| 1 | registerUserReturn | Server -> Client | Return assigned user id |
| 2 | login | Client -> Server | Login request |
| 3 | loginSucReturn | Server -> Client | Login success with online friend list |
| 4 | loginLoseReturn | Server -> Client | Login failed |
| 5 | sendMsg | Client -> Server | Private message send |
| 6 | receiveMsg | Server -> Client | Private message delivery |
| 7 | userOnline | Server -> Client | Presence online notification |
| 8 | userOffline | Server -> Client | Presence offline notification |
| 9 | createGroup | Client -> Server | Create channel |
| 10 | createGroupReturn | Server -> Client | Return channel id |
| 11 | searchGroup | Client -> Server | List/search channels |
| 12 | searchGroupReturn | Server -> Client | Channel list |
| 13 | joinGroup | Client -> Server | Join channel |
| 14 | joinGroupReturn | Server -> Client | Joined channel member snapshot |
| 15 | leaveGroup | Client -> Server | Leave channel |
| 16 | sendGroupMsg | Client -> Server | Send channel message |
| 17 | receiveGroupMsg | Server -> Client | Deliver channel message |
| 18 | userJoinGroup | Server -> Client | Channel member joined |
| 19 | userLeaveGroup | Server -> Client | Channel member left |
| 20-33 | reserved | Both | LanChat-Next extension space |

## Common Fields

- `type`: integer message type.
- `id`: user id for user-scoped messages.
- `password`: plaintext only on the wire during login/register; persisted as salted hash.
- `headId`: legacy avatar id.
- `nickname`: display name.
- `fromId`, `toId`: private-message sender/receiver.
- `groupId`: channel id.
- `name`: channel name.
- `msg`: text content.

## Examples

Register:

```json
{"type":0,"password":"secret","headId":0,"nickname":"Alice"}
{"type":1,"id":100001}
```

Private chat:

```json
{"type":5,"fromId":100001,"toId":100002,"msg":"hello"}
{"type":6,"fromId":100001,"toId":100002,"msg":"hello"}
```

Channel flow:

```json
{"type":9,"name":"general"}
{"type":10,"groupId":200000}
{"type":13,"id":100001,"headId":0,"nickname":"Alice","groupId":200000}
{"type":16,"id":100001,"headId":0,"nickname":"Alice","groupId":200000,"msg":"hi all"}
```

Error frame:

```json
{"type":33,"status":"error","code":400,"msg":"invalid message type"}
```

## Persistence

- Users: `hhuser.db`, table `user`.
- Private messages: `hhchat.db`, table `private_messages`.
- Channels: `hhchat.db`, tables `channels`, `channel_members`, `channel_messages`.


