param(
    [Parameter(Mandatory=$true)][string]$BundleDirectory,
    [string]$OutputDirectory='',
    [ValidatePattern('^\d{1,4}\.\d{1,2}\.\d{1,2}(\.\d{1,4})?$')][string]$ReleaseVersion='2026.11.23.1',
    [string]$LanHostAddress='',
    [string]$TailscaleHostAddress='',
    [string]$IsccPath=''
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $OutputDirectory){$OutputDirectory=Join-Path $root 'build\windows-installer-20261123-ai-ui'}
$bundle=(Resolve-Path -LiteralPath $BundleDirectory).Path
if(-not $IsccPath){
    $candidates=@(
        (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
    )
    $IsccPath=$candidates | Where-Object {Test-Path -LiteralPath $_} | Select-Object -First 1
}
if(-not $IsccPath -or -not (Test-Path -LiteralPath $IsccPath)){throw 'Install Inno Setup 6 (winget: JRSoftware.InnoSetup), or supply -IsccPath.'}
$output=[IO.Path]::GetFullPath($OutputDirectory)
$payload=Join-Path $output 'payload'
$setup=Join-Path $output "LanChat-Next-Setup-$ReleaseVersion-x64.exe"
if((Test-Path -LiteralPath $payload) -or (Test-Path -LiteralPath $setup)){throw 'Choose a new OutputDirectory; previous artifacts are never overwritten.'}
$helpers=@('start-lan-host.ps1','check-tailscale.ps1','LAN-使用说明.md','Tailscale-使用说明.md')
$legacyInstaller='install.ps1'
$pluginFolders=@('platforms','imageformats','iconengines','styles','generic','networkinformation','tls')

# Validate every source file before copying. This prevents accidental profile/data inclusion.
$files=Get-ChildItem -LiteralPath $bundle -Recurse -File
foreach($file in $files){
    $relative=$file.FullName.Substring($bundle.Length+1)
    if($relative -eq $legacyInstaller){continue}
    $parts=$relative.Split('\')
    $binary=$file.Extension.ToLowerInvariant() -in @('.exe','.dll','.ico')
    $allowed=$false
    if($parts.Count -eq 1){$allowed=$binary -or $relative -in $helpers}
    elseif($parts[0] -in $pluginFolders){$allowed=$file.Extension -eq '.dll'}
    elseif($parts[0] -eq 'robomind-console'){
        $allowed=($parts.Count -eq 2 -and $binary) -or ($parts.Count -gt 2 -and $parts[1] -in $pluginFolders -and $file.Extension -eq '.dll')
    }
    if(-not $allowed){throw "Unexpected bundle file: $relative"}
}
foreach($required in @('lanchat_qt.exe','lanchat_server_next.exe','lanchat-rounded-v2.ico','platforms\qwindows.dll','robomind-console\RoboMindConsole.exe','robomind-console\Qt6SerialPort.dll')+$helpers){
    if(-not (Test-Path -LiteralPath (Join-Path $bundle $required))){throw "Missing bundle file: $required"}
}
New-Item -ItemType Directory -Force -Path $payload | Out-Null
foreach($file in $files){
    $relative=$file.FullName.Substring($bundle.Length+1)
    if($relative -eq $legacyInstaller){continue}
    $destination=Join-Path $payload $relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination
}
Copy-Item -LiteralPath (Join-Path $root 'docs\windows-quickstart.txt') -Destination (Join-Path $payload '联机使用说明.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'installer\third-party') -Destination (Join-Path $payload 'third-party') -Recurse

$fileVersion=if($ReleaseVersion.Split('.').Count -eq 3){$ReleaseVersion+'.0'}else{$ReleaseVersion}
& $IsccPath ("/DPayloadDirectory=$payload") ("/DOutputDirectory=$output") ("/DReleaseVersion=$ReleaseVersion") ("/DFileVersion=$fileVersion") (Join-Path $PSScriptRoot 'installer\lanchat.iss')
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $setup)){throw 'Inno Setup compilation failed.'}
$manifest=[ordered]@{
    version=$ReleaseVersion
    source_commit=(& git -C $root rev-parse HEAD)
    source_worktree_dirty=([bool](& git -C $root status --porcelain -- src/client-qt scripts docs README.md))
    setup=(Split-Path -Leaf $setup)
    setup_sha256=(Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash.ToLowerInvariant()
    files=@(Get-ChildItem -LiteralPath $payload -Recurse -File | ForEach-Object {
        [ordered]@{path=$_.FullName.Substring($payload.Length+1);sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
    })
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'installer-manifest.json') -Encoding utf8
$testkit=Join-Path $output 'testkit'
New-Item -ItemType Directory -Path $testkit | Out-Null
Copy-Item -LiteralPath $setup -Destination $testkit
Copy-Item -LiteralPath (Join-Path $payload '联机使用说明.txt') -Destination (Join-Path $testkit '安装与使用说明.txt')
Copy-Item -LiteralPath (Join-Path $output 'installer-manifest.json') -Destination $testkit
$lanAddress=if($LanHostAddress){$LanHostAddress}else{'向测试发起者索取消息主机的局域网 IPv4 地址'}
$tailscaleAddress=if($TailscaleHostAddress){$TailscaleHostAddress}else{'向测试发起者索取消息主机的 Tailscale 地址'}
@"
LanChat-Next $ReleaseVersion 联机测试连接信息

同一局域网主机：$lanAddress
异地 Tailscale 主机：$tailscaleAddress
聊天端口：12346

安装后在登录页填写「服务器地址」（地址:12346），点击「连接」，再注册 / 登录。
凭测试发起者提供的群 ID 加入；群 ID、账户与 Tailscale 权限分别配置。
异地用户需安装官方 Tailscale 并取得访问消息主机的权限；同一局域网可直接连接。
主机需保持开机联网；地址变更时由发起者提供新地址。

登录后：左下角齿轮 → 设置 → 服务器 / 板端连接。
普通 PC 聊天、图片和文件无需连接板子或配置 AI 密钥。
询问板端状态时，发起者需提供专用板端账号或已配置群 ID。
AI 接口由连接板子的桥接主机配置；分发包不含主机账号、密钥或日志。
本版本包含文字状态 AI、长消息完整显示、关闭按钮红色悬停和登录后设置入口。
"@ | Set-Content -LiteralPath (Join-Path $testkit '发给测试者-连接信息.txt') -Encoding utf8
$zip=Join-Path $output "LanChat-Next-完整联机测试包-$ReleaseVersion.zip"
Compress-Archive -LiteralPath @(Get-ChildItem -LiteralPath $testkit -File | ForEach-Object {$_.FullName}) -DestinationPath $zip
Write-Host "Installer: $setup"
Write-Host "Complete test kit: $zip"
