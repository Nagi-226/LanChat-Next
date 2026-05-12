# build.ps1  Build LanChat-Next server
# Usage: .\scripts\build.ps1 [-BuildType Debug|Release] [-BuildDir <path>]
param(
    [string]$BuildType = "Debug",
    [string]$BuildDir = ""
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

if (-not $BuildDir) {
    $BuildDir = Get-ChildItem -Path $Root -Directory -Filter "build/server-next-*" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}

if (-not $BuildDir -or -not (Test-Path (Join-Path $BuildDir 'CMakeCache.txt'))) {
    Write-Error "No CMake build directory found. Run .\scripts\configure.ps1 first."
    exit 1
}

Write-Host "Building $BuildDir ($BuildType)..." -ForegroundColor Cyan

$isMultiConfig = (Select-String -Path (Join-Path $BuildDir 'CMakeCache.txt') -Pattern 'CMAKE_CONFIGURATION_TYPES' -Quiet)

if ($isMultiConfig) {
    & cmake --build $BuildDir --config $BuildType
} else {
    & cmake --build $BuildDir
}

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Run tests if available
$ctestPath = Join-Path $BuildDir 'CTestTestfile.cmake'
if (Test-Path $ctestPath) {
    Write-Host "`nRunning tests..." -ForegroundColor Cyan
    if ($isMultiConfig) {
        & ctest --test-dir $BuildDir -C $BuildType --output-on-failure
    } else {
        & ctest --test-dir $BuildDir --output-on-failure
    }
}

Write-Host "`nBuild succeeded." -ForegroundColor Green
