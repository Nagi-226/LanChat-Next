<#
.SYNOPSIS
  LanChat-Next 一键启动器 — 编译并启动 C++ server（后台）+ Tauri 桌面客户端

.DESCRIPTION
  1. 若 server exe 不存在则自动 cmake --build
  2. 杀掉 12346 端口的旧进程
  3. 后台启动 server → 等待就绪
  4. 前台启动 Tauri dev → 打开桌面窗口
  5. Tauri 退出后自动关停 server

.PARAMETER SkipBuild
  跳过 server 编译（仅启动已编译的 exe）

.PARAMETER Release
  使用 Release 配置（默认 Debug）

.PARAMETER Port
  Server 监听端口 (默认 12346)

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\launch.ps1
  powershell -ExecutionPolicy Bypass -File scripts\launch.ps1 -SkipBuild -Release
#>

param(
  [switch]$SkipBuild,
  [switch]$Release,
  [int]$Port = 12346
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$config = if ($Release) { 'Release' } else { 'Debug' }
$serverExe = Join-Path $root "build\server-next-vs\src\server\$config\lanchat_server_next.exe"
$clientDir = Join-Path $root 'src\client'

# ── 1. Build server ──────────────────────────────────────────────
if (-not $SkipBuild) {
  Write-Host '[BUILD] CMake configure + build ...' -ForegroundColor Cyan
  cmake -S $root -B "$root\build\server-next-vs" | Out-Null
  if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
  cmake --build "$root\build\server-next-vs" --config $config
  if ($LASTEXITCODE -ne 0) { throw 'CMake build failed' }
}

if (-not (Test-Path $serverExe)) {
  Write-Host "[MISSING] $serverExe — 请先 cmake --build" -ForegroundColor Red
  $SkipBuild = $false
  throw 'Server exe not found'
}

# ── 2. Kill old processes on both ports ────────────────────────────
$portsToKill = @($Port, 1420)
foreach ($targetPort in $portsToKill) {
  $old = netstat -ano 2>$null | Select-String ":$targetPort " | ForEach-Object {
    ($_ -split '\s+')[-1]
  } | Where-Object { $_ -match '^\d+$' } | Select-Object -Unique

  foreach ($killed in $old) {
    Write-Host "[KILL] Port $targetPort occupied by PID $killed — stopping ..." -ForegroundColor Yellow
    Stop-Process -Id $killed -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
  }
}

# ── 3. Start server (background) ─────────────────────────────────
Write-Host "[SERVER] Starting on port $Port ..." -ForegroundColor Green
$serverProc = Start-Process -FilePath $serverExe -ArgumentList $Port `
  -WindowStyle Hidden -PassThru
Start-Sleep -Seconds 1

if ($serverProc.HasExited) {
  throw 'Server exited immediately — check port or configuration'
}
Write-Host "[SERVER] PID $($serverProc.Id) running" -ForegroundColor Green

# ── 4. Start Tauri dev (foreground) ───────────────────────────────
Write-Host '[TAURI] Starting desktop client ...' -ForegroundColor Cyan
Push-Location $clientDir
try {
  $tauri = Start-Process -FilePath cmd.exe -ArgumentList '/c','npm run tauri dev' `
    -Wait -PassThru -WindowStyle Hidden
}
finally {
  Pop-Location
}

# ── 5. Cleanup ────────────────────────────────────────────────────
Write-Host '[CLEANUP] Stopping server ...' -ForegroundColor Yellow
if (-not $serverProc.HasExited) {
  Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
}
Write-Host '[DONE] LanChat-Next session ended.' -ForegroundColor Green
