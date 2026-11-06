# PDFAccessLayer.dll (v24)

- Arquivo: Contents\Windows\PDFAccessLayer.dll, 2,52 MB
- Exports: 1.493 símbolos C++
- Imports: 29 DLLs

## Evidência de conteúdo (exports)

- Mensagens de documento: GReceiver<GADoc>,
  GReceiver<GADocDeletePagesMessage>, GReceiver<GADocDidChangePagesMessage>,
  GReceiver<GADocDidSaveMessage>, GReceiver<GADocInsertPagesMessage>,
  GReceiver<GADocDigSignatureMessage>.

## Relações comprovadas (import tables)

Document.dll, DocumentEdit.dll, DocumentMarks.dll, DocumentTrap.dll,
PDFObjectActions.dll, PDFObjectLayer.dll, PreflightProfileLayer.dll
importam PDFAccessLayer.

## Interpretação (grau de confiança: FORTE INDÍCIO)

É a camada central de acesso/manipulação de PDF e emite notificações de
alteração/salvamento — modelo orientado a mensagens (GReceiver). O ArtPlus pode
espelhar o conceito (documento notifica views) sem copiar código.

## Pendências

- Mapear a interface real (parâmetros) e o ciclo save/reload no Ghidra.