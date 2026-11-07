# ArtPlus — REMEDIATION PLAN (AUDIT-001)

## Audit Context

This remediation plan results from the AUDIT-001 general implementation and integration audit. It identifies and prioritizes the remediation actions needed to advance ArtPlus to the next development phase. No new code was written during this audit phase; this plan organizes and prioritizes existing blocker information.

## 1. Immediate Blockers (P0 Priority)

### 1.1 Windows GUI Initialization Failure

| Identifier | Error Code | Impact | Status |
|---|---|---|---|
| USER32/GDI32 DLL load failure | Win32 1114 (ERROR_DLL_INIT_FAILED) | Blocks 3/6 CTest product tests; 3/6 skipped due to fixture dependency | **OPEN** |
| Process exit code | 0xc0000142 | artplus.edit, artplus.command, artplus.pageview fail at startup | **OPEN** |
| Root cause | Not established in current execution context | Requires minimal DLL loader probe without machine changes | **OPEN** |

**Remediation Actions**:
1. Implement `tests/runtime_loader_probe.cpp` (already exists as opt-in target) — static MSVC CRT, one absolute DLL path per process
2. Run four existing loader controls: kernel32 load/unload (pass), missing absolute DLL (Win32 126), relative path/absent argument (exit 2)
3. Isolated loads: KERNEL32, ADVAPI32, MSVCP140, VCRUNTIME140 succeeded; USER32 and GDI32 failed with 1114
4. Deployed Qt6Core/Gui/Widgets and PDFium likewise failed with 1114
5. **Next**: Isolate the failing module with repository-local minimal loader probe using permitted local diagnostics
6. Rerun normal targeted CBuild and CTest suite after module identification

**Owner**: Team Lead / C++ developer  
**Target Resolution**: Before M24 Geometry Engine commencement  
**Dependency**: None (independent diagnostic)

### 1.2 PageView.cpp Namespace Corruption

| Identifier | Issue | Location | Status |
|---|---|---|---|
| Anonymous namespace never closed | ~100 compilation errors (C2888) | `src/ui/src/PageView.cpp` line 17-27 | **OPEN** |
| Lost file tail missing | ~144 lines lost (wheelEvent, mouse events, resizeEvent, etc.) | Same file, after line 276 | **OPEN** |
| paintEvent / namespace artplus::ui never closed | C1075 fatal error | Same file | **OPEN** |

**Remediation Actions** (already performed in 2026-11-05 session):
- Added `}` closing `mmText()` 
- Reorganized anonymous namespace close
- Reconstructed lost file tail: `wheelEvent`, `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `resizeEvent`, `notifyStatus`, `requestRender`, selection and crosshair drawing
- Brace depth verified to 0 by script; no orphan braces

**Note**: Fix already applied in previous session — verify current file state and ensure commit/push reflects the correction.

**Owner**: C++ developer  
**Target Resolution**: Verified (2026-11-05) — pending commit and push to GitHub

---

## 2. High-Priority Remediation (P1 Priority)

### 2.1 Complete ArtPro+ v24 Visual Comparison

| Identifier | Action | Evidence Needed | Status |
|---|---|---|---|
| Capture ArtPro+ v24 UI | Machine with ArtPro+ v24 required | Screenshots of: View→Rotate View menu, shortcuts (Ctrl+Alt+R/U/L), selection behavior after rotation, TAC panel | **OPEN** |
| Pixel-level rotation comparison | ArtPlus vs ArtPro+ v24 rendered output | 918×1188 → 1188×918 at 90°, full cycle SHA-256 preservation | **OPEN** |
| Shortcut exact mapping | Ctrl+Alt+R/U/L vs v24 equivalents | Menu item labels, atalhot tooltips, keyboard response | **OPEN** |
| Selection after rotation | Box alignment at 90°/270° | pdfRectToDisplayed() mapping effectiveness | **OPEN** |

**Remediation Actions**:
1. Identify machine with ArtPro+ v24 installed (user-supplied reference)
2. Capture menu screenshots: `View → Rotate View` submenu at each angle
3. Record keyboard shortcut behavior: which atalhot trigger which rotation
4. Capture selection overlay at 0°, 90°, 180°, 270°
5. Compare TAC panel behavior with CMYK PDFs
6. Document findings in `ARTPLUS_REVERSE_ENGINEERING_AUDIT.md` addendum

**Owner**: Team Lead (requires user coordination)  
**Target Resolution**: After GUI blocker resolution  
**Dependency**: 1.1 (Windows GUI init must be unblocked first, or machine separate from development context)

### 2.2 Integrate Scaffolding Modules into Editor Workflows

| Module | Current Status | Integration Action | Status |
|---|---|---|---|
| Geometry Engine (M24) | Compiles, not integrated | Wire into editor: add geometry transformation commands, connect to UI panel | **OPEN** |
| Color Engine (M25) | Scaffold only | Integrate ICC profile workflow, CMYK conversion (approximate per ARTPRO_FIDELITY_RULES.md) | **OPEN** |
| Preflight Engine (M26/27) | QC panel UI integrated, model partial | Complete model integration: calculateTacDocument full pipeline, add missing check categories | **OPEN** |
| Marks Engine (M29) | Mark geometry classes in development | Connect to document model, add panel UI | **OPEN** |

**Remediation Actions**:
1. Geometry: Connect `src/geometry` scaffold to existing edit command stack
2. Color: Integrate RGB→CMYK approximation (already documented as approximate, no ICC profile)
3. Preflight: Extend beyond QC panel UI — add more check categories, save/load profiles
4. Marks: Review `research/artpro_analysis/REVERSE_ENGINEERING_PANELS.md` for export structure

**Owner**: C++ developers (per module)  
**Target Resolution**: After P0 blockers, before M24 commencement  
**Dependency**: 1.1 (GUI init), 1.2 (PageView namespace already fixed)

---

## 3. Medium-Priority Remediation (P2 Priority)

### 3.3 Real Layers / OCG Support

| Identifier | Blockers | Workaround | Status |
|---|---|---|---|
| PDFium 158 has no public OCG API | No `FPDFDoc_GetInkNames()`, `FPDFDoc_GetSeparations()`, `FPDFPageObj_GetInkCoverage()` | Document as known limitation per ARTPRO_FIDELITY_RULES.md; synthetic "Layer 1" only | **OPEN** |
| Layer names not inventable | Per AGENTS.md Rule 4 | Announce as " Layer 1" only, no layer management UI | **OPEN** |

**Remediation Actions**:
1. Document clearly in `ARTPRO_FIDELITY_RULES.md` and `FEATURE_MATRIX.md`
2. Do not invent OCG/OCMD accessors or layer names
3. If PDFium version upgrade possible, evaluate new API availability
4. Monitor Esko/OEM PDFium updates for OCG accessor

**Owner**: C++ developer / Architecture  
**Target Resolution**: Long-term (post-M32)  
**Dependency**: 1.1 (GUI init — lowest priority for this item)

### 3.4 Delete Object with Safe Undo

| Identifier | Blockers | Approach | Status |
|---|---|---|---|
| PDFium offers no re-insert preserving z-order | Cannot undo delete safely | Document as known limitation; do not implement delete UI item | **OPEN** |
| Per AGENTS.md Rule 4 | Would violate rule to announce non-functional feature | Revert to disabled placeholder (already done) | **COMPLETED** |

**Remediation Actions** (already completed):
- Group/Ungroup/Lock/Unlock All/Hide/Show All reverted to disabled placeholders
- Code comments explain PDFium limitation
- No delete menu item implemented

**Status**: Completed — no further action required (per fidelity rules)

**Owner**: — (already completed)  
**Target Resolution**: 2026-11-04 (already done)  
**Dependency**: None

### 3.5 Multi-Select / Marquee

| Identifier | Blockers | Deferred Action | Status |
|---|---|---|---|
| Per-object integrity protection | Deferred to protect integrity | Do not implement until safe multi-select possible | **OPEN** |
| Per AGENTS.md | Do not invent APIs or behaviors | Document as deferred; do not implement stub | **OPEN** |

**Remediation Actions**:
1. Document as deferred in FEATURE_MATRIX.md
2. Do not implement multi-select or marquee selection
3. Revisit when per-object integrity solution designed
4. Add to M31-Automation or post-roadmap considerations

**Owner**: C++ developer / Lead  
**Target Resolution**: Post-M32 (after integration goals)  
**Dependency**: 1.1 (GUI init — lower priority)

---

## 4. Low-Priority Remediation (P3 Priority)

### 3.6 Handle-Drag Transform

| Identifier | Current State | Action | Status |
|---|---|---|---|
| Numeric/keyboard paths exist | Not UI-wired | Do not implement handle-drag UI; keep keyboard/ numeric paths | **OPEN** |
| Per AGENTS.md Rule 4 | Do not invent behaviors | Document as deferred | **OPEN** |

**Remediation Actions**:
1. Document in FEATURE_MATRIX.md as "numeric/keyboard paths exist, not UI-wired"
2. Do not implement handle-drag transform UI
3. Revisit when per-object integrity solution designed

**Owner**: C++ developer  
**Target Resolution**: Post-M32  
**Dependency**: 1.1 (lower priority)

### 3.7 PDF Export Beyond Save As

| Identifier | Current State | Action | Status |
|---|---|---|---|
| Save As Copy verified | Functional (export gate for shading/unknown) | Do not implement other export modes without safe PDF rebuild | **OPEN** |
| Per AGENTS.md | Do not invent APIs | Document limitation | **OPEN** |

**Remediation Actions**:
1. Document: pages with Shading/Unknown objects block saving
2. Do not implement PDF export, PostScript export, or other formats
3. Keep Save As Copy as only export mode

**Owner**: —  
**Target Resolution**: — (current behavior sufficient)  
**Dependency**: None

---

## 5. Long-Term Remediation (Post-M32)

### 5.7 Automation / Action Lists

| Identifier | Current State | Action | Status |
|---|---|---|---|
| Model only (not integrated) | `src/automation` scaffold exists, not wired | Integrate after M31 roadmap objectives | **OPEN** |

**Remediation Actions**:
1. Review `research/artpro_analysis/REVERSE_ENGINEERING_PANELS.md` for automation structure
2. Integrate action list execution after geometry and preflight are solid
3. Use Ghidra reverse engineering findings as reference for action list format

**Owner**: C++ developer  
**Target Resolution**: Post-M32  
**Dependency**: M24-M31 completion

### 5.8 Step & Repeat / Imposition

| Identifier | Current State | Action | Status |
|---|---|---|---|
| Parameters exist, not integrated | `params only` per FEATURE_MATRIX.md | Integrate after geometry and preflight solid | **OPEN** |

**Remediation Actions**:
1. Review ArtPro+ v24 step & repeat UI flow (capture from real machine)
2. Implement parameter dialog connected to document object copying
3. Test with multi-page fixtures

**Owner**: C++ developer  
**Target Resolution**: Post-M32  
**Dependency**: M24 (Geometry Engine) solidification

---

## 6. Remediation Roadmap Summary

| Priority | Item | Owner | Target | Dependency |
|---|---|---|---|---|
| **P0** | Windows GUI init (0xc0000142) | Lead / C++ dev | Before M24 | None (independent) |
| **P0** | PageView.cpp namespace fix | C++ dev | Verified (2026-11-05) | None |
| **P1** | ArtPro+ v24 visual comparison | Lead (user coord.) | After P0 | 1.1, 1.2 |
| **P1** | Integrate scaffolding (geom, color, preflight, marks) | Per-module devs | After P0 | 1.2 already done |
| **P2** | Real layers / OCG support | Architecture | Long-term | 1.1 (low prio.) |
| **P2** | Delete object safe undo | — (completed) | Completed (2026-11-04) | None |
| **P2** | Multi-select / marquee | C++ dev | Post-M32 | P2 items done |
| **P3** | Handle-drag transform | C++ dev | Post-M32 | — |
| **P3** | PDF export beyond Save As | — | Current suff. | — |
| **P3** | Automation / action lists | C++ dev | Post-M32 | M31 objectives |
| **P3** | Step & repeat / imposition | C++ dev | Post-M32 | M24 solidification |

---
*Remediation plan generated as AUDIT-001 task. Based on evidence from 2026-11-05 audit. No new code written during this audit phase. Priorities aligned with roadmap M21–M32 and fidelity rules.*