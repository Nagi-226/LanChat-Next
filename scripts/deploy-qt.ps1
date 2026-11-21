<#
.SYNOPSIS
  把 Qt 客户端部署到 %LOCALAPPDATA%\LanChat-Qt 并更新桌面快捷方式。

.DESCRIPTION
  为什么需要这个脚本：
    桌面快捷方式指向的是 %LOCALAPPDATA%\LanChat-Qt\lanchat_qt.exe，不是 build-qt 里的产物。
    过去是手工复制部署的，结果出现过"源码已经加了新功能、快捷方式跑的还是旧 exe"的情况
    （记住账户勾选框在源码里有、在部署版里没有）。部署必须可重复执行。

  做五件事：
    1. 构建 Qt 客户端（可用 -SkipBuild 跳过）
    2. 把 lanchat_qt.exe 复制到部署目录
    3. 复制服务端 exe（存在才复制）
    4. windeployqt 补齐 Qt DLL 与插件
    5. 创建/更新桌面快捷方式

.PARAMETER SkipBuild
  跳过构建，只做复制部署（源码没改时用）

.PARAMETER QtDir
  Qt 安装前缀，例如 D:\Qt\6.10.3\mingw_64。默认自动探测。

.PARAMETER SkipShortcut
  不更新桌面快捷方式

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\deploy-qt.ps1

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\deploy-qt.ps1 -SkipBuild -QtDir D:\Qt\6.10.3\mingw_64
#>

param(
  [switch]$SkipBuild,
  [string]$QtDir = '',
  [switch]$SkipShortcut
)

$ErrorActionPreference = 'Stop'
$root      = Split-Path -Parent $PSScriptRoot
$clientDir = Join-Path $root 'src\client-qt'
$buildDir  = Join-Path $root 'build-qt'
$deployDir = Join-Path $env:LOCALAPPDATA 'LanChat-Qt'

function Step($msg) { Write-Host "[DEPLOY] $msg" -ForegroundColor Cyan }
function Fail($msg) { Write-Host "[FAIL] $msg" -ForegroundColor Red; exit 1 }

# ── 0. 定位 Qt ────────────────────────────────────────────────────
if (-not $QtDir) {
  # 从已有 build-qt 缓存里读，避免每次手填
  $cache = Join-Path $buildDir 'CMakeCache.txt'
  if (Test-Path $cache) {
    $hit = Select-String -Path $cache -Pattern '^Qt6_DIR:PATH=(.+)$' | Select-Object -First 1
    if ($hit) {
      # Qt6_DIR 形如 <prefix>/lib/cmake/Qt6
      $QtDir = (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $hit.Matches[0].Groups[1].Value.Trim())))
    }
  }
  if (-not $QtDir -and $env:QTDIR) { $QtDir = $env:QTDIR }
}
if (-not $QtDir) { Fail '未定位到 Qt。请用 -QtDir 指定，例如 -QtDir D:\Qt\6.10.3\mingw_64' }
if (-not (Test-Path (Join-Path $QtDir 'bin\windeployqt.exe'))) {
  Fail "QtDir 下找不到 bin\windeployqt.exe：$QtDir"
}
Step "Qt = $QtDir"

# ── 1. 构建 ───────────────────────────────────────────────────────
if (-not $SkipBuild) {
  Step 'cmake configure ...'
  & cmake -S $clientDir -B $buildDir -G Ninja "-DCMAKE_PREFIX_PATH=$QtDir"
  if ($LASTEXITCODE -ne 0) { Fail 'cmake configure 失败' }
  Step 'cmake build ...'
  & cmake --build $buildDir -j 8
  if ($LASTEXITCODE -ne 0) { Fail 'cmake build 失败' }
}

$srcExe = Join-Path $buildDir 'lanchat_qt.exe'
if (-not (Test-Path $srcExe)) { Fail "找不到构建产物：$srcExe（先跑一次不带 -SkipBuild 的部署）" }

# ── 2~4. 复制 + 部署依赖 ──────────────────────────────────────────
New-Item -ItemType Directory -Force -Path $deployDir | Out-Null

$dstExe = Join-Path $deployDir 'lanchat_qt.exe'
if (Test-Path $dstExe) { Copy-Item $dstExe "$dstExe.bak" -Force }
Copy-Item $srcExe $dstExe -Force
Step "已复制 lanchat_qt.exe -> $deployDir"

# 服务端：桌面快捷方式只启动客户端，服务端由客户端 sidecar 拉起，所以要放在同目录
#
# ⚠️ 候选顺序有讲究：必须取「最新的那份」，不能写死某个目录。
#    build/server-next-vs 是 VS2022 构建（可能是几个月前的旧产物）
#    build/server-mingw   是 MinGW+Ninja 构建（本项目常用的那条路）
#    写死 VS 目录会把旧 server 覆盖上线 —— 曾因此让客户端发用户名登录失败
#    （旧 server 没有 Login.id 的 string 分支，那个是 2026-11-19 才加的）
$serverCandidates = @(
  (Join-Path $root 'build\server-mingw\src\server\lanchat_server_next.exe'),
  (Join-Path $root 'build\server-next-vs\src\server\Release\lanchat_server_next.exe'),
  (Join-Path $root 'build\server-next-vs\src\server\Debug\lanchat_server_next.exe')
) | Where-Object { Test-Path $_ }

if ($serverCandidates.Count -gt 0) {
  # 取修改时间最新的那份
  $newest = $serverCandidates | Sort-Object { (Get-Item $_).LastWriteTime } -Descending | Select-Object -First 1
  Copy-Item $newest (Join-Path $deployDir 'lanchat_server_next.exe') -Force
  Step "已复制服务端：$newest"
} else {
  Write-Host '[WARN] 未找到 lanchat_server_next.exe，跳过（客户端将无法自启服务端）' -ForegroundColor Yellow
}

Step 'windeployqt ...'
& (Join-Path $QtDir 'bin\windeployqt.exe') --no-translations --no-system-d3d-compiler --no-opengl-sw $dstExe | Out-Null
if ($LASTEXITCODE -ne 0) { Fail 'windeployqt 失败' }

# ── 5. 桌面快捷方式 ──────────────────────────────────────────────
# 桌面可能被 OneDrive 重定向，实际存在两处：
#   [Environment]::GetFolderPath('Desktop')  → 通常是 OneDrive\桌面（真桌面）
#   $env:USERPROFILE\Desktop                 → 旧路径，可能残留一份旧的 .lnk
# 两处都要更新，否则用户看到的可能是残留的那份旧图标。
if (-not $SkipShortcut) {
  $desktopDirs = @(
    [Environment]::GetFolderPath('Desktop'),
    (Join-Path $env:USERPROFILE 'Desktop')
  ) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique

  $sh = New-Object -ComObject WScript.Shell
  foreach ($dir in $desktopDirs) {
    $lnk = Join-Path $dir 'LanChat-Next.lnk'
    $isNew = -not (Test-Path $lnk)
    # 只更新已存在的，和在真桌面上新建；不要在旧路径凭空造一个新的
    if ($isNew -and $dir -ne $desktopDirs[0]) { continue }
    $sc = $sh.CreateShortcut($lnk)
    $sc.TargetPath       = $dstExe
    $sc.WorkingDirectory = $deployDir
    $sc.Description      = 'LanChat-Next (Qt6 native client)'
    $sc.IconLocation     = "$dstExe,0"
    $sc.Save()
    Step ("桌面快捷方式已{0}：{1}" -f $(if ($isNew) { '创建' } else { '更新' }), $lnk)
  }
}

# ── 6. 自检 ──────────────────────────────────────────────────────
$exeTime = (Get-Item $dstExe).LastWriteTime
$srcTime = (Get-Item $srcExe).LastWriteTime
if ($exeTime -lt $srcTime) {
  Fail "部署产物比构建产物旧（部署 $exeTime < 构建 $srcTime）"
}
Write-Host ''
Write-Host "[DONE] 已部署到 $deployDir" -ForegroundColor Green
Write-Host "       exe 大小 $((Get-Item $dstExe).Length) 字节，时间 $exeTime"
Write-Host '       自检建议：双击桌面快捷方式，确认登录页有「记住账户，下次自动填写」' -ForegroundColor Gray
