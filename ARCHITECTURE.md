# ArtPlus — ARCHITECTURE

This document describes the architecture that exists in the repository today
(milestone 21) and the boundary it is designed to grow along.

---

## 1. Layering

```
┌──────────────────────────────────────────────────────────────┐
│ app/                     ArtPlus.exe (WinMain entry)          │
├──────────────────────────────────────────────────────────────┤
│ ui/                      MainWindow, PageView, docks, status  │
├──────────────────────────────────────────────────────────────┤
│ pdf/                     PdfDocument / PdfEditor / PdfSnapshot  │
│ edit/                    Command stack, object properties      │
│ document/                ArtDocument / ArtPage / ArtLayer     │
│ objects/                 GraphicObject, Paint                 │
│ geometry/                Point, Rect, Matrix, Path            │
│ colors/                  Color, colour spaces                 │
│ tags/                    TagSet (metadata attached to nodes)  │
│ marks/                   mark geometry and TaskMarkSet        │
│ stepandrepeat/           grid parameters                      │
│ trapping/                trap model (scaffold)                │
│ preflight/               rule model (scaffold)                │
│ automation/              action-list model (scaffold)         │
│ export/                  export entry points (scaffold)       │
│ core/                    Logger, Units, Version               │
└──────────────────────────────────────────────────────────────┘
```

Rules:

- `core` and `geometry` are leaf libraries (no UI, no PDF).
- `pdf` depends on PDFium and Qt only.
- `ui` depends on `pdf` and `document`; it must not contain file-format logic.
- Libraries are STATIC; the only shared artefact is `pdfium.dll`.
- A module may only call *down* its own dependency tier.

## 2. Runtime data flow (viewing, milestone 21)

```
CLI arg / Open dialog / drop
        │
        ▼
MainWindow::openDocument(path)
        │  PdfDocument::open()
        ▼
artplus::pdf::PdfDocument ──(PDFium)──► FPDF_DOCUMENT
        │
        │ pageCount(), pageSize(), title(), ...
        ▼
MainWindow: populate page list, info panel, status
        │
        │ view->fitPage()  (zoom = px per PDF point)
        ▼
MainWindow::renderCurrentPage()
        │  PdfDocument::renderPage(page, zoom, userRotation)
        ▼
QImage (rendered by PDFium at the requested scale, capped at 8000 px)
        │
        ▼
PageView::setPageImage() + setPageSize(displayed size in points)
        │
        ▼
PageView::paintEvent(): centred page rect, shadow, border, image
```

Zoom/pan/rotation live entirely in `PageView`; the window re-renders only
when asked. This keeps the viewport responsive and the PDF engine stateless.

### Coordinate conventions

- PDF points everywhere in the engine (`pageSize` is unrotated).
- `PageView::zoom()` = pixels per PDF point.
- `PageRenderResult::pageSizePoints` is unrotated; the window converts it to
  the *displayed* size using `intrinsic /Rotate + user rotation`.
- Page Y axis is PDF-native (origin bottom-left); the renderer returns an
  upright bitmap, so the viewport treats it as an image (top-left origin).

## 3. Threading model

- All PDFium calls happen on the GUI thread in milestone 21.
- Rendering is debounced with a single-shot `QTimer` (120 ms) to avoid
  re-rendering on every wheel tick.
- Milestone 22 will move rendering to a worker with a tile cache; the view
  API (`setPageImage`) is already decoupled from the renderer.

## 4. Document model (present, used by milestones 22+)

- `document::ArtDocument` owns pages; `ArtPage` owns layers; `ArtLayer` owns
  `objects::GraphicObject` instances.
- `objects::GraphicObject` is the polymorphic base (bounds, transform, paint).
- `tags::TagSet` attaches arbitrary metadata to pages/layers without changing
  their layout.
- The model is deliberately *not* yet wired into the viewer: milestone 21
  renders PDF bytes directly so that untouched resources cannot be damaged.

## 5. Preservation strategy

- `PdfDocument::open()` opens the file read-only through PDFium.
- Nothing in the viewing path writes to the source file.
- `saveCurrent()` currently returns `false`; there is no implicit save.
- The smoke test asserts the SHA-256 of the source file is identical after
  open + render.

## 6. Error handling

- `PdfDocument::open(path, &error)` reports a human-readable reason.
- The UI shows a modal error for open failures and logs via `core::Logger`.
- The renderer returns an empty `QImage` on failure; the window keeps the last
  good frame and logs a warning.

## 7. Build topology

- `build-msvc/` — the supported build tree (MSVC + Qt msvc2022_64 + PDFium).
- `build/` — legacy MinGW tree, unbuildable with the MSVC Qt kit; kept only
  until reviewed, then to be deleted.
- `third_party/` — PDFium package (`PDFiumConfig.cmake`, `bin/pdfium.dll`).
- Qt comes from `C:\Qt\6.8.3\msvc2022_64`.

## 8. Deliberate non-goals for milestone 21

- No edit/save pipeline (would require the internal object model and resource
  preservation, milestone 22).
- No custom rasteriser: PDFium is the rendering authority.
- No plugin system yet: modules are static libraries with explicit interfaces.

## 9. Editing pipeline (milestone 23)

```
user action (nudge / property edit / future handles)
        │  constructs a Command with absolute before/after matrices
        ▼
artplus::edit::CommandStack::execute()
        │  apply() -> PdfEditor::setObjectMatrix()
        ▼
FPDFPageObj_TransformF(new = relative * current)   [PDFium]
        │
        ├─ PdfEditor::applyEdits()  -> FPDFPage_GenerateContent()
        ├─ MainWindow re-renders from the edited page handle
        ├─ Object list / properties / title refresh
        └─ CommandStack::isModified() drives the unsaved marker

Undo  -> Command::revert() restores the stored *before* matrix
Redo  -> Command::apply()  restores the stored *after* matrix
```

Design rules:

- Commands store absolute matrices, never deltas, so repeated undo/redo is
  numerically stable.
- Selection is UI state and is not part of the document history.
- A command that changes nothing is not pushed (`apply()` must change state).
- Save writes a separate file, verifies by reopening, then `markClean()`.

## 10. Boundary prepared for the Geometry Engine (M24)

`artplus::geometry` stays a pure library (points, rects, paths, matrix helpers;
no UI, no PDFium). Commands and the editor already exchange matrices and
bounds, so path-level editing (move points, intersect, offset) can be added by
(a) extending `EditableObject::path`, (b) adding commands that call geometry
functions, and (c) writing the new path back through PDFium path APIs — without
changing the history or persistence layers.