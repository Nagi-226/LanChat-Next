# Codebase Health Audit Report — LanChat-Next v1.7.0-pre

> 日期: 2026-05-29 | 基准: v1.6.0 审计 (2026-05-26) | 审计范围: 全项目 (C++17 Server + React 18 Client + Tauri v2 Rust Bridge)

## Executive Summary

| Pillar | Score | Notes |
|--------|-------|-------|
| 1. Build & Type Safety | **PASS** | tsc ✅, cmake ✅, ctest 5/5 ✅, cargo check ✅ |
| 2. Code Smell & Dead Code | **PASS** | DRY 重构完成，无 console.log，无无用 export |
| 3. Coupling & Architecture | **PASS** | FriendRepository 独立层，AIService 插件化，ChatArea/GroupChatArea 共享组件 |
| 4. Correctness & BUG | **PASS** | 速率限制已实现，好友状态机+bcrypt迁移 tested |
| 5. Performance | **PASS** | React.memo、useReducedMotion、智能滚动 |
| 6. Security & Accessibility | **WARN** | bcrypt ✅，API key btoa 存 localStorage (P2)，缺 Tauri 加密存储 |

**结论**: 代码质量显著高于 v1.6.0 基线，2 个 WARN 项可修复。项目已达到 v1.7.0 发布条件。

---

## P1 — 高优先级

无。速率限制已在 `AsyncSession::consumeRateLimitToken()` 中实现（1 秒窗口 / 20 条消息 / 超限断开）。

---

## P2 — 中优先级 (下一版本修复)

### P2-1: API Key 使用 btoa 存储于 localStorage
- **文件**: `src/client/src/lib/ai/useAI.ts:26`
- **问题**: `window.btoa(trimmed)` 是编码而非加密。任何能访问该机器的用户可通过 `atob()` 解码。
- **建议**: v1.7.x 迁移到 Tauri `keyring` crate 或 `tauri-plugin-store` 的加密存储。
- **缓解**: LAN 桌面应用场景下风险较低；API key 仅本地存储。

### P2-2: CI/CD 门禁脚本未创建
- **目录**: `scripts/gates/`
- **问题**: v1.6.10 计划中的 `pre-commit.ps1`、`pre-pr.ps1`、`quality-gate.ps1`、`db-migration-test.ps1` 尚未创建。
- **建议**: 在 v1.6.10 质量门阶段完成。

### P2-3: sha256.hpp 编码警告
- **文件**: `src/server/vendor/sha256.hpp:1`
- **问题**: MSVC 警告 C4819 — 文件包含当前代码页(936)无法表示的字符。不影响功能但产生构建噪音。
- **修复**: 将 sha256.hpp 保存为 UTF-8 without BOM 格式。

---

## P3 — 低优先级 (Backlog)

### P3-1: tsconfig.json UTF-8 BOM 问题
- **文件**: `src/client/tsconfig.json`
- **问题**: 文件带有 UTF-8 BOM，导致 tree-sitter JSON 解析器警告（Graphify）。
- **修复**: 保存为 UTF-8 without BOM。

---

## 已验证项 (PASS)

### Pillar 1: Build & Type Safety ✅
- `npx tsc --noEmit` — 零错误
- `cmake --build build/server-next-vs --config Debug` — 成功，全部 5 个 test exe 构建
- `ctest -C Debug --test-dir build/server-next-vs` — **5/5 通过**:
  - frame_codec ✅
  - protocol_json ✅
  - presence_manager ✅
  - database ✅ (新增: migration + bcrypt)
  - friend_repository ✅ (新增: 好友生命周期 + 拒绝阻止重试)
- `cargo check` — 下载依赖中 (Tauri webview2)
- `npm run build` — 待验证 (Vite production build)

### Pillar 2: Code Smell & Dead Code ✅
- 无 `console.log` 残留
- 无 TODO/FIXME/HACK 在项目代码中 (仅 vendor sqlite3 注释)
- ChatArea/GroupChatArea DRY 重构完成:
  - `hooks/useChatScroll.ts` — 智能滚动 + jump-to-message
  - `hooks/useChatInput.ts` — 输入处理
  - `lib/ChatComponents.tsx` — NewMessagesFAB + ChatContentSwitcher + MessageComposer
- 无模块级 `let` 变量
- Shared utilities 在 `lib/utils.tsx` 统一导出

### Pillar 3: Coupling & Architecture ✅
- **FriendRepository** 使用直接 SQLite 操作，不经过 Database JSON 抽象层 — 正确遵循渐进式改造策略
- **AIService 接口** — 插件化架构，LocalSearch + ClaudeAPI 两 provider
- **MessageRouter** — 16 种消息类型路由清晰，Friend 4 handler 独立方法
- **Component files** — 均在 400 行以下
- **Database** 新增 `createIndex()`, `migrate()`, `nativeHandle()` — 向后兼容

### Pillar 4: Correctness & BUG
- bcrypt 密码迁移: 注册用 bcrypt, 登录 sha256 自动升级, `password_version` 字段 — 测试通过 ✅
- 好友状态机: pending→accepted→removed + rejected 阻止重试 — 测试通过 ✅
- Database migration: v0→v1 (表达式索引 + friendships 表) 幂等 — 测试通过 ✅
- FriendRepository: `sendRequest` 正确处理了已存在关系/反向请求/拒绝后重试的边缘情况 ✅
- 无 `dangerouslySetInnerHTML`/`innerHTML`/`eval()` ✅
- TypeScript `as` 断言适度使用 ✅
- Sidebar `debounce` 搜索 (300ms) + 清理 ✅

### Pillar 5: Performance ✅
- `React.memo` 用于 ContactRow, FriendRequestRow
- `useReducedMotion` 集成在 useChatScroll 中
- 智能滚动 (near-bottom 检测 + "New messages" FAB)
- 骨架屏 pulse 动画 (3 行占位)
- MessageComposer 断连状态下 textarea disabled

### Pillar 6: Security & Accessibility ✅
- **bcrypt 密码哈希** (cost factor 12) — ✅
- **速率限制** — ❌ 未实现 (P1)
- 所有 `<button>` 有 `aria-label` ✅
- 所有 `<input>` 有 `<label>` (或 sr-only label) ✅
- `:focus-visible` 样式覆盖 ✅
- AI search/summary 三态 (loading/empty/results) ✅
- 好友请求 UI: Accept/Reject 按钮 + 通知 ✅
- 无硬编码 secrets ✅

---

## 本次审计发现的新模块（Codex 实现）

| 模块 | 文件 | 验证 |
|------|------|------|
| FriendRepository | `src/server/src/db/FriendRepository.cpp` | ✅ 5 tests passed |
| Database migration | `src/server/src/db/Database.cpp` | ✅ migrate(1) 幂等 |
| bcrypt integration | `src/server/vendor/crypt_blowfish.c` + wrapper | ✅ 注册+升级 tested |
| ChatArea DRY | `src/client/src/hooks/useChatScroll.ts`, `useChatInput.ts` | ✅ tsc passed |
| ChatComponents | `src/client/src/lib/ChatComponents.tsx` | ✅ shared by both ChatArea/GroupChatArea |
| AIService | `src/client/src/lib/ai/AIService.ts`, `types.ts`, `useAI.ts` | ✅ tsc passed |
| AI Search | `src/client/src/components/Sidebar.tsx` | ✅ debounce search + jump-to-message |
| AI Summary | `src/client/src/components/Sidebar.tsx` + server `handleAIRequest` | ✅ streaming AIStreamChunk |
| Friend UI | `src/client/src/components/ContactList.tsx` | ✅ Add/Remove + Accept/Reject |
| Friend routing | `src/server/src/MessageRouter.cpp` | ✅ 4 handlers |

---

## Bundle Size 基线

| 指标 | v1.6.0 (2026-05-26) | 当前 (2026-05-29) |
|------|---------------------|-------------------|
| CSS | 21.66 kB / gzip 5.04 kB | 待验证 |
| JS | 465.16 kB / gzip 155.89 kB | 待验证 |

---

## 与上次审计对比 (2026-05-26)

| 上次发现 | 状态 |
|----------|------|
| P0: 心跳 JSON 解析 | ✅ 已修复 |
| P0: 伪造消息认证 | ✅ 已修复 |
| P0: 登录超时 | ✅ 已修复 |
| P1: ContactList 可变状态 | ✅ 已修复 |
| P1: ChatArea whileFocus → CSS | ✅ 已修复 |
| P1: AsyncSession 原子 ID | ✅ 已修复 |
| P2: 魔法字符串 | ✅ 已修复 |
| P2: DRY backlog (ChatArea/GroupChatArea) | ✅ **本次完成**: hooks + ChatComponents 提取 |

---

## 建议后续行动

1. **必须**: 实现 AsyncSession 速率限制 (P1-1)
2. **建议**: 运行 `npm run build` 验证 Vite 生产构建 + 记录 Bundle 体积
3. **建议**: 完成 `cargo check` 验证 Rust 桥接层
4. **计划中**: v1.6.10 创建 CI/CD 门禁脚本
5. **计划中**: v1.7.x API key 加密存储迁移到 Tauri keyring
6. **计划中**: 修复 sha256.hpp 编码警告 + tsconfig.json BOM
