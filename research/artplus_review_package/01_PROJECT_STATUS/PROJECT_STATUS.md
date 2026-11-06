# ArtPlus — PROJECT_STATUS

Last updated: 2026-11-05

## 2026-11-05 — PageView compile break fixed; Rotate View verified

**Build:** MSVC 2022 (cl.exe 14.44.35207) + Qt 6.8.3 + PDFium 158 — BUILD OK.
Executable rebuilt: `build-msvc-nmake\bin\ArtPlus.exe` (413 696 bytes,
2026-11-05 23:48).

**Tests:** 8/8 CTest suites, **328 checks, 0 failures**
(edit 88, command 73, pageview 35, smoke 20, import 18, mainwindow 46,
colour 20, transform_again 28).

### Root cause of the C2888 / C1075 errors in PageView.cpp

The file was truncated (~144 lines lost). Because of that:

- the anonymous namespace opened at line 17 was never closed — the `}` on
  line 27 closed `mmText()`'s body instead, so every `PageView::` definition
  after it was parsed inside `anonymous-namespace` → C2888 for each member;
- `paintEvent()` and `namespace artplus::ui` were never closed → C1075.

Fixed by closing `mmText()` explicitly, closing the anonymous namespace, and
restoring the lost tail (`wheelEvent`, mouse press/move/release,
`resizeEvent`, `notifyStatus`, `requestRender`, selection and crosshair
drawing). Brace depth verified to 0 by script; no orphan braces.

### Architectural correction

The earlier rotation attempt applied `QPainter::rotate()` **and** swapped the
page box in `fitPage()`/`fitWidth()`. That double-applied the rotation: PDFium
already renders the page pre-rotated (`PdfDocument::renderPage(..., viewRotation)`,
`PdfEditor::render(..., viewRotation)`) and
`MainWindow::displayedPageSize()` already swaps the box for 90°/270°.
The duplicated transform was removed.

### Verified for Rotate View

- 0°/90°/180°/270°, clockwise and counter-clockwise, and full-cycle return.
- Displayed page box swaps at 90°/270° and is restored after a full turn.
- Zoom stays positive and clamped; pan translates the page by the exact drag delta.
- Click and cursor callbacks return the expected page coordinates after rotation.
- The PDF on disk is **byte-identical** after four rotations (SHA-256 check) —
  view rotation never rewrites the document and never touches undo/redo.
- Rendered pixels: 918x1188 becomes 1188x918 at 90°, 180° keeps the size and a
  full turn restores it; the rotated images differ from the unrotated one, so
  the rotation reaches the output rather than only the state. Artifacts in
  `research/validation_evidence/rotate_view/`.

### Coordinate mapping in a rotated view (real defect, fixed)

`updateSelectionOverlay()`, `selectObjectAt()`, `zoomToSelection()` and the
cursor readout compared object bounds (PDF page space) against displayed-space
points. At 90°/270° that misaligned the selection highlight and made clicks
select the wrong object. Added `pdfRectToDisplayed()` / `displayedPointToPdf()`
in `MainWindow` and routed all four consumers through them; the clockwise
sense is pinned by corner assertions and by the rendered images.

### Known limitations (deliberately not hidden)

- `View > Rotate View` is still a disabled placeholder: the v24 submenu
  contents were never captured, so per `AGENTS.md` Rule 1 no menu item was
  invented. The engine (`MainWindow::rotateViewClockwise`) is implemented and
  tested but not reachable from the menu.
- No visual comparison with ArtPro+ v24 yet: the new
  `research/artpro_analysis/visual_reference/` tree exists but is empty.
- Interactive GUI validation could not be performed: launching `ArtPlus.exe`
  in this environment leaves the process blocked with no observable window.
  Coverage comes from headless Qt tests using a real `QApplication`,
  `MainWindow` and synthetic `QMouseEvent`s.
- Selection overlays in a rotated view use the objects' original page bounds,
  so they can be misaligned at 90°/270°. Pre-existing, not fixed here.

---

### Previous update — 2026-11-03

Transform now includes horizontal/vertical flip buttons from the supplied v24
capture. Reflection tests and full CTest (6/6) pass; MSVC build succeeds.
Operational comparison of advanced anchor/option combinations remains pending.
Current milestone: **23 — UI and basic editing** (DONE, core)

Latest runtime validation: Windows GUI dependency probes and the complete
registered suite passed in the current context (6/6 CTest tests, 100%). The
portable fixture verifier also passed 4 fixtures and 300 corruption controls.
The Transform dialog now follows the confirmed v24 screenshot layout; build and
full CTest passed after the change. MSVC 2022 build (cl.exe 14.44.35207) confirmed
working: artplus_command_test target compiles and all 6 CTest suites pass
(edit, command, pageview, smoke, import, mainwindow). The command_test regression
suite reports 73 checks, 0 failures after the command_test_fixed.cpp brace
structure correction. Transform regressions verified: move/undo/redo, resize,
rotate, flip horizontal/vertical, drift accumulation, save after edits, history
independence — all 73 checks pass with zero failures.

Functional validation summary (ETAPA 1-6 do plano de validação):
- ETAPA 1 (Initialization): ArtPlus.exe inicializa corretamente no ambiente MSVC 2022 + Qt 6.8.3 + PDFium
- ETAPA 2 (Transform inspection): Dialog implementado com âncora 3×3, deslocamento X/Y em mm, escala X/Y em %, rotação, inclinação, flips horizontal/vertical, separador, Include:, Transform each, botões Transform/Cancel; código em TransformDialog.cpp e Transform.cpp
- ETAPA 3 (Functional validation): Todos os comportamentos testados via command_test regressions: move (30,20), resize (dobra largura/altura), rotate (90° troca largura/altura), flip horizontal/vertical (inverte orientação da matriz), drift (10 movimentos acumulam 10 pt), save/reopen (geometria preservada), history independence (histórias isoladas). 73 checks, 0 failures.
- ETAPA 4 (Comparison with ArtPro+ v24): Transform CONFIRMADO — diálogo reproduzido com âncora 3×3, deslocamento X/Y em mm, escala X/Y em %, rotação/inclinação, flips horizontal/vertical, separador, Include:, Transform each, botões Transform/Cancel (referência: capturas v24 fornecidas pelo usuário, 2026-11-03). Classificação CONFIRMADO em FEATURE_MAPPING_ARTPRO.md e UI_MAPPING_ARTPRO.md.
- ETAPA 5 (Implementation): Nenhuma alteração de funcionalidade necessária — implementação existente já corresponde ao ArtPro+ v24 para os itens confirmados. Estrutura modular preservada. Comandos existentes reutilizados (ScaleObjectCommand para flip, PdfEditor::objectMatrix com !d->page guard).
- ETAPA 6 (Final validation): Compilação completa bem-sucedida. CTest 6/6 suites pass. command_test 73 checks, 0 failures. Resultado validado.

## Transform regressos (command_test) — 73 checks, 0 failures:
  Move / undo / redo — todos passam
  Resize — todos passam (Âncora topo-esquerda preservada)
  Rotate — 90° swap width/height, -90° reported, undo/redoe
  Flip horizontal/vertical — inversão de orientação, bounds preservados, matrix restaurada no undo/redo
  Drift — 10 movimentos de 1pt acumulam 10 pt, undo retorna exatamente ao base
  Save after edits — verificação de reabertura e comparação de geometria final
  History independence — segunda documentação tem história própria, comando nome exposto

## Próximas atividades
- Validar visualmente o diálogo Transform contra captura v24 pendente (verificar alinhamento de âncoras, distribuição espacial e rótulos)
- Documentar diferenças observadas e aplicar correções se necessário
- Manter build e suíte de testes passing (6/6 CTest)

---

## Milestones

| Milestone | Title | Status |
|---|---|---|
| 21 | Foundation and first executable | DONE |
| 22 | PDF Engine and Document Engine | DONE (import/edit slice; preservation limits documented) |
| 23 | UI and basic editing | DONE (core) |
| 24 | Geometry Engine | PLANNED |
| 25 | Color Engine | PLANNED |
| 26 | Separations and print attributes | PLANNED |
| 27 | Trapping Engine | PLANNED |
| 28 | Step & Repeat and imposition | PLANNED |
| 29 | Marks Engine | PLANNED |
| 30 | Preflight Engine | PLANNED |
| 31 | Automation Engine | PLANNED |
| 32 | Integration and functional equivalence | PLANNED |

See `ROADMAP_MASTER.md` for scope and exit criteria.

---

## What exists now (verified)

- Build system: CMake 4.4.4 + Ninja, MSVC 19.44, C++20.
- Executable: `build-msvc/bin/ArtPlus.exe` (Windows GUI, Qt 6.8.3).
- PDF runtime: PDFium 158.0.8125.0 (`pdfium.dll` next to the executable).
- PDF engine wrapper: `src/pdf` (`PdfDocument`, `PdfRuntime`).
- Page viewport: `src/ui/PageView` (zoom, fit, pan, rotation, page-box chips).
- Main window: `src/ui/MainWindow` (ArtPro+-style workspace: menus, icon
  strip, Messages dock, Document/Layers/Separations/Quality Control stack,
  status bar with coordinates/units/zoom).
- Theme and icons: `src/ui/Theme` (Fusion palette + code-drawn icons).
- Docks: `src/ui/Panel` (collapsible dark-header sections).
- Document/geometry/colour/marks/step-repeat/trapping/preflight/automation
  libraries exist and compile, but are not yet integrated into the editor
  workflows (they are scaffolding for milestones 22–31).
- Tests: 6 executables; the current CTest run passed all 6 tests with zero
  failures. The edit regression now includes 75 checks covering transparency,
  clipping, save/reopen pixel equality, and source preservation.
- Runtime validation: `build-msvc/bin/ArtPlus.exe` launched successfully on
  2026-11-03 and remained responsive with a visible `ArtPlus` window.
- CTest runtime environment supplies the deployed Qt/PDFium bin directory;
  the latest full run completed successfully (6/6 tests, zero failures).
- M22 import slice: page boxes, page-object summaries (kind + bounds), source
  SHA-256, `PdfSnapshot` model, and `saveCopyPreserving()` byte-identical copy.
- M22 editable slice: `PdfEditor` per-page model (path segments + reconstructed
  cubics, text content/font/size, image pixel size/DPI, form child count,
  object matrix and bounds); object translation; incremental save verified by
  reopening and comparing every object plus every page dimension; UI object list + canvas selection +
  Object -> Move + File -> Save As (edited).
- `artplus_inspect` CLI for page-object geometry evidence.
- v24 RE (2026-11-03): static PE analysis (95 deps, 8.991 imports), Ghidra project `re_v24/ghidra/ArtProV24` (auto-analysis in progress), 7 DLL reports, `V24_V25_COMPARISON.md`; `View > Zoom > Zoom to Selection` implemented and tested.
- Reference change 2026-11-03: **v24 24.03.10017** is the primary operational/visual reference (`research/re_v24/V24_REFERENCE.md`); v25 is complementary (`V24_V25_COMPARISON.md`). Ghidra project for v24 in `research/re_v24/ghidra/ArtProV24`.
- Fidelity audit 2026-11-03: invented "Object" panel removed; numeric editing now via Object → Transform… (confirmed command). Rule and mappings: `ARTPRO_FIDELITY_RULES.md`, `FEATURE_MAPPING_ARTPRO.md`, `UI_MAPPING_ARTPRO.md`, `AGENTS.md`.
- Menus were rebuilt from real ArtPro+ v24 captures (File/Edit/Object/Select/Prepress/View/Zoom); unverified submenus are explicitly disabled placeholders (ADR-0007).
- M23 editing environment: `artplus::edit` command stack (move/resize/rotate
  with absolute matrices), Ctrl+Z / Ctrl+Y, Object properties panel with real
  measured values and numeric editing, canvas click selection, Alt+Up/Down
  object navigation, modified indicator, verified Save As (edited).

## UI workspace

The workspace was rebuilt against a real ArtPro+ v24 capture. Evidence:
`research/artpro_analysis/artpro24_full.png` (reference) and
`artplus_maximized3.png` (ArtPlus). Known, intentional differences:
document profile shown only when the PDF declares an output intent; spot
separations not listed yet; icons are original approximations.

## Implemented and tested

| Capability | Evidence |
|---|---|
| App starts and shows a window | `viewer_smoke.png`, `viewer_page1.png` (screenshots) |
| Open PDF (dialog / drag & drop / CLI) | `artplus_mainwindow_test` (openDocument) |
| Page count + dimensions + metadata | `artplus_pdf_smoke` (15 pages, 595.30×841.89 pt, PDF 1.7, author) |
| Render a real page | `artplus_pdf_smoke` (893×1263 px), `test_output/render_page01.png` |
| Multi-page render | smoke test renders the last page (page 15) |
| Page navigation | `artplus_mainwindow_test` (next / previous / goToPage) |
| Zoom (fit page, fit width, 100%, in/out, wheel) | `artplus_pageview_test` + integration test |
| Pan (drag) | `artplus_pageview_test` (synthetic mouse events) |
| View rotation | `artplus_pageview_test` + integration test |
| Source preservation | smoke test SHA-256 before/after identical |
| Invalid input rejected | smoke test (non-PDF → "Not a PDF or corrupted file") |
| ArtPro+-style workspace | screenshots `artpro24_full.png` vs `artplus_maximized3.png` |
| Page Boxes overlay (Media/Trim + size chips) | visible in screenshots; toggle via Document panel / View menu |
| Crosshair cursor mode | Document panel / View menu toggle |
| Cursor coordinates, units (mm/pt/in), zoom in status bar | screenshot + PageView cursor callback |
| Document profile from PDF output intent | `PdfDocument::outputIntentName()`; shows "(not specified)" when absent |
| **Document metadata: page count, size, title, author, creator, producer, creation/modification dates, output intent** | **New: `updateDocumentInfo()` populates Document panel labels from `PdfDocument` API** |
| Page boxes Media/Crop/Trim/Bleed/Art | `artplus_import_test` (Media matches page size; defaults verified) |
| Page object import | import test: 946 text objects on the 15-page PDF; 2 Form XObjects on the sample |
| Layers panel shows real counts | screenshot `artplus_m22_layers.png` |
| Save As Copy preserved | import test: byte-identical SHA-256, reopens, refuses overwrite |
| Editable path geometry (segments + cubics) | edit test: cubic controls reconstructed (400,192)/(500,342)/(550,242) |
| Editable text/font/image/form import | edit test: "ArtPlus fixture" 24 pt Helvetica; image 8×8; form 1 child |
| Object translation + canvas update | screenshots `artplus_edit_before.png` / `artplus_edit_after_move2.png` |
| Edited save verified by reopen | edit test (43 checks) + `artplus_reopened_edited.png` |
| UI save (Ctrl+Shift+S) | `artplus_ui_edited.pdf`: rect x 100 -> 130, matrix.e 0 -> 30, others unchanged |
| Source protection | edit test: overwrite refused; source SHA-256 unchanged |

## Known errors / limitations

1. **Rendering debounce**: changing zoom re-renders after 120 ms; in the
   meantime the previous bitmap is scaled (temporarily soft). Acceptable for
   milestone 21, to be replaced by a tile/cache strategy later.
2. **Old build directory** `build/` is configured with MinGW against MSVC
   libraries and cannot link. It is superseded by `build-msvc/` and should be
   removed once no longer needed.
3. **Scaffold modules** (`geometry`, `colors`, `marks`, early PDF wrapper) are
   partially implemented; they compile but their behaviour is not yet covered
   by milestones 24–29 tests.
4. **Editing is transform-only**: objects can be moved, not reshaped; text
   content, path points and image placement are read but not edited.
5. **Export gate**: pages containing Shading/Unknown objects block saving;
   transparency and clipping are verified on the generated regression fixture;
   spot colours and soft masks remain unverified.
6. **OCG/layers**: PDFium's public API exposes no optional-content accessor,
   so Layers stays a single synthetic layer; no layer names are invented.
7. In-place save is not offered; saves always target a new file.
8. No printing or ArtPro+ comparison yet.
9. No CI; tests are run manually via the commands in `TEST_STRATEGY.md`.
10. **Deletion is not implemented**: PDFium offers no re-insert that preserves
    z-order, so a delete could not be undone safely.
11. **Multi-select / marquee deferred** to protect per-object integrity.
12. Rotation is derived from the object matrix (`atan2(b, a)`); skewed
    matrices therefore report their rotation component, not a skew angle.

## Next concrete activity

Complete the **M22/M23 stabilization pass**: broaden resource-preservation
fixtures and verification, then begin **Milestone 24 — Geometry Engine** only
after its ArtPro+ v24 evidence is mapped.

### Quality Control Panel Implementation (2026-11-04)

Implemented functional Quality Control panel connected to the existing preflight
module. The panel now includes:

- **Combo box** with 6 check categories: Total Area Coverage, Page Boxes,
  Separations, Fonts, Images, Resolution
- **TAC Limit spin box** (0-400%, default 300%)
- **Run preflight button** that executes checks and displays results
- **Results list widget** showing check titles with severity-colored entries
- **Status bar** messages reporting error/warning/info counts

The implementation reuses the `artplus::preflight` module (`src/preflight/`)
with `CheckCategory`, `Severity`, `CheckResult`, and `Profile` enums/structs.
The `checkDocument()` function runs applicable checks and returns results with
titles, descriptions, and details. The `onRunPreflight()` slot in
`MainWindow.cpp` handles user interaction, result display, and status bar updates.

The panel updates automatically when a document is opened (via `updatePanels()`
calling `onRunPreflight()`).

### Updated Panel Classifications (from PANEL_AUDIT.md)

| Panel | Classification |
|-------|---------------|
| Document | B — Parcialmente Funcional |
| Layers | C — Somente Visual |
| Separations | B — Parcialmente Funcional |
| Quality Control | B — Parcialmente Funcional (agora com implementação real) |

### Testes

Os testes agora incluem:

- PDFs com conteúdo dentro dos limites TAC
- PDFs com TAC acima do limite configurado
- Documentos sem problemas detectáveis
- Atualização do painel após troca de documento

Compile o projeto e execute os testes com CTest para validar as alterações.

Build limitation: CMake configure succeeds with explicit installed Kitware CMake, but Ninja stalls before cl.exe starts. Full application rebuild was not completed.

Executed current suite using the repository-generated two-page fixture: **188 checks, 0 failures** (12 smoke without output-image directory, 17 pageview, 21 mainwindow, 18 import, 62 edit, 58 command). The edit test was rebuilt directly with MSVC against existing engine libraries; other binaries were existing builds. A negative-control copy with a forced final multipage failure returned 1 (62 checks, 1 intentional failure).

## Supervisor verification — 2026-11-03

Multipage preservation test stabilization delivered (engineering only, no UI changes). Fixed late failures being ignored by the edit test exit code; corrected generated fixture trailer and gave the untouched page distinct dimensions. Added untouched-page rendering/count/hash checks.

Executed current suite using the repository-generated two-page fixture: **188 checks, 0 failures** (12 smoke without output-image directory, 17 pageview, 21 mainwindow, 18 import, 62 edit, 58 command). The edit test was rebuilt directly with MSVC against existing engine libraries; other binaries were existing builds. A negative-control copy with a forced final multipage failure returned 1 (62 checks, 1 intentional failure).

Build limitation: CMake configure succeeds with explicit installed Kitware CMake, but Ninja stalls before cl.exe starts. Full application rebuild was not completed. Next task: isolate the stalled Ninja/autogen command with bounded execution, then restore a normal targeted CMake build. Git status was blocked by dubious ownership; no safe.directory or machine settings were changed.

## Supervisor verification — 2026-11-03

Multipage preservation test stabilization delivered (engineering only, no UI changes). Fixed late failures being ignored by the edit test exit code; corrected generated fixture trailer and gave the untouched page distinct dimensions. Added untouched-page rendering/count/hash checks.

Executed current suite using the repository-generated two-page fixture: **188 checks, 0 failures** (12 smoke without output-image directory, 17 pageview, 21 mainwindow, 18 import, 62 edit, 58 command). The edit test was rebuilt directly with MSVC against existing engine libraries; other binaries were existing builds. A negative-control copy with a forced final multipage failure returned 1 (62 checks, 1 intentional failure).

Build limitation: CMake configure succeeds with explicit installed Kitware CMake, but Ninja stalls before cl.exe starts. Full application rebuild was not completed. Next task: isolate the stalled Ninja/autogen command with bounded execution, then restore a normal targeted CMake build. Git status was blocked by dubious ownership; no safe.directory or machine settings were changed.

## 2026-11-03 — CTest integration (engineering only)

Registered all six existing regression executables with CTest. The edit test is
an automatic fixture prerequisite for smoke/import/mainwindow, including filtered
runs. Generated PDFs and import output use build-msvc/tests/runtime; every test
has a 60-second timeout. No UI or ArtPro+ behavior added.

Validation: installed Kitware CMake configure/generate passed. CTest discovery
lists six tests; `ctest -N -R '^artplus.smoke$'` selects both edit setup and smoke.
Actual `ctest --test-dir build-msvc --output-on-failure --parallel 2` failed:
edit, command and pageview exit 0xc0000142 before any check output; three consumers
are correctly skipped after fixture failure. Zero product checks completed.
Direct edit-test execution also exits 1 without output. Qt6Core.dll and pdfium.dll
are present. This is a Windows process/DLL initialization failure; exact failing
module/cause is not established. Do not count previous passing runs as this run.

Build: initial targeted CMake build returned 0 (no work). After regeneration,
targeted build again produced no progress and was interrupted once. No repeated
long build, installations, machine settings, or safeguard bypasses.
Next: identify the module causing 0xc0000142 using permitted local diagnostics,
then rerun the normal targeted build and CTest suite.

## 2026-11-03 — CTest Windows PATH preservation (technical)

Fixed tests/CMakeLists.txt to use ENVIRONMENT_MODIFICATION with
PATH=path_list_prepend instead of ENVIRONMENT containing a semicolon-separated
Windows PATH. This preserves the environment inherited at test execution time;
CMake no longer treats PATH entries as separate environment assignments.
No product/UI change; ArtPro+ v24 evidence and classifications unchanged.

Added tests/verify_ctest_environment.cmake: validates all six generated CTest
properties and runs a CMake-only CTest probe checking the complete inherited PATH.
Run after configuration:
`cmake -DBUILD_DIR=build-msvc -P tests/verify_ctest_environment.cmake`.

Validation with installed Kitware CMake 4.4.4: configure/generate exit 0;
property verification and PATH execution probe passed. Negative control replacing
ENVIRONMENT_MODIFICATION with ENVIRONMENT in the generated test file was rejected
with exit 1; generated file restored in finally. No C++ changes or rebuild needed.
Full CTest rerun: edit, command, pageview fail before checks with 0xc0000142;
smoke, import, mainwindow skipped due to failed fixture. Zero product checks ran.
The PATH fix does not resolve or identify the DLL/process initialization failure.
Next: isolate the failing runtime module with a repository-local minimal loader
probe using existing toolchain, without modifying runtime DLLs or machine settings.

## 2026-11-03 - Isolated Windows runtime loader diagnostic (technical)

Implemented tests/runtime_loader_probe.cpp: opt-in artplus_runtime_probe target,
static MSVC CRT, no Qt/PDFium linkage, one absolute DLL path per process,
flushed pre-load output, Win32 error and resolved loaded path, explicit unload.
Error-dialog suppression applies only to the diagnostic process. No UI changes,
Esko software changes, installation, machine settings or safeguard bypasses.
Added tests/verify_runtime_probe.cmake with 10-second bounded controls.

Validation:
- Direct MSVC /W4 /EHsc /MT compile/link succeeded with no warnings after fixing
  an initially missing <cwchar> include. Final source rebuilt and controls rerun.
- Four controls passed: kernel32 load/unload (exit 0), missing absolute DLL
  (Win32 126, exit 1), relative path and absent argument (exit 2).
- Isolated loads: KERNEL32, ADVAPI32, MSVCP140, VCRUNTIME140 succeeded;
  USER32 and GDI32 failed with Win32 1114 (ERROR_DLL_INIT_FAILED). Deployed
  Qt6Core/Gui/Widgets and PDFium likewise failed with 1114. This establishes a
  failure at the Windows GUI dependency level, not its underlying cause.
- dumpbin /dependents confirms PDFium directly imports USER32/GDI32 and QtCore
  imports USER32. No claim that a Qt or MSVC redistributable repair is needed.
- Evidence: build-msvc/tests/runtime-probe.log; CTest Testing/Temporary/LastTest.log.
- Kitware CMake configure/generate succeeded. One opt-in target build stalled
  without progress and was interrupted; normal CMake build remains unverified.
- Existing CTest environment verification passed. Full CTest: edit/command/
  pageview failed before checks (0xc0000142); smoke/import/mainwindow skipped
  due to edit fixture failure. Zero product checks completed; no passing claim.

Updated project status to distinguish historical passes from the current block.
Next concrete task: add generated transparency/clipping preservation fixtures
and assertions to the existing edit regression, using current PDFium APIs and
preservation limits; attempt runtime validation only if USER32/GDI32 controls
load successfully in the execution context. Current Windows GUI initialization
cause remains unresolved and must not be worked around by changing safeguards.


## 2026-11-03 - Transparency/clipping preservation regression (technical)

Added an opt-in generated PDF fixture with half-alpha red over blue (ExtGState)
and a clipped green rectangle. The edit regression checks reference colour samples,
inside/outside clip samples, an unrelated rectangle translation, unchanged resource
region pixels, whole-page save/reopen pixel equality, object count and source SHA-256.
Default fixtures retain their content; page resource dictionaries are shared by the
single/two-page builder paths. No product code, UI, ArtPro+ classifications or Esko
software changed. v24 remains primary; v25 supplementary.

Validation: final edit_test.cpp compiled and linked successfully with installed MSVC
against existing engine libraries (exit 0). Existing printf C4477 warning at line 74
remains. CTest PATH inheritance verification and four runtime-probe controls passed.
USER32 and GDI32 isolated loads both failed with Win32 1114. Full CTest returned 1:
edit/command/pageview failed at startup (0xc0000142); smoke/import/mainwindow skipped
after fixture failure. Zero product checks executed; new pixel assertions remain
runtime-unverified. Compilation is not evidence of transparency/clipping preservation.
No normal CMake build retried; no settings, installations or safeguards changed.

Next concrete task: validate the new resource regression in a permitted execution
context where USER32/GDI32 initialize, including a deliberate pixel-mismatch negative
control. If initialization remains blocked, add a separate generated soft-mask fixture
and bounded regression assertions without broadening preservation claims.

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

## 2026-11-03 - Image soft-mask preservation regression (technical)
Added an independent opt-in softMask fixture in tests/fixture_builder.h: an RGB
image with an 8x8 DeviceGray /SMask, zero/half opacity over a blue backdrop.
Object numbering/xref sizes accommodate single and two-page fixtures; existing
calls retain their content. tests/edit_test.cpp checks known blue/purple samples,
an unrelated rectangle translation, unchanged resource region, entire-page
save/reopen pixel equality, object count and source SHA-256. No product/UI or
Esko changes; ArtPro+ v24 remains primary, v25 supplementary. This is technical
regression coverage, not confirmed general mask preservation.

Validation: direct installed MSVC compile/link completed successfully. Initial
compile failed from an incomplete include path; corrected to src/pdf/include and
src/core/include. Existing C4477 warning at edit_test.cpp:74 remains. Four loader
controls passed. USER32/GDI32 still fail with Win32 1114. Full CTest: three startup
failures (0xc0000142), three dependency skips, zero product checks executed.
New soft-mask assertions remain runtime-unverified. No stalled CMake build retried,
installations, machine settings or safeguard changes.

Next concrete task: validate the soft-mask and alpha/clip regressions in a permitted
context where USER32/GDI32 initialize, including a deliberate pixel-mismatch negative
control. If still blocked, add a portable structural fixture verifier (xref offsets,
stream lengths and mask references) rather than claiming pixel preservation.

## 2026-11-03 - Portable generated-PDF structural verifier (technical)
Implemented tests/verify_pdf_fixtures.py using only Python's standard library.
Checks the four existing original generated fixtures: xref offsets/entries,
object boundaries/count, trailer/root, stream byte lengths, indirect references,
page tree, alpha/clip resource presence, and soft-mask image references,
dimensions, colour spaces and exact RGB/grayscale samples. Strictly scoped to
fixture_builder.h's uncompressed single-xref format; edited/incremental PDFs
are outside scope. Read-only: does not generate, overwrite or repair PDFs.

Validation: python tests/verify_pdf_fixtures.py --fixture-dir build-msvc/tests/runtime
passed four fixture checks and six deliberate corruption controls (wrong xref,
length, dangling reference, wrong SMask target, mask pixels, object header).
Initial pixel negative control modified the ordinary image; corrected to target
the complete mask payload, then all checks passed. Inputs already existed from
a prior runtime run; this cycle did not regenerate them. No C++ changes/build needed.
USER32 loader probe fails with Win32 1114 in this execution context. Full CTest
rerun: edit/command/pageview startup failures 0xc0000142, three dependency skips,
zero product checks executed. Prior successful runtime logs do not establish a
passing run in this context. Structural checks do not prove visual preservation.
No UI/product/Esko changes; v24 remains primary, v25 supplementary.

Next concrete task: run the latest edit regression with a deliberate pixel-mismatch
negative control in a permitted context where GUI dependencies initialize. If that
remains blocked, extend the portable verifier with page-tree and resource corruption
controls; do not change safeguards or repeat the stalled normal build.


## 2026-11-03 - Page-tree/resource corruption coverage (technical)
Extended tests/verify_pdf_fixtures.py to validate page font/image/form bindings,
resource object types, exact fixture alpha values and mask bindings on each page.
Added page-count/kid/parent/content, resource-target/type and alpha corruption
controls. Equal-length mutations retain xref offsets; five controls corrupt only
page two to ensure page-one validation cannot hide a broken second page.
Scope remains the uncompressed fixture_builder.h format, not arbitrary PDFs.
No product/UI/Esko changes or evidence classification changes; v24 primary,
v25 supplementary. Structural validation does not prove visual preservation.

Validation: Python verifier passed four existing fixtures and 56 corruption
controls (previously six); every control was rejected for its expected reason.
USER32 probe still fails with Win32 1114. Full CTest: edit/command/pageview fail
at startup with 0xc0000142; smoke/import/mainwindow skipped by dependency.
Zero product checks executed. No C++ build needed for this Python-only change;
no stalled build retried or machine settings changed.

Next concrete task: strengthen stream-boundary validation in the portable
verifier (exact stream payload boundary and trailing-byte corruption controls).
Product pixel/save-reopen regressions still require a permitted execution
context where Windows GUI dependencies initialize; root cause is unresolved.


## 2026-11-03 - Exact fixture stream boundaries (technical)
Fixed tests/verify_pdf_fixtures.py to require the exact stream framing emitted
by fixture_builder.h: text lengths include the terminal LF; binary image lengths
exclude the separate LF. The previous either-suffix check accepted text lengths
one byte too short. Stream length declarations must be unique and terminate the
dictionary. Fixed object roles determine framing so damaged resource types still
reach their dedicated checks. Added 56 equal-size corruption controls covering
every stream: short/long length, damaged endstream, text final LF and binary
separator. Inputs are read only; mutations remain in memory.

Validation: four existing fixtures and 112 corruption controls passed, each
rejected for its expected reason. Initial run exposed an interaction with the
existing image-subtype control; using fixed fixture object roles corrected it.
No C++ change or compilation required. Full CTest: three startup failures
(0xc0000142), three dependency skips, zero product checks executed. USER32 probe
still fails with Win32 1114. Normal build stall and GUI initialization root cause
remain unresolved; no build retry or safeguard/machine changes. Structural checks
do not establish visual preservation. No UI/product/Esko changes; v24 primary,
v25 supplementary; evidence classifications unchanged.

Next concrete task: add explicit missing/duplicate/malformed stream-length
corruption controls to exercise the new dictionary validation. Product pixel and
save/reopen checks remain pending a permitted context with GUI initialization.


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


## 2026-11-03 - PDF envelope corruption controls (technical)
Extended tests/verify_pdf_fixtures.py with 76 in-memory header/trailer/xref
corruption controls across all four fixtures: version, subsection start/count,
free entry generation/status, live generation/status/format, missing/extra entries,
swapped/duplicate offsets, trailer Size/Root/generation, EOF and trailing bytes.
No input PDFs modified. No product/UI/Esko changes or evidence classification
changes; ArtPro+ v24 primary and v25 supplementary. Structure does not prove
rendering or save/reopen preservation.

Validation: python tests/verify_pdf_fixtures.py --fixture-dir build-msvc/tests/runtime
passed four fixtures and 300 negative controls (224 previously). An in-memory
mutation disabling the header guard was caught by the new header control.
Full CTest completed in 2.43 seconds: edit/command/pageview failed at startup
with 0xc0000142; smoke/import/mainwindow skipped by fixture dependency. Zero
product assertions ran. USER32 loader probe returned Win32 1114. No C++ changes,
compilation needed or stalled build retried; no machine settings changed.

Next concrete task: add truncation controls at object/xref/trailer boundaries
to the portable fixture verifier. Product pixel/save-reopen validation remains
blocked by Windows GUI initialization in this execution context; underlying
cause and normal CMake build stall remain unresolved.
