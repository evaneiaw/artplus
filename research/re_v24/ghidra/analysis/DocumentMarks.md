# DocumentMarks.dll (v24)

- Arquivo: Contents\Windows\DocumentMarks.dll, 4,62 MB
- Exports: 2.230 símbolos C++
- Imports: 39 DLLs (inclui PDFAccessLayer)
- O executável importa DocumentMarks.dll diretamente.

## Evidência de conteúdo (exports)

Classes de marca: BarcodeMark, BearerBar, CornerMark, CustomMark,
CutMark, DynamicMark; visitantes ConstVisitor<...> no namespace ArtDoc.
Tipos de tarefa: TaskMarkSets, TaskReporter.

## Interpretação (grau de confiança: FORTE INDÍCIO)

As marcas são objetos de documento tipados (não desenhos soltos), com
visitantes e conjuntos de tarefas. Isso orienta um modelo próprio de marcas:
tipos + parâmetros + geração para PDF.

## Pendências

- Enumerar todos os tipos de marca e seus parâmetros no Ghidra.