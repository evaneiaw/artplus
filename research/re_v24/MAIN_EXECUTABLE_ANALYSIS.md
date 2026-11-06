# MAIN_EXECUTABLE_ANALYSIS — ArtPro+.exe v24 (estático)

Arquivo: `C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe`
SHA-256: `9D33166126305F25C50349BB881F7D9ACDC996F3C0C9181D0BE61295D70B0F03`
Versão: 24.03.10017 — assinatura **HashMismatch** (ver V24_REFERENCE.md)

## 1. Cabeçalho PE

| Campo | Valor |
|---|---|
| Machine | 8664 (x64) |
| ImageBase | `0x140000000` |
| Entry point (RVA) | `0x10729DC` → VA `0x1410729DC` |
| Subsystem | 2 (Windows GUI) |
| Subsystem version | 6.00 |
| SizeOfImage | `0x208C000` |
| TimeDateStamp | 2024-03-12 14:23:28 |
| Certificates directory | RVA `0x1DD1400`, tamanho `0x2BB0` (assinatura presente, mas inválida) |
| Seções | `.text` 0x12D3000, `.rdata` 0x6EF000, `.data` 0x4EA000, `.pdata`, `.rsrc`, `.reloc`, `.textidx`, **`.fnp_dir`**, **`.fnp_mar`** |

As seções `.fnp_dir`/`.fnp_mar` são marcadores do **FlexNet Publisher**
(licenciamento). Isso explica `Flex.dll` existir só na v24 e é consistente com
o HashMismatch (binário licenciado/patchado). Grau de confiança: **FORTE
INDÍCIO**.

## 2. Dependências diretas

- **95 DLLs** no import table, **8.991 símbolos importados**.
- Módulos do produto: `Document.dll`, `DocumentMarks.dll`,
  `DocumentFlexoTools.dll`, `PDFConversionRules.dll`, `CurveApprox.dll`,
  `GeoMapper.dll`, `Snapping.dll`, `Cornering.dll`, `PredistortEngine.dll`,
  `LayoutEngine.dll`, `NDLModel*.dll`, `JEF*.dll`, `StudioCommon.dll`,
  `StudioDesignerCore*.dll`, `dali.dll`, `lib3d.dll`, `euclid.dll`, `Pack3D.dll`,
  `ArtSys.dll`, `ADT.dll`, `YAGL.dll`, `lcms.dll`, `libxml.dll`,
  `Localization.dll`, `Logging.dll`, `Preferences.dll`.
- Qt: `Qt6Core`, `Qt6Gui`, `Qt6Widgets`, `Qt6Network`, `Qt6PrintSupport`,
  `Qt6WebEngineCore` (41 imports), `Qt6WebEngineWidgets`, `Qt6WebChannel`.
- Sistema: `KERNEL32`, `USER32`, `GDI?` (via Qt), `ADVAPI32`, `SHELL32`,
  `ole32`, `OLEAUT32`, `COMDLG32`, `CRYPT32`, `bcrypt`, `WS2_32`, `WINHTTP`.

### Top importadores (por número de símbolos importados)

| DLL | Símbolos |
|---|---|
| NDLModelActionListTypes.dll | 2.188 |
| Qt6Widgets.dll | 1.808 |
| NDLModel.dll | 952 |
| Qt6Core.dll | 612 |
| Qt6Gui.dll | 572 |
| ExtraQt.dll | 446 |
| JEFView.dll | 270 |
| MSVCP140.dll | 260 |
| KERNEL32.dll | 149 |
| JEFUtils.dll | 148 |
| YAGL.dll | 127 |

Leitura: o executável é sobretudo **UI Qt + modelo de action list (NDL)**; a
lógica de documento/PDF fica nas DLLs especializadas.

## 3. Carregamento dinâmico

O exe importa `LoadLibraryA`, `LoadLibraryW`, `LoadLibraryExA` e
`GetProcAddress` (KERNEL32). Consequência comprovada: módulos podem ser
carregados em runtime e **não** aparecem no import table. Exemplo:
`PDFObjectActions.dll` (16.751 exports) **não** está entre as 95 dependências
diretas, embora `PreflightProfileLayer.dll` a importe.

## 4. Relações entre módulos (import tables)

Comprovado por `dumpbin /imports`:

- `Document.dll` importa: `PDFAccessLayer`, `PDFGeometryLayer`,
  `PDFObjectLayer`, `ColorManipulationLayer`, `CurveApprox`, `GeoMapper`,
  `FontLayer`, `FontNDSLayer`, `ArtSys`, `ADT`, `YAGL`, `CMS`, `tiff`,
  `libpng16`, `VariableEngine`, `BarCodeWrapper`, `EGXMPlib`.
- `PDFAccessLayer` é importada por `Document`, `DocumentEdit`,
  `DocumentMarks`, `DocumentTrap`, `PDFObjectActions`, `PDFObjectLayer`,
  `PreflightProfileLayer` → camada central de PDF.
- `PDFObjectActions` é importada por `PreflightProfileLayer` (entre as DLLs
  analisadas).
- O exe importa diretamente `DocumentMarks`, `Document`, `DocumentFlexoTools`,
  `PDFConversionRules`; não importa `DocumentTrap`, `StepAndRepeat`,
  `PreflightProfileLayer`, `PDFObjectActions` → carregamento dinâmico/plugin.

## 5. Classes e estruturas observadas

O log do Ghidra (RTTI/TypeDescriptor) revelou nomes de tipos:
`ArtProPlus::UploadFile::TransmissionStatus`, `Localization::Quantity`,
`GMGOpenColorModel::Exception`, `QString`, `QHeaderView`, `QPainter`, `QRect`,
`Parallel::Task`. Isso confirma C++ com RTTI e dá pistas de namespaces
(`ArtProPlus`, `Localization`, `Parallel`).

## 6. Fluxos funcionais — estado

Ainda **não** mapeados no nível de função: a análise automática do Ghidra está
em andamento (ver GHIDRA_PROJECT_AUDIT.md). O que se pode afirmar com evidência:

- `PDFAccessLayer` emite mensagens `GADocDidChangePagesMessage`,
  `GADocDidSaveMessage`, `GADocInsertPagesMessage`,
  `GADocDeletePagesMessage`, `GADocDigSignatureMessage` → modelo de documento
  orientado a eventos (abrir/alterar/salvar).
- `DocumentTrap` expõe `Module::Create`/`kModuleID` e operações de trapa →
  trapping é um **módulo de documento** registrável.
- `StepAndRepeat` opera sobre `SheetIterator`/`RunList`/`GridParameters` e
  converte para PDF via `DocumentMarks`.

## 7. Limitações

- Contagem de funções e mapa de chamadas: **pendentes** (análise ativa).
- Nenhuma função foi renomeada; nada foi copiado.
- A assinatura inválida impede tratar o binário como artefato oficial íntegro.
