# Windows 完整联机测试包 2026.11.23.1

发布记录：2026-11-23（Asia/Shanghai）。

## 本次内容

- LanChat 文字状态自由提问，由 Console 获取实板快照并调用配置的模型接口。
- 长消息气泡完整显示；关闭按钮悬停红色、移开恢复。
- 登录后左下角齿轮进入服务器与板端设置；登录页保留精简服务器地址。
- 群聊、PC 图片 / GIF / 文件、设备关键词查询和 USB 日志继续可用。
- S3 ESP-IDF 6.0.1 兼容源码另在 RoboMind-S3 主分支发布；安装包不含固件刷写工具。

## 输出与校验

维护者本机构建目录：`build/windows-installer-20261123-ai-ui/`；二进制产物不随 Git 源码提交。

| 文件 | 字节数 | SHA-256 |
|---|---:|---|
| `LanChat-Next-完整联机测试包-2026.11.23.1.zip` | 36,835,013 | `9ea731abffb879fc71508df0437fdc8d150b60b18ca5a9034c23d644c40b3a80` |
| `LanChat-Next-Setup-2026.11.23.1-x64.exe` | 37,333,123 | `5a9c99e521616e21a16306aea1e07d1b60407588047418b98167e9093f13ab19` |

ZIP 含安装器、`安装与使用说明.txt`、`发给测试者-连接信息.txt`、`installer-manifest.json` 四件。可直接将完整 ZIP 发给测试者；异地测试需先取得 Tailscale 访问权限，再用 LanChat 自由注册、凭群 ID 加入。模型接口和密钥由桥接主机配置，普通测试者无需各自配置 AI 密钥。

包内不含个人账号配置、密钥、数据库、附件缓存、串口日志、固件备份或 QtTest。未做代码签名；Windows 显示发布者未知时可用上述哈希核对文件。

构建时 LanChat 源码为 `bfc146235da7b1b81104a60e33f5ccaf0ec88a8c`，相关源码路径干净。Console 功能提交为 `b1a2678`，安装目录测试入口为 `c6836fa`。本页与 README 为产物验收后的文档更新；原始安装 manifest 保留构建时信息。

## 验证范围

Qt 15/15、Console Anthropic / OpenAI 模拟器与原状态 / 日志回归通过。实际安装器在隔离目录核对 60 个 payload 文件、开始菜单快捷方式、Qt / 服务端双客户端 LAN 通信、Console 两种模拟模型接口、重复安装、卸载及个人文件保留；运行阶段已移除开发 Qt PATH。

ZIP 四件分别与原文件哈希一致，payload 中客户端、Console 和服务端与实际构建产物一致。详细本机证据：`release-verification.json`、`build/windows-installer-validation-ai-ui/7da72967fcc949c4abc5beb220e416e1/`。

这轮未进行第二台物理 PC 的 Tailscale 联机、人工窗口吸附 / 多屏或新的板端 / 付费模型验收；此前真板 SDK 和问答记录继续保留。设置扩展、语音和小智集成见[后续计划](settings-roadmap.md)。
