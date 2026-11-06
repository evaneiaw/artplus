# Painel Audit — ArtPlus Lateral Panels

## Technical Audit — 2026-11-04

This audit examines the four lateral panels in the ArtPlus main window:
Document, Layers, Separations, and Quality Control. The analysis is based on
code inspection of `src/ui/src/MainWindow.cpp` and does not require running the
application (GUI initialization is blocked by Windows DLL initialization errors
in the current execution context).

---

## 1. Document Panel

### Classification: **B — Parcialmente Funcional**

### Classe responsável
- `MainWindow::m_documentPanel` (`CollapsiblePanel`)

### Arquivos
- `src/ui/src/MainWindow.cpp` linhas 631-677 (criação)
- `src/ui/include/artplus/ui/Panel.h` — base `CollapsiblePanel`

### Widgets e controles
- `QToolButton` Crosshair — conecta via `toggled` a `m_view->setCrosshairEnabled()`
- `QToolButton` Page Boxes — conecta via `toggled` a `m_view->setPageBoxesVisible()`
- Nenhum display de metadados do documento (contagem de páginas, dimensões, etc.)

### Dados que alimentam o painel
- Botões conectados à visualização (`m_view`), não a dados do documento
- Em `updatePanels()` (linha 1732): apenas `m_documentPanel->setEnabled(true)`
- Nenhum dado de documento (page count, size, metadata) é populado neste painel

### Conexões de sinais e slots
- `crosshair->toggled` → `m_view->setCrosshairEnabled(on)` (linha 648-652)
- `boxes->toggled` → `m_view->setPageBoxesVisible(on)` (linha 669-675)

### Conexão com documento PDF ativo
- Nenhuma. O painel não recebe atualizações baseadas no documento aberto.
- Em `updatePanels()`, o painel é apenas habilitado/desabilitado (linha 1732)

### Atualização ao trocar de documento / abrir/fechar
- Apenas habilitado/desabilitado (`setEnabled(true)` na linha 1732)
- Não há populacão de informações do documento (page count, dimensions, metadata)

### Verificação funcional
- ✅ Crosshair toggle funciona para ativar/desativar cursor crosshair
- ✅ Page Boxes toggle funciona para mostrar/ocultar quadros de página
- ❌ Nenhuma informação do documento é exibida (pages, size, metadata)
- ❌ Não atualiza quando documento é trocado

### Evidências
- `src/ui/src/MainWindow.cpp:631-677` — criação do painel
- `src/ui/src/MainWindow.cpp:1732` — `m_documentPanel->setEnabled(true)`
- `src/ui/src/MainWindow.cpp:648-652,669-675` — conexões de botão

---

## 2. Layers Panel

### Classification: **C — Somente Visual**

### Classe responsável
- `MainWindow::m_layersPanel` (`CollapsiblePanel`)

### Arquivos
- `src/ui/src/MainWindow.cpp` linhas 680-732 (criação)
- `src/ui/include/artplus/ui/Panel.h` — base `CollapsiblePanel`

### Widgets e controles
- Rótulo "Layer 1" (linha 701) — texto fixo
- Rótulo `m_layerInfo` — mostra "no document" ou contagens de objetos (texto fixo "Layer 1" sempre)
- `QListWidget m_objectList` — lista de objetos (conecta ao `onObjectSelectionChanged`)
- Botões: Add layer, Remove layer, Layer order, Layer options (ferramentas sem funcionalidade conectada)

### Dados que alimentam o painel
- `m_layerName` definido como `QStringLiteral("Layer 1")` (linha 1735) — sempre "Layer 1"
- `m_layerInfo` (linhas 1736-1747): quando há documento e snapshot, mostra totais:
  - total de objetos
  - texto
  - paths (curtas)
  - imagens
- Caso contrário: "no document"

### Conexões de sinais e slots
- `m_objectList->itemSelectionChanged` → `MainWindow::onObjectSelectionChanged` (linha 718-719)
- Ferramentas "Add layer", "Remove layer", "Layer order", "Layer options" — sem conexões de sinal visíveis

### Conexão com documento PDF ativo
- Atualizado em `updatePanels()` (linhas 1725-1747)
- Habilitado/desabilitado via `m_layersPanel->setEnabled(hasDoc)` (linha 1726)
- Informações baseadas em `m_snapshot->objectCount()` etc.

### Atualização ao trocar de documento / abrir/fechar
- Atualizado via `updatePanels()` chamada de `openDocument()` (linha 895)
- `m_layerName` sempre "Layer 1" (não varia)
- `m_layerInfo` mostra contagens do snapshot

### Verificação funcional
- ❌ "Layer 1" permanece fixo — não reflete camadas reais do PDF
- ❌ Botões Add/Remove layer não têm conexões de sinal visíveis
- ❌ Lista de objetos conecta ao selectionChanged, mas não mostra dados de camadas
- ❌ Informações de contagem de objetos são exibidas quando há documento

### Evidências
- `src/ui/src/MainWindow.cpp:701` — `m_layerName = makeLabel(QStringLiteral("Layer 1"), ...)`
- `src/ui/src/MainWindow.cpp:1735` — `m_layerName->setText(QStringLiteral("Layer 1"))`
- `src/ui/src/MainWindow.cpp:1736-1747` — contagens de snapshot no m_layerInfo
- `src/ui/src/MainWindow.cpp:718-719` — conexão selectionChanged

---

## 3. Separations Panel

### Classification: **B — Parcialmente Funcional**

### Classe responsável
- `MainWindow::m_separationsPanel` (`CollapsiblePanel`)

### Arquivos
- `src/ui/src/MainWindow.cpp` linhas 736-814 (criação)
- `src/ui/include/artplus/ui/Panel.h` — base `CollapsiblePanel`

### Widgets e controles
- Rótulo "Document Profile:" com valor `m_profileValue` — mostra "(not specified)" ou nome do perfil
- 4 linhas de tintas CMYK hardcodadas com:
  - Ciano (0x00, 0xAE, 0xEF), número 1
  - Magenta (0xEC, 0x00, 0x8C), número 2
  - Amarelo (0xFF, 0xF2, 0x00), número 3
  - Preto (0x23, 0x1F, 0x20), número 4
  - Cada linha tem: ícone de olho, amostra de cor, nome, número, checkbox
- Ferramentas: Print setup, Add separation, Remove separation, Separation options

### Dados que alimentam o painel
- `m_profileValue` (linhas 744-757): mostra `m_doc->outputIntentName()` quando há documento, senão "(not specified)"
- 4 tintas CMYK — **hardcodadas**, não extraídas do PDF
- Checkboxes por tinta — existirem mas sem conexão clara com dados do PDF

### Conexões de sinais e slots
- Botões de ferramenta: Print setup, Add separation, Remove separation, Separation options
- Sem conexões de sinal visíveis para as operações de add/remove

### Conexão com documento PDF ativo
- Habilitado/desabilitado via `m_separationsPanel->setEnabled(hasDoc)` (linha 1728)
- Perfil atualizado via `m_doc->outputIntentName()` (linha 1748-1757)
- Tintas permanecem hardcodadas independentemente do documento

### Atualização ao trocar de documento / abrir/fechar
- Perfil atualizado com `m_doc->outputIntentName()` (linha 1749)
- Tintas permanecem as mesmas (hardcodadas)
- Habilitado/desabilitado pelo status do documento

### Verificação funcional
- ✅ Perfil do documento exibe nome real quando PDF tem output intent, senão "(not specified)"
- ❌ 4 tintas CMYK são hardcodadas — não refletem cores reais do PDF
- ❌ Checkboxes de tinta não têm funcionalidade conectada demonstrada
- ❌ Botões Add/Remove separation não têm conexões de sinal aparentes
- ✅ Painel é habilitado/desabilitado conforme documento

### Evidências
- `src/ui/src/MainWindow.cpp:744-757` — perfil do documento
- `src/ui/src/MainWindow.cpp:766-801` — tintas CMYK hardcodadas
- `src/ui/src/MainWindow.cpp:1748-1757` — update do perfil no updatePanels
- `src/ui/src/MainWindow.cpp:1728` — `m_separationsPanel->setEnabled(hasDoc)`

---

## 4. Quality Control Panel

### Classification: **D — Não Implementado**

### Classe responsável
- `MainWindow::m_qualityPanel` (`CollapsiblePanel`)

### Arquivos
- `src/ui/src/MainWindow.cpp` linhas 817-841 (criação)
- `src/ui/include/artplus/ui/Panel.h` — base `CollapsiblePanel`

### Widgets e controles
- `QComboBox` com item "Total Area Coverage" (única opção)
- `QSpinBox m_tacLimit` — range 0-400%, valor padrão 300%

### Dados que alimentam o painel
- Nenhum dado real de qualidade
- Limite fixo de 300%

### Conexões de sinais e slots
- Nenhuma visível

### Conexão com documento PDF ativo
- Habilitado/desabilitado via `updatePanels()`, mas a função `updatePanels()` **não menciona** `m_qualityPanel` (verificar linhas 1687-1758)
- O painel existe mas não é atualizado nem referenciado na atualização de painéis

### Atualização ao trocar de documento / abrir/fechar
- **Não atualizado**. A função `updatePanels()` (linhas 1687-1758) não contém referência alguma a `m_qualityPanel`.
- O painel é criado mas nunca recebendo dados ou atualizações baseadas no documento.

### Verificação funcional
- ❌ Nenhuma funcionalidade de quality control real
- ❌ Nenhuma integração com módulo Preflight
- ❌ Nenhum problema identificado ou exibido do documento
- ❌ Spin box de limite existe mas não faz nada conectado
- ❌ Combo box tem apenas "Total Area Coverage" — única opção, sem ação

### Evidências
- `src/ui/src/MainWindow.cpp:817-841` — criação do painel
- `src/ui/src/MainWindow.cpp:1687-1758` — `updatePanels()` — **nenhuma referência a m_qualityPanel**
- O painel é criado mas "esquecido" na atualização

---

## 5. Resumo e Recomendações

### Classificação Final

| Painel | Classificação | Status |
|--------|---------------|--------|
| Document | B — Parcialmente Funcional | Tem funcionalidade conectada (crosshair, page boxes) mas não exibe metadados do documento |
| Layers | C — Somente Visual | Interface existe com controles, mas nenhuma operação real sobre camadas PDF |
| Separations | B — Parcialmente Funcional | Mostra perfil do documento; tintas são hardcodadas; UI framework existe |
| Quality Control | D — Não Implementado | Painel existe mas sem nenhuma funcionalidade real conectada |

### Funcionalidades Comprovadas

1. **Document panel**: Crosshair toggle, Page Boxes visibility toggle
2. **Separations panel**: Document profile display (output intent name), CMYK color swatches (hardcoded)

### Funcionalidades Apenas Visuais

1. **Document panel**: Nenhum display de metadados (pages, size, title, author)
2. **Layers panel**: "Layer 1" fixo, botões Add/Remove sem conexão, lista de objetos sem dados de camada
3. **Separations panel**: 4 tintas CMYK hardcodadas, checkboxes sem ação conectada
4. **Quality control panel**: Spin box e combo box sem ação conectada

### Funcionalidades Ausentes

1. **Document panel**: Page count, dimensions, title, author, metadata display
2. **Layers panel**: Gestão real de camadas (add/remove/rename), conexão com estrutura de camadas do PDF
3. **Separations panel**: Tintas reais extraídas do PDF, operações de add/remove separação que persistam
4. **Quality control panel**: Qualquer verificação real de qualidade, integração Preflight, exibição de problemas

### Arquivos Responsáveis

- `src/ui/src/MainWindow.cpp` — criação e atualização de todos os painéis
- `src/ui/include/artplus/ui/Panel.h` — base CollapsiblePanel
- `src/pdf/src/PdfDocument.cpp` — `outputIntentName()` (usado pelo painel de separations)

### Testes Necessários

1. Abrir PDFs com output intent diferente e verificar se o perfil do documento atualiza
2. Verificar se as tintas do painel separations refletem as cores do PDF
3. Testar operações dos botões Document (crosshair, page boxes)
4. Verificar se a lista de objetos no painel Layers reflete a seleção real

### Prioridade Recomendada de Implementação

1. **Alta**: Quality Control panel — precisa de implementação completa ou remoção
2. **Média**: Layers panel — adicionar gestão real de camadas e conexão com estrutura do PDF
3. **Média**: Document panel — adicionar display de metadados (pages, size, metadata)
4. **Baixa**: Separations panel — conectar tintas ao PDF e adicionar operações de add/remove

---

## Próximos Passos

1. Este é um relatório de auditoria apenas — nenhuma alteração funcional foi feita
2. O `ARTPLUS_STATUS.md` pode ser atualizado com o resultado desta auditoria
3. As prioridades acima orientam o próximo desenvolvimento de funcionalidades
4. Para validar as observações, seria necessário executar o aplicativo com sucesso

---
*Relatório gerado a partir de inspeção de código estática. A GUI do ArtPlus apresenta erros de inicialização no contexto de execução atual (erro 0xc0000142 relacionado a dependências Windows GUI), o que impede a verificação visual direta. No entanto, a estrutura de código foi completamente analisada.*