# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> Last updated: 2026-05-29

## Project Identity

LanChat-Next is a LAN-based desktop instant messaging platform for small-to-medium enterprises (up to 500 users). Target: zero-cloud deployment, AI-assisted, Discord-quality UI. The project progressively replaces a legacy Qt 5.4 codebase with C++17 server + Tauri v2 / React 18 client.

- **Server port**: 12346 (new) / 12345 (legacy Qt, archived)
- **Target platform**: Windows 11 desktop
- **Version goal**: v2.0.0 = zero Qt dependencies

## Build & Verify Commands

```powershell
# Full checkpoint verification (protocol audit + CMake + CTest + Vite + Cargo)
./scripts/verify_v1_1_5.ps1

# Server only — configure, build, test
cmake -S . -B build/server-next-vs
cmake --build build/server-next-vs --config Debug
ctest -C Debug --test-dir build/server-next-vs

# Server only — single test
ctest -C Debug --test-dir build/server-next-vs -R frame_codec

# Client — type-check
cd src/client
npx tsc --noEmit

# Client — production build
cd src/client
npm run build

# Client — dev server (port 1420, strict)
cd src/client
npm run dev

# Client — Tauri desktop (requires GUI environment)
cd src/client
npm run tauri dev

# Cargo check (Rust bridge)
cd src/client
cargo check

# Protocol audit (34 schemas)
node scripts/generate_protocol.mjs --check
node scripts/audit_protocol.mjs

# Multi-client TCP smoke test
node tests/server_multi_client_smoke.mjs
```

## Tech Stack (Actual — not aspirational)

| Layer | Technology | Notes |
|-------|-----------|-------|
| Server language | C++17 | `std::string_view`, structured bindings |
| Server build | CMake 3.28+ | MSVC generator on Windows |
| Server networking | mini-asio (vendored) | `vendor/mini_asio.hpp` — to be replaced with real asio or formally accepted |
| Server JSON | mini-json (vendored) | `vendor/mini_json.hpp` — nlohmann/json wrapper |
| Server DB | SQLite + vendored wrapper | WAL mode, `vendor/sqlite/` |
| Server logging | mini-spdlog (vendored) | `vendor/mini_spdlog.hpp` |
| Server hashing | sha256 (vendored) | `vendor/sha256.hpp` — bcrypt planned for v1.2.x |
| Client framework | Tauri v2 | Rust backend + web frontend |
| Client UI | React 18 + TypeScript strict | Vite 5, port 1420 |
| Client styling | Tailwind CSS v3.4 | `class` dark mode, PostCSS |
| Client state | Zustand 4.5 | `persist` middleware on uiStore only |
| Client animation | React Bits components | Copied into `src/lib/`, adapted for Tailwind v3 |
| Protocol | JSON over TCP, 4-byte BE length prefix | 34 message types, JSON Schema in `protocol/schemas/` |
| Rust deps | tokio (net, io-util, sync, rt-multi-thread) | serde, serde_json |

## Directory Boundaries

```
src/server/          ← Claude Code domain (C++ server)
src/client/src/      ← Gemini/Codex domain (React UI), Claude can modify for integration
src/client/src-tauri/ ← Gemini/Codex domain (Rust bridge), Claude can review
protocol/            ← Shared — Claude owns message_types.h, Gemini owns message_types.ts
                       Claude changes first, then Gemini syncs .ts
legacy/              ← Read-only archive. NEVER modify. Qt 5.4 code, reference only.
docs/                ← Project documentation, checkpoints, plans
scripts/             ← Build/verify automation
tests/               ← Integration tests
```

## Architecture

### Data Flow (end-to-end)
```
React UI (user action)
  → chatStore.login/sendPrivateMessage (serialize JSON)
    → connectionStore.sendRawJson (Tauri invoke)
      → Rust commands.rs → TcpManager (encode frame, write to TCP stream)
        → C++ Server: TcpServer → AsyncSession → LanChatServer routing
          → SQLite persistence / P2P forward / group broadcast
        ← TCP response
      ← Rust read loop: emit("message-received", json)
    ← App.tsx Tauri event listener → chatStore.handleIncomingMessage
  ← React re-render (Zustand state change)
```

### Server Architecture
- `TcpServer` — accepts connections on port 12346, creates `AsyncSession` per client
- `AsyncSession` — owns per-session read accumulator (prevents cross-client pollution), tracks `write_offset_` for partial writes
- `LanChatServer` — routes messages: registration, login, private messages, group messages, heartbeat
- `Database` — SQLite WAL mode, schema creation, connection pool wrapper
- `UserRepository` — CRUD for users table, password verification (sha256)
- `LegacyMigrator` — imports users from legacy Qt server's user file format
- `FrameCodec` — 4-byte big-endian length prefix + JSON body
- `ServerLogger` — timestamped file logging

### Client Architecture
- `App.tsx` — root orchestrator with auth gate. Logged out → LoginPanel/RegisterPanel. Logged in → ContactList + ChatArea + Sidebar. Sets up Tauri event listeners for `message-received` and `connection-lost`.
- `chatStore` (Zustand) — auth state, currentUser, contacts[], messagesByContact (Record<number, ChatMessage[]>), groups[], actions: login/register/sendPrivateMessage/handleIncomingMessage/logout. Optimistic send for private messages (local push before TCP write).
- `connectionStore` (Zustand) — TCP connection lifecycle: status ('disconnected'|'connecting'|'connected'), host, port, heartbeat age. Exposes connect/disconnect/sendRawJson via Tauri invoke.
- `uiStore` (Zustand + persist) — theme ('dark'|'light'), sidebarCollapsed, aiPanelOpen. Persisted to localStorage key `lanchat-ui`.
- Rust `TcpManager` (Arc-wrapped, Tauri managed state) — splits TcpStream into read/write halves. Read loop decodes frames via `try_decode_frame` and emits Tauri events. Send path encodes via `encode_frame` and writes to the stored write half.

### Tailwind Design Tokens (defined in `tailwind.config.ts`)
Custom tokens use `dark-*` / `light-*` naming. Dark mode via `class` strategy — `<html class="dark">` toggled by uiStore.

| Token | Dark | Light |
|-------|------|-------|
| `*-bg` | `#1a1a2e` | `#ffffff` |
| `*-sidebar` | `#16213e` | `#f0f2f5` |
| `*-accent` | `#0f3460` | `#1677ff` |
| `*-highlight` | `#e94560` | `#1677ff` |
| `*-text` | `#e0e0e0` | `#1a1a1a` |
| `*-muted` | `#a0a0b0` | `#8c8c8c` |
| `*-border` | `rgba(255,255,255,0.08)` | `#e5e7eb` |
| `*-bubble-self` | `#0f3460` | `#1677ff` |
| `*-bubble-other` | `#252545` | `#f0f2f5` |

Custom widths: `w-sidebar` (240px), `w-panel` (320px). Custom radius: `rounded-bubble` (12px). Global transition: `300ms ease-in-out` on color/bg/border for all elements (defined in `global.css` on `*, *::before, *::after`).

### Protocol (34 Message Types)
Defined in `protocol/protocol_definitions.json` as source of truth. TypeScript types in `protocol/message_types.ts`, C++ header in `protocol/message_types.h`.

Key types: RegisterUser(0), Login(2), LoginSuccessReturn(3), SendMsg(5), ReceiveMsg(6), UserOnline(7), UserOffline(8), Heartbeat(20), HeartbeatAck(21), Logout(30).

JSON Schemas in `protocol/schemas/`. Generated/audited by `scripts/generate_protocol.mjs` and `scripts/audit_protocol.mjs`.

Frame format: `[4-byte BE uint32 length][JSON body]`. Max frame size: 4 MiB.

### Message Dispatch Pattern
`chatStore.handleIncomingMessage(raw)` is the single message router. It handles 5 types:
- LoginSuccessReturn(3) — sets currentUser, contacts, groups, resets auth
- ReceiveMsg(6) — pushes ChatMessage into messagesByContact[fromId]
- UserOnline(7) / UserOffline(8) — updates contact status
- Other valid types — silently ignored

HeartbeatAck(21) is handled by App.tsx directly (calls `connectionStore.updateHeartbeat()`), bypassing the dispatcher.

## Development Conventions

- **Version numbering**: MAJOR.MINOR.PATCH. `.5` is a quality checkpoint — must pass all acceptance criteria to advance to next major version. Failing → `.6/.7/.8` polish.
- **Progressive migration**: New tech modules coexist with legacy Qt code. Replaced Qt modules archived to `legacy/`. v2.0.0 removes the last Qt line.
- **Stale closure avoidance**: Components call `useXxxStore.getState()` directly in callbacks (not via hook selector).
- **Error handling**: Login/Register errors from `chatStore.auth.error` or component-local state. TCP errors from `connectionStore.error`.
- **No routing library**: App.tsx uses conditional rendering for auth gating.
- **Client component interfaces**: Each component exports its own TS interfaces (e.g., `ChatMessage` from ChatArea.tsx, `Contact` from ContactList.tsx). Stores import from component files.

## React Bits Integration

Animation components are copied from `E:\Open-Source Projects by others\react-bits\` into `src/client/src/lib/`. Each is a single self-contained `.tsx` file. NOT installed as npm packages.

### Adaptation Rules
- `motion/react` → `framer-motion` (React Bits targets React 19; LanChat uses React 18)
- Hardcoded dark colors → LanChat Tailwind tokens with `dark:` variants
- Tailwind v4 utilities → verify v3 compatibility (most standard classes are fine)
- Remove `'use client'` directives (Vite doesn't use React Server Components)
- Default colors: `sparkColor`→`'#e94560'`, brand gradient→`['#e94560','#1677ff','#52c41a']`

### Phase 1 (completed — v1.3.0) — Zero-dependency
| Component | File | Used In |
|-----------|------|---------|
| `ClickSpark` | `lib/ClickSpark.tsx` | ChatArea, GroupChatArea, LoginPanel, RegisterPanel, ConnectionBar (send/connect buttons) |
| `SpotlightCard` | `lib/SpotlightCard.tsx` | LoginPanel, RegisterPanel (form card hover effect) |
| `BorderGlow` | `lib/BorderGlow.tsx` | ContactList (active contact/group highlight) |

### Phase 2 (completed — v1.4.6) — Text + Counter + UI polish
gsap + framer-motion installed. Components delivered across 7 sub-versions: TextType (login/register titles), Counter (online/unread counts), GradientText (app name + splash), ShinyText (status labels), plus micro-interactions, skeleton screens, and reduced-motion support. See CODEX.md v1.4.0–v1.4.6 for details.

### Phase 3 (completed — v1.5.0) — List + Scroll
AnimatedList/AnimatedListItem (framer-motion useInView, ChatArea + GroupChatArea message entry), FadeContent (GSAP ScrollTrigger, ContactList fade-in), AnimatedContent (GSAP ScrollTrigger, App.tsx horizontal transition between chat views). No new npm deps.

### Phase 4 (completed — v1.6.0) — Polish
6 sub-versions, no new components, no new deps. v1.5.1 (AnimatePresence bubble exit + ContactList exit), v1.5.2 (animation param unification — durations/easing/scale), v1.5.3 (smart scroll + "New messages" button + ContactList scroll position), v1.5.4 (loading→content crossfade transitions), v1.5.5 (dead code + duplicate DateDivider extraction + export audit), v1.5.6 (pre-v1.6.0 quality gate: reduced-motion, theme consistency, a11y, profiler, bundle size, Tauri desktop). See CODEX.md for per-version task breakdown.

### Excluded from Integration
All `Backgrounds/*` (full-page WebGL/Canvas, inappropriate for chat window). All `@react-three/*` components (Three.js adds ~500KB). `FluidGlass`, `Lanyard`, `ModelViewer` (3D models, irrelevant to chat).

## Current Version State

- **Latest checkpoint**: v1.7.0 — AI Assistant Panel + Friend System + Security Hardening. 2026-05-29 full-stack audit: 6-pillar PASS (5/6 pillars PASS, 1 WARN). All 5 C++ tests passing. See `docs/audit-report-2026-05-29.md`.
- **Completed in this sprint**: DRY refactor (ChatArea/GroupChatArea shared hooks+components), connection hardening (reconnectTimer→store, max 5 retries), bcrypt password migration (sha256 auto-upgrade), rate limiting (20 msg/s per session), frame hardening (256 KiB limit), database indexes (9 expression indexes + migration framework), AI panel (search + summarize + AIService plugin arch), Friend system (protocol 34–42 + FriendRepository + 4 routing handlers + ContactList UI).
- **Server**: TcpServer + AsyncSession (rate-limited), Database (indexed + migration v1), UserRepository (bcrypt+sha256 auto-upgrade), FriendRepository (direct SQLite), SessionPool, PresenceManager, MessageRouter (16 handlers), MessageRepository, ChannelRepository.
- **Client**: ChatArea/GroupChatArea DRY (useChatScroll + useChatInput + ChatComponents), ContactList friend UI (Add/Remove/Accept/Reject), Sidebar AI panel (search + summarize + provider config), AIService plugin arch (LocalSearch + ClaudeAPI), connectionStore hardening.
- **Protocol**: 43 message types (0–42). v1.2.0 — 9 friend system types added (34–42).
- **Graphify**: 8,826 nodes / 30,036 edges / 523 communities knowledge graph, AST-only (100% extracted). Git hooks auto-update.
- **CI/CD**: 4-layer gate pipeline designed (L1 pre-commit / L2 pre-PR / L3 quality gate / L4 merge gate). Scripts pending at `scripts/gates/`.
- **Audit tooling**: `codebase-health-audit` skill at `.claude/skills/codebase-health-audit/SKILL.md`.
- **Still deferred**: real asio (mini-asio still vendored), real spdlog, heartbeat timeout sweep, longer load soak, Tauri keyring for API key storage.

## Key Constraints

- **Never modify `legacy/`** — archived reference code only.
- **Ports must not conflict** — new server 12346, legacy server 12345. Tauri dev on 1420.
- **Protocol types are auto-generated** — edit `protocol/protocol_definitions.json` first, then regenerate. C++ (.h) and TS (.ts) type enums must stay aligned.
- **Color changes** update both `tailwind.config.ts` tokens and the color adaptation table above.
- **Tauri capabilities** are minimal (`core:default` + `core:event:default`) — do not add permissions without explicit need.
- **No existing npm animation library** — do not introduce framer-motion or gsap before Phase 2 is approved.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
