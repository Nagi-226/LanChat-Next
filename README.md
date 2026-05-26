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
  <a href="#"><img src="https://img.shields.io/badge/version-v1.6.0-blueviolet" alt="Version"></a>
  <a href="#"><img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B" alt="C++17"></a>
  <a href="#"><img src="https://img.shields.io/badge/Tauri-2.0-FFC131?logo=tauri" alt="Tauri"></a>
  <a href="#"><img src="https://img.shields.io/badge/React-18-61DAFB?logo=react" alt="React 18"></a>
  <a href="#"><img src="https://img.shields.io/badge/TypeScript-5.x-3178C6?logo=typescript" alt="TypeScript"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-Windows%2011-0078D6?logo=windows" alt="Windows 11"></a>
  <a href="#"><img src="https://img.shields.io/badge/status-active%20development-success" alt="Status"></a>
</p>

---

## What is LanChat-Next?

A complete rewrite of a graduation project LAN chat application, rebuilt from the ground up with modern technology and AI-assisted development. Designed for **small-to-medium enterprises (≤ 500 users)** — deploy on a single LAN server, no cloud dependency.

> **Vision**: Discord-level UI polish, Telegram-level simplicity, enterprise LAN reliability.
> **Endgame (v2.0.0)**: Zero Qt dependencies. Pure C++17 server + Tauri/React desktop client.

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  LanChat-Next Client (Tauri v2)                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  React 18 / TypeScript UI                             │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐  │   │
│  │  │ Login    │ │ ChatArea │ │ Contacts │ │ Sidebar │  │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────┘  │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Tauri Rust Bridge: TCP Manager · Frame Codec ·      │   │
│  │  Notifications                                        │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────┬───────────────────────────────────────┘
                       │  TCP + JSON (4-byte BE length-prefixed frames)
                       │  Port 12346
┌──────────────────────┴───────────────────────────────────────┐
│  LanChat-Next Server (C++17)                                 │
│  ┌────────────┐ ┌──────────────┐ ┌──────────────────────┐   │
│  │ TcpServer  │ │ MessageRouter│ │ PresenceManager      │   │
│  │ · Accept   │ │ · P2P/Group  │ │ · Online/Offline     │   │
│  │ · Heartbeat│ │ · Offline Q  │ │ · Status broadcast   │   │
│  └────────────┘ └──────────────┘ └──────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Data Layer: SQLite (WAL) · User/Message/Channel     │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────┘
```

---

## Tech Stack

| Layer | Technology | Notes |
|-------|-----------|-------|
| **Server** | C++17 · CMake 3.28 | MSVC generator on Windows |
| **Server Net** | mini-asio (vendored) | To be replaced with real asio |
| **Server JSON** | mini-json (vendored) | nlohmann/json wrapper |
| **Server DB** | SQLite + vendored wrapper | WAL mode |
| **Server Log** | mini-spdlog (vendored) | |
| **Server Hash** | sha256 (vendored) | bcrypt planned |
| **Client UI** | React 18 · TypeScript (strict) · Tailwind CSS 3.4 | Vite 5, class-based dark mode |
| **Client State** | Zustand 4.5 | `persist` on uiStore only |
| **Client Anim** | Framer Motion + GSAP | 11 lib components (React Bits) |
| **Desktop** | Tauri v2 · Rust · Tokio | Native window, notifications |
| **Protocol** | TCP · JSON · 4-byte BE length-prefixed frames | 34 message types, JSON Schema |

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
./build/server-next-vs/Debug/lanchat_server_next.exe

# 4. Seed a test user (optional)
./build/server-next-vs/Debug/lanchat_server_next.exe --migrate scripts/seed_users.json

# 5. Build & run client (separate terminal)
cd src/client
npm install
npm run tauri dev        # Desktop app
# or
npm run dev              # Browser dev server (port 1420)
```

> **Legacy Qt version**: The original Qt 5.4 graduation project is archived under `legacy/`. Not required for development.

---

## Features Roadmap

| Version | Name | Key Deliverables | Status |
|---------|------|-----------------|--------|
| v1.0.5 | Legacy Seal | Frame protocol, channel persistence, offline messages, password hashing | ✅ Done |
| v1.1.0 | Blueprint | C++ server + Tauri/React skeleton, 34 protocol schemas | ✅ Done |
| v1.2.0–v1.2.5 | Server Core | SQLite migration, message routing, session pool, presence, unit tests | ✅ Done |
| v1.3.0–v1.3.6 | Client Base | Login/Register, chat UX, contact search/sort, group chat wiring, error handling | ✅ Done |
| v1.4.0–v1.4.6 | Anim Phase 1–2 | 7 lib components, UI transitions, toast animations, micro-interactions, skeleton screens | ✅ Done |
| v1.5.0–v1.5.6 | Anim Phase 3–4 | Scroll-triggered animations, AnimatePresence exits, smart scroll, quality gate | ✅ Done |
| **v1.6.0** | **Animation Complete** | **React Bits Phase 1–4 full integration, full-stack audit, skill tooling** | ✅ **Current** |
| v1.6.1 | DRY Refactor | ChatArea/GroupChatArea shared hooks & components | ⬜ Next |
| v1.6.2 | Connection Hardening | Store-level reconnect timer, max retry limit | ⬜ |
| v1.6.3 | Server Hardening | Rate limiting, frame validation, port validation | ⬜ |
| v1.6.4 | UI Edge Cases | Loading/empty/error/disconnected state coverage | ⬜ |
| v1.6.5 | Quality Gate | Pre-v1.7.0 full audit pass | ⬜ |
| v1.7.0 | AI + Friends | AI sidebar panel, friend request/accept system | ⬜ |
| v2.0.0 | Release | Zero Qt, real asio/spdlog/bcrypt, .msi installer | 🎯 |

> Full plan: [`DEVELOPMENT_PLAN.md`](DEVELOPMENT_PLAN.md) | Client guide: [`CODEX.md`](CODEX.md) | Server guide: [`CLAUDE.md`](CLAUDE.md)

---

## Current Feature Set (v1.6.0)

### Working End-to-End
- User registration & login (sha256 password hashing)
- Private messaging (real-time, optimistic send, delivery status)
- Group chat (create, join, leave, member list, @mentions)
- Online/offline presence (real-time broadcast)
- Offline message delivery (queued on server, delivered on login)
- Heartbeat keep-alive with timeout sweep
- Auto-login with saved credentials (localStorage)
- Dark/Light theme toggle (persisted)
- Contact list with search, sort (online-first), unread badges
- Recent server address memory (last 5)

### UI/UX Polish
- 11 animation components (ClickSpark, SpotlightCard, BorderGlow, TextType, Counter, GradientText, ShinyText, AnimatedList, AnimatedContent, FadeContent, Toast)
- Smart scroll (won't yank you to bottom when reading history)
- "New messages" floating button with unread count
- Skeleton screens (contacts, messages)
- AnimatePresence transitions (sidebar, AI panel, message list, toast)
- `prefers-reduced-motion` support
- Focus-visible accessibility styling

### Developer Tooling
- `codebase-health-audit` skill (6-pillar multi-dimensional audit framework)
- Protocol schema generation & audit scripts (34 message types)
- Multi-client TCP smoke tests
- Full verification pipeline (tsc + Vite + Cargo + CMake + CTest)

---

## Project Structure

```
LanChat-Next/
├── src/
│   ├── server/                  # C++17 server
│   │   ├── CMakeLists.txt
│   │   ├── src/                 # TcpServer, AsyncSession, LanChatServer,
│   │   │   │                    #   MessageRouter, PresenceManager, SessionPool,
│   │   │   │                    #   FrameCodec, ServerLogger, main
│   │   │   └── db/              # Database, UserRepository, MessageRepository,
│   │   │                        #   ChannelRepository, LegacyMigrator
│   │   ├── include/             # Public headers (lanchat::server)
│   │   ├── config/              # Server config files
│   │   └── vendor/              # mini-asio, mini-json, mini-spdlog, sha256, sqlite
│   └── client/                  # Tauri v2 + React client
│       ├── src/                 # React UI
│       │   ├── components/      # App, ChatArea, GroupChatArea, ContactList,
│       │   │                    #   LoginPanel, RegisterPanel, ConnectionBar, Sidebar
│       │   ├── stores/          # chatStore, connectionStore, uiStore (Zustand)
│       │   ├── lib/             # 11 animation components + shared utils
│       │   └── styles/          # global.css
│       └── src-tauri/           # Rust backend
│           └── src/             # main, lib, commands, tcp_manager, message_codec,
│                                #   notifications
├── protocol/                    # Shared protocol truth source
│   ├── protocol_definitions.json  # 34 message types (source of truth)
│   ├── message_types.h          # C++ enum
│   ├── message_types.ts         # TypeScript const object
│   └── schemas/                 # 34 JSON Schema files
├── legacy/                      # Original Qt 5.4 project (read-only archive)
├── docs/                        # Design docs, research, checkpoints
├── scripts/                     # Build/verify automation, launch scripts
├── tests/                       # Integration & smoke tests
├── .claude/skills/              # Reusable AI agent skills
├── CMakeLists.txt               # Root build entry
├── CLAUDE.md                    # Server development guide
├── CODEX.md                     # Client development guide
└── DEVELOPMENT_PLAN.md          # Full development plan
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

See [`CODEX.md`](CODEX.md) for client conventions and [`CLAUDE.md`](CLAUDE.md) for server architecture.

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
ctest -C Debug --test-dir build/server-next-vs

# Protocol audit
node scripts/audit_protocol.mjs
```

---

## Contributing

This is a personal graduation project rewrite. Contributions, issues, and feature requests are welcome.

1. Read [`DEVELOPMENT_PLAN.md`](DEVELOPMENT_PLAN.md) for the full vision
2. Check existing issues and discussions
3. Open a PR against `master` with clear description

---

## License

MIT © 2024-2026 [Nagi-226](https://github.com/Nagi-226)

---

<p align="center">
  <sub>Built with ❤️ using AI · Graduation project redemption arc · <a href="https://github.com/Nagi-226/LanChat-Next">Star this repo</a> if you find it interesting</sub>
</p>
