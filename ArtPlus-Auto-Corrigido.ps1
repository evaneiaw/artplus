# ArtPlus Codex Supervisor - PowerShell 5.1+
# Runs bounded, sequential Codex CLI tasks. Does not control the Codex desktop UI.
param(
    [ValidateRange(1, 1000)][int]$MaxCycles = 10,
    [string]$ProjectPath = 'H:\Desktop\artplus-lite',
    [switch]$SkipGitRepoCheck,
    [ValidateRange(0, 3600)][int]$DelaySeconds = 5
)

$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath $ProjectPath -ErrorAction Stop).Path
if (-not (Test-Path -LiteralPath (Join-Path $project 'AGENTS.md'))) {
    throw "AGENTS.md not found in $project. Aborting to avoid running in the wrong directory."
}
$codexCmd = Get-Command codex -ErrorAction SilentlyContinue
if (-not $codexCmd) { throw 'Codex CLI not found in PATH. Install it and run codex login first.' }
$stateDir = Join-Path $project '.codex-supervisor'
New-Item -ItemType Directory -Path $stateDir -Force | Out-Null
$progressFile = Join-Path $stateDir 'PROGRESS.md'
$logFile = Join-Path $stateDir 'supervisor.log'
if (-not (Test-Path -LiteralPath $progressFile)) {
    @'
# ArtPlus - Codex supervisor progress

The agent must update this file after each successful cycle with: work completed, tests and their results, changed files, blockers, and the next concrete priority. Do not claim tests passed unless executed.
'@ | Set-Content -LiteralPath $progressFile -Encoding UTF8
}
function Log([string]$message) {
    $line = ('[{0}] {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $message)
    Write-Host $line
    Add-Content -LiteralPath $logFile -Value $line -Encoding UTF8
}

$prompt = @'
You are continuing the ArtPlus project in the current working directory.
Read AGENTS.md and the relevant project status, roadmap, architecture and fidelity documents before choosing work. ArtPro+ v24 is the primary observed reference; v25 is only supplementary. Never invent UI behavior or claim unverified fidelity. Do not modify Esko reference binaries or files outside the workspace. Preserve existing work.
Read .codex-supervisor/PROGRESS.md and inspect the current repository state. Choose ONE concrete, useful, safe next task and complete it, including appropriate build/tests. Do not merely propose a plan when implementation is feasible. You may edit files inside the project; do not install software, access secrets, alter system configuration, delete data, or bypass approval/security controls. If blocked, do useful independent work if possible.
At the end, UPDATE .codex-supervisor/PROGRESS.md with a new dated cycle entry including: objective, exact changed files, commands/tests with actual results, blockers, and the next priority. Even if no code was changed, document what you verified and the blocker. Avoid repeating a completed task. Keep the final response concise.
'@

Push-Location -LiteralPath $project
try {
    Log "Starting Codex supervisor: max cycles=$MaxCycles; project=$project"
    for ($cycle = 1; $cycle -le $MaxCycles; $cycle++) {
        $lastMessage = Join-Path $stateDir ('cycle-{0:D3}-last.txt' -f $cycle)
        $runLog = Join-Path $stateDir ('cycle-{0:D3}-output.log' -f $cycle)
        $before = (Get-Item -LiteralPath $progressFile).LastWriteTimeUtc
        Log "Cycle $cycle/$MaxCycles started"
        # --full-auto uses workspace-write sandbox. Git check bypass is explicit opt-in.
        $gitCheckArgs = @()
        if ($SkipGitRepoCheck) { $gitCheckArgs += '--skip-git-repo-check' }
        # --cd makes the project explicit even if launched elsewhere.
        # 2>&1 is redirected to a per-cycle log; avoid PowerShell native stderr terminating the script.
        $oldEap = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            & $codexCmd.Source exec --cd $project --full-auto @gitCheckArgs --output-last-message $lastMessage $prompt 2>&1 |
                Out-File -LiteralPath $runLog -Encoding utf8
            $exitCode = $LASTEXITCODE
        } finally { $ErrorActionPreference = $oldEap }
        Log "Cycle $cycle exit code: $exitCode; output: $runLog"
        if ($exitCode -ne 0) {
            Log 'Stopping: Codex returned an error. Review cycle output and authentication/usage limits.'
            break
        }
        $after = (Get-Item -LiteralPath $progressFile).LastWriteTimeUtc
        if ($after -le $before) {
            Log 'Stopping: PROGRESS.md was not updated. Avoiding an unproductive loop.'
            break
        }
        if ($cycle -lt $MaxCycles -and $DelaySeconds -gt 0) { Start-Sleep -Seconds $DelaySeconds }
    }
    Log 'Supervisor finished. Inspect .codex-supervisor/PROGRESS.md and cycle logs.'
} finally { Pop-Location }
