# StepAndRepeat.dll (v24)

- Arquivo: Contents\Windows\StepAndRepeat.dll, 0,30 MB
- Exports: 29 símbolos C++
- Imports: 22 DLLs

## Evidência de conteúdo (exports)

PDFParameters, SNRException, SheetIterator (Iterator2Pdf,
CheckIteratorBoxes), SNR_GridParameters, SheetParameters,
InputRunList, InputRunListGroupParams, SheetSide,
Jdf2Iterator, MIS2Iterator, Template2Iterator,
ConvertMISXMLToRunlist, ConvertPdfList2RunList, ConvertXmlList2RunList,
getNumParams, getParam1.

## Interpretação (grau de confiança: FORTE INDÍCIO)

Step & Repeat é dirigido por **run lists** e **grid parameters** (não apenas
"copiar N vezes"): entradas JDF/MIS/XML/PDF, sheet side, template, e conversão
para PDF (Iterator2Pdf) usando DocumentMarks (TaskMarkSets,
TaskReporter). A UI Prepress > Step & Repeat corresponde a esse módulo.

## Pendências

- Mapear SNR_GridParameters (campos) no Ghidra.
- Entender SheetSide e a relação com marcas.