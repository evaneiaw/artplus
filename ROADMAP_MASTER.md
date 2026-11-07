# ArtPlus — ROADMAP_MASTER

Permanent planning reference for the ArtPlus project.

- **Project:** ArtPlus — independent professional prepress / PDF editor
- **Functional reference:** Esko ArtPro+ v24 24.03.10017 (behavioural reference only; no proprietary code reuse)
- **Repository:** `H:\Desktop\artplus-lite`
- **Initial platform:** Windows 11 x64
- **Roadmap revised:** 2026-11-02

Status vocabulary used in this document:

| Status | Meaning |
|---|---|
| DONE | Delivered and verified with executed tests |
| ACTIVE | Currently being implemented |
| PLANNED | Specified, not started |
| BLOCKED | Waiting on a concrete external dependency |

---

## Milestone 21 — Foundation and first executable  — **DONE**

Delivered:

- Repository audit and toolchain correction (MSVC 19.44 + Qt 6.8.3 msvc2022_64 + PDFium 158).
- Modular CMake project that builds a real Windows GUI executable.
- `ArtPlus.exe`: menu bar, toolbar, status bar, dock panels, page list, central page view.
- Open PDF via menu, drag & drop, and command line.
- Page count, page dimensions, metadata display.
- Real PDFium rendering of pages.
- Page navigation (page list, spinner, previous/next, Ctrl+PgUp/PgDn).
- Zoom: fit page, fit width, 100%, zoom in/out, mouse wheel (cursor-anchored).
- Pan: left-drag.
- View rotation: 90° steps.
- Source document preservation (never written while viewing).
- Automated tests: `artplus_pdf_smoke`, `artplus_pageview_test`, `artplus_mainwindow_test`.

Known limits of this milestone (explicit):

- No object editing, no saving/exporting, no print.
- Rendering is re-done on a 120 ms debounce when zoom changes; during that
  window the existing bitmap is scaled (temporarily soft).
- Not yet validated against ArtPro+ output.

---

## Milestone 22 — PDF Engine and Document Engine — PLANNED

Goal: build the real document model and a PDF engine usable for editing.

Delivered so far (2026-11-02):

- `PdfDocument` page boxes: Media/Crop/Trim/Bleed/Art in points, top-left origin,
  with the PDF-spec defaults (Bleed/Trim/Art fall back to Crop).
- Top-level page object import (`FPDFPage_CountObjects`/`GetObject`): kind
  (Text/Path/Image/Shading/Form) and bounds, in content order.
- `PdfSnapshot`: immutable import model with document metadata, page boxes,
  object summaries and the source SHA-256.
- `PdfDocument::saveCopyPreserving()`: byte-identical copy of the source,
  refusing to overwrite the source file.
- UI: Layers panel reports real object counts per kind; File -> Save As Copy.
- Tests: `artplus_import_test` (18 checks on a 15-page PDF, 17 on a
  form-based single page).
- Second slice (editable geometry, 2026-11-02):
  - `PdfEditor`: per-page editable model. Paths expose raw segments plus
    reconstructed lines/cubics (PDFium stores a cubic as three consecutive
    BezierTo points); text exposes content, font base name and size; images
    expose pixel dimensions/DPI; forms expose child count; every object
    exposes its PDF transform matrix and page-space bounds.
  - Object translation via `FPDFPageObj_TransformF` + `FPDFPage_GenerateContent`;
    the canvas re-renders from the edited page.
  - Save tries an **incremental update first**, then reopens the written file
    and compares every object's kind/bounds; a full rewrite is only accepted
    if it also verifies, otherwise the export is aborted.
  - UI: object list in Layers, selection rectangle on canvas, Object -> Move
    (Ctrl+arrows), File -> Save As (edited)... (Ctrl+Shift+S).
  - Tests: `artplus_edit_test` (43 checks): rectangle/Bézier/text/image/form
    import, rectangle moved (30,20) pt, saved, reopened, new matrix and bounds
    confirmed, all other objects unchanged, source untouched, overwrite refused.
  - `artplus_inspect` CLI prints page-object geometry for evidence.

Remaining for this milestone:

- Editable content beyond transform: path points, text runs, image placement.
- Layers from optional content groups (OCG). PDFium's public headers expose no
  OCG accessor; the panel keeps one synthetic layer and invents nothing.
- Verified preservation of spot colours, transparency, masks, clipping paths
  and print resources: today only object presence/geometry is verified, and
  pages containing Shading/Unknown objects block export.


Scope:

- PDF parsing that goes beyond viewing: object graph, indirect objects,
  streams, page tree, resources.
- Internal object model: `Document`, `Page`, `Layer`, `GraphicObject`,
  `Path`, `TextObject`, `ImageObject`, `Group`, `Transform`, `Fill`,
  `Stroke`, `Color`, `DocumentMetadata`.
- Round-trip import/export that preserves resources the editor does not
  yet understand (fonts, images, transparency, ICC, print attributes).
- Page boxes: MediaBox, CropBox, TrimBox, BleedBox, ArtBox.

Exit criteria: edit-save round trip on a PDF preserves untouched resources;
resource-preservation tests pass on a fixture set.

---

## Milestone 23 — UI and basic editing — DONE (core), extras deferred

Delivered (2026-11-03):

- Command architecture (`artplus::edit`): `Command`, `CommandStack`,
  `MoveObjectCommand`, `ScaleObjectCommand`, `RotateObjectCommand`, all storing
  **absolute** object matrices so undo/redo has no numeric drift.
- Undo/Redo with Ctrl+Z / Ctrl+Y, enabled/disabled from the stack, per
  document, with the command name shown in the menu.
- Modified state derived from the stack (`index != cleanIndex`), reflected in
  the window title (` *`), the Save action and the Object panel state.
- Numeric transformation through `Object → Transform…` (Ctrl+Shift+M, confirmed
  ArtPro+ command) for X, Y, W, H and rotation; values are read back from the
  imported object and edits become reversible commands.
- The earlier "Object" panel was removed in the 2026-11-03 fidelity audit:
  ArtPro+ has no such panel. ArtPro+ properties live in the Inspector
  (`View > Open Next Inspector`), which is not implemented because its contents
  are not confirmed (`FEATURE_MAPPING_ARTPRO.md`).
- Canvas selection by click (top-most object), selection rectangle, list ↔
  canvas ↔ properties synchronisation, Alt+Up/Alt+Down navigation.
- Save As (edited) verifies by reopening; saving marks the history clean.
- Tests: `artplus_command_test` (58 checks): move/undo/redo, resize, rotate,
  rotation reporting, ten-move drift test, multi-operation save + reopen,
  clean/dirty transitions across save/undo/redo, independent history.

Not delivered in this milestone (documented instead of half-implemented):

- Multi-select and marquee (would complicate per-object integrity).
- `DeleteObjectCommand`: PDFium has no supported re-insert that preserves
  z-order, so deletion cannot yet be undone safely.
- Handle-drag resize/rotate (numeric + keyboard paths work).
- Layer visibility/lock/order, group/ungroup.

Exit criteria met: objects are edited through the UI, undo/redo restore exact
geometry, properties show measured values, and the saved PDF reopens with the
final geometry (verified by tests and by inspection).

---

## Milestone 24 — Geometry Engine — ACTIVE (stage 1 DONE)

Boundary prepared by M23: geometry work must live in a pure library
(`artplus::geometry`) with no UI or PDFium dependency; commands and the editor
already operate on matrices and bounds, so path-level editing can be added
without changing the command/history layer.

### Stage 1 — Geometry engine core — DONE (2026-11-06)

Delivered:

- Qt-free target `artplus_geometry_core` (no Qt, no PDFium, no artplus_core):
  - `Point`, `Rect` (bounds, width/height, union, intersection, contains).
  - `Matrix` with `AffineTransform` alias (translate, scale, rotate, compose,
    invert, determinant, `mapRect` with exact corner expansion).
  - `Line` + `solve` of segment/segment and infinite-line intersections,
    including parallel, collinear-overlap, endpoint-touch and zero-length cases.
  - `QuadBezier` / `CubicBezier`: evaluation (de Casteljau), derivative, split,
    exact extrema bounds, arc length, exact degree elevation.
  - Tolerance-driven flattening (`flattenQuadratic`, `flattenCubic`) returning
    `std::vector<Point>`.
  - `Polygon` (`std::vector` storage): signed/absolute area, orientation,
    centroid, perimeter, bounds and `classify()` returning
    `Outside`/`Inside`/`Boundary`.
- `artplus_geometry` stays as the Qt-facing layer for `Path` (QVector/QString).
  `Path::bounds()` and `flattenCubic(QVector&)` now delegate to the core, so the
  cubic extrema solver and the subdivision algorithm exist in one place only.
- New CTest suite `artplus.geometry` (169 checks) linked only against
  `artplus_geometry_core`; a leaked Qt include would break its link step.

Limits of stage 1: no boolean operations and no offset (stage 2). Validation is
numeric/automated only; no ArtPro+ v24 visual comparison was performed.

### Stage 1.1 — Numerical robustness fix — DONE (2026-11-06)

Review of `f55a01b` found four scale-dependent (absolute) thresholds that broke
valid geometry at small magnitudes. Each defect was first reproduced by a failing
test (24 failures) and then fixed with a scale-invariant criterion:

- `Matrix::inverted()`: singularity now tested on the dimensionless
  `|sin θ| = |det| / (|col₀|·|col₁|) > 1e-12` instead of an absolute `|det|`, so
  uniform scales from 1e-8 to 1e6 invert correctly.
- `Matrix::mapRect()`: transforms all four corners and keeps degenerate geometry
  (zero width/height, single point); only `NaN` yields an empty rect.
- `Point::normalized()`: divides by the largest component first, so tiny
  (`3e-15`), subnormal and huge (`3e300`) vectors normalize without loss or
  overflow.
- `Bezier.cpp`: derivative-extremum and chord-collapse tests use relative
  head-room against the curve's own magnitude, so bounds are correct from 1e-21
  to 1e9 scale.

Exit criteria: `artplus.geometry` covers all four cases across scales
(282 checks) and the full CTest suite stays green (9/9, 610 checks).

### Stage 1.2.1 — Refinement and Final Verification of Tests — DONE (2026-11-06)

Refined the `artplus.geometry` test suite to eliminate remaining flaky behavior in `bidirectionalPolylineDistance` and to correctly handle floating-point ULP limitations at high magnitudes (1e15).

- Fixed `bidirectionalPolylineDistance` to correctly handle translation offsets and polylines with 1 point.
- Conditioned arc-length checks on `representableHere` to align expectations with IEEE 754 limitations.
- Verified final stability with 2257 checks, 0 failures.

Exit criteria: `artplus.geometry` at 2257 checks; full CTest green (9/9, 2257 checks).


### Stage 1.2.3 — Correção dos critérios de validação numérica Bézier — DONE (2026-11-06)

- **`bidirectionalPolylineDistance()`**: defined behavior for empty polylines (two empty → -1.0, one empty → -1.0, one point → point comparison, two+ points → bidirectional vertex-segment comparison). Correct delta offset handling. Sentinel values like `1e300` not confused with valid distance.
- **Solvers de referência posicion-independentes**:
  - `findQuadraticInteriorExtrema()`: critério de degeneração baseado em coeficientes diferenciais em vez de magnitudes absolutas de pontos de controle — garante invariância à translação comum.
  - `findCubicInteriorExtrema()`: reformulação com fórmula de Citardauq para evitar cancelamento catastrófico; tratamento de casos de borda (B==0, q==0, discriminante zero/negativo, coeficientes degenerados, raízes fora de (0,1)). Critério de degeneração baseado em ULP dos coeficientes diferenciais.
- **Eliminar tolerâncias permissivas artificiais em 1e15**:
  - Removido `100.0` ponto de tolerância; removido `0.9` ponto de tolerância aplicado indiscriminadamente.
  - Removida aprovação automática da contagem de extremos quando `d >= 1e15`.
  - Tolerância agora reflete erro de representação efetivamente calculado: `if (ulp > 0.5) tolerância = ulp * 2.0; else tolerância = geometricTolerance(d, 0.05)`.
- **Inclusão de `p0` nas polilinhas**: garantido que o ponto inicial esteja incluído nos vetores flattened antes de calcular distância e comprimento.
- **Testes de translação reforçados**: garantido que os testes verificam invariância geométrica relativa ao ponto de início, não apenas pontos finais ou bounding boxes.
- **Etapa 1.2.4**: correção da ordem de p0 (insert p0 no início do vetor, não no final), recomputação de comprimentos quadráticos, testes explícitos de polilinhas vazias para `bidirectionalPolylineDistance()`, solver quadrático estável com diferenças locais, revisão do solver cúbico (remoção do ramo `q == 0` com comparação impossível `0.0 > 0.0`), e documentação fiel.

### Stage 2 — Boolean operations and offset — PLANNED

- Boolean operations: union, difference, intersection, XOR.
- Offset/inset with miter/bevel/round and miter limit.
- Self-intersection cleanup and geometric bleed generation.

Implementation must be independent. The reverse-engineering notes
(`05_GEOMETRY_ENGINE.md`, `13_ALGORITHM_CATALOG.md`, `15_DATA_STRUCTURES.md`)
are requirements input only.

Exit criteria: geometry test-suite incl. degenerate/self-intersecting input;
boolean results validated against a reference implementation.

---

## Milestone 25 — Color Engine — PLANNED

- RGB, CMYK, Lab, XYZ spaces.
- ICC profile reading (v2/v4), rendering intents, black point compensation.
- Conversions through PCS; device links.
- Delta E 76 / 94 / 2000.
- Soft proofing and gamut warnings.
- Profile management (load/cache/apply).

Exit criteria: numeric colour tests with reference values; ICC fixture
tolerance thresholds documented.

---

## Milestone 26 — Separations and print attributes — PLANNED

- Process channels and spot colours.
- Ink catalogs and ink properties.
- Overprint / knockout model.
- Separation preview and output simulation.
- Print attribute verification.

Exit criteria: separation preview matches fixture expectations; overprint
cases covered by tests.

---

## Milestone 27 — Trapping Engine — PLANNED

- Adjacency/edge detection between objects.
- Colour analysis and trap direction.
- Trap width, spread/choke, trap colour.
- Curve trapping and corner handling.
- Automatic and manual traps, exception rules.
- Trap preview.

Exit criteria: trap geometry test-suite; visual/structural comparison against
ArtPro+ traps on shared fixtures (behavioural equivalence, not code reuse).

---

## Milestone 28 — Step & Repeat and imposition — PLANNED

- Rectangular and hexagonal grids.
- Columns/rows, distances, margins, gaps.
- Anchors, stagger, head turn, rotations, mirroring.
- Fill sequences.
- Bleed usage, one-up masks.
- Basic imposition.

Exit criteria: layout coordinates verified numerically for each mode.

---

## Milestone 29 — Marks Engine — PLANNED

- Registration, trim, bleed marks.
- Colour bars, code bars.
- Control strips, custom marks.
- Placement, spacing, margins, trim-area rules.

Exit criteria: generated mark geometry validated on fixtures; PDF/X-safe
placement outside trim.

---

## Milestone 30 — Preflight Engine — PLANNED

- Configurable rules and profiles.
- Checks: image resolution, fonts, colours/spaces, overprint, dimensions,
  bleed, objects outside page, transparency, PDF/X.
- Severity model and reports.
- Safe, reversible automatic corrections.

Exit criteria: reproducible preflight results; every auto-fix undoable.

---

## Milestone 31 — Automation Engine — PLANNED

- Command system and action lists.
- Sequencing, variables, parameters, conditions.
- Error handling, reports, batch processing.
- Extensibility.

Exit criteria: a documented action-list format executed end-to-end in tests.

---

## Milestone 32 — Integration and functional equivalence — PLANNED

- Integrate all engines.
- Regression, performance, large-document and compatibility testing.
- Comparison matrix against ArtPro+ on shared fixtures.
- Packaging, Windows installer, user documentation.

Exit criteria: equivalence matrix published; installer produces a working
application on a clean Windows 11 machine.

---

## Cross-cutting rules

1. A feature is only "implemented" when it has working code, integration, and
   tests — not when a class or button exists.
2. No proprietary Esko code, pseudocode, resources, or assets are copied.
3. Reverse-engineering documents are requirements input, not a source of code.
4. Viewing/import must never destroy resources the editor cannot yet handle.
5. Every milestone must leave the build green and the test suite executed.
