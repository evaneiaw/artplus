# MISSION_GHIDRA_V24_FINAL_REPORT

Data: 2026-11-03
Referência: ArtPro+ v24 24.03.10017 (principal), v25 25.03.116 (complementar)
Projeto Ghidra: `research/re_v24/ghidra/ArtProV24`

## Respostas obrigatórias

1. **O executável principal foi analisado integralmente?**
   **Não.** A análise estática de PE/imports/exports está completa; a análise
   automática do Ghidra **ainda está em execução** (fase de RTTI/TypeDescriptors).
2. **A análise automática terminou?**
   **Não.** Processo `java` ativo, log em `ghidra_v24_import.log`.
3. **Quantas funções foram identificadas?**
   **Ainda não é possível afirmar.** A contagem será extraída do
   `FunctionManager` quando a análise concluir. Nenhum número foi inventado.
4. **Quais funções relevantes foram investigadas manualmente?**
   Nenhuma no nível de decompilação nesta janela de tempo. Foram investigados
   exports, imports, relações entre módulos e símbolos RTTI.
5. **Quais DLLs foram analisadas?**
   Estaticamente (exports/imports/relações): `PDFObjectActions`,
   `PDFAccessLayer`, `CCurve`, `DocumentTrap`, `StepAndRepeat`,
   `DocumentMarks`, `PreflightProfileLayer`, mais `Document`, `PDFObjectLayer`
   e `DocumentEdit` como contexto. Relatórios em
   `research/re_v24/ghidra/analysis/`.
6. **Quais relações entre executável e DLLs foram comprovadas?**
   - exe → 95 DLLs, 8.991 símbolos (import table).
   - exe importa diretamente `Document.dll`, `DocumentMarks.dll`,
     `DocumentFlexoTools.dll`, `PDFConversionRules.dll`, `CurveApprox.dll`,
     `GeoMapper.dll`, `NDLModel*`, `Qt6*`, `lcms`, `YAGL`, `ArtSys`, `ADT`.
   - `PDFObjectActions`, `PDFAccessLayer`, `CCurve`, `DocumentTrap`,
     `StepAndRepeat`, `PreflightProfileLayer` **não** estão no import table do
     exe → carga dinâmica (`LoadLibraryA/W/ExA`, `GetProcAddress`).
   - `Document.dll` importa `PDFAccessLayer`, `PDFGeometryLayer`,
     `PDFObjectLayer`, `ColorManipulationLayer`, `CurveApprox`, `GeoMapper`,
     `FontLayer`, `FontNDSLayer`.
   - `PDFAccessLayer` é importada por `Document`, `DocumentEdit`,
     `DocumentMarks`, `DocumentTrap`, `PDFObjectActions`, `PDFObjectLayer`,
     `PreflightProfileLayer`.
   - `PDFObjectActions` é importada por `PreflightProfileLayer`.
7. **Quais fluxos funcionais foram identificados?**
   No nível de módulo (FORTE INDÍCIO): documento por mensagens
   (`GADocDidSave`, `GADocDidChangePages`, `GADocInsertPages`,
   `GADocDeletePages`, `GADocDigSignature`); módulos de documento registráveis
   (`Module::Create`, `kModuleID` em DocumentTrap); Step & Repeat por
   `SheetIterator`/`RunList`/`GridParameters`.
8. **Quais algoritmos ou estruturas puderam ser compreendidos?**
   - Trapping: `AddTrap`, `AutomaticTrap`, `CanTrap`, `RemoveTrap`,
     `SessionParameters`, `TrapCandidate`, `ReverseTrapping`, pullback.
   - Step & Repeat: `SNR_GridParameters`, `SheetParameters`, `InputRunList`,
     `SheetSide`, conversões JDF/MIS/XML → `Iterator2Pdf`.
   - Marcas: classes `BarcodeMark`, `CornerMark`, `CutMark`, `DynamicMark`,
     `BearerBar`, `CustomMark`, visitantes e `TaskMarkSets`.
   - Preflight: família `Gpet*` de verificações independentes.
   - `CCurve`: correção de tom (`TToneRangeCorrectionParameters`) — **não** é
     a geometria Bézier (evidência contra inferência pelo nome).
9. **Quais questões permanecem sem resposta?**
   Contagem/mapa de funções; XREFs de `LoadLibrary`; fluxo exato abrir→modelo→
   render; interface binária de `PDFAccessLayer`; algoritmo de trapping;
   estrutura de perfil de preflight; parâmetros de grid do Step & Repeat;
   comportamento do Inspector.
10. **Quais descobertas foram transformadas em requisitos?**
    - `View > Zoom > Zoom to Selection / Previous` (Ctrl+Alt+0): requisito
      implementado.
    - Modelo de trapping com sessão/candidatos; Step & Repeat por run list;
      marcas tipadas; documento por eventos — registrados em
      `FEATURE_MAPPING_ARTPRO.md` como FORTE INDÍCIO para as próximas missões.
11. **Quais descobertas resultaram em código?**
    `PageView::zoomToSelection()` + ação `Zoom to Selection / Previous`
    (Ctrl+Alt+0) + 3 testes. Nenhum código proprietário foi copiado.
12. **Quais testes foram executados?**
    Suíte completa: smoke 12, pageview 17, mainwindow 21, import 18, edit 43,
    command 58 = **169 checks, 0 falhas**.
13. **Qual foi o resultado do build?**
    Compilação MSVC/Qt/PDFium concluída sem erros.
14. **Quais recursos foram validados diretamente no v24?**
    Menus File/Edit/Object/Select/Prepress/View e submenu Zoom (capturas
    anteriores); existência de `Zoom to Selection / Previous`. O comportamento
    do zoom foi validado no ArtPlus (761,6% com o retângulo selecionado), mas
    **não** observado no v24 → FORTE INDÍCIO, não CONFIRMADO.
15. **Quais diferenças da v25 foram registradas?**
    `V24_V25_COMPARISON.md`: Qt 6.3.1+WebEngine vs sem WebEngine; 29 módulos só
    na v24 e 24 só na v25; tamanhos diferentes (`PDFAccessLayer` 2,52→3,49 MB,
    `DocumentOCR` 4,23→4,77, `DocumentTypesetting` 3,37→4,09,
    `StepAndRepeatCAD` 0,33→0,44); assinatura válida só na v25.
16. **Qual é o próximo passo concreto?**
    Concluir a análise automática; extrair contagem de funções e XREFs de
    `LoadLibrary`; criar projetos Ghidra por DLL (começando por
    `PDFObjectActions` e `PDFAccessLayer`); em paralelo, confirmar o Inspector
    no v24 antes de implementar propriedades.

## Evidências principais

| Arquivo | Evidência | Confiança |
|---|---|---|
| `static/exe_headers.txt` | PE64, base `0x140000000`, entry `0x1410729DC`, seções `.fnp_dir/.fnp_mar` | CONFIRMADO |
| `static/exe_imports.txt` | 95 DLLs, 8.991 símbolos | CONFIRMADO |
| `static/*.imports.txt` | relações Document/PDF/Preflight | CONFIRMADO |
| `DocumentTrap.dll.exports.txt` | `AddTrap`, `AutomaticTrap`, `CanTrap`, `RemoveTrap` | CONFIRMADO (existência) |
| `StepAndRepeat.dll.exports.txt` | `SheetIterator`, `SNR_GridParameters`, `Jdf2Iterator`, `Iterator2Pdf` | CONFIRMADO (existência) |
| `DocumentMarks.dll.exports.txt` | `BarcodeMark`, `CornerMark`, `CutMark`, `DynamicMark` | CONFIRMADO (existência) |
| `CCurve.dll.exports.txt` | `TToneRangeCorrectionParameters` | CONFIRMADO (existência) |
| `PDFAccessLayer.dll.exports.txt` | mensagens `GADocDid*` | CONFIRMADO (existência) |
| `ghidra_v24_import.log` | análise automática em andamento | CONFIRMADO |

## Nota de integridade

O executável v24 tem **Authenticode HashMismatch**. Achados estáticos valem
como referência funcional, mas não como prova de integridade oficial; a v25
(assinatura válida) serve de contraprova quando aplicável.
