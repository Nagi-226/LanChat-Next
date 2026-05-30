# Codebase Health Audit Report — v2.0.0-a1 (real asio)

> Date: 2026-05-30
> Scope: Full-stack (C++ Server + React Client + Rust Bridge + Protocol)
> Previous: v1.9.0 audit (`docs/audit-report-2026-05-30-v1.9.0.md`)

---

## Executive Summary

| Pillar | v1.9.0 | v2.0.0-a1 | Delta |
|--------|--------|-----------|-------|
| 1. Build & Type Safety | ✅ PASS | ✅ PASS | +1 CTest (7/7) |
| 2. Code Smell & Dead Code | ✅ PASS | ⚠️ WARN | mini-asio dead files remain |
| 3. Coupling & Architecture | ⚠️ WARN | ✅ PASS | chatStore split done, all files < 400L* |
| 4. Correctness & BUG | ✅ PASS | ✅ PASS | No regression |
| 5. Performance | ✅ PASS | ✅ PASS | 500-client smoke OK, auth still slow |
| 6. Security & Access | ✅ PASS | ✅ PASS | No regression |

\* messageStore.ts 715L with 30-case dispatcher — expected over 400L.

**Overall: 5/6 pillars PASS, 1 WARN (mini-asio dead files). v2.0.0-a1 milestone achieved.**

---

## v2.0.0-a1 Delivery: real asio Migration

### What Changed
- **Networking layer**: mini-asio (poll-based single-threaded) → standalone Asio (io_context + thread_pool)
- **New files**: `vendor/asio/include/asio.hpp`, `include/lanchat/server/Net.h`
- **New test**: `asio_transport_tests` (7th CTest, verifying asio transport layer)
- **TcpServer**: rewritten for `asio::io_context` with multi-threaded `run()`
- **AsyncSession**: rewritten for `asio::async_read`/`async_write` with strand synchronization
- **SessionPool/PresenceManager**: strand-safe mutex guards

### Verification
- ✅ 7/7 CTest passing (frame_codec, protocol_json, **asio_transport**, presence_manager, database, friend_repository, message_repository)
- ✅ 500-client smoke test passed
- ✅ `asio_transport` test: 0.25s (transport layer only)

### Orphaned Files (P2 — remove before v2.0.0 final)
| File | Status |
|------|--------|
| `src/server/vendor/mini_asio.hpp` | Dead — no includes found |
| `src/server/vendor/mini-asio/VERSION.txt` | Dead |

---

## Build & Test Verification

| Check | Result |
|-------|--------|
| `npx tsc --noEmit` | Zero errors ✅ |
| `cmake --build build/server-next-vs` | All 8 targets + 7 tests ✅ |
| `ctest` | 7/7 passing ✅ |
| `npx vite build` | 524.10 KB JS + 24.19 KB CSS ✅ |
| `cargo check` | v2.0.0-a1, clean ✅ |
| `node scripts/audit_protocol.mjs` | 51 schemas aligned ✅ |

### CTest Details
```
1/7 frame_codec ............... Passed (0.14s)
2/7 protocol_json ............. Passed (0.15s)
3/7 asio_transport ............ Passed (0.25s) ← NEW
4/7 presence_manager .......... Passed (0.15s)
5/7 database .................. Passed (1.30s)
6/7 friend_repository ......... Passed (1.30s)
7/7 message_repository ........ Passed (0.16s)
```

### Source Line Counts
| Layer | Lines |
|-------|-------|
| Server C++ | 2,476 |
| Client TypeScript/React | 5,421 |
| Tests (C++) | 731 |
| Total | 8,628 |

---

## Security Sweep

- ✅ Zero `dangerouslySetInnerHTML` / `innerHTML` / `eval()`
- ✅ Only 2 `console.*` calls (ErrorBoundary crash log + i18n missing key warning)
- ✅ No secrets/credentials in source code
- ✅ CSP enforced
- ✅ API keys in Tauri encrypted store
- ✅ bcrypt $2b$ cost 12
- ✅ SQL injection prevention (parameterized queries + identifier validation)
- ✅ Rate limiting: 20 msg/s per session
- ✅ Frame size limit: 256 KiB (3-layer enforcement)

---

## Remaining v2.0.0 Blockers (from Codex)

| # | Blocker | Priority | Status |
|---|---------|----------|--------|
| a2 | mini-spdlog → real spdlog | 🟠 HIGH | Deferred |
| a3 | mini-json → nlohmann/json | 🟡 MED | Deferred |
| a4 | DB JSON doc → normalized schema | 🔴 CRITICAL | Deferred |
| a5 | bcrypt offload to worker threads | 🟠 HIGH | Deferred |
| — | 30-min soak test | 🟠 HIGH | Deferred |
| — | Auth bootstrap optimization | 🟡 MED | Deferred |

---

## P1 Issues (Fix before v2.0.0)

| # | Issue | Fix |
|---|-------|-----|
| P1-1 | `vendor/mini_asio.hpp` + `vendor/mini-asio/` dead files | Remove |
| P1-2 | Auth bootstrap slow under 500-client load | bcrypt offload (v2.0.0-a5) |
| P1-3 | DB JSON-doc model loads full table into memory | Normalize schema (v2.0.0-a4) |

---

## Conclusion

v2.0.0-a1 delivers the #1 critical blocker — real asio replacing mini-asio. The 500-client smoke test confirms the multi-threaded transport layer works. 4 blockers remain (a2-a5) before final v2.0.0.
