param([string]$DeploymentDirectory=(Join-Path $env:LOCALAPPDATA 'LanChat-Qt'),
      [string]$OutputDirectory=(Join-Path (Split-Path -Parent $PSScriptRoot) 'build\lan-distribution'))
$ErrorActionPreference='Stop'
if(-not (Test-Path -LiteralPath (Join-Path $DeploymentDirectory 'lanchat_qt.exe'))){throw 'Deploy the application first'}
$bundle=Join-Path $OutputDirectory 'LanChat-Next'
if(Test-Path -LiteralPath $bundle){throw 'Output bundle already exists; choose a new OutputDirectory'}
New-Item -ItemType Directory -Force -Path $bundle | Out-Null
# Explicit binary allowlist: never include user profiles, account secrets, databases or logs.
Get-ChildItem -LiteralPath $DeploymentDirectory -File | Where-Object {$_.Extension -in @('.exe','.dll','.ico')} | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $bundle}
foreach($folder in @('platforms','imageformats','iconengines','styles','generic','networkinformation','tls')){
    $source=Join-Path $DeploymentDirectory $folder
    if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $bundle -Recurse}
}
$consoleSource=Join-Path $DeploymentDirectory 'robomind-console'
if(Test-Path -LiteralPath $consoleSource){
    $consoleTarget=Join-Path $bundle 'robomind-console';New-Item -ItemType Directory -Force -Path $consoleTarget | Out-Null
    Get-ChildItem -LiteralPath $consoleSource -File | Where-Object {$_.Extension -in @('.exe','.dll')} | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $consoleTarget}
    foreach($folder in @('platforms','imageformats','iconengines','styles','generic','networkinformation','tls')){
        $source=Join-Path $consoleSource $folder
        if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $consoleTarget -Recurse}
    }
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'start-lan-host.ps1') -Destination $bundle
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'check-tailscale.ps1') -Destination $bundle
Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $PSScriptRoot) 'docs\tailscale-setup.md') -Destination (Join-Path $bundle 'Tailscale-使用说明.md')
Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $PSScriptRoot) 'docs\lan-deployment.md') -Destination (Join-Path $bundle 'LAN-使用说明.md')
@'
$ErrorActionPreference='Stop'
$destination=Join-Path $env:LOCALAPPDATA 'LanChat-Qt'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
Get-ChildItem -LiteralPath $PSScriptRoot | Where-Object {$_.Name -ne 'install.ps1'} | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $destination -Recurse -Force}
$shell=New-Object -ComObject WScript.Shell
$shortcut=$shell.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Desktop')) 'LanChat-Next.lnk'))
$shortcut.TargetPath=Join-Path $destination 'lanchat_qt.exe'
$shortcut.WorkingDirectory=$destination
$shortcut.IconLocation=(Join-Path $destination 'lanchat-rounded-v2.ico')+',0'
$shortcut.Save()
Write-Host 'Installed. Enter the host address on the LanChat login page, connect, then register or sign in.'
'@ | Set-Content -LiteralPath (Join-Path $bundle 'install.ps1') -Encoding utf8
$zip=Join-Path $OutputDirectory 'LanChat-Next-Windows-LAN.zip'
Compress-Archive -LiteralPath $bundle -DestinationPath $zip
Write-Host "LAN distribution: $zip"
