# ADR-0004 — ArtPro+-style workspace with original artwork

- Status: accepted (2026-11-02)
- Context: the user asked for the workspace to look like ArtPro+

## Problem

The workspace must be visually close to Esko ArtPro+ without copying
proprietary assets, and every visible panel must reflect real application
state.

## Reference method

Instead of guessing, the installed ArtPro+ v24 was launched and captured
(`research/artpro_analysis/artpro24_full.png`) with the same PDF used for
ArtPlus. Colours, panel order, header style, page framing and tool-strip
contents were read from that capture. No Esko bitmap, icon, font or string
asset is shipped.

## Decision

1. **Fusion style + tuned palette** (`theme::apply`) reproducing the light-grey
   chrome, dark panel headers (`#595959`), white table bodies and light canvas
   (`#DCDCDC`).
2. **All tool icons drawn in code** (`Theme.cpp`) from primitives; visual
   kind matches the reference, artwork is ours.
3. **Custom widgets**:
   - `CollapsiblePanel` for the dark-header dock sections;
   - `PageView` extended with Media Box frame, `Media Box` / `Trim Box` chips
     and size chip in millimetres.
4. **Layout**: menu bar, a single icon strip (large file/browser tools +
   small prepress tools + right-aligned 3D/Preview), `Messages` dock on the
   left, `Document/Layers/Separations/Quality Control` stack on the right,
   status bar with cursor coordinates, page, units and zoom.
5. **Honesty rules**: panels never show invented data. The document profile
   is read from the PDF output intent; when absent the panel says
   “(not specified)”. Spot separations are not listed until the separation
   engine exists (milestone 26).

## Consequences

- Positive: workspace is immediately recognisable as ArtPro+-like; no legal
  exposure from copied assets; all panels are functional.
- Negative: icons are approximations; exact pixel parity is not claimed.
- Follow-up: milestone 26 must feed real spot separations and profiles into
  the Separations panel; milestone 22 must populate Layers from the document
  model instead of the placeholder “Layer 1”.
