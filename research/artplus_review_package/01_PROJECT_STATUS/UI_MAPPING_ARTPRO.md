# UI_MAPPING_ARTPRO — elemento por elemento

Regra: `ARTPRO_FIDELITY_RULES.md`. Cada elemento visível do ArtPlus precisa de
correspondência com o ArtPro+ e evidência registrada.

Referência visual principal: **ArtPro+ v24 24.03.10017**
(`research/re_v24/V24_REFERENCE.md`). Todas as capturas `artpro24_*` são da v24.
A v25 é referência técnica complementar (`V24_V25_COMPARISON.md`).

## Auditoria de 2026-11-03

### Elementos inventados encontrados e corrigidos

| Elemento | Onde estava | Evidência de ausência | Ação |
|---|---|---|---|
| Painel dock "Object" (Type/ID/X/Y/W/H/Rot/Bounds/State) | dock direito, abaixo de Document | nenhuma captura do ArtPro+ mostra esse painel; propriedades pertencem ao Inspector (View > Open Next Inspector), cujo conteúdo não foi capturado | **removido**; edição numérica movida para `Object → Transform…` (comando confirmado) |
| Atalho oculto Ctrl+setas (nudge 5 pt) | ação de janela | não existe atalho equivalente no ArtPro+ | **removido** |
| `Window → Reset Layout` | menu Window | conteúdo do menu Window não capturado | **removido** |
| `View → Rotate View → Rotate 90 degrees Clockwise` | submenu View | submenu existe; conteúdo não capturado | **removido**; submenu ficou como placeholder |
| `File → Save As Copy…` | menu File | não existe no ArtPro+ | já removido antes (API/teste permanece) |

### Mantidos como técnicos (declarados, não são funcionalidades)

| Elemento | Motivo |
|---|---|
| Barra de status (página, unidades, zoom, coordenadas) | necessária para diagnóstico; localização/estrutura no ArtPro+ ainda NÃO CONFIRMADA |
| `Help → About ArtPlus...` | menu Help não capturado; conteúdo mínimo de identificação |
| Mensagens de diagnóstico no painel Messages | infraestrutura de desenvolvimento |

## Estrutura atual × referência

| Elemento | ArtPro+ | Classificação | ArtPlus |
|---|---|---|---|
| Barra de menus: File, Edit, Object, Select, Prepress, View, Window, Help | artpro24_full / capturas | CONFIRMADO | mesma sequência |
| File (itens e ordem) | artpro24_menu_file.png | CONFIRMADO | replicado; não implementados desabilitados |
| Edit (itens e ordem) | artpro24_menu_edit.png | CONFIRMADO | replicado; Redo Ctrl+Shift+Z |
| Object (itens e ordem) | artpro24_menu_object.png | CONFIRMADO | replicado; Transform… funcional |
| Select (itens e ordem) | artpro24_menu_select2.png | CONFIRMADO | replicado; Next/Previous e Deselect All funcionais |
| Prepress (itens e ordem) | artpro24_menu_prepress2.png | CONFIRMADO | replicado; itens desabilitados |
| View + Zoom | artpro24_menu_view2.png / submenu_zoom | CONFIRMADO | replicado; Fit/Actual/In/Out funcionais |
| Rotate View | menu confirmado, conteúdo não | NÃO CONFIRMADO (conteúdo) | placeholder "Not implemented in ArtPlus yet" |
| Window | não capturado | NÃO CONFIRMADO | apenas toggles de painéis existentes |
| Help | não capturado | NÃO CONFIRMADO | About mínimo |
| Barra de ferramentas: Object Browser, Colors, Images, Placed Art, Messages, Action Lists, Share & Approve, Clean, Select, Recolor, Shaper, Barcode, Screening, Trapping, Wrap, 3D, Preview | artpro24_full | CONFIRMADO | mesmos grupos/rótulos; ícones próprios; ações não implementadas avisam |
| Painel Messages (esquerda) | artpro24_full | CONFIRMADO | implementado |
| Dock direito: Document, Layers, Separations, Quality Control | artpro24_full | CONFIRMADO | implementado (sem painel extra) |
| Quadros/chips Media Box / Trim Box no canvas | artpro24_full | CONFIRMADO | implementado |
| Status bar com controles | não capturado | NÃO CONFIRMADO | técnico, pendente de mapeamento |
| Inspector de propriedades | View > Open Next Inspector | FORTE INDÍCIO (recurso) | não implementado |
| Diálogo Transform | Object > Transform… | **CONFIRMADO** (captura v24 fornecida pelo usuário, 2026-11-03) | diálogo reproduzido com âncora 3×3, deslocamento X/Y em mm, escala X/Y em %, rotação/inclinação, opções de máscaras/efeitos, cópias e botões Transform/Cancel; refinamento visual validado contra a captura |

## Atalhos

Flip horizontal/vertical — **CONFIRMADO** na captura v24 fornecida pelo usuário
(`codex-clipboard-efe334bb-9a51-422f-ab6b-112d94648fb2.png`) e identificado pelo usuário.
Local: dois botões à direita dos campos de rotação/inclinação no diálogo Transform.
Implementação: botões de espelhamento por eixo; combinações com âncoras/opções avançadas ainda pendentes de comparação operacional.

| Atalho | ArtPro+ | Classificação | ArtPlus |
|---|---|---|---|
| Ctrl+O / Ctrl+S / Ctrl+Shift+S / Ctrl+W / Ctrl+Q | File | CONFIRMADO | iguais |
| Ctrl+Z / Ctrl+Shift+Z | Edit | CONFIRMADO | iguais |
| Ctrl+A / Ctrl+Shift+A / Ctrl+Alt+A | Select | CONFIRMADO | Ctrl+Shift+A funcional; demais desabilitados |
| Ctrl+Tab / Ctrl+Shift+Tab | Select Next/Previous Object | CONFIRMADO | iguais |
| Ctrl+0 / Ctrl+1 / Ctrl++ / Ctrl+- / Ctrl+Alt+0 | View > Zoom | CONFIRMADO | iguais; Ctrl+Alt+0 funcional (zoom to selection) |
| Ctrl+B / Ctrl+R | View | CONFIRMADO | iguais |
| Ctrl+Shift+M | Object > Transform… | CONFIRMADO | igual |
| Ctrl+arrows | — | NÃO EXISTE | removido |

## Pendências

1. Capturar o Inspector (selecionar objeto + Ctrl+I) e mapear campos.
2. Capturar submenus não confirmados e os menus Window/Help.
3. Validar visualmente o diálogo Transform reproduzido contra a captura v24.
4. Capturar a UI da v25 e comparar com a v24 (`V24_V25_COMPARISON.md`).
5. Decidir o destino da barra de status (manter técnica ou mapear o original).

## Technical verification — 2026-11-03

Multipage regression harness changes only; no UI element, menu, shortcut or behavior added. ArtPro+ v24 remains primary; outstanding capture requirements above are unchanged.

## CTest integration — 2026-11-03
Engineering-only change in CMake/test orchestration; no visible elements or shortcuts added. Existing UI evidence/classifications unchanged.


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
execution evidence only and does not add or reclassify ArtPro+ UI elements.
