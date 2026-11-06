# PDFObjectActions.dll (v24)

- Arquivo: Contents\Windows\PDFObjectActions.dll, 7,63 MB
- Exports: 16.751 símbolos C++ (mangled)
- Imports: 36 DLLs, incluindo PDFAccessLayer.dll
- Relação com o exe: **não** está no import table do executável; é carregado
  dinamicamente (o exe importa LoadLibrary/GetProcAddress) e/ou por
  PreflightProfileLayer.dll (que o importa).

## Evidência de conteúdo (exports)

- Visitantes/ações de objeto gráfico: GOVDefaultBleedAroundContour,
  GOVDefaultAdd, GOVDefaultChange, GOVDefaultCheck, GOVAddPages,
  GSet<GOOCGroup> (grupos de conteúdo opcional/OCG), AddSourceInfo.
- Namespace aparente: GOV* (graphic object visitor) + GADoc.

## Interpretação (grau de confiança: FORTE INDÍCIO)

A DLL concentra **ações sobre objetos PDF** (adicionar páginas, bleed em
contorno, grupos OCG), separadas do modelo de documento (Document.dll) e do
acesso PDF (PDFAccessLayer.dll). Não é possível, só pelos nomes, afirmar o
algoritmo de cada ação.

## Pendências

- Importar no Ghidra e mapear XREFs de GOVAddPages, ações de bleed e OCG.
- Confirmar a interface com PDFAccessLayer e Document.