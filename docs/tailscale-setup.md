# Tailscale 接入 LanChat

2026-11-22。已完成本机安装、在线状态、UDP/DERP 网络检查、虚拟 IPv4 的 TCP 12346 可达性与窄范围防火墙配置。当前只有一台可配合的 PC，**异地两机直连/中继、100 MiB 文件速度和跨网络掉线恢复尚未验收**。

## 1. 主机

1. 保持 LanChat 消息主机开机、联网；建议运行 `start-lan-host.ps1` 独立托管，关闭聊天窗口不会停止它。
2. 安装官方 Tailscale Windows 客户端；本轮通过 winget 官方源的 `Tailscale.Tailscale` 安装 1.104.7，安装器来自 `pkgs.tailscale.com` 且哈希校验通过。
3. 登录并加入所需 tailnet。机器的实际登录状态以 `tailscale status` 为准，身份操作由使用者完成。
4. 使用 `tailscale ip -4` 获取主机虚拟地址；也可使用 `tailscale status --json` 中的设备 DNSName。实际地址留在本机，不提交到公共文档。
5. 防火墙仅为必要的服务配置规则，例如下列管理员 PowerShell 命令。将占位值替换为主机地址和实际 exe 路径；已有规则保持原用途。

```powershell
New-NetFirewallRule -DisplayName 'LanChat Tailscale TCP 12346' `
  -Direction Inbound -Action Allow -Protocol TCP -LocalPort 12346 `
  -LocalAddress '<主机的 Tailscale IPv4>' -RemoteAddress '100.64.0.0/10' `
  -Profile Private -Program '<LanChat 部署目录>\lanchat_server_next.exe'
```

Tailscale 管理端访问规则与 Windows 防火墙共同决定是否可达；规则应只允许所需成员访问消息主机的 12346。当前尚未修改托管网络的全局访问策略，也未自动创建 LanChat 开机任务。

## 2. 异地 PC

安装 Tailscale，获得访问该主机的 tailnet 成员权限后，在 LanChat **服务器 → 连接公司局域网主机** 中填主机虚拟地址 / 可解析的 MagicDNS 名称和端口 `12346`。

主机上的客户端继续使用本机托管即可。远程 PC 随后在 LanChat 注册/登录，凭群 ID 加入群；组网授权和应用群 ID 是两层独立配置。

S3 继续连接附近 PC 的本地 2.4 GHz Wi-Fi / VVOS TCP 9000。当前不在 S3 上安装 Tailscale，也未建立子网路由；组网加密覆盖参与组网的 PC，本地 S3 TCP 段保持现有实现。

## 3. 检查与后续验收

```powershell
# 在源码仓库根目录：
.\scripts\check-tailscale.ps1 -Port 12346
# 在安装目录 / 解压后的分发包目录：
.\check-tailscale.ps1 -Port 12346
# 异地 PC 检查消息主机，需明确填写目标：
.\check-tailscale.ps1 -ServerAddress '<主机的 Tailscale 地址>' -Port 12346
tailscale netcheck
tailscale ping <主机的 Tailscale 地址>
```

检查脚本可显示本机组网状态、虚拟地址和端口可达性。**从本机访问自己的虚拟地址只能验证本机接入和监听，不能证明异地流量已直连或已通过加密隧道传输。**

有第二台 PC 时记录：两端所用网络、直连/DERP 状态、注册和群聊、四项设备查询、100 MiB 附件速度、暂停/续传及断线恢复。

本轮 netcheck 显示 UDP 可用、映射随目标变化；测得最近 DERP 为 Los Angeles，约 188 ms。此值是当前网络下的中继探测延迟，不能作为未来两端直连或文件速度的结论。企业计划/费用和目标地点可达性仍需按实际用途确认。

方案原理：[异地通信比较](https://github.com/Nagi-226/LanChat-Next/blob/master/docs/cross-network-options.md)。查询使用：[板端面板](https://github.com/Nagi-226/LanChat-Next/blob/master/docs/board-panel.md)。
