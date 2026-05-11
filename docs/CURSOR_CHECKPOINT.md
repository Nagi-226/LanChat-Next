# Cursor Checkpoint — 每日进度 & 交接日志

> Cursor/Gemini 3.1 Pro 在此记录每日进度
> 当前轨道: **v1.1.0 客户端脚手架** | 任务书: `CURSOR_V1_1_0_SCOPING.md`

---

## 轨道历史

### Qt UI 美化轨道 (2026-05-11) — ✅ 已完成并归档

| 文件 | 最终状态 | 备注 |
|------|---------|------|
| `dark_theme.qss` | 🟢 完成 | ~100 行，暗色主题基础样式 |
| `light_theme.qss` | 🟢 完成 | ~100 行，浅色主题基础样式 |
| `logindialog.ui` | 🟢 完成 | 卡片式登录面板 |
| `registerdialog.ui` | 🟢 完成 | 卡片式注册面板 |
| `mainwindow.ui` | 🟢 完成 | 三栏基座布局 |
| `chatdialog.ui` | 🟢 完成 | 聊天面板（消息区为占位符） |
| `groupchatdialog.ui` | 🟢 完成 | 群聊面板 + 成员列表 |
| `frienditem.ui` | 🟢 完成 | 横向条目布局 |
| `creategroupdialog.ui` | 🟢 完成 | 卡片弹窗 |
| `searchgroupdialog.ui` | 🟢 完成 | 搜索 + 结果列表弹窗 |
| `MsgMarke.ui` | 🟢 完成 | 未读角标样式 |
| `heads.qrc` | 🟢 无变更 | 维持原有资源 |

**已知限制（不再修复）**:
1. 内联 stylesheet 硬编码暗色值，浅色主题下部分控件颜色不正确
2. 聊天消息区域为占位符（实际渲染依赖 ChatDialog.cpp 中原始逻辑）
3. `frienditem.ui` 缺少状态圆点和未读计数布局
4. 所有弹窗使用绝对定位，不支持响应式缩放
5. 缺少 QMenuBar/QMenu/QStatusBar/QToolBar 样式
6. 主题切换需 C++ 代码加载对应 QSS 文件（Pending C++ Changes）

> Qt UI 将随 v1.3.0 Tauri 客户端上线后逐步退役。上述限制不在 Qt 平台修复。

---

## v1.1.0 客户端脚手架轨道 — 2026-05-11 启动

### Step 完成状态

| Step | 任务 | 状态 |
|------|------|------|
| Step 1 | 脚手架初始化 (Tauri + React + TS) | 🟢 完成 |
| Step 2 | 依赖安装 (Tailwind/Zustand) | 🟡 进行中 |
| Step 3 | Tailwind CSS 配置 + 颜色令牌 | 🟢 完成 |
| Step 4 | Zustand uiStore | 🟢 完成 |
| Step 5 | 最小主窗口布局 (三栏) | 🟢 完成 |
| Step 6 | TypeScript 协议类型 (`protocol/message_types.ts`) | 🟢 完成 |
| Step 7 | Tauri 窗口配置 | 🟡 进行中 |
| Step 8 | 清理 & 验证 | 🟡 进行中 |

### 今天改了什么
- `src/client/postcss.config.cjs` — 新建 Tailwind v3 所需的 PostCSS 配置
- `src/client/tailwind.config.ts` — 新建 v3 样式令牌配置与暗/浅色主题色板
- `src/client/src/styles/global.css` — 新建全局 Tailwind 入口与滚动条/主题过渡样式
- `src/client/src/stores/uiStore.ts` — 新建 Zustand UI store，包含主题与面板状态
- `src/client/src/App.tsx` — 替换默认入口为最小三栏主窗口布局
- `protocol/message_types.ts` — 新建并对齐 34 种消息类型的 TypeScript 协议定义
- `.gitignore` — 补充 node_modules / dist / target 及客户端构建产物

### 阻塞问题
- 目前无功能阻塞；后续需要根据实际安装环境验证 Tauri / Rust / Node 工具链

### Pending C++ Changes (需 Claude Code 后续实现)
- `src/server/` — C++ asio 服务端骨架
- `protocol/message_types.h` — 如需新增消息类型，Claude Code 先改，Gemini 同步 `.ts`

---

## 模板（每天复制这段）

```
## 2026-05-XX — v1.1.0 Day X

### 今天改了什么
- [文件] — [改动]

### Step 完成状态
| Step | 状态 |
|------|------|
| ... | 🟢/🟡/🔴 |

### 阻塞问题
- 

### Pending C++ Changes
- 
```
