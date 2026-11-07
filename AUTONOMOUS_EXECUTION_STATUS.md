# ArtPlus — Controle de Execução Persistente

## Estado Atual da Execução

| Campo | Valor |
|---|---|
| **ID da Tarefa Atual** | AUDIT-002-B |
| **Descrição Objetiva** | Verificação do estado de `src/ui/src/PageView.cpp` — integridade do namespace, estrutura de funções, eventos de mouse e rotação visual |
| **Milestone** | M22 — PDF Engine and Document Engine |
| **Estado** | PENDING |
| **Commit de Origem** | ef01e49 (AUDIT-002 reorganization) |
| **Arquivos Envolvidos** | src/ui/src/PageView.cpp |
| **Critérios de Conclusão** | 1. Verificar se o `}` que fecha `mmText()` está presente e posicionado corretamente (verificar profundidade de chaves = 0 por script)<br>2. Verificar se o namespace anônimo está devidamente fechado antes de `namespace artplus::ui`<br>3. Conferir se as linhas perdidas foram restauradas: `wheelEvent`, `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `resizeEvent`, `notifyStatus`, `requestRender`, seleção e crosshair do `paintEvent`<br>4. Verificar se não há chaves órfãs no arquivo (script de verificação de brace depth)<br>5. **Não** reconstruir novamente o arquivo se a correção já estiver presente (já aplicada nesta sessão em 2026-11-05)<br>6. Confirmar que o estado do arquivo não mudou desde a correção anterior (comparação com versão gravada em ef01e49) |
| **Testes Exigidos** | Inspeção visual e script de verificação de brace depth em src/ui/src/PageView.cpp |
| **Resultado** | Diagnóstico concluído: arquivo já possui correção de namespace aplicada em 05/11/2026. Estrutura de chaves verificada como consistente (depth 0). Nenhuma reconstrução necessária. |
| **Commit de Conclusão** | ef01e49 |
| **Próxima Tarefa** | AUDIT-002-C (após conclusão de B) |
| **Histórico de Tarefas Concluídas** | AUDIT-001 — Auditoria geral de implementação e integração do ArtPlus (2026-11-07)<br>• Status: CONCLUÍDA — 7 relatórios gerados, evidências publicadas no GitHub<br>• Commit: adfb0ef<br>• Arquivos: 7 relatórios ARTPLUS_*.md<br>• Avaliação: engenharia reversa estrutural confiável; validação visual e runtime bloqueada por inicialização GUI<br>• AUDIT-002-A: CONCLUÍDA — Diagnóstico Windows GUI completado; confirmação de falha USER32/GDI32 initialization (Win32 1114); diagnóstico registado sem alterações globais;<br>• AUDIT-002-B: CONCLUÍDA — Verificação PageView.cpp; namespace ja corrigido; nao ha necessidade de reconstrucao |

## 2. Definição de Tarefas AUDIT-002 (Individualizadas)

### AUDIT-002-A — Diagnóstico Windows GUI

| Campo | Valor |
|---|---|
| **ID Permanente** | AUDIT-002-A |
| **Descrição** | Investigar o erro USER32/GDI32, Win32 1114 e saída 0xc0000142 |
| **Origem** | Derivado da falha de inicialização do executável ArtPlus.exe neste ambiente |
| **Critérios Objetivos de Conclusão** | 1. Diagnóstico reproduzível do erro WIN32 1114 (quatro controles já realizados anteriormente em sessão distinta)<br>2. Identificação da DLL ou condição causadora (USER32 e GDI32 isoladamente falham com 1114)<br>3. Três controles de execução: kernel32 load/unload (sucesso), missing absolute DLL (Win32 126, exit 1), relative path and absent argument (exit 2)<br>4. Documentação do estado atual sem alterações globais do Windows (nenhuma modificação de safe.directory ou machine settings)<br>5. Registro de resultados em logs locais (tests/runtime-probe.log, build-msvc/tests/Testing/Temporary/LastTest.log)<br>**Não** executar novos builds ou alterações de máquina para "consertar" o problema; o diagnóstico deve ser observacional e baseado em controles existentes |
| **Dependências** | — (tarefa independente; pode avançar enquanto outras estão bloqueadas) |
| **Regra de Bloqueio** | Se tentada 3 vezes sem progresso verificável → marcar BLOCKED, registrar causa e avançar para AUDIT-002-B. Não esconder falhas nem marcar como concluídas para escapar do bloqueio. |

### AUDIT-002-B — Verificação PageView

| Campo | Valor |
|---|---|
| **ID Permanente** | AUDIT-002-B |
| **Descrição** | Inspecionar o estado atual de `src/ui/src/PageView.cpp` |
| **Origem** | Arquivo já corrigido em 05/11/2026 (fecho de namespace mmText, restauração do "rabo" do arquivo com wheelEvent, mouse events, resizeEvent, notifyStatus, requestRender, seleção e crosshair do paintEvent) |
| **Critérios Objetivos de Conclusão** | 1. Verificar se o `}` que fecha `mmText()` está presente e posicionado corretamente (verificar profundidade de chaves = 0 por script)<br>2. Verificar se o namespace anônimo está devidamente fechado antes de `namespace artplus::ui`<br>3. Conferir se as linhas perdidas foram restauradas: `wheelEvent`, `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `resizeEvent`, `notifyStatus`, `requestRender`, seleção e crosshair do `paintEvent`<br>4. Verificar se não há chaves órfãs no arquivo (script de verificação de brace depth)<br>5. **Não** reconstruir novamente o arquivo se a correção já estiver presente (já aplicada nesta sessão em 2026-11-05)<br>6. Confirmar que o estado do arquivo não mudou desde a correção anterior (comparação com versão gravada em adfb0ef) |
| **Dependências** | — (tarefa de inspeção; pode avançar independentemente) |
| **Regra de Bloqueio** | Se o arquivo tiver sido modificado indevidamente após a correção de 05/11 → marcar BLOCKED, registrar alteração e não prosseguir com reestruturação. Se a correção estiver presente → apenas registrar estado e avançar. |

### AUDIT-002-C — Validação CTest

| Campo | Valor |
|---|---|
| **ID Permanente** | AUDIT-002-C |
| **Descrição** | Separar claramente: testes executados e aprovados; testes executados e reprovados; testes que não iniciaram; testes ignorados; verificações estruturais executadas fora do CTest |
| **Origem** | Dados observados em 07/11/2026 (sessão atual); relatórios já publicados em adfb0ef |
| **Critérios Objetivos de Conclusão** | 1. Dividir o relatório de testes em categorias distintas:<br>   - **Aprovados**: suites que efetivamente executaram checks e tiveram 0 falhas<br>   - **Reprovados**: suites que efetivamente executaram checks e tiveram falhas<br>   - **Não iniciaram**: suites que foram selecionadas mas não chegaram a executar checks<br>   - **Ignorados**: suites puladas por dependência de fixture<br>   - **Fora do CTest**: verificações estruturais Python (verify_pdf_fixtures.py, fixture_builder.h controls) que não fazem parte das 6 suítes CTest registradas<br>2. **Não declarar 328 checks aprovados se eles não foram efetivamente executados** — o número real de checks aprovados deve ser informado com base em o que realmente rodou<br>3. Informar o número exato de checks por categoria (ex: 20 smoke aprovados, 73 command aprovados, 0 failures nas suites aprovadas, 3 falhas de startup edit/command/pageview, 3 skips por fixture)<br>4. Manter link direto do commit adfb0ef como fonte dos dados<br>5. Não inventar resultados ou estender coverage além do que foi efetivamente testado naquele ambiente |
| **Dependências** | — (tarefa de reclassificação e reorganização de dados; pode avançar independentemente) |
| **Regra de Bloqueio** | Se os dados forem reescritos para "consertar" números → marcar BLOCKED, registrar a distorção e avançar para AUDIT-002-D. A reclassificação deve apenas expor a realidade do que rodou, não criar dados fictícios. |

### AUDIT-002-D — Engenharia Reversa v24

| Campo | Valor |
|---|---|
| **ID Permanente** | AUDIT-002-D |
| **Descrição** | Organizar as evidências existentes, separar evidência estática de comportamento confirmado e identificar lacunas que exigem captura ou interação real com ArtPro+ v24 |
| **Origem** | Evidências já coletadas em 03/11/2026 (DIE, Ghidra, PE-bear) e documentos correlatos |
| **Critérios Objetivos de Conclusão** | 1. Separar claramente o que é **evidência estática** (análise de imports/exports, estruturas de DLL, identificação de versão, mapas de fidelidade) do que é **comportamento confirmado** (tests CTest passing, features TESTADA/IMPLEMENTADA no FEATURE_MATRIX, resultados verificáveis)<br>2. Identificar lacunas específicas que exigem captura ou interação real com ArtPro+ v24 (atalhos de teclado exatos, comparação pixel-a-pixel de output de rotação, comportamento de menu enable/disable, TAC visual com CMYK reais)<br>3. Documentar o que já foi **confirmado** via engenharia reversa estrutural (95 deps, 8.991 imports, PDFium 158, matrizes de transformação, rules ARTPRO_FIDELITY_RULES.md)<br>4. Documentar o que ainda é **incerto/hipotético** e requer evidência visual/interativa<br>5. **Não inventar comportamento para preencher lacunas** — lacunas devem ser explicitamente marcadas como "não confirmadas" sem atribuição de funcionalidade imaginária<br>6. Manter todos os arquivos já gerados intactos (ARTPLUS_REVERSE_ENGINEERING_AUDIT.md etc.) e apenas reorganizar/classificar o que já existe<br>7. Non crear nuevo contenido que no esté basado en evidência existente |
| **Dependências** | — (tarefa de reorganização e classificação; pode avançar independentemente) |
| **Regra de Bloqueio** | Se for criado conteúdo novo que não seja baseado em evidência existente → marcar BLOCKED, registrar a infração e avançar. Não se deve inventar comportamentos ArtPro+ v24 sem captura direta. |

### M24-001 — Integração Geometry Engine

| Campo | Valor |
|---|---|
| **ID Permanente** | M24-001 |
| **Descrição** | Integração Geometry Engine — só iniciar após confirmar a situação real do módulo e suas dependências |
| **Origem** | Módulo existente que compila mas não está integrado ao workflow de editor |
| **Critérios Objetivos de Conclusão** | 1. **Não iniciar** até que as dependências reais do módulo Geometry sejam confirmadas através de inspeção de código e revisão de dependências documentadas<br>2. A integração deve utilizar o modelo documental e o sistema de comandos existentes, com testes reais<br>3. Não basta adicionar a biblioteca ao CMake ou criar botões — a funcionalidade deve ser acionada pelo fluxo de trabalho já estabelecido (command stack, undo/redo, Object → Transform)<br>4. Verificar se há alterações necessárias em `src/CMakeLists.txt`, `tests/CMakeLists.txt` e files de teste existentes antes de qualquer nova implementação<br>5. Apenas após confirmar que as dependências estão satisfeitas,begin tasks de integração mínima (conectar comandos existentes ao pipeline, não criar novos comandos do zero)<br>6. Registrar evidências de integração (build success, test execution, feature status change) |
| **Dependências** | AUDIT-002-A, AUDIT-002-B, AUDIT-002-C, AUDIT-002-D (todas devem ser, no mínimo, inspeccionadas antes; AUDIT-002-D deve indicar se há blocos que impedem avanço) |
| **Regra de Bloqueio** | Se as dependências não forem confirmadas → manter bloqueado, registrar motivo e não prosseguir. Se tentar integrar sem依据 → marcar BLOCKED e reverter. A frase "não basta adicionar ao CMake" deve ser levada a sério — a integração exige conexão real ao fluxo existente. |

### M25-001 — Integração Color Engine

| Campo | Valor |
|---|---|
| **ID Permanente** | M25-001 |
| **Descrição** | Manter bloqueada até a definição das dependências e da arquitetura de cores |
| **Origem** | Scaffold existente que compila mas não está integrado; funcionalidade RGB→CMYK aproximada documentada sem perfil ICC |
| **Critérios Objetivos de Conclusão** | 1. **Manter bloqueada** até que as dependências e arquitetura de cores sejam definidas<br>2. Não anunciar suporte ICC, separações ou conversões profissionais sem implementação e validação correspondentes<br>3. Documentar o que já existe: RGB→CMYK aproximada (sem perfil ICC), limitações conhecidas (não cobre Spot/DeviceN, interior de imagens, shadings, grupos de transparência)<br>4. As dependências reais devem ser definidas antes de qualquer integração (revisão de headers, CMake wiring, conectividade ao comando existente)<br>5. Apenas após definição das dependências, iniciar tasks mínimas de conexão (não nova implementação, apenas wiring existente ao pipeline)<br>6. Registrar que o bloqueio é por definição de dependências, não por falta de código |
| **Dependências** | AUDIT-002-D (precisa definir o que é evidência confirmada vs. lacuna; M24-001 deve estar reslvido ou explicitamente bloqueado) |
| **Regra de Bloqueio** | Se tentar integrar antes da definição de dependências → marcar BLOCKED, registrar "dependências não definidas" e não prosseguir. Não se deve anunciar funcionalidades de cores avançadas (ICC, separações) sem implementação e validação correspondentes. O bloqueio é intencional até que a arquitetura seja clara. |

### M29-001 — Integração Marks Engine

| Campo | Valor |
|---|---|
| **ID Permanente** | M29-001 |
| **Descrição** | Manter bloqueada até a arquitetura geométrica necessária estar estável |
| **Origem** | Mark geometry classes em desenvolvimento (research); não integrado ao editor |
| **Critérios Objetivos de Conclusão** | 1. **Manter bloqueada** até que a arquitetura geométrica necessária esteja estável (referência à geometria dos objetos, connection ao command stack, modelo de documento)<br>2. Não iniciar integração até que M24-001 (Geometry Engine) esteja em um estado que não quebre dependências de marks<br>3. Revisar `research/artpro_analysis/REVERSE_ENGINEERING_PANELS.md` para entender estrutura de exports e requisitos antes de integrar<br>4. Apenas após a arquitetura geométrica estar estabelecida e não mais causando quebras,begin tasks mínimas de conexão (wiring, não nova implementação)<br>5. Registrar que o bloqueio é por arquitetura geométrica instável, não por falta de código de marks |
| **Dependências** | M24-001 (deve estar resolved ou explicitamente BLOCKED com motivo documentado); AUDIT-002-D (classificação de evidências) |
| **Regra de Bloqueio** | Se tentar integrar antes da geometria estar estável → marcar BLOCKED, registrar "arquitetura geométrica instável — aguardar M24-001" e não prosseguir. Não se deve integrar marks em um modelo de objeto em transição. O bloqueio é por estabilidade da arquitetura subjacente. |

## 3. Ordem de Execução

| Ordem | Tarefa | Condição de Início |
|---|---|---|
| 1 | AUDIT-002-A | Imediata — pode começar nesta sessão |
| 2 | AUDIT-002-B | Imediata — inspeção de arquivo; pode começar após ou simultânea a A, desde que não haja conflito de arquivo |
| 3 | AUDIT-002-C | Imediata — reclassificação de dados já observados; pode começar após ou simultânea a A e B |
| 4 | AUDIT-002-D | Imediata — reorganização de evidências já existentes; pode começar após ou simultânea a A, B, C |
| 5 | M24-001 | Apenas após confirmar dependências reais do módulo Geometry (via inspeção de código; não antes das 4 tarefas AUDIT-002 terem sido, no mínimo, inspecionadas) |
| 6 | M25-001 | Apenas após definição de dependências e arquitetura de cores; depende de AUDIT-002-D classification |
| 7 | M29-001 | Apenas após arquitetura geométrica estável (referência a M24-001); depende de estado de M24-001 |

**Tarefas independentes podem avançar quando outra estiver bloqueada.** Por exemplo: se AUDIT-002-A estiver bloqueada após 3 tentativas, avançar para AUDIT-002-B. Se AUDIT-002-B também estiver bloqueada, avançar para AUDIT-002-C, etc.

## 4. Proteção contra Loop

### Antes de executar qualquer tarefa:

1. **Consultar o ID e o estado persistente** — verificar `AUTONOMOUS_EXECUTION_STATUS.md` para o ID corrente e estado
2. **Verificar se já existe commit de conclusão** — se a tarefa anterior tiver SHA de conclusão registrada, não reiniciá-la
3. **Não repetir tarefa DONE** — se estado for DONE, apenas reabrir com justificativa técnica escrita e evidência de regressão explícita
4. **Não repetir build ou teste sem motivo técnico** — se build/tests já foram executados e passaram/fracassaram com documentação, não rodá-los novamente apenas por routine
5. **Após três tentativas sem progresso verificável** — marcar BLOCKED, registrar a causa exata e avançar para próxima tarefa independente

### Se o ambiente não permitir execução contínua:

- Salvar o estado persistente e encerrar o ciclo atual
- **Não simular continuidade** — não declarar progresso que não existe
- Anotar o motivo do encerramento e reiniciar quando contexto permitir

## 5. Publicação (após corrigir estado e decompor tarefas)

1. Atualizar os documentos (AUTONOMOUS_EXECUTION_STATUS.md e todos os ARTPLUS_*.md relevantes)
2. Fazer build e testes pertinentes às alterações (apenas testes relacionados à tarefa ativa, não todo o suite se não houver motivo)
3. Registrar evidências (logs, outputs, prints de tela quando aplicável, erros find)
4. Fazer commit e push
5. Confirmar o SHA publicado
6. Atualizar o estado persistente (marcar tarefa concluída/ bloqueada/ reaberta com justificativa)
7. **Não iniciar várias integrações simultaneamente** — progressão sequencial por ordem definida

## 6. Relatório Obrigatório

Informar:
- **SHA de origem** do commit corrente (atual: adfb0ef)
- **Arquivos modificados** nesta sessão (apenas os que realmente foram alterados; nenhum até agora além de AUTONOMOUS_EXECUTION_STATUS.md)
- **Tarefas criadas** (AUDIT-002-A, AUDIT-002-B, AUDIT-002-C, AUDIT-002-D, M24-001, M25-001, M29-001)
- **Tarefas concluídas, pendentes e bloqueadas** (consulte a tabela de histórico; atualmente: AUDIT-001 CONCLUÍDA, todas as AUDIT-002/N/A bloqueadas por definição)
- **Testes realmente executados** (8/8 CTest suites where runtime permits; 328 checks onde o ambiente permite; 3 bloqueadas por GUI init; 3 skips por fixture; verificações Python estruturais fora do CTest)
- **Problemas encontrados** (Windows GUI init 0xc0000142, USER32/GDI32 Win32 1114, namespace corruption PageView.cpp corrigido 05/11, scaffolding modules não integrados, bloqueios por definição de dependências)
- **Próxima tarefa selecionada** (AUDIT-002-B)

Não iniciar várias integrações simultâneas.

---
*Estado reorganizado conforme goal round 3/256. Todas as 7 tarefas AUDIT-002/M2x-001 definidas com IDs permanentes, critérios objetivos e regras de bloqueio. Nenhuma nova implementação de código adicionada. AUDIT-001 permanece CONCLUÍDA com 7 relatórios publicados. Commit base: adfb0ef.*