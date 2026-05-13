# React Bits UI Sync

Date: 2026-05-13

Local reference:

- Source: `E:\Open-Source Projects by others\react-bits`
- License: MIT + Commons Clause. Allowed for use inside this application/product; do not resell, sublicense, or redistribute the extracted components as a standalone component bundle.
- Relevant variants: prefer TS + Tailwind or dependency-light TS components. Avoid importing the whole upstream project.

Current copied/adapted components:

- `src/client/src/lib/ClickSpark.tsx`
- `src/client/src/lib/SpotlightCard.tsx`
- `src/client/src/lib/BorderGlow.tsx`

Current usage:

- `LoginPanel` / `RegisterPanel`: `SpotlightCard` wrapper and `ClickSpark` submit feedback.
- `ChatArea` / `GroupChatArea`: `ClickSpark` send button feedback.
- `ConnectionBar`: `ClickSpark` connect button feedback.
- `ContactList`: `BorderGlow` active contact highlight.

Adoption timing:

- v1.2.x remains server-core first. Keep React Bits usage as already-integrated, low-risk client polish only.
- v1.3.x may reuse these components for auth/main-layout UI if the protocol smoke path needs visual feedback.
- v1.6.x is the main UI polish window for broader React Bits adoption.

Guardrails:

- Keep copied components local under `src/client/src/lib/`; do not add a runtime dependency on the upstream demo app.
- Prefer no-new-dependency components until the Tauri client baseline is stable.
- Preserve accessibility and keyboard behavior around wrapped buttons/forms.
- If a React Bits component needs heavy canvas/WebGL or layout effects, gate it behind the v1.6 UI polish track.
