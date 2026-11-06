# V24_REFERENCE — instalação de referência operacional

Data do registro: 2026-11-03
Versão de referência principal: **ArtPro+ 24.03.10017** (v240)
Versão complementar: **ArtPro+ 25.03.116** (v250)

## Identidade do executável v24

| Campo | Valor |
|---|---|
| Caminho | `C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe` |
| FileVersion | 24.03.10017 |
| ProductVersion | 24.03.10017 |
| Descrição | ArtPro+ 24.03 |
| Empresa | Esko |
| Tamanho | 31.276.976 bytes |
| LastWriteTime | 2024-03-19 17:12:30 |
| SHA-256 | `9D33166126305F25C50349BB881F7D9ACDC996F3C0C9181D0BE61295D70B0F03` |
| Assinatura Authenticode | **HashMismatch** — assinada por `CN=Esko Software, O=Esko Software, S=Oost-Vlaanderen, C=BE` |

### Aviso de integridade

A assinatura existe, mas o hash do arquivo **não corresponde** ao assinado:
o binário foi modificado após a assinatura (patch/licenciamento) ou está
corrompido. Consequências:

- A versão e o comportamento observado continuam válidos como referência
  funcional (o programa executa e foi capturado).
- **Não** use este binário como prova de integridade oficial da Esko.
- Achados de engenharia reversa devem ser cruzados com a v25 (assinatura válida)
  e com o comportamento observado.

## Toolchain e formato

- PE64, MSVC 19.38.33134 (VS 2022 17.8), runtime C/C++ dinâmico.
- Qt 6.3.1.0; zlib; OpenGL; Direct3D 9.
- 305 módulos (DLL/EXE) no diretório `Contents\Windows`, 442,7 MB no total.
  A v24 carrega Qt6WebEngineCore.dll (121 MB) e Qt6 Qml/Quick — ausentes na v25.

## Módulos relevantes para as próximas missões

| Domínio | Módulos v24 (MB) |
|---|---|
| PDF | `PDFAccessLayer.dll` (2,52), `PDFObjectActions.dll` (7,63), `PDFObjectLayer.dll` (2,25), `PDFObjectToolsLayer.dll` (1,11), `PDFObjectActionViewLayer.dll` (3,13), `PDFGeometryLayer.dll` (0,08), `ArtProPDF.dll` (0,84), `PDFRenderingLayer.dll` (0,43) |
| Geometria | `CCurve.dll` (0,68), `CurveApprox.dll` (0,09), `PDFGeometryLayer.dll` (0,08), `ARDLib.dll` (6,29), `GeoMapper.dll` (0,69) |
| Cor | `ICC.dll` (0,23), `LibICCProfile.dll` (0,05), `ColorManipulationLayer.dll` (0,32), `GMGOpenColor.dll` (0,81) |
| Trapping | `DocumentTrap.dll` (0,77) |
| Step & Repeat | `StepAndRepeat.dll` (0,30), `StepAndRepeatCAD.dll` (0,33) |
| Marks | `DocumentMarks.dll` (4,62) |
| Preflight | `PreflightProfileLayer.dll` (2,28), `PreflightProfileEditor.exe` (0,11) |
| Automação | `DocumentAutomationEngine.dll` (0,78), `NDLModelActionList.dll` (2,66) |
| Documento | `Document.dll` (4,22), `DocumentEdit.dll` (1,49), `DocumentLayout.dll` (0,66), `DocumentTypesetting.dll` (3,37) |

Inventário completo: `research/re_v24/modules_v24.csv` (e `modules_v25.csv`).

## Análise estática em andamento

- Projeto Ghidra: `research/re_v24/ghidra/ArtProV24`
- Log: `research/re_v24/ghidra_v24_import.log`
- Comando: `analyzeHeadless … -import ArtPro+.exe -overwrite -analysisTimeoutPerFile 2400`
- Status inicial: importação/análise em background (executável de ~31 MB).

Próximos alvos, em ordem: `PDFObjectActions.dll`, `PDFAccessLayer.dll`,
`CCurve.dll`, `DocumentTrap.dll`, `StepAndRepeat.dll`, `DocumentMarks.dll`,
`PreflightProfileLayer.dll`.

## Relatórios desta missão

- `GHIDRA_PROJECT_AUDIT.md` — estado real do projeto (análise automática ainda ativa).
- `MAIN_EXECUTABLE_ANALYSIS.md` — PE, 95 dependências, 8.991 imports, carga dinâmica.
- `MAIN_EXECUTABLE_FUNCTION_MAP.md` — mapa provisório (contagem de funções pendente).
- `ghidra/analysis/*.md` — 7 DLLs prioritárias (exports/imports/relações).
- Evidência estática bruta: `static/` (dumpbin headers/imports/exports/dependents).

## Regras

- v24 é a referência **operacional e visual** principal.
- v25 permanece referência **técnica complementar** (documentação já produzida).
- Não copiar código/pseudocódigo proprietário nem depender de DLLs da Esko.
- Toda interface nova deve ser confirmada na v24 antes de implementar
  (`ARTPRO_FIDELITY_RULES.md`).
