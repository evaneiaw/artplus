# ArtPlus — CHANGELOG

All notable changes to the ArtPlus application.

## Flip buttons — 2026-11-03

- Added horizontal and vertical reflection buttons at the right of Transform,
  matching the supplied v24 screenshot. Selected flips apply with Transform.
- Verified reflection orientation, preserved bounds and exact undo/redo matrix
  restoration for both axes. MSVC build and all six CTest suites passed.
- Advanced anchor/option combinations still require operational comparison.

## [0.1.3] — 2026-11-03 — Transform dialog fidelity

### Changed

- Reworked `Object → Transform…` to match the confirmed ArtPro+ v24 layout:
  anchor grid, X/Y millimetre movement, percentage scales, rotation/skew,
  transform options, copy count and Transform/Cancel actions.
- Preserved the existing reversible move, scale and rotation command path.

### Verified

- MSVC build passed.
- Full CTest suite passed: 6/6 tests.

## [0.1.4] — 2026-11-03 — Transform visual refinement

### Changed

- Refined sizing, spacing, labels, literal ampersands, field widths and divider to match the supplied ArtPro+ v24 capture more closely.

### Verified

- MSVC build passed.
- Full CTest suite passed: 6/6 tests.

## [0.1.2] — 2026-11-03 — Runtime validation resumed

### Verified

- Windows GUI dependencies loaded successfully in the current execution context.
- Full registered CTest suite passed: 6/6 tests, 100%, 1.21 s.
- Portable PDF fixture verifier passed 4 fixtures and 300 corruption controls.

## [0.1.1] — 2026-11-03 — Regression validation

### Fixed

- Revalidated the MSVC/Qt/PDFium runtime after the transient `0xc0000142`
  startup failure. The complete CTest suite now passes again.

### Verified

- Transparency and clipping preservation regression: 75 edit checks passed,
  including save/reopen pixel equality and source preservation.
- All six registered CTest tests pass with zero failures.
- `ArtPlus.exe` launches and remains responsive with a visible main window.

## [0.1.0] — 2026-11-02 — Milestone 21: foundation and first executable

### Added

- Working Windows GUI executable (`ArtPlus.exe`) built with MSVC 19.44,
  Qt 6.8.3 (msvc2022_64) and PDFium 158.0.8125.0.
- Main window with File/View/Page/Help menus, toolbar, status bar, left
  document dock (metadata + page list) and right messages dock.
- Open a PDF via dialog, drag & drop, or command-line argument.
- Document information: page count, page dimensions, PDF version, title/author,
  encryption state.
- Real page rendering through PDFium, including multi-page documents.
- Page navigation: page list, page spinner, previous/next, Ctrl+PgUp/PgDn.
- Zoom: fit page, fit width, 100 %, zoom in/out and cursor-anchored mouse wheel;
  zoom range clamped to 0.02–64 px per point.
- Pan by left-drag; view rotation in 90° steps.
- 120 ms debounced re-render to keep wheel zoom responsive.
- Automated tests:
  - `artplus_pdf_smoke` — engine: open/count/size/metadata/render/invalid/
    preservation (14 checks).
  - `artplus_pageview_test` — viewport maths: fit/zoom/clamp/rotation/pan
    (14 checks).
  - `artplus_mainwindow_test` — GUI integration: open/navigate/zoom/rotation/
    close (21 checks).
- Project documentation: `ROADMAP_MASTER.md`, `PROJECT_STATUS.md`,
  `ARCHITECTURE.md`, `BUILD_INSTRUCTIONS.md`, `TEST_STRATEGY.md`,
  `DEPENDENCIES_AND_LICENSES.md`, `FEATURE_MATRIX.md`, `ADR/`.

### Fixed

- Build moved from a MinGW/MSVC-Qt mismatch (unlinkable) to a coherent MSVC
  toolchain; legacy `build/` superseded by `build-msvc/`.
- `artplus_marks` no longer fails to compile: corrected `geometry::` namespace
  usage, `QPainterPath` argument types, missing paint implementations, invalid
  `TaskMarkSet` serialisation helpers, and mangled function bodies.
- Missing moc output for `MainWindow`/`StatusBarPanel`: headers are now listed
  in the target sources so AUTOMOC processes them.
- `nextPage`/`previousPage` passed 0-based page numbers into the 1-based
  `goToPage`, so paging did nothing; corrected.
- Status bar message overlapped the document label; removed the duplicate
  `showMessage` path.
- Smoke test invalid-file setup used an unwritable temp location; now writes to
  the working directory and cleans up.

### Known limitations

- No editing, saving, exporting or printing.
- Rendering is not cached across zoom changes beyond the debounce.
- No behavioural comparison against ArtPro+ yet.

## [Unreleased]

### Verified

- Fixed CTest runtime environment so tests launched from the generated runtime
  directory can locate the Qt and PDFium DLLs; the complete CTest suite now
  passes without manual PATH setup.
- Re-deployed the existing Qt 6.8.3 runtime beside the MSVC build and reran the
  relevant suite: 185 checks, 0 failures (14 smoke, 17 pageview, 21 mainwindow,
  18 import, 57 edit, 58 command).
- Strengthened edited-PDF verification to compare the dimensions of every page
  after reopening, including pages untouched by the current editor session.

### Added (v24 reverse engineering + Zoom to Selection)

- Static PE analysis of ArtPro+ v24: 95 direct dependencies, 8.991 imported
  symbols, sections `.fnp_dir`/`.fnp_mar` (FlexNet), dynamic loading via
  `LoadLibrary*`/`GetProcAddress`; `PDFObjectActions` not in the exe import
  table. Reports: `research/re_v24/MAIN_EXECUTABLE_ANALYSIS.md`,
  `GHIDRA_PROJECT_AUDIT.md`, `ghidra/analysis/*.md`.
- Ghidra project `research/re_v24/ghidra/ArtProV24` created and auto-analysis
  running (function counts pending; project not restarted).
- Implemented `View > Zoom > Zoom to Selection / Previous` (Ctrl+Alt+0), a
  confirmed v24 command: `PageView::zoomToSelection()` zooms to the selected
  object bounds and centres it; enabled only with a selection.
- Tests: 3 new PageView checks (17 total in that binary), 169 overall.

### Changed (reference switched to ArtPro+ v24)

- v24 24.03.10017 is now the primary operational/visual reference
  (`research/re_v24/V24_REFERENCE.md`): path, size, SHA-256 and the
  Authenticode **HashMismatch** warning are recorded.
- v25 25.03.116 remains the complementary technical reference; the comparison
  matrix `V24_V25_COMPARISON.md` records observed differences (Qt 6.3.1 +
  WebEngine in v24 vs WebUIQt/no WebEngine in v25; 29 v24-only and 24 v25-only
  modules; size deltas in PDFAccessLayer, DocumentOCR, DocumentTypesetting,
  StepAndRepeatCAD, ARDLib).
- Ghidra headless analysis of the v24 executable started:
  project `research/re_v24/ghidra/ArtProV24`, log
  `research/re_v24/ghidra_v24_import.log`.
- `AGENTS.md`, `ARTPRO_FIDELITY_RULES.md`, `FEATURE_MAPPING_ARTPRO.md` and
  `UI_MAPPING_ARTPRO.md` updated to make v24 the reference and keep v25 as
  complementary evidence.

### Changed (fidelity audit — invented UI removed)

- Adopted the permanent fidelity rule: `ARTPRO_FIDELITY_RULES.md`, with
  evidence/classification tables in `FEATURE_MAPPING_ARTPRO.md` and
  `UI_MAPPING_ARTPRO.md`, and mandatory instructions in `AGENTS.md`.
- Removed the invented "Object" dock panel: ArtPro+ has no such panel; object
  properties belong to the Inspector (`View > Open Next Inspector`), whose
  contents are not confirmed yet.
- Added `Object > Transform…` (Ctrl+Shift+M, a confirmed ArtPro+ command) as a
  minimal dialog with X, Y, Width, Height and Rotation; edits remain reversible
  commands and are verified on save.
- Removed the hidden Ctrl+arrows nudge (invented shortcut), `Window > Reset
  Layout` (Window menu not captured) and `View > Rotate View > Rotate 90°`
  (submenu content not captured; now a disabled placeholder).
- Documented remaining unconfirmed areas instead of guessing: Inspector,
  submenus (Rotate View, Pages, Go To, Action Lists, Arrange, Path/masks, …),
  Window/Help menus, Transform dialog layout, status bar and the v250 reference.

### Changed (menus aligned to the real ArtPro+ v24 structure)

- Captured File, Edit, Object, Select, Prepress, View and the Zoom submenu from
  the installed ArtPro+ v24 and rebuilt the ArtPlus menus from that evidence.
- Object menu now has the real ArtPro+ entries (Arrange; Transform…;
  Transform Again; Group/Ungroup; Lock/Unlock All; Hide/Show All; Path,
  Compound, Clipping Mask, Opacity Mask, Image, Text, Placed Art; Remove Color
  Profile; Recolor…; Shaper…; Spread/Choke…; Expand; Rasterize…; Clean/Clip/Fit;
  Convert to Placeholder…), with unimplemented items disabled.
- Removed invented entries: object nudge from the Object menu (kept as a hidden
  Ctrl+arrows shortcut), "Save As Copy" from File (API only), Alt+Up/Down
  navigation (now Select Next/Previous Object, Ctrl+Tab / Ctrl+Shift+Tab).
- Redo shortcut is now Ctrl+Shift+Z (ArtPro+); zoom lives under View → Zoom
  (Ctrl+0 fit, Ctrl+1 actual size, Ctrl++ / Ctrl+-).
- View now uses "Hide Page Boxes" (Ctrl+B) and "Show Crosshair" (Ctrl+R as in
  the capture), and Select/Prepress/File follow the captured item order.
- Submenus not captured (Rotate View content, Pages, Go To, Action Lists,
  Export To, …) contain a disabled "Not implemented in ArtPlus yet" entry and
  are documented in ADR-0007.

### Added (Milestone 23 — editing environment, commands and history)

- New `artplus::edit` library: `Command`, `CommandStack`,
  `ObjectMatrixCommand`, `MoveObjectCommand`, `ScaleObjectCommand`,
  `RotateObjectCommand`, and `ObjectProperties`/`describe()`.
- Commands store **absolute** object matrices, so repeated undo/redo never
  drifts; verified by a ten-move test that returns exactly to the base geometry.
- `PdfEditor::objectMatrix()` / `setObjectMatrix()`: absolute matrix access that
  corrects the composition order used by `FPDFPageObj_TransformF`
  (`new = relative * current`). Page-space move now works for rotated objects.
- Undo/Redo actions with Ctrl+Z / Ctrl+Y, command names in the menu, and
  enabled state derived from the stack.
- Object properties panel: type, ID, X/Y, W/H, rotation (from the matrix),
  bounds, edit state, text content/font/size, image pixels/DPI, form children.
  X/Y/W/H/rotation are editable and become reversible commands.
- Canvas click selection (top-most object), selection rectangle, list ↔ canvas
  ↔ properties synchronisation, Alt+Up/Alt+Down navigation.
- Modified indicator in the window title (` *`); save marks the history clean;
  closing or changing page with unsaved edits asks for confirmation.
- `artplus_command_test`: 58 checks (move/undo/redo, resize anchor, rotation and
  reporting, drift, clean/dirty across save, multi-operation save + reopen,
  independent history).

### Added (Milestone 22, second slice — editable geometry and real round-trip)

- `artplus::pdf::PdfEditor`: per-page editable model with, per object, kind,
  page-space bounds, PDF transform matrix, path segments (Move/Line/Bezier,
  closed flag), reconstructed cubic curves (control points + end point),
  text content/font base name/size, image pixel size and DPI, form child count.
- `translateObject()`: moves an object with `FPDFPageObj_TransformF` followed
  by `FPDFPage_GenerateContent`; the canvas re-renders from the edited page.
- `saveAs()`: never touches the source; prefers an incremental update, then
  reopens the written file and verifies every object's kind and bounds before
  accepting the result; aborts (and deletes the temp file) if verification
  fails. Pages containing Shading/Unknown objects refuse export.
- UI: real object list in the Layers panel, dashed selection rectangle on the
  canvas, Object → Move (Ctrl+arrows), File → Save As (edited)… (Ctrl+Shift+S),
  discard-edit confirmation on page change/close.
- `artplus_edit_test`: 43 checks proving import of rectangle/Bézier/text/image/
  form, translation by (30, 20) pt, save, reopen, persisted matrix (30, -20
  PDF) and bounds (130, 212), all other objects unchanged, source untouched,
  overwrite refused.
- `artplus_inspect`: CLI that prints page-object geometry for evidence.

### Added (Milestone 22, first slice — PDF import model and preservation, 2026-11-02)

- `PdfDocument::pageBoxes()`: Media/Crop/Trim/Bleed/Art boxes in points with a
  top-left origin, applying the PDF defaults (Bleed/Trim/Art fall back to Crop).
- `PdfDocument::pageObjects()`: top-level page objects in content order with
  kind (Text, Path, Image, Shading, Form) and bounds.
- `PdfDocument::sourceSha256()` and `saveCopyPreserving()`: the source hash is
  captured on open; Save As Copy writes a byte-identical file and refuses to
  overwrite the source.
- `artplus::pdf::PdfSnapshot`: immutable import model (metadata, page boxes,
  object summaries, source hash) with per-kind counters.
- UI: Layers panel now reports real object counts
  (“N objects · X text · Y paths · Z images”); File → Save As Copy added.
- `artplus_import_test`: 18 checks on a 15-page PDF and 17 on a form-based
  single page (page count, Media Box, defaults, classification, byte-identical
  copy, reopen, overwrite refusal).

### Added (UI workspace rework, 2026-11-02)

- Workspace redesigned to match the ArtPro+ v24 layout, using a real
  reference capture (`artpro24_full.png`) taken from the installed ArtPro+:
  - menu bar `File Edit Object Select Prepress View Window Help`;
  - top icon strip: Object Browser, Colors, Images, Placed Art, Messages,
    Action Lists, Share & Approve, plus Clean/Select/Recolor/Shaper/Barcode/
    Screening/Trapping/Wrap and 3D/Preview;
  - left dock `Messages` (Other → Missing Real Data);
  - right dock stack `Document`, `Layers`, `Separations`, `Quality Control`;
  - light-grey canvas, white page with orange Media Box frame, and the
    `Media Box` / `Trim Box` / size chips above the page;
  - status bar with cursor coordinates, page, units and zoom;
  - window title `file @ zoom% – ArtPlus` kept in sync while zooming.
- `src/ui/Theme.cpp`: Fusion palette, global stylesheet and 30+ tool icons
  drawn in code (no proprietary artwork).
- `src/ui/Panel.cpp`: collapsible dark-header dock panel.
- `PdfDocument::outputIntentName()`: reads `/OutputConditionIdentifier` /
  `/OutputCondition` from the PDF so the Separations panel can show the real
  document profile when present.

### Fixed

- **Shutdown crash**: `MainWindow` (owner of the open document) was destroyed
  after `PdfRuntime::shutdown()`, so PDFium was asked to close a document
  whose library was already destroyed. Window is now scoped before shutdown,
  in both the app and the integration test.
- Maximising no longer leaves a stale zoom in the window title.
- Right dock is capped in width so its `Setup` column is not clipped when the
  window is maximised.
- Document-panel toggles no longer show a misleading “selected” highlight.

## [0.1.0] — 2026-11-02 — Milestone 21: foundation and first executable

## Supervisor stabilization — 2026-11-03

- Fixed `artplus_edit_test` returning a stale success code before its multipage checks. Exit status now reflects all checks, after runtime shutdown.
- Multipage generated fixture now uses distinct 612x792 / 700x900 pt pages and a correct trailer `/Size`. Added checks for visible untouched content, pixel-identical untouched-page rendering, object count and source SHA-256 preservation.
- Direct MSVC build passed against existing engine libraries; six test executables passed: 188 checks, zero failures. A separate negative-control build forced the final multipage check to fail and returned exit 1 as required.
- Standard CMake/Ninja build remained stalled before compilation (no active cl.exe), including after explicit Kitware reconfiguration; interrupted rather than repeatedly waiting. No UI or product behavior changed.

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


## 2026-11-03 - Fixture resource validation (technical)
Portable verifier now checks font/image/form bindings and types, alpha values,
and per-page mask references. Four existing fixtures and 56 corruption controls
passed, including five isolated second-page corruptions. No product/UI or Esko
changes; v24 remains primary, v25 supplementary. USER32 Win32 1114 persists;
full CTest has three startup failures (0xc0000142), three dependency skips,
zero product checks. Structural checks do not establish visual preservation.
See .codex-supervisor/PROGRESS.md for blockers and next concrete task.


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
