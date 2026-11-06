# PreflightProfileLayer.dll (v24)

- Arquivo: Contents\Windows\PreflightProfileLayer.dll, 2,28 MB
- Exports: 2.423 símbolos C++
- Imports: 17 DLLs (inclui PDFObjectActions, PDFAccessLayer)
- O executável importa PreflightProfileEditor.exe separado.

## Evidência de conteúdo (exports)

Família Gpet* (preflight checks): GpetActionListDatabaseFilter,
GpetAnnotationInPrintableArea, GpetAnnotationsHyperlink,
GpetAnnotationsOtherType, GpetRestrictionsToolNotification.

## Interpretação (grau de confiança: FORTE INDÍCIO)

Perfis de preflight são compostos por verificações independentes (Gpet*) e
produzem notificações; há integração com action lists e banco de perfis
(ProfileAndActionListDatabases.dll).

## Pendências

- Listar todas as verificações Gpet* e seus parâmetros.
- Entender o formato de perfil e o resultado/classificação de problemas.