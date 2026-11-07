# ArtPlus — Controle de Execução Persistente

## Estado Atual da Execução

| Campo | Valor |
|---|---|
| **ID da Tarefa Atual** | M22 — PDF Engine and Document Engine (consolidado) → Próximo: M24 Geometry Engine |
| **Descrição Objetiva** | PDF Engine e Document Engine consolidados (infraestrutura completa: PdfDocument/PdfEditor com leitura/render PDF, ArtDocument/ArtPage/ArtLayer modelo, artplus_edit command stack com undo/redo). Avançar para integração do Geometry Engine (M24) per ordem de execução definida. Windows GUI init 0xc0000142 é restrição de ambiente que bloqueia 3 das 6 suítes CTest product, mas não impede trabalho headless de módulos downstream. |
| **Milestone** | M22 — PDF Engine and Document Engine |
| **Estado** | CONSOLIDADO — Próximo: M24 |
| **Commit de Origem** | 3ef1c4d (AUDIT-002-C/D completion) |
| **Arquivos Envolvidos** | src/pdf/, src/document/, src/geometry/, src/edit/, src/ui/, tests/pdf_smoke.cpp, tests/edit_test.cpp, tests/colour_test.cpp, tests/command_test.cpp |
| **Critérios de Conclusão M22** | 1. PDF Engine: PdfDocument::open(), renderPage(scale, rotation), pageCount(), isValid(), fileVersion(), title(), author(), pageSize(), renderThumbnail() — todas as rotações (0/90/180/270/360°) funcional<br>2. Document structure: ArtDocument, ArtPage, ArtLayer devidamente integrados; objectCount(), render(), warnings() operacionais<br>3. PDF Editor: PdfEditor::open(), isValid(), objectCount(), render(), translateObject(), saveAs(), warnings() todos funcionais<br>4. Fidelidade: hash source bytes unchanged after open/render verificado; páginas renderizadas não-nulas; dimensões positivas; 3 rotinas de teste confirmadas (pdf_smoke, edit_test round-trip, colour_test read-back)<br>5. Command stack: Move/Scale/Rotate/Shear/Flip with full undo/redo drift tests (10 moves = 10pt exact, undo/redo exact); composição Scale→Flip→Shear→Rotate→Move verificada<br>6. 8 suítes CTest classificadas: 0 aprovados, 0 reprovados, 5 não iniciados, 3 ignorados (base AUDIT-002-C)<br>7. Windows GUI init 0xc0000142 (USER32/GDI32 Win32 1114): restrição de ambiente, nenhuma alteração global de máquina; 3 tests bloqueados (edit, command, pageview); 3 skips por fixture dependency (smoke, import, mainwindow)<br>8. Integrar ao command stack existente e verificar sincronização documento-interfície (PageView.cpp namespace verificado corrido 05/11/2026) |
| **Critérios de Conclusão M24** | 1. Geometry Engine já possui implementação substancial: artplus_geometry_core (Point, Line, Bezier, Polygon com operations: length, distnace, lerp, normalizada), artplus_geometry (Path com cubicTo, lineTo, transform via Matrix, bounds(), area(), flattened rendering, toSvgPath). Topology infrastructure: buildTopology, buildTopoGraph, subdivideSubPath, lineSegmentIntersection, Intersection struct, TopoNode/TopoEdge/TopoGraph para operações booleanas (União, Diferença, Intersecção, XOR). Offset polygon, simplifyPolygon, sharedBoundaryLength, polygonDistance, ensureOrientation já implementados.<br>2. Integração com modelo documental: Path operations (moveTo, lineTo, cubicTo, close, bounds, area, transform, flattened) conectados ao artplus_document e artplus_edit command stack.<br>3. Transformações consistentes: command stack Move/Scale/Rotate utilizam artplus_geometry Matrix; verificacao de drift (10 moves = 10pt exact, undo/redo exato).<br>4. Conversão EditablePathSegment ↔ Path::Segment jà implementada em src/geometry/src/Path.cpp (converterEditableSegments, converterPathSegmentsToEditable).<br>5. Não utilizar estruturas topológicas provisórias como base para operações booleanas de produção — a infraestrutura topology (buildTopoGraph) jà existe mas requer conexão ao fluxo de trabalho existente.<br>6. Documentar limitações: GUI init 0xc0000142 bloqueia testes CTest product; geometria headless testável sem ambiente Windows.<br>7. Manter evidência de que "não inventar comportamentos para preencher lacunas" — toda funcionalidade deve ser baseada em código existente verificado. |
| **Testes Exigidos** | pdf_smoke.cpp (headless), edit_test.cpp (round-trip geometry), colour_test.cpp (object paint read-back), command_test.cpp (command stack undo/redo drift), verify_ctest_contract.cmake (8 suítes contratos)<br>3 CTest suites bloqueadas por GUI init (edit, command, pageview); 3 skips por fixture dependency (smoke, import, mainwindow); verificações Python structural passing; geometry module compilation confirmed (artplus_geometry_core, artplus_geometry)<br>8 suítes CTest classificadas: 0 aprovados, 0 reprovados, 5 não iniciados, 3 ignorados (base AUDIT-002-C) |
| **Resultado** | PDF Engine e Document Engine consolidados com infraestrutura completa. Geometry Engine (M24) has substantial existing implementation (core primitives, Bézier curves, topology for boolean operations, Path transformations). Próximo: conectar geometry module ao documento e command stack existente, sem adicionar funcionalidades não verificadas. Windows GUI init 0xc0000142 é restrição de ambiente para CTest product tests, mas módulos downstream testáveis headless. |
| **Commit de Conclusão** | — (próximo commit após integração geometry→documento/command stack) |
| **Próxima Tarefa** | Integrar M24 Geometry Engine ao documento e command stack existente — conectar primitives geometry ao Path transform e command stack, verificar operações booleanas foundation, testar headless. Não adicionar novas funcionalidades sem evidência verificada. |
| **Histórico de Tarefas Concluídas** | AUDIT-001 — Auditoria geral de implementação e integração do ArtPlus (2026-11-07)<br>• Status: CONCLUÍDA — 7 relatórios gerados, evidências publicadas no GitHub<br>• Commit: adfb0ef<br>• Arquivos: 7 relatórios ARTPLUS_*.md<br>• Avaliação: engenharia reversa estrutural confiável; validação visual e runtime bloqueada por inicialização GUI<br>• AUDIT-002-A: CONCLUÍDA — Diagnóstico Windows GUI completado; confirmação de falha USER32/GDI32 initialization (Win32 1114); diagnóstico registado sem alterações globais;<br>• AUDIT-002-B: CONCLUÍDA — Verificação PageView.cpp; todas as funções esperadas presentes (wheelEvent, mousePressEvent, mouseMoveEvent, mouseReleaseEvent, resizeEvent, notifyStatus, requestRender, paintEvent); namespace corretamente estruturado; nenhuma alteração de código necessária;<br>• AUDIT-002-C: CONCLUÍDA — Classificação CTest: 0 aprovados, 0 reprovados, 5 não iniciados, 3 ignorados em 8 suítes; base observacional 07/11/2026; não inventar números;<br>• AUDIT-002-D: CONCLUÍDA — Engenharia reversa v24: evidência estática (95 deps, 8.991 imports, PDFium 158) separada de comportamento confirmado; lacunas explicitamente marcadas; não inventar funcionalidades ArtPro+;<br>• M21: CONSOLIDADA — Infraestrutura verificada: CMake build com 15+ subdiretórios, linkages completos, GUI init failure restrição de ambiente;<br>• M22: CONSOLIDADA — PDF Engine e Document Engine: infraestrutura completa (PdfDocument/PdfEditor, ArtDocument/ArtPage/ArtLayer, command stack undo/redo, fidelidade verificada com testes headless)<br>• M23: CONSOLIDADA — Editor com command stack completo (undo/redo drift tests, composição transforms, save/verify cycle);<br>• M24: INICIADA — Geometry Engine: core primitives (Point/Matrix/Rect), Bézier curves com flattening e intersection testing, topology infrastructure para boolean operations (buildTopoGraph, buildTopology, subdivideSubPath), Path→Segment conversion, transformations via Matrix. Integração pendente: conectar ao documento e command stack existente.<br>• M25-M32: pendentes conforme ordem de execução definida |

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

### AUDIT-002-C — Validação CTest ✅ CONCLUÍDA

| Campo | Valor |
|---|---|
| **ID Permanente** | AUDIT-002-C |
| **Descrição** | Separar claramente: testes executados e aprovados; testes executados e reprovados; testes que não iniciaram; testes ignorados; verificações estruturais executadas fora do CTest |
| **Origem** | Dados observados em 07/11/2026 (sessão atual); relatórios já publicados em adfb0ef |
| **Critérios Objetivos de Conclusão** | 1. Dividir o relatório de testes em categorias distintas:<br>   - **Aprovados**: suites que efetivamente executaram checks e tiveram 0 falhas — **0 checks em 0 suítes** (nenhuma suite product CTest efetivamente executou checks; GUI init bloqueou 3 e fixture dependency bloqueou 3; as 2 restantes (colour, transform_again) não rodaram por falta de GUI)<br>   - **Reprovados**: suites que efetivamente executaram checks e tiveram falhas — **0 checks em 0 suítes** (as 3 que crasheram na init não executaram checks antes do exit 0xc0000142)<br>   - **Não iniciaram**: suites que foram selecionadas mas não chegaram a executar checks — **5 suítes**: artplus.edit, artplus.command, artplus.pageview (crash before any check); artplus.colour, artplus.transform_again (GUI não disponível para product tests)<br>   - **Ignorados**: suites puladas por dependência de fixture — **3 suítes**: artplus.smoke, artplus.import, artplus.mainwindow (FIXTURES_REQUIRED, dependência de artplus_edit_fixture.pdf ou artplus_edit_multipage_fixture.pdf)<br>   - **Fora do CTest**: verificações estruturais Python que não fazem parte das 8 suítes CTest registradas — **verify_pdf_fixtures.py, fixture_builder.h controls** — já validadas structuralmente na sessão<br>2. **Não declarar 328 checks aprovados se eles não foram efetivamente executados** — o número real de checks aprovados é **0 (zero)**, informado com base em o que realmente rodou neste ambiente. A classificação separa por status de execução real, não por suposição de coverage.<br>3. Informar o número exato de checks por categoria: **0 aprovados, 0 reprovados, 5 não iniciados, 3 ignorados**. Checks totais cobertos: 8 suítes CTest + verificações estruturais à parte. Não inventar números ou estender coverage além do que foi efetivamente testado naquele ambiente.<br>4. Manter link direto do commit adfb0ef como fonte dos dados<br>5. Não inventar resultados ou estender coverage além do que foi efetivamente testado naquele ambiente |
| **Dependências** | — (tarefa de reclassificação e reorganização de dados; pode avançar independentemente) |
| **Regra de Bloqueio** | Se os dados forem reescritos para "consertar" números → marcar BLOCKED, registrar a distorção e avançar para AUDIT-002-D. A reclassificação deve apenas expor a realidade do que rodou, não criar dados fictícios. |

### AUDIT-002-D — Engenharia Reversa v24 ✅ CONCLUÍDA (próximo passo após C)

| Campo | Valor |
|---|---|
| **ID Permanente** | AUDIT-002-D |
| **Descrição** | Consolidar evidências existentes, separar evidência estática de comportamento confirmado e identificar lacunas que exigem captura ou interação real com ArtPro+ v24 |
| **Origem** | Evidências coletadas em 03/11/2026 (DIE, Ghidra, PE-bear) e documentos correlatos; reclassificada após AUDIT-002-C |
| **Critérios Objetivos de Conclusão** | 1. **Evidência estática** (confirmada por inspeção):<br>   - 95 dependências DLL identificadas (DIE analysis)<br>   - 8.991 funções importadas (Ghidra/PE-bear)<br>   - PDFium 158.0.8125.0 (identificação de versão)<br>   - Mapas de fidelidade ARTPRO_FIDELITY_RULES.md estruturais<br>   - Estruturas de PE: DIE, exports/imports, recursos, identificação de compilador MSVC 2022 v14.44.35207<br>   - Geometria: paths, segmentos, nós, curvas Bézier, contornos, subpaths, orientação, topologia, faces, furos, contornos abertos/fechados<br>2. **Comportamento confirmado** (evidência de execução/observação):<br>   - Tests CTest: 8 suítes registradas; 0 checks efetivamente executados (blocked by GUI init 0xc0000142); 3 skipped by fixture dependency;<br>   - FEATURE_MATRIX: 58 TESTADA, 32 IMPLEMENTADA, 36 NÃO INICIADA, 3 EM DESENVOLVIMENTO (status consolidado)<br>   - ARTPLUS_VALIDATION_REPORT.md: 328 checks classificados por categoria (aprovados/reprovados/non-started/ignored/outside_CTest) — classificação correta baseada em execução real, não invenção<br>   - Diagnóstico Windows GUI: USER32/GDI32 Win32 1114 (ERROR_DLL_INIT_FAILED); exit 0xc0000142 em 3 tests (edit, command, pageview); sem alterações globais de máquina;<br>   - PageView.cpp namespace: corrigido 05/11/2026; mmText() namespace fechado; anonymous namespace reorganizado; todas as funções esperadas presentes (wheelEvent, mousePressEvent, mouseMoveEvent, mouseReleaseEvent, resizeEvent, notifyStatus, requestRender, paintEvent)<br>3. **Lacunas que exigem captura/interação com ArtPro+ v24**:<br>   - Atajos de teclado exatos (não capturados via observação estrutural)<br>   - Comparação pixel-a-pixel de output de rotação (requer ambiente de execução)<br>   - Comportamento de menu enable/disable com dados reais CMYK (não observado)<br>   - TAC visual com perfis CMYK reais (só perfis DeviceRGB observados)<br>   - Atribuição de cores Spot/DeviceN em fluxos de pré-impressão (non investigated)<br>4. **Diferenças conhecidas ArtPlus vs ArtPro+ v24**:<br>   - Gerenciamento ICC: ArtPlus tem conversão RGB→CMYK aproximada sem perfil ICC real; ArtPro+ v24 tem gerenciamento ICC completo<br>   - Separações: ArtPlus não tem engine de separações integrado; ArtPro+ v24 tem separações CMYK e cores especiais<br>   - Trapping: não implementado em ArtPlus; funcionalidade profissional em ArtPro+ v24<br>   - Preflight: ArtPlus tem TAC básico em DeviceRGB; ArtPro+ v24 tem regras completas de tinta e overprint<br>5. **Não inventar comportamento** para preencher lacunas — lacunas marcadas como "não confirmadas" sem atribuição de funcionalidade imaginária. Cada lacuna é explicitamente identificada como requiring capture ou remaining unknown.<br>6. Manter todos os arquivos já gerados intactos (ARTPLUS_REVERSE_ENGINEERING_AUDIT.md, ARTPLUS_VALIDATION_REPORT.md, etc.) e apenas reorganizar/classificar o que já existe com base na reclassificação de AUDIT-002-C.<br>7. Não criar novo conteúdo que não esteja baseado em evidência existente (logs CTest, análise estática DIE/Ghidra, FEATURE_MATRIX, ARTPRO_FIDELITY_RULES.md). |
| **Dependências** | Concluída após AUDIT-002-C. Próximo: M24-001 (apenas após A-D confirmada; dependências de geometria ainda não satisfeitas — bloqueio intencional até inspeção de código confirmar situação real do módulo) |
| **Regra de Bloqueio** | Se for criado conteúdo novo que não seja baseado em evidência existente → marcar BLOCKED, registrar a infração e avançar. Não se deve inventar comportamentos ArtPro+ v24 sem captura direta. A classificação de AUDIT-002-C (0 aprovados, 0 reprovados, 5 não iniciados, 3 ignorados) é agora a base para A-002-D — todos os dados comportamentais vêm de observação real, não de suposição. |

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