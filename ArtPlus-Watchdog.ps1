# Keeps the ArtPlus autonomous supervisor running.
param(
    [string]$ProjectPath = 'H:\Desktop\artplus-lite',
    [ValidateRange(15,300)][int]$PollSeconds = 30
)

$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath $ProjectPath).Path
$state = Join-Path $project '.codex-supervisor'
$log = Join-Path $state 'watchdog.log'
$watchdogLock = Join-Path $state 'WATCHDOG.lock'
$supervisor = Join-Path $project 'ArtPlus-Auto-v4.ps1'

if (Test-Path -LiteralPath $watchdogLock) {
    throw "Watchdog lock exists: $watchdogLock"
}
New-Item -ItemType Directory -Path $state -Force | Out-Null
Set-Content -LiteralPath $watchdogLock -Encoding ASCII -Value "PID=$PID; Started=$(Get-Date -Format o)"

function Log([string]$Message) {
    $line = '[{0}] {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $Message
    Add-Content -LiteralPath $log -Encoding UTF8 -Value $line
}

try {
    while (-not (Test-Path -LiteralPath (Join-Path $state 'STOP'))) {
        $lock = Join-Path $state 'RUNNING.lock'
        $running = $false
        if (Test-Path -LiteralPath $lock) {
            $text = Get-Content -LiteralPath $lock -Raw
            if ($text -match 'PID=(\d+)') {
                $running = $null -ne (Get-Process -Id ([int]$Matches[1]) -ErrorAction SilentlyContinue)
            }
            if (-not $running) {
                Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
                Log 'Removed stale supervisor lock.'
            }
        }
        if (-not $running) {
            $child = Start-Process -FilePath 'powershell.exe' -ArgumentList @(
                '-NoProfile','-ExecutionPolicy','Bypass','-File',$supervisor,
                '-MaxCycles','30','-TimeoutMinutes','25','-DelaySeconds','5'
            ) -WorkingDirectory $project -WindowStyle Hidden -PassThru
            Log "Started supervisor PID=$($child.Id)."
        }
        Start-Sleep -Seconds $PollSeconds
    }
    Log 'STOP file detected; watchdog ending safely.'
}
finally {
    Remove-Item -LiteralPath $watchdogLock -Force -ErrorAction SilentlyContinue
}
