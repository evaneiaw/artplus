# ADR-0005 — Editing and saving strategy (milestone 22, second slice)

- Status: accepted (2026-11-02)
- Context: first real edit/save cycle

## Problem

ArtPlus must edit PDF content and save without destroying resources it does not
understand. PDFium is the only PDF engine in the project.

## Decision

1. **Edits go through PDFium page objects.** `PdfEditor` keeps one `FPDF_PAGE`
   open, imports every top-level object (kind, bounds, matrix, path segments,
   text, image metadata, form children) and applies transforms with
   `FPDFPageObj_TransformF` followed by `FPDFPage_GenerateContent`.
2. **Render the edited page from the editor handle.** Rendering through a fresh
   `FPDF_LoadPage` would not see ungenerated edits, so the canvas renders from
   the editor's page while a page is being edited.
3. **Save with verification, never blind.**
   - Source file is never written; an absolute-path check refuses it.
   - Pages containing `Shading` or `Unknown` objects are rejected before
     writing (cannot be preserved safely yet).
   - The save is first attempted **incrementally** (`FPDF_INCREMENTAL`); the
     written file is reopened and every object's kind and bounds compared
     against the post-edit model. If that fails, a **full rewrite**
     (`FPDF_NO_INCREMENTAL`) is attempted and verified the same way.
   - If neither verifies, the temp file is deleted and the export is aborted
     with an explanatory error.
4. **UI is transform-only for now:** object list, canvas selection, move by 5 pt
   (Ctrl+arrows), Save As (edited). Page change/close asks before discarding.

## Consequences

- Positive: a real edit round-trip that is proven, not assumed; unknown content
  is blocked instead of silently lost; the source stays intact.
- Negative: editing is limited to object transforms. Path points, text runs and
  image placement are imported but not editable yet.
- Negative: OCG layers are unavailable because PDFium's public headers expose
  no optional-content accessor; the Layers panel keeps one synthetic layer and
  does not invent names.
- Negative: verification covers object presence and geometry only; spot
  colours, transparency, masks, clipping paths and print resources are not yet
  compared after save and must be treated as unverified.
- Follow-up: extend verification to resource dictionaries and add per-object
  geometry editing (path points, text runs, image placement).
