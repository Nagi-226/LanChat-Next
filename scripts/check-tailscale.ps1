param([string]$ServerAddress='', [int]$Port=12346)
$ErrorActionPreference='Stop'
$taskTailscale=Join-Path $env:ProgramFiles 'Tailscale\tailscale.exe'
if(-not (Test-Path -LiteralPath $taskTailscale)){throw 'Install the official Tailscale Windows client first.'}
$taskRaw=& $taskTailscale status --json
if($LASTEXITCODE -ne 0){throw 'Unable to read Tailscale status.'}
$taskState=$taskRaw | ConvertFrom-Json
if($taskState.BackendState -ne 'Running'){throw ('Tailscale requires login or connection: '+$taskState.BackendState)}
if(-not $ServerAddress){$ServerAddress=@($taskState.TailscaleIPs | Where-Object {$_ -match '^\d+\.'})[0]}
if(-not $ServerAddress -or $Port -lt 1 -or $Port -gt 65535){throw 'Supply a valid server address and TCP port.'}
$taskReachable=Test-NetConnection -ComputerName $ServerAddress -Port $Port -InformationLevel Quiet -WarningAction SilentlyContinue
[pscustomobject]@{Device=$taskState.Self.HostName;Online=$taskState.Self.Online;Server=$ServerAddress;Port=$Port;TcpReachable=$taskReachable;Health=$taskState.Health;Scope='Connectivity check only; test from another PC to verify a cross-network path.'}
if(-not $taskReachable){exit 1}
