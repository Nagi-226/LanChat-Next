param(
    [switch]$AllowMissingFrontendDeps,
    [switch]$SkipCargoCheck
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

function Invoke-Step {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Action
    )
    Write-Host "`n=== $Name ===" -ForegroundColor Cyan
    & $Action
}

function Invoke-Native {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments
    )
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

function Get-CMakeGenerator {
    if (Get-Command ninja -ErrorAction SilentlyContinue) { return @{ Name = 'Ninja'; Args = @(); MultiConfig = $false } }
    if (Get-Command mingw32-make -ErrorAction SilentlyContinue) { return @{ Name = 'MinGW Makefiles'; Args = @(); MultiConfig = $false } }
    if (Get-Command nmake -ErrorAction SilentlyContinue) { return @{ Name = 'NMake Makefiles'; Args = @(); MultiConfig = $false } }
    $msbuild = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe'
    if (Test-Path $msbuild) { return @{ Name = 'Visual Studio 17 2022'; Args = @('-A', 'x64'); MultiConfig = $true } }
    return $null
}

Set-Location $Root

Invoke-Step 'Protocol generation check' {
    node scripts/generate_protocol.mjs --check
    node scripts/audit_protocol.mjs
}

Invoke-Step 'CMake configure/build/test' {
    $generator = Get-CMakeGenerator
    if (-not $generator) {
        throw 'No CMake build driver found. Install/expose ninja, mingw32-make, nmake, or Visual Studio Build Tools, then rerun scripts/verify_v1_1_5.ps1.'
    }
    $safeGeneratorName = ($generator.Name -replace '[^A-Za-z0-9]+', '-').Trim('-').ToLowerInvariant()
    $BuildDir = Join-Path $Root "build/server-next-$safeGeneratorName"
    Invoke-Native cmake -S . -B $BuildDir -G $generator.Name @($generator.Args) -DLANCHAT_BUILD_TESTS=ON
    if ($generator.MultiConfig) {
        Invoke-Native cmake --build $BuildDir --config Debug
        Invoke-Native ctest --test-dir $BuildDir -C Debug --output-on-failure
        $serverExe = Join-Path $BuildDir 'src/server/Debug/lanchat_server_next.exe'
    }
    else {
        Invoke-Native cmake --build $BuildDir
        Invoke-Native ctest --test-dir $BuildDir --output-on-failure
        $serverExe = Join-Path $BuildDir 'src/server/lanchat_server_next.exe'
    }
    Invoke-Native node tests/server_multi_client_smoke.mjs $serverExe 12347
}

Invoke-Step 'Vite build' {
    Push-Location (Join-Path $Root 'src/client')
    try {
        if (-not (Test-Path 'node_modules')) {
            if ($AllowMissingFrontendDeps) {
                Write-Warning 'node_modules is missing; skipping npm run build because -AllowMissingFrontendDeps was set.'
                return
            }
            throw 'node_modules is missing. Run npm install in src/client, then rerun the v1.1.5 verification script.'
        }
        Invoke-Native npm run build
    }
    finally {
        Pop-Location
    }
}

Invoke-Step 'Cargo metadata/check' {
    $manifest = Join-Path $Root 'src/client/src-tauri/Cargo.toml'
    Invoke-Native cargo metadata --manifest-path $manifest --no-deps --format-version 1
    if (-not $SkipCargoCheck) {
        Invoke-Native cargo check --manifest-path $manifest
    }
}

Write-Host "`nv1.1.5 checkpoint verification completed." -ForegroundColor Green
