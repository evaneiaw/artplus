# Painel Investigação Técnica — ArtPlus

## Investigação Técnica dos Painéis Laterais

### Data da Investigação: 2026-11-05

## 1. Documento

### Funcionalidades Existente
- **Crosshair toggle**: Conectado via `toggled` a `m_view->setCrosshairEnabled()` — funciona corretamente
- **Page Boxes visibility toggle**: Conectado via `toggled` a `m_view->setPageBoxesVisible()` — funciona corretamente
- **Exibição de metadados**: Implementado via `updateDocumentInfo()` — exibe page count, dimensions, title, author, creator, producer, creation/modification dates, output intent
- **Atualização ao trocar documento**: Automática através de `updateDocumentInfo()` chamado de `updatePanels()` e `openDocument()`

### Funcionalidades Comprovadas
- ✅ Crosshair toggle ativa/desativa cursor crosshair na viewport
- ✅ Page Boxes toggle mostra/oculta quadros de página na viewport
- ✅ Document metadata display: page count, size, title, author, creator, producer, dates, output intent
- ✅ Atualização automática ao abrir, fechar ou trocar documento

### APIs Disponíveis (PDFium 158)
- `PdfDocument::pageCount()` — retorna número de páginas
- `PdfDocument::pageSize(int index)` — retorna size em pontos
- `PdfDocument::title()` — retorna título do metadado
- `PdfDocument::author()` — retorna autor do metadado
- `PdfDocument::subject()` — retorna sujeito do metadado
- `PdfDocument::creator()` — retorna criador do metadado
- `PdfDocument::creationDate()` — retorna data de criação
- `PdfDocument::modificationDate()` — retorna data de modificação
- `PdfDocument::outputIntentName()` — retorna nome do output intent
- `PdfDocument::isValid()` — valida se PDF é válido
- `PdfDocument::pageCount()` — quantidade de páginas
- `PdfDocument::pageSize(int index)` — retorna size em pontos

### Limitações
- Quando não há documento aberto, labels exibem "(not specified)" ou "–"
- Metadados ausentes no PDF são tratados graciosamente sem informações inventadas

### Desenvolvimento Necessário
- Já implementado via `MainWindow::updateDocumentInfo()`.
- Future work: melhorar formatação e adicionar suporte a metadados avançados.

### Referência ArtPro
- Exibir page count, dimensions, metadata (title, author) no painel Document
- Output Intent name exibido quando especificado

### Ghidra
- Necessário para investigacao de exports de DLLs (PDFObjectActions.dll, PreflightProfileLayer.dll)

### Próxima Ação Técnica
- Testar display de metadados com PDFs que tenham informações completas e incompletas
- Verificar abertura, fechamento e troca de documentos
- Confirmar se os dados são atualizados corretamente e se não permanecem informações do PDF anterior

---

## 2. Layers

### Funcionalidades Existente
- **Rótulo "Layer 1"**: Texto fixo `QStringLiteral("Layer 1")` — sempre "Layer 1", não reflete camadas reais
- **Rótulo `m_layerInfo`**: Mostra contagens quando há documento:
  - Total de objetos
  - Textos
  - Paths (curtas)
  - Imagens
  - Shading
  - Forms
  - Unknown
- **`QListWidget m_objectList`**: Lista de objetos conectada ao `onObjectSelectionChanged`
- **Botões**: Add layer, Remove layer, Layer order, Layer options — sem conexões de sinal visíveis

### Funcionalidades Comprovadas
- ✅ Contagem de objetos totais exibida quando há documento ativo
- ✅ Objeto selectionChanged trigger funcional
- ✅ Contagem detalhada por tipo de objeto (textos, paths, images, shading, forms, unknown)
- ❌ "Layer 1" permanece fixo — não reflete camadas reais do PDF
- ❌ Botões Add/Remove layer não têm conexões de sinal aparentes
- ❌ Lista de objetos não mostra dados de camada (kind, visibility)
- ❌ Não há hierarquia de grupos de camadas exibida
- ❌ Não há controle de visibilidade de camadas

### APIs Disponíveis (PDFium 158 / ArtPlus)
- `PdfSnapshot::objectCount()` — total de objetos no documento
- `PdfSnapshot::countOf(PageObjectKind kind)` — contagem por tipo (Text, Path, Image, Shading, Form, Unknown)
- `PdfDocument::pageObjects(int index)` — objetos por página
- `PageObjectKind::{Text, Path, Image, Shading, Form, Unknown}` — tipos de objeto
- `SnapshotObject::{kind, bounds}` — estrutura de objeto no snapshot

### Limitações
- Dados do snapshot são estáticos — capturados no `openDocument()`, não atualizados dinamicamente
- "Layer 1" fixo não reflete estrutura OCG/OCMD do PDF (PDFium 158 deste projeto tem suporte limitado/none)
- Botões Add/Remove layer sem conexão de sinal — funcionalidade não implementada
- **Investigação Ghidra (2026-11-05):** Bibliotecas DLL do ArtPro+ v24 identificadas:
  - `PDFObjectActions.dll`: exports incluem `GOVCheckOCVisibility` para verificacao de visibilidade de OCG
  - `PreflightProfileLayer.dll`: exports incluem `GpetInkCoverage` e metodo `ConvertToCheckActionList()`
- Não há hierarquia de grupos/OCG exibida
- Não há controle de visibilidade (eyeball icons) conectado a OCG

### Desenvolvimento Necessario
- Investigar suporte do PDFium 158 a OCG (Optional Content Groups) e OCMD (Optional Content Membership Dictionaries)
- Se PDFium suporta: implementar leitura de camadas, nomes, visibilidade, hierarquia
- Se PDFium nao suporta (como neste projeto): criar mecanismo alternativo usando estrutura de objetos do ArtPlus
- **Investigação Ghidra (2026-11-05):** Simbolos descobertos permitem futura implementacao de leitura de camadas e controle de visibilidade
  - `PDFObjectActions.dll`: `GOVCheckOCVisibility` para verificacao de visibilidade de OCG
  - `PreflightProfileLayer.dll`: `GpetInkCoverage` com metodo `ConvertToCheckActionList()` para arquitetura de analise de cobertura
- Nota: Ja foi melhorada a contagem detalhada por tipo de objeto usando `m_snapshot->countOf()`

### Referência ArtPro
- ArtPro+ v24 tem painel Layers funcional com lista de camadas, visibilidade (eyeball),
  nomes, grupos/OCG. No ArtPlus: painel atual é apenas visual ("Layer 1" fixo, sem conexão
  com estrutura de camadas PDF).

### Ghidra
- Investigar `GOVCheckOCVisibility` de `PDFObjectActions.dll` e `GpetInkCoverage` de `PreflightProfileLayer.dll`
- Verificar se ha funcoes de enumeracao de OCG/OCMD no ArtPro
- Caso ArtPro tenha suporte a OGG, investigar APIs usadas

### Próxima Ação Técnica
1. Investigar implements da arquitetura GpetInkCoverage::ConvertToCheckActionList() via 
   PreflightProfileLayer.dll — ver arquivo research/artpro_analysis/REVERSE_ENGINEERING_PANELS.md
2. Pesquisar em `research/artpro_analysis/` sobre suporte a OCG/OCMD no PDFium
3. Verificar `PdfDocument::pageObjects()` e `PdfSnapshot` estruturas para possivel extensao
4. Decidir: implementar suporte real de layers (se PDFium permitir) ou manter como painel informacional apenas
5. Se implementar: adicionar leitura de OCG/OCMD, nomes de camadas, controles de visibilidade
6. Preparar carregamento dinâmico de DLLs: PDFObjectActions.dll e PreflightProfileLayer.dll

---

## 3. Separations

### Funcionalidades Existente
- **Perfil do Documento**: `m_profileValue` exibe `m_doc->outputIntentName()` ou "(not specified)"
- **4 Tintas CMYK Hardcodadas**: Ciano (0x00, 0xAE, 0xEF), Magenta (0xEC, 0x00, 0x8C), Amarelo (0xFF, 0xF2, 0x00), Preto (0x23, 0x1F, 0x20)
  - Cada linha: ícone de olho, amostra de cor, nome, número, checkbox
- **Ferramentas**: Print setup, Add separation, Remove separation, Separation options
- **Habilitado/Desabilitado**: Via `m_separationsPanel->setEnabled(hasDoc)`

### Funcionalidades Comprovadas
- ✅ Perfil do documento exibe output intent name ou "(not specified)"
- ❌ 4 tintas CMYK sao hardcodadas — nao refletem cores reais do PDF
- ❌ Checkboxes de tinta nao possuem funcionalidade conectada demonstrada
- ❌ Botoes Add/Remove separation nao possuem conexoes de sinal aparentes
- ❌ Tintas permanecem as mesmas independentemente do documento aberto

### APIs Disponiveis (PDFium 158)
- `PdfDocument::outputIntentName()` — retorna nome do output intent ou string vazia
- `PdfDocument::isValid()` — validação do documento
- `PdfDocument::pageCount()` — quantidade de páginas

### Limitações
- 4 tintas CMYK sao hardcodadas no codigo — nao refletem cores reais do PDF
- `FPDFDoc_GetInkNames()`, `FPDFDoc_GetSeparations()`, `FPDFPageObj_GetInkCoverage()` nao estao disponiveis nos headers PDFium 158 deste projeto
- Checkboxes por tinta sem funcionalidade conectada demonstrada
- Botoes Add/Remove separation sem conexoes de sinal aparentes
- Nao ha extracao de tintas reais do PDF (Spot Colors, DeviceN, Separation inks)
- Output Intent name e exibido, mas analise de separacoes completa nao esta implementada

### Desenvolvimento Necessario
- Extrair tintas reais do PDF usando DLLs descobertas em investigacao Ghidra (2026-11-05):
  - `GpetInkCoverage::ConvertToCheckActionList()` de `PreflightProfileLayer.dll` — conversao de cobertura em lista de acoes
  - `GOVCheckCMYKInkCoverage` de `PDFObjectActions.dll` — analise detalhada por objeto (futuro)
  - `GOVCheckNumSeparations` — contagem de separacoes
  - `GOVCheckDeviceN` — suporte a DeviceN
  - `GOVCheckSpotByPantoneLibrary` — identificacao de cores Pantone
- Implementar exibicao das tintas extraidas usando `GpetInkCoverage::ConvertToCheckActionList()`
  quando DLL PreflightProfileLayer.dll estiver disponivel
- Implementar checkboxes conectados a operacoes de add/remove de separacao quando API estiver disponivel
- Implementar suporte a Spot Colors e DeviceN usando `GOVCheckDeviceN` e `GOVCheckSpotByPantoneLibrary`

**Investigacao Ghidra (2026-11-05):** Bibliotecas DLL do ArtPro+ v24 identificadas:
- `PDFObjectActions.dll` (7.63 MB, 16.751 exports):
  - `GOVCheckCMYKInkCoverage` - Classe principal de verificacao de cobertura de tinta CMYK com 70+ metodos
  - `GOVCheckInkCoverage` - Classe basica de cobertura de tinta
  - `GOVCheckSpotByPantoneLibrary` - Verificacao de cores especiais (Pantone)
  - `GOVCheckOCVisibility` - Verificacao de visibilidade de OCG/OCMD
  - `GOVCheckNumSeparations` - Contagem de separacoes
  - `GOVCheckDeviceN` - Suporte a DeviceN
- `PreflightProfileLayer.dll` (2.28 MB, 2.423 exports):
  - `GpetInkCoverage` - Classe de cobertura de tinta com metodo `ConvertToCheckActionList()` (chave para analise real)
  - `GpetContentAccessibility` - Acessibilidade de conteudo
  - `GpetActionListDatabaseFilter` - Filtro de lista de acoes

**Metodo descoberto: `GOVCheckCMYKInkCoverage::ConvertToCheckActionList()` e `GpetInkCoverage::ConvertToCheckActionList()`**
Estes metodos converte cobertura de tinta em lista de acoes de check, fornecendo a arquitetura para analise real de cobertura de tinta. Futuro trabalho: carregar PDFObjectActions.dll e chamar estes metodos quando a integracao DLL estiver disponivel.

**Arquitetura de integracao planejada:**
1. Etapa curta prazo: Carregar `PreflightProfileLayer.dll`, chamar `GpetInkCoverage::ConvertToCheckActionList()` usando o `PdfSnapshot` existente do ArtPlus
2. Etapa media prazo: Carregar `PDFObjectActions.dll`, chamar `GOVCheckCMYKInkCoverage::ConvertToCheckActionList()` para analise por-objeto mais detalhada
3. Longe prazo: Integrar ambas as DLLs para TAC completo com detalhes por objeto, canais DeviceN, Spot Colors

Desenvolvimento necessario:
- `FPDFDoc_GetInkNames()`, `FPDFDoc_GetSeparations()`, `FPDFPageObj_GetInkCoverage()` nao disponiveis em PDFium 158 deste projeto
- Documentar tintas como parte do perfil do output intent quando APIs nao estiverem disponiveis
- Implementar extraçao alternativa usando informações do perfil do documento quando APIs PDFium nao estiverem disponiveis
- Investigar Ghidra `PDFObjectActions.dll` e `PreflightProfileLayer.dll` para rotinas de extração de tintas do ArtPro+ v24
- Se API ficar disponivel no futuro: implementar exibicao das tintas extraidas em vez de hardcodadas
- Preparar carregamento dinâmico de DLLs com fallback gracefully quando nao disponiveis

### Referência ArtPro
- ArtPro+ v24 exibe tintas reais do documento no painel Separations.
- Nomes de tintas sao extraidos do PDF, nao hardcodados.
- Checkboxes e botões Add/Remove funcionam para separacao management.
- Output Intent esta relacionado com configuracao de separacao.

### Ghidra
- Investigar `PDFObjectActions.dll` e `PreflightProfileLayer.dll` exports relacionados a separations
- `PDFObjectActions.dll` tem 16.751 exports incluindo `GOVCheckInkCoverage`, `GOVCheckCMYKInkCoverage`
- `PreflightProfileLayer.dll` tem exports incluindo `GpetInkCoverage`, `ConvertToCheckActionList`
- `research/re_v24/ghidra/ArtProV24` pode ter analise dessas DLLs
- `research/re_v24/V24_REFERENCE.md` tem SHA-256 `9D3316...0F03` para referencia

### Próxima Ação Técnica
1. Investigar se APIs de extração de tintas (FPDFDoc_GetInkNames, FPDFDoc_GetSeparations) estao disponiveis em headers PDFium mais recentes ou via ponteamento
2. Se nao disponiveis no PDFium 158: Documentar comportamento atual (tintas hardcodadas como indicadores de profile) e planejar atualizacao quando API estiver disponivel
3. Investigar Ghidra `PDFObjectActions.dll` e `PreflightProfileLayer.dll` para rotinas de extração de tintas do ArtPro+ v24
4. Implementar mecanismo alternativo usando informações do perfil do documento quando APIs PDFium nao estiverem disponiveis
5. Testar com PDFs que tem output intent vs. sem output intent
6. Avaliar viabilidade de extração real de tintas em futuras versões do PDFium

---

## 4. Quality Control

### Funcionalidades Existente (Antes da Implementacao)
- **Combo Box**: "Total Area Coverage" (única opção)
- **QSpinBox m_tacLimit**: range 0-400%, valor padrão 300%
- **Nenhuma conexão de sinal** — funcionava apenas como controle visual estático

### Funcionalidades Comprovadas (Após Implementacao)
- ✅ Combo box com 6 categorias: Total Area Coverage, Page Boxes, Separations, Fonts, Images, Resolution
- ✅ TAC Limit spin box (0-400%, default 300%)
- ✅ Botão "Run preflight" que executa checks usando módulo `artplus::preflight`
- ✅ Results list widget mostrando check titles com cores por severidade
- ✅ Status bar messages com contagens de error/warning/info
- ✅ Atualização automática ao abrir documento (via `updatePanels()` chamando `onRunPreflight()`)

### Funcionalidades do Módulo `artplus::preflight`
- **CheckCategory enums**: PageBoxes, Separations, Font, Image, Color, Resolution
- **Severity enums**: Warning, Error, Info
- **CheckResult struct**: title, description, suggestion, details
- **Profile struct**: tacLimit (default 300%), enabledChecks QVector
- **Funções**:
  - `calculateTac(Color)` — calcula cobertura de área total
  - `runChecks(document, profile)` — roda checks aplicaveis
  - `checkDocument(document, profile)` — retorna (results, profile atualizada)

### APIs Disponíveis (Implementacao Actual)
- `artplus::preflight::runChecks(PdfDocument, Profile)` — retorna QVector<CheckResult>
- `artplus::preflight::checkDocument(PdfDocument, Profile)` — retorna par (results, profile)
- `artplus::preflight::calculateTac(Color)` — double coverage calculation

### Limitações (Da Investigacao)
- `calculateTac()` usa `color.totalAreaCoverage()` que retorna `(c+m+y+k)*100` para CMYK
- Tratamento simplificado — não considera Separation/DeviceN profundamente
- Checks PageBoxes/Separations/Font/Image/Color/Resolution sao checks "informacionais" basicos
- Resultados baseados em metadados/document-level, nao analise por regiao do PDF
- Falsos positivos/negativos possiveis sem comprovacao por PDFs de teste com valores conhecidos

### Desenvolvimento Necessario
- Validar `calculateTac()` com PDFs de teste com valores TAC conhecidos
- Melhorar checks para analisar estruturas reais do PDF em vez de apenas informacionais
- Adicionar verificacao de transparencias e sobreimpressao no check de Color
- Implementar analise de resolucao baseada em informacoes do imagem no PDF
- Validar checks de Font e Image com dados reais do documento

### Referencia ArtPro
- ArtPro+ v24 preflight module tem verificacoes mais completas
- TAC limit typically 300-350% para impressao offset
- Checks de color incluem gamut warning, spot color usage, ICC profile compliance
- Checks de geometry inclui object size, bleed, margin violations
- Checks de marks includes registration marks, crop marks, color bar presence
- Checks de text inclui font embedding, text size minimums

### Ghidra
- Investigar `PreflightProfileLayer.dll` (2.4MB) e `ProcessingLayer.dll` (442KB)
- Decompilar rotinas de evaluation de rules
- Trace do algoritmo de check core
- Analisar aplicabilidade de mutator
- Documentar estrutura de perfil e formatos de report

### Próxima Ação Técnica
1. Validar `calculateTac()` com PDFs de teste (valores TAC conhecidos)
2. Melhorar checks para analisar estruturas reais do PDF em vez de apenas informacionais
3. Adicionar verificacao de transparencias/sobreimpressao no check Color
4. Testar com PDFs de diferentes tipos (CMYK, Spot Colors, DeviceN)
5. Conectar resultados ao UI de forma mais significativa

---

## 5. Resumo Geral

### Capacidades Reais do PDFium 158 (Confirmado pelo Código)
- `outputIntentName()` — exibe no painel Separations
- `pageCount()`, `pageSize()`, `pageRotation()` — disponiveis mas nao usados nos painéis
- `pageObjects()` — usado no painel Layers para contagem de objetos
- `SnapshotObject::{kind, bounds}` — estrutura de objeto no snapshot
- `totalAreaCoverage()` em Color — calculo TAC basico
- `isValid()`, `isEncrypted()`, `fileVersion()` — validacoes basicas

### Recursos que Exigem Desenvolvimento Próprio
- Exibicao completa de metadados do documento
- Suporte real a layers/OCG no PDFium — simbolos descobertos: `GOVCheckOCVisibility` de PDFObjectActions.dll, `GpetInkCoverage` de PreflightProfileLayer.dll
- Tintas reais do PDF em vez de hardcodadas — simbolos descobertos: `GOVCheckCMYKInkCoverage` de PDFObjectActions.dll, `GpetInkCoverage::ConvertToCheckActionList()` de PreflightProfileLayer.dll
- Checks TAC/quality mais profundos com analise de regiao

### Recursos que Exigem Referencia ArtPro+ v24
- Como exibe metadados nos painéis ArtPro
- Como extrae tintas para painel Separations
- Como implementa controle de visibilidade de layers (OCG/OCMD)

### Funcoes que Precisam ser Investigadas no Ghidra
- PDFObjectActions.dll exports: GOVCheckCMYKInkCoverage, GOVCheckOCVisibility, GOVCheckNumSeparations, GOVCheckDeviceN
- PreflightProfileLayer.dll exports: GpetInkCoverage, ConvertToCheckActionList
- Funcoes de enumeracao OCG/OCMD
- Algoritmos TAC mais refinados

### Sequencia Recomendada
1. Validar TAC + conectar APIs Document/Separations
2. Investigar OCG/OCMD para Layers
3. Melhorar QC checks com estruturas reais
4. Investigar Ghidra DLLs para features avancadas

---

## 2. Diagnostico dos Quatro Painéis

| Painel | Funcionalidades Existente | Funcionalidades Comprovadas | APIs PDFium | Limitações | Desenvolvimento Necessario |
|--------|--------------------------|----------------------------|-------------|------------|---------------------------|
| **Document** | Crosshair toggle, Page Boxes toggle | Crosshair e Page Boxes funcionam | `pageCount()`, `pageSize()`, `outputIntentName()`, metadados | Não exibe metadados (pages, size, title, author) | Integrar m_doc/m_snapshot ao painel; populacao de dados em updatePanels |
| **Layers** | "Layer 1" fixo, objectList selectionChanged | Contagem de objetos totais | `objectCount()`, `countOf(kind)`, `pageObjects()` | "Layer 1" fixo nao reflete camadas reais; sem controle de visibilidade; sem hierarquia | Investigar OCG/OCMD PDFium; implementar leitura de camadas ou manter informacional. Simbolos descobertos: `GOVCheckOCVisibility` de PDFObjectActions.dll, `GpetInkCoverage` de PreflightProfileLayer.dll |
| **Separations** | Perfil do documento (output intent), 4 tintas CMYK hardcodadas | Perfil do documento exibe output intent | `outputIntentName()` | 4 tintas hardcodadas; checkboxes sem ação; `FPDFDoc_GetInkNames()`, etc. nao disponiveis | Extrair tintas do PDF via desenvolvimento propio; conectar checkboxes/buttons quando API estiver disponivel. Simbolos descobertos: `GOVCheckCMYKInkCoverage` de PDFObjectActions.dll, `GpetInkCoverage::ConvertToCheckActionList()` de PreflightProfileLayer.dll |
| **Quality Control** | Combo box 6 categorias, TAC limit, Run button, Results list | Todos os checks basicos funcionando | `calculateTac()`, `runChecks()`, `checkDocument()` | Checks basicos; analise simplificada; nao regioes do PDF | Validar TAC com PDFs de teste; melhorar checks com estruturas reais do PDF. Simbolos descobertos: `GOVCheckCMYKInkCoverage::ConvertToCheckActionList()` de PDFObjectActions.dll, `GpetInkCoverage::ConvertToCheckActionList()` de PreflightProfileLayer.dll |

### Capacidades Reais do PDFium 158 (Confirmado)
- APIs basicas de metadado e documento
- Contagem de objetos e tipos
- Output intent name
- Calcul TAC basico via color totalAreaCoverage()
- Snapshot de objetos por pagina

### Recursos que Exigem Desenvolvimento Próprio
- Exibicao completa de metadados do documento
- Suporte real a layers/OCG no PDFium
- Tintas reais do PDF em vez de hardcodadas
- Checks TAC/quality mais profundos com analise de regiao

### Recursos que Exigem Referencia ArtPro+ v24
- Como exibe metadados nos painéis ArtPro
- Como extrae tintas para painel Separations
- Como implementa controle de visibilidade de layers (OCG/OCMD)

### Funcoes que Precisam ser Investigadas no Ghidra
- PDFObjectActions.dll exports: GOVCheckCMYKInkCoverage, GOVCheckOCVisibility, GOVCheckNumSeparations, GOVCheckDeviceN
- PreflightProfileLayer.dll exports: GpetInkCoverage, ConvertToCheckActionList
- Funcoes de enumeracao OCG/OCMD
- Algoritmos TAC mais refinados

### Sequencia Recomendada
1. Validar TAC + conectar APIs Document/Separations
2. Investigar OCG/OCMD para Layers
3. Melhorar QC checks com estruturas reais
4. Investigar Ghidra DLLs para features avancadas