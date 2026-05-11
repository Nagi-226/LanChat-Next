# LanChat Legacy — Original Graduation Project (v1.0.1)

> Built: 2021-01 ~ 2021-05 | Qt 5.4 + C++11 | QMake

## Overview

LAN ChatRoom is a TCP-based local network instant messaging application. It supports user registration/login, private messaging, and basic group chat. This directory contains the original graduation project code, preserved as reference for the LanChat-Next rebuild.

## Directory Map

| Directory | Original Name | Description |
|-----------|--------------|-------------|
| `original-client/` | HHClient | Qt 5.4 GUI client — login, register, friend list, private/group chat |
| `original-server/` | HHServer | Qt 5.4 TCP server — connection management, message routing, SQLite users |
| `qq-chat-prototype/` | QQChat | QQ-style chat UI prototype |
| `chat-demo-with-group-and-unread/` | 聊天demo | Group chat demo with unread message badges |

## Architecture

```
Client (HHClient.exe)                    Server (HHServer.exe)
┌──────────────────────┐                ┌──────────────────────────┐
│  QTcpSocket (singleton)│─── TCP ────→ │  QTcpServer               │
│  cJSON parsing         │              │  1 thread per connection  │
│  .ui form layouts      │              │  MsgBuilder (cJSON)       │
│  heads.qrc (3 avatars) │              │  UserDao (QSQLITE)        │
└──────────────────────┘                │  GroupData (in-memory)    │
                                        └──────────────────────────┘
```

## Protocol

- **Format**: JSON over TCP (no length prefix, assumes single-frame reads)
- **JSON Library**: cJSON (C library with manual memory management)
- **Message Types**: 19 types defined in `MsgBuilder.h` (enum), covering auth, private chat, group lifecycle
- **Encoding**: `QString::toLocal8Bit()` / `fromLocal8Bit()` (GBK on Chinese Windows)

### Message Type Enum

```
0  registerUser        10 createGroupReturn
1  registerUserReturn  11 searchGroup
2  login               12 searchGroupReturn
3  loginSucReturn      13 joinGroup
4  loginLoseReturn     14 joinGroupReturn
5  sendMsg             15 leaveGroup
6  receiveMsg          16 sendGroupMsg
7  userOnline          17 receiveGroupMsg
8  userOffline         18 userJoinGroup
9  createGroup         19 userLeaveGroup
```

## Build Requirements

- **Qt 5.4** (or Qt 5.15 LTS for better Win11 compatibility)
- **Compiler**: MSVC 2015+ or MinGW 5.3+
- **Qt Modules**: `core`, `gui`, `widgets`, `network`, `sql`

### Build Steps

```powershell
# Client
cd legacy/original-client
qmake HHClient.pro
nmake   # or mingw32-make

# Server
cd legacy/original-server
qmake HHServer.pro
nmake
```

## Database

- **Engine**: SQLite via `QSqlDatabase("QSQLITE")`
- **File**: `hhuser.db` (auto-created in working directory)
- **Schema** (only 1 table):

```sql
CREATE TABLE user (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    password CHAR(20),
    nickname CHAR(20),
    headid   INTEGER
);
```

## Known Issues (Targets for v1.0.2)

| Severity | Issue | Impact |
|----------|-------|--------|
| 🔴 Critical | Passwords stored as plaintext in SQLite | Security |
| 🔴 Critical | No message persistence — messages lost on restart | Data loss |
| 🟠 High | `cJSON` manual malloc/free — memory leak risk | Stability |
| 🟠 High | One thread per client model — doesn't scale past ~50 users | Scalability |
| 🟠 High | Group/Channel data stored only in `std::map` memory — lost on restart | Data loss |
| 🟡 Medium | `QString::fromLocal8Bit()` assumes GBK encoding — breaks on UTF-8 systems | Compatibility |
| 🟡 Medium | No heartbeat/keepalive — zombie connections not detected | Reliability |
| 🟡 Medium | `.ui` files hard-coded QRC avatar resources (3 hardcoded avatars) | Flexibility |
| 🟢 Low | Chinese comments — encoding issues on non-Chinese systems | Portability |
| 🟢 Low | `using namespace std` + `using namespace neb` in headers | Code smell |

## Migration Notes

This legacy codebase serves as the **protocol reference** and **interaction blueprint** for LanChat-Next. The new project will:

1. **v1.0.2**: Fix critical issues (bcrypt passwords, SQLite message persistence, cJSON → nlohmann)
2. **v1.1.0+**: Gradually replace modules — protocol first, then server (asio), then client UI (Tauri+React)
3. **v2.0.0**: Legacy code fully retired

See `../DEVELOPMENT_PLAN.md` for the full migration roadmap.
