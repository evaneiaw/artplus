# ArtPlus — FEATURE_MATRIX

Status vocabulary (from the project plan):

- `NÃO INICIADA` — not started
- `EM DESENVOLVIMENTO` — code exists, incomplete
- `IMPLEMENTADA` — code integrated into the application
- `TESTADA` — covered by executed automated tests
- `VALIDADA POR COMPARAÇÃO` — behaviourally compared with ArtPro+ on shared fixtures

Current validation note: the generated transparency/clipping preservation
fixture passes in the full CTest run. This evidence is limited to the fixture;
spot colours and soft masks remain unverified.

Runtime validation on 2026-11-03: all six registered CTest tests passed (100%),
including edit, command, page view, smoke, import and main window coverage.
`Object → Transform…` now has the confirmed v24 dialog layout and remains
covered by the existing command/edit regression path.

Milestone 21 scope only is claimed below. Scaffolding modules that compile but
are not wired into the editor are marked `NÃO INICIADA` at feature level.

## Application / shell

| Feature | Status | Evidence |
|---|---|---|
| Transform horizontal/vertical flip | TESTADA | User-supplied v24 capture; both axes preserve bounds, reverse orientation, undo/redo restores matrix; full CTest 6/6 |
| Executable with main window | TESTADA | `artplus_mainwindow_test`, screenshots |
| ArtPro+-style menus (File…Help) | IMPLEMENTADA | screenshots vs `artpro24_full.png` |
| Top icon strip (Object Browser … Preview) | IMPLEMENTADA | screenshots; state mirroring reference |
| Status bar (coords, page, units, zoom) | TESTADA | screenshot; cursor callback |
| Left dock: Messages tree | IMPLEMENTADA | screenshot |
| Right dock: Document / Layers / Separations / Quality Control | IMPLEMENTADA | screenshot |
| Page Boxes overlay + Media/Trim chips | TESTADA | screenshots; toggles wired |
| Crosshair cursor mode | IMPLEMENTADA | UI toggle wired to PageView |
| Drag & drop PDF | IMPLEMENTADA | code path present; manual check |
| CLI PDF argument | TESTADA | app launched with PDF path |

## PDF viewing

| Feature | Status | Evidence |
|---|---|---|
| Open PDF file | TESTADA | smoke + integration tests |
| Page count | TESTADA | 15 pages reported |
| Page dimensions | TESTADA | 595.30 × 841.89 pt |
| Metadata (title/author/version/encrypted) | TESTADA | author + PDF 1.7 reported |
| Render page (PDFium) | TESTADA | 893×1263 px + PNG output |
| Render last page | TESTADA | render_page15.png |
| Thumbnail render | TESTADA | smoke test |
| Invalid file rejection | TESTADA | "Not a PDF or corrupted file" |
| Source preservation | TESTADA | SHA-256 unchanged |
| Page boxes (Media/Crop/Trim/Bleed/Art) | TESTADA | `artplus_import_test` incl. defaults |
| Page object import (kind + bounds) | TESTADA | 946 text objects (15-page PDF), 2 Form XObjects (sample) |
| Import snapshot model (`PdfSnapshot`) | TESTADA | metadata, boxes, objects, source hash |
| Save As Copy (byte-identical) | TESTADA | import test: hash equal, reopens, refuses overwrite |
| Editable path import (segments + cubic reconstruction) | TESTADA | edit test: re/bezier fixture |
| Text/image/form import details | TESTADA | edit test: text/font/size, image 8×8, form 1 child |
| Object selection | IMPLEMENTADA | Layers object list + canvas dashed rectangle (screenshots) |
| Object translation | TESTADA | edit test + UI Ctrl+arrows |
| Save edited PDF (verified by reopen) | TESTADA | edit test (43 checks) + UI-saved file inspected |
| UI save (Ctrl+Shift+S) | TESTADA manual | `artplus_ui_edited.pdf` inspected: rect x 100→130 |
| OCG / real layers | NÃO INICIADA | no PDFium public OCG API; synthetic layer only |
| Edit path points / text runs / image placement | NÃO INICIADA | M22 remaining |
| Page navigation (list/spinner/prev/next) | TESTADA | integration test |
| Zoom: fit page / fit width / 100 % | TESTADA | pageview + integration tests |
| Zoom: in / out / wheel (cursor anchored) | TESTADA | pageview test (in/out); wheel logic implemented |
| Pan (drag) | TESTADA | pageview test |
| View rotation 90° | TESTADA | pageview + integration tests |

## Editing

| Feature | Status | Evidence |
|---|---|---|
| Menu structure mirrors ArtPro+ v24 | TESTADA manual | captures `artpro24_menu_*.png` vs `artplus_menu_object.png`; ADR-0007 |
| Object selection (list + canvas click) | TESTADA | screenshots; click hit-test in `selectObjectAt` |
| Object navigation (Alt+Up/Down) | IMPLEMENTADA | actions wired |
| Move objects | TESTADA | command test + UI Ctrl+arrows |
| Resize objects (numeric W/H) | TESTADA | command test (anchor verified) + Object panel |
| Rotate objects (numeric + command) | TESTADA | command test (90° swap, -90 reporting) |
| Transform dialog (X/Y/W/H/Rot) | TESTADA manual | `Object → Transform…`; confirmed v24 layout and visual refinement; real values from selection; commands verified |
| Fidelity mapping (docs) | IMPLEMENTADA | `ARTPRO_FIDELITY_RULES.md`, `FEATURE_MAPPING_ARTPRO.md`, `UI_MAPPING_ARTPRO.md`, `AGENTS.md` |
| Undo / Redo (Ctrl+Z / Ctrl+Y) | TESTADA | 58 command checks + UI screenshots |
| Modified indicator + clean state | TESTADA | title ` *`; undo→clean, save→clean |
| Save As (edited), verified by reopen | TESTADA | command test + UI save inspected |
| Delete objects | NÃO INICIADA | unsafe (no z-order-preserving re-insert) |
| Multi-select / marquee | NÃO INICIADA | deferred for integrity |
| Handle-drag transform | NÃO INICIADA | numeric/keyboard paths exist |
| Layer management | NÃO INICIADA | needs OCG access |
| PDF export beyond Save As | NÃO INICIADA | — |
| Printing | NÃO INICIADA | — |

## Professional prepress

| Feature | Status |
|---|---|
| Geometry engine (Bézier, boolean, offset) | EM DESENVOLVIMENTO (scaffold, not integrated) |
| Colour engine (ICC, conversions, Delta E) | EM DESENVOLVIMENTO (scaffold) |
| Separations / overprint | NÃO INICIADA |
| Trapping | NÃO INICIADA (model only) |
| Step & Repeat | NÃO INICIADA (params only) |
| Marks generation | EM DESENVOLVIMENTO (mark geometry classes) |
| Preflight | NÃO INICIADA (model only) |
| Automation / action lists | NÃO INICIADA (model only) |

## Equivalence with ArtPro+

| Area | Status |
|---|---|
| Any behavioural comparison | NÃO INICIADA |

## Engineering verification — 2026-11-03

| Capability | Status | Evidence |
|---|---|---|
| Multipage regression exit status | TESTADA (technical) | late intentional failure returns 1 in negative-control executable |
| Untouched page after other-page transform/save | TESTADA (generated fixture only) | distinct size, object count, pixel-identical rendering, source hash; edit suite 62 checks |

These checks do not establish general preservation of spot colours, masks, clipping or transparency.

| CTest regression orchestration (technical) | IMPLEMENTED; runtime blocked | six tests discovered; filtered fixture dependency verified; execution fails before checks with 0xc0000142 |

| CTest Windows PATH preservation (technical) | VERIFIED via CMake-only probe | six properties checked; inherited PATH preserved; negative control rejected. Product runtime remains blocked by 0xc0000142. |

| Isolated Windows DLL loader (technical) | VERIFIED controls; product runtime blocked | Static CRT build; four controls pass; USER32/GDI32 load fails with 1114. See tests/runtime_loader_probe.cpp and build-msvc/tests/runtime-probe.log. |


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
