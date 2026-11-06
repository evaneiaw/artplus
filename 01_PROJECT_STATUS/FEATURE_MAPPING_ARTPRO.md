# FEATURE_MAPPING_ARTPRO — funcionalidades × evidência × ArtPlus

Regra: ver `ARTPRO_FIDELITY_RULES.md`. Nenhum item abaixo entra na interface
sem classificação **CONFIRMADO**.

Referências de captura (pasta `research/artpro_analysis/`):

- `artpro24_menu_file.png`, `artpro24_menu_edit.png`, `artpro24_menu_object.png`,
  `artpro24_menu_select2.png`, `artpro24_menu_prepress2.png`,
  `artpro24_menu_view2.png`, `artpro24_submenu_zoom.png`
- `artpro24_full.png` (workspace completo, v24)
- `artpro24_inspector.png`, `artpro24_inspector2.png` (tentativas de Inspector)

Versão de referência operacional: **ArtPro+ v24 24.03.10017**
(`research/re_v24/V24_REFERENCE.md`). A v25 25.03.116 é referência técnica
complementar; comparação em `V24_V25_COMPARISON.md`. As capturas `artpro24_*`
são todas da v24.

## Arquivo

| Recurso | Local no ArtPro+ | Evidência | Classificação | ArtPlus |
|---|---|---|---|---|
| Open… | File > Open… (Ctrl+O) | menu_file | CONFIRMADO | implementado |
| Save | File > Save (Ctrl+S) | menu_file | CONFIRMADO | implementado (grava PDF; ArtPro salva documento nativo — diferença registrada) |
| Save As… | File > Save As… (Ctrl+Shift+S) | menu_file | CONFIRMADO | implementado (PDF) |
| Close | File > Close (Ctrl+W) | menu_file | CONFIRMADO | implementado |
| Close All | File > Close All (Ctrl+Alt+W) | menu_file | CONFIRMADO | desabilitado |
| Exit | File > Exit (Ctrl+Q) | menu_file | CONFIRMADO | implementado |
| Print… | File > Print… (Ctrl+P) | menu_file | CONFIRMADO | desabilitado |
| Pages | File > Pages ▸ | menu_file | CONFIRMADO (menu) / conteúdo NÃO CONFIRMADO | placeholder |
| Export To | File > Export To ▸ | menu_file | CONFIRMADO (menu) / conteúdo NÃO CONFIRMADO | placeholder |
| Action Lists | File > Action Lists ▸ | menu_file | CONFIRMADO (menu) / conteúdo NÃO CONFIRMADO | placeholder |
| Place File… / Place Image Inside… | File | menu_file | CONFIRMADO | desabilitado |
| Convert To PDF+ / Report / Variable Data | File | menu_file | CONFIRMADO | desabilitado |
| Page Boxes Setup… / Document Setup… | File | menu_file | CONFIRMADO | desabilitado |

## Edição

| Recurso | Local no ArtPro+ | Evidência | Classificação | ArtPlus |
|---|---|---|---|---|
| Undo | Edit > Undo (Ctrl+Z) | menu_edit | CONFIRMADO | implementado |
| Redo | Edit > Redo (Ctrl+Shift+Z) | menu_edit | CONFIRMADO | implementado (atalho corrigido) |
| Cut/Copy/Paste | Edit | menu_edit | CONFIRMADO | desabilitado |
| Paste in Front/Back/Layer Structure/Match Style | Edit | menu_edit | CONFIRMADO | desabilitado |
| Delete | Edit > Delete (Backspace) | menu_edit | CONFIRMADO | desabilitado (remoção insegura, ver ADR-0006) |
| Options… | Edit > Options… (Ctrl+K) | menu_edit | CONFIRMADO | desabilitado |
| Transform… | Object > Transform… (Ctrl+Shift+M) | menu_object + captura fornecida pelo usuário (v24, 2026-11-03) | **CONFIRMADO** | implementado com deslocamento X/Y, escala X/Y, rotação, inclinação, âncora, opções visuais e botões compatíveis; refinamento visual validado contra a captura |
| Transform Again | Object > Transform Again (Ctrl+D) | menu_object | CONFIRMADO | desabilitado |
| Group/Ungroup | Object | menu_object | CONFIRMADO | desabilitado |
| Lock/Unlock All/Hide/Show All | Object | menu_object | CONFIRMADO | desabilitado |
| Arrange | Object > Arrange ▸ | menu_object | CONFIRMADO (menu) / conteúdo NÃO CONFIRMADO | placeholder |
| Path/Compound/Clipping Mask/Opacity Mask/Image/Text/Placed Art | Object ▸ | menu_object | CONFIRMADO (menus) / conteúdo NÃO CONFIRMADO | placeholder |
| Shaper…/Spread-Choke…/Rasterize…/Expand/Clean/Clip/Fit | Object | menu_object | CONFIRMADO | desabilitado |
| Movimento por Ctrl+setas | — | nenhuma | NÃO EXISTE | removido |
| Inspector de propriedades | View > Open Next/Previous Inspector (Ctrl+I / Ctrl+Shift+I) | menu_view | FORTE INDÍCIO (recurso) / conteúdo NÃO CONFIRMADO | não implementado; painel "Object" removido |

## Seleção

| Recurso | Local | Evidência | Classificação | ArtPlus |
|---|---|---|---|---|
| Select All (Ctrl+A) | Select | menu_select | CONFIRMADO | desabilitado |
| Deselect All (Ctrl+Shift+A) | Select | menu_select | CONFIRMADO | implementado |
| Select Inverse (Ctrl+Alt+A) | Select | menu_select | CONFIRMADO | desabilitado |
| Select Next Object (Ctrl+Tab) | Select | menu_select | CONFIRMADO | implementado |
| Select Previous Object (Ctrl+Shift+Tab) | Select | menu_select | CONFIRMADO | implementado |
| Select… (Ctrl+U) / Same Fill / Same Stroke | Select | menu_select | CONFIRMADO | desabilitado |
| Select by Paths (Outline Mode) | Select | menu_select | CONFIRMADO | desabilitado |
| Clique no canvas para selecionar | workspace | artpro24_full | CONFIRMADO | implementado (objeto mais acima) |

## Visualização

| Recurso | Local | Evidência | Classificação | ArtPlus |
|---|---|---|---|---|
| Fit in Window / Previous (Ctrl+0) | View > Zoom | submenu_zoom | CONFIRMADO | implementado |
| Actual Size / Previous (Ctrl+1) | View > Zoom | submenu_zoom | CONFIRMADO | implementado |
| Zoom In / Zoom Out (Ctrl++ / Ctrl+-) | View > Zoom | submenu_zoom | CONFIRMADO | implementado |
| Zoom to Selection / Previous (Ctrl+Alt+0) | View > Zoom | submenu_zoom | CONFIRMADO (comando); comportamento FORTE INDÍCIO | implementado: zooma para os bounds do objeto selecionado e centraliza |
| Hide Page Boxes (Ctrl+B) | View | menu_view | CONFIRMADO | implementado (toggle) |
| Show Crosshair (Ctrl+R) | View | menu_view | CONFIRMADO | implementado |
| Switch to Outline (Ctrl+Y) | View | menu_view | CONFIRMADO | desabilitado |
| Color Managed Preview (Ctrl+Alt+Y) | View | menu_view | CONFIRMADO | desabilitado |
| Rotate View ▸ | View | menu_view | CONFIRMADO (menu) / conteúdo NÃO CONFIRMADO | placeholder (item inventado removido) |
| Go To ▸ | View | menu_view | CONFIRMADO (menu) / conteúdo NÃO CONFIRMADO | placeholder |

## Evidência de engenharia reversa v24 (FORTE INDÍCIO)

- `DocumentTrap.dll`: exports `AddTrap`, `AutomaticTrap`, `CanTrap`, `RemoveTrap`,
  `InitializeSession`, `SessionParameters`, `Trapping`/`ReverseTrapping`,
  `AddBleed` → trapping é módulo de documento com sessão e candidatos.
- `StepAndRepeat.dll`: `SheetIterator`, `SNR_GridParameters`, `InputRunList`,
  `SheetSide`, `Jdf2Iterator`/`MIS2Iterator`/`Template2Iterator`,
  `Iterator2Pdf` → Step & Repeat dirigido por run list/grid, não por "N cópias".
- `DocumentMarks.dll`: marcas tipadas (`BarcodeMark`, `CornerMark`, `CutMark`,
  `DynamicMark`, `BearerBar`, `CustomMark`) e `TaskMarkSets`.
- `PDFAccessLayer.dll`: mensagens `GADocDidSave`, `GADocDidChangePages`,
  `GADocInsertPages`, `GADocDeletePages` → modelo de documento por eventos.
- `CCurve.dll`: `CCurve_TToneRangeCorrectionParameters` — **curvas tonais**, não
  geometria Bézier (não inferir pelo nome).
- `PDFObjectActions.dll`: 16.751 exports (`GOV*` ações, `GSet<GOOCGroup>`,
  `GOVAddPages`), importada por PreflightProfileLayer, ausente do import table
  do exe (carga dinâmica).

## Prepress

Todos os itens do menu Prepress são CONFIRMADOS na captura
(`artpro24_menu_prepress2.png`) e estão desabilitados no ArtPlus:
Add Barcode, Convert to Barcode, Add Dimensions, Add Bleed…, White Underprint…,
Rich Black…, Ink Eater Areas ▸, Screening…, Ink Coverage…, Convert Separations
With ▸, Thin Parts ▸, Update Trap Layer, Trapping…, Warp ▸, Preflight…,
Manage Preflight Profiles…, Variable Data ▸, Step & Repeat ▸,
Create Seamless Repetition…, Flexo Tools ▸.

## Painéis

| Painel | Local | Evidência | Classificação | ArtPlus |
|---|---|---|---|---|
| Messages | esquerda | artpro24_full | CONFIRMADO | implementado |
| Document (Crosshair, Page Boxes) | direita | artpro24_full | CONFIRMADO | implementado |
| Layers | direita | artpro24_full | CONFIRMADO | implementado (camada sintética; OCG indisponível no PDFium) |
| Separations | direita | artpro24_full | CONFIRMADO | implementado |
| Quality Control | direita | artpro24_full | CONFIRMADO | implementado |
| Object Browser / Colors / Images / Placed Art / Action Lists / Share & Approve | barra/tabs | artpro24_full | CONFIRMADO | botões presentes; conteúdo não implementado |
| Painel "Object" | — | nenhuma | INVENTADO | removido |
| Barra de status com página/zoom/coordenadas | — | não capturada | NÃO CONFIRMADO | mantida como diagnóstico técnico |

## Salvamento

- Formato nativo do ArtPro+ (documento) não existe no ArtPlus; Save/Save As
  gravam PDF com verificação por reabertura (ADR-0005).
- "Save As Copy" (byte-a-byte) não é comando do ArtPro+: permanece apenas como
  API/teste interno, fora da interface.

## Pendências de confirmação

1. Conteúdo do Inspector de propriedades (campos e layout).
2. Submenus Rotate View, Pages, Go To, Action Lists, Export To, Arrange,
   Path/Compound/Masks/Image/Text/Placed Art, Clean/Clip/Fit.
3. Menus Window e Help.
4. Barra de status e controles de página/zoom.
5. Diálogo Transform (campos e comportamento).
6. Comportamento de seleção direta e alças de transformação.
7. Versão v25: capturar a UI e comparar com a v24 (`V24_V25_COMPARISON.md`).

## Technical verification — 2026-11-03

Flip horizontal/vertical em Object > Transform: **CONFIRMADO** pela captura v24
fornecida e identificação do usuário. Espelhamento reversível por eixo; comparação
operacional das combinações avançadas permanece pendente.

Multipage regression harness only: no ArtPro+ product feature or visible element added. Generated fixture validates untouched-page dimensions, object count, appearance and source hash after the existing transform/save workflow. This is technical test infrastructure, not evidence of new v24/v25 behavior.

## CTest integration — 2026-11-03
Technical test infrastructure only: six existing suites registered; no product capability added or reclassified. ArtPro+ v24 remains the operational reference. Runtime validation blocked by process initialization error 0xc0000142, not evidence of new ArtPro+ behavior.


## CTest PATH correction — 2026-11-03
Technical infrastructure only: corrected Windows PATH inheritance and added a CMake-only regression probe. No ArtPro+ feature, visible element, behavior or classification added. v24 primary / v25 supplementary unchanged. Product test execution remains blocked by 0xc0000142.


## Runtime loader diagnostic - 2026-11-03
Technical infrastructure only: isolated static-CRT DLL loader and four control
checks; no product/UI additions or changed classifications. ArtPro+ v24 remains
primary, v25 supplementary. USER32/GDI32 fail isolated initialization (Win32 1114)
in the current execution context; product checks remain blocked before startup.


## 2026-11-03 - Generated transparency/clipping regression
Engineering-only change in tests/fixture_builder.h and tests/edit_test.cpp: controlled
alpha blend and clip samples, unchanged region and whole-page save/reopen comparisons,
object count and source hash. No UI or v24/v25 evidence/classification changes.
MSVC compile/link passed; runtime assertions NOT VERIFIED: current CTest has three
startup failures (0xc0000142) and three dependency skips, zero product checks.
General transparency/clipping/mask preservation remains unverified. See
.codex-supervisor/PROGRESS.md for controls, blocker and next concrete task.

## 2026-11-03 - CTest contract validation (engineering only)
Added tests/check_ctest_contract.cmake and tests/verify_ctest_contract.cmake to
verify six unique suites, exact commands/fixture inputs, 60-second timeouts,
build-local working directories and fixture dependencies. Real discovery, 11
negative controls and three filtered fixture selections passed with cached
Kitware CMake 4.4.4. Existing PATH and loader controls passed. Full product CTest:
three startup failures (0xc0000142), three dependency skips, zero product checks.
USER32/GDI32 isolated loads still fail with Win32 1114. No C++ build needed,
product/UI changes or ArtPro+ evidence/classification changes. v24 remains primary;
v25 supplementary. Next: separate generated soft-mask preservation regression;
runtime verification remains blocked. Details: .codex-supervisor/PROGRESS.md.

## 2026-11-03 - Soft-mask regression (technical only)
Generated image /SMask fixture and edit/save pixel assertions; no visible element,
feature classification or v24 evidence changed. Runtime validation blocked by
USER32/GDI32 Win32 1114; preservation is not claimed. See PROGRESS.md.

## 2026-11-03 - Technical fixture validation
Added tests/verify_pdf_fixtures.py (read-only, standard-library structural verifier).
Engineering infrastructure only; no product functionality or visible element added,
no evidence classifications changed. ArtPro+ v24 primary; v25 supplementary.


## 2026-11-03 - Technical resource-verifier coverage
Engineering-only fixture validation: page resource bindings/types and alpha
values checked; 56 corruption controls pass. No visible element, product
feature, classification or v24 evidence changed; v25 supplementary.


## 2026-11-03 - Technical stream-boundary validation
Fixture verifier now enforces exact generated text/binary stream framing.
Four existing fixtures and 112 corruption controls pass. Engineering only:
no visible element, product functionality or evidence classification changed.
ArtPro+ v24 primary; v25 supplementary. Runtime preservation remains unverified.


## 2026-11-03 - Stream-length declaration corruption controls (technical)
Extended tests/verify_pdf_fixtures.py with missing/duplicate /Length keys and
negative, signed, decimal, string, indirect and absent values for all 14 streams.
In-memory mutations rebase xref offsets/startxref; 14 valid leading-zero length
controls verify rebasing without changing payloads. Four existing fixtures and
224 corruption controls pass; disabling the uniqueness guard in memory makes the
duplicate control fail as intended. No PDF inputs modified, C++ build required,
UI/product/Esko changes or evidence classifications changed. v24 remains primary,
v25 supplementary. Structural checks do not prove rendering/preservation.
Current full CTest: three startup failures (0xc0000142), three dependency skips,
zero product checks executed; USER32 diagnostic still reports Win32 1114.


## 2026-11-03 - Technical PDF envelope validation
Header/trailer/xref corruption coverage in the portable fixture verifier only.
Four fixtures and 300 negative controls pass; header guard mutation detected.
No visible element, product behavior or evidence classification changed.
v24 primary; v25 supplementary. Product runtime blocked by 0xc0000142;
structural checks do not establish visual preservation.
## 2026-11-03 — Runtime validation update (technical)

The Windows GUI runtime initialized successfully in the current execution
context. The registered ArtPlus regression suite passed 6/6 tests; this updates
execution evidence only and does not add or reclassify ArtPro+ features.
