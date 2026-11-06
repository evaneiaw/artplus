# DocumentTrap.dll (v24)

- Arquivo: Contents\Windows\DocumentTrap.dll, 0,77 MB
- Exports: 25 símbolos C++
- Imports: 24 DLLs (inclui PDFAccessLayer)

## Evidência de conteúdo (exports)

Module::Create / Module::GetName / kModuleID (padrão de módulo de
documento), AddTrap, AutomaticTrap, CanTrap, RemoveTrap,
CanRemoveTrap, InitializeSession, SessionParameters,
AddBleed, GetTrapping/SetTrapping, GetReverseTrapping/
SetReverseTrapping, IsAutomaticTrapLayerInSync, GetLayerData.

Tipos observados nas assinaturas: CollapsionSelection, PureTrapParameters,
PullbackParameters, ActionSelector, Trapping, ReverseTrapping,
TrapCandidate, SessionParameters, Layer, Content.

## Interpretação (grau de confiança: FORTE INDÍCIO)

Modelo de trapping: sessão, candidatos, trapa automática, trapa/ reverso,
pullback/collapsion, sincronização de camada automática. A interface pública
dessas operações está confirmada; o algoritmo interno não.

## Pendências

- Decompilar AutomaticTrap e CanTrap para entender entradas e critérios.
- Relacionar CollapsionSelection/PullbackParameters a parâmetros de UI.