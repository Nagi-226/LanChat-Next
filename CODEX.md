# CODEX.md

LanChat-Next 客户端开发指南。Codex 负责 React 18 UI + Tauri v2 Rust 桥接层的实际开发，
Claude 负责 C++17 服务端架构设计。两端通过 `protocol/` 对齐。

> 最后更新: 2026-05-30 | 当前版本: v2.0.0-a1 | 下一目标: v2.0.0-a2/a3 spdlog + nlohmann/json → v2.0.0 正式发布
> 完整路线图: `docs/ROADMAP_v2.0.0.md`
> 竞品调研: `docs/agent-reach-research-v1.5.md` — 8 同类项目 + 5 Chat UI 组件库 + shadcn 官方 Blocks + 设计系统 + 色系对标 + Glassmorphism 库

## Codex 负责范围

```
src/client/src/           ← React UI (Codex 主战场)
src/client/src-tauri/     ← Rust 桥接 (Codex 维护，Claude review)
protocol/message_types.ts ← 协议类型 (Claude 改 .h 后 Codex 同步)
protocol/schemas/         ← JSON Schema (共享，只读)
```

**Codex 不应修改**：`src/server/`、`protocol/message_types.h`、`legacy/`、`docs/`（除非 UI 截图）。

## 构建 & 验证命令

```powershell
# 进入客户端目录
cd src/client

# 类型检查（每次改完先跑这个）
npx tsc --noEmit

# 生产构建
npm run build

# Vite 开发服务器 (port 1420)
npm run dev

# Tauri 桌面应用（需要 Windows GUI）
npm run tauri dev

# Rust 桥接检查
cargo check

# 协议对齐校验
npm run protocol:check

# real-asio 全量验证（包含 500 client load smoke）
powershell -ExecutionPolicy Bypass -File scripts/verify_v2_0_0_a1.ps1
```

## 技术栈（仅客户端）

| 层 | 技术 | 版本 |
|----|------|------|
| 框架 | Tauri | v2 |
| UI | React | 18.2 |
| 语言 | TypeScript | strict, 5.x |
| 构建 | Vite | 5.x |
| 样式 | Tailwind CSS | 3.4, class-based dark mode |
| 状态管理 | Zustand | 4.5 |
| Rust 异步 | tokio | 1.x (net, io-util, sync, rt-multi-thread) |
| 序列化 | serde + serde_json | 1.x |
| 动画 (Phase 1) | 纯 React (零依赖) | ✅ 已完成 |
| 动画 (Phase 2) | gsap + framer-motion | ✅ 已完成 |

## 版本路线图

```
已完成:
v1.3.x ✅ → v1.4.x ✅ → v1.5.x ✅ → v1.6.0 ✅ → v1.7.0 ✅
Phase 1-2     Phase 2       Phase 3-4    Phase 4 标签   AI+好友+安全+i18n

冲刺中 (28 子版本 → v2.0.0):
Phase 0: v1.7.1→v1.8.0 (10轮 质量加固) → 崩溃兜底/CSP/离线队列/CI/CD
Phase 1: v1.8.1→v1.9.0 (功能补全) → 编辑/删除/回应/群组/已读/文件/协议协商/50用户基线
Phase 2: v2.0.0-a1→v2.0.0 (11轮 基础设施) → real asio已完成/DB重构/bcrypt卸载/500人测试

详见: docs/ROADMAP_v2.0.0.md
```

---

### ✅ v1.3.0 — Phase 1 动画（已完成，随 v1.2.5 提交）

| 组件 | 源文件 | 使用位置 | 依赖 |
|------|--------|----------|------|
| ClickSpark | `lib/ClickSpark.tsx` | ChatArea, GroupChatArea, LoginPanel, RegisterPanel, ConnectionBar | 无 |
| SpotlightCard | `lib/SpotlightCard.tsx` | LoginPanel, RegisterPanel | 无 |
| BorderGlow | `lib/BorderGlow.tsx` | ContactList | 无 |

---

### ✅ v1.3.1 — 桌面 E2E 验证

**目标**：首次在 Tauri 桌面环境下跑通全流程，发现并修复原生环境特有的问题。

**背景**：当前所有开发和验证都在浏览器 dev server (port 1420) 下完成，`npm run tauri dev` 从未执行过完整的端到端测试。

**任务清单**：

| # | 任务 | 验证方法 |
|---|------|----------|
| 1 | 启动 C++ server (port 12346)，确保 server 已 build 且可运行 | `cmake --build build/server-next-vs --config Debug` |
| 2 | `npm run tauri dev` 启动桌面窗口 | 窗口正常显示，无白屏或崩溃 |
| 3 | 走通 注册 → 登录 → 连接 → 发私聊消息 → 接收回复 | 用 smoke test 脚本模拟另一客户端回复 |
| 4 | 验证主题切换（Dark ↔ Light） | 点击 Dark/Light 按钮，全局颜色即时切换 |
| 5 | 验证侧栏折叠/展开 + AI 面板开关 | ContactList 和 Sidebar 正确显示/隐藏 |
| 6 | 验证窗口拖拽区 (`data-tauri-drag-region`) | 拖拽标题栏可移动窗口 |
| 7 | 检查 Tauri 系统通知 (`notifications.rs`) | 收到新消息时 Windows 通知弹出 |
| 8 | 记录所有发现的问题 | 列出 bug/视觉差异/功能缺失，归入 v1.3.2 |

**涉及文件**：无预期修改，但可能波及任何组件/rust 文件（修复发现的问题）。

**完成标准**：`npm run tauri dev` 下 login → connect → send → receive → theme toggle 全流程无报错。

---

### ✅ v1.3.2 — 聊天 UX 打磨

**目标**：消除 ChatArea 和 GroupChatArea 之间的不一致，提升消息列表的可用性。

**背景**：两个聊天组件存在明显的代码重复和不一致——ChatArea 有 textarea auto-resize 但 GroupChatArea 没有；`fmtTime` 和 `getInitials` 在两个文件中各写了一遍；消息列表无日期分隔。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 新建 `lib/utils.ts`，抽取 `fmtTime`、`getInitials` | `lib/utils.ts` (new) |
| 2 | ChatArea 改用 `lib/utils.ts` 的共享函数 | `ChatArea.tsx` |
| 3 | GroupChatArea 改用共享函数 + 补充 textarea auto-resize 逻辑 | `GroupChatArea.tsx` |
| 4 | 消息列表加日期分隔线（"Today" / "Yesterday" / 日期） | `ChatArea.tsx`, `GroupChatArea.tsx` |
| 5 | 乐观发送的消息显示 "sending..." 指示器，收到服务端确认后转为正常状态 | `ChatArea.tsx`, `chatStore.ts` |
| 6 | 消息列表空态优化——区分 "未选联系人" vs "选了但无历史" | `App.tsx`, `ChatArea.tsx` |

**完成标准**：两个聊天组件功能对齐，`npx tsc --noEmit` + `npm run build` 通过。

---

### ✅ v1.3.3 — 联系人列表增强

**目标**：让 ContactList 从 "能看" 升级到 "好用"。

**背景**：当前 ContactList 无搜索、无排序、无右键菜单。联系人多了以后（50+）靠肉眼滚动查找不可接受。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 顶部加搜索/过滤输入框，支持按昵称或 ID 实时过滤 | `ContactList.tsx` |
| 2 | 排序规则：在线优先 → 有未读优先 → 按昵称字母序 | `ContactList.tsx` |
| 3 | 搜索无结果时的空态提示 | `ContactList.tsx` |
| 4 | lastMessage 预览截断优化——单行省略号，去掉 `No message preview yet` 占位文案 | `ContactList.tsx` |
| 5 | 预留 "typing..." 状态槽位（UI 占位，协议端暂不实现） | `ContactList.tsx`, `Contact` 接口 |
| 6 | `Contact` 接口新增 `typing?: boolean` 字段 | `ContactList.tsx` |
| 7 | 联系人在线状态指示器动画——在线脉冲呼吸效果 | `ContactList.tsx` |

**完成标准**：Contacts 列表支持即时搜索过滤、按状态排序，视觉上比 v1.3.2 更精致。

---

### ✅ v1.3.4 — 群聊对接

**目标**：把 GroupChatArea 真正接入应用，使其从 "死代码" 变成可用功能。

**背景**：GroupChatArea 组件已写好但 **App.tsx 从未渲染它**——用户登录后只能私聊，无法进入群组。服务端的群组消息路由（MessageRouter）和协议（sendGroupMsg/ReceiveGroupMsg/UserJoinGroup/UserLeaveGroup）均已就绪，纯属客户端未对接。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | chatStore 新增 `activeGroupId`、`messagesByGroup`、`selectGroup()` | `chatStore.ts` |
| 2 | `handleIncomingMessage` 新增 3 个 case：`ReceiveGroupMsg`(9)、`UserJoinGroup`(14)、`UserLeaveGroup`(15) | `chatStore.ts` |
| 3 | chatStore 新增 `sendGroupMessage(groupId, content)` action | `chatStore.ts` |
| 4 | ContactList 底部或独立区域渲染群组列表项（从 `chatStore.groups` 读取） | `ContactList.tsx` |
| 5 | App.tsx 根据 `activeGroupId` 决定渲染 ChatArea 还是 GroupChatArea | `App.tsx` |
| 6 | 群聊消息气泡显示发送者昵称（非本人时） | `GroupChatArea.tsx` |

**完成标准**：用户可点击群组进入群聊界面，发送和接收群消息，看到成员列表和加入/离开通知。

---

### ✅ v1.3.5 — 错误处理 & 连接体验

**目标**：用户不会因为网络抖动或操作失败而困惑。

**背景**：当前错误处理非常原始——Error 只是一行红色文字；断连后的重连没有 UI 提示；登录表单缺少基本的 UX 细节（密码可见切换、记住用户 ID）。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 新建 `lib/Toast.tsx` 轻量 toast 通知组件（3 种类型：info / error / success，自动消失） | `lib/Toast.tsx` (new) |
| 2 | App.tsx 顶层渲染 Toast 容器，监听 `connectionStore.error` 和 `chatStore.auth.error` 自动弹出 toast | `App.tsx` |
| 3 | 断连重连 banner——"Connection lost. Reconnecting in Xs..." 顶部横条，可手动关闭 | `App.tsx` 或新 `ReconnectBanner.tsx` |
| 4 | ConnectionBar 错误展示改进：可关闭的错误标签 + 图标 | `ConnectionBar.tsx` |
| 5 | ConnectionBar 记录最近 5 个 server 地址（localStorage），支持快速选择 | `ConnectionBar.tsx`, `connectionStore.ts` |
| 6 | LoginPanel 加密码可见切换按钮（眼睛图标） | `LoginPanel.tsx` |
| 7 | LoginPanel 记住上次登录的 User ID（localStorage） | `LoginPanel.tsx` |

**完成标准**：断连有明确提示和重连倒计时，错误以 toast 形式呈现，登录表单体验完整。

---

### ✅ v1.3.6 — 预 v1.4.0 质量审计

**目标**：在引入新依赖（gsap + framer-motion）之前，确保现有代码库整洁、类型安全、无性能隐患。

**背景**：v1.4.0 将引入两个重量级依赖，在此之前做一次全量代码审计，避免把债务带入下一阶段。

**任务清单**：

| # | 任务 | 具体做法 |
|---|------|----------|
| 1 | TypeScript strict 审计 | 检查所有 `as` 类型断言是否可替换为类型守卫；确保无 `any` 逃逸 |
| 2 | DRY 检查 | 搜索跨文件的重复代码（如多个 `getInitials` 残留），v1.3.2 之后理论上已消除 |
| 3 | 组件 Props 审计 | 检查每个组件的 interface 是否有未使用的 prop 或缺少应有的 prop |
| 4 | 性能检查 | 对 ContactList 列表项加 `React.memo`；对 ChatArea 消息气泡加 `React.memo` |
| 5 | 无障碍审计 | 所有 icon button 有 `aria-label`；tab 键导航顺序合理；输入框有关联 label |
| 6 | 暗色模式覆盖检查 | 逐个组件确认每个颜色 class 都有 `dark:` 变体，无硬编码颜色遗漏 |
| 7 | 删除未使用代码 | 清理 import 了但未使用的变量、未使用的 helper 函数 |

**涉及文件**：全部 `src/client/src/` 下的文件。

**完成标准**：`npx tsc --noEmit` 零错误，`npm run build` 通过，React DevTools 下无不必要的 re-render。

---

---

### ✅ v1.4.0 — Phase 2 动画依赖安装 + 4 组件集成

**目标**：安装 gsap + framer-motion，集成 TextType / Counter / GradientText / ShinyText。

**前置操作**：
```powershell
cd src/client
npm install gsap framer-motion
```

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | `npm install gsap framer-motion` | `package.json` |
| 2 | 从 react-bits 复制 `TextType.tsx` 到 `lib/`，按适配规则修改 | `lib/TextType.tsx` (new) |
| 3 | 从 react-bits 复制 `Counter.tsx` 到 `lib/`，按适配规则修改 | `lib/Counter.tsx` (new) |
| 4 | 从 react-bits 复制 `GradientText.tsx` 到 `lib/`，按适配规则修改 | `lib/GradientText.tsx` (new) |
| 5 | 从 react-bits 复制 `ShinyText.tsx` 到 `lib/`，按适配规则修改 | `lib/ShinyText.tsx` (new) |
| 6 | TextType 集成到 LoginPanel/RegisterPanel 标题 | `LoginPanel.tsx`, `RegisterPanel.tsx` |
| 7 | Counter 集成到 ContactList 在线人数 + 未读计数 | `ContactList.tsx` |
| 8 | GradientText 集成到 shell header "LanChat-Next" 应用名 | `App.tsx` |
| 9 | ShinyText 集成到 ConnectionBar 状态标签 | `ConnectionBar.tsx` |

**完成标准**：4 个 Phase 2 组件均可工作，`npx tsc --noEmit` + `npm run build` 通过。

---

### ✅ v1.4.1 — 动画桌面验证 + 包体积审计

**目标**：Phase 2 是项目首次引入外部动画库，需在 Tauri 桌面实际验证动画帧率与包体积。

**背景**：gsap (~30KB gzip) + framer-motion (~35KB gzip) 合计约 65KB。4 个新组件在桌面原生 WebView 中的表现需要实测确认。

**任务清单**：

| # | 任务 | 验证方法 |
|---|------|----------|
| 1 | 记录 `npm run build` 前后 JS bundle gzip 体积差 | 对比 build 产物大小 |
| 2 | `npm run tauri dev` 验证 4 个组件在桌面端实际渲染效果 | 肉眼检查 TextType/Counter/GradientText/ShinyText |
| 3 | Chrome DevTools Performance 面板录制典型操作（登录→选联系人→发消息→切换群组），确认动画帧率 >55fps | Performance 录制 |
| 4 | 确认与现有零依赖组件（ClickSpark/SpotlightCard/BorderGlow）无冲突 | 操作时观察无闪烁/卡顿 |
| 5 | 记录并修复发现的问题 | issue 列表 |

**涉及文件**：无预期修改（验证为主，按需修复）。

**完成标准**：包体积可接受，Tauri 桌面端动画流畅无 jank。

---

### ✅ v1.4.2 — UI 过渡动画

**目标**：用 framer-motion `AnimatePresence` 替代当前的 `hidden`/`block` 瞬间切换。

**背景**：当前侧栏折叠、AI 面板开关、主题切换都是 CSS `display` toggle，无过渡。联系人列表区域在 `sidebarCollapsed` 时直接消失。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 侧栏折叠/展开加 sliding 过渡（从侧边滑出/滑入） | `App.tsx`, `ContactList.tsx` |
| 2 | AI 面板开关加 sliding 过渡（从右侧滑入/滑出） | `Sidebar.tsx` |
| 3 | 主题切换加背景色渐变过渡（framer-motion `animate` 替换当前的瞬间 class toggle） | `App.tsx` |
| 4 | 联系人选中切换时列表项平滑高亮过渡（`layoutId` 或 `AnimatePresence`） | `ContactList.tsx` |

**完成标准**：侧栏/面板/主题切换有流畅过渡动画，不再瞬间消失/出现。

---

### ✅ v1.4.3 — Toast & 通知动画

**目标**：v1.3.5 的 Toast 和 ReconnectBanner 从瞬间出现升级为有进出场动画。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | Toast 进入动画（从右侧滑入 + 淡入，`initial`/`animate`/`exit`） | `Toast.tsx` |
| 2 | Toast 退出动画（向上滑出 + 淡出） | `Toast.tsx` |
| 3 | ReconnectBanner 进入/退出动画（从顶部展开/收起） | `App.tsx` |
| 4 | ConnectionBar 错误标签进出动画 | `ConnectionBar.tsx` |

**完成标准**：Toast/Banner/Error 标签均有进出场动画。

---

### ✅ v1.4.4 — 微交互打磨

**目标**：当前按钮 hover、输入框 focus 用 CSS `transition-colors`，升级为 framer-motion `whileHover`/`whileTap`，交互响应更细腻。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 发送按钮 hover/tap 缩放反馈（`whileHover={{ scale: 1.05 }}` `whileTap={{ scale: 0.95 }}`） | `ChatArea.tsx`, `GroupChatArea.tsx` |
| 2 | 登录/注册提交按钮 loading 状态过渡（按钮宽度/颜色平滑变化） | `LoginPanel.tsx`, `RegisterPanel.tsx` |
| 3 | 消息输入框 focus 时边框光晕动画（`motion.div` 包裹，focus 时 `boxShadow` 渐变） | `ChatArea.tsx`, `GroupChatArea.tsx` |
| 4 | 联系人 hover 时行背景色平滑变化（CSS transition 已可用，可选升级） | `ContactList.tsx` |

**完成标准**：核心交互（发送/登录/focus）有触觉反馈级的动画响应。

---

### ✅ v1.4.5 — 加载 & 骨架屏

**目标**：用 framer-motion 实现骨架屏 pulse 动画，覆盖消息列表、联系人列表、登录中的加载状态。

**背景**：当前加载状态只有文字（如 "Signing in..."），或直接显示空态。骨架屏可以在数据加载期间提供更好的感知性能。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 消息列表骨架屏（3 行灰色脉冲气泡，带头像占位圆） | `ChatArea.tsx` |
| 2 | 联系人列表骨架屏（5 行头像圆 + 文字条 pulse） | `ContactList.tsx` |
| 3 | 登录按钮 loading 视觉优化（spinner + "Signing in..." → motion 脉冲） | `LoginPanel.tsx` |
| 4 | 应用初始加载全屏 splash（"LanChat-Next" 文字 GradientText 淡入 + 底部 loading 指示器） | `App.tsx` |

**完成标准**：所有可加载区域在等待数据时显示骨架屏，非纯白屏或死文字。

---

### ✅ v1.4.6 — 预 v1.5.0 动画质量审计

**目标**：v1.5.0 将引入 GSAP ScrollTrigger（监听滚动事件），与已有 framer-motion 动画叠加可能导致性能问题。进入 Phase 3 前做一次动画专项审计。

**背景**：GSAP ScrollTrigger 会持续监听 `scroll` 事件，framer-motion 的 `layout` 动画依赖 `requestAnimationFrame`。两者叠加可能引发 jank 或竞态。

**任务清单**：

| # | 任务 | 具体做法 |
|---|------|----------|
| 1 | 性能审计 | DevTools 录一段完整操作流程，确认动画帧率 >55fps |
| 2 | 动画冲突检查 | 检查 framer-motion `layout` 动画 + GSAP 是否有竞态（同一元素同时被两个库接管） |
| 3 | `prefers-reduced-motion` 适配 | 在全局 CSS 或组件层面检测媒体查询，关闭所有非必要动画 |
| 4 | `will-change` 审计 | 确保仅对动画元素使用 `will-change`，避免内存膨胀 |
| 5 | TypeScript + Build | `npx tsc --noEmit` + `npm run build` 通过 |

**涉及文件**：全部 `src/client/src/` 下的文件（审计为主，按需修复）。

**完成标准**：动画帧率稳定，无不必要重绘，reduced-motion 用户可正常使用。

---

---

### ✅ v1.5.0 — Phase 3 列表+滚动动画

**目标**：集成 GSAP ScrollTrigger 驱动的列表入场动画和内容切换动画。

**背景**：Phase 3 的三个组件使用 GSAP ScrollTrigger 进行滚动驱动的动画触发，AnimatedList 使用 framer-motion `useInView` 实现轻量级列表项入场。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 从 react-bits 复制 `AnimatedList.tsx` 到 `lib/` | `lib/AnimatedList.tsx` (new) |
| 2 | 从 react-bits 复制 `FadeContent.tsx` 到 `lib/` | `lib/FadeContent.tsx` (new) |
| 3 | 从 react-bits 复制 `AnimatedContent.tsx` 到 `lib/` | `lib/AnimatedContent.tsx` (new) |
| 4 | AnimatedListItem → ChatArea + GroupChatArea 消息列表项滚动入场（fade-in + slide-up） | `ChatArea.tsx`, `GroupChatArea.tsx` |
| 5 | FadeContent → ContactList 联系人/群组项滚动淡入 | `ContactList.tsx` |
| 6 | AnimatedContent → App.tsx 聊天区内容切换（私聊/群聊/空态之间切换的水平滑动过渡） | `App.tsx` |

**代码质量修复**（验收时发现并修复）：
- 移除 `AnimatedList` 未使用的容器组件（死代码，-6行）
- 移除 `ConnectionBar.tsx` 无效 `statusClass`（死代码，-8行）
- 提取共享 `ButtonSpinner` 到 `lib/utils.tsx`（消除 LoginPanel/RegisterPanel 重复定义）
- 提取共享 `MessageSkeleton` 到 `lib/utils.tsx`（消除 ChatArea/GroupChatArea 重复定义）
- 移除 `AnimatedContent.tsx` 中重复的 `gsap.registerPlugin(ScrollTrigger)`（FadeContent 保留——import 顺序优先）
- `utils.ts` → `utils.tsx`（加入 JSX 组件后扩展名升级，净 -45 行）

**完成标准**：`npx tsc --noEmit` 零错误，`npm run dev` 浏览器验证通过，3 个 Phase 3 组件全部集成到位。

---

---

### ✅ v1.5.1 — 消息气泡 + 联系人 AnimatePresence 进出场

**目标**：消息气泡和联系人列表项在 mount/unmount 时有完整的进出场动画，消除快速切换时的"瞬间消失"。

**背景**：当前 `AnimatedListItem` 使用 framer-motion `useInView` 实现入场动画（fade-in + slide-up），但**没有任何退出动画**——切换联系人或搜索过滤时，旧消息/联系人瞬间消失。`DateDivider` 在两个 ChatArea 中各定义了一份完全相同的 memo 组件。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | ChatArea 消息列表用 `AnimatePresence mode="wait"` 包裹，`AnimatedListItem` 加 `exit={{ opacity: 0, y: -10 }}`（退出 0.12s，比入场快 45%） | `ChatArea.tsx` |
| 2 | GroupChatArea 同步 ChatArea 的 `AnimatePresence` + `exit` 处理 | `GroupChatArea.tsx` |
| 3 | `DateDivider` 随消息列表一同进出场（首次出现 fade-in，切换联系人时退出） | `ChatArea.tsx`, `GroupChatArea.tsx` |
| 4 | ContactList 联系人/群组 row 加 `exit` 动画（搜索过滤时匹配项平滑消失，不匹配项平滑出现） | `ContactList.tsx` |

**完成标准**：快速切换不同联系人 3-5 次，旧消息列表 fade-out + 新消息列表 fade-in，无瞬间消失/出现；搜索联系人时过滤结果平滑过渡。

---

### ✅ v1.5.2 — 动画参数全局统一

**目标**：统一全项目所有动画组件的 duration/ease/delay/scale 参数，消除节奏不一致感。

**背景**：Phase 1–3 的 9 个 lib 组件 + App.tsx/ChatArea/GroupChatArea/ContactList/ConnectionBar/LoginPanel/RegisterPanel/Sidebar 中散布着多种 duration、ease 和 scale 值。实测发现 Toast 0.24s 偏慢，LoginPanel 按钮 scale (1.02/0.98) 与 ChatArea 发送按钮 (1.05/0.95) 不一致。

**参考实现**：[nchat Animation System](https://github.com/nself-org/nchat/wiki/Animations) — 3 级 spring 预设 + 4 级 ease tokens 体系，是当前最完善的聊天应用 Framer Motion 动画规范。

**任务清单**：

| # | 范围 | 当前值 | 目标值 |
|---|------|--------|--------|
| 1 | 消息气泡入场 | duration 0.22s, delay `index*0.018` max 0.16s, easeOut | **保持**（已验证合适） |
| 2 | 消息气泡退出 | 无 | **0.12s easeIn**（退出比入场快 45%） |
| 3 | 联系人/群组 row | FadeContent duration 0.24s | **保持** |
| 4 | 侧栏/AI 面板滑入 | 0.24s easeOut | **保持** |
| 5 | 页面切换 (AnimatedContent) | 0.35s | **保持** |
| 6 | Toast 入场/退场 | 0.24s easeOut | **0.2s easeOut**（toast 应更快） |
| 7 | 按钮 hover/tap scale | LoginPanel 1.02/0.98, ChatArea 1.05/0.95 | **统一 1.05/0.95** |
| 8 | TextType typing speed | LoginPanel 42ms, RegisterPanel 38ms | **统一 40ms** |
| 9 | Splash exit | 0.35s | **保持** |
| 10 | ReconnectBanner enter/exit | 0.22s | **保持** |

**涉及文件**：`lib/Toast.tsx`, `LoginPanel.tsx`, `RegisterPanel.tsx`, `lib/TextType.tsx`

**完成标准**：快速走一遍 full flow（login → select → send → switch → toast），动画节奏一致，无突兀快慢切换。

---

### ✅ v1.5.3 — 智能滚动 + 新消息提示

**目标**：修复消息列表无条件滚到底部的问题，用户阅读历史时不被新消息打断。

**背景**：当前 `scrollIntoView({ behavior: 'smooth' })` 在 `messages` 变化时**无条件触发**——用户上滚看历史消息时，任何新消息到达都会强制拉回底部。此外 ContactList 滚动位置在切换联系人后丢失。

**参考实现**：[shadcn/ui Chat New Message Banner](https://www.shadcn.io/blocks/chat-new-message-banner) — 浮动未读计数 pill + scroll 检测 + framer-motion 入场动画，与 v1.5.3 需求完全匹配，可直接借鉴其 `useScroll` hook 和 `AnimatePresence` 模式。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 智能滚动检测：用 `useRef` 追踪消息容器 `scrollTop`，仅当用户距底部 < 80px 时才 `scrollIntoView` | `ChatArea.tsx` |
| 2 | "↓ New messages" 浮动按钮：用户不在底部且有新消息时，右下角显示半透明圆按钮（motion spring + Counter badge），点击 `scrollIntoView` | `ChatArea.tsx` |
| 3 | GroupChatArea 同步智能滚动 + ↓ 按钮 | `GroupChatArea.tsx` |
| 4 | ContactList 滚动位置保持：`useRef` 记住 `scrollTop`，从聊天返回列表时 `requestAnimationFrame` 恢复 | `ContactList.tsx` |

**完成标准**：上滚 200px 后收到新消息不会被拉回底部，出现 ↓ 按钮；点击 ↓ 平滑滚到底部；从聊天切回联系人不丢滚动位置。

---

### ✅ v1.5.4 — 加载态→内容态 crossfade 过渡

**目标**：骨架屏和真实内容之间的切换从"硬切"升级为平滑 crossfade。

**背景**：当前 ChatArea/GroupChatArea/ContactList 的骨架屏和真实内容通过 `loading` prop 条件渲染，切换是瞬间的。App.tsx splash 消失后主界面直接出现，无淡入。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | ChatArea：`AnimatePresence` 包裹 3 种状态（skeleton / 空态 / 消息列表），skeleton exit + 消息 enter 同时 crossfade | `ChatArea.tsx` |
| 2 | GroupChatArea：同步 ChatArea 的 3 态 `AnimatePresence` | `GroupChatArea.tsx` |
| 3 | ContactList：skeleton exit → 联系人列表 enter，用 `AnimatePresence mode="wait"` | `ContactList.tsx` |
| 4 | App.tsx splash exit 后，主界面首次渲染加 `motion.div initial={{ opacity: 0 }} animate={{ opacity: 1 }}` | `App.tsx` |

**完成标准**：登录成功后联系人列表从骨架屏平滑 crossfade 到真实列表；splash 消失后主界面淡入。

---

### ✅ v1.5.5 — DateDivider 去重 + 死代码清理 + 导出审计

**目标**：在打 v1.6.0 标签前做最后一轮代码瘦身。

**背景**：代码审查发现 `DateDivider` 组件在 ChatArea.tsx（L31-41）和 GroupChatArea.tsx（L29-39）中各有一份**完全相同的 memo 定义**（包括内嵌的 `fmtDateDivider` 调用和 Tailwind class）。此外 v1.5.0 已清理一轮（-45 行），但 v1.5.1–v1.5.4 改动后可能引入新冗余。

**任务清单**：

| # | 任务 | 具体做法 |
|---|------|----------|
| 1 | 提取共享 `DateDivider` 到 `lib/utils.tsx` | ChatArea + GroupChatArea 移除本地定义，统一从 utils 导入 |
| 2 | `ButtonSpinner` 返回类型 `ReactNode` → `JSX.Element` | 更精确的类型 |
| 3 | 未使用的 import 清理 | 全项目检查 |
| 4 | 未使用的 export 审计 | grep `export function/const/default` 验证有 import 方 |
| 5 | `as` 类型断言替换为类型守卫 | TypeScript strict 最终审计 |
| 6 | ChatArea/GroupChatArea 共享逻辑扫描 | 除 DateDivider 外还有哪些可提取 |

**涉及文件**：`lib/utils.tsx`, `ChatArea.tsx`, `GroupChatArea.tsx`，以及全部 `src/client/src/`

**完成标准**：零重复组件定义，零未使用 export，零未使用 import，零可消除 `as` 断言。

---

### ✅ v1.5.6 — 预 v1.6.0 质量门

**目标**：Phase 4 终点前的最终审计——动画全覆盖、无障碍、性能、构建、桌面验证。

**背景**：这是 v2.0.0 之前最后一个客户端动画里程碑的最后一步。

**任务清单**：

| # | 任务 | 具体做法 |
|---|------|----------|
| 1 | 全量 `prefers-reduced-motion` 覆盖审计 | 确认 9 个 lib/ 组件 + 所有 motion 元素都有降级路径 |
| 2 | 暗色/亮色主题动画一致性 | 两个主题下分别录一段操作，确认所有动画可见 |
| 3 | 无障碍检查（tab 导航/focus-visible/sr-only） | 全项目 aria 属性、键盘可操作性 |
| 4 | React DevTools Profiler | 录一段完整操作，确认无不必要 re-render |
| 5 | `npm run build` + bundle 体积记录 | 记录最终 JS/CSS 大小，作为 v1.6.0 基线 |
| 6 | `npm run tauri dev` 桌面端验证 | Tauri 原生窗口中动画效果 + 功能确认 |
| 7 | `npx tsc --noEmit` 零错误 | 最终类型检查 |

**涉及文件**：全部 `src/client/src/` + `src/client/src-tauri/`。

**完成标准**：所有指标达标，`npx tsc --noEmit` 零错误，Tauri 桌面端无已知问题，打 v1.6.0 标签。

---

### ✅ v1.6.0 — Phase 4 正式标签

v1.5.1–v1.5.6 全部完成后的汇总标签。标志着 React Bits Phase 1–4 动画集成全部完成，客户端达到 Discord-quality 动画水准。此标签后进入 v2.0.0 服务端加固轨道。

### 排除项（永久不做）

Backgrounds/* (全屏 WebGL/Canvas)，所有 `@react-three/*` 组件（Three.js ~500KB），FluidGlass, Lanyard, ModelViewer（3D 模型无关）。

---

## 客户端架构

### 组件树
```
App.tsx (根组件 — auth gate)
├─ [未登录]
│   ├─ LoginPanel     ← SpotlightCard 包裹表单
│   └─ RegisterPanel  ← SpotlightCard 包裹表单
├─ [已登录]
│   ├─ Sidebar        ← 用户信息 + AI 面板开关
│   ├─ ContactList    ← 联系人列表 + BorderGlow 高亮
│   ├─ ChatArea       ← 私聊面板 + ClickSpark
│   ├─ GroupChatArea  ← 群聊面板 + ClickSpark
│   └─ ConnectionBar  ← 连接控制 + ClickSpark
```

### 数据流
```
用户操作 (React UI)
  → chatStore.login/sendPrivateMessage (序列化 JSON)
    → connectionStore.sendRawJson (Tauri invoke)
      → Rust commands.rs → TcpManager (encode_frame, TCP write)
        → [C++ Server 处理]
      ← Rust read loop: emit("message-received", json)
    ← App.tsx Tauri event listener → chatStore.handleIncomingMessage
  ← React re-render (Zustand 状态变更)
```

### 三个 Zustand Store

| Store | 文件 | 职责 | 持久化 |
|-------|------|------|--------|
| `chatStore` | `stores/chatStore.ts` | 登录态、当前用户、联系人列表、消息(按contactId分组)、群组 | 否 |
| `connectionStore` | `stores/connectionStore.ts` | TCP 连接生命周期、host/port、心跳年龄、错误 | 否 |
| `uiStore` | `stores/uiStore.ts` | 主题(dark/light)、侧栏折叠、AI面板开关 | ✅ localStorage `lanchat-ui` |

### Rust 桥接层

| 文件 | 职责 |
|------|------|
| `main.rs` | Tauri 入口 |
| `lib.rs` | Tauri Plugin 注册，`TcpManager` 作为 managed state |
| `commands.rs` | Tauri commands: `connect`, `disconnect`, `send_raw_json` |
| `tcp_manager.rs` | TCP 连接管理：split 读写、异步 read loop、encode/decode frame |
| `message_codec.rs` | Frame 编解码：4-byte BE length prefix + JSON body |
| `notifications.rs` | Windows 系统通知 |

---

## 协议同步规则

```
protocol/protocol_definitions.json  ← 唯一真相源 (34 种消息类型)
        │
        ├── message_types.h          ← Claude 维护 (C++ enum)
        └── message_types.ts         ← Codex 维护 (TypeScript const object)
```

**工作流**：Claude 先改 `.h` → Codex 同步改 `.ts` → 运行 `npm run protocol:check` 验证对齐。

### 客户端需要处理的 5 种入站消息

`chatStore.handleIncomingMessage(raw)` 是唯一消息路由：

| type | 消息名 | 处理 |
|------|--------|------|
| 3 | LoginSuccessReturn | 设置 currentUser, contacts, groups，重置 auth |
| 6 | ReceiveMsg | 推入 messagesByContact[fromId] |
| 7 | UserOnline | 更新联系人状态为在线 |
| 8 | UserOffline | 更新联系人状态为离线 |
| 21 | HeartbeatAck | **由 App.tsx 直接处理**，调用 connectionStore.updateHeartbeat()，不走 dispatcher |
| 其他 | — | 静默忽略 |

---

## 开发约定

- **Stale closure 避免**：回调中用 `useXxxStore.getState()` 直接读取，不用 hook selector
- **无路由库**：App.tsx 纯条件渲染做 auth gate
- **接口定义**：每个组件文件自己 export TS 接口（如 `ChatMessage` 从 ChatArea.tsx，`Contact` 从 ContactList.tsx），store 从组件文件 import 类型
- **错误处理**：Login/Register 错误从 `chatStore.auth.error` 或组件 local state；TCP 错误从 `connectionStore.error`
- **Tauri capabilities 最小化**：仅 `core:default` + `core:event:default`，不加多余权限
- **不做的事**：不引入路由库、不自行安装 npm 动画库（按 Phase 计划来）、不改 legacy/

---

## 动画路线图（React Bits Phased Adoption）

源码位置：`E:\Open-Source Projects by others\react-bits\`（MIT + Commons Clause，复制到 `lib/` 使用，不整体引入）。

### 适配规则（所有 Phase 通用）

1. `motion/react` import → `framer-motion`
2. 硬编码暗色 → LanChat Tailwind token (`dark-*` / `light-*`)
3. Tailwind v4 工具类 → 确认 v3 兼容
4. 删除 `'use client'` 指令
5. 默认颜色：sparkColor → `'#e94560'`，渐变 → `['#e94560','#1677ff','#52c41a']`

### Phase 1 — v1.3.0 ✅ 已完成

ClickSpark, SpotlightCard, BorderGlow（零依赖，纯 React + CSS）。

### Phase 2 — v1.4.x ✅ 已完成

**已安装**：`gsap` + `framer-motion`。7 个子版本 (v1.4.0–v1.4.6) 全部交付：
4 组件集成 → 桌面验证 → UI 过渡 → Toast 动画 → 微交互 → 骨架屏 → 动画审计。

| 组件 | 源文件 | 使用位置 |
|------|--------|----------|
| TextType | `lib/TextType.tsx` | LoginPanel, RegisterPanel（标题打字动画） |
| Counter | `lib/Counter.tsx` | ContactList（在线人数、未读计数） |
| GradientText | `lib/GradientText.tsx` | App.tsx shell header + splash |
| ShinyText | `lib/ShinyText.tsx` | ConnectionBar（Connected/Connecting 状态） |

### Phase 3 — v1.5.0 ✅ 已完成

AnimatedList（framer-motion `useInView`, ChatArea + GroupChatArea 消息入场），
FadeContent（GSAP ScrollTrigger, ContactList 联系人/群组滚动淡入），
AnimatedContent（GSAP ScrollTrigger, App.tsx 聊天区水平滑动切换）。
共 -45 行代码质量修复（死代码 + DRY 提取 + 重复注册移除）。

### Phase 4 — v1.5.1–v1.6.0 ✅ 已完成

6 个子版本 (v1.5.1–v1.5.6) + v1.6.0 正式标签：
气泡 AnimatePresence → 参数调优 → 滚动打磨 → 加载过渡 → 死代码清理 → 最终审计 → Phase 4 打标。
无新组件，无新依赖。详见上方版本章节。

---

## 当前执行：v1.7.0 正式标签 ✅

> v1.3.x ✅ + v1.4.x ✅ + v1.5.0 ✅ + v1.5.1–v1.5.6 ✅ + v1.6.0 ✅ + v1.6.1–v1.6.10 ✅ + v1.7.0 ✅ 全部完成。
> **2026-05-26** 全项目深度审计完成：发现 3 P0 + 4 P1 + 3 P2，已全部修复。
> **2026-05-29** 重新规划→Codex 全量实现→审计验证通过。设计文档：`docs/superpowers/specs/2026-05-29-v1.6.0-v1.7.0-redesign.md`，审计报告：`docs/audit-report-2026-05-29.md`。

### 架构决策（2026-05-29）

| 决策 | 选项 | 理由 |
|------|------|------|
| 数据库改造 | 渐进式（旧表加 JSON 表达式索引 + 新表规范化） | 匹配项目渐进迁移哲学，500 用户 LAN 下性能足够 |
| 好友系统 | 双向确认制（请求→接受/拒绝→互为好友） | 标准 IM 语义 |
| AI 面板首期 | 搜索 MVP + 对话摘要，AIService 抽象接口预留扩展 | 服务端搜索已就绪，LLM 通过接口解耦 |
| 密码哈希 | 本迭代迁移 bcrypt，旧 sha256 自动升级 | 开发阶段用户少迁移成本低 |
| CI/CD | 4 层门禁（L1 pre-commit / L2 pre-PR / L3 质量门 / L4 merge gate） | 企业级质量保障 |

### 版本路线图 (v1.6.0 → v1.7.0)

```
v1.6.0 ✅ (当前)   Phase 1-4 动画完成
    │
v1.6.1   客户端 DRY + 代码质量           (ChatArea/GroupChatArea 重构)
v1.6.2   连接状态 & Store 加固            (connectionStore + 最大重试)
v1.6.3   C++ Server 安全加固              (bcrypt + 速率限制 + 帧校验)
v1.6.4   UI 边界状态全覆盖                 (断连/空态/错误/loading)
v1.6.5   数据库渐进式改造 ★               (索引 + migration + friendships 表)
v1.6.6   AI 面板 — 搜索 MVP               (Sidebar 对接已有 search 接口)
v1.6.7   AI 面板 — 对话摘要               (LLM 集成 + AIService 抽象)
v1.6.8   好友系统 — 协议 + 数据库          (protocol 34-42 + FriendRepository)
v1.6.9   好友系统 — 服务端 + 客户端        (路由 + UI)
v1.6.10  预 v1.7.0 质量门                  (L3 全栈审计 + 集成测试)
    │
v1.7.0   正式标签                          (AI 面板 + 好友系统 + 安全加固)
```

v1.6.0 审计基线（2026-05-26）：

1. `npx tsc --noEmit` ✅ 零错误
2. `npm run build` ✅ 通过；CSS 21.66 kB / gzip 5.04 kB，JS 465.16 kB / gzip 155.89 kB
3. `cargo check` ✅ 通过
4. C++ Server CMake build ✅ 待重新验证
5. 审计 P0-P2 修复：8 项已修复，1 项（DRY）记录到 v1.6.1
6. `codebase-health-audit` skill ✅ 已创建 (`.claude/skills/codebase-health-audit/SKILL.md`)

---

### v1.6.1 — ChatArea/GroupChatArea DRY 重构

**目标**：消除两个聊天组件 ~80% 代码重复（~150 行）。

**重复项清单**：
| 逻辑 | 提取方案 |
|------|----------|
| `scrollToBottom` + `syncNearBottom` + "New messages" 状态 | `hooks/useChatScroll.ts` |
| `handleSend` + `handleInput` + `handleKeyDown` + auto-resize | `hooks/useChatInput.ts` |
| "↓ New messages" 浮动按钮 | `lib/ChatComponents.tsx` → `NewMessagesFAB` |
| `AnimatePresence` 三态切换 (skeleton/empty/messages) | `lib/ChatComponents.tsx` → `ChatContentSwitcher` |
| 消息输入区 (textarea + send button) | `lib/ChatComponents.tsx` → `MessageComposer` |

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 新建 `hooks/useChatScroll.ts` | new |
| 2 | 新建 `hooks/useChatInput.ts` | new |
| 3 | 提取 `NewMessagesFAB`, `ChatContentSwitcher`, `MessageComposer` 到 `lib/ChatComponents.tsx` | new |
| 4 | ChatArea 改用提取的 hooks + components | `ChatArea.tsx` |
| 5 | GroupChatArea 改用提取的 hooks + components | `GroupChatArea.tsx` |
| 6 | 验证两组件功能无回归 | 手动测试 |

**完成标准**：ChatArea + GroupChatArea 合计减少 ≥120 行，`npx tsc --noEmit` 零错误，两个聊天面板功能完全一致。

---

### v1.6.2 — 连接状态 & Store 加固

**目标**：消除 connectionStore 的模块级 `reconnectTimer`，加强连接生命周期管理。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | `reconnectTimer` 从模块级移入 store state（`number \| null`） | `connectionStore.ts` |
| 2 | 最大重连次数限制 5 次，超限停止并设 error | `connectionStore.ts` |
| 3 | `disconnect()` 清理 `reconnectTimer` | `connectionStore.ts` |
| 4 | 连接成功重置 `retryCount` 为 0 | `connectionStore.ts` |
| 5 | ConnectionBar 显示重连次数/状态 | `ConnectionBar.tsx` |

**完成标准**：store 内无模块级变量引用，自动重连最多 5 次后停止。

---

### v1.6.3 — C++ Server 安全加固

**目标**：基础安全措施补齐 + bcrypt 密码迁移。

#### bcrypt 密码迁移
- vendor: `vendor/bcrypt.h`（BSD 许可，单头文件）
- `UserRepository::create()` 改用 bcrypt，cost factor=12
- `UserRepository::verifyPassword()` 查 `password_version` 字段（0=sha256, 1=bcrypt），sha256 验证成功后自动升级
- 用户 Row 新增 `password_version` 字段
- 现存用户零感知：下次登录时自动完成升级

#### 速率限制
- `AsyncSession` 每 session 每秒最多 20 条消息，超限断开

#### 帧大小 + 端口校验
- 帧大小上限 4 MiB → 256 KiB
- 无效帧丢弃 + 日志，不返回伪 JSON（防止信息泄露）
- `main.cpp` 端口参数校验（拒绝 0、>65535、<1024 警告）

**改动范围**：`vendor/bcrypt.h`(new), `UserRepository.h/cpp`, `AsyncSession.h/cpp`, `FrameCodec.h/cpp`, `main.cpp`, `frame_codec_tests.cpp`

**完成标准**：`cmake --build` + `ctest` 通过，bcrypt 迁移幂等。

---

### v1.6.4 — UI 边界状态全覆盖

**目标**：所有组件在 loading / empty / error / disconnected 四种边界状态有合理 UI。

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | 断连状态：发送按钮 disabled + tooltip "Not connected" | `ChatArea.tsx`, `GroupChatArea.tsx` |
| 2 | 断连状态：输入框 disabled + placeholder "Disconnected..." | `ChatArea.tsx`, `GroupChatArea.tsx` |
| 3 | 群聊成员列表空态（"No members" 占位） | `GroupChatArea.tsx` |
| 4 | 联系人全部离线时的视觉提示 | `ContactList.tsx` |
| 5 | 群组列表为空时的引导文案 | `ContactList.tsx` |
| 6 | 消息发送失败后的重试按钮 | `ChatArea.tsx`, `GroupChatArea.tsx` |

**完成标准**：遍历 4 种边界状态 × 关键组件，每组合有合理 UI。

---

### v1.6.5 — 数据库渐进式改造 ★ 核心基础设施

**目标**：为现有表添加 SQLite 表达式索引（对 Repository 透明），建立 migration 框架，创建规范化的 friendships 表。

#### Database 类新增

```cpp
void createIndex(const std::string& table, const std::string& indexName, const std::string& expression);
bool migrate(int targetVersion);
```

#### 表达式索引（Repository 无需修改）

```sql
CREATE INDEX idx_users_id ON users(json_extract(data, '$.id'));
CREATE INDEX idx_users_nickname ON users(json_extract(data, '$.nickname'));
CREATE INDEX idx_messages_from_id ON messages(json_extract(data, '$.from_id'));
CREATE INDEX idx_messages_to_id ON messages(json_extract(data, '$.to_id'));
CREATE INDEX idx_messages_group_id ON messages(json_extract(data, '$.group_id'));
CREATE INDEX idx_messages_timestamp ON messages(json_extract(data, '$.timestamp'));
CREATE INDEX idx_groups_group_id ON groups(json_extract(data, '$.group_id'));
CREATE INDEX idx_gm_user_id ON group_members(json_extract(data, '$.user_id'));
CREATE INDEX idx_gm_group_id ON group_members(json_extract(data, '$.group_id'));
```

#### Migration 框架

基于已有 `schema_version` 表，顺序迁移链：v0→v1（添加表达式索引 + 创建 friendships 表）。

#### 规范化 friendships 表

```sql
CREATE TABLE friendships (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    from_uid INTEGER NOT NULL, to_uid INTEGER NOT NULL,
    status TEXT NOT NULL CHECK(status IN ('pending','accepted','rejected')),
    request_msg TEXT DEFAULT '',
    created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL,
    UNIQUE(from_uid, to_uid)
);
CREATE INDEX idx_fs_from ON friendships(from_uid, status);
CREATE INDEX idx_fs_to ON friendships(to_uid, status);
```

**改动范围**：`Database.h/cpp`, `database_tests.cpp`(new)

**完成标准**：migrate(1) 幂等，已有 Repository 查询全部通过，索引对上层透明。

---

### v1.6.6 — AI 面板：搜索 MVP

**目标**：Sidebar 对接服务端已有的 AI 消息搜索接口（`MessageRouter::handleAIRequest`，`ai_type="search"`）。

**任务清单**：

| # | 任务 | 涉及文件 |
|---|------|----------|
| 1 | chatStore 新增 `searchResults[]`, `isSearching`, `searchMessages(query)` action | `chatStore.ts` |
| 2 | `handleIncomingMessage` 新增 case `AIResponse`(30) | `chatStore.ts` |
| 3 | Sidebar 搜索 UI：输入框（300ms debounce）+ 结果列表 + 分类过滤 [全部/私聊/群聊] | `Sidebar.tsx` |
| 4 | 搜索结果关键词高亮 | `Sidebar.tsx` |
| 5 | ChatArea 新增 `scrollToMessage(msgId)` 方法 | `ChatArea.tsx` |
| 6 | 点击搜索结果跳转到对应消息位置 | `Sidebar.tsx` |
| 7 | 三态：loading（骨架脉冲）/ empty（"No results"）/ results | `Sidebar.tsx` |

**完成标准**：搜索关键词返回匹配结果，点击跳转到正确消息。

---

### v1.6.7 — AI 面板：对话摘要 + AIService 抽象

**目标**：实现对话摘要功能（AI summarize），建立可扩展的 AIService 插件架构为后续 translate/chat/Ollama 预留接口。

#### AIService 抽象

```
lib/ai/
├── AIService.ts         ← 抽象接口
├── providers/
│   ├── LocalSearch.ts   ← v1.6.6 已有（ai_type="search"）
│   └── ClaudeAPI.ts     ← v1.6.7 新增
├── useAI.ts             ← AI hook（provider 选择 + 状态管理）
└── types.ts             ← AIRequestType, AIProvider config
```

#### 接口定义

```typescript
interface AIService {
  readonly id: string;
  readonly name: string;
  readonly capabilities: AIRequestType[];
  execute(request: AIRequest): Promise<AIResponse>;
  executeStream?(request: AIRequest, onChunk: (chunk: AIStreamChunk) => void): Promise<void>;
}
```

#### ClaudeAPI Provider
- 通过服务端中转（客户端不直接调 Anthropic API）
- 服务端新增 `ai_type="summarize"` 处理：收集最近 50 条消息 → 调 Claude API → 流式 `AIStreamChunk`
- API key 存储：Tauri 端加密（Rust keyring crate 或 AES-GCM）
- 协议 `AIStreamChunk(31)` 已就绪，天然支持 SSE → frame 转换

#### Sidebar 扩展
- 模式切换 [搜索] [摘要] tabs
- 摘要模式：选择对话 → 点击"生成摘要" → 流式显示
- AI 设置区：Provider 选择 / Model / API Key 管理

**完成标准**：选择对话→生成摘要→流式显示，provider 可切换。

---

### v1.6.8 — 好友系统：协议 + 数据库设计

**目标**：定义好友协议（9 个消息类型 34–42），实现 FriendRepository。

#### 协议新增

```
FriendRequest(34)       A → B 发送好友请求
FriendRequestAck(35)    服务端确认收到请求
FriendAccept(36)        B 接受好友请求
FriendAcceptReturn(37)  返回好友信息给双方
FriendRemove(38)        解除好友关系
FriendRemoveReturn(39)  确认解除
FriendList(40)          请求好友列表
FriendListReturn(41)    返回好友列表
FriendOnline(42)        好友上线通知
```

#### 好友状态机

```
A 发送请求 → [pending] → B 接受 → [accepted] → 互为好友
                  ↓ B 拒绝
              [rejected] → 30 天后清理
[accepted] → A 或 B 删除 → 行立即删除
```

#### FriendRepository（直接操作 sqlite3，不经过 Database JSON 抽象层）

```cpp
class FriendRepository {
    int64_t sendRequest(int fromUid, int toUid, const std::string& msg);
    bool acceptRequest(int64_t requestId, int responderUid);
    bool rejectRequest(int64_t requestId, int responderUid);
    bool removeFriendship(int uidA, int uidB);
    std::vector<UserInfo> getFriends(int uid);
    std::vector<FriendRequest> pendingRequests(int uid);
    bool areFriends(int uidA, int uidB);
};
```

**改动范围**：`protocol/protocol_definitions.json`, `message_types.h`, `message_types.ts`, `protocol/schemas/`(regenerate), `FriendRepository.h/cpp`(new)

**完成标准**：协议类型对齐校验通过，DDL 在空库执行成功。

---

### v1.6.9 — 好友系统：服务端 + 客户端实现

**目标**：端到端好友功能链路。

#### 服务端
- `MessageRouter` 新增 4 个 handler：`handleFriendRequest`, `handleFriendAccept`, `handleFriendRemove`, `handleFriendList`
- `broadcastPresence()` 扩展：上线/离线通知仅广播给好友
- 私聊权限：可选开启"仅好友可私聊"

#### 客户端

| 层面 | 改动 |
|------|------|
| chatStore | 新增 `friends[]`, `friendRequests[]`, `sendFriendRequest()`, `respondToRequest()`, `removeFriend()` |
| handleIncomingMessage | 新增 5 个 case（FriendRequestAck/FriendAcceptReturn/FriendRemoveReturn/FriendListReturn/FriendOnline） |
| ContactList | 右键菜单 "Add Friend"；好友列表分组（在线/离线/非好友联系人） |
| App.tsx | 好友请求 toast 通知 + pending badge |
| GroupChatArea | 可选：群成员右键 "Add Friend" |

**改动范围**：`MessageRouter.h/cpp`, `chatStore.ts`, `ContactList.tsx`, `App.tsx`, `GroupChatArea.tsx`

**完成标准**：A 发送好友请求 → B 收到通知 → B 接受 → 双方好友列表出现对方 → 好友上线通知。

---

### v1.6.10 — 预 v1.7.0 质量门

**目标**：L3 全栈审计 + 集成测试，v1.7.0 标签前的最终验证。

| # | 任务 | 方法 |
|---|------|------|
| 1 | 全栈审计 | `codebase-health-audit` 6-pillar |
| 2 | 构建三连 | `npx tsc --noEmit` + `npm run build` + `cargo check` |
| 3 | C++ 构建+测试 | `cmake --build` + `ctest` |
| 4 | 协议对齐 | `node scripts/audit_protocol.mjs` |
| 5 | 多客户端压力 | `node tests/server_multi_client_smoke.mjs` |
| 6 | Tauri 桌面 E2E | login→send→AI search→friend→theme toggle |
| 7 | React Profiler | 帧率 > 55fps |
| 8 | Bundle 体积 | 与 v1.6.0 基线对比 |
| 9 | CHANGELOG | 所有 1.6.x 版本变更记录 |

**完成标准**：所有指标达标，打 v1.7.0 标签。

---

## CI/CD 门禁流水线

### L1: Pre-commit（秒级）
`clang-format --dry-run` | `prettier --check` | `node scripts/generate_protocol.mjs --check` | `grep` 无 console.log/TODO/FIXME

### L2: Pre-PR（分钟级）
`npx tsc --noEmit` + `npm run build` + `cargo check` + `cmake --build` + `ctest` + `node scripts/audit_protocol.mjs` + bundle ±5%

### L3: 版本质量门（每 0.1 版本）
codebase-health-audit + Tauri 冒烟 + 压力测试 + 性能回归 + CHANGELOG

### L4: Merge Gate
L2 + Code Review + 无冲突 + 版本标签对齐

### 版本 → 门禁映射

| 版本范围 | 触发 |
|----------|------|
| v1.6.1–v1.6.4 | L1 per-commit, L2 per-version |
| v1.6.5 | L1 + L2 + DB migration 回滚测试 |
| v1.6.6–v1.6.7 | L1 + L2 + AI 响应格式校验 |
| v1.6.8–v1.6.9 | L1 + L2 + 好友状态机测试 |
| v1.6.10 | L1 + L2 + **L3** 全量 |
| v1.7.0 | **L4** Merge Gate |

### 工具链脚本

```
scripts/gates/
├── pre-commit.ps1         # L1
├── pre-pr.ps1             # L2
├── quality-gate.ps1       # L3
└── db-migration-test.ps1  # 数据库迁移回滚
```

---

### ✅ v1.7.0 — AI 升级 + i18n（2026-05-30 完成）

| 变更 | 文件 | 说明 |
|------|------|------|
| DeepSeek API 替换 ClaudeAPI | `lib/ai/providers/DeepSeekAPI.ts` | 直接调 `api.deepseek.com`，支持 streaming |
| 模型扩展 | `components/Sidebar.tsx` | V4 Pro / V3 / R1 / Flash 四个模型 |
| 中英文全界面切换 | `lib/i18n/` (4 新文件) + 14 文件修改 | ~140 翻译 key, `t()` 可脱离 React 使用 |
| useAI 更新 | `lib/ai/useAI.ts` | ClaudeAPIProvider → DeepSeekAPIProvider |
| uiStore 扩展 | `stores/uiStore.ts` | 新增 `language: 'en' | 'zh'` + `setLanguage()` |
| 日期本地化 | `lib/utils.tsx` | fmtTime/fmtDateDivider 随语言自动切换 |
| 语言切换按钮 | `App.tsx` | header 右侧 "中文"/"EN" 按钮 |

**i18n 架构**: 自定义轻量模块 (~2.5KB gzipped)，`{{param}}` 插值，`useTranslation()` hook 订阅 uiStore.language 触发重渲染，`t()` 函数可独立 import 到 Zustand store 使用。

---

## v1.7.0 → v2.0.0 冲刺路线图

详见 `docs/ROADMAP_v2.0.0.md`。三阶段 28 个子版本：

```
Phase 0 (10轮): v1.7.1 → v1.8.0    质量加固
Phase 1 (7轮):  v1.8.1 → v1.9.0    功能补全
Phase 2 (11轮): v2.0.0-a1 → v2.0.0 基础设施迁移 + 正式发布
```

关键基础设施迁移：mini-asio→real asio、JSON文档DB→规范化Schema、bcrypt卸载、500用户负载验证。

---

### 排除项（永久不做）

Backgrounds/* (全屏 WebGL/Canvas)，所有 `@react-three/*` 组件（Three.js ~500KB），FluidGlass, Lanyard, ModelViewer（3D 模型无关）。
