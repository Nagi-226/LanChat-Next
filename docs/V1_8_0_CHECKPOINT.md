# v1.8.0 Checkpoint — Phase 0 Quality Hardening

> Date: 2026-05-30

## Scope Delivered

- v1.7.1 Error Boundary: React root is wrapped with a crash fallback so render failures do not blank the shell.
- v1.7.2 API key storage: AI keys moved from base64 localStorage to Tauri commands backed by Windows DPAPI.
- v1.7.3 Vitest: client unit-test infrastructure added with secure-storage and offline-queue coverage.
- v1.7.4 CSP: Tauri CSP is no longer `null`; the policy is restricted to app, IPC, assets, and DeepSeek API.
- v1.7.5 Typing indicator: client emits throttled typing events and server relays them via `SystemBroadcast` without new protocol IDs.
- v1.7.6 Offline queue: disconnected direct/group sends are queued in localStorage and flushed after reconnect.
- v1.7.7 Native notifications: incoming messages request and use WebView/OS notification support when permission is granted.
- v1.7.8 C++ hardening: Release/LTCG, optional ASan, optional warnings-as-errors, UTF-8 source mode, and vendor warning isolation.
- v1.7.9 CI/CD gates: `scripts/pre_pr_gate.ps1` and `scripts/verify_v1_8_0.ps1` added.
- v1.7.10 Quality gate: client tests/build/protocol, Cargo check, server Release build, and CTest passed locally.

## Verification

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_v1_8_0.ps1
```

Result: PASS

Notes:
- Vite reports JS chunk size `511.25 kB` raw / `169.60 kB` gzip. Gzip remains under the v2.0.0 budget.
- Visual Studio emits non-fatal `/UNDEBUG` and missing `pwsh.exe` messages during MSBuild, but all targets and CTest pass.

## Tagging

Create `v1.8.0` after committing this checkpoint so the tag points to an immutable reviewed commit.
