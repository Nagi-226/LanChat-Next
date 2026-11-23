param(
    [Parameter(Mandatory=$true)][string]$InstallerPath,
    [Parameter(Mandatory=$true)][string]$ManifestPath,
    [Parameter(Mandatory=$true)][string]$TestClientPath,
    [Parameter(Mandatory=$true)][string]$QtTestDllPath,
    [string]$BoardLogProbePath='',
    [string]$BoardLogToolPath='',
    [string]$PythonPath='',
    [string]$AiStatusToolPath='',
    [string]$BoardLogPort='',
    [string]$TestRoot=''
)
$ErrorActionPreference='Stop'
if(-not $TestRoot){$TestRoot=Join-Path (Split-Path -Parent $PSScriptRoot) 'build\windows-installer-validation'}
$registry='HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{8AA962BF-0263-4A45-9EA1-89D1DAFF07F1}_is1'
# This test uses the actual distributable. Refuse if an existing installer owns its key.
if(Test-Path -LiteralPath $registry){throw 'An installed LanChat setup is registered. Use a fresh Windows user for this test.'}
$group='LanChat-Next'
$groupDirectory=Join-Path ([Environment]::GetFolderPath('Programs')) $group
if(Test-Path -LiteralPath $groupDirectory){throw 'An existing LanChat start menu group is present. Use a fresh Windows user for this test.'}
$installer=(Resolve-Path -LiteralPath $InstallerPath).Path
$manifest=Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if((Get-FileHash -LiteralPath $installer).Hash.ToLowerInvariant() -ne $manifest.setup_sha256){throw 'Setup hash does not match the manifest.'}
$validationId=[guid]::NewGuid().ToString('N')
$run=Join-Path ([IO.Path]::GetFullPath($TestRoot)) $validationId
$destination=Join-Path $run 'installed'
New-Item -ItemType Directory -Force -Path $run | Out-Null

function Assert-OwnedInstall {
    $key=Get-ItemProperty -LiteralPath $registry
    if($key.InstallLocation.TrimEnd('\') -ne $destination){throw 'Installer registration points outside the isolated test directory.'}
}
function Invoke-Setup([string]$logName) {
    $process=Start-Process -FilePath $installer -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/LANG=chinesesimp',('/DIR="'+$destination+'"'),'/TASKS=""',('/LOG="'+(Join-Path $run $logName)+'"')) -WindowStyle Hidden -PassThru -Wait
    if($process.ExitCode -ne 0){throw "Setup failed: $($process.ExitCode)"}
    Assert-OwnedInstall
}
function Assert-Payload {
    foreach($file in $manifest.files){
        $path=Join-Path $destination $file.path
        if(-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -ne $file.sha256){throw "Installed file hash mismatch: $($file.path)"}
    }
}

try {
    Invoke-Setup 'install.log'
    Assert-Payload
    $shortcut=Join-Path ([Environment]::GetFolderPath('Programs')) ($group+'\LanChat-Next.lnk')
    if(-not (Test-Path -LiteralPath $shortcut)){throw 'Start menu shortcut missing.'}
    $shell=New-Object -ComObject WScript.Shell
    $link=$shell.CreateShortcut($shortcut)
    if($link.TargetPath -ne (Join-Path $destination 'lanchat_qt.exe') -or $link.WorkingDirectory -ne $destination -or $link.IconLocation -notlike ($destination+'\lanchat-rounded-v2.ico,*')){throw 'Shortcut target, directory or icon mismatch.'}

    # The real Qt LAN test redirects settings and accounts to temporary directories.
    # Place it beside the installed DLLs so it cannot silently use the developer Qt tree.
    $probe=Join-Path $destination 'installer-runtime-probe.exe'
    Copy-Item -LiteralPath $TestClientPath -Destination $probe
    # QtTest is only a dependency of the test probe, and is not part of the installer.
    Copy-Item -LiteralPath $QtTestDllPath -Destination (Join-Path $destination 'Qt6Test.dll')
    $savedPath=$env:PATH
    $savedPlugin=$env:QT_PLUGIN_PATH
    $savedPlatform=$env:QT_QPA_PLATFORM
    try {
        $env:PATH=(Join-Path $env:SystemRoot 'System32')+';'+$env:SystemRoot
        $env:QT_PLUGIN_PATH=$destination
        $env:QT_QPA_PLATFORM='offscreen'
        & $probe --server (Join-Path $destination 'lanchat_server_next.exe') 2>&1 | Out-File -LiteralPath (Join-Path $run 'runtime.log') -Encoding utf8
        if($LASTEXITCODE -ne 0){throw "Installed Qt/server LAN test failed: $LASTEXITCODE"}
        if($AiStatusToolPath){
            if(-not $PythonPath){throw 'Supply PythonPath for the installed AI mock test.'}
            & $PythonPath $AiStatusToolPath --console (Join-Path $destination 'robomind-console\RoboMindConsole.exe') 2>&1 | Out-File -LiteralPath (Join-Path $run 'ai-mock-runtime.log') -Encoding utf8
            if($LASTEXITCODE -ne 0){throw "Installed Console AI mock test failed: $LASTEXITCODE"}
        }
        if($BoardLogPort){
            if(-not $BoardLogProbePath -or -not $BoardLogToolPath -or -not $PythonPath){throw 'Supply the board log probe, tool and Python paths for USB installer validation.'}
            $logProbe=Join-Path $destination 'installer-log-probe.exe'
            Copy-Item -LiteralPath $BoardLogProbePath -Destination $logProbe
            & $PythonPath $BoardLogToolPath --server (Join-Path $destination 'lanchat_server_next.exe') --console (Join-Path $destination 'robomind-console\RoboMindConsole.exe') --port $BoardLogPort --qt-probe $logProbe --capture (Join-Path $run 'board-logs-panel.png') --result (Join-Path $run 'board-logs-result.json') 2>&1 | Out-File -LiteralPath (Join-Path $run 'board-logs-runtime.log') -Encoding utf8
            if($LASTEXITCODE -ne 0){throw "Installed USB log/Qt query test failed: $LASTEXITCODE"}
        }
    } finally {
        $env:PATH=$savedPath
        $env:QT_PLUGIN_PATH=$savedPlugin
        $env:QT_QPA_PLATFORM=$savedPlatform
    }
    $marker=Join-Path $destination 'personal-data-preservation.txt'
    [IO.File]::WriteAllText($marker,'preserve-user-data')
    Invoke-Setup 'upgrade.log'
    Assert-Payload
    if([IO.File]::ReadAllText($marker) -ne 'preserve-user-data'){throw 'Reinstall lost personal data.'}
    Assert-OwnedInstall
    $uninstaller=Join-Path $destination 'unins000.exe'
    $process=Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/LOG="'+(Join-Path $run 'uninstall.log')+'"')) -WindowStyle Hidden -PassThru -Wait
    if($process.ExitCode -ne 0){throw "Uninstall failed: $($process.ExitCode)"}
    if((Test-Path -LiteralPath $registry) -or (Test-Path -LiteralPath $shortcut) -or (Test-Path -LiteralPath (Join-Path $destination 'lanchat_qt.exe'))){throw 'Uninstall left its registration, shortcut or program executable.'}
    if([IO.File]::ReadAllText($marker) -ne 'preserve-user-data'){throw 'Uninstall lost personal data.'}
    [ordered]@{result='PASS';installer_sha256=$manifest.setup_sha256;payload_files=@($manifest.files).Count;installed_runtime_lan_test='PASS';installed_ai_mock_test=$(if($AiStatusToolPath){'PASS'}else{'not_run'});reinstall='PASS';uninstall='PASS';personal_data_preserved=$true;logs=$run} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
    Write-Host "PASS actual installer: payload, shortcut, installed Qt/server LAN test, reinstall, uninstall and data preservation. Logs: $run"
} finally {
    # If a test fails after installation, remove only its own registration/files.
    if(Test-Path -LiteralPath $registry){
        Assert-OwnedInstall
        $cleanup=Join-Path $destination 'unins000.exe'
        if(Test-Path -LiteralPath $cleanup){Start-Process -FilePath $cleanup -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -WindowStyle Hidden -Wait}
    }
}
