# configure.ps1  CMake configure for LanChat-Next server
# Usage: .\scripts\configure.ps1 [-BuildType Debug|Release]
param([string]$BuildType = "Debug")

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

function Get-CMakeGenerator {
    if (Get-Command ninja -ErrorAction SilentlyContinue)       { return @{ Name = 'Ninja'; MultiConfig = $false } }
    if (Get-Command mingw32-make -ErrorAction SilentlyContinue) { return @{ Name = 'MinGW Makefiles'; MultiConfig = $false } }
    if (Get-Command nmake -ErrorAction SilentlyContinue)        { return @{ Name = 'NMake Makefiles'; MultiConfig = $false } }
    $msbuild = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe'
    if (Test-Path $msbuild) { return @{ Name = 'Visual Studio 17 2022'; Args = @('-A', 'x64'); MultiConfig = $true } }
    return $null
}

$generator = Get-CMakeGenerator
if (-not $generator) {
    Write-Error "No CMake build driver found. Install ninja, mingw32-make, nmake, or Visual Studio Build Tools."
    exit 1
}

$safeName = ($generator.Name -replace '[^A-Za-z0-9]+', '-').Trim('-').ToLowerInvariant()
$BuildDir = Join-Path $Root "build/server-next-$safeName"

Write-Host "Generator : $($generator.Name)" -ForegroundColor Cyan
Write-Host "Build dir : $BuildDir" -ForegroundColor Cyan
Write-Host "Build type: $BuildType" -ForegroundColor Cyan

$cmakeArgs = @(
    '-S', $Root,
    '-B', $BuildDir,
    '-G', $generator.Name
)
if ($generator.Args) { $cmakeArgs += $generator.Args }
$cmakeArgs += "-DLANCHAT_BUILD_TESTS=ON"

if (-not $generator.MultiConfig) {
    $cmakeArgs += "-DCMAKE_BUILD_TYPE=$BuildType"
}

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`nConfigure succeeded. Next: .\scripts\build.ps1" -ForegroundColor Green
