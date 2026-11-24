# 鲸鱼娘的本机少女声线

2026-11-24。源码与本机版本 **2026.11.24.2** 增加可选本地 VITS 语音。当前使用中文派蒙社区模型作为鲸鱼娘的初版少女声线；并非 ZipZipPipe 官方鲸鱼娘配音。文字回答仍由用户配置的 DeepSeek 等接口生成，语音在桥接 PC 本地合成，S3 只接收 16kHz 单声道 PCM。

## 使用

登录后进入 **左下角齿轮 → 设置 → 板端连接 → 连接与屏幕 → 鲸鱼娘沉浸模式**，选择：

- **自动选择**：本机已安装少女模型时使用本地 VITS；没有安装时使用 Windows 中文语音。
- **Windows 中文语音**：使用 Windows 已安装的中文声音。
- **派蒙风格少女（本地 VITS）**：当前中文少女声线；未安装时选项禁用。

切换声线会停止当前语音并重连板端桥。进入沉浸模式后在专用板端账号私聊提问，回答自动转为字幕与语音。其他 PC 无须安装声音模型，只需连接 LanChat 主机；合成工作由连接 S3 的桥接 PC 承担。无需新的云 TTS 密钥或按次语音费用。

## 找到的项目与实际用途

| 项目 | 实际内容 | 本轮选择 |
|---|---|---|
| [大肥鱼 AstrBot](https://github.com/whyever77/dafeiyu-astrbot) | 社区大肥鱼声音演示、阿里云百炼音色复刻与 Qwen TTS 接入；演示音频仅展示、非商用，音色绑定服务账户 | 可参考声音；不能直接移植成离线声线，也不把合成演示当原始训练素材 |
| [DeepSeek娘 QQ bot](https://github.com/Mentha-Azure/deepseek-chan-qqbot) | GPT-SoVITS 本地语音接入与训练指南，不附角色录音或权重 | 后续自有鲸鱼娘声线可参考 |
| [中文派蒙 VITS 模型](https://huggingface.co/spaces/Misaka20001/Paimon-is-not-a-food/tree/main/model) | 多角色模型、完整符号表与角色编号，中文派蒙编号 **551** | 当前本机初版采用 |
| [AI 娘设定图集](https://github.com/lmy414/ai-chan-sheets) | 包括用户指定的冬装小肥鱼及作者署名、图像授权说明 | 继续作为形象来源；未发现配套统一语音合集 |

目前查到的是社区声音演示、单角色或多角色 TTS 模型与接入工程，尚未找到经核实、包含主要 AI 娘全部声线的统一语音包。固定台词 WAV 包不能直接朗读任意新回答，因此本轮接入可生成新语音的 TTS 模型。

模型空间标记 Apache-2.0，推理代码来自 MIT 的 [CjangCjengh/vits](https://github.com/CjangCjengh/vits)。代码许可证与原神角色、表演声音的使用权分别处理。当前角色及声音只用于本机个人测试，权重与本机路径配置没有纳入公共 Git 或现有安装包。

## 可复现的本机安装

模型、Python 环境和缓存在 **`D:\AI\RoboMind-Voice`**。不要将大文件放在空间紧张的 C 盘。安装脚本需要 `uv`；可将运行目录改到其他有至少 3 GB 空间的盘。

在 RoboMind-Console 仓库执行：

```powershell
py -3.12 tools/install_local_voice.py --runtime 'D:\AI\RoboMind-Voice' --console-dir 'C:\Users\FJL03\AppData\Local\LanChat-Qt\robomind-console'
```

脚本使用固定版本的源码、配置及模型，校验下载完整性和 SHA-256，完成模型初始化后才原子写入 `voice-backend.json`。模型缺失时不会覆盖旧配置。此文件是本机路径指针，不放入给其他人的安装包；现有安装包的文件白名单会拒绝该文件。

| 内容 | 固定版本 / 校验 |
|---|---|
| 模型空间 revision | `23aa81589339ff1ef49568b261ab8a0e642879bc` |
| 权重 | `G_953000.pth`，479,276,657 字节 |
| 权重 SHA-256 | `0c54396a7a9027952e4d72fceb7e1da1f003d108837138927c2054a33eda0292` |
| 推理代码 revision | `43c74f369834294464eca69316048d2f9a7d54f1` |
| 运行环境 | Python 3.11，PyTorch 2.7.1 CPU，4 个推理线程 |

模型在第一次需要语音时加载，同一桥进程后续复用。短试听约 7 秒音频，合成约 2 秒，首次加载约 5 秒；实际耗时随文本和机器变化。没有修改声音音高来冒充新声线。当前未提供语速、音高、情绪或精确口型调节。

## 媒体与退出

Console 用独立子进程的 UTF-8 JSON 管道传入当前句子，不开启额外网络监听。子进程环境移除 AI 密钥、LanChat 密码等凭据；公共检查点使用 `weights_only=True` 加载并严格核对模型和符号表。输出必须为有界 PCM16，字幕与音频继续使用原媒体协议。

新问题、停止或退出模式终止当前合成与播放；旧结果不接管新问题。未在合成时取消的空闲模型可继续复用。音频发送忙碌时，页面 / 停止请求保留并重试到有效期结束，避免停止指令被瞬时背压丢弃。

后续可加入自有 / 明确授权的鲸鱼娘专属参考音频、声线模型与其他 AI 娘的选择。麦克风、ASR / 唤醒、精准口型和小智通用协议继续按[沉浸模式待办](whale-immersion-plan.md)推进。

## 本轮验证

本机试听和数字 / 英文缩写、空文本、384 字节上限及失败后恢复已验证。真实 LanChat 服务端 → Console → COM3 实板完成三段语音，共 **425,764 PCM 字节**；停止、新问题打断、返回五页与 **5 条 PC 回复单次持久化**通过，解码失败为 0。内容使用本机模拟模型，没有新增官方 API 费用。用户已确认声线变换。针对说话中的爆音 / 断续，两端启用 TCP_NODELAY，S3 增加预缓冲 / DMA 并平滑段首尾；固定 221,054 字节音源与中途 300ms 停顿实测完整、CRC 一致、欠载 / DMA 队列断档计数为 0。3 秒停顿触发一次重新缓冲并完整恢复。用户于 2026-11-24 明确确认“音效合格”，批准本轮合入主分支；源码与测试见 [S3 音质记录](https://github.com/Nagi-226/RoboMind-S3/blob/master/docs/audio-quality-20261124.md)。

本机证据：`RoboMind-Console/build/ai-validation/whale-chat-local-vits.json`、`D:\AI\RoboMind-Voice/preview-result.json`、`D:\AI\RoboMind-Voice/worker-validation.json`。鲸鱼娘初版源码随本轮主分支更新，完整安装包仍为 2026.11.23.2。
