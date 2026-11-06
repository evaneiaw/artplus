# UI_WORKSPACE_TARGET — alvo visual do workspace ArtPlus

Status: **implementado** (2026-11-02). Referência: captura real do ArtPro+ v24
(`research/artpro_analysis/artpro24_full.png`), obtida executando o ArtPro+
instalado e abrindo `SamplePDFFile.pdf`. Nenhum ícone, bitmap ou texto
proprietário foi copiado: todos os ícones são desenhados por código
(`src/ui/src/Theme.cpp`).

## Resultado obtido

| Elemento | Estado |
|---|---|
| Menu: File, Edit, Object, Select, Prepress, View, Window, Help | implementado |
| Faixa superior: Object Browser, Colors, Images, Placed Art, Messages, Action Lists, Share & Approve | implementado (ícones próprios) |
| Faixa central: Clean, Select, Recolor, Shaper, Barcode, Screening, Trapping, Wrap | implementado, com estados habilitado/desabilitado como na referência |
| Canto direito: 3D, Preview | implementado |
| Dock esquerdo “Messages” → Other → Missing Real Data (2) | implementado |
| Dock direito: Document (Crosshair, Page Boxes) | implementado e funcional |
| Dock direito: Layers (cabeçalho, Layer 1, ferramentas) | implementado |
| Dock direito: Separations (Document Profile, CMYK, Setup) | implementado com dados reais de processo |
| Dock direito: Quality Control (Total Area Coverage, Limit) | implementado |
| Workspace cinza-claro, página branca, moldura laranja (Media Box) | implementado |
| Chips “Media Box” / “Trim Box” / “215,90 x 279,40 mm” | implementado |
| Barra de status (coordenadas, página, unidades, zoom) | implementado |
| Título `arquivo @ zoom% – ArtPlus` | implementado e atualizado em tempo real |
| Tema claro Fusion com cores extraídas da referência | implementado |

Evidência visual: `artplus_maximized3.png` (maximizado) e `artplus_ui_v3.png`.

## Diferenças assumidas (honestidade técnica)

1. **Document Profile**: o ArtPro+ mostra “ISO Coated v2 300% (ECI)” porque
   aplica um perfil padrão próprio. Lemos o `/OutputConditionIdentifier` real
   do PDF; se o arquivo não declara output intent, exibimos
   “(not specified)”. Não inventamos perfil.
2. **Separações spot**: listamos apenas CMYK. Cores especiais exigem leitura
   do color space `/Separation` + função de tint (milestone 26).
3. **Ícones**: aproximações desenhadas, não os ícones originais da Esko.
4. **Barra de status**: inclui coordenadas/unidades que a referência mantém
   em outra área; mantida por utilidade.
5. Zoom de “Fit Page” difere ~1% (104,5% vs 103,7%) por margens de ajuste.

## Regras mantidas

- Não copiar ícones, bitmaps, fontes ou textos proprietários da Esko.
- Todo painel visível reflete estado real do aplicativo.

## Menus (evidenced against ArtPro+ v24, 2026-11-03)

The installed ArtPro+ v24 was captured menu by menu:
`artpro24_menu_file.png`, `artpro24_menu_edit.png`, `artpro24_menu_object.png`,
`artpro24_menu_select2.png`, `artpro24_menu_prepress2.png`,
`artpro24_menu_view2.png`, `artpro24_submenu_zoom.png`.

ArtPlus menus now follow that structure (order, separators, shortcuts and
enabled/disabled state). Highlights:

- File: New, New Report Template…, Open…, Open Recent, Close, Close All, Save,
  Save As…, Export To, Convert To…, Action Lists, Launch Workflow…, Insert
  Content From PDF…, Place File…, Place Image Inside…, Pages, Page Boxes
  Setup…, Document Setup…, Print…, Exit (Ctrl+Q).
- Edit: Undo (Ctrl+Z), Redo (Ctrl+Shift+Z), Cut/Copy/Paste, paste variants,
  Delete (Backspace), Options… (Ctrl+K).
- Object: the real ArtPro+ list including Arrange, Transform… (Ctrl+Shift+M),
  Transform Again (Ctrl+D), Group/Ungroup, Lock/Unlock All, Hide/Show All,
  Path/Compound/Clipping Mask/Opacity Mask/Image/Text/Placed Art, Remove Color
  Profile, Recolor…, Shaper…, Spread/Choke…, Expand, Rasterize…, Clean/Clip/Fit,
  Convert to Placeholder….
- Select: Select All, Deselect All, Select Inverse, Select Next/Previous Object
  (Ctrl+Tab / Ctrl+Shift+Tab), Select…, Select Same Fill/Stroke Color, Select by
  Paths.
- Prepress: Add Barcode, Convert to Barcode, Add Dimensions, Add Bleed…, White
  Underprint…, Rich Black…, Ink Eater Areas, Screening…, Ink Coverage…, Convert
  Separations With, Thin Parts, Update Trap Layer, Trapping…, Warp, Preflight…,
  Manage Preflight Profiles…, Variable Data, Step & Repeat, Create Seamless
  Repetition…, Flexo Tools.
- View: Zoom (Fit in Window/Previous Ctrl+0, Actual Size/Previous Ctrl+1,
  Zoom In, Zoom Out, Zoom to Selection/Previous), Rotate View, Hide Page Boxes
  (Ctrl+B), Show Crosshair (Ctrl+R), Switch to Outline, Color Managed Preview,
  Open Next/Previous Inspector, Go To.

Implemented entries work; unimplemented ones are disabled. Submenus whose
contents were not captured contain a disabled "Not implemented in ArtPlus yet"
entry instead of invented commands. Window and Help remain ArtPlus-specific.

Caveats: the reference is **v24** (v250 may differ); Rotate View submenu content
and the Window/Help menus were not captured; "Save As Copy" is no longer in the
File menu (the byte-identical preservation API/test remains).
## Fidelity audit (2026-11-03)

The workspace now follows `ARTPRO_FIDELITY_RULES.md`. The right dock contains
only the confirmed ArtPro+ panels: **Document, Layers, Separations, Quality
Control** (plus Messages on the left). The previously invented **"Object"**
panel was removed; object editing is done through `Object → Transform…`
(Ctrl+Shift+M), a confirmed ArtPro+ command. See `UI_MAPPING_ARTPRO.md` for the
element-by-element evidence table and the list of pending confirmations
(Inspector, submenus, Window/Help menus, status bar, v250 comparison).