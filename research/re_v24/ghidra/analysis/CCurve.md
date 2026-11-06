# CCurve.dll (v24)

- Arquivo: Contents\Windows\CCurve.dll, 0,68 MB
- Exports: 268 símbolos C++
- Imports: 20 DLLs

## Evidência de conteúdo (exports)

CCurveImportInfo, CCurve_TCombiner, CCurve_TContext, CCurve_TResource,
CCurve_TToneRangeCorrectionParameters (EToneRangeCorrectionType,
minimum/maximum action type), construtores com coordenadas NNNN.

## Interpretação (grau de confiança: FORTE INDÍCIO, com ressalva)

Apesar do nome "Curve", os símbolos indicam **curvas tonais/recursos de
correção de tom** (tone range correction), não Bézier/geometria. Isso confirma
a regra do projeto: não inferir função pelo nome. A geometria de paths está em
PDFGeometryLayer.dll/ARDLib.dll/CurveApprox.dll.

## Pendências

- Confirmar a estrutura TCurve e o algorítmo de tone mapping no Ghidra.