# 不自建公网服务器的跨网络通信方案

资料核查与用户决策：2026-11-21；实施更新：2026-11-22。**已采用 Tailscale 托管组网并完成本机接入**，保留常开 LanChat 主机。当前只有一台 PC，异地两机连接与附件传输待验；未自建中继或更换聊天协议。

## 已确定的实施方向

当前 LanChat 使用明确的主机地址和 TCP 端口，不依赖跨网络广播发现，因此先采用 Tailscale 的加密 IP 通道。ZeroTier 保留为后续需要虚拟以太网 / 广播发现时的备选资料。

异地 PC → Tailscale → 常开 LanChat 主机 → 本地 Console → S3。S3 继续使用本地 2.4 GHz Wi-Fi 和 VVOS TCP，不在 MCU 上安装组网客户端。托管控制与必要的中继不替代消息主机；聊天历史和附件仍存放在消息主机。

“感知 / 链路 / 设备 / 帮助”四项只读查询已完成初版和实板验收，Tailscale 本机也已接入。接下来等待第二台 PC 进行异地验收；外部大模型 API 为后续阶段。操作见 [Tailscale 接入说明](tailscale-setup.md)，职责、验收和后续入口见[实施清单](next-steps-20261122.md)。

## 推荐顺序

**先保留 LanChat，用托管组网连接异地 PC。**现有公司主机继续保存账号、群、消息和文件，异地用户通过组网地址连接它。无需租 VPS、做路由器公网端口映射或维护公网中继，但那台消息主机仍需在线。

若以后要求 **任何公司主机都不用常开**，再考虑托管消息服务或 SimpleX/Jami 的通信模型，届时需要改传输协议、身份体系和持久化策略。

| 方向 | 需要用户自行维护什么 | 适合现有 LanChat 的程度 | 限制 |
|---|---|---|---|
| **Tailscale 托管组网** | 公司原有消息 PC；各参与电脑安装并加入同一 tailnet | 高：当前服务器设置填主机的 Tailscale 地址，聊天协议基本可复用 | 第三方身份/控制平面；直连失败会使用中继；公司计划、权限、费用需另行确认 |
| **ZeroTier Central 托管组网** | 公司消息 PC；终端加入网络并由管理员授权 | 高：用分配的虚拟网络地址连接 | 依赖 Central / 网络基础设施；企业计划和软件许可不能从“GitHub 有源码”推断为免费无限制 |
| **SimpleX 的默认公共中继** | 终端应用和身份/连接邀请 | 中低：需重新接入协议与加密体系；不是给现有 TCP 服务端直接换地址 | 默认中继由第三方运行；文件保留/限额和公司治理需求需核查；不能把 CLI/bot 接口等同于现成 Qt SDK |
| **Jami / OpenDHT / TURN** | 终端应用；一般可使用默认引导和中继基础设施 | 中低：可参考 P2P、Swarm 和文件设计，接入成本显著高于组网 | NAT、离线投递、多设备同步、移动推送都需要处理；可靠直连不能保证 |
| **自行实现 WebRTC / libp2p 打洞** | 仍需发现/信令渠道，必要时还要可用的 TURN/relay | 低：适合后续专项研发 | 单靠打洞不能保证跨 CGNAT、严格防火墙、移动网络成功；没有中继兜底不能作为公司可靠通信承诺 |

## 为什么仍可能有服务器

Tailscale 控制服务管理设备身份、公钥与访问策略；数据通道使用 WireGuard。当前官方连接机制通过 DERP 协商，然后尝试 NAT 穿透以升级为直接 UDP 通道；无法直连时，可使用已配置的 Peer Relay 或保持 DERP 中继。DERP 转发已加密的数据，不能解密聊天流量。默认可使用 Tailscale 运营的 DERP，无需自行部署。

ZeroTier 使用底层加密 P2P 传输 VL1 与上层虚拟以太网 VL2。根节点帮助设备发现与连接；Central 控制器管理网络配置、成员授权和签名凭证。私有网络输入 Network ID 后仍需授权，能够直连时数据在设备间传输，否则使用中继。

两种方案都依赖网络基础设施；托管组网可以减少自行运维公网服务的工作，但不能保证所有 NAT 下始终直接连接。

SimpleX 把持久聊天数据保存在客户端，服务器承担中继；可以使用应用预配置的公共服务器。Jami 官方文章也明确说明引导、名字查询、移动推送/OpenDHT 代理和 TURN 等场景使用服务器，尤其 TURN 用于无法建立 P2P 时的转发。这两种架构都应继续评估群聊、离线文件及企业账号治理。

## Wi-Fi、以太网和手机流量

这些方案跨越的是 IP 网络，不要求两边使用相同 Wi-Fi，也不要求网卡类型相同。异地 Windows PC 通过以太网、Wi-Fi 或手机热点联网均可尝试。Android/iOS 能运行组网客户端，但 **LanChat 当前只有 PC 客户端**；直接在手机聊天需要开发移动客户端或采用已有跨平台聊天产品。

实际可用性要用目标地点的公司网和移动运营商测试，关注中继延迟、100 MB 文件速度、掉线恢复、后台运行、电池/流量和访问权限。海外公共服务在实际使用地区的可达性也需要现场确认。

## 官方与开源来源

- [Tailscale 源码与平台范围](https://github.com/tailscale/tailscale)，本次读取官方 README。
- [Tailscale DERP 文档](https://tailscale.com/kb/1232/derp-servers)，本次读取直连回退、WireGuard 加密和默认 DERP 说明。
- [Tailscale 连接类型](https://tailscale.com/kb/1257/connection-types)、[MagicDNS](https://tailscale.com/kb/1081/magicdns)、[子网网关](https://tailscale.com/kb/1019/subnets)，用于核查连接协商、名称访问与不运行客户端的设备接入方式。
- [ZeroTier 源码](https://github.com/zerotier/ZeroTierOne)；[Central 入门](https://docs.zerotier.com/start/)，本次核查网络 ID、终端安装和组织网络管理流程。
- [ZeroTier 协议](https://docs.zerotier.com/protocol/)，用于核查 VL1 / VL2、控制器、根节点与中继职责。
- [SimpleX Chat](https://github.com/simplex-chat/simplex-chat)；[平台设计](https://github.com/simplex-chat/simplex-chat/blob/stable/docs/SIMPLEX.md)，本次读取公共中继、客户端数据和 SDK/bot 边界说明。
- [Jami 官方：为什么是分布式](https://jami.net/why-is-jami-truly-distributed/)，2019 年架构解释，用于说明辅助服务器角色；其内容不作为 2026 年完整功能/性能验收。

本轮没有引用星数作为质量证据，没有克隆/执行外部仓库脚本，也没有将上述产品的完整能力宣布为 LanChat 已实现。
