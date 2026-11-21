<# Launch the deployed Qt native application; it owns its server and optional Console bridge. #>
param(
  [switch]$Build,
  [switch]$SkipBuild,
  [switch]$Release,
  [int]$Port = 12346,
  [switch]$BoardConnection
)
$ErrorActionPreference = 'Stop'
$deployDir = Join-Path $env:LOCALAPPDATA 'LanChat-Qt'
$exe = Join-Path $deployDir 'lanchat_qt.exe'
if ($Port -ne 12346) { throw 'Qt 原生客户端当前使用本机服务端端口 12346。' }
if ($Build -or -not (Test-Path -LiteralPath $exe)) {
  & (Join-Path $PSScriptRoot 'deploy-qt.ps1')
  if ($LASTEXITCODE -ne 0) { throw 'Qt 部署失败' }
}
$arguments = if ($BoardConnection) { @('--board-connection') } else { @() }
if ($arguments.Count) {
  Start-Process -FilePath $exe -WorkingDirectory $deployDir -ArgumentList $arguments
} else {
  Start-Process -FilePath $exe -WorkingDirectory $deployDir
}
Write-Host "[STARTED] $exe"
