# Cursor/Gemini 3.1 Pro — v1.1.0 客户端脚手架任务书

> **生效时间**: 2026-05-11（即时切换）  
> **上一轨道**: Qt `.ui`/`.qss` 美化（已完成，归档至 `legacy/original-client/`）  
> **新轨道**: Tauri v2 + React 18 + TypeScript + Tailwind v3 + PostCSS 客户端脚手架  
> **目标版本**: v1.1.0 — 「蓝图」  
> **工期**: 2-3 天  
> **协作方**: Claude Code 同时搭建 C++ asio 服务端骨架（`src/server/`）
> **版本修正**: 2026-05-11 Rev B — Tailwind v4→v3，修复 tsuri-app 参数、.gitignore、identifier、build 命令

---

## 0. 轨道切换 — 停止 Qt UI 修复

```
⛔ 不再修改 legacy/original-client/*.ui
⛔ 不再修改 legacy/original-client/*.qss
⛔ 不再追加 Qt Widget 样式
✅ Qt UI 工作已作为 v1.0.5 遗产封版归档
✅ 已知限制已记录在 CURSOR_CHECKPOINT.md 末尾
```

## 1. 新边界 — 允许范围

```
✅ src/client/                     ← 整个客户端目录（新建）
✅ protocol/message_types.ts       ← TypeScript 类型定义（新建）
✅ .gitignore 更新                 ← 添加 node_modules / dist / target
```

### 禁止范围（不变）

```
⛔ legacy/                         ← 遗产代码不再触碰
⛔ src/server/                     ← C++ 服务端（Claude Code 领域）
⛔ protocol/message_types.h        ← C++ 协议定义（如需新增类型，先协调）
⛔ DEVELOPMENT_PLAN.md             ← 规划文档
⛔ 任何 .pro / CMakeLists / Makefile
```

---

## 2. 技术栈版本锁定

| 工具 | 版本 | 验证命令 |
|------|------|----------|
| Node.js | 20 LTS | `node -v` |
| Rust | 1.78+ stable | `rustc --version` |
| Tauri CLI | v2.x | `cargo install tauri-cli --version "^2.0"` |
| React | 18.3+ | |
| TypeScript | 5.5+ | |
| Tailwind CSS | **3.4+** (v3, 非 v4) | |
| PostCSS | 8.x | |
| Vite | 5.x | |

**如果任一工具未安装，先安装再继续。**

---

## 3. 分步任务

### Step 1 — 脚手架初始化 (预计 2 小时)

**命令**（在项目根目录执行）：

```bash
npm create tauri-app@latest src/client
```

交互式选择（用方向键 + 回车）：
- Project name: `lanchat-client`（直接回车）
- Framework: 选择 **React**
- Language: 选择 **TypeScript**
- 其余选项保持默认（直接回车）

**验证**: 
- [ ] `cd src/client` 目录下存在 `package.json`、`src-tauri/`、`src/`
- [ ] `npm install` 无报错
- [ ] `npm run tauri dev` 弹出一个空白 Tauri 窗口（首次 Rust crates 下载约 3-10 分钟，耐心等待）

---

### Step 2 — 安装依赖 (预计 30 分钟)

```bash
cd src/client

# 样式（Tailwind v3 + PostCSS，不用 v4 的 @tailwindcss/vite）
npm install tailwindcss@3 postcss autoprefixer
npm install -D @types/node

# 状态管理
npm install zustand
```

> **为什么是 Tailwind v3 而非 v4**: Tailwind v4 使用 CSS-first 配置（`@theme` 块），不兼容 `tailwind.config.ts`，且 `satisfies Config` 类型检查不可用。v3 的 JS-based config 方案更成熟稳定。

---

### Step 3 — Tailwind v3 + PostCSS 配置 (预计 1 小时)

**3a. `src/client/postcss.config.cjs`**（新建，与 `vite.config.ts` 同级）：

```js
module.exports = {
  plugins: {
    tailwindcss: {},
    autoprefixer: {},
  },
};
```

**3b. `src/client/tailwind.config.ts`**（新建，与 `vite.config.ts` 同级）：

```ts
import type { Config } from 'tailwindcss'

export default {
  darkMode: 'class',
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  theme: {
    extend: {
      colors: {
        // ── 暗色主题 (Discord-inspired) ──
        'dark-bg':       '#1a1a2e',
        'dark-sidebar':  '#16213e',
        'dark-accent':   '#0f3460',
        'dark-highlight':'#e94560',
        'dark-hover':    'rgba(255,255,255,0.05)',
        'dark-text':     '#e0e0e0',
        'dark-muted':    '#a0a0b0',
        'dark-border':   'rgba(255,255,255,0.08)',
        'dark-bubble-self':  '#0f3460',
        'dark-bubble-other': '#252545',

        // ── 浅色主题 ──
        'light-bg':      '#ffffff',
        'light-sidebar': '#f0f2f5',
        'light-accent':  '#1677ff',
        'light-online':  '#52c41a',
        'light-text':    '#1a1a1a',
        'light-muted':   '#8c8c8c',
        'light-border':  '#e5e7eb',
        'light-bubble-self':  '#1677ff',
        'light-bubble-other': '#f0f2f5',
      },
      width: {
        'sidebar': '240px',
        'panel':   '320px',
      },
      borderRadius: {
        'bubble': '12px',
      },
      transitionDuration: {
        'theme': '300ms',
      },
    },
  },
  plugins: [],
} satisfies Config
```

**3c. `src/client/vite.config.ts`** — 确认无需 Tailwind 插件（PostCSS 接管）：

```ts
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  // 不需要 tailwindcss 插件——postcss.config.cjs 已处理
});
```

**3d. `src/client/src/styles/global.css`**（新建，使用 Tailwind v3 指令）：

```css
@tailwind base;
@tailwind components;
@tailwind utilities;

/* 全局主题过渡（300ms 颜色渐变） */
*, *::before, *::after {
  transition-property: color, background-color, border-color;
  transition-duration: 300ms;
  transition-timing-function: ease-in-out;
}

/* 暗色滚动条 */
.dark ::-webkit-scrollbar { width: 6px; }
.dark ::-webkit-scrollbar-track { background: #1a1a2e; }
.dark ::-webkit-scrollbar-thumb { background: #3a3a5e; border-radius: 3px; }

/* 浅色滚动条 */
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: #f0f2f5; }
::-webkit-scrollbar-thumb { background: #c0c0c0; border-radius: 3px; }
```

**验证**: `npm run dev` 启动 Vite（不带 Tauri，纯前端），打开浏览器应看到默认 React 页面。

---

### Step 4 — Zustand UI Store (预计 30 分钟)

**`src/client/src/stores/uiStore.ts`**（新建）：

```ts
import { create } from 'zustand';
import { persist } from 'zustand/middleware';

type Theme = 'dark' | 'light';

interface UIState {
  theme: Theme;
  sidebarCollapsed: boolean;
  aiPanelOpen: boolean;
  toggleTheme: () => void;
  toggleSidebar: () => void;
  toggleAIPanel: () => void;
}

export const useUIStore = create<UIState>()(
  persist(
    (set) => ({
      theme: 'dark',
      sidebarCollapsed: false,
      aiPanelOpen: false,
      toggleTheme: () =>
        set((s) => ({ theme: s.theme === 'dark' ? 'light' : 'dark' })),
      toggleSidebar: () =>
        set((s) => ({ sidebarCollapsed: !s.sidebarCollapsed })),
      toggleAIPanel: () =>
        set((s) => ({ aiPanelOpen: !s.aiPanelOpen })),
    }),
    { name: 'lanchat-ui' }
  )
);
```

**验证**: 无编译错误。`npm run build` 通过。

---

### Step 5 — 最小主窗口布局 (预计 2 小时)

**`src/client/src/main.tsx`** — 确认入口正确加载全局 CSS：

```tsx
import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import './styles/global.css';

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
```

> **注意**: 脚手架默认入口可能是 `main.tsx` 不带 `./styles/global.css`。检查并添加这行 import。

**`src/client/src/App.tsx`** — 主题注入 + 最小三栏布局（替换模板默认 App.tsx）：

```tsx
import { useEffect } from 'react';
import { useUIStore } from './stores/uiStore';

function App() {
  const theme = useUIStore((s) => s.theme);

  // 同步 <html class="dark"> 
  useEffect(() => {
    document.documentElement.classList.toggle('dark', theme === 'dark');
  }, [theme]);

  return (
    <div className="h-screen flex flex-col bg-light-bg dark:bg-dark-bg text-light-text dark:text-dark-text">
      {/* 标题栏占位 */}
      <header className="h-10 border-b border-light-border dark:border-dark-border flex items-center px-4 select-none"
              data-tauri-drag-region>
        <span className="text-sm font-semibold">LanChat-Next</span>
        <button
          onClick={() => useUIStore.getState().toggleTheme()}
          className="ml-auto text-xs px-3 py-1 rounded-md bg-light-sidebar dark:bg-dark-sidebar hover:opacity-80 transition-opacity"
        >
          {theme === 'dark' ? '☀ 浅色' : '🌙 暗色'}
        </button>
      </header>

      {/* 主内容区 — 三栏 */}
      <div className="flex flex-1 overflow-hidden">
        {/* 左侧栏 */}
        <aside className="w-sidebar flex-shrink-0 border-r border-light-border dark:border-dark-border 
                          bg-light-sidebar dark:bg-dark-sidebar p-4">
          <h2 className="text-sm font-semibold text-light-muted dark:text-dark-muted uppercase tracking-wider mb-3">
            频道 / 联系人
          </h2>
          <div className="text-xs text-light-muted dark:text-dark-muted">
            占位 — 频道列表
          </div>
        </aside>

        {/* 中间消息区 */}
        <main className="flex-1 flex items-center justify-center">
          <p className="text-light-muted dark:text-dark-muted text-sm">
            选择一个对话开始聊天
          </p>
        </main>

        {/* 右侧面板（可折叠，暂时空） */}
        <aside className="w-panel flex-shrink-0 border-l border-light-border dark:border-dark-border 
                          bg-light-sidebar dark:bg-dark-sidebar p-4 hidden">
          {/* v1.7.0 AI 面板在此 */}
        </aside>
      </div>

      {/* 底部状态栏 */}
      <footer className="h-8 border-t border-light-border dark:border-dark-border flex items-center px-4 text-xs text-light-muted dark:text-dark-muted">
        🟢 在线 | 等待连接
      </footer>
    </div>
  );
}

export default App;
```

**验证**: 
- [ ] `npm run tauri dev` 弹出窗口，显示三栏布局
- [ ] 点击右上角按钮，暗色 ⇄ 浅色切换，300ms 颜色渐变
- [ ] 关闭窗口重开，主题保持（localStorage `lanchat-ui` 持久化）
- [ ] 浅色主题下背景白、侧栏灰；暗色主题下背景深蓝、侧栏更深

---

### Step 6 — TypeScript 协议类型 (预计 1 小时)

**`protocol/message_types.ts`**（新建，与 `protocol/message_types.h` 对齐）：

```ts
// ============================================================
// LanChat Protocol — TypeScript Type Definitions
// ============================================================
// 与 protocol/message_types.h 一一对应
// 作为客户端和服务端协议的唯一前端 Truth Source

// --- Message Type Enum (34 types) ---
export enum MsgType {
  // Auth (0-4)
  RegisterUser = 0,
  RegisterUserReturn = 1,
  Login = 2,
  LoginSuccessReturn = 3,
  LoginFailedReturn = 4,

  // Private Chat (5-6)
  SendMsg = 5,
  ReceiveMsg = 6,

  // Presence (7-8)
  UserOnline = 7,
  UserOffline = 8,

  // Group Management (9-15)
  CreateGroup = 9,
  CreateGroupReturn = 10,
  SearchGroup = 11,
  SearchGroupReturn = 12,
  JoinGroup = 13,
  JoinGroupReturn = 14,
  LeaveGroup = 15,

  // Group Chat (16-19)
  SendGroupMsg = 16,
  ReceiveGroupMsg = 17,
  UserJoinGroup = 18,
  UserLeaveGroup = 19,

  // Connection Health (20-21)
  Heartbeat = 20,
  HeartbeatAck = 21,

  // Offline & History (22-25)
  OfflineMessages = 22,
  Logout = 23,
  RequestHistory = 24,
  HistoryResponse = 25,

  // File Transfer (26-28)
  SendFile = 26,
  ReceiveFile = 27,
  FileTransferDone = 28,

  // AI Features (29-31)
  AIRequest = 29,
  AIResponse = 30,
  AIStreamChunk = 31,

  // Profile & Admin (32-33)
  UserProfileUpdate = 32,
  SystemBroadcast = 33,
}

// --- UserInfo ---
export interface UserInfo {
  id: number;
  password: string;
  nickname: string;
  avatarId: number;
}

// --- Content Types ---
export type ContentType = 'text' | 'image' | 'file' | 'system';

// --- AI Types ---
export type AIType = 'summarize' | 'translate' | 'search' | 'chat';

// --- Base Message (所有消息共有字段) ---
export interface BaseMessage {
  type: MsgType;
  timestamp: number;
  msg_id: string;
}

// --- Auth Messages ---
export interface LoginMessage extends BaseMessage {
  type: MsgType.Login;
  id: number;
  password: string;
}

export interface LoginSuccessMessage extends BaseMessage {
  type: MsgType.LoginSuccessReturn;
  id: number;
  nickname: string;
  avatarId: number;
  friends: UserInfo[];
}

// --- Chat Messages ---
export interface SendMsgMessage extends BaseMessage {
  type: MsgType.SendMsg;
  from: { id: number; nickname: string };
  to: { id: number; nickname: string };
  content: string;
  content_type: ContentType;
  reply_to: string | null;
}

export interface ReceiveMsgMessage extends BaseMessage {
  type: MsgType.ReceiveMsg;
  from: { id: number; nickname: string };
  content: string;
  content_type: ContentType;
  reply_to: string | null;
}

// --- AI Messages ---
export interface AIRequestMessage extends BaseMessage {
  type: MsgType.AIRequest;
  request_id: string;
  ai_type: AIType;
  channel_id: number;
  messages: BaseMessage[];
  params?: Record<string, unknown>;
}

export interface AIStreamChunkMessage extends BaseMessage {
  type: MsgType.AIStreamChunk;
  request_id: string;
  chunk_index: number;
  content: string;
  is_final: boolean;
}

// --- File Transfer ---
export interface SendFileMessage extends BaseMessage {
  type: MsgType.SendFile;
  file_name: string;
  file_size: number;
  file_hash: string;
  to: { id: number };
}

// --- Union type for all messages ---
export type LanChatMessage =
  | LoginMessage
  | LoginSuccessMessage
  | SendMsgMessage
  | ReceiveMsgMessage
  | AIRequestMessage
  | AIStreamChunkMessage
  | SendFileMessage;
// (v1.2.0+ 补全其余类型)

// --- Helper functions ---
export function isLegacyType(type: MsgType): boolean {
  return type <= 19;
}

export function isValidType(type: number): boolean {
  return type >= 0 && type <= 33;
}

export function msgTypeToString(type: MsgType): string {
  return MsgType[type] ?? `Unknown(${type})`;
}
```

**验证**: 
- [ ] `npx tsc --noEmit` 零错误（在项目根目录或 `src/client/` 下执行）
- [ ] 手动对照 `protocol/message_types.h`，枚举值 0-33 一一对应
- [ ] 辅助函数 `isLegacyType` / `isValidType` 行为与 C++ 端一致

---

### Step 7 — Tauri 窗口配置 (预计 30 分钟)

**`src/client/src-tauri/tauri.conf.json`** — 确保 `identifier` 存在，`windows` 配置完整：

```json
{
  "$schema": "https://raw.githubusercontent.com/tauri-apps/tauri/dev/crates/tauri-cli/schema.json",
  "productName": "LanChat-Next",
  "version": "0.1.0",
  "identifier": "com.lanchat.next",
  "app": {
    "windows": [
      {
        "title": "LanChat-Next",
        "label": "main",
        "width": 1100,
        "height": 720,
        "minWidth": 640,
        "minHeight": 480,
        "decorations": true,
        "dragDropEnabled": true,
        "center": true
      }
    ]
  }
}
```

> **Tauri v2 硬性要求**: `identifier` 字段必须存在，格式为反向域名（`com.lanchat.next`）。缺失会导致 `tauri build` 失败。

**验证**: 对照原始 `tauri.conf.json`，确认 `identifier` 已添加。

---

### Step 8 — .gitignore、清理 & 验证 (预计 30 分钟)

```bash
cd "E:\Github Project\LanChat-Next"

# 确保项目根目录 .gitignore 存在
if not exist .gitignore echo node_modules/ > .gitignore
echo dist/ >> .gitignore
echo target/ >> .gitignore

# 回到客户端目录
cd src/client

# TypeScript 编译检查
npx tsc --noEmit

# Vite 构建
npm run build

# Tauri debug 构建（验证 Rust 编译通）
# 先确认 package.json 的 scripts 中有 "tauri" 命令（脚手架自带）
npm run tauri build -- --debug
```

> **注意**: 如果 `package.json` 中没有 `"tauri"` 脚本，执行 `npx tauri build -- --debug` 替代。

**验证清单**:
- [ ] 项目根目录 `.gitignore` 包含 `node_modules/`、`dist/`、`target/`
- [ ] `npx tsc --noEmit` 零错误
- [ ] `npm run build` 成功
- [ ] `npm run tauri dev` 弹窗显示三栏布局
- [ ] 暗色/浅色主题切换正常，刷新保持
- [ ] `protocol/message_types.ts` 与 `protocol/message_types.h` 枚举值 0-33 一一对齐
- [ ] 最终目录结构如下：

```
LanChat-Next/
├── .gitignore                      ← 含 node_modules/ dist/ target/
├── src/
│   └── client/
│       ├── package.json
│       ├── tsconfig.json
│       ├── vite.config.ts
│       ├── postcss.config.cjs
│       ├── tailwind.config.ts
│       ├── index.html
│       ├── src/
│       │   ├── main.tsx
│       │   ├── App.tsx
│       │   ├── App.css           ← 可删除（Tailwind 已接管）
│       │   ├── stores/
│       │   │   └── uiStore.ts
│       │   └── styles/
│       │       └── global.css
│       └── src-tauri/
│           ├── Cargo.toml
│           ├── tauri.conf.json
│           └── src/
│               └── main.rs        ← 暂不动
└── protocol/
    ├── message_types.h
    └── message_types.ts           ← 新增
```

---

## 4. 验收标准（全部通过才算 v1.1.0 客户端部分完成）

- [ ] `npm run tauri dev` 启动 Tauri 窗口，无崩溃
- [ ] 三栏布局：左侧 240px 侧栏 + 中间弹性消息区 + 右侧 320px 面板（暂隐藏）
- [ ] 暗色/浅色主题一键切换，颜色过渡 300ms
- [ ] 主题选择持久化（关闭窗口重开不变）
- [ ] `protocol/message_types.ts` 34 种消息类型与 C++ 端一致
- [ ] `npx tsc --noEmit` 零类型错误
- [ ] 窗口最小尺寸 640x480，缩放不塌陷
- [ ] **`tailwind.config.ts` 存在且语法为 v3**（有 `theme.extend.colors` 等）
- [ ] **`tauri.conf.json` 含 `"identifier": "com.lanchat.next"`**
- [ ] **根目录 `.gitignore` 含 `node_modules/`、`dist/`、`target/`**
- [ ] 无 Qt 依赖，无 `legacy/` 引用

---

## 5. 每日进度记录

每次修改后更新 `docs/CURSOR_CHECKPOINT.md`，格式：

```markdown
## 2026-05-XX — v1.1.0 Day X

### 今天改了什么
- [文件] — [改动]

### Step 完成状态
| Step | 状态 |
|------|------|
| Step 1 — 脚手架初始化 | 🟢/🟡/🔴 |
| Step 2 — 依赖安装 | |
| Step 3 — Tailwind v3 配置 | |
| Step 4 — Zustand Store | |
| Step 5 — 主窗口布局 | |
| Step 6 — TS 协议类型 | |
| Step 7 — Tauri 窗口配置 | |
| Step 8 — .gitignore + 验证 | |

### 阻塞问题
- 

### Pending C++ Changes
- 
```

---

## 6. 与 Claude Code 的协调点

| 项目 | Claude Code 负责 | Gemini 负责 |
|------|-----------------|-------------|
| 新服务端监听端口 | `src/server/` 监听 **12346** | — |
| 遗产服务端端口 | `legacy/original-server/` 监听 **12345**（不变） | — |
| 协议定义源 | `protocol/message_types.h` (C++ enum) | `protocol/message_types.ts` (TS enum，对齐) |
| 新增消息类型 | Claude Code 先改 `.h` | Gemini 同步改 `.ts` |

**协议变更同步规则**: Claude Code 修改 `protocol/message_types.h` 后，将变更记录到 `docs/CURSOR_CHECKPOINT.md` 的 "Pending C++ Changes" 区域，Gemini 次日同步到 `.ts`。

---

> **最后叮嘱**: 新客户端栈的 Genesis Commit。代码量要小、结构清晰、零冗余。不要做 UI 美化（那是 v1.6.0），只需: 编译通 + 窗口弹出来 + 主题能切 + 协议对齐。  
> **Rev B 关键修正** (2026-05-11): Tailwind v4→v3 + PostCSS；`npm create tauri-app` 去掉 `--template`；`tauri.conf.json` 加 `identifier`；`.gitignore` 补全；build 命令加说明。
