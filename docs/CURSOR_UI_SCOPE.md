# Cursor/Gemini 3.1 Pro — Legacy Qt UI Beautification Scope

> **有效期**: 2026-05-11 ~ 2026-05-25 (Cursor 到期日)  
> **接手方**: Claude Code (DeepSeek-V4-Pro) — 到期后无缝接管  
> **主战场**: v1.0.3+ 并行 UI 美化轨道（不影响 Codex 后端工作）

---

## 1. 硬边界 — 绝对不碰

```
⛔ legacy/original-server/        ← 服务端（Codex 在改）
⛔ protocol/                       ← 协议定义
⛔ DEVELOPMENT_PLAN.md             ← 规划文档
⛔ legacy/original-client/*.cpp    ← C++ 业务逻辑（Codex 在改）
⛔ legacy/original-client/*.h      ← C++ 头文件（Codex 在改）
⛔ legacy/original-client/MsgBuilder.*  ← 协议编解码
⛔ legacy/original-client/HHTcpSocket.* ← 网络层
⛔ 任何 Makefile / .pro / CMakeLists   ← 构建系统不变
```

## 2. 允许范围 — 仅限以下

```
✅ legacy/original-client/*.ui    ← 9 个 Qt UI 布局文件
✅ legacy/original-client/*.qss   ← 新建 QSS 样式表（目前不存在）
✅ legacy/original-client/*.qrc   ← 资源文件（如需添加图标/字体）
```

### 具体 9 个 .ui 文件

| 文件 | 用途 | 优先级 |
|------|------|--------|
| `logindialog.ui` | 登录窗口 | 🔴 高 |
| `registerdialog.ui` | 注册窗口 | 🔴 高 |
| `mainwindow.ui` | 主界面（好友列表+频道列表） | 🔴 高 |
| `chatdialog.ui` | 私聊窗口（含气泡区域） | 🔴 高——当前最丑 |
| `groupchatdialog.ui` | 群聊窗口 | 🟡 中 |
| `frienditem.ui` | 好友列表项 | 🟡 中 |
| `creategroupdialog.ui` | 创建群聊弹窗 | 🟢 低 |
| `searchgroupdialog.ui` | 搜索群聊弹窗 | 🟢 低 |
| `MsgMarke.ui` | 消息标记 | 🟢 低 |

## 3. 设计规范

### 3.1 暗色主题（默认）

```css
/* 参考: Discord + Telegram 暗色模式 */
背景主色:     #1a1a2e
侧栏底色:     #16213e
卡片/气泡自己: #0f3460
卡片/气泡他人: #1e2a45
高亮/强调色:   #e94560
在线状态绿:    #52c41a
忙碌状态黄:    #faad14
离线状态灰:    #666666
文字主色:      #e0e0e0
文字次级:      #a0a0a0
边框/分割线:   rgba(255,255,255,0.06)
```

### 3.2 浅色主题（切换）

```css
背景主色:     #ffffff
侧栏底色:     #f0f2f5
卡片/气泡自己: #1677ff
卡片/气泡他人: #f0f0f0
高亮/强调色:   #e94560
在线状态绿:    #52c41a
文字主色:      #1a1a1a
文字次级:      #888888
边框/分割线:   #e8e8e8
```

### 3.3 尺寸规范

```
窗口最小尺寸:     800 × 600 (mainwindow)
侧栏宽度:         240px (最小 180px)
聊天气泡最大宽度:  70% 的聊天区域
聊天气泡圆角:     12px (自己: 右下 2px, 他人: 左下 2px)
头像尺寸:         40px 圆形 (列表项) / 36px 圆形 (气泡旁)
输入框高度:       最低 40px，最大 120px (自动扩展)
按钮圆角:         6px
字体:             "Microsoft YaHei" 14px (正文) / 12px (时间戳)
```

### 3.4 组件状态

```css
/* 按钮 */
QPushButton {
    background: #e94560; color: white; border: none;
    border-radius: 6px; padding: 8px 24px; font-weight: bold;
}
QPushButton:hover   { background: #d63850; }
QPushButton:pressed { background: #c23045; }
QPushButton:disabled { background: #555; color: #888; }

/* 输入框 */
QLineEdit, QTextEdit {
    background: #16213e; border: 1px solid #0f3460;
    border-radius: 8px; color: #e0e0e0; padding: 8px 12px;
}
QLineEdit:focus, QTextEdit:focus {
    border-color: #e94560;
}

/* 列表 */
QListWidget {
    background: transparent; border: none; outline: none;
}
QListWidget::item {
    padding: 8px 12px; border-radius: 6px; margin: 2px 8px;
}
QListWidget::item:hover    { background: rgba(255,255,255,0.05); }
QListWidget::item:selected { background: rgba(233,69,96,0.25); }

/* 滚动条 */
QScrollBar:vertical {
    width: 6px; background: transparent;
}
QScrollBar::handle:vertical {
    background: rgba(255,255,255,0.15); border-radius: 3px;
    min-height: 30px;
}
```

## 4. 禁止事项

| 禁止 | 原因 |
|------|------|
| 修改任何 .cpp / .h 文件 | Codex 正在改这些文件，冲突风险 |
| 添加新的 Qt Widget 类 | 需要改 .pro 和 #include，超出边界 |
| 改 QMainWindow 的 objectName | 业务代码通过 objectName 查找控件 |
| 删现有控件 | 只改样式和布局，不删业务依赖的控件 |
| 加外部依赖库 | Qt 5.4 环境不保证有 |
| 改信号/槽连接 | 在 C++ 代码中定义，属于 Codex 范围 |

## 5. 交付物清单

- [ ] `dark_theme.qss` — 暗色主题样式表（~300 行）
- [ ] `light_theme.qss` — 浅色主题样式表（~300 行）
- [ ] `logindialog.ui` — 重设计登录窗口
- [ ] `registerdialog.ui` — 重设计注册窗口
- [ ] `mainwindow.ui` — 三栏布局主窗口
- [ ] `chatdialog.ui` — 聊天气泡+输入区重设计
- [ ] `groupchatdialog.ui` — 群聊窗口
- [ ] `frienditem.ui` — 好友列表项（头像+状态+未读）
- [ ] `creategroupdialog.ui` — 创建群聊弹窗
- [ ] `searchgroupdialog.ui` — 搜索群聊弹窗
- [ ] `MsgMarke.ui` — 消息标记组件
- [ ] `heads.qrc` — 更新资源引用（如添加 SVG 图标）

## 6. 交接协议（5月25日到期时）

### 6.1 每日自动同步

Cursor 每次修改 `.ui` / `.qss` 文件后，将**实际效果截图**（3-5 张关键界面）和**修改摘要**更新到同一个 checkpoint 文件：

```
docs/CURSOR_CHECKPOINT.md
```

格式：
```markdown
## 2026-05-XX 进度

### 今天改了什么
- [文件名] — 具体改动（1 句话）

### 当前状态
- 登录界面: 🟢 完成 / 🟡 进行中 / 🔴 未开始
- 主窗口:   🟢 完成 / 🟡 进行中 / 🔴 未开始
- ...

### 阻塞问题
- 无 / 遇到 XXX 问题，需要 Claude 帮忙查 .cpp 文件
```

### 6.2 到期交接清单

5月25日到期前，确保以下事项完成：

| 交接项 | 状态 |
|--------|------|
| 所有 .ui 和 .qss 文件已提交到项目目录 | |
| `CURSOR_CHECKPOINT.md` 最终更新，标记每个文件完成状态 | |
| 记录 1-3 个已知但未解决的 UI 问题 | |
| 如需修改 .cpp 才能完成的 UI 效果，记录为 "Pending C++ changes" | |
| Claude Code 可以直接 `Read` 所有产出文件并继续修改 | |

### 6.3 Claude Code 接手流程

```bash
# Claude Code 5月26日接手时执行：
1. Read docs/CURSOR_CHECKPOINT.md  ← 了解进度
2. Read legacy/original-client/*.qss  ← 审查样式
3. Read legacy/original-client/*.ui  ← 审查布局
4. 对照 DEVELOPMENT_PLAN.md v1.6.0 验收标准检查
5. 继续未完成的 UI 文件
```

## 7. 与 Codex v1.0.3 的并行流水线

```
时间线 →
           
Codex (v1.0.3):     [网络加固] [心跳机制] [TCP帧协议] [UTF-8编码] [日志系统]
Cursor (UI并行):    [QSS主题]  [登录窗口] [主窗口布局] [聊天气泡] [群聊窗口]
                    
                    ← 互不干扰，各自在独立文件范围内工作 →
                    
冲突点: 无。.qss 在运行时加载，不影响 C++ 编译；.ui 的 objectName 不变则 C++ 代码不受影响。
唯一需要同步的点: 如果 Cursor 需要改某个控件的 objectName，先发到 CURSOR_CHECKPOINT.md 标记。
```

---

> **最后叮嘱 Cursor/Gemini**: 你的工作范围是 `legacy/original-client/*.ui` 和 `*.qss`。改样式、改布局都行，但别碰 .cpp/.h。如果某个 UI 效果**必须通过改 C++ 代码才能实现**（比如自定义绘制、动画），记录到 CHECKPOINT 文件，由 Claude Code 后续实现。
