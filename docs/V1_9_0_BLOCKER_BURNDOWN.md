# v1.8.1 → v1.9.0 Blocker Burndown Plan

> Date: 2026-05-30

This phase keeps feature work moving while preventing the known v2.0.0 blockers from being deferred until the end.

## Completed In This Slice

- Protocol expanded from 43 to 51 types: message edit/delete/reaction, read receipt, and protocol negotiation.
- Server `MessageRepository` now supports edit, soft delete, reaction append, and read marking with CTest coverage.
- Server router now handles message operations, read receipts, protocol hello, and file metadata delivery.
- Client exposes message edit/delete/reaction controls, read receipt sending, protocol hello on connect, and group create/join/leave controls.
- `ProtocolHello` now rejects incompatible version ranges instead of always returning success.
- `scripts/load_smoke.mjs` establishes the 50-user load baseline before the real-asio migration.

## v2.0.0 Blocker Timing

| Blocker | Target | Gate |
|---|---:|---|
| mini-asio → real asio | v2.0.0-a1 | Real Asio build + 500-client load smoke; 30-minute soak remains before final v2.0.0 |
| DB JSON document → normalized schema | v2.0.0-a4 | no full-table load on hot message paths |
| mini-spdlog / mini-json → official libs | v2.0.0-a2/a3 | structured logs + JSON schema validation pass |
| bcrypt blocking event loop → worker thread | v2.0.0-a5 | auth load test shows TCP loop remains responsive |
| E2E + protocol version negotiation | v1.9.0 | protocol smoke + 50-user load baseline scripts present |

## Guardrails

- Do not mix real asio with message feature work. Finish feature protocol semantics first, then swap transport behind the same router.
- Do not normalize DB and change message semantics in the same patch. Message edit/delete/read behavior is now covered before DB migration.
- Keep official library swaps isolated: logging first, JSON second, transport third.
- All v2.0.0 blocker patches must run `scripts/verify_v1_8_0.ps1` plus the new targeted stress/E2E script for that blocker.

## v2.0.0-a1 Result

- Server transport now builds against vendored standalone Asio via `lanchat/server/Net.h`.
- `asio_transport` CTest prevents regressing back to mini-asio on the server transport path.
- `scripts/verify_v2_0_0_a1.ps1` is the new real-asio gate and defaults to 500 clients / 50 messages.
