param(
  [switch]$SkipClient,
  [switch]$SkipServer,
  [int]$LoadClients = 500,
  [int]$LoadMessages = 50
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

function Invoke-Step {
  param(
    [string]$Name,
    [scriptblock]$Body
  )
  Write-Host "==> $Name" -ForegroundColor Cyan
  & $Body
}

if (-not $SkipClient) {
  Invoke-Step "client unit tests" {
    Push-Location "$root\src\client"
    npm test
    Pop-Location
  }

  Invoke-Step "client production build" {
    Push-Location "$root\src\client"
    npm run build
    Pop-Location
  }

  Invoke-Step "protocol alignment" {
    Push-Location "$root\src\client"
    npm run protocol:check
    Pop-Location
  }

  Invoke-Step "tauri rust check" {
    Push-Location "$root\src\client\src-tauri"
    cargo check
    Pop-Location
  }
}

if (-not $SkipServer) {
  Invoke-Step "server release configure" {
    cmake -S "$root" -B "$root\build\server-release" -DLANCHAT_WARNINGS_AS_ERRORS=ON
  }

  Invoke-Step "server release build" {
    cmake --build "$root\build\server-release" --config Release
  }

  Invoke-Step "server CTest" {
    ctest --test-dir "$root\build\server-release" -C Release --output-on-failure
  }

  Invoke-Step "protocol E2E smoke" {
    node "$root\scripts\e2e_protocol_smoke.mjs" "$root\build\server-release\src\server\Release\lanchat_server_next.exe"
  }

  Invoke-Step "real asio 500-user load smoke" {
    node "$root\scripts\load_smoke.mjs" --server "$root\build\server-release\src\server\Release\lanchat_server_next.exe" --clients $LoadClients --messages $LoadMessages
  }
}

Write-Host "v2.0.0-a1 real-asio gate PASS" -ForegroundColor Green
