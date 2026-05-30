# v1.9.0 Checkpoint — Feature Completion Baseline

> Date: 2026-05-30

## Scope Delivered

- v1.8.1 Message edit: protocol type 43-44, server repository mutation, router broadcast, and client UI controls.
- v1.8.2 Message delete: soft-delete semantics, server/client state updates, and repository coverage.
- v1.8.3 Emoji reactions: protocol type 47-48, persisted reaction entries, and direct/group chat UI actions.
- v1.8.4 Group management UI: create, join, and leave controls are wired to the existing group protocol.
- v1.8.5 Read receipts + protocol negotiation: read markers use type 49, and `ProtocolHello` rejects incompatible version ranges.
- v1.8.6 File transfer baseline: `SendFile` metadata is relayed as `ReceiveFile` and acknowledged with `FileTransferDone`.
- v1.8.11 Gate baseline: protocol E2E smoke and 50-user load smoke scripts are now part of the v1.9.0 verification path.

## v2.0.0 Blocker Rhythm

| Blocker | Current v1.9.0 State | v2.0.0 Entry Gate |
|---|---|---|
| mini-asio -> real asio | Load script exists and captures the 50-user baseline only. | Replace transport in v2.0.0-a1, then pass 500 users / 30-minute soak. |
| DB JSON document -> normalized schema | Message semantics are covered before migration. | v2.0.0-a4 must remove full-table loads on hot message paths. |
| mini-spdlog / mini-json -> official libs | Protocol audit still protects generated schema alignment. | v2.0.0-a2/a3 should be isolated library swaps. |
| bcrypt blocking event loop -> worker thread | Auth load can now be exercised by `load_smoke.mjs`. | v2.0.0-a5 must prove TCP loop responsiveness under auth load. |
| E2E / protocol negotiation | Positive and incompatible `ProtocolHello` cases are covered. | Expand to Playwright desktop specs in v2.0.0-a7. |

## Verification

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_v1_9_0.ps1
```

Expected gate:
- Client Vitest, production build, protocol audit, and Tauri `cargo check`.
- Server Release build and CTest.
- Protocol E2E smoke, including incompatible protocol rejection.
- 50-client / 20-message load smoke baseline.

Latest local result: PASS on 2026-05-30.

Observed metrics:
- Vitest: 2 files / 5 tests passed.
- CTest: 6/6 tests passed.
- Load smoke: 50 clients / 20 messages / ~17.8s.
- Vite JS raw chunk warning remains at ~523 kB; gzip size is ~171.85 kB.

## Notes

- The 50-user load baseline is a pacing guard, not the v2.0.0 concurrency acceptance test.
- Keep real asio, normalized DB, official dependency swaps, and bcrypt offload in separate v2.0.0-a patches.
