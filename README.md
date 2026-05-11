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
  <a href="#"><img src="https://img.shields.io/badge/version-v1.1.0-blueviolet" alt="Version"></a>
  <a href="#"><img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B" alt="C++17"></a>
  <a href="#"><img src="https://img.shields.io/badge/Tauri-2.0-FFC131?logo=tauri" alt="Tauri"></a>
  <a href="#"><img src="https://img.shields.io/badge/React-18-61DAFB?logo=react" alt="React 18"></a>
  <a href="#"><img src="https://img.shields.io/badge/TypeScript-5.5-3178C6?logo=typescript" alt="TypeScript"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-Windows%2011-0078D6?logo=windows" alt="Windows 11"></a>
  <a href="#"><img src="https://img.shields.io/badge/status-active%20development-success" alt="Status"></a>
</p>

---

## What is LanChat-Next?

A complete rewrite of a graduation project LAN chat application, rebuilt from the ground up with modern technology and AI-assisted development. Designed for **small-to-medium enterprises (≤ 500 users)** — deploy on a single LAN server, no cloud dependency, no registration required beyond the local network.

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
│  │  │ Login    │ │ ChatView │ │ Friends  │ │ AI Panel│  │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────┘  │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Tauri Rust Bridge: TCP Manager · Frame Codec ·      │   │
│  │  Notifications · File System                         │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────┬───────────────────────────────────────┘
                       │  TCP + JSON (length-prefixed frames)
                       │  Port 12346
┌──────────────────────┴───────────────────────────────────────┐
│  LanChat-Next Server (C++17 + asio)                          │
│  ┌────────────┐ ┌──────────────┐ ┌──────────────────────┐   │
│  │ Connection │ │ Message      │ │ AI Scheduler         │   │
│  │ Pool (500) │ │ Router       │ │ · Summarize          │   │
│  │ · Heartbeat│ │ · P2P/Group  │ │ · Translate          │   │
│  │ · Timeout  │ │ · Offline Q  │ │ · Semantic Search    │   │
│  └────────────┘ └──────────────┘ └──────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Data Layer: SQLiteCpp (WAL) · FTS5 · Embeddings    │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────┘
```

---

## Tech Stack

| Layer | Technology | Purpose |
|-------|-----------|---------|
| **Server** | C++17 · asio · nlohmann/json · SQLiteCpp · bcrypt · spdlog | High-performance async message routing |
| **Client UI** | React 18 · TypeScript 5.5 · Tailwind CSS 3.4 · Zustand · Framer Motion | Modern responsive desktop UI |
| **Desktop Shell** | Tauri v2 · Rust · Tokio | Native window, notifications, file system |
| **Protocol** | TCP · JSON · 4-byte BE length-prefixed frames | 34 message types, typed schemas |
| **Database** | SQLite 3 (WAL mode) · FTS5 | User/Message/Channel/Embedding persistence |
| **AI** | DeepSeek-V4-Pro · Ollama (local fallback) · text-embedding-3-small | Chat summary, semantic search, translation |
| **Build** | CMake 3.28 · Vite 5 · Cargo | Cross-platform build orchestration |

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
cmake -S . -B build/server-next
cmake --build build/server-next --config Release

# 3. Start server (port 12346)
./build/server-next/Release/lanchat_server_next

# 4. Build client (separate terminal)
cd src/client
npm install
npm run tauri dev
```

> **Legacy Qt version**: The original Qt 5.4 graduation project is archived under `legacy/`. Run `build_legacy.bat` in a Qt 5.15 Developer Prompt if you want to see the original UI. Not required for development.

---

## Features Roadmap

| Version | Name | Key Deliverables | Status |
|---------|------|-----------------|--------|
| v1.0.5 | Legacy Seal | Frame protocol, channel persistence, offline messages, password hashing | ✅ Done |
| **v1.1.0** | **Blueprint** | **New C++ server + Tauri/React client skeleton · 34 protocol schemas** | ✅ **Current** |
| v1.2.0 | Server Core | Full asio async, FTS5 search, GTest coverage, WAL optimization | ⬜ Next |
| v1.3.0 | Client Base | Login/Register UI, Tauri window polish, clipboard support | ⬜ |
| v1.4.0 | Chat Core | Real-time messaging, virtual scrolling, file/image preview | ⬜ |
| v1.5.0 | Social | Friend list, channels, online presence, @mentions | ⬜ |
| v1.6.0 | UI Polish | Discord-inspired themes, animations, responsive layout | ⬜ |
| v1.7.0 | AI Agent | Chat summary, semantic search, translation, AI bot | ⬜ |
| v1.8.0 | Advanced | File transfer, admin panel, message read receipts | ⬜ |
| v1.9.0 | Hardening | Stress testing, CI/CD, security audit, installer packaging | ⬜ |
| **v2.0.0** | **Release** | **Zero Qt · Production-ready · .msi installer** | 🎯 |

> Full plan: [`DEVELOPMENT_PLAN.md`](DEVELOPMENT_PLAN.md)

---

## Project Structure

```
LanChat-Next/
├── src/
│   ├── server/              # C++17 asio server (new)
│   │   ├── CMakeLists.txt
│   │   ├── src/             # FrameCodec, TcpServer, LanChatServer
│   │   └── include/         # Public headers (lanchat::server)
│   └── client/              # Tauri v2 + React client (new)
│       ├── src/             # React UI (App, stores, lib)
│       └── src-tauri/       # Rust backend (TCP, commands)
├── protocol/                # Shared protocol truth source
│   ├── message_types.h      # C++ enum (34 types)
│   ├── message_types.ts     # TypeScript enum (aligned)
│   └── schemas/             # 34 JSON Schema files
├── legacy/                  # Original Qt 5.4 project (reference)
│   ├── original-client/     # Qt client (archived)
│   ├── original-server/     # Qt server (archived)
│   └── legacy-common/       # Shared frame protocol & logging
├── docs/                    # Design docs, API reference, checkpoints
├── tests/                   # Test scripts
├── CMakeLists.txt           # Root build entry
└── DEVELOPMENT_PLAN.md      # Full development plan (authoritative)
```

---

## Development

### AI-Assisted Workflow

This project is developed with a multi-AI-agent architecture:

| Agent | Model | Primary Role |
|-------|-------|-------------|
| **Claude Code** | DeepSeek-V4-Pro | Architecture design, C++ server, code review, documentation |
| **Codex** | GPT-5.5 | Code generation, React/TypeScript, Rust bridge, testing |
| **Gemini 3.1 Pro** | (via Cursor) | UI beautification, CSS/Tailwind theming |

### Quality Gates

Every milestone follows a `.0 → .3 → .4 → .5` progression with mandatory checkpoints before advancing. See [`DEVELOPMENT_PLAN.md`](DEVELOPMENT_PLAN.md) for the full iteration rules and acceptance criteria.

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
