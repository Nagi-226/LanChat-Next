# LanChat-Next Qt6 原生客户端设计（取代 Tauri 套壳）

日期：2026-11-19 · 状态：待用户审阅 · 方案：A（Qt6 Widgets + 移植 legacy 业务逻辑 + 新 QSS 主题）

## 0. 目标与非目标

**目标**：用本机 Qt 6.10.3（mingw_64）做一个原生聊天客户端，完全取代 Tauri 套壳；
登录标识改为用户名（`Nagi_226`），管理员密码保持 `965163` 不变；
开发者 Console 入口隐藏到开发者模式之后（非开发者模式不可见、不可开）。

**第一版范围（用户已裁定）**：登录/注册/私聊/群聊/好友/在线状态/离线消息/历史 + 本地消息搜索。
**非目标**：LLM 摘要与 provider/key 管理（P2）；QML；跨平台验证（仅 Windows）；E2EE；
服务端除登录标识外的任何改动。

## 1. 协议与用户名登录（服务端 bounded 改动）

- `Login.id` 接受 **integer 或 string**。string 按 nickname 查 `UserRepository::findByNickname`
  解析为内部数字 id；integer 走原路径。两条路径汇合后逻辑完全不变。
- `protocol/schemas/login.schema.json` 的 `id` 改为 `oneOf: [integer>=1, string 1..64]`；
  `protocol/message_types.h` 注释同步说明。**内部 id 保持数字**：fromId/toId/group.members 等
  全协议仍是整数，改字符串会破坏所有现有消息类型，不做。
- 注册不变（nickname 即登录名，唯一性已保证抢不走 `Nagi_226`）。
  admin / `is_admin` / announce 权限门 / 密码哈希（v2 预哈希）**全部不动**；不重种子。
- 界面一律显示 nickname；数字 id 仅在开发者模式可见。
- 验收（独立审计客户端，非自证）：`Nagi_226`+`965163` → type 3；错密码 → 401；
  不存在用户名 → 401；数字 id 1000 登录仍 → type 3；非 admin announce 仍 403。

## 2. Qt 客户端架构（新目录 `src/client-qt/`）

CMake + Qt6::Widgets/Network/Gui（不引 Quick）。四层，各自可独立测试：

- `net/`：`QTcpSocket` + 独立 `FrameBuffer`（`[u32 BE len]+JSON`，半包/粘包/坏头处理，
  坏头清缓冲重对齐，与服务端 FrameCodec 同语义但客户端独立实现）；
  会话状态机 Disconnected→Connecting→LoggingIn→Ready；
  指数退避重连 1→2→4→8→16→30s 上限 30s（连上复位）；heartbeat type 20 每 10s，收 21 确认。
- `protocol/`：QJsonDocument 编解码；直接 include 框架无关的 `protocol/message_types.h`；
  subset 类型 0-19、20/21、22（离线）、24/25（历史）、33（announce/typing）、42（好友在线）。
- `model/`：contacts / groups / messages 内存模型 + 本地搜索索引（对已收与历史消息建
  substring 索引，对应现 UI 的 Local Search；不联网）。
- `ui/`：LoginView / RegisterView / MainView（左联系人与群、中聊天、右可折叠面板）+
  消息气泡 delegate + 变量化暗色 QSS。

**服务端进程管理**（沿用 Tauri 现行为）：启动探测 127.0.0.1:12346；不通则 spawn exe 旁
`lanchat_server_next.exe --port 12346 --data <统一目录>`；统一目录 =
`%APPDATA%/LanChat-Next/data`（与已落地的 lib.rs 约定一致）；客户端退出 kill 子进程。

**开发者模式**：QSettings ini `developer_mode=false` 默认。Console 按钮/协议日志/数字 id
仅该模式渲染。**开启**途径：快捷键 Ctrl+Shift+D，或直接改 ini（README 记载）；
设置页里的开发者模式开关**仅在已进入该模式时显示**（用于关闭），避免"隐藏开关无法首次开启"的死锁。
非开发者模式无可见入口。

## 3. UI 重做与参考来源

布局对齐现 React 版截图但重绘：顶栏仅 主题/语言/窗口控制（**Console 撤出明面**）；
左 CONTACTS/GROUPS 栏、中聊天区、右可折叠面板（本地搜索在；AI 面板 P2 不渲染）；
登录标识字段标签 = "用户名"。

色板 token（取自现截图观感）：bg `#151A23`、panel `#1B212C`、accent `#E8465A`、
text `#E6E9F0`、muted `#8B93A7`、online `#35C46A`。变量化于 QSS 生成处，单点改色。

参考与许可证分级（GPL 只借设计不搬代码）：
- **QDarkStyleSheet（MIT）**：代码级基底 QSS，覆写上述 token。
- **qtmodern（MIT）**：无边框窗口 + 自绘标题栏模式。
- **qt-material（BSD-2-Clause）**：设计级 token/间距参考。
- **Telegram Desktop（GPLv3）/ Chatterino2（许可证未核实）/ NeoChat、Quaternion（GPLv3）**：
  仅设计级——气泡几何、留白、消息列表渲染、暗色层次。不复制其代码。

## 4. Tauri 处置

`git mv src/client legacy/tauri-client`（保留历史，不删除）；根构建脚本不再构建它；
npm/vite 彻底退出客户端路径。服务端 `src/server` 除第 1 节外零改动。不 push。

## 5. 验证与 UNKNOWN

- 服务端：ctest 8/8 保持 + 第 1 节审计用例全过。
- Qt 侧：protocol 单测（frame roundtrip / 坏头 / 半包 / 粘包，QtTest 或 ctest）；
  offscreen `QWidget::grab()` 存 PNG 供目检 UI；真窗口交用户试 登录/私聊/群/搜索/开发者模式隐藏。
- 独立审计客户端继续作为非自证验收器（扩展 username 用例）。
- **UNKNOWN 如实标**：LLM 摘要 P2 未做；仅 Windows 验证；Chatterino2 许可证未核实（故仅设计参考）；
  Qt5→Qt6 移植清扫中可能暴露的 legacy 隐藏缺陷（发现即报，不静默修业务语义）。

## 6. 验收清单

1. 用户名 `Nagi_226` + `965163` 登录成功；数字 id 登录不回归。
2. 私聊/群聊/好友/在线状态/离线/历史/本地搜索 可用（对现服务端）。
3. 非开发者模式：Console 不可见不可开；开发者模式开启后可见。
4. Tauri 客户端不再被构建；Qt 客户端单 exe + sidecar 可跑。
5. 服务端 ctest 8/8 + 审计 username 用例全过；Qt protocol 单测全过。
6. 无 push；spec 与实现提交留在本地待用户裁决。