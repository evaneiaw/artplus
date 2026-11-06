param(
    [string]$Project = 'H:\Desktop\artplus-lite',
    [ValidateRange(1,100)][int]$MaxCycles = 10,
    [ValidateRange(0,600)][int]$PauseSeconds = 5
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

if (-not (Test-Path -LiteralPath $Project -PathType Container)) { throw "Pasta do projeto inexistente: $Project" }
if (-not (Test-Path -LiteralPath (Join-Path $Project 'AGENTS.md'))) { throw 'AGENTS.md nao encontrado. Confirme a pasta do projeto.' }
$codex = Get-Command codex -ErrorAction SilentlyContinue
if (-not $codex) { throw 'Codex CLI nao encontrado. Instale com npm install -g @openai/codex e autentique com codex login.' }

$stateDir = Join-Path $Project '.codex-supervisor'
New-Item -ItemType Directory -Path $stateDir -Force | Out-Null
$checkpoint = Join-Path $stateDir 'PROGRESS.md'
$stopFile = Join-Path $stateDir 'STOP'
if (-not (Test-Path $checkpoint)) {
    @'
# ArtPlus - Supervisor checkpoint

O supervisor iniciou. Cada ciclo deve atualizar este arquivo com:
- Data/hora e objetivo
- Alteracoes realizadas
- Comandos/testes e resultados
- Proxima tarefa objetiva
- Impedimentos reais
'@ | Set-Content -LiteralPath $checkpoint -Encoding UTF8
}
Write-Host "Projeto: $Project"
Write-Host "Ciclos maximos: $MaxCycles"
Write-Host "Parar: Ctrl+C ou criar $stopFile"
Push-Location $Project
try {
    for ($i = 1; $i -le $MaxCycles; $i++) {
        if (Test-Path $stopFile) { Write-Warning 'Arquivo STOP detectado. Encerrando.'; break }
        $before = (Get-FileHash -LiteralPath $checkpoint -Algorithm SHA256).Hash
        $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
        $log = Join-Path $stateDir ("cycle-{0:D2}-{1}.log" -f $i, $stamp)
        $last = Join-Path $stateDir ("cycle-{0:D2}-{1}-answer.txt" -f $i, $stamp)
        $prompt = @"
Voce e o agente de desenvolvimento do ArtPlus. Esta e a iteracao $i de $MaxCycles de um supervisor local.
Leia AGENTS.md, os documentos de estado e .codex-supervisor/PROGRESS.md. Identifique UMA tarefa concreta de maior prioridade que possa ser concluida nesta iteracao; implemente e teste dentro da pasta do projeto. Se nao houver tarefa implementavel com evidencias suficientes, faca investigacao util e registre o resultado. A referencia de fidelidade e ArtPro+ v24; v25 e complementar. Nunca invente UI, menus ou atalhos. Nao altere os binarios originais da Esko, nao apague arquivos ou historico, nao execute operacoes destrutivas, nao instale dependencias e nao modifique arquivos fora do projeto. Nao use permissoes elevadas. Nao crie commits automaticamente. Nao afirme testes sem executa-los.
OBRIGATORIO antes de terminar: atualize .codex-supervisor/PROGRESS.md com objetivo, arquivos alterados, verificacoes, resultados, pendencias e a proxima tarefa concreta. Se estiver bloqueado, explique por que e registre um passo verificavel para destravar. Se o trabalho ja estiver concluido ou nao houver proxima tarefa segura, escreva a linha exata SUPERVISOR_DONE no checkpoint.
Nao pare apenas para pedir a proxima missao: escolha-a com base nas evidencias. Trabalhe apenas dentro das permissoes atuais.
"@
        Write-Host "`n=== CICLO $i/$MaxCycles - $stamp ===" -ForegroundColor Cyan
        # Each cycle starts a new non-interactive turn; the checkpoint carries continuity.
        # Use PowerShell pipeline to pass UTF-8 prompt via stdin; avoid command-line quoting issues.
        $prompt | & $codex.Source exec --sandbox workspace-write --output-last-message $last - 2>&1 | Tee-Object -FilePath $log
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) { Write-Warning "Codex retornou codigo $exitCode. Log: $log"; break }
        if (Test-Path $stopFile) { Write-Warning 'STOP detectado.'; break }
        $after = (Get-FileHash -LiteralPath $checkpoint -Algorithm SHA256).Hash
        if ($before -eq $after) { Write-Warning "Checkpoint nao atualizado: possivel ausencia de progresso. Log: $log"; break }
        $contents = Get-Content -LiteralPath $checkpoint -Raw
        if ($contents -match '(?m)^SUPERVISOR_DONE\s*$') { Write-Host 'Projeto sem proxima tarefa segura declarada; encerrando.'; break }
        if ($i -lt $MaxCycles -and $PauseSeconds -gt 0) { Start-Sleep -Seconds $PauseSeconds }
    }
} finally { Pop-Location }
Write-Host "`nSupervisor encerrado. Leia: $checkpoint" -ForegroundColor Green
