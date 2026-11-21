param([int]$Port=12346,[int]$MaxAttachmentMb=100,[int]$QuotaMb=5120,
      [string]$ProgramDirectory=(Join-Path $env:LOCALAPPDATA 'LanChat-Qt'))
$ErrorActionPreference='Stop'
if($Port -lt 1024 -or $Port -gt 65534 -or $MaxAttachmentMb -lt 1 -or $QuotaMb -lt $MaxAttachmentMb){throw 'Invalid port or attachment limits'}
$program=Join-Path $ProgramDirectory 'lanchat_server_next.exe'
if(-not (Test-Path -LiteralPath $program)){throw "Missing server executable: $program"}
$probe=New-Object System.Net.Sockets.TcpClient
try {
    $pending=$probe.ConnectAsync('127.0.0.1',$Port)
    if($pending.Wait(500) -and $probe.Connected){Write-Host "Port $Port already has a listener. Keep the existing host running.";return}
} catch {} finally {$probe.Dispose()}
$data=Join-Path $env:APPDATA 'LanChat-Next\data'
New-Item -ItemType Directory -Force -Path $data | Out-Null
$env:LANCHAT_ATTACHMENT_MAX_MB=[string]$MaxAttachmentMb
$env:LANCHAT_ATTACHMENT_QUOTA_MB=[string]$QuotaMb
$process=Start-Process -FilePath $program -ArgumentList @('--port',[string]$Port,'--data',('"'+$data+'"')) -WorkingDirectory $ProgramDirectory -WindowStyle Hidden -PassThru
Write-Host "LanChat host PID=$($process.Id), TCP=$Port, attachment=$MaxAttachmentMb MB, quota=$QuotaMb MB"
Write-Host 'This host stays running when the chat window closes. Keep the PC awake.'
