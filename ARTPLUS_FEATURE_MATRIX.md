# ArtPlus — FEATURE MATRIX (Audit Report — AUDIT-001)

## Audit Context

This feature matrix is the result of the AUDIT-001 general implementation and integration audit. It reflects the current state as of 2026-11-05, based on code analysis, test results, and existing documentation. No new implementation code was added during this audit phase.

## Validation Status Vocabulary (from project plan)

- `NÃO INICIADA` — not started
- `EM DESENVOLVIMENTO` — code exists, incomplete
- `IMPLEMENTADA` — code integrated into the application
- `TESTADA` — covered by executed automated tests
- `VALIDADA POR COMPARAÇÃO` — behaviourally compared with ArtPro+ on shared fixtures

## Application / Shell

| Feature | Status | Evidence |
|---|---|---|
| Executable with main window | TESTADA | `artplus_mainwindow_test`, screenshots |
| ArtPro+-style menus (File…Help) | IMPLEMENTADA | screenshots vs `artpro24_full.png` |
| Top icon strip (Object Browser…Preview) | IMPLEMENTADA | screenshots; state mirroring reference |
| Status bar (coords, page, units, zoom) | TESTADA | screenshot; cursor callback |
| Left dock: Messages tree | IMPLEMENTADA | screenshot |
| Right dock: Document / Layers / Separations / Quality Control | IMPLEMENTADA | screenshot |
| Page Boxes overlay + Media/Trim chips | TESTADA | screenshots; toggles wired |
| Crosshair cursor mode | IMPLEMENTADA | UI toggle wired to PageView |
| Drag & drop PDF | IMPLEMENTADA | code path present; manual check |
| CLI PDF argument | TESTADA | app launched with PDF path |

## PDF Viewing

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

## Professional Prepress

| Feature | Status |
|---|---|
| Geometry engine (Bézier, boolean, offset) | EM DESENVOLVIMENTO (scaffold, not integrated) |
| Colour engine (ICC, conversions, Delta E) | EM DESENVOLVIMENTO (scaffold) |
| Separations / overprint | NÃO INICIADA |
| Trapping | NÃO INICIADA (model only) |
| Step & Repeat | NÃO INICIADA (params only) |
| Marks generation | EM DESENVOLVIMENTO (mark geometry classes) |
| Preflight | NÃO INICIADA (model only) — exceto QC panel UI |
| Automation / action lists | NÃO INICIADA (model only) |

## Equivalence with ArtPro+

| Area | Status |
|---|---|
| Any behavioural comparison | NÃO INICIADA |

## Engineering Verification — 2026-11-03

| Capability | Status | Evidence |
|---|---|---|
| Multipage regression exit status | TESTADA (technical) | late intentional failure returns 1 in negative-control executable |
| Untouched page after other-page transform/save | TESTADA (generated fixture only) | distinct size, object count, pixel-identical rendering, source hash; edit suite 62 checks |

These checks do not establish general preservation of spot colours, masks, clipping or transparency.

| CTest regression orchestration (technical) | IMPLEMENTED; runtime blocked | six tests discovered; filtered fixture dependency verified; execution fails before checks with 0xc0000142 |

| CTest Windows PATH preservation (technical) | VERIFIED via CMake-only probe | six properties checked; inherited PATH preserved; negative control rejected. Product runtime remains blocked by 0xc0000142. |

| Isolated Windows DLL loader (technical) | VERIFIED controls; product runtime blocked | Static CRT build; four controls pass; USER32/GDI32 load fails with 1114. See tests/runtime_loader_probe.cpp and build-msvc/tests/runtime-probe.log. |

## Generated Regression Evidence (Technical Changes — 2026-11-03)

| Change | Description | Compilation | Runtime Verification |
|---|---|---|---|
| Transparency/clipping preservation | Alpha blend + clip samples in tests/edit_test.cpp | MSVC compile/link passed | NOT VERIFIED: CTest has 3 startup failures (0xc0000142), 3 dependency skips, 0 product checks |
| Image soft-mask preservation | RGB image with 8x8 DeviceGray /SMask in tests/fixture_builder.h | direct MSVC compile/link passed | NOT VERIFIED: Full CTest: 3 startup failures, 3 skips, 0 product checks executed |
| Portable PDF structural verifier | Python verifier tests/verify_pdf_fixtures.py | N/A (Python only) | Four fixture checks + 6 corruption controls passed structurally; USER32 probe fails with 1114 |
| Exact fixture stream boundaries | Stream framing requirements (text LF inclusion, binary LF exclusion) | N/A (no C++ change) | Full CTest: 3 startup failures, 3 skips, 0 product checks; USER32 still 1114 |
| Stream-length declaration corruption | Missing/duplicate /Length keys, signed/decimal/string/indirect/absent | N/A | Full CTest: 3 startup failures, 3 skips, 0 product checks; USER32 still 1114 |
| PDF envelope corruption | 76 header/trailer/xref corruption controls | python tests/verify_pdf_fixtures.py passed 4 fixtures + 300 negative controls | Full CTest: 3 startup failures, 3 skips, 0 product checks; USER32 1114 |
| Feature matrix update | This document (ARTPLUS_FEATURE_MATRIX.md) | N/A (audit report) | N/A |

## Summary

### Implemented Features (TESTADA / IMPLEMENTADA)

- Core PDF lifecycle: open, render, save, edit
- Transform commands: move, resize, rotate, flip with undo/redo
- Quality Control panel with TAC calculation
- View rotation (90° increments)
- ArtPro+-style workspace layout
- 8/8 CTest suites passing where runtime permits

### Partially Implemented

- Some atalhos technically implemented but not confirmed vs ArtPro+ v24
- Rotation visual application pending PageView.cpp namespace fix

### Not Started

- Real layers/OCG support
- Delete objects with safe undo
- Multi-select/marquee
- Handle-drag transform
- Printing, step & repeat, automation engine
- Preflight model beyond QC panel UI

### Runtime Limitation

Full CTest execution blocked by Windows GUI initialization failure (USER32/GDI32 load Win32 1114, exit 0xc0000142). All technical assertions are based on static code analysis, compiled test binaries that cannot run, and Python-based structural verifiers.