<p align="center">
  <img src="https://raw.githubusercontent.com/Nagi-226/LanChat-Next/master/docs/assets/logo.svg" alt="LanChat-Next" width="120" height="120" onerror="this.style.display='none'">
</p>

<h1 align="center">LanChat-Next</h1>

<p align="center">
  <strong>面向中小企业的局域网智能社交平台</strong><br>
  现代化桌面即时通讯 · AI 驱动 · 零云端依赖
</p>

<p align="center">
  <a href="https://github.com/Nagi-226/LanChat-Next/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/version-v2.0.0--a1-blueviolet" alt="Version"></a>
  <a href="#"><img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B" alt="C++17"></a>
  <a href="#"><img src="https://img.shields.io/badge/Asio-standalone-00599C" alt="Asio"></a>
  <a href="#"><img src="https://img.shields.io/badge/Tauri-2.0-FFC131?logo=tauri" alt="Tauri"></a>
  <a href="#"><img src="https://img.shields.io/badge/React-18-61DAFB?logo=react" alt="React 18"></a>
  <a href="#"><img src="https://img.shields.io/badge/TypeScript-5.x-3178C6?logo=typescript" alt="TypeScript"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-Windows%2011-0078D6?logo=windows" alt="Windows 11"></a>
  <a href="#"><img src="https://img.shields.io/badge/build-passing-brightgreen" alt="Build"></a>
  <a href="#"><img src="https://img.shields.io/badge/tests-7%2F7-brightgreen" alt="Tests"></a>
</p>

---

## What is LanChat-Next?

A complete rewrite of a graduation project LAN chat application, rebuilt from the ground up with modern technology and AI-assisted development. Designed for **small-to-medium enterprises (≤ 500 users)** — deploy on a single LAN server, no cloud dependency.

> **Vision**: Discord-level UI polish, Telegram-level simplicity, enterprise LAN reliability.
> **Endgame (v2.0.0)**: Zero Qt dependencies. Pure C++17 server + Tauri/React desktop client.

---

## Features

### Chat & Social
- **Private messaging** — real-time, optimistic send, delivery/read status
- **Group chat** — create, join, leave, member list
- **Friend system** — request, accept, reject, remove, online presence
- **Message features** — edit, delete (soft), emoji reactions (👍❤️😂😮😢😡🎉✅)
- **Read receipts** — sent → delivered → read (single/double/blue checks)
- **Typing indicators** — real-time "typing..." display
- **Offline queue** — compose messages offline, auto-send on reconnect
- **File/image transfer** — metadata relay, image paste support

### AI Assistant
- **DeepSeek API** — V4-Pro / V3 / R1 / Flash models
- **Message search** — full-text LAN history search
- **Conversation summary** — AI-generated chat summaries
- **Plugin architecture** — extensible AI provider interface

### Desktop Experience
- **Native Tauri window** — 1180×760, min 800×560
- **OS notifications** — Windows native toast on new messages
- **Dark/Light theme** — persisted, class-based Tailwind dark mode
- **Chinese/English i18n** — full UI translation, toggle in header
- **Responsive layout** — 3 breakpoints (lg/md/sm)
- **Error resilience** — ErrorBoundary at root + section level

### Developer Tooling
- **codebase-health-audit** — 6-pillar automated audit
- **Protocol generation** — 51 JSON schemas from single source of truth
- **CI/CD gates** — pre-commit, pre-PR, quality gate, DB migration test
- **Full verification** — tsc + Vite + Cargo + CMake + CTest (7/7)

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  LanChat-Next Client (Tauri v2)                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  React 18 / TypeScript UI                             │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐  │   │
│  │  │ Login    │ │ ChatArea │ │ Contacts │ │ AI Panel│  │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────┘  │   │
│  │  State: authStore · messageStore · friendStore ·      │   │
│  │         searchStore · connectionStore · uiStore       │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Tauri Rust Bridge: TCP Manager · Frame Codec ·      │   │
│  │  Notifications · Encrypted Storage                    │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────┬───────────────────────────────────────┘
                       │  TCP + JSON (4-byte BE length-prefixed frames)
                       │  Port 12346 · Max frame 256 KiB
┌──────────────────────┴───────────────────────────────────────┐
│  LanChat-Next Server (C++17 + standalone Asio)               │
│  ┌────────────┐ ┌──────────────┐ ┌──────────────────────┐   │
│  │ TcpServer  │ │ MessageRouter│ │ PresenceManager      │   │
│  │ · multi-   │ │ · 21 handlers│ │ · Online/Offline     │   │
│  │   threaded │ │ · P2P/Group  │ │ · Status broadcast   │   │
│  │ · Heartbeat│ │ · Offline Q  │ │                      │   │
│  └────────────┘ └──────────────┘ └──────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Data Layer: SQLite (WAL) · User/Message/Channel/    │   │
│  │  Friend Repositories · bcrypt ($2b$ cost 12)         │   │
│  │  Rate limiting: 20 msg/s per session                 │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────┘
```

---

## Tech Stack

| Layer | Technology | Notes |
|-------|-----------|-------|
| **Server** | C++17 · CMake 3.28 | MSVC generator on Windows |
| **Server Net** | standalone Asio | Multi-threaded io_context, strand sync |
| **Server JSON** | mini-json (vendored) | → nlohmann/json in v2.0.0-a3 |
| **Server DB** | SQLite + vendored wrapper | WAL mode, indexed |
| **Server Auth** | bcrypt $2b$ (cost 12) + sha256 auto-upgrade | openwall crypt_blowfish |
| **Server Log** | mini-spdlog (vendored) | → real spdlog in v2.0.0-a2 |
| **Client UI** | React 18 · TypeScript (strict) · Tailwind CSS 3.4 | Vite 5, class-based dark mode |
| **Client State** | Zustand 4.5 | 6 stores (auth/message/friend/search/connection/ui) |
| **Client i18n** | Custom lightweight module (~2.5KB) | `useTranslation()` hook + standalone `t()` |
| **Client Anim** | Framer Motion + GSAP | 11 lib components (React Bits) |
| **Desktop** | Tauri v2 · Rust · Tokio | Native window, notifications, encrypted storage |
| **AI** | DeepSeek API (client-direct) | V4-Pro/V3/R1/Flash, SSE streaming |
| **Protocol** | TCP · JSON · 4-byte BE length-prefixed frames | 51 message types (0–50), JSON Schema |

---

## Quick Start

### Prerequisites

| Tool | Version | Install |
|------|---------|---------|
| CMake | ≥ 3.28 | `winget install Kitware.CMake` |
| MSVC Build Tools | 2022 | `winget install Microsoft.VisualStudio.2022.BuildTools` |
| Node.js | 20 LTS | `winget install OpenJS.NodeJS.LTS` |
| Rust | ≥ 1.78 | `winget install Rustlang.Rustup` |

### Build & Run

```bash
# 1. Clone
git clone https://github.com/Nagi-226/LanChat-Next.git
cd LanChat-Next

# 2. Build server
cmake -S . -B build/server-next-vs
cmake --build build/server-next-vs --config Debug

# 3. Start server (port 12346)
./build/server-next-vs/src/server/Debug/lanchat_server_next.exe

# 4. Build & run client (separate terminal)
cd src/client
npm install
npm run tauri dev        # Desktop app
# or
npm run dev              # Browser dev server (port 1420)
```

---

## Features Roadmap

| Version | Name | Key Deliverables | Status |
|---------|------|-----------------|--------|
| v1.0.5 | Legacy Seal | Frame protocol, channel persistence, offline messages | ✅ Done |
| v1.1.0 | Blueprint | C++ server + Tauri/React skeleton | ✅ Done |
| v1.2.x | Server Core | SQLite, routing, session pool, presence, unit tests | ✅ Done |
| v1.3.x | Client Base | Login/Register, chat UX, contacts, groups, error handling | ✅ Done |
| v1.4.x–v1.5.x | Animation | 11 React Bits components, UI transitions, smart scroll | ✅ Done |
| v1.6.0 | Animation Complete | Full-stack audit, quality baseline | ✅ Done |
| v1.7.0 | AI + Friends | DeepSeek API, friend system, i18n, security hardening | ✅ Done |
| v1.8.0 | Quality Hardening | ErrorBoundary, CSP, offline queue, CI/CD gates | ✅ Done |
| v1.9.0 | Feature Complete | Edit/delete/reactions, read receipts, file transfer | ✅ Done |
| v1.9.1 | Store Split | chatStore → 4 stores, component extraction | ✅ Done |
| **v2.0.0-a1** | **real asio** | **Standalone Asio transport, 500-client smoke** | ✅ **Current** |
| v2.0.0-a2 | real spdlog | Async logging, structured logs | ⬜ Next |
| v2.0.0-a3 | nlohmann/json | Full JSON compliance | ⬜ |
| v2.0.0-a4 | DB Normalized | Relational schema, migration v3 | ⬜ |
| v2.0.0-a5 | bcrypt offload | Worker thread pool, non-blocking auth | ⬜ |
| **v2.0.0** | **Release** | **Zero Qt, 500-user validated, .msi installer** | 🎯 |

> Full roadmap: [`docs/ROADMAP_v2.0.0.md`](docs/ROADMAP_v2.0.0.md) | Client guide: [`CODEX.md`](CODEX.md) | Server guide: [`CLAUDE.md`](CLAUDE.md)

---

## Project Structure

```
LanChat-Next/
├── src/
│   ├── server/                  # C++17 server
│   │   ├── CMakeLists.txt
│   │   ├── src/                 # TcpServer, AsyncSession, LanChatServer,
│   │   │   │                    #   MessageRouter (21 handlers),
│   │   │   │                    #   SessionPool, PresenceManager, FrameCodec,
│   │   │   │                    #   ServerLogger, main
│   │   │   └── db/              # Database, UserRepository, MessageRepository,
│   │   │                        #   ChannelRepository, FriendRepository,
│   │   │                        #   LegacyMigrator
│   │   ├── include/             # Public headers (lanchat::server)
│   │   └── vendor/              # Asio, mini-json, mini-spdlog, sha256, bcrypt, sqlite
│   └── client/                  # Tauri v2 + React client
│       ├── src/                 # React UI
│       │   ├── components/      # App, ChatArea, GroupChatArea, ContactList,
│       │   │                    #   LoginPanel, RegisterPanel, ConnectionBar, Sidebar,
│       │   │                    #   HeaderActions, ReconnectBanner, AppSplash,
│       │   │                    #   ContactRow, FriendRequestRow, GroupRow, ...
│       │   ├── stores/          # authStore, messageStore, friendStore,
│       │   │                    #   searchStore, connectionStore, uiStore
│       │   ├── lib/             # i18n, AI providers, 11 animation components,
│       │   │                    #   offlineQueue, ErrorBoundary, secureStorage, Toast
│       │   └── hooks/           # useChatScroll, useChatInput
│       └── src-tauri/           # Rust backend
│           └── src/             # main, lib, commands, tcp_manager, message_codec,
│                                #   notifications
├── protocol/                    # Shared protocol truth source
│   ├── protocol_definitions.json  # 51 message types (source of truth)
│   ├── message_types.h          # C++ enum
│   ├── message_types.ts         # TypeScript types
│   └── schemas/                 # 51 JSON Schema files
├── legacy/                      # Original Qt 5.4 project (read-only archive)
├── docs/                        # Design docs, roadmap, audit reports
├── scripts/                     # Build/verify automation, CI/CD gates
├── tests/                       # Integration, smoke, load tests
├── .claude/skills/              # Reusable AI agent skills
├── CMakeLists.txt               # Root build entry
├── CLAUDE.md                    # Server development guide
├── CODEX.md                     # Client development guide
└── README.md                    # This file
```

---

## Development

### AI-Assisted Workflow

This project is developed with a multi-AI-agent architecture:

| Agent | Primary Role |
|-------|-------------|
| **Claude Code** (DeepSeek-V4-Pro) | Architecture design, C++ server, code review, documentation |
| **Codex** (GPT-5.5) | Code generation, React/TypeScript, Rust bridge, testing |

### Quality Gates

Every milestone follows a `.0 → .5 → .0` progression with mandatory checkpoints:
- `.0` — feature delivery
- `.1–.4` — polish iterations
- `.5` — quality gate (full audit, build verification, type check)

### Build Verification

```powershell
# Client type-check
cd src/client && npx tsc --noEmit

# Client production build
cd src/client && npm run build

# Rust bridge check
cd src/client && cargo check

# Server build + test
cmake -S . -B build/server-next-vs
cmake --build build/server-next-vs --config Debug
ctest -C Debug --test-dir build/server-next-vs   # 7/7 passing

# Protocol audit
node scripts/audit_protocol.mjs                    # 51 schemas aligned

# Full checkpoint
./scripts/verify_v2_0_0_a1.ps1
```

### Current Quality Metrics

| Metric | Value |
|--------|-------|
| CTest | 7/7 passing |
| TypeScript | strict, zero errors |
| Bundle (JS gzip) | 172 KB |
| Protocol schemas | 51 aligned |
| Server handlers | 21 |
| Client stores | 6 (all < 420L) |

---

## Contributing

This is a personal graduation project rewrite. Contributions, issues, and feature requests are welcome.

1. Read [`docs/ROADMAP_v2.0.0.md`](docs/ROADMAP_v2.0.0.md) for the full roadmap
2. Check existing issues and discussions
3. Open a PR against `master` with clear description

---

## License

MIT © 2024–2026 [Nagi-226](https://github.com/Nagi-226)

---

<p align="center">
  <sub>Built with AI · Graduation project redemption arc · <a href="https://github.com/Nagi-226/LanChat-Next">Star this repo</a></sub>
</p>
