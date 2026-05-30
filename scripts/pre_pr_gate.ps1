param(
  [switch]$Fast
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

Push-Location "$root\src\client"
npm test
npm run build
npm run protocol:check
Pop-Location

Push-Location "$root\src\client\src-tauri"
cargo check
Pop-Location

if (-not $Fast) {
  cmake -S "$root" -B "$root\build\server-gate" -DLANCHAT_WARNINGS_AS_ERRORS=ON
  cmake --build "$root\build\server-gate" --config Release
  ctest --test-dir "$root\build\server-gate" -C Release --output-on-failure
  node "$root\scripts\e2e_protocol_smoke.mjs" "$root\build\server-gate\src\server\Release\lanchat_server_next.exe"
  node "$root\scripts\load_smoke.mjs" --server "$root\build\server-gate\src\server\Release\lanchat_server_next.exe" --clients 50 --messages 20
}
