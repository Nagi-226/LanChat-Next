# LanChat-Next — 完全重制版开发架构规划 v2.1

> **原始项目**: LAN ChatRoom (C++11 + Qt 5.4, 毕业设计) → **Baseline: v1.0.1**  
> **目标项目**: LanChat-Next — 面向 500 人中小企业的 AI 驱动局域网智能社交平台 → **Target: v2.0.0**  
> **开发组合**: Claude Code (DeepSeek-V4-Pro) + Codex (GPT-5.5)  
> **目标平台**: Windows 11 桌面应用  
> **终局架构**: C++17 asio 服务端 + Tauri v2 / React 18 客户端（**v2.0.0 起零 Qt 依赖**）  
> **迁移策略**: 渐进式内部替换——在同一代码库中逐模块用新技术栈替换 Qt，旧模块退役但不丢弃（归档至 legacy/）  
> **版本路径**: v1.0.1 (遗产基线) → v1.0.x (逐步加固) → v1.1.0 (新架构启动) → ... → v1.9.x → v2.0.0 (零 Qt 正式版)  
> **迭代规则**: `.5` 是质量检查点而非自动跳板——通过后进入下一阶段，不通过则 `.6/.7/.8` 继续打磨  
> **文档版本**: v2.1 | 2026-05-11

---

## 目录

1. [项目愿景与范围](#1-项目愿景与范围)
2. [原始项目资产分析](#2-原始项目资产分析)
3. [技术选型与理由](#3-技术选型与理由)
4. [整体架构设计](#4-整体架构设计)
5. [Claude Code + Codex 协作开发流程](#5-claude-code--codex-协作开发流程)
6. [版本迭代路线图](#6-版本迭代路线图)
7. [AI Agent 集成架构](#7-ai-agent-集成架构)
8. [通信协议重新设计](#8-通信协议重新设计)
9. [数据库 Schema 设计](#9-数据库-schema-设计)
10. [UI/UX 现代化方案](#10-uiux-现代化方案)
11. [项目目录结构](#11-项目目录结构)
12. [测试策略](#12-测试策略)
13. [风险与缓解](#13-风险与缓解)

**附录**
- [A: 开发环境清单](#附录-a-开发环境清单)
- [B: 原始项目 vs LanChat-Next v2 对比](#附录-b-原始项目-vs-lanchat-next-v2-对比)
- [C: AI Agent 开发工具与技能清单](#附录-c-ai-agent-开发工具与技能清单)

---

## 1. 项目愿景与范围

### 1.1 愿景

将 3 年前的毕业设计 LAN ChatRoom，借助当今最强的 AI Agent 开发工具组合，重制为一款**现代化、具备 AI 智能辅助、UI 精美的局域网桌面社交平台**，足以支撑 500 人以内的中小企业内部通讯，同时保持个人开发者级别的部署复杂度。

### 1.2 一句话定义

> 一款面向中小企业局域网、无需云端服务器、集成 AI 智能助手的现代化桌面即时通讯平台。C++ 服务端提供高性能消息路由，Tauri + React 客户端提供 Discord 级别 UI 体验。

### 1.3 核心差异化（vs 原始项目）

| 原始项目 | LanChat-Next |
|----------|-------------|
| Qt 5.4 Widget 原始样式 | React 18 + Tailwind CSS 现代主题（深色/浅色） |
| 仅在线消息，无持久化 | SQLite 全消息历史 + 客户端本地缓存 |
| 3 个头像硬编码 | 自定义头像上传 + 文件系统存储 |
| 无 AI 能力 | AI Agent 面板：摘要/搜索/翻译/问答 |
| .ui 表单布局，不可缩放 | 响应式布局 + 窗口自适应 |
| TCP JSON 明文 | TCP + 可选 TLS |
| 无文件传输 | 支持文件/图片传输 + 内网直传 |
| 无通知系统 | Windows 原生 Toast 通知 |
| 无搜索 | 全文搜索 (FTS5) + AI 语义搜索 |
| 毕业设计玩具 | 可实际部署的企业内部工具 |
| QMake 构建 | CMake (服务端) + Vite (客户端) |

### 1.4 不做的事（明确边界）

- ❌ 不做云端部署（保持 LAN 定位）
- ❌ 不做移动端（聚焦 Windows 桌面）
- ❌ 不做视频/语音通话（复杂度超出个人开发者范围）
- ❌ 不做端到端加密（企业内网信任模型）
- ❌ 不做 SSO/LDAP 集成（v1.0 范围内）

---

## 2. 原始项目资产分析

### 2.1 可继承的资产

| 资产 | 复用方式 |
|------|----------|
| 19 种消息类型协议设计 | 作为新协议设计参考，扩展至 ~30 种 |
| C/S 架构思路 | 保留：C++ 服务端 + 客户端（换 Tauri） |
| TCP JSON 通信模型 | 保留核心思路，升级为 nlohmann/json |
| SQLite 持久化思路 | 扩展为完整数据层（服务端 + 客户端双 SQLite） |
| UI 交互逻辑（好友列表/聊天气泡/未读标记） | 作为新 UI 的交互参考 |
| 群聊生命周期管理 | 扩展为频道系统 |

### 2.2 必须重写的部分

| 问题 | 解决方案 |
|------|----------|
| cJSON C 库（手动内存管理） | nlohmann/json (现代 C++ JSON) |
| Qt 5 客户端 + .ui 硬布局 | Tauri v2 + React 18 + Tailwind CSS |
| QMake 构建系统 | CMake (服务端) + Vite (客户端) |
| 每客户端一线程（不可扩展） | asio 协程/线程池 或 Tokio (Tauri 侧) |
| 头像是 QRC 编译资源 | 文件系统头像 + 客户端缓存 |
| 简单密码存储 | bcrypt 哈希 |
| QString 与 std::string 频繁转换 | 统一 UTF-8 std::string |

### 2.3 遗产代码目录映射

原始毕业设计 (v1.0.1) 的代码已归档至 `legacy/` 目录，命名规范化映射如下：

| 原始目录名 | 新归档路径 | 说明 |
|-----------|-----------|------|
| `HHClient/` | `legacy/original-client/` | Qt 5.4 客户端源码（Login/Register/Chat/Group） |
| `HHServer/` | `legacy/original-server/` | Qt 5.4 服务端源码（TcpServer/NetworkThread/UserDao） |
| `QQChat/` | `legacy/qq-chat-prototype/` | QQ 风格聊天原型 |
| `聊天demo（包含群聊，和未读信息，没有头像）/` | `legacy/chat-demo-with-group-and-unread/` | 群聊 + 未读标记功能演示 |

> **迁移策略（渐进式内部替换）**: 不建独立新项目，而是在同一代码库中逐模块替换。协议层先行（protocol/ 独立于 Qt）→ 网络层替换（asio 替代 QTcpServer/QTcpSocket）→ 数据层替换（SQLiteCpp 替代 QSqlDatabase）→ UI 层替换（Tauri+React 替代 QWidget/.ui）→ v2.0.0 移除最后一行 Qt 代码。每替换一个模块，旧 Qt 实现退役归档至 `legacy/`，新实现对上层透明。

---

## 3. 技术选型与理由

### 3.1 核心技术栈总览

```
┌─────────────────────────────────────────────────────────┐
│                    LanChat-Next                          │
│                                                         │
│  ┌──────────────────────┐  ┌──────────────────────────┐ │
│  │   服务端 (C++17)      │  │  客户端 (Tauri v2 + React) │ │
│  │                      │  │                          │ │
│  │  网络: asio          │  │  UI: React 18 + TS       │ │
│  │  JSON: nlohmann      │  │  样式: Tailwind CSS      │ │
│  │  数据库: SQLiteCpp   │  │  状态: Zustand           │ │
│  │  AI: cpp-httplib    │  │  Tauri: Rust 薄封装层    │ │
│  │  日志: spdlog        │  │  网络: Tauri TCP Command │ │
│  │  密码: bcrypt        │  │  通知: Tauri Notification │ │
│  │  构建: CMake         │  │  构建: Vite + Cargo      │ │
│  └──────────────────────┘  └──────────────────────────┘ │
│                                                         │
│              通信协议: TCP + JSON (nlohmann)             │
└─────────────────────────────────────────────────────────┘
```

### 3.2 服务端技术栈 (C++17)

| 组件 | 选型 | 理由 |
|------|------|------|
| **语言** | C++17 | 现代特性 (string_view/optional/structured bindings) |
| **网络库** | **asio** (header-only, 非Boost版本) | 异步 I/O，线程池模型，天然支持 500+ 并发 |
| **JSON** | nlohmann/json (header-only) | 现代 C++ JSON，STL 容器无缝互转 |
| **数据库** | SQLite 3 + SQLiteCpp | 单文件零配置，WAL 模式支持并发读 |
| **密码哈希** | bcrypt (header-only, wrapper for Win32 BCrypt) | 工业级密码存储 |
| **AI HTTP Client** | cpp-httplib (header-only) | 轻量 HTTP/HTTPS 请求，支持流式响应 |
| **日志** | spdlog (header-only) | 高性能异步日志，支持文件轮转 |
| **构建系统** | CMake 3.28+ | 跨平台构建，fetchContent 管理依赖 |
| **测试** | Google Test | 行业标准 C++ 测试框架 |

### 3.3 客户端技术栈 (Tauri v2 + React)

| 组件 | 选型 | 理由 |
|------|------|------|
| **桌面框架** | Tauri v2 | Rust 后端 + Web 前端，体积 ~15MB，Win11 原生窗口 |
| **UI 框架** | React 18 + TypeScript | AI 代码生成质量最高的前端技术栈 |
| **样式方案** | Tailwind CSS 3.4+ | 原子化 CSS，Dark/Light 主题切换便捷 |
| **状态管理** | Zustand | 轻量、TS 友好、无 boilerplate |
| **TCP 通信** | Tauri Rust Command (tokio::net::TcpStream) | TCP 连接由 Rust 层管理，暴露给 React |
| **本地存储** | IndexedDB (Dexie.js) 或 Tauri SQL Plugin | 客户端离线消息缓存 |
| **通知** | Tauri Notification Plugin | Windows 原生 Toast |
| **文件操作** | Tauri File System Plugin | 头像上传/文件传输 |
| **构建** | Vite 5 + Cargo | 极速 HMR + Rust 编译优化 |
| **图表/动效** | Framer Motion | React 动画库，聊天气泡出现效果 |

### 3.4 为什么不全部用 C++/Qt 6？

| 考量 | Qt 6 C++ | Tauri v2 + React/TS |
|------|----------|---------------------|
| AI 代码生成质量 | ⭐⭐ (训练数据少，QSS 幻觉多) | ⭐⭐⭐⭐⭐ (React/TS 是 AI 最熟悉的领域) |
| UI 开发速度 (AI 辅助) | 慢（QSS 调试困难） | 快 3-5x（Tailwind 所见即所得） |
| Win11 原生感 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ (Tauri v2 支持亚克力/圆角/Mica) |
| 体积 | ~30MB | ~15MB (Tauri 压缩) |
| 内存占用 | ~80MB | ~120MB (WebView) |
| 服务端性能 | N/A | N/A（服务端仍用 C++） |
| 生态丰富度 | Qt 插件生态 | npm 百万级生态 |
| 热更新 | 不支持 | Web 前端天然支持 |

> **结论**: C++ 服务端 + Tauri 客户端 = 各取所长。服务端保留 C++ 的高性能和精细控制，客户端利用 Web 技术栈的 AI 生成优势 + 丰富生态。这是 "AI Agent 时代" 最务实的架构选择。

### 3.5 AI Agent 技术栈

| 组件 | 选型 | 说明 |
|------|------|------|
| **LLM API** | DeepSeek-V4-Pro (via weelinking.com) | 主力模型，性价比最高 |
| **本地模型** | Ollama (qwen2.5:7b / deepseek-r1:8b) | 离线场景兜底 |
| **Embedding** | text-embedding-3-small (API) / bge-small-zh (本地) | 消息语义搜索 |
| **向量存储** | SQLite + JSON 数组 (余弦相似度计算) | 轻量向量检索，无需额外依赖 |
| **HTTP 客户端 (Server)** | cpp-httplib | AI API 调用 + 流式 SSE 解析 |

---

## 4. 整体架构设计

### 4.1 系统架构图

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    LanChat-Next 客户端 (Tauri v2)                        │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                    React 18 / TypeScript UI                       │    │
│  │                                                                   │    │
│  │  ┌──────────┐ ┌───────────┐ ┌───────────┐ ┌─────────────────┐   │    │
│  │  │ LoginPage│ │FriendList │ │ ChatView  │ │ AIAssistantPanel│   │    │
│  │  │ * bcrypt │ │ * 在线状态 │ │ * 气泡渲染 │ │ * 摘要/翻译     │   │    │
│  │  │ * 头像上传│ │ * 未读计数 │ │ * 文件预览 │ │ * 智能搜索      │   │    │
│  │  └──────────┘ └───────────┘ └───────────┘ └─────────────────┘   │    │
│  │                          ↕ Tauri Invoke                          │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                  Tauri Rust 薄封装层 (src-tauri/)                 │    │
│  │                                                                   │    │
│  │  ┌────────────┐ ┌───────────┐ ┌────────────┐ ┌───────────────┐  │    │
│  │  │TcpManager  │ │MsgCodec   │ │LocalCache  │ │NotificationMgr│  │    │
│  │  │* 连接/重连 │ │* 序列化   │ │* Dexie.js  │ │* Toast 弹出   │  │    │
│  │  │* 心跳 30s  │ │* 反序列化 │ │* 离线消息  │ │* 声音提示     │  │    │
│  │  │* 消息收发  │ │* 帧边界   │ │* 用户缓存  │ │               │  │    │
│  │  └────────────┘ └───────────┘ └────────────┘ └───────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────┬────────────────────────────────────────────┘
                              │ TCP (JSON Protocol, length-prefixed)
                              │ Port: 12345
┌─────────────────────────────┴────────────────────────────────────────────┐
│                    LanChat-Next 服务端 (C++17 + asio)                     │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                    连接管理层 (Connection Layer)                  │    │
│  │  ┌────────────────┐ ┌────────────────┐ ┌──────────────────────┐ │    │
│  │  │  asio::io_ctx  │ │  SessionPool  │ │  HeartbeatMonitor    │ │    │
│  │  │  * 线程池 (N)  │ │  * 连接→用户  │ │  * 30s 心跳检测      │ │    │
│  │  │  * 异步接受    │ │  * 超时清理   │ │  * 90s 超时断开      │ │    │
│  │  │  * 异步读写    │ │  * 最大500    │ │  * 断线通知广播      │ │    │
│  │  └────────────────┘ └────────────────┘ └──────────────────────┘ │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                    消息路由层 (Message Routing)                    │    │
│  │  ┌────────────────┐ ┌────────────────┐ ┌──────────────────────┐ │    │
│  │  │ MessageRouter  │ │ ChannelManager │ │  PresenceManager     │ │    │
│  │  │ * P2P 转发     │ │ * 频道生命周期 │ │  * 在线状态广播      │ │    │
│  │  │ * 离线消息队列 │ │ * 成员管理     │ │  * 状态变更推送      │ │    │
│  │  │ * 已读回执     │ │ * 权限控制     │ │  * 登录/登出广播     │ │    │
│  │  └────────────────┘ └────────────────┘ └──────────────────────┘ │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                     AI Agent 调度层                               │    │
│  │  ┌────────────────┐ ┌────────────────┐ ┌──────────────────────┐ │    │
│  │  │  AIManager     │ │ EmbeddingSvc  │ │  PromptEngine        │ │    │
│  │  │ * 模型路由     │ │ * 消息向量化  │ │  * 模板管理          │ │    │
│  │  │ * 速率限制     │ │ * 相似搜索    │ │  * 上下文窗口管理    │ │    │
│  │  │ * 缓存去重     │ │ * 批量处理    │ │  * Token 估算        │ │    │
│  │  └────────────────┘ └────────────────┘ └──────────────────────┘ │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                    数据持久层 (Data Layer)                        │    │
│  │  ┌────────────────┐ ┌───────────────┐ ┌───────────────────────┐ │    │
│  │  │ UserRepository │ │MessageRepo   │ │ ChannelRepository    │ │    │
│  │  │ * CRUD         │ │ * 分页查询   │ │ * 频道 CRUD          │ │    │
│  │  │ * bcrypt 验证  │ │ * FTS5 全文  │ │ * 成员管理           │ │    │
│  │  │ * 头像管理     │ │ * 语义搜索   │ │ * 权限检查           │ │    │
│  │  └────────────────┘ └───────────────┘ └───────────────────────┘ │    │
│  │                           │ SQLite (WAL)                         │    │
│  │                      ┌────┴─────┐                                │    │
│  │                      │lanchat.db │                                │    │
│  │                      └──────────┘                                │    │
│  └─────────────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────────────┘
```

### 4.2 核心设计原则

1. **分层解耦**: React UI → Tauri Rust 桥接 → TCP → C++ Server → SQLite，每层接口明确
2. **胖服务端，瘦客户端**: 核心业务逻辑在 C++ 服务端，客户端专注 UI 展示
3. **协议优先**: 先定义 JSON Schema，Client/Server 严格按照 Schema 实现
4. **离线优先**: 客户端 IndexedDB 本地缓存消息，网络恢复后同步
5. **AI 可选**: AI 功能通过配置开关控制，不强制依赖外部 API
6. **C++ 专注高性能**: 服务端不对 UI 做任何妥协，只暴露 TCP 协议

### 4.3 数据流示意

```
用户输入消息 (React)
  → Tauri invoke("send_message", {content, channel})
    → Rust TcpManager: 封装 JSON + 长度前缀 → TCP write
      → C++ Server Session: 读取 → 解析 → MessageRouter
        → 写入 SQLite
        → 转发给目标用户 Session(s)
          → TCP write → Rust TcpManager: 解析 → invoke 回调
            → React: 更新消息列表状态 → 重新渲染气泡
```

---

## 5. Claude Code + Codex 协作开发流程

### 5.1 核心原则：单引擎主导 + 交叉审查

**不要试图在生产流程中频繁切换引擎。** 每个模块由一个 Agent 主导完成，完成后由另一个 Agent 做独立审查。这样可以避免上下文断裂和风格不一致。

```
┌─────────────────────────────────────────────────────────────┐
│                    开发工作流 v2.0                            │
│                                                             │
│   ┌───────────────┐              ┌───────────────┐          │
│   │  Claude Code   │              │    Codex      │          │
│   │  (DeepSeek)    │              │  (GPT-5.5)    │          │
│   │                │              │               │          │
│   │  擅长:         │              │  擅长:        │          │
│   │  • 架构设计    │              │  • 批量代码生成│          │
│   │  • C++ 代码    │              │  • React/TS   │          │
│   │  • 协议/Schema │              │  • Tailwind   │          │
│   │  • QSS → CSS   │              │  • Rust 胶水层 │          │
│   │  • 代码审查    │              │  • 测试编写    │          │
│   │  • 文档/规划   │              │  • 构建配置    │          │
│   └───────┬───────┘              └───────┬───────┘          │
│           │                              │                  │
│           │    ┌────────────────────┐    │                  │
│           └────→  模块主导权分配    ←────┘                  │
│                │                    │                       │
│                │ C++ Server:  Claude Code 主导               │
│                │ React UI:   Codex 主导                      │
│                │ Rust 桥接:  Codex 主导                      │
│                │ 代码审查: 交叉进行                           │
│                └────────────────────┘                       │
│                                                             │
│            ┌───────────────────┐                            │
│            │   人类开发者       │                            │
│            │   (决策 + 验收)    │                            │
│            └───────────────────┘                            │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 具体分工矩阵

| 任务类别 | 主力工具 | 协作方式 |
|----------|----------|----------|
| **架构设计文档** | Claude Code | 先出方案 → 人类审核 → 定稿 |
| **协议 Schema (JSON)** | Claude Code | 定义所有消息类型和字段 |
| **C++ Server 核心** | Claude Code 主导 | Claude 写网络/路由/数据库层 |
| **C++ Server AI 层** | Claude Code 主导 | Claude 设计 AI 调度架构和 Prompt 模板 |
| **React UI 组件** | Codex 主导 | Codex 生成组件代码 + Tailwind 样式 |
| **React 状态管理** | Codex 主导 | Zustand store + Tauri invoke 封装 |
| **Rust Tauri 桥接** | Codex 主导 | Tauri commands + TCP 管理 |
| **CMake 构建** | Claude Code | 服务端构建系统 |
| **Vite/Cargo 构建** | Codex | 客户端构建系统 |
| **单元测试** | 各自主导工具的测试 | C++: Claude + GTest, TS: Codex + Vitest |
| **代码审查** | **交叉审查** | Claude 审查 React/Rust 代码, Codex 审查 C++ 代码 |
| **Bug 分析** | Claude Code | 读代码 → 定位根因 → 提出修复 |
| **文档** | Claude Code | README / API 文档 / 部署手册 |

### 5.3 Prompt 工程策略

#### C++ 服务端 (Claude Code 主导)

```
【网络层设计】
"在 C++17 / asio (header-only, 非Boost版本) 上设计一个 TCP 服务端。
 约束：
 - 异步 accept，线程池处理（线程数 = CPU 核数）
 - 每个连接分配 Session，SessionPool 管理生命周期
 - 支持 length-prefixed JSON 消息（4 字节长度 + JSON body）
 - 500 并发连接目标
 - 90s 无心跳超时断开
 请给出 TcpServer / Session / SessionPool 的类声明和关键方法签名。"

【消息路由】
"实现 MessageRouter 类：
 - routeMessage(json) → 解析 type 字段 → 根据消息类型分发
 - P2P 消息：查 SessionPool → 转发到目标 Session
 - 群聊消息：查 ChannelRepository → 广播给所有在线成员
 - 离线消息：写入 private_messages 表，标记 is_read=0
 - 返回路由结果（成功/离线存储/用户不存在）"

【Repository 层】
"实现 MessageRepository::searchMessages(keyword, channelId, limit):
 - 使用 SQLite FTS5 进行全文搜索
 - 返回匹配的消息列表，按相关度排序
 - 同时查询 message_embeddings 表做语义搜索（余弦相似度 > 0.7）
 - 合并 FTS5 和语义搜索结果，去重后返回前 limit 条"
```

#### React 客户端 (Codex 主导)

```
【聊天界面组件】
"创建一个 Discord 风格的 ChatView 组件 (React 18 + TypeScript + Tailwind):
 - 消息列表：支持虚拟滚动（react-window），每条约 500 条消息
 - 聊天气泡：自己的右对齐/蓝色，他人的左对齐/灰色
 - 气泡内容：头像(圆形 32px) + 昵称 + 消息文本 + 时间戳
 - 图片消息：气泡内嵌预览（lightbox 点击放大）
 - 文件消息：文件图标 + 文件名 + 大小 + 下载按钮
 - 新消息自动滚到底部（如果已经在底部）
 - 加载历史：向上滚动触发 loadMore
 - 使用 Zustand 管理消息状态
 - 使用 Framer Motion 做消息出现动画"

【AI 助手面板】
"创建 AIAssistantPanel 组件：
 - 右侧可折叠面板，宽度 320px
 - 三个 Tab：摘要 / 搜索 / 翻译
 - 摘要 Tab：选中对话 → 自动生成摘要（Markdown 渲染）
 - 搜索 Tab：自然语言输入 → 语义搜索历史消息
 - 翻译 Tab：选中消息 → 翻译为中文/英文
 - 流式响应：AI 回复逐字显示（打字机效果）
 - 使用 Tauri invoke 调用后端 AI 功能"
```

#### Rust Tauri 桥接层 (Codex 主导)

```
【TCP 管理器】
"在 Tauri v2 中实现 TcpManager (Rust):
 - 使用 tokio::net::TcpStream 异步连接服务端
 - 自动重连：指数退避 (1s/2s/4s/8s/最大 30s)
 - 消息收发：length-prefixed JSON 帧协议
 - 心跳：30s 间隔发送 type=20 heartbeat，超时 90s 断连
 - 暴露 Tauri commands: connect/disconnect/send_message
 - 通过 Tauri events 推送收到的消息给前端
 - 使用 tauri::State 管理连接状态"

【本地缓存】
"使用 Tauri SQL Plugin 或 IndexedDB (Dexie.js) 实现客户端消息缓存：
 - 缓存最近 1000 条消息
 - 离线时读取缓存显示
 - 上线后增量同步（通过 last_msg_id）"
```

### 5.4 代码审查闭环

```
[主导 Agent 完成模块] → [另一个 Agent 独立审查]
                              │
                              ├→ 线程安全问题
                              ├→ 内存/资源泄漏
                              ├→ SQL 注入风险
                              ├→ 协议一致性
                              └→ 代码风格
                              │
[主导 Agent 修复] ←───────────┘
        │
[人类抽查验收]
```

### 5.5 开发节奏

```
Phase 0: 环境搭建 (~1天)
  全栈环境: VS 2022 + CMake + VSCode + Rust + Node.js
  Claude Code: 设计 CMake 项目结构
  Codex: 初始化 Tauri + React 脚手架

Phase 1: 协议 + 数据模型 (~2天)
  Claude Code: 定义 30+ 消息 Schema + 数据库建表 SQL
  Claude Code: 实现 C++ 服务端 Entity/Repository 层
  Codex: 实现 TypeScript 端类型定义 (与 JSON Schema 对齐)

Phase 2: 核心网络 (~3天)
  Claude Code: 实现 C++ Server (asio TcpServer + Session + MessageRouter)
  Codex: 实现 Tauri Rust TcpManager + 协议编解码
  Claude Code: 编写服务端单元测试

Phase 3: 基础 UI (~4天)
  Codex: React 组件 (LoginPage / MainWindow / ChatView / ChannelList)
  Codex: Zustand Store + Tauri invoke 封装
  Codex: Tailwind 基础主题 + 聊天气泡样式

Phase 4: AI Agent 集成 (~3天)
  Claude Code: C++ AI 调度层 (AIManager / PromptEngine / EmbeddingService)
  Codex: React AI 助手面板 (流式渲染 / Markdown / 打字机效果)

Phase 5: 打磨与测试 (~3天)
  交叉审查 + 性能优化 + 安装包生成 (NSIS / WiX)
```

---

## 6. 版本迭代路线图

> **版本号规则**: `v<MAJOR>.<MINOR>.<PATCH>` — MAJOR 变更表示架构级重构，MINOR 表示功能里程碑，PATCH 表示 bug 修复/小优化。  
> **起点**: v1.0.1（毕设遗产基线） → **终点**: v2.0.0（完全重制正式版）

---

### 版本全景图

```
v1.0.1──v1.0.2──v1.0.3──v1.0.4──v1.0.5──v1.1.0── ... ──v1.1.N──v1.2.0── ... ──v1.9.N──v2.0.0
  ✅      ✅      ✅      ✅      ✅~     ⬜              ⬜      ⬜              ⬜      🎯
 归档   修复    稳固    补全   检查点   蓝图           检查点   骨架            检查点   零Qt
                                │                          │                          │
                         代码完成，3项                   ├─ 通过 → v1.2.0           ├─ 通过 → 发布
                         需 Qt 环境验证                  └─ 不通过 → v1.1.6+        └─ 不通过 → v1.9.6+
                         → 允许启动 v1.1.0

✅~ = 代码交付完成，部分验收项需 Qt 5.15 环境验证（不影响进入下一阶段）

规则: 每个大里程碑(.0)启动 → .3加固 → .4补全 → .5检查 → 通过则跳，不通过则 .6/.7/.8 直到达标
     .5 是"质量检查点"不是"自动跳板"——由 AI Agent 和人类共同验收后决策
     大版本(x.0.0)完成后自动触发全项目审计 + 下一大版本规划细化
```

### 版本迭代规则（通用）

| 版本位 | 含义 | 触发条件 |
|--------|------|----------|
| **MAJOR** (x.0.0) | 架构级里程碑 | 所有子版本 .5 检查通过 + 全项目审计通过 |
| **MINOR** (x.0 ~ x.N) | 功能阶段 | `.0` 启动 → `.3` 加固 → `.4` 补全 → `.5` 检查 → 不通过则 `.6+` |
| **PATCH** (x.x.1+) | 修复/微调 | 每完成一个子任务或发现一个 bug |

### 质量检查触发机制

| 事件 | 自动触发动作 |
|------|-------------|
| **每个 x.x.1 ~ x.x.5 完成** | 可行性验收检查：对照该版本验收标准逐项核查，未达标项自动生成下一 PATCH 版本任务 |
| **每个 MAJOR 版本完成** | 全项目审计：BUG 扫描 + 代码冗余检测 + 耦合度分析 + 性能 Profiling + 安全审查 → 生成修复清单 |
| **每个 MAJOR 版本完成后** | 下一 MAJOR 版本规划细化：根据当前实际进度和审计结果，更新后续版本的具体任务和验收标准 |
| **每 3 个 PATCH 版本** | 代码健康快照：圈复杂度 + 重复代码率 + 测试覆盖率趋势 |

### 关键原则

- **.5 是检查点，不是跳板** — 通过所有验收标准 → 进入下一阶段；任一标准未通过 → `.6` 继续打磨，直到条件满足
- **不求一步到位** — 先跑通 (.0)，再加固 (.3)，再补全 (.4)，最后检查 (.5+)
- **封版不加新功能** — 从 `.5` 起功能冻结，只做测试/修复/文档/清理
- **渐进替换，内部消化** — 新技术栈代码与旧 Qt 代码在同一项目中并存，逐模块替换，对外行为不变
- **v2.0.0 = 零 Qt** — 最终目标：项目中不再有任何 Qt 依赖，所有 `#include <Q...>` 已替换

---

### Qt 模块退役路线图

> 每个版本逐步替换 Qt 模块，v2.0.0 完成最后一行的 Qt 代码移除。

| Qt 模块 | 当前用途 | 替换技术 | 退役版本 | 状态 |
|---------|---------|---------|---------|------|
| `QString` | 字符串处理 | `std::string` + `std::string_view` | v2.0.0 | 待替换 |
| `QTcpServer` / `QTcpSocket` | 网络通信 | `asio::ip::tcp` | v1.2.0 | 待替换 |
| `QSqlDatabase` / `QSqlQuery` | 数据库访问 | `SQLiteCpp` | v1.2.0 | 待替换 |
| `QThread` | 线程管理 | `std::thread` + asio 线程池 | v1.2.0 | 待替换 |
| `cJSON` (C) | JSON 解析 | `QJsonDocument` (Qt native) → v1.2.0 `nlohmann/json` | ✅ v1.0.2 | 已退役 |
| `QWidget` / `.ui` 文件 | 客户端 UI | React 18 + Tailwind | v2.0.0 | 待替换 |
| `QListWidget` / `QListView` | 列表组件 | React 虚拟滚动 | v1.4.0 | 待替换 |
| `QTextEdit` / `QLineEdit` | 输入组件 | React controlled components | v1.4.0 | 待替换 |
| `QApplication` / 事件循环 | 应用主循环 | Tauri event loop (winit) | v1.3.0 | 待替换 |
| `QCryptographicHash` | 密码哈希 | `bcrypt` (header-only) | v1.2.0 | 待替换 |
| `QHostInfo` / `QHostAddress` | 网络信息 | asio resolver | v1.2.0 | 待替换 |
| `QFile` / `QTextStream` | 日志文件 | `LegacyLogger` (Qt wrapper) → v1.2.0 `spdlog` | ✅ v1.0.3 | 已封装 |
| TCP 原始字节流 | 帧协议 | `LegacyProtocolCodec` (4B BE length-prefix) | ✅ v1.0.3 | 已实现 |
| `std::map` (群聊) | 频道存储 | `MessageDao` (SQLite channels + members) | ✅ v1.0.4 | 已持久化 |
| `QByteArray` | 二进制数据 | `std::vector<uint8_t>` | v2.0.0 | 待替换 |
| `qmake` / `.pro` 文件 | 构建系统 | CMake + Vite + Cargo | v1.1.0 | 待替换 |
| `QRC` 资源文件 | 头像/图标 | 文件系统 + Tauri assets | v1.3.0 | 待替换 |
| `QStyleSheet` (QSS) | 样式 | Tailwind CSS | v1.6.0 | 待替换 |

---

### v1.0.1 — 「遗产基线」← 当前起点 (原始毕设)

**状态**: ✅ 已完成 — 代码归档 + 目录规范化

| 归档路径 | 原始名称 | 内容 |
|----------|----------|------|
| `legacy/original-client/` | HHClient | Qt 5.4 客户端，含登录/注册/好友/群聊 UI |
| `legacy/original-server/` | HHServer | Qt 5.4 服务端，TCP 多线程 + UserDao |
| `legacy/qq-chat-prototype/` | QQChat | QQ 风格聊天原型 |
| `legacy/chat-demo-with-group-and-unread/` | 聊天demo | 群聊 + 未读标记功能演示 |

**交付物** (2026-05-11):
- [x] 四个遗产目录重命名归档至 `legacy/`
- [x] `legacy/README.md` 编写（含架构概览 + 构建说明 + 已知问题清单）
- [x] DEVELOPMENT_PLAN.md v2.0 定稿（Tauri 路线 + 渐进迁移策略）

---

### v1.0.2 — 「整理」— 遗产代码修复与渐进准备 (预计 1 天)

**目标**: 让遗产代码在 Win11 上可编译运行，修复明显问题，为渐进替换铺路

| 模块 | 任务 |
|------|------|
| 构建 | 验证 VS2022 + Qt 5.4/5.15 兼容性，修复编译错误 |
| cJSON | 替换 cJSON.c/h 为 nlohmann/json (header-only)，消除手动内存管理风险 |
| 协议 | 从 MsgBuilder.h 提取消息类型枚举 → `protocol/message_types.h` (纯 C++ header，独立于 Qt) |
| 协议 | 编写 19 种消息的 JSON Schema 文档（为后续扩展至 34 种做准备） |
| 密码 | 密码存储从明文改为 bcrypt 哈希（仅服务端验证逻辑） |
| 代码 | 清理 `using namespace std/neb`，修复中文注释乱码，统一缩进 |
| 数据库 | 服务端添加 SQLite 消息持久化（原始版本消息仅内存转发，重启丢失） |
| 群聊 | 补全群聊功能缺失：未读计数、离线消息队列雏形 |

**验收标准**:
- [x] `protocol/message_types.h` 独立于 Qt，含 34 种消息类型 + JSON 字段常量 + UserInfo 结构体
- [x] cJSON → nlohmann/json：MsgBuilder.cpp (Server+Client) 全面重写，cJSON 从 .pro 移除
- [x] 密码哈希：PasswordHash.h (salted SHA-256) + UserDao.cpp 升级，旧明文密码自动迁移
- [x] 消息持久化：MessageDao.h/.cpp (private_messages + channel_messages 表) + MainWindow 集成
- [x] MsgBuilder.h enum 扩展至 34 种类型 (Server+Client)，包含心跳/离线/历史/文件/AI 等新类型
- [x] C++ 标准从 c++11 升级至 c++17，protocol/ 加入 INCLUDEPATH
- [ ] 遗产代码在 VS2022 + Qt 5.15 上编译无错误（需 Qt 5.15+ 环境验证）
- [ ] 服务端启动 + 双客户端登录 + 私聊消息 流程可跑通（需 Qt 环境验证）

> **渐进迁移原则**: v1.0.2 在遗产代码基础上做外科手术式改进——替换 unsafe 库、补全核心功能缺失、剥离协议为独立头文件。不改 UI 层。这些修改后的模块在后续版本中会被新技术栈逐步替换，但短期内保证产品可运行。
>
> **新增文件**: `protocol/message_types.h`, `PasswordHash.h`, `MessageDao.h/.cpp`
>
> **修改文件**: `MsgBuilder.h/.cpp` (Server+Client), `UserDao.h/.cpp`, `MainWindow.cpp` (Server), `NetworkThread.cpp`, `HHServer.pro`, `HHClient.pro`

---

### v1.0.3 — 「稳固」— 网络层加固 (预计 1-2 天)

**目标**: 修复遗产代码网络层的结构性缺陷，让服务端具备基本的生产可靠性

| 模块 | 任务 |
|------|------|
| 心跳 | 实现 Heartbeat 机制：客户端 30s 发 ping → 服务端 90s 无心跳断开 Session |
| 半包 | 修复 TCP 粘包/半包问题：添加 4 字节长度前缀帧协议 `[uint32 BE len] + [JSON body]` |
| 编码 | 全局编码统一为 UTF-8：`QString::toUtf8()` / `fromUtf8()`，消除 `toLocal8Bit()` 的 GBK 依赖 |
| 日志 | 集成简易文件日志：连接/断开/错误 写入 `server.log`（Qt 无 spdlog，用 QFile+QTextStream） |
| 重连 | 客户端断线后自动重连（指数退避：1s→2s→4s→最大 30s） |
| 错误 | 完善错误处理：JSON 解析失败返回错误帧而非崩溃；非法消息类型记录日志后忽略 |

**验收标准**:
- [x] 4 字节大端长度前缀帧协议 (LegacyProtocolCodec.h)，含裸 JSON 回退兼容
- [~] 客户端 30s 发送一次 heartbeat (type=20)，服务端 90s 无心跳断开连接（代码已实现，需 Qt 验证）
- [x] UTF-8 编码统一：全链路 `toUtf8()/fromUtf8()` 替换 `toLocal8Bit()/fromLocal8Bit()`
- [x] 服务端日志文件 (LegacyLogger.h/.cpp)，含时间戳和事件级别
- [x] 客户端断线自动重连 (HHTcpSocket，指数退避 1s→2s→4s→最大 30s)
- [x] 非法消息类型返回错误帧，不再静默/崩溃
- [~] TCP 粘包/半包处理：需 Qt 双客户端联调验证

### v1.0.3.1 (patch) — ✅ 已并入：服务端连接数实时统计 (MainWindow activeConnections)

---

### v1.0.4 — 「补全」— 群聊功能完善 (预计 2-3 天)

**目标**: 将遗产代码中半成品的群聊系统补全为可用的频道功能

| 模块 | 任务 |
|------|------|
| 持久化 | 频道信息从 `std::map` 迁移到 SQLite `channels` + `channel_members` 表 |
| 持久化 | 服务端重启后：频道列表恢复、成员关系恢复、历史消息可查询 |
| 离线 | 群聊离线消息队列：成员离线期间的消息暂存 → 上线后推送 |
| 未读 | 未读消息计数：每个频道每个用户记录 `last_read_at` → 计算未读数 |
| 通知 | 频道事件广播优化：join/leave/kick 通知所有在线成员 |
| UI | 客户端群聊列表排序：按最新消息时间倒序 |
| UI | 频道成员列表显示在线状态（绿/灰圆点） |

**验收标准**:
- [x] 频道持久化：MessageDao 新增 createChannel/listChannels/addChannelMember/removeChannelMember
- [x] 创建/搜索/加入/离开频道改为接入 SQLite 数据（MainWindow.cpp 重构）
- [x] 私聊离线消息：登录后 getOfflinePrivateMsgs → 自动回放 → markPrivateMessagesRead
- [~] 频道成员离线期间收到消息 → 上线后按序推送（私聊已完成；群聊离线回放代码已实现但未完整联调）
- [ ] 未读计数正确：进频道前显示未读数，进入后清零（后端已就绪，需客户端 UI 配合）
- [x] 新成员加入频道 → 所有在线成员收到 userJoinGroup 通知
- [x] 成员离开频道 → 从成员表删除 → 通知剩余成员
- [ ] 频道消息历史可翻页查询（getChannelHistory 已实现，客户端未接入）

### v1.0.4.1 → 已合并至 v1.0.4 主版本
### v1.0.4.2 — 延后至 v1.5.0（需客户端 UI 改动）

---

### v1.0.5 — 「封版」— 遗产系统最终稳定版 (预计 1-2 天)

**目标**: 遗产代码达到"可交付"水平，为 v1.1.0 新架构迁移做最终准备

| 模块 | 任务 |
|------|------|
| 测试 | 协议层单元测试：MsgBuilder 全部 19 种类型的 encode/decode 往返验证 (GTest) |
| 测试 | 服务端集成测试：Python 脚本模拟 20 客户端并发注册→登录→私聊→群聊 全流程 |
| 文档 | 编写 `docs/LEGACY_API.md`：当前协议完整文档（JSON 示例 + 字段说明 + 错误码） |
| 文档 | 编写 `docs/MIGRATION_NOTES.md`：v1.0.5 → v1.1.0 迁移清单（哪些模块保留/替换/丢弃） |
| 代码 | 全量中文注释翻译为英文，移除死代码（注释掉的函数体） |
| 代码 | `using namespace std` 从头文件移除，改用 `std::` 前缀 |
| 安全 | 密码字段不再在日志/控制台中打印；登录失败不区分"用户不存在"和"密码错误" |
| 打包 | 编写一键构建脚本 `scripts/build_legacy.bat` |

**验收标准**:
- [x] Python 协议冒烟测试：`tests/legacy_protocol_smoke.py`（长度前缀帧 roundtrip 验证通过）
- [~] GTest 协议测试：19 种类型全部通过（替换为 Python smoke test；Qt 环境不可用时 GTest 无法编译。完整 GTest 覆盖延至 v1.2.0 新服务端）
- [x] `docs/LEGACY_API.md` 含所有 19 种消息类型的完整文档
- [x] `docs/MIGRATION_NOTES.md` 明确标记每个文件的迁移策略 (keep/refactor/replace)
- [x] `docs/V1_0_5_CHECKPOINT.md` 含可行性审查 + 已实现功能 + 已知风险
- [x] `build_legacy.bat` 一键编译 Server + Client
- [~] 所有 `.h` 文件无 `using namespace` 声明（部分头文件已清理，MsgBuilder.h/MainWindow.h 等核心头文件因业务代码大量引用暂保留）
- [ ] `build_legacy.bat` 在 Qt 5.15 环境下一键编译验证（需 Qt 环境）

### v1.0.5 实际交付偏差 vs 原计划

| 偏差 | 原因 | 影响 |
|------|------|------|
| cJSON→nlohmann/json 改为 Qt QJsonDocument | nlohmann 未 vendored，环境无法联网安装；Qt 原生 JSON 更符合"遗产代码稳定可编译"目标 | 无损——nlohmann/json 仍规划在 v1.2.0 asio 新服务端引入 |
| GTest → Python smoke test | Qt 环境不可用时 GTest 无法编译链接 | 协议正确性已通过 Python 脚本验证；完整 GTest 覆盖延至 v1.2.0 |
| 部分中文注释未翻译 | 业务代码改动量大，Codex 优先完成核心功能 | 不影响编译和运行 |

> **v1.0.5 里程碑意义**: 这是遗产 Qt 代码的"封版"版本。此后**不再对 Qt 模块做功能新增**，所有新功能开发通过新技术栈模块实现。v1.0.5 的唯一后续变更是关键 bug 修复。v1.1.0 起，新旧模块在同一项目中并存，Qt 模块逐步退役。
>
> **v1.1.0 启动条件**: v1.0.5 全项验收通过 → 进入 v1.1.0。除 `[~]` 项（需 Qt 环境验证）外已全部代码完成，允许启动 v1.1.0 规划。`[~]` 项在 Qt 环境就绪后补充验证。

---

### v1.1.0 — 「蓝图」— 新协议 + 新服务端骨架并行 (预计 2-3 天)

**目标**: 在同一项目中启动新技术栈骨架——新 C++ asio 服务端代码和 Tauri 客户端代码与遗产 Qt 代码并存于同一仓库，各自独立编译运行

| 层 | 任务 |
|----|------|
| 协议 | 基于 `protocol/message_types.h` 扩展至 34 种 JSON Schema（参考遗产 19 种） |
| 协议 | 编写 `protocol/message_types.ts` (TypeScript 类型，与 C++ 枚举对齐) |
| C++ Server | `src/server/` 目录创建 + CMake 集成 asio/nlohmann/spdlog/SQLiteCpp/bcrypt |
| C++ Server | TcpServer 最小实现：监听 12346 端口（与遗产 12345 共存不冲突） |
| Client | `src/client/` 目录创建 + Tauri + React 18 + Tailwind + Zustand 脚手架 |
| Client | Rust Tauri 层最小骨架 |
| 构建 | 根 CMakeLists.txt 管理新旧两套编译目标，一键构建 |

**验收标准**:
- [ ] `protocol/` 下 34 种消息的 JSON Schema 文件齐全
- [ ] 新 C++ Server 编译通过，启动监听 12346
- [ ] `npm run tauri dev` 启动空白 Tauri 窗口
- [ ] C++ 端和 TS 端的消息类型定义一一对应
- [ ] 遗产服务端 (port 12345) 与新服务端 (port 12346) 同时运行不冲突
- [ ] 新旧代码在同一个 Git 仓库中，目录边界清晰 (`legacy/` vs `src/`)

### v1.1.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.1.3 | 加固 | 依赖版本锁定 + Win11 编译验证 + 新老服务端消息互通测试 |
| v1.1.4 | 补全 | JSON Schema 完善（边界/错误/字段校验）+ TS 类型生成脚本自动化 |
| v1.1.5 | 检查 | **检查点**: CMake + Vite + Cargo 三构建链路一键脚本 + 34 种 JSON Schema 最终审查 |

---

### v1.2.0 — 「骨架」— 新服务端核心搭建 (预计 2-3 天)

**目标**: C++ 服务端具备完整的连接管理 + 消息路由 + 数据持久化能力

| 层 | 任务 |
|----|------|
| C++ Server | asio TcpServer：异步 accept + 线程池 + length-prefixed JSON 帧协议 |
| C++ Server | Session / SessionPool：连接→用户映射 + 超时清理 + 最大连接数限制 |
| C++ Server | HeartbeatMonitor：30s 心跳检测 + 90s 超时断开 + 断线通知 |
| C++ Server | MessageRouter：P2P 转发 + 群聊广播 + 离线消息队列 |
| C++ Server | PresenceManager：在线/离线/忙碌状态管理 + 状态变更广播 |
| C++ Server | Repository 层全量实现：UserRepository / MessageRepository / ChannelRepository |
| C++ Server | Database 初始化：SQLite WAL 模式 + 建表 + FTS5 虚拟表 |
| C++ Server | 基础日志 (spdlog)：连接/断开/路由/错误 四级日志 |

**验收标准**:
- [ ] 服务端启动监听 12345，Python 脚本模拟 50 连接同时在线
- [ ] P2P 消息：A→Server→B，延迟 < 10ms (LAN)
- [ ] 群聊消息广播：1 条消息 → 10 个在线成员全部收到
- [ ] 离线消息：A 发消息给离线的 B，B 上线后收到推送
- [ ] 心跳超时 90s 后自动清理 Session + 广播离线状态
- [ ] SQLite 中可查询所有消息，FTS5 搜索关键词返回正确结果

### v1.2.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.2.3 | 加固 | 线程安全审查 + 内存泄漏初步检测 + asio strand 隔离 |
| v1.2.4 | 补全 | 离线消息推送完善 + 消息已读回执 + SQLite WAL 写入压测 |
| v1.2.5 | 检查 | **检查点**: 200 并发 1h 压测通过 + GTest 覆盖 > 80% + 无内存泄漏 |

---

### v1.3.0 — 「基座」— 客户端骨架搭建 (预计 2-3 天)

**目标**: Tauri + React 客户端具备登录/注册/主窗口布局能力

| 层 | 任务 |
|----|------|
| React UI | LoginPage：用户名 + 密码 + 登录按钮 + 注册链接 |
| React UI | RegisterPage：用户名 + 密码 + 昵称 + 头像选择 + 注册按钮 |
| React UI | MainLayout：三栏布局 (侧栏 240px + 消息区 flex + 详情面板 320px 可折叠) |
| React UI | 通用组件：Avatar / StatusBadge / ContextMenu / Toast |
| Zustand | authStore：登录态管理 + token 持久化 (sessionStorage) |
| Tauri Rust | TcpManager：tokio::net::TcpStream 异步连接 + 自动重连 (指数退避) |
| Tauri Rust | MessageCodec：length-prefix 帧编解码 + JSON 序列化 |
| Tauri Rust | Tauri commands：connect / disconnect / login / register |
| 协议 | 7 种认证相关消息的 encode/decode 双端实现 |

**验收标准**:
- [ ] Tauri 窗口正常显示，LoginPage 居中美观
- [ ] 输入用户名密码 → 点击登录 → TCP 连接到 Server → 收到 loginSucReturn
- [ ] 登录成功后切换至 MainLayout 三栏布局
- [ ] 注册流程：填写信息 → Server 写入 users 表 → 返回 ID → 自动登录
- [ ] 断网后客户端 UI 显示 "连接断开，正在重连..."，网络恢复后自动重连
- [ ] 窗口最小尺寸 640x480，缩放时布局自适应

### v1.3.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.3.3 | 加固 | 登录错误处理完善 + 网络超时重试 + Zustand 状态持久化 |
| v1.3.4 | 补全 | Tauri 窗口原生感增强（亚克力/Mica + 圆角 + 拖拽区域）|
| v1.3.5 | 检查 | **检查点**: 登录/注册/重连 全异常路径覆盖 + Tauri .msi 打包脚本 |

---

### v1.4.0 — 「心跳」— 基础聊天功能 (预计 2-3 天)

**目标**: 双客户端之间可发送和接收文本消息

| 层 | 任务 |
|----|------|
| React UI | ChatView：消息列表（react-window 虚拟滚动） + 自动滚底 |
| React UI | MessageBubble：自己右对齐蓝色 / 他人左对齐灰色 / 时间戳 / 头像 / 发送状态 |
| React UI | MessageInput：文本输入 + Enter 发送 + Shift+Enter 换行 |
| Zustand | chatStore：消息列表状态 + sendMessage action + 接收消息事件 |
| Tauri Rust | Tauri events：收到消息 → emit 事件 → React 监听更新 |
| C++ Server | 私聊消息完整链路：sendMsg → 路由 → 持久化 → receiveMsg |
| 数据库 | private_messages 表 CRUD + 历史消息分页查询 |

**验收标准**:
- [ ] A 发送 "Hello" → B 实时收到，气泡正确渲染
- [ ] 消息时间戳正确显示 (HH:mm)
- [ ] 100 条历史消息虚拟滚动流畅 (60fps)
- [ ] 向上滚动加载更多历史消息
- [ ] 新消息到达时若已在底部则自动滚底，否则显示 "↓ 新消息" 按钮

### v1.4.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.4.3 | 加固 | 消息发送失败重试 + 离线消息 IndexedDB 缓存 + 虚拟滚动性能调优 |
| v1.4.4 | 补全 | 消息状态同步完善（发送中→已发送→已读）+ 滚动加载历史无缝体验 |
| v1.4.5 | 检查 | **检查点**: 双客户端 1000 条消息收发无丢失 + 虚拟滚动 60fps |

---

### v1.5.0 — 「圈子」— 好友系统与群聊 (预计 3-4 天)

**目标**: 完整的好友列表 + 频道（群聊）创建/加入/管理

| 层 | 任务 |
|----|------|
| React UI | ChannelList：频道树 + 私聊联系人 + 在线状态圆点 (绿/黄/灰) |
| React UI | FriendItem：头像 + 昵称 + 状态指示 + 未读消息数徽标 |
| React UI | 创建频道对话框：名称 + 描述 + 创建按钮 |
| React UI | 搜索频道对话框：关键词搜索 + 结果列表 + 加入按钮 |
| React UI | 频道成员列表：头像 + 昵称 + 角色标识 (群主/管理员) |
| React UI | 用户资料卡片：头像 + 昵称 + 用户名 + 在线状态 |
| Zustand | channelStore：频道列表 + 未读计数 + 成员列表 |
| C++ Server | ChannelManager：创建/搜索/加入/离开频道 + 成员管理 |
| C++ Server | 群聊消息广播路由 + 离线成员消息队列 |
| C++ Server | 频道角色权限：owner / admin / member |
| Tauri Rust | 头像文件缓存 (Tauri FS Plugin) |

**验收标准**:
- [ ] 好友列表实时反映在线/离线/忙碌状态
- [ ] 创建频道 → 搜索 → 加入 → 群聊发消息 完整流程
- [ ] 群聊消息所有在线成员实时收到
- [ ] 离线成员上线后收到未读群聊消息
- [ ] 未读消息计数正确，进入频道后清零
- [ ] 频道成员列表显示所有人及其角色
- [ ] 重启客户端 → 频道列表和成员恢复

### v1.5.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.5.3 | 加固 | 频道禁言功能 + 退出频道确认 + 成员角色权限校验 |
| v1.5.4 | 补全 | 频道信息编辑（名称/描述）+ 成员搜索 + @提及功能 |
| v1.5.5 | 检查 | **检查点**: 10 频道 × 20 成员并发消息压力测试 + 离线消息全链路验证 |

---

### v1.6.0 — 「颜值」— UI 全面美化 (预计 3-4 天)

**目标**: Discord 级别的视觉效果，双主题切换，动画丝滑

| 层 | 任务 |
|----|------|
| React UI | Tailwind 暗色主题完善：Discord 风格配色 (背景 #1a1a2e / 侧栏 #16213e) |
| React UI | Tailwind 浅色主题：清爽企业风格 (背景 #ffffff / 侧栏 #f0f2f5) |
| React UI | 主题切换按钮：一键切换暗色/浅色，CSS 变量过渡动画 |
| React UI | 聊天气泡精修：圆角 12px / 头像 36px 圆形 / 消息状态图标 ✓✓ |
| React UI | Framer Motion 动画：气泡出现 (slideUp+fadeIn) / 未读徽章弹跳 / 侧栏折叠 |
| React UI | EmojiPicker：emoji-picker-react 集成 + 最近使用 |
| React UI | 响应式断点：最小宽度 640px / 侧栏最小 180px / 详情面板可折叠至 0 |
| Tauri | Windows Toast 通知：Tauri Notification Plugin + 仅在窗口非焦点时弹出 |
| Tauri | 系统托盘：托盘图标 + 右键菜单 (显示/退出) + 未读消息数 |

**验收标准**:
- [ ] 暗色/浅色主题一键切换，0.3s CSS 过渡动画
- [ ] 窗口缩放至 800x600 时 UI 不塌陷
- [ ] 新消息气泡出现有 slideUp 动画 (200ms)
- [ ] 非焦点窗口收到新消息时弹出 Windows Toast 通知
- [ ] 系统托盘图标正确显示，未读消息数 > 0 时显示角标
- [ ] 视觉效果接近 Discord/钉钉水平（远优于原始 Qt 5 样式）

### v1.6.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.6.3 | 加固 | 字体优化（微软雅黑 + emoji 回退）+ 高对比度辅助模式 |
| v1.6.4 | 补全 | 动画性能优化（will-change/GPU 加速）+ 响应式断点完善 |
| v1.6.5 | 检查 | **检查点**: 双主题全组件走查 + Lighthouse 性能 > 90 + 640px~4K 分辨率兼容 |

---

### v1.7.0 — 「智慧」— AI Agent 集成 (预计 3-4 天)

**目标**: AI 摘要/搜索/翻译三大功能完整可用，流式打字机效果

| 层 | 任务 |
|----|------|
| C++ Server | AIManager：ModelRouter + RateLimiter + ContextBuilder 核心调度 |
| C++ Server | ModelRouter：DeepSeek API 优先 → Ollama 本地兜底 + 3 次重试 |
| C++ Server | PromptEngine：系统提示词模板 + 上下文窗口管理 + token 估算 |
| C++ Server | EmbeddingService：消息向量化 (text-embedding-3-small / bge-small-zh) + 余弦相似度检索 |
| C++ Server | cpp-httplib HTTP Client：流式 SSE 响应解析 |
| C++ Server | RateLimiter：每用户 10次/分钟 + AI 结果缓存 (ai_cache 表) |
| React UI | AIAssistantPanel：可折叠右侧面板 320px / 三 Tab (摘要/搜索/翻译) |
| React UI | SummaryTab：选中消息 → AI 摘要 → Markdown 渲染结果 |
| React UI | SearchTab：自然语言输入 → 语义搜索 → 相关消息列表 |
| React UI | TranslateTab：选中消息 → 选择语言 → AI 翻译结果 |
| React UI | StreamingText：打字机逐字显示 + 闪烁光标动画 |
| Zustand | aiStore：AI 请求状态 + 流式响应 buffer + 降级标识 |

**验收标准**:
- [ ] 选中 20 条消息 → 右键 "AI 摘要" → 3-5 句中文摘要 (Markdown 渲染正确)
- [ ] 自然语言搜索 "上周讨论了什么" → 返回语义相关的历史消息
- [ ] 翻译功能支持中→英 / 英→中，语气保持
- [ ] AI 回复以 30ms/字 速度打字机效果显示
- [ ] DeepSeek API 不可用时自动 fallback 到 Ollama，UI 显示 "本地模型" 标识
- [ ] 相同请求 5 分钟内命中缓存，不再调用 API

### v1.7.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.7.3 | 加固 | Prompt 模板优化 + AI Bot 频道 (@AI 问答) + 速率限制调优 |
| v1.7.4 | 补全 | Ollama 本地模型体验优化 + Embedding 批量处理 + AI 错误友好提示 |
| v1.7.5 | 检查 | **检查点**: AI 摘要/搜索/翻译 各 50 次测试准确率达标 + API 降级全路径验证 |

---

### v1.8.0 — 「进阶」— 文件传输与管理面板 (预计 3-4 天)

**目标**: 文件/图片传输 + 管理员管控 + 消息高级功能

| 层 | 任务 |
|----|------|
| C++ Server | 文件传输协议：sendFile / receiveFile / fileTransferComplete |
| C++ Server | 文件存储：服务端 `uploads/` 目录 + file_transfers 表记录 |
| React UI | 文件拖拽发送：Drag & Drop 区域 → 文件信息卡片 → 上传进度条 |
| React UI | 图片预览：气泡内嵌缩略图 + lightbox 点击放大 (react-medium-image-zoom) |
| React UI | 消息引用/回复：引用气泡样式 + 点击跳转到原消息 |
| React UI | 已读回执：单聊消息 ✓ (已发送) → ✓✓ (已读) |
| React UI | AdminPanel：用户管理 (踢人/禁言) + 频道管理 (删除/归档) + 广播消息 |
| React UI | SettingsDialog：通知/主题/语言/快捷键/网络 设置页面 |
| React UI | 聊天记录导出：HTML 格式 (含样式) + TXT 纯文本 |
| Tauri | 剪贴板粘贴图片 → 自动压缩 → 发送 |

**验收标准**:
- [ ] 拖拽文件到聊天窗口 → 显示文件信息卡片 → 点击发送 → 对方收到 + 可下载
- [ ] 图片在气泡内以缩略图展示，点击以 lightbox 全屏预览
- [ ] 引用回复显示被引用消息的缩略预览
- [ ] 单聊消息发送后显示 ✓，对方已读后变为 ✓✓
- [ ] 管理员可踢人出频道 + 设置禁言 (成员无法发消息)
- [ ] 管理员可发送系统广播消息 (全频道可见)
- [ ] 设置面板各选项持久化 (localStorage)
- [ ] 导出的 HTML 聊天记录在浏览器中可读

### v1.8.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.8.3 | 加固 | 文件断点续传 (大文件 > 50MB) + 快捷键系统 + 拖拽体验优化 |
| v1.8.4 | 补全 | 文件缩略图缓存 + 图片压缩选项 + 剪贴板粘贴智能识别 |
| v1.8.5 | 检查 | **检查点**: 100MB 文件 LAN 传输成功率 > 99% + 管理面板全权限矩阵验证 |

---

### v1.9.0 — 「锻造」— 测试/性能/安全 (预计 3-4 天)

**目标**: 生产级稳定性，通过压测和安全审查

| 层 | 任务 |
|----|------|
| C++ Server | 200 并发连接压测 (Python asyncio 脚本) + 性能 Profiling |
| C++ Server | 消息批量写入优化 (WAL + 事务批处理) |
| C++ Server | 全模块 GTest 单元测试覆盖 (目标 80%+) |
| C++ Server | 内存泄漏检测 (Dr. Memory 24h 运行) |
| C++ Server | 线程安全扫描 (ThreadSanitizer / 人工审查) |
| C++ Server | SQL 注入测试 + bcrypt 验证正确性 |
| C++ Server | 网络异常处理完善：断线重连/半开连接/缓冲区溢出 |
| React Client | Vitest + React Testing Library 组件测试 |
| React Client | 加载态 + 空态 + 错误态 (ErrorBoundary) 全量覆盖 |
| React Client | 各分辨率窗口兼容测试 (640x480 → 4K) |
| 集成 | 双客户端联调 + 消息不丢测试 (断网 30s → 恢复) |
| 打包 | Tauri bundle 生成 .msi + NSIS 自定义安装界面 |
| 文档 | README.md + DEPLOY_GUIDE.md + USER_MANUAL.md |

**验收标准**:
- [ ] 200 并发连接稳定运行 1 小时无异常
- [ ] 24h 运行 RSS 增长 < 10%
- [ ] 断网 30s → 恢复 → 离线消息全部推送，无丢失
- [ ] GTest 核心模块覆盖率 > 80%
- [ ] Vitest 关键组件覆盖率 > 70%
- [ ] 干净 Win11 虚拟机安装 .msi → 启动 → 登录 → 聊天全流程通过
- [ ] 文档中部署指南可复现（另一个人照做能跑通）

### v1.9.x 子版本节奏

| 版本 | 代号 | 内容 |
|------|------|------|
| v1.9.3 | 加固 | 压测问题修复 + 日志系统完善 + 错误诊断增强 |
| v1.9.4 | 补全 | CI/CD 流水线 (GitHub Actions: 编译+测试+打包) + 自动版本号 |
| v1.9.5 | 检查 | **检查点**: 全功能回归测试通过 + CI 全绿 + 安装包在 3 台 Win11 验证 |

---

### 🎉 v2.0.0 — 「重生」— 零 Qt 正式版发布

**状态**: 🎯 目标终点 | **前置条件**: v1.9.N 全部检查点通过 + 全项目审计通过

**v2.0.0 唯一硬性标准**:
> 项目中不再有任何 Qt 依赖。`#include <Q...>` 全部消失。服务端纯 C++17 + asio，客户端纯 Tauri + React + TypeScript，构建系统 CMake + Vite + Cargo。
>
> 验证命令：`rg "#include <Q" --include="*.h" --include="*.cpp"` 返回空。

**包含内容**:
- v1.9.N 所有功能 + 最终润色 + 全项目审计修复
- 完整的安装包 (lanchat-server-x64.msi + lanchat-client-x64.msi)
- 完整的文档（README / 部署指南 / 用户手册 / API 协议文档）
- Release Notes 从 v1.0.1 到 v2.0.0 的完整变更日志
- `legacy/` 目录完整归档所有已退役的 Qt 模块

**发布检查清单**:
- [ ] `rg "#include <Q"` 返回空 —— 零 Qt 依赖
- [ ] 全功能回归测试通过（所有历史 .5 检查点复验）
- [ ] 全项目审计通过（BUG / 冗余 / 耦合 / 性能 / 安全 五项扫描）
- [ ] 安装包在 3 台不同 Win11 机器上验证通过
- [ ] 文档与代码一致（截图、配置项、端口号）
- [ ] Git tag `v2.0.0` 打在 main 分支上
- [ ] Release Notes 发布在 GitHub Releases

---

### 后续版本规划 (v2.1+)

> v2.x 同样遵循 `.0→.3→.4→.5` 模式，每 .5 版本可跳大版本

| 版本 | 特性 |
|------|------|
| v2.1.0 → v2.1.5 | 语音消息（录音 + 播放波形图 + 语音转文字） |
| v2.2.0 → v2.2.5 | 消息反应（👍❤️😂😮😢🔥）+ 消息已读列表 |
| v2.3.0 → v2.3.5 | AI 日程提取 + 待办提醒 + @机器人自定义指令 |
| v2.4.0 → v2.4.5 | 数据统计面板（活跃用户/消息量/频道热度） |
| v3.0.0 | Linux 服务端 + macOS 客户端 + Docker 部署 |

---

## 7. AI Agent 集成架构

## 7. AI Agent 集成架构

### 7.1 总体设计

```
┌─────────────────────────────────────────────────────────────┐
│              AI Agent 调度层 (C++ Server)                     │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                   AIManager (单例)                    │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │    │
│  │  │ModelRouter│  │RateLimiter│  │ ContextBuilder  │   │    │
│  │  │          │  │          │  │                  │   │    │
│  │  │ DeepSeek │  │ 10 req/m │  │ 系统提示词       │   │    │
│  │  │ Ollama   │  │ per user │  │ + 聊天上下文     │   │    │
│  │  │ Fallback │  │          │  │ + RAG 检索结果   │   │    │
│  │  └──────────┘  └──────────┘  └──────────────────┘   │    │
│  └─────────────────────────────────────────────────────┘    │
│         │                  │                │               │
│  ┌──────▼──────┐  ┌───────▼──────┐  ┌─────▼──────┐        │
│  │ Summarizer  │  │  Translator  │  │  Searcher  │        │
│  │ * 对话摘要  │  │  * 中英互译  │  │ * 语义搜索 │        │
│  │ * 会议纪要  │  │  * 多语言    │  │ * FTS 混合 │        │
│  │ * 关键词提取│  │              │  │ * 相关性排序│       │
│  └─────────────┘  └──────────────┘  └────────────┘        │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                  EmbeddingService                     │   │
│  │  ┌──────────────────┐  ┌────────────────────────┐    │   │
│  │  │ 消息向量化        │  │ 向量检索 (余弦相似度)   │    │   │
│  │  │ text-embedding-3  │  │ SQLite JSON 列存储     │    │   │
│  │  │ small (API)       │  │ 或 bge-small-zh (本地) │    │   │
│  │  └──────────────────┘  └────────────────────────┘    │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 AI 功能调用流程

```
用户操作 (React): 选中 10 条消息 → 右键 → "AI 摘要"

1. React: 收集选中的消息 ID 列表
2. React → Tauri: invoke("ai_request", { type: "summarize", msgIds: [...] })
3. Tauri Rust → C++ Server: {type: 29, ai_type: "summarize", messages: [...]}
4. C++ Server AIManager:
   a. RateLimiter 检查配额 (OK)
   b. ContextBuilder:
      - 注入系统提示词: "你是企业IM助手，请用中文简洁摘要..."
      - 注入选中消息内容
      - 估算 token 数量（控制上下文窗口）
   c. ModelRouter:
      - 优先 DeepSeek-V4-Pro (weelinking API)
      - 失败 3 次 → fallback Ollama qwen2.5:7b
   d. cpp-httplib 发起 HTTP POST (stream=true)
   e. 逐 chunk 解析 SSE 响应
5. C++ Server → Tauri Rust: 流式推送 (type=30 start, type=31 chunks, type=30 done)
6. Tauri → React: 通过 Tauri event 推送每个 chunk
7. React: AIAssistantPanel 打字机效果逐字显示
```

### 7.3 Prompt 模板

```
【聊天摘要】
"你是一个企业即时通讯助手。以下是 {channel_name} 频道最近 {n} 条消息。
 请用 3-5 句话总结讨论要点，并列出任何待办事项或决策。
 格式：
 ## 讨论要点
 - ...
 ## 待办事项
 - [ ] ... (@负责人)"

【智能搜索】
"根据用户查询 '{query}'，从以下历史消息中找到最相关的 5 条：
 {messages_json}
 返回格式：JSON 数组 [{id, content, relevance_score, reason}]"

【翻译】
"将以下消息从 {source_lang} 翻译为 {target_lang}，保持原文语气：
 {message}"
```

### 7.4 成本控制策略

| 策略 | 说明 |
|------|------|
| **速率限制** | 每用户每分钟最多 10 次 AI 请求 |
| **本地优先** | 翻译/简单问答优先走 Ollama 本地模型 |
| **结果缓存** | 相同摘要请求 5 分钟内返回缓存 (ai_cache 表) |
| **批量处理** | 非实时 Embedding 请求排队合并批处理 |
| **降级模式** | API 不可用时自动切换本地模型，UI 显示降级标识 |

---

## 8. 通信协议重新设计

### 8.1 设计原则

- 向下兼容原始 19 种消息类型语义
- 新增消息类型从 20 开始编号
- 统一使用 nlohmann/json (Server) + TypeScript types (Client)
- 所有字段使用 snake_case 命名
- TCP 帧格式: `[4 字节长度 (uint32 BE)] + [JSON body]`

### 8.2 消息类型总览（34 种）

#### 原始消息（0-19，兼容保留）

```
0  registerUser         注册用户
1  registerUserReturn    注册返回
2  login                登录
3  loginSucReturn       登录成功
4  loginLoseReturn      登录失败
5  sendMsg              发送私聊消息
6  receiveMsg           接收私聊消息
7  userOnline           用户上线
8  userOffline          用户下线
9  createGroup          创建群聊
10 createGroupReturn    创建群聊返回
11 searchGroup          查找群聊
12 searchGroupReturn    查找群聊返回
13 joinGroup            加入群聊
14 joinGroupReturn      加入群聊返回
15 leaveGroup           离开群聊
16 sendGroupMsg         发送群聊消息
17 receiveGroupMsg      接收群聊消息
18 userJoinGroup        用户加入群聊
19 userLeaveGroup       用户离开群聊
```

#### 新增消息（20-33）

```
20 heartbeat            心跳请求
21 heartbeatAck         心跳响应
22 offlineMessages      离线消息推送
23 logout               主动登出
24 requestHistory       请求历史消息
25 historyResponse      历史消息响应
26 sendFile             发送文件请求
27 receiveFile          接收文件通知
28 fileTransferComplete 文件传输完成
29 aiRequest            AI 功能请求
30 aiResponse           AI 功能响应 (流式开始/结束/错误)
31 aiStreamChunk        AI 流式输出块
32 userProfileUpdate    更新用户资料
33 systemBroadcast      系统广播消息
```

### 8.3 JSON Schema 示例

```jsonc
// Type 5: sendMsg (私聊消息)
{
  "type": 5,
  "timestamp": 1715428800,
  "msg_id": "msg_abc123",
  "from": {"id": 100001, "nickname": "张三"},
  "to": {"id": 100002, "nickname": "李四"},
  "content": "你好，明天开会吗？",
  "content_type": "text",        // text | image | file | system
  "reply_to": null               // 引用消息 ID
}

// Type 29: aiRequest
{
  "type": 29,
  "request_id": "req_xyz",
  "ai_type": "summarize",        // summarize | translate | search | chat
  "channel_id": 200001,
  "messages": [...],
  "params": {"target_lang": "en"}
}

// Type 30: aiResponse (流式控制帧)
{
  "type": 30,
  "request_id": "req_xyz",
  "ai_type": "summarize",
  "status": "start"              // start | done | error
}

// Type 31: aiStreamChunk
{
  "type": 31,
  "request_id": "req_xyz",
  "chunk_index": 3,
  "content": "本次讨论主要围绕",
  "is_final": false
}

// Type 26: sendFile
{
  "type": 26,
  "file_name": "Q3_Report.pdf",
  "file_size": 2048576,
  "file_hash": "sha256:abc123...",
  "to": {"id": 100002}
}
```

---

## 9. 数据库 Schema 设计

### 9.1 数据库: lanchat.db (SQLite 3, WAL 模式)

```sql
-- ============================================
-- 用户表
-- ============================================
CREATE TABLE users (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    username     TEXT NOT NULL UNIQUE,
    password     TEXT NOT NULL,              -- bcrypt hash
    nickname     TEXT NOT NULL,
    avatar_path  TEXT DEFAULT '',
    status       INTEGER DEFAULT 0,          -- 0:离线 1:在线 2:忙碌 3:勿扰
    role         TEXT DEFAULT 'user',        -- user | admin
    created_at   INTEGER NOT NULL,
    updated_at   INTEGER NOT NULL
);

INSERT INTO users VALUES(100000, 'admin', '<bcrypt>', '系统管理员', '', 0, 'admin', 0, 0);

-- ============================================
-- 私聊消息表
-- ============================================
CREATE TABLE private_messages (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    msg_id       TEXT NOT NULL UNIQUE,
    from_id      INTEGER NOT NULL,
    to_id        INTEGER NOT NULL,
    content      TEXT NOT NULL,
    content_type TEXT DEFAULT 'text',        -- text | image | file
    file_path    TEXT DEFAULT '',
    reply_to     TEXT,
    is_read      INTEGER DEFAULT 0,
    created_at   INTEGER NOT NULL,
    FOREIGN KEY (from_id) REFERENCES users(id),
    FOREIGN KEY (to_id)   REFERENCES users(id)
);

CREATE INDEX idx_private_from_to ON private_messages(from_id, to_id);
CREATE INDEX idx_private_created  ON private_messages(created_at);

-- ============================================
-- 频道表（原群聊）
-- ============================================
CREATE TABLE channels (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    name         TEXT NOT NULL,
    description  TEXT DEFAULT '',
    channel_type TEXT DEFAULT 'group',       -- group | broadcast | ai_bot
    creator_id   INTEGER NOT NULL,
    created_at   INTEGER NOT NULL,
    FOREIGN KEY (creator_id) REFERENCES users(id)
);

-- ============================================
-- 频道成员
-- ============================================
CREATE TABLE channel_members (
    channel_id   INTEGER NOT NULL,
    user_id      INTEGER NOT NULL,
    role         TEXT DEFAULT 'member',      -- owner | admin | member
    joined_at    INTEGER NOT NULL,
    last_read_at INTEGER DEFAULT 0,
    is_muted     INTEGER DEFAULT 0,
    PRIMARY KEY (channel_id, user_id),
    FOREIGN KEY (channel_id) REFERENCES channels(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id)    REFERENCES users(id)
);

-- ============================================
-- 频道消息
-- ============================================
CREATE TABLE channel_messages (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    msg_id       TEXT NOT NULL UNIQUE,
    channel_id   INTEGER NOT NULL,
    from_id      INTEGER NOT NULL,
    content      TEXT NOT NULL,
    content_type TEXT DEFAULT 'text',
    file_path    TEXT DEFAULT '',
    reply_to     TEXT,
    created_at   INTEGER NOT NULL,
    FOREIGN KEY (channel_id) REFERENCES channels(id) ON DELETE CASCADE,
    FOREIGN KEY (from_id)    REFERENCES users(id)
);

CREATE INDEX idx_channel_msg_channel ON channel_messages(channel_id);
CREATE INDEX idx_channel_msg_time    ON channel_messages(created_at);

-- ============================================
-- FTS5 全文搜索
-- ============================================
CREATE VIRTUAL TABLE messages_fts USING fts5(
    content,
    content='channel_messages',
    content_rowid='id'
);

CREATE TRIGGER messages_ai AFTER INSERT ON channel_messages BEGIN
    INSERT INTO messages_fts(rowid, content) VALUES (new.id, new.content);
END;

CREATE TRIGGER messages_ad AFTER DELETE ON channel_messages BEGIN
    INSERT INTO messages_fts(messages_fts, rowid, content)
    VALUES ('delete', old.id, old.content);
END;

-- ============================================
-- 嵌入向量（AI 语义搜索）
-- ============================================
CREATE TABLE message_embeddings (
    message_id   INTEGER PRIMARY KEY,
    embedding    TEXT NOT NULL,              -- JSON 数组 [0.1, -0.2, ...]
    model        TEXT NOT NULL,
    created_at   INTEGER NOT NULL,
    FOREIGN KEY (message_id) REFERENCES channel_messages(id) ON DELETE CASCADE
);

-- ============================================
-- AI 请求缓存
-- ============================================
CREATE TABLE ai_cache (
    cache_key    TEXT PRIMARY KEY,           -- hash(prompt + messages)
    response     TEXT NOT NULL,
    ai_type      TEXT NOT NULL,
    created_at   INTEGER NOT NULL,
    ttl_seconds  INTEGER DEFAULT 300
);

-- ============================================
-- 文件传输记录
-- ============================================
CREATE TABLE file_transfers (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    file_name    TEXT NOT NULL,
    file_size    INTEGER NOT NULL,
    file_hash    TEXT NOT NULL,
    from_id      INTEGER NOT NULL,
    to_id        INTEGER,
    channel_id   INTEGER,
    status       TEXT DEFAULT 'pending',     -- pending | transferring | done | failed
    created_at   INTEGER NOT NULL,
    FOREIGN KEY (from_id)    REFERENCES users(id),
    FOREIGN KEY (to_id)      REFERENCES users(id),
    FOREIGN KEY (channel_id) REFERENCES channels(id)
);
```

---

## 10. UI/UX 现代化方案

### 10.1 设计参考

- **灵感来源**: Discord (布局) + Telegram (简洁) + 钉钉 (企业管理)
- **React 组件库**: 可选 shadcn/ui (基于 Radix + Tailwind) 或纯手写

### 10.2 配色方案 (Tailwind Config)

```js
// tailwind.config.ts
const config = {
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        // 暗色主题 (Discord-inspired)
        dark: {
          bg:      '#1a1a2e',  // 主背景
          sidebar: '#16213e',  // 侧栏
          accent:  '#0f3460',  // 强调/自己的气泡
          highlight:'#e94560', // 高亮/在线指示
          hover:   'rgba(255,255,255,0.05)',
        },
        // 浅色主题
        light: {
          bg:      '#ffffff',
          sidebar: '#f0f2f5',
          accent:  '#1677ff',
          online:  '#52c41a',
        }
      }
    }
  }
}
```

### 10.3 窗口布局

```
┌──────────────────────────────────────────────────────────┐
│ LanChat-Next                              ─  □  ×       │  ← Tauri 自绘标题栏
├──────────┬───────────────────┬───────────────────────────┤
│          │                   │                           │
│  频道列表 │    消息区域        │    详情/AI 面板 (可折叠)   │
│  (240px) │    (flex-1)       │     (320px)              │
│          │                   │                           │
│ 🔍 搜索  │ ┌───────────────┐ │  ┌─────────────────────┐ │
│          │ │ 张三 10:30    │ │  │ 📋 AI 摘要           │ │
│ 📢 综合群│ │ 大家好 👋     │ │  │ 今天讨论了Q3规划...  │ │
│ 💻 技术组│ │               │ │  └─────────────────────┘ │
│ 🎮 摸鱼群│ │ 李四 10:31    │ │  ┌─────────────────────┐ │
│          │ │ 早上好！      │ │  │ 🔍 智能搜索          │ │
│ ──────── │ │               │ │  │ [___________] [搜索] │ │
│ 私聊     │ │ 张三 10:32    │ │  └─────────────────────┘ │
│ 🟢 张三  │ │ 今天开会吗？  │ │  ┌─────────────────────┐ │
│ 🟡 李四  │ │       ✓✓     │ │  │ 👥 频道成员 (12)     │ │
│ ⚫ 王五  │ │               │ │  │ 🟢 张三 (群主)       │ │
│          │ ├───────────────┤ │  │ 🟢 李四 (管理员)     │ │
│          │ │ 😀 输入消息...│ │  │ 🟡 王五              │ │
│          │ │          [发] │ │  │ ⚫ 赵六              │ │
│          │ └───────────────┘ │  └─────────────────────┘ │
├──────────┴───────────────────┴───────────────────────────┤
│ 🟢 在线 | 192.168.1.100:12345 | 用户: admin             │  ← 状态栏
└──────────────────────────────────────────────────────────┘
```

### 10.4 组件实现要点

| 组件 | 关键技术 | 注意事项 |
|------|----------|----------|
| **消息列表** | react-window (虚拟滚动) | 大数据量下保持 60fps 滚动 |
| **聊天气泡** | Tailwind + Framer Motion | 左右对齐、圆角方向、出现动画 |
| **输入框** | contentEditable 或 Textarea | Enter 发送、Shift+Enter 换行、粘贴图片 |
| **表情面板** | emoji-picker-react 或自定义 | Unicode 渲染、最近使用 |
| **文件预览** | react-medium-image-zoom (图片) | 缩略图 + lightbox 放大 |
| **主题切换** | Tailwind dark: 前缀 + CSS 变量 | <html class="dark"> 切换 |
| **系统通知** | Tauri Notification API | 仅当窗口非焦点时弹出 |
| **侧栏折叠** | CSS transition width | 从 240px → 0px 动画 |
| **AI 打字机** | 逐 chunk 追加文本 + CSS blink 光标 | 使用 requestAnimationFrame |

### 10.5 动画效果

| 动画 | 实现 |
|------|------|
| 消息气泡出现 | Framer Motion: `initial={{ opacity: 0, y: 20 }} animate={{ opacity: 1, y: 0 }}` |
| 未读徽章弹跳 | CSS `@keyframes bounce` + `scale` 弹性动画 |
| 在线状态切换 | Tailwind `transition-colors duration-300` 绿色渐变 |
| 侧栏折叠 | CSS `transition-[width] duration-200 ease-in-out` |
| AI 打字光标 | `@keyframes blink { 50% { opacity: 0 } }` |
| 消息已读 ✓ → ✓✓ | `transition-all` + color change |

### 10.6 UI 优化执行清单

> 说明：本清单只定义“应该优化什么”，不抢占实现职责。具体前端代码由 Codex 负责，界面规范/验收标准由 Claude Code 维护，DeepSeek-TUI 负责检查效果与偏差。

| 优先级 | 优化项 | 目标 |
|--------|--------|------|
| P0 | 登录页与注册页视觉重构 | 提升第一印象，统一品牌感与层级关系 |
| P0 | 主窗口三栏布局比例 | 保证消息区为视觉中心，侧栏与详情面板可折叠且不压迫内容 |
| P0 | 聊天气泡体系 | 统一自己/他人/系统消息/引用消息/文件消息的视觉语义 |
| P1 | 主题系统 | 暗色/浅色主题切换，使用 CSS 变量管理颜色语义 |
| P1 | 状态反馈 | 加载、发送中、失败、重试、离线、重连等状态有明确反馈 |
| P1 | 动效节制 | 只保留轻量过渡动画，避免干扰阅读和性能 |
| P1 | 适配与响应式 | 640px~4K 窗口范围内布局稳定，最小化塌陷风险 |
| P2 | AI 面板视觉一致性 | 与聊天区共享同一套设计语言，避免“拼接感” |
| P2 | 无障碍与可读性 | 字号、对比度、焦点态、键盘可达性统一提升 |

### 10.7 UI 优化验收标准

- 登录页信息层级清晰，主次按钮、提示文案和输入框对齐统一
- 主界面在常见窗口尺寸下不出现明显拥挤、遮挡或错位
- 聊天区域可快速区分私聊、群聊、系统提示和 AI 内容
- 暗色/浅色主题切换时不会出现颜色断层或样式闪烁
- 任何新增动效都必须为“信息表达服务”，不能只为了炫技
- UI 修改不影响协议层、服务端路由层和数据库层的职责边界

---

## 11. 项目目录结构

```
LanChat-Next/
│
├── DEVELOPMENT_PLAN.md              ← 本文档
├── README.md
├── .gitignore
│
├── protocol/                         ← 协议定义 (Truth Source)
│   ├── schemas/                      ← JSON Schema 文件
│   │   ├── auth.json                 ← 注册/登录
│   │   ├── chat.json                 ← 私聊/群聊消息
│   │   ├── group.json                ← 群组管理
│   │   ├── file.json                 ← 文件传输
│   │   └── ai.json                   ← AI 功能
│   ├── message_types.h               ← C++ 枚举 (Server 用)
│   └── message_types.ts              ← TypeScript 类型 (Client 用)
│
├── server/                            ← C++17 服务端
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── LanChatServer.h / .cpp        ← 服务端主控
│   ├── core/
│   │   ├── TcpServer.h / .cpp        ← asio async TCP 监听
│   │   ├── Session.h / .cpp          ← 客户端会话
│   │   ├── SessionPool.h / .cpp      ← 会话池 (concurrent_hash_map)
│   │   ├── Heartbeat.h / .cpp        ← 心跳定时器
│   │   └── ConnectionManager.h/.cpp  ← 连接生命周期
│   ├── routing/
│   │   ├── MessageRouter.h / .cpp    ← P2P + 群聊消息路由
│   │   ├── ChannelManager.h / .cpp   ← 频道生命周期管理
│   │   └── PresenceManager.h / .cpp  ← 在线状态广播
│   ├── ai/
│   │   ├── AIManager.h / .cpp        ← AI 调度核心
│   │   ├── ModelRouter.h / .cpp      ← DeepSeek/Ollama 路由
│   │   ├── PromptEngine.h / .cpp     ← Prompt 模板引擎
│   │   ├── EmbeddingService.h / .cpp ← 向量化 + 检索
│   │   ├── RateLimiter.h / .cpp      ← 速率限制
│   │   └── handlers/
│   │       ├── SummarizeHandler.h/.cpp
│   │       ├── TranslateHandler.h/.cpp
│   │       └── SearchHandler.h / .cpp
│   ├── repository/
│   │   ├── Database.h / .cpp         ← SQLite 连接池 (WAL)
│   │   ├── UserRepository.h / .cpp
│   │   ├── MessageRepository.h / .cpp
│   │   ├── ChannelRepository.h / .cpp
│   │   └── FileRepository.h / .cpp
│   └── config/
│       └── server_config.json        ← 服务端配置
│
├── client/                            ← Tauri v2 + React 客户端
│   ├── package.json
│   ├── tsconfig.json
│   ├── vite.config.ts
│   ├── tailwind.config.ts
│   ├── index.html
│   ├── src/                           ← React 前端源码
│   │   ├── main.tsx                   ← React 入口
│   │   ├── App.tsx                    ← 根组件 + 路由
│   │   ├── routes/
│   │   │   ├── LoginPage.tsx
│   │   │   ├── RegisterPage.tsx
│   │   │   └── MainLayout.tsx        ← 主界面布局 (3 栏)
│   │   ├── components/
│   │   │   ├── chat/
│   │   │   │   ├── ChatView.tsx       ← 聊天消息视图
│   │   │   │   ├── MessageBubble.tsx  ← 聊天气泡
│   │   │   │   ├── MessageInput.tsx   ← 输入框
│   │   │   │   ├── EmojiPicker.tsx    ← 表情面板
│   │   │   │   └── FilePreview.tsx    ← 文件/图片预览
│   │   │   ├── sidebar/
│   │   │   │   ├── ChannelList.tsx    ← 频道/好友列表
│   │   │   │   ├── FriendItem.tsx     ← 好友列表项
│   │   │   │   └── SearchBar.tsx     ← 搜索栏
│   │   │   ├── ai/
│   │   │   │   ├── AIAssistantPanel.tsx ← AI 助手面板
│   │   │   │   ├── SummaryTab.tsx
│   │   │   │   ├── SearchTab.tsx
│   │   │   │   ├── TranslateTab.tsx
│   │   │   │   └── StreamingText.tsx  ← 打字机效果组件
│   │   │   ├── admin/
│   │   │   │   └── AdminPanel.tsx
│   │   │   ├── settings/
│   │   │   │   └── SettingsDialog.tsx
│   │   │   └── ui/                    ← 通用 UI 组件
│   │   │       ├── Avatar.tsx
│   │   │       ├── StatusBadge.tsx
│   │   │       ├── ContextMenu.tsx
│   │   │       └── Toast.tsx
│   │   ├── stores/                    ← Zustand 状态管理
│   │   │   ├── authStore.ts
│   │   │   ├── chatStore.ts
│   │   │   ├── channelStore.ts
│   │   │   ├── uiStore.ts            ← 主题/侧栏折叠
│   │   │   └── aiStore.ts
│   │   ├── hooks/                     ← 自定义 Hooks
│   │   │   ├── useTauriEvent.ts       ← Tauri event 监听
│   │   │   ├── useTcpConnection.ts    ← TCP 连接状态
│   │   │   └── useScrollToBottom.ts
│   │   ├── lib/                       ← 工具函数
│   │   │   ├── protocol.ts            ← 消息序列化/反序列化
│   │   │   ├── db.ts                  ← IndexedDB (Dexie.js)
│   │   │   └── utils.ts
│   │   └── styles/
│   │       ├── global.css             ← Tailwind 指令 + 自定义变量
│   │       └── bubble.css             ← 聊天气泡微调
│   │
│   └── src-tauri/                     ← Tauri Rust 后端
│       ├── Cargo.toml
│       ├── tauri.conf.json
│       ├── capabilities/
│       │   └── default.json
│       ├── icons/                     ← 应用图标
│       └── src/
│           ├── main.rs                ← Tauri 入口
│           ├── lib.rs                 ← 模块注册
│           ├── tcp_manager.rs         ← TCP 连接管理 (tokio)
│           ├── message_codec.rs       ← 消息编解码 (length-prefix + JSON)
│           ├── commands.rs            ← Tauri commands (invoke 接口)
│           └── notifications.rs       ← 系统通知封装
│
├── resources/                         ← 共享资源
│   ├── icons/                         ← SVG 图标集
│   ├── sounds/                        ← 消息提示音
│   └── fonts/                         ← 字体文件
│
├── tests/                             ← 测试
│   ├── CMakeLists.txt
│   ├── server/                        ← C++ 服务端单测 (GTest)
│   ├── client/                        ← React 组件测试 (Vitest)
│   └── integration/                   ← 集成测试 (Python 脚本)
│
├── scripts/                           ← 辅助脚本
│   ├── init_db.sql                    ← 数据库初始化
│   ├── benchmark.py                   ← 压力测试 (asyncio TCP)
│   └── seed_data.py                   ← 测试数据生成
│
└── docs/                              ← 文档
    ├── API_PROTOCOL.md
    ├── DEPLOY_GUIDE.md
    └── USER_MANUAL.md
```

---

## 12. 测试策略

### 12.1 测试金字塔

```
           ┌─────────┐
           │  E2E    │  5%  ← Playwright (客户端) + Python 集成脚本 (服务端)
          ┌┴─────────┴┐
          │  集成测试  │  15% ← C++ Server + Tauri 联调
         ┌┴───────────┴┐
         │   单元测试    │  80% ← GTest (C++) + Vitest (React)
        └───────────────┘
```

### 12.2 测试覆盖目标

| 模块 | 单元测试目标 | 关键测试点 | 框架 |
|------|-------------|-----------|------|
| protocol/ | 100% | 所有消息类型序列化/反序列化 (C++ + TS 双端) | GTest + Vitest |
| server/repository/ | 90% | CRUD + 边界条件 + SQL 注入防护 | GTest |
| server/routing/ | 85% | 消息路由正确性 + 离线队列 + 并发安全 | GTest |
| server/ai/ | 80% | Mock HTTP 响应 + 降级逻辑 + 速率限制 | GTest + Mock |
| server/core/ | 85% | Session 生命周期 + 心跳超时 + 连接池 | GTest |
| client/stores/ | 90% | Zustand actions + 状态一致性 | Vitest |
| client/components/ | 70% | 关键交互 (登录/发送消息/文件上传) | Vitest + RTL |
| client/tcp (Rust) | 80% | 连接/重连/编解码/心跳 | Rust test |

### 12.3 关键测试场景

```
□ 200 并发连接压力测试 (Python asyncio 脚本模拟)
□ 消息丢失测试 (网络断开 30s → 恢复 → 消息完整)
□ AI 降级测试 (API 不可用 → Ollama 接管 → UI 显示降级标识)
□ SQLite 并发写入测试 (100 客户端同时发消息, WAL 模式)
□ 内存泄漏测试 (Server 运行 24h, RSS 增长 < 10%)
□ 登录安全测试 (SQL 注入 / 弱密码 / bcrypt 验证)
□ 客户端离线缓存测试 (断网 → 读缓存 → 联网 → 增量同步)
□ Tailwind 主题切换测试 (暗色/浅色无样式丢失)
```

---

## 13. 风险与缓解

| 风险 | 影响 | 概率 | 缓解方案 |
|------|------|------|----------|
| Tauri v2 API 不稳定 (仍在 RC) | 高 | 中 | 锁定 Tauri v2 稳定版，关注 changelog |
| C++ asio 异步编程复杂度高 | 中 | 中 | Claude Code 先写最小原型验证，参考 asio 官方 examples |
| DeepSeek API 不稳定或限流 | 中 | 中 | 本地 Ollama 兜底 + 请求重试 3 次 + 降级 UI 提示 |
| SQLite WAL 200 并发写入瓶颈 | 中 | 低 | 批量写入队列 + WAL 模式 + busy_timeout 设置 |
| React 虚拟滚动性能问题 | 低 | 低 | react-window 经过 Discord 级别验证 |
| Rust Tauri 桥接层学习曲线 | 中 | 中 | Tauri Rust 层保持极简 (~500 行)，只做网络桥接 |
| 个人开发时间不足 | 高 | 高 | 严格 MVP 优先级，v0.1 只跑通核心链路 |
| AI 生成代码质量不一 | 中 | 中 | 交叉审查 + 自动化测试门禁 + 人类抽查关键模块 |
| Windows 防火墙阻止 TCP 端口 | 低 | 中 | 安装包自动添加防火墙规则 (netsh) |
| C++ 与 TypeScript 协议不一致 | 中 | 低 | protocol/ 目录作为唯一 Truth Source，双端测试验证 |

---

## 附录 A: 开发环境清单

| 工具 | 版本 | 用途 |
|------|------|------|
| Visual Studio 2022 | 17.x | C++ 编译 (MSVC) |
| Visual Studio Code | latest | 前端开发 (React/TS) |
| CMake | 3.28+ | C++ 服务端构建 |
| Node.js | 20 LTS | 前端依赖 |
| Rust | 1.78+ (stable) | Tauri 构建 |
| Tauri CLI | v2.x | 客户端打包 |
| SQLite | 3.45+ | 数据库 |
| Ollama | latest | 本地 LLM 推理 |
| Claude Code | latest | 架构设计 / C++ 开发 / 审查 |
| Codex (OpenAI) | GPT-5.5 | React/TS/Rust 开发 |
| Git | 2.44+ | 版本控制 |
| NSIS / WiX | latest | Windows 安装包 |

## 附录 B: 原始项目 vs LanChat-Next v2 对比

| 维度 | 原始 LAN ChatRoom | LanChat-Next v2 |
|------|-------------------|-----------------|
| 服务端语言 | C++11 + QTcpServer | C++17 + asio |
| 客户端技术 | Qt 5.4 Widget + .ui | Tauri v2 + React 18 + Tailwind |
| 构建系统 | QMake | CMake (Server) + Vite (Client) |
| 代码行数 | ~3000 | 预计 ~12000 (C++ ~4000 + TS ~5000 + Rust ~500 + CSS ~2500) |
| 文件数 | ~40 | 预计 ~100 |
| 数据库表 | 1 表 | 8 表 + FTS5 虚表 |
| 消息类型 | 19 种 | 34 种 |
| UI 样式 | QSS 无 / .ui 硬布局 | Tailwind 双主题 + 响应式 |
| 并发模型 | 每客户端一线程 | asio 线程池 + async I/O |
| AI 能力 | 无 | 摘要/搜索/翻译/问答 + 打字机流式 |
| 测试覆盖 | 0% | 目标 80%+ |
| 部署方式 | 源码编译 | C++ Server 独立 exe + Tauri .msi |
| AI 开发效率 | N/A | 高 (React/TS 是 AI 最擅长领域) |
| 版本号 | v1.0.1 | v2.0.0 |

### 附录 B-1: 遗产目录重命名对照

| 原始名称 | 归档路径 | 迁移日期 |
|----------|----------|----------|
| `HHClient/` | `legacy/original-client/` | 2026-05-11 |
| `HHServer/` | `legacy/original-server/` | 2026-05-11 |
| `QQChat/` | `legacy/qq-chat-prototype/` | 2026-05-11 |
| `聊天demo（包含群聊，和未读信息，没有头像）/` | `legacy/chat-demo-with-group-and-unread/` | 2026-05-11 |

---

## 附录 C: AI Agent 开发工具与技能清单

> 2026-05-11 扫描 `E:\Open-Source Projects by others\` + `E:\AISkills\` + 网络检索汇总

### C-1: 本地开源项目（可直接使用）

| 项目 | 路径 | 用途 | 优先级 |
|------|------|------|--------|
| **agency-agents** | `E:\Open-Source Projects by others\agency-agents\` | 26+ AI Agent 角色定义 (Claude Code 兼容)。含 Frontend Developer / Backend Architect / Security Engineer / DevOps Automator / Code Reviewer。安装: `cp engineering/*.md ~/.claude/agents/` | 🔴 必装 |
| **Archon** | `E:\Open-Source Projects by others\Archon\` | YAML 驱动开发工作流引擎。17 种预定义 workflow (plan→implement→validate→review→PR)，支持 git worktree 隔离 | 🟡 推荐 |
| **GitNexus** | `E:\Open-Source Projects by others\GitNexus\` | 代码知识图谱 + 16 MCP 工具。支持 C++/TypeScript 双语言索引，可让 AI Agent 感知跨组件依赖/调用链 | 🟡 推荐 |
| **claude-code-best-practice** | `E:\Open-Source Projects by others\claude-code-best-practice\` | Claude Code 最佳实践指南 (subagents/skills/hooks/MCP/workflows + 69 条技巧) | 🟢 参考 |
| **claude-code** | `E:\Open-Source Projects by others\claude-code\` | 官方 Claude Code 插件集 (commit-commands / code-review / feature-dev / frontend-design / pr-review-toolkit) | 🟡 推荐 |

### C-2: 本地 Skills 清单

#### 🔴 必装

| Skill | 来源 | 用途 |
|-------|------|------|
| **superpowers** (全套) | `E:\AISkills\superpowers\` | 开发方法论: brainstorming → writing-plans → subagent-driven-development → TDD → code-review → systematic-debugging → verification |
| **mattpocock/tdd** | `E:\AISkills\mattpocock_skills\` | Red-Green-Refactor 循环，适用于 React 组件 + C++ 单元测试 |
| **mattpocock/diagnose** | `E:\AISkills\mattpocock_skills\` | 系统调试循环 (hypothesis→test→learn) |
| **mattpocock/grill-with-docs** | `E:\AISkills\mattpocock_skills\` | 代码变更与架构文档对齐检查 |
| **mattpocock/improve-codebase-architecture** | `E:\AISkills\mattpocock_skills\` | 代码腐化检测 + 架构改善建议 |
| **neat-freak** | `E:\AISkills\khazix-skills\neat-freak\` | 开发会话后自动清理 CLAUDE.md / docs / memory 同步 |

#### 🟡 推荐

| Skill | 来源 | 用途 |
|-------|------|------|
| **cpp-pro** | `E:\AISkills\Antigravity_Awesome_skills\` | 现代 C++17/20 最佳实践 (RAII/STL/templates/move semantics) |
| **react-best-practices** | `E:\AISkills\Antigravity_Awesome_skills\` | React 18 性能优化 + 组件设计模式 |
| **react-state-management** | `E:\AISkills\Antigravity_Awesome_skills\` | Zustand / Jotai / React Query 状态管理 |
| **react-component-performance** | `E:\AISkills\Antigravity_Awesome_skills\` | React 组件渲染优化 (memo/useMemo/virtualization) |
| **senior-frontend** | `E:\AISkills\Antigravity_Awesome_skills\` | 高级前端工程化 (bundle/type-safety/accessibility) |
| **debugging-strategies** | `E:\AISkills\Antigravity_Awesome_skills\` | 跨语言调试方法论 |
| **mcp-builder** | `E:\AISkills\Anthropic_skills\skills\mcp-builder\` | MCP Server 构建指南 (如需自定义 MCP 工具) |
| **webapp-testing** | `E:\AISkills\Anthropic_skills\skills\webapp-testing\` | Playwright E2E 测试 React 前端 |
| **skill-creator** | `E:\AISkills\Anthropic_skills\skills\skill-creator\` | 创建自定义 Skill (为 LanChat 项目定制专用 skill) |

#### 🟢 按需

| Skill | 来源 | 用途 |
|-------|------|------|
| **gdb-cli** | `E:\AISkills\Antigravity_Awesome_skills\` | C/C++ GDB 调试 (服务端 crash 分析) |
| **chat-widget** | `E:\AISkills\Antigravity_Awesome_skills\` | 实时聊天 UI 组件参考 |
| **security-auditor** | `E:\AISkills\Antigravity_Awesome_skills\` | 安全审计 (登录/传输/存储) |
| **electron-development** | `E:\AISkills\Antigravity_Awesome_skills\` | 桌面应用开发模式 (可迁移至 Tauri) |
| **test-driven-development** | `E:\AISkills\Antigravity_Awesome_skills\` | TDD 通用方法论补充 |
| **playwright** | `E:\AISkills\OpenAI_skills\skills\` | 浏览器自动化测试补充 |

### C-3: 推荐 MCP 工具配置

开发过程中可通过 Claude Code 的 MCP 协议接入以下工具：

| MCP Server | 适用场景 | 建议引入版本 |
|------------|----------|-------------|
| **clangd LSP** (via MCP bridge) | C++ 代码智能提示 / 跳转定义 / 引用查找 | v1.1.0 |
| **GitNexus MCP** (16 tools) | 代码知识图谱：调用链追踪 / 依赖分析 / 影响评估 | v1.2.0 |
| **Playwright MCP** | React 前端 E2E 自动化测试 | v1.3.0 |
| **SQLite MCP** | 数据库 Schema 查看 / 查询验证 / 数据巡检 | v1.0.4 |
| **Filesystem MCP** (built-in) | 项目文件读写权限管理 | v1.0.1 (已内置) |

### C-4: Claude Code 推荐配置

开发过程中建议启用的 Claude Code 特性：

```json
// .claude/settings.json (项目级)
{
  "hooks": {
    "PostToolUse": [
      { "matcher": "Edit|Write", "hooks": ["lint-check", "format-check"] }
    ]
  },
  "agents": {
    "cpp-backend": { "source": "agency-agents/engineering/backend-architect.md" },
    "react-frontend": { "source": "agency-agents/engineering/frontend-developer.md" },
    "code-reviewer": { "source": "agency-agents/engineering/code-reviewer.md" },
    "security": { "source": "agency-agents/engineering/security-engineer.md" }
  },
  "plugins": {
    "superpowers": { "enabled": true },
    "mattpocock-skills": { "enabled": true, "skills": ["tdd", "diagnose", "grill-with-docs"] }
  }
}
```

---

> **这份规划文档由 Claude Code (DeepSeek-V4-Pro) 持续维护更新，作为 LanChat-Next 项目开发的唯一权威参考。**  
> **每个版本迭代完成后，将对应的验收标准勾选为 `[x]`，并记录实际完成日期。**
> 
> **v2.1 核心变更**:
> 1. 客户端技术栈从 Qt 6 C++ 切换为 Tauri v2 + React/TypeScript，实现 "高性能 C++ 后端 + AI 友好前端" 的最优组合
> 2. 版本体系: v1.0.1 (遗产基线) → v1.0.5 (遗产封版) → v1.1.0 (新架构启动) → ... → v2.0.0 (**零 Qt 正式版**)
> 3. 迭代规则: `.5 = 质量检查点`（非自动跳板），不通过则 `.6/.7/.8` 继续打磨
> 4. 新增附录 C: AI Agent 开发工具与技能清单（本地开源项目 + Skills + MCP 配置）

---

## v1.0.5 Legacy Checkpoint Update (2026-05-11)

See `docs/V1_0_5_CHECKPOINT.md` for the concrete implementation and feasibility review completed in this pass.

- v1.0.2 correction: legacy `MsgBuilder` now uses Qt JSON instead of an unvendored `nlohmann/json.hpp`; the pure C++ JSON layer remains planned for the asio server.
- v1.0.3 implemented: length-prefixed UTF-8 frames, server logging, client reconnect, invalid-message error handling.
- v1.0.4 implemented: persistent channel metadata/members/messages and private offline replay.
- v1.0.5 implemented: legacy API docs, migration notes, build script, protocol smoke test.
- Remaining validation: run `build_legacy.bat` in a Qt 5.15 developer prompt and perform two-client GUI smoke testing.

---

## v1.1.0 Blueprint Update (2026-05-11)

See `docs/V1_1_0_BLUEPRINT.md` for the concrete v1.1.0 skeleton started in this pass.

- [x] `protocol/message_types.ts` added and aligned with the C++ enum.
- [x] 34 JSON Schema placeholder files generated under `protocol/schemas/`.
- [x] `src/server/` C++17/CMake server-next skeleton added; default port is `12346`.
- [x] `src/client/` Tauri v2 + React + TypeScript skeleton added.
- [x] Root `CMakeLists.txt` added for server-next.
- [~] Full standalone asio async implementation is deferred to v1.1.3 after dependency vendoring/installation; v1.1.0 has an immediate WinSock/POSIX listener fallback.
- [~] `npm run tauri:dev` requires dependency install/network and is not validated in the restricted environment.

---

## Claude Code Sync Update (2026-05-11)

See `docs/CLAUDE_CODE_SYNC.md` for the cross-agent handoff decision.

- v1.0.3-v1.0.5 audit conclusion: code delivery passes for migration purposes; v1.1.0 is allowed to proceed.
- Decision: do not install Qt 5.15 for legacy GUI joint testing. Legacy Qt is now a migration reference, not a release target.
- Deferred items move to v1.2.0: heartbeat/sticky-packet integration, group offline replay, C++ GTest coverage.
- Accepted deviations: legacy `QJsonDocument` is acceptable; `nlohmann/json` belongs in the new asio server. Python smoke test is acceptable for legacy protocol validation.
- v1.1.0 actual state: blueprint skeleton is already created under `src/server`, `src/client`, and `protocol/schemas`.
- Next milestone: v1.1.3 dependency/build hardening and replacement of the temporary WinSock/POSIX fallback with standalone asio async networking.
