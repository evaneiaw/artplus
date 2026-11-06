# ADR-0007 — Menus mirror the observed ArtPro+ structure

- Status: accepted (2026-11-03)
- Context: the ArtPlus menus contained items invented from generic assumptions
  (for example "Move Left/Right" inside Object and "Save As Copy" inside File).
  The user challenged this and asked where ArtPro+ actually has an Object menu.

## Evidence

The installed ArtPro+ **v24** was launched with a sample PDF and every menu was
captured with the real items:

- `research/artpro_analysis/artpro24_menu_file.png`
- `artpro24_menu_edit.png`
- `artpro24_menu_object.png`
- `artpro24_menu_select2.png`
- `artpro24_menu_prepress2.png`
- `artpro24_menu_view2.png`
- `artpro24_submenu_zoom.png`

The Object menu is real and contains: Arrange; Transform… (Ctrl+Shift+M);
Transform Again (Ctrl+D); Group (Ctrl+G); Ungroup (Ctrl+Shift+G); Lock (Ctrl+2);
Unlock All (Ctrl+Alt+2); Hide (Ctrl+3); Show All (Ctrl+Alt+3); Path, Compound,
Clipping Mask, Opacity Mask, Image, Text, Placed Art (submenus); Remove Color
Profile (submenu); Recolor…; Shaper… (Ctrl+E); Spread/Choke… (Ctrl+Shift+T);
Expand; Rasterize…; Clean, Clip, Fit (submenus); Convert to Placeholder…

## Decision

1. **Menus are rebuilt from the captures**, not from assumptions. Menu titles,
   order, separators, shortcut assignments and enabled/disabled state follow the
   reference.
2. **Unimplemented features are disabled**, and submenus whose contents were not
   captured contain a single disabled entry "Not implemented in ArtPlus yet".
   Nothing is invented as if it worked.
3. **ArtPlus-only conveniences are removed from menus**: object nudge
   (Ctrl+arrows) still works as a hidden shortcut, and the byte-identical
   "Save As Copy" moved out of File (the API/test remains). Redo is now
   Ctrl+Shift+Z as in ArtPro+; object navigation is Ctrl+Tab / Ctrl+Shift+Tab
   under Select.
4. **Window and Help keep ArtPlus-specific content** (panel toggles, About),
   because their ArtPro+ captures were not reliable; this is stated explicitly.

## Consequences

- Positive: the menus now match the real application structure and no longer
  pretend that unimplemented ArtPro+ features exist.
- Negative: many entries are disabled (honest, but not functional), and
  submenu contents not captured (Rotate View, Pages, Go To, Action Lists, …)
  remain placeholders.
- The reference is v24; v250 may differ. Only v24 was captured.
- Follow-up: capture the remaining submenus (Rotate View, Pages, Go To, Action
  Lists, Window) before enabling any of those entries.
