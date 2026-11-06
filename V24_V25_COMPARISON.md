# V24 × V25 — matriz comparativa

Referência principal: **v24 24.03.10017** (`research/re_v24/V24_REFERENCE.md`).
Referência complementar: **v25 25.03.116** (análise já produzida em
`research/artpro_analysis/01..20_*.md`).

Legenda: **CONFIRMADO** = observado diretamente/evidência inequívoca;
**FORTE INDÍCIO** = módulo presente e documentado, comportamento não comparado;
**NÃO CONFIRMADO** = sem evidência suficiente.

| Elemento | v24 | v25 | Situação |
|---|---|---|---|
| Interface | **CONFIRMADO** — capturas de workspace, menus File/Edit/Object/Select/Prepress/View e submenu Zoom (`artpro24_*.png`) | NÃO CONFIRMADO — nenhuma captura de UI da v25 neste projeto | v24 é a referência visual; v25 só por documentação |
| Menus | **CONFIRMADO** — itens, ordem, separadores e atalhos capturados | NÃO CONFIRMADO | implementar menus pela v24 |
| PDF Engine | FORTE INDÍCIO — `PDFAccessLayer` 2,52 MB, `PDFObjectActions` 7,63 MB, `PDFObjectLayer` 2,25 MB, `ArtProPDF` 0,84 MB; Ghidra em andamento | Investigado — `04_PDF_ENGINE.md` | comparar superfícies; `PDFAccessLayer` cresceu ~0,97 MB na v25 |
| Geometry Engine | FORTE INDÍCIO — `CCurve` 0,68, `CurveApprox` 0,09, `PDFGeometryLayer` 0,08, `ARDLib` 6,29 MB | Investigado — `05_GEOMETRY_ENGINE.md` | nomes iguais, tamanhos diferentes (`ARDLib` +0,27 MB na v25); comportamento não comparado |
| Color Engine | FORTE INDÍCIO — `ICC` 0,23, `LibICCProfile` 0,05, `ColorManipulationLayer` 0,32, `GMGOpenColor` 0,81 MB | Investigado — `06_COLOR_ENGINE.md` | comparar ICC/CMM e perfis |
| Trapping | FORTE INDÍCIO — `DocumentTrap` 0,77 MB | Investigado — `07_TRAPPING_ENGINE.md` | `DocumentTrap` +0,01 MB na v25 |
| Step & Repeat | FORTE INDÍCIO — `StepAndRepeat` 0,30, `StepAndRepeatCAD` 0,33 MB | Investigado — `08_STEP_AND_REPEAT.md` | `StepAndRepeatCAD` +0,11 MB na v25 (provável mudança de CAD) |
| Marks | FORTE INDÍCIO — `DocumentMarks` 4,62 MB | Investigado — `09_MARKS_ENGINE.md` | `DocumentMarks` +0,07 MB |
| Preflight | FORTE INDÍCIO — `PreflightProfileLayer` 2,28 MB, `PreflightProfileEditor.exe` 0,11 MB | Investigado — `10_PREFLIGHT.md` | `PreflightProfileLayer` −0,03 MB; editor maior na v25 (0,23 MB) |
| Automação | FORTE INDÍCIO — `DocumentAutomationEngine` 0,78, `NDLModelActionList` 2,66 MB | Investigado — `11_AUTOMATION.md` | +0,01 / +0,10 MB na v25 |
| Documento/Edição | FORTE INDÍCIO — `Document` 4,22, `DocumentEdit` 1,49, `DocumentTypesetting` 3,37 MB | Investigado — `03_DOCUMENT_ENGINE.md` | Typesetting +0,72 MB na v25 |
| OCR / Scan | `DocumentOCR` 4,23, `DocumentScanConverter` 4,19 MB | `DocumentOCR` 4,77 MB | v25 cresceu ~0,54 MB |
| Runtime Qt | Qt **6.3.1** com QtWebEngineCore (121 MB), Qml/Quick/Positioning/WebChannel | Sem WebEngineCore; `WebUIQt`/`WebBrowser`; estilo `qmodernwindowsstyle` | arquitetura de UI/web diferente |
| Módulos exclusivos | `Flex.dll`, `RC4SupportLayer.dll`, `CloudResourceSyncingLayer.dll`, `ImageList.dll` (7,79 MB), OpenCV 4.5.2, `Qt6Qml/Quick/WebEngine*`, `aws-sdk-cpp-*` | `PlateCut.dll`, `StudioCollada.dll`, `TIFFIOLayer.dll`, `WebUIQt.dll`, `WebBrowser.dll`, `CloudCore.dll`, `DocumentCloud.dll`, `DocumentReportGeneration.dll`, `DocumentDynamicContent.dll`, `SSOTokenStorage.dll`, `meshoptimizer.dll`, OpenCV 4.10 | 29 módulos só na v24; 24 só na v25 |
| Contagem/tamanho | 305 módulos, 442,7 MB | 300 módulos, 327,0 MB | v24 maior pelo WebEngine |
| Assinatura do EXE | **HashMismatch** (modificado após assinatura) | **Valid** | v24 não serve como prova de integridade; usar v25 para validar achados |
| Executável | 31.276.976 bytes, SHA-256 `9D3316…0F03` | 34.852.904 bytes, SHA-256 `D27DE8…2E4B` | binários distintos |

## Diferenças que exigem cuidado

1. **Não presumir equivalência por nome de módulo.** `CCurve`, `PDFAccessLayer`,
   `DocumentOCR`, `StepAndRepeatCAD` e outros mudaram de tamanho; funções e
   algoritmos podem ter mudado.
2. **Qt diferente** (6.3.1 na v24; sem WebEngine na v25) — a UI pode divergir em
   detalhes de estilo/comportamento. A referência visual é a v24.
3. **Módulos novos na v25** (`PlateCut`, `StudioCollada`, `TIFFIOLayer`,
   `WebUIQt`, `Cloud*`, `DocumentReportGeneration`) indicam funcionalidades que
   podem não existir na v24 — não implementar sem confirmar na v24.
4. **Módulos removidos na v25** (`Flex`, `RC4SupportLayer`, `ImageList`,
   OpenCV 4.5.2, Qt WebEngine) — não implementar com base apenas na v24 se a
   função não existir na v25; registrar como diferença de versão.
5. **Assinatura**: só a v25 é íntegra segundo Authenticode.

## Próximos passos de comparação

1. Concluir Ghidra da v24 (executável) e comparar contagem/funções com os
   catálogos da v25 (`14_FUNCTION_CATALOG.md`).
2. Analisar `PDFObjectActions.dll` e `PDFAccessLayer.dll` nas duas versões.
3. Capturar a UI da v25 para confirmar diferenças de menu/painel.
4. Registrar cada divergência em `FEATURE_MAPPING_ARTPRO.md` com a versão.
