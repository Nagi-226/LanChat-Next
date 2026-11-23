# 商汤日日新：DeepSeek V4.1 Flash 1017

核对与联调：2026-11-23（Asia/Shanghai）。模型推理仍由 PC / Console 调用外部服务，S3 提供真实状态；语音与角色界面另见[沉浸模式方案](whale-immersion-plan.md)。

## 配置

登录后进入 **左下角齿轮 → 设置 → 板端连接 → AI 状态问答**：

| 字段 | 值 |
|---|---|
| 接口协议 | OpenAI 兼容 |
| 接口地址 | `https://token.sensenova.cn/v1` |
| 模型 | `deepseek-flash` |
| 思考强度 | `none`，日常短问答初版采用快速响应 |
| API 密钥 | 用户自己的商汤平台密钥，保存在本机 Windows DPAPI 加密配置中 |

勾选启用，**保存并连接**，在专用板端账号私聊自然提问；配置群用 `/AI 问题`。普通知识、技术和学习问题可以回答；设备事实仍依据当前快照，离线、过期和未计算不能虚构为正常。`感知 / 链路 / 设备 / 帮助` 和 `/日志` 继续走确定性查询。

官方文档将 `deepseek-flash` 标注为 **DeepSeek V4.1 Flash 1017**。这是 API 模型 ID，显示名称不用作为请求参数；`deepseek-v4-flash` 是列表中的另一个 ID，不能混用来宣称验证了同一版本。渠道是商汤，模型是平台提供的 DeepSeek；不是在 ESP32-S3 运行模型。

`POST /v1/chat/completions` 支持 OpenAI 兼容消息格式与 Bearer API key。该模型默认开启思考，Chat API 使用 `reasoning_effort: "none"` 关闭思考；当前 Console 保持每请求 1200 输出 token / 45 秒超时，日常问答优先使用 none。较高思考强度会占用输出预算、增加等待，支持程度也随提供方变化。选择“按模型默认”时不发送这一可选参数；Anthropic 请求不发送 OpenAI 的 reasoning_effort 字段。

本机已通过 LanChat 可见设置页切换并复核 `provider=openai`、上述地址 / 模型、`reasoning_effort=none`、加密凭据存在，Console 为 enabled / configured。此前的 DeepSeek Anthropic 配置以加密形式保留在本机私有配置目录；密钥、备份和账号信息不进入源码或分发包。

## 已验证与边界

- 用户授权少量真实接口联调。模型列表 HTTP 200，包含 `deepseek-flash`。
- 短中文直连 HTTP 200，返回一句 LAN / Internet 区别，finish_reason=stop，usage 为 19 输入 / 12 输出 / 31 总 token，推理 token=0。
- 真实 LanChat 服务端 + 新 Console + 临时账号 / 隔离数据库完成一次模型调用：解释一般知识并明确报告实际板端离线，requested=1、replied=1、failed=0，聊天历史仅一条机器人回复，耗时约 6.8 秒。
- Qt 15/15、两种模型协议的本机模拟器回归通过，包括可选思考参数、状态真实性、会话隔离、判重、错误和凭据不进入正文 / 日志。
- 本轮 S3 没有恢复 Wi-Fi 在线，也没有新增刷写或语音播放验收；不能把上面的接口 / LAN 结果写成真板语音问答完成。

本机证据位于 `build/sensenova-20261123/` 与 Console 的 `build/ai-validation/sensenova-lan-smoke.json`。提供了 `tools/test_ai_provider_smoke.py --allow-paid` 供已获授权的单次真实渠道复验；默认拒绝付费联调，需要显式配置环境与授权参数。不得为检查配置反复自动请求模型。

思考强度与一般知识问答支持已部署，并纳入 2026.11.23.2 的主分支发布范围；完整包和验收见[发布记录](windows-release-20261123.md)。密钥不随任何包分发。

后续生产请求曾出现 HTTP 429，新版按服务商错误字段归类为限流并冷却；早先一次 HTTP 200 不能证明当前持续可用，详见[设置说明](settings-roadmap.md)。用户决定下一次使用自己的 DeepSeek 官方 API 尝试，本轮没有切换或调用官方渠道。

官方资料：[商汤平台文档](https://platform.sensenova.cn/docs)。版本名称依据当日文档；别名后续版本映射由服务方决定。
