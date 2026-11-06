# ADR-0006 — Editing commands, history and properties

- Status: accepted (2026-11-03)
- Context: milestone 23, turning the single transform into an editing system

## Decision

1. **New `artplus::edit` library** between the PDF engine and the UI:
   `Command`, `CommandStack`, transform commands and `ObjectProperties`.
   The UI depends on it; it depends only on `artplus::pdf` and Qt Core/Gui.
2. **Commands store absolute matrices.** Each command captures the object's
   `before` matrix and a computed `after` matrix. `apply()` writes `after`,
   `revert()` writes `before`. Deltas are never accumulated, which removes
   numeric drift over long undo/redo chains.
3. **Composition order was established empirically.** `FPDFPageObj_TransformF`
   composes on the left (`new = relative * current`), so
   `PdfEditor::setObjectMatrix(target)` applies
   `relative = target * inverse(current)`. The first implementation assumed
   right-composition and produced half-size moves and off-anchor scaling; the
   command tests caught it.
4. **Selection is not a document command.** Selecting an object does not modify
   the document and must not create undo entries; selection lives in the UI and
   is synchronised between canvas, list and properties panel.
5. **No `DeleteObjectCommand` yet.** PDFium's `FPDFPage_RemoveObject` changes
   object indices and `FPDFPage_InsertObject` appends at the end, so z-order
   cannot be restored reliably. A delete that cannot be undone safely would
   violate the project rules, so it is deliberately not offered.
6. **No multi-select/marquee yet.** Single-object selection keeps every
   transform command unambiguous and verifiable; multi-select is deferred.
7. **History is per document and page-scoped.** Opening a document creates a
   new stack; changing page with unsaved edits asks for confirmation and then
   clears the stack, because the editor handle for the previous page is closed.
8. **Modified state is stack-derived** (`index != cleanIndex`). Saving calls
   `markClean()`; undo after save marks the document modified again.
9. **Properties are measured, not estimated.** Values come from PDFium bounds,
   the object matrix, text extraction and image metadata. Rotation is
   `atan2(b, a)` of the matrix and is reported as “no rotation” near zero.
   Skewed matrices therefore expose their rotation component, not a skew angle.

## Consequences

- Positive: exact undo/redo, no drift, honest modified state, real properties,
  and a command layer that future tools (path editing, boolean ops) can reuse.
- Negative: editing is still transform-only (no point/text editing), no delete,
  no multi-select, and no handle dragging; those remain for later milestones.
- Follow-up (M24): geometry operations should be implemented as pure functions
  in `artplus::geometry` and wrapped by new commands; the history and save
  layers need no change.
