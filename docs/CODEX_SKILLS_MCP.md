# Codex Skills And MCP Notes For LanChat-Next

Local source: `E:\AISkills`.

## Useful Local Skills

- `mattpocock_skills/skills/engineering/tdd`: use for protocol codec, C++ frame tests, and React component tests.
- `mattpocock_skills/skills/engineering/diagnose`: use for cross-stack failures across CMake/Vite/Cargo/Tauri.
- `mattpocock_skills/skills/engineering/grill-with-docs`: use to check implementation against `DEVELOPMENT_PLAN.md` before each `.5` checkpoint.
- `mattpocock_skills/skills/engineering/improve-codebase-architecture`: use before v1.2.0 service extraction.
- `Anthropic_skills/skills/webapp-testing`: use once the Tauri webview shell can run the React UI.
- `Anthropic_skills/skills/frontend-design`: useful for UI polish, but Cursor/Gemini currently owns that lane.
- `superpowers/skills/verification-before-completion`: use before marking v1.1.5 or later checkpoints complete.

## MCP Candidates

- `clangd` MCP bridge: introduce now for server-next navigation and diagnostics.
- SQLite MCP: use when the new SQLiteCpp data layer starts in v1.2.0.
- Playwright MCP: use when client-next has real pages and routes.
- GitNexus/code graph MCP: useful after service/routing modules appear.

## Policy

Do not let skills or MCP tools change the architecture source of truth. `DEVELOPMENT_PLAN.md`, `docs/MIGRATION_NOTES.md`, and `docs/V1_1_0_BLUEPRINT.md` remain the project baseline.
