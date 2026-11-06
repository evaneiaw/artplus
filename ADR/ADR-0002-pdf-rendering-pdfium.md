# ADR-0002 — PDF rendering: PDFium

- Status: accepted (2026-11-02)
- Context: milestone 21, first executable must display real PDFs

## Problem

ArtPlus needs to parse and rasterise arbitrary production PDFs. Writing a PDF
rasteriser is a multi-year effort; the first executable must render accurately
now.

## Options considered

1. **PDFium** — BSD-3, used by Chrome, robust C API, good text/vector/image
   fidelity, prebuilt MSVC binary available locally.
2. **MuPDF** — AGPL or commercial licence; AGPL is unsuitable for a closed
   commercial prepress product.
3. **PoDoFo** — LGPL, parsing/manipulation oriented, weak rasteriser.
4. **qpdf** — Apache-2, structural tooling, no rasteriser.
5. Custom rasteriser — rejected for milestone 21 (scope).

## Decision

Use PDFium as the rendering authority, wrapped behind `artplus::pdf::PdfDocument`
and `artplus::pdf::PdfRuntime`. No PDFium types leak into UI or document code.

## Consequences

- Positive: fast path to accurate rendering; BSD licence compatible with a
  commercial product when notices are preserved.
- Negative: editing must not be done by mutating PDFium page objects directly;
  the editable object model is ours (milestone 22). PDFium will be used for
  rasterisation and low-level inspection, not as the document model.
- Follow-up: keep `third_party/licenses/` with the binary; record the version
  (158.0.8125.0) in `DEPENDENCIES_AND_LICENSES.md` (done).
