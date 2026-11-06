# MAIN_EXECUTABLE_FUNCTION_MAP — mapa provisório

Status: **provisório**. A análise automática do Ghidra para `ArtPro+.exe` (v24)
está em andamento; ainda não há contagem de funções nem mapa de chamadas.
Este documento registra o que já está comprovado por import table e por
símbolos RTTI, sem inventar nomes.

## 1. Camadas comprovadas (import table)

| Camada | Evidência |
|---|---|
| UI Qt | 1.808 imports de `Qt6Widgets`, 612 de `Qt6Core`, 572 de `Qt6Gui`, 446 de `ExtraQt`; RTTI `QString`, `QHeaderView`, `QPainter`, `QRect` |
| Modelo de action list | 2.188 imports de `NDLModelActionListTypes`, 952 de `NDLModel`, 82 de `NDLTypes` |
| Documento | imports de `Document.dll`, `DocumentMarks.dll`, `DocumentFlexoTools.dll`, `PDFConversionRules.dll` |
| Geometria/curvas | `CurveApprox.dll`, `GeoMapper.dll`, `YAGL.dll`, `euclid.dll`, `lib3d.dll` |
| Cor | `lcms.dll`, `GMGOpenColorModel.dll` |
| 3D/warp | `Pack3D.dll`, `PredistortEngine.dll`, `dali.dll` |
| Licenciamento | seções `.fnp_dir`/`.fnp_mar`, `Flex.dll` (apenas v24) |
| Carga dinâmica | `LoadLibraryA/W/ExA`, `GetProcAddress` |

## 2. Módulos carregados dinamicamente (não no import table)

Comprovado por ausência + presença em imports de outras DLLs:

- `PDFObjectActions.dll` — importada por `PreflightProfileLayer`; não direta no
  exe.
- `PDFAccessLayer.dll` — via `Document.dll` e outras.
- `CCurve.dll`, `DocumentTrap.dll`, `StepAndRepeat.dll`,
  `PreflightProfileLayer.dll` — não aparecem no import table do exe.

## 3. Política de nomes

- Funções sem símbolo serão registradas como `FUN_<endereço>` (ex.:
  `FUN_1410729DC`) e **não** renomeadas especulativamente.
- Nomes só serão atribuídos quando houver evidência (RTTI, exports, string
  inequívoca, comportamento observado).

## 4. O que falta

1. Concluir a análise automática.
2. Extrair `FunctionManager`: total, definidas, indefinidas, `FUN_*`.
3. Mapear XREFs de `LoadLibrary*`/`GetProcAddress` para descobrir os caminhos
   de carga dos módulos dinâmicos.
4. Mapear chamadas de UI → comandos (Qt signals/slots e QAction).
5. Mapear abertura/salvamento (`PDFAccessLayer` messages) e edição
   (`DocumentEdit`).
6. Comparar com `14_FUNCTION_CATALOG.md` da v25.

Nenhuma conclusão de fluxo deve ser publicada antes de 2–5.
