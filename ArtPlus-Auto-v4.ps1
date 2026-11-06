# ArtPlus autonomous Codex supervisor - Windows PowerShell 5.1+
# Sequential runs, per-cycle timeout, persistent progress, bounded retries.
param(
    [ValidateRange(1,1000)][int]$MaxCycles = 30,
    [string]$ProjectPath = 'H:\Desktop\artplus-lite',
    [ValidateRange(5,240)][int]$TimeoutMinutes = 25,
    [ValidateRange(0,120)][int]$DelaySeconds = 5,
    [ValidateRange(0,5)][int]$MaxConsecutiveFailures = 2,
    [switch]$SkipGitRepoCheck
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath $ProjectPath).Path
if (-not (Test-Path -LiteralPath (Join-Path $project 'AGENTS.md'))) { throw "AGENTS.md missing in $project" }
$cmd = Get-Command codex.cmd -ErrorAction SilentlyContinue
if (-not $cmd) { throw 'codex.cmd not found. Check npm installation / PATH.' }
$state = Join-Path $project '.codex-supervisor'
New-Item -ItemType Directory -Path $state -Force | Out-Null
$progress = Join-Path $state 'PROGRESS.md'
$log = Join-Path $state 'supervisor.log'
$stop = Join-Path $state 'STOP'
$lock = Join-Path $state 'RUNNING.lock'
if (Test-Path $lock) { throw "Lock exists: $lock. Verify no other supervisor is running, then remove stale lock." }
if (-not (Test-Path $progress)) { Set-Content -LiteralPath $progress -Encoding UTF8 -Value '# ArtPlus autonomous progress' }
function Log([string]$s) {
    $line = '[{0}] {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'),$s
    Write-Host $line
    Add-Content -LiteralPath $log -Encoding UTF8 -Value $line
}
$prompt = @'
Continue implementing the ArtPlus project autonomously. Read AGENTS.md, project status/roadmap and .codex-supervisor/PROGRESS.md. Inspect actual current state. Select ONE feasible, useful next engineering task, implement it and test it; do not just describe a plan. ArtPro+ v24 is primary reference, v25 supplementary. Do not invent UI behavior or edit original Esko software. Work only inside this repository; do not install software, change machine settings, access secrets, or bypass safeguards. Avoid destructive actions. If a build is slow, investigate and use targeted tests; avoid repeatedly invoking the same failing long-running build. Update .codex-supervisor/PROGRESS.md at the end with what changed, real test outcomes, blockers, and next concrete task. If blocked, document the exact cause and do a different safe useful task where possible. No need to ask user to say continue between tasks; the external supervisor starts the next cycle.
'@
# Start-Process argument quoting: pass prompt via stdin file instead of command-line quoting.
$promptFile = Join-Path $state 'cycle-prompt.txt'
Set-Content -LiteralPath $promptFile -Encoding UTF8 -Value $prompt
Set-Content -LiteralPath $lock -Encoding ASCII -Value "PID=$PID; Started=$(Get-Date -Format o)"
$failures = 0
try {
    Log "Starting: cycles=$MaxCycles timeout=${TimeoutMinutes}m project=$project"
    for ($i=1; $i -le $MaxCycles; $i++) {
        if (Test-Path $stop) { Log 'STOP file detected. Ending safely.'; break }
        $tag = '{0:D3}' -f $i
        $stdout = Join-Path $state "cycle-$tag-output.log"
        $stderr = Join-Path $state "cycle-$tag-error.log"
        $last = Join-Path $state "cycle-$tag-last.txt"
        # A resposta final de um ciclo anterior não pode sinalizar conclusão
        # antes que o novo processo tenha começado.
        Remove-Item -LiteralPath $last -Force -ErrorAction SilentlyContinue
        $before = (Get-Item -LiteralPath $progress).LastWriteTimeUtc
        $args = @('exec','--cd',('"'+$project+'"'),'--sandbox','workspace-write','--output-last-message',('"'+$last+'"'))
        if ($SkipGitRepoCheck) { $args += '--skip-git-repo-check' }
        $args += '-'
        Log "Cycle $i/$MaxCycles started"
        # cmd.exe wrapper is required for .cmd; stdin supplies prompt, stdout/stderr go to separate files.
        $p = Start-Process -FilePath $cmd.Source -ArgumentList $args -WorkingDirectory $project -RedirectStandardInput $promptFile -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru -WindowStyle Hidden
        $timedOut = $false
        $stopped = $false
        $completionSeenAt = $null
        $deadline = (Get-Date).AddMinutes($TimeoutMinutes)
        while (-not $p.HasExited) {
            if (Test-Path $stop) { $stopped = $true; break }
            if ((Get-Date) -ge $deadline) { $timedOut = $true; break }
            if (Test-Path $last) {
                if (-not $completionSeenAt) { $completionSeenAt = Get-Date }
                elseif (((Get-Date) - $completionSeenAt).TotalSeconds -ge 15) {
                    Log "Cycle $i produced its final response but child stayed alive; terminating the completed child tree PID=$($p.Id)"
                    & taskkill.exe /PID $p.Id /T /F 2>&1 | Out-Null
                    break
                }
            }
            Start-Sleep -Seconds 3
            $p.Refresh()
        }
        if ($timedOut -or $stopped) {
            Log "Cycle $i interrupted: timeout=$timedOut stop=$stopped. Terminating process tree PID=$($p.Id)"
            & taskkill.exe /PID $p.Id /T /F 2>&1 | Out-Null
            try { $p.WaitForExit(10000) | Out-Null } catch {}
        } else { $p.WaitForExit() }
        $p.Refresh()
        $exitCode = if ($timedOut) { 124 } elseif ($stopped) { 130 } else { $p.ExitCode }
        Log "Cycle $i exit=$exitCode; stdout=$stdout stderr=$stderr"
        if ($stopped) { break }
        $after = (Get-Item -LiteralPath $progress).LastWriteTimeUtc
        # Some Windows .cmd wrappers expose a blank ExitCode even after the
        # child completed normally. A fresh progress timestamp is the stronger
        # evidence that the cycle actually performed its requested work.
        if ($after -gt $before -and ($exitCode -eq 0 -or $null -eq $exitCode -or $exitCode -eq '')) {
            $failures = 0
        } else {
            $failures++
            Log "Cycle $i did not complete cleanly or did not update progress. Consecutive failures=$failures"
            if ($failures -gt $MaxConsecutiveFailures) { Log 'Too many consecutive failures; stopping to avoid wasted usage.'; break }
        }
        if ($i -lt $MaxCycles -and $DelaySeconds -gt 0) { Start-Sleep -Seconds $DelaySeconds }
    }
    Log 'Supervisor finished.'
} finally {
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
