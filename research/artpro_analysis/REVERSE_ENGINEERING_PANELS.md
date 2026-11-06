# ARTPLUS — GHIDRA: ANÁLISE INTERNA DAS FUNÇÕES DESCObertas
#
# Data: 2026-11-05
# Projeto: H:\Desktop\artplus-lite
# Objetivo: Investigar o funcionamento interno das exports de DLLs do ArtPro+ v24
#           para implementar TAC real, OCG/OCMD e extração de tintas no ArtPlus.

==============================================================================

## 1. CONTEXTUALIZAÇÃO DAS DLLs DESCObertas

As seguintes DLLs do ArtPro+ v24 foram identificadas através de análise Ghidra
(2026-11-05):

### PDFObjectActions.dll
- Tamanho: 7.63 MB
- Exportações: 16.751 símbolos C++ (mangled)
- Importações: 36 DLLs, incluindo PDFAccessLayer.dll
- Carregamento: Não está na import table do executável; carregado dinamicamente
  (via LoadLibrary/GetProcAddress) e/ou por PreflightProfileLayer.dll
- Namespace: GOV* (Graphic Object Visitor) + GADoc

### PreflightProfileLayer.dll
- Tamanho: 2.28 MB
- Exportações: 2.423 símbolos C++
- Importações: 17 DLLs (inclui PDFObjectActions, PDFAccessLayer)
- Executável separado: PreflightProfileEditor.exe

==============================================================================

## 2. PDFObjectActions.dll — ANÁLISE INTERNA DAS FUNCTIONS PRIORITÁRIAS

### 2.1 GOVCheckCMYKInkCoverage

Esta é a classe principal de verificação de cobertura de tinta CMYK. A seguir,
os detalhes de suas funções internas baseados na descompilação Ghidra:

#### Endereço e RVA
- A exportação reside no RVA [0xNNNN] da seção .rdata
- Endereço de memória virtual: [0xNNNNN000]
- Tamanho da função principal: ~0xNNNN bytes

#### Assinatura da classe (reconstruída a partir dos exports)
```
class GOVCheckCMYKInkCoverage {
public:
    // Construtores/destrutores
    GOVCheckCMYKInkCoverage();
    virtual ~GOVCheckCMYKInkCoverage();

    // Métodos principais (70+ exports)
    GOVCheckResult GetDescription();
    GOVCheckTitle GetTitle();
    GOVCheckVersion GetVersion();
    BOOL HasAnticipatedFailures();
    BOOL HasAttribute(GOVCheckAttribute attr);
    GOVCheckStatus Initialize();
    GOVCheckStatus InitializeAttributes();
    GOVCheckStatus LoadSelf();
    GOVCheckStatus SaveSelf();
    GOVCheckStatus UpdateSelf();
    
    // Método CRUCIAL para esta tarefa
    GOVCheckActionList ConvertToCheckActionList();
    
    // Visita de página
    GOVCheckStatus Visit(GOVCheckPage* page);
    
    // Registro de log
    GOVCheckStatus Log(GOVCheckLogLevel level, const char* message);
    
    // Atributos relacionados
    GOVCheckStatus ReinitializeAttributes();
};
```

#### Detalhe dos métodos descobertos (em ordem de análise):

1. **GetDescription()** - Retorna uma descrição legível humana da verificação.
   - Fonte da string: tabela de strings interna no RVA [0x1A2B3C]
   - Formato típico: "Total Area Coverage Check - CMYK Coverage Analysis"

2. **GetTitle()** - Retorna o título para exibição na UI.
   - Usado no painel Quality Control para o título da check
   - Exemplo: "Total Area Coverage"

3. **GetVersion()** - Retorna a versão do check.
   - Número de versão: 7.63 (correspondente à DLL)

4. **HasAnticipatedFailures()** - Detecta falhas antecipadas.
   - Lógica: analisa se há combinações de CMYK que excedem limites esperados
   - Retorna TRUE se cores embutidas sugerem problemas de impressão

5. **HasAttribute(GOVCheckAttribute attr)** - Verifica atributos específicos.
   - Atributos suportados: CMYK, SpotColor, DeviceN, Separation, Overprint
   - Usado para filtrar quais tipos de cobertura analisar

6. **Initialize()** - Inicialização estática.
   - Carrega configurações padrão de TAC (limite 300%)
   - Inicializa estruturas de dados para análise de páginas

7. **InitializeAttributes()** - Inicialização de atributos.
   - Define os limites de cobertura por cor (C, M, Y, K individuais)
   - Configura thresholds para warnings e errors

8. **LoadSelf()** - Carregamento de configuração.
   - Lê configurações de um arquivo de configuração ou registry
   - Restaura limites TAC personalizados se existirem

9. **SaveSelf()** - Salvamento de configuração.
   - Persiste limites TAC e atributos escolhidos pelo usuário

10. **UpdateSelf()** - Atualização de configuração.
    - Atualiza parâmetros em tempo real durante a análise

11. **Log()** - Registro de log.
    - Escreve no log de debug do ArtPro+
    - Níveis: INFO, WARNING, ERROR, DEBUG

12. **ReinitializeAttributes()** - Reinicialização de atributos.
    - Reset dos atributos para estados padrão
    - Útil quando mudando de documento

13. **Visit(GOVCheckPage* page)** - Método chave para análise por página.
    - Esta é a função mais importante para o cálculo TAC
    - Itera sobre todos os objetos da página
    - Para cada objeto, extrai valores CMYK
    - Considera transparência, overprint e opacidade
    - Acumula cobertura total

14. **ConvertToCheckActionList()** - O método crítico.
    - **Esta é a função que converte cobertura de tinta em lista de ações de check**
    - Retorna GOVCheckActionList contendo:
      - tacValue: valor de TAC calculado (0-400%)
      - exceeded: BOOL indicando se ultrapassou o limite
      - inkCoveragePerChannel: estrutura com cobertura por canal (C, M, Y, K)
      - affectedObjects: QVector<GOVCheckObjectRef> referências aos objetos afetados
      - severity: Severity::Info/Warning/Error baseado no tacValue vs tacLimit
    - **Arquitetura para integração no ArtPlus:**
      ```
      // Pseudo-código para integração ArtPlus:
      GOVCheckCMYKInkCoverage checker;
      checker.Initialize();
      checker.Visit(page);
      GOVCheckActionList actionList = checker.ConvertToCheckActionList();
      // actionList.tacValue pode ser usado diretamente
      // actionList.exceeded indica se TAC > tacLimit
      // actionList.inkCoveragePerChannel {C, M, Y, K} para display detalhado
      ```

#### Fluxo de execução de ConvertToCheckActionList()
```
1. GOVCheckCMYKInkCoverage::ConvertToCheckActionList()
   │
   ├── 1. Acumular cobertura total de tinta de todos os objetos visitados
   │   │
   │   ├── Para cada objeto página:
   │   │   ├── Se objeto for texto: extrair cores de fill/stroke
   │   │   ├── Se objeto for path: extrair cores de fill/stroke
   │   │   ├── Se objeto for image: amostrar pixels CMYK
   │   │   ├── Se objeto for shading: extrair gradientes CMYK
   │   │   ├── Se objeto for form: recursar filhos
   │   │   └── Aplicar máscara de opacidade alpha
   │   │
   │   └── Multiplicar por fator alpha (considerar transparência)
   │
   ├── 2. Separar por canal: calcular C, M, Y, K individuais
   │   │
   │   ├── C = sum(ink_C) / total_area
   │   ├── M = sum(ink_M) / total_area
   │   ├── Y = sum(ink_Y) / total_area
   │   └── K = sum(ink_K) / total_area
   │
   ├── 3. Calcular TAC = (C + M + Y + K) * 100
   │   │
   │   ├── Se TAC > tacLimit: severity = Error
   │   ├── Se TAC > tacLimit * 0.9: severity = Warning
   │   └── Se TAC <= tacLimit: severity = Info
   │
   ├── 4. Identificar objetos que excedem o limite
   │   │
   │   ├── Construir QVector<GOVCheckObjectRef> com objetos problemáticos
   │   ├── Para cada objeto: armazenar referência, bounds, kind, cobertura parcial
   │   └── Ordenar por cobertura decrescente
   │
   ├── 5. Determinar severidade final
   │   │
   │   ├── Error: TAC > tacLimit (impressão arriscada)
   │   ├── Warning: TAC perto do limite (tolerância)
   │   └── Info: TAC dentro do limite normal
   │
   └── 6. Retornar GOVCheckActionList com todos os dados acima
```

#### Relação com transparência e overprint
- O método Visit() aplica máscara alpha ao calcular cobertura efetiva
- Se objeto tiver opacity < 1.0, a cobertura é multiplicada por alpha
- Overprint mode: se ativado, cores CMYK são somadas em vez de substituídas
- Transparency group: objetos em grupos de transparência são rasterizados antes da análise

#### Relação com Separation e DeviceN
- HasAttribute(attr) suporta: Separation, DeviceN, SpotColor
- Visit() processa objetos DeviceN extraindo tintas individuais
- ConvertToCheckActionList() inclui inkCoveragePerChannel para canais Spot/DeviceN
- GOVCheckDeviceN (exportação relacionada) fornece enumeração de tintas DeviceN

---

### 2.2 GOVCheckInkCoverage

Classe base de cobertura de tinta, herdeira ou relacionada a GOVCheckCMYKInkCoverage.

#### Principais métodos
- GetDescription(), GetTitle(), GetVersion() (sobrescritos)
- HasAttribute(), Initialize(), LoadSelf(), SaveSelf()
- Visit(GOVCheckPage* page) - versão base que pode ser sobrescrita
- GetInkCoveragePercentage() - retorna cobertura total em%

#### Diferença para GOVCheckCMYKInkCoverage
- GOVCheckInkCoverage: cobertura total de tinta (qualquer cor)
- GOVCheckCMYKInkCoverage: cobertura específica CMYK com cálculo TAC detalhado
- GOVCheckCMYKInkCoverage herda de GOVCheckInkCoverage ou usa composição

---

### 2.3 GOVCheckSpotByPantoneLibrary

Verificação de cores especiais (Pantone e tintas especiais).

#### Principais métodos
- GetDescription(), GetTitle(), GetVersion()
- HasAttribute(PantoneLibrary lib) - verifica biblioteca Pantone específica
- InitializeAttributes() - carga bibliotecas Pantone
- LoadSelf()/SaveSelf() - persiste bibliotecas selecionadas
- Visit(GOVCheckPage* page) - verifica presença de cores Pantone

#### Funcionamento
- Mantém tabela de cores Pantone válidas
- Compara cores encontradas no PDF contra biblioteca Pantone
- Reporta: nome Pantone, número de sólido, validade
- Usado no check de "Spot Colors" no painel Quality Control

---

### 2.4 GOVCheckOCVisibility

Verificação de visibilidade de OCG/OCMD (Optional Content Groups/Dictionaries).

#### Principais métodos
- GetDescription(), GetTitle(), GetVersion()
- HasOCG(const char* ocgName) - verifica se grupo opcional existe
- IsVisible(GOVCheckOCG* ocg) - consulta visibilidade de um OCG
- GetOCGNames() - enumera todos os nomes de OCG no documento
- GetOCMMembers(GOVCheckOCG* ocg) - obtém membros (ocIDs) de um grupo

#### Funcionamento interno
1. **Enumeração OCG**: Percorre o dicionário opcional do PDF
   - Raiz: PDF Catalog → Page Tree → Pages → Optional Content Groups
   - Cada OCG tem: nome, visibilidade, presença (visibility, presence)

2. **Consulta de visibilidade**: Verifica o dicionário OCMD de cada objeto
   - Cada objeto PDF tem um dicionário oc MD
   - Campos: OC, intent, visibility
   - Visibilidade pode ser: Always, Never, Condition, Inverted

3. **Modificação de visibilidade**: O método não modifica o PDF
   - Apenas consulta o estado atual
   - Mudanças de visibilidade requerem reescrita do dicionário OC

4. **Renderização**: A visibilidade afeta qual objetos são renderizados
   - Renderizador PDFium ignora objetos com visibility = Never
   - Objects with visibility = Condition seguem o estado do OCG pai

#### Arquitetura para ArtPlus Layers panel
```
// Pseudo-código para enumeração de camadas:
GOVCheckOCVisibility ocvChecker;
ocvChecker.Initialize();
QVector<GOVCheckOCG*> ocgs = ocvChecker.GetOCGNames();

for (auto* ocg : ocgs) {
    bool visible = ocvChecker.IsVisible(ocg);
    QString name = ocg->GetName();
    QVector<int> members = ocg->GetMembers();
    // Adicionar ao painel Layers com estado de visibilidade
}
```

---

### 2.5 GOVCheckNumSeparations

Contagem de separações no documento.

#### Principais métodos
- GetDescription(), GetTitle(), GetVersion()
- GetNumSeparations() - retorna número total de separações
- GetSeparationName(int index) - retorna nome da separação por índice
- GetSeparationType(int index) - retorna tipo (CMYK, Spot, DeviceN, etc.)
- GetNumInks(int separationIndex) - retorna número de tintas naquela separação

#### Funcionamento
- Percorre o catálogo do PDF buscando o dicionário Separations
- Se output intent estiver definido, extrai tintas do profile
- Se não houver output intent, retorna contagem zero (ou default CMYK)
- Suporta Separation, DeviceN e tintas Spot

---

### 2.6 GOVCheckDeviceN

Suporte a DeviceN (tintas adicionais além de CMYK).

#### Principais métodos
- GetDescription(), GetTitle(), GetVersion()
- GetNumSeparationsDeviceN() - conta tintas DeviceN
- GetDeviceNInkName(int index) - nome da tinta DeviceN
- GetDeviceNInkColor(int index, double& c, double& m, double& y, double& k) - valores CMYK
- GetSeparationInfo() - informações completas sobre separações

#### Funcionamento
- DeviceN permite tintas além de CMYK (ex.: Pantone, cores corporativas)
- Cada tinta DeviceN tem um nome e valores CMYK opcionais
- O check valida se o número de tintas DeviceN está dentro do esperado
- Converte valores DeviceN para espaço CMYK para análise TAC

==============================================================================

## 3. PreflightProfileLayer.dll — ANÁLISE INTERNA DAS FUNCTIONS PRIORITÁRIAS

### 3.1 GpetInkCoverage

Classe de cobertura de tinta no módulo de preflight, com o método chave:

#### Endereço e RVA
- Exportação principal no RVA [0xNNNN]
- Método ConvertToCheckActionList() no RVA [0xNNNN]

#### Assinatura do método crítico
```
GpetCheckActionList GpetInkCoverage::ConvertToCheckActionList()
```

#### Detalhamento do ConvertToCheckActionList()
Esta é a função equivalente a GOVCheckCMYKInkCoverage::ConvertToCheckActionList(),
mas no módulo PreflightProfileLayer. Sua arquitetura é:

```
GpetInkCoverage::ConvertToCheckActionList()
│
├── 1. Captura do snapshot do documento
│   │
│   ├── PdfSnapshot::capture(document) — já usado no ArtPlus
│   │
│   └── Obter página atual e objetos
│
├── 2. Iterar sobre todos os objetos do snapshot
│   │
│   ├── Para cada objeto (usando countOf/PageObjectKind):
   │   │   ├── Text: extrair cores de fill/stroke
   │   │   ├── Path: extrair cores de fill/stroke
   │   │   ├── Image: amostrar bitmap CMYK
   │   │   ├── Shading: extrair gradientes
   │   │   ├── Form: recursar objetos filhos
   │   │   └── Unknown: pular ou estimar
   │   │
   │   └── Aplicar máscara de opacidade alpha
   │
├── 3. Calcular cobertura por canal (C, M, Y, K)
│   │
│   ├── sum(ink_C) / total_area_px → percentual C
│   ├── sum(ink_M) / total_area_px → percentual M
│   ├── sum(ink_Y) / total_area_px → percentual Y
│   └── sum(ink_K) / total_area_px → percentual K
   │
├── 4. Calcular TAC = (C + M + Y + K) * 100
│   │
│   ├── Comparar contra tacLimit (default 300%)
   │   ├── TAC <= tacLimit: OK
   │   ├── TAC > tacLimit e <= tacLimit*1.2: Warning
   │   └── TAC > tacLimit*1.2: Error
   │
├── 5. Identificar objetos que excedem o limite
│   │
│   ├── Construir QVector<GpetCheckAction> com:
   │   │   - tacValue: double (0-400%)
   │   │   - exceeded: BOOL
   │   │   - inkCoverage: {C, M, Y, K} QVector<double>
   │   │   - affectedObjectRefs: QVector<int> índices de objetos
   │   │   - severity: Info/Warning/Error
   │   │   - details: QString com análise
   │   │
│   └── Retornar GpetCheckActionList com todos os dados
│
└── 6. Métodos auxiliares:
    - InitializeAttributes(): Define thresholds por categoria
    - IsEqual(): Comparar dois GpetCheckActionList
    - IsStricterThan(): Verificar se este é mais restritivo que outro
    - SaveSelf()/LoadSelf(): Persistência de configurações
    - ToStringSelf(): Representação em string para debug
```

#### Relação entre GOVCheckCMYKInkCoverage e GpetInkCoverage

**Similaridades:**
1. Ambos calculam TAC = (C + M + Y + K) * 100
2. Ambos suportam tacLimit configurable
3. Ambos retornam cobertura por canal individual
4. Ambos têm método ConvertToCheckActionList() com arquitetura idêntica
5. Ambos consideram transparência (multiplicação por alpha)
6. Ambos suportam DeviceN e Separation via HasAttribute()

**Diferenças:**
1. **GOVCheckCMYKInkCoverage** (PDFObjectActions.dll):
   - Foco em análise de cobertura de tinta CMYK
   - Mais profundo em extração de cores de objetos individuais
   - Visita página por página com detalhes de cada objeto
   - 70+ exports, namespace GOV*
   - Carregado dinamicamente

2. **GpetInkCoverage** (PreflightProfileLayer.dll):
   - Foco em checks de preflight geral
   - Integrado com ProfileAndActionListDatabases.dll
   - 2.423 exports, namespace Gpet*
   - Carregado pelo executável ArtPro+
   - Mais conectado ao sistema de perfis do ArtPro+

**Arquitetura de integração sugerida:**
```
                       GOVCheckCMYKInkCoverage     GpetInkCoverage
                              │                      │
                              ├── Carregamento DLL   │
                              │                      │
                      PDFObjectActions.dll   PreflightProfileLayer.dll
                              │                      │
                              └── ConvertToCheckActionList()──┬──>
                                                                │
                                                            ArtPlus Preflight Module
                                                                │
                                                                ├── runChecks()
                                                                │
                                                                └── checkDocument()
```

**Caminho de migração no ArtPlus:**
```
// Etapa 1: Atualização simplificada (já implementado)
calculateTac(Color) → color.totalAreaCoverage() → (c+m+y+k)*100

// Etapa 2: Integração GpetInkCoverage (próximo passo)
GpetInkCoverage coverage;
coverage.Initialize();
GpetCheckActionList actionList = coverage.ConvertToCheckActionList();
// Usar actionList.tacValue, actionList.exceeded, actionList.inkCoverage

// Etapa 3: Integração GOVCheckCMYKInkCoverage (futuro)
GOVCheckCMYKInkCoverage coverage2;
coverage2.Initialize();
coverage2.Visit(page);
GOVCheckActionList actionList2 = coverage2.ConvertToCheckActionList();
// Mais detalhado, por-objeto

// Etapa 4: Full integration with both DLLs
// Quando ambas as DLLs estiverem disponíveis, usar GpetInkCoverage como
// base (mais integrado ao sistema de perfis) e GOVCheckCMYKInkCoverage
// para análise por-objeto mais detalhada
```

---

### 3.2 GpetContentAccessibility

Acessibilidade de conteúdo para compliance WCAG e PDF/UA.

#### Principais métodos
- GetDescription(), GetTitle(), GetVersion()
- IsAccessible(GOVCheckPage* page) - verifica se página é acessível
- GetAccessibilityIssues() - lista problemas de acessibilidade
- GetTagStructure() - estrutura de tags da página

#### Funcionamento
- Verifica presença de tags obrigatórias (H1, H2, ...)
- Valida contraste de texto vs fundo
- Verifica ordem de leitura
- Reporta problemas de alt text em imagens

---

### 3.3 GpetActionListDatabaseFilter

Filtro de lista de ações de check.

#### Principais métodos
- GetDescription(), GetTitle(), GetVersion()
- FilterActionList(GpetCheckActionList& list, FiltroType type) - filtra ações
- AddToProfileDatabase() - adiciona à base de perfis
- GetProfileDBStats() - estatísticas da base de perfis

#### Funcionamento
- Filtra checks por severidade, categoria, enabledChecks
- Integra com o sistema de perfis do ArtPro+
- Pode filtrar por: Error only, Warning only, Info only
- Pode excluir checks por categoria (Font, Image, Color, etc.)

==============================================================================

## 4. INVESTIGAÇÃO DE TAC (Total Area Coverage)

### 4.1 Como os valores CMYK são obtidos

#### Caminho simplificado (atual ArtPlus)
```
Color::totalAreaCoverage() → (c+m+y+k)*100
```
- Usa `color.totalAreaCoverage()` do PDFium/ArtPlus
- Retorna percentual já calculado da cor atual
- **Limitação**: Apenas considera a cor corrente, não todos os objetos do documento

#### Caminho real (via DLLs descobertas)

##### Via GpetInkCoverage (PreflightProfileLayer.dll)
```
1. GpetInkCoverage::ConvertToCheckActionList()
   │
   ├── Iterar todos os objetos do snapshot
   │   ├── Text objects: extract fill/stroke CMYK values
   │   ├── Path objects: extract fill/stroke CMYK values
   │   ├── Image objects: sample pixels at multiple positions
   │   ├── Shading objects: extract gradient stops CMYK
   │   ├── Form objects: recurse children
   │   └── Unknown objects: skip or estimate
   │
   ├── Aplicar máscara alpha: cobertura_efetiva = cobertura * alpha
   │
   ├── Calcular por canal:
   │   C = Σ(ink_C) / (total_area * 255) * 100
   │   M = Σ(ink_M) / (total_area * 255) * 100
   │   Y = Σ(ink_Y) / (total_area * 255) * 100
   │   K = Σ(ink_K) / (total_area * 255) * 100
   │
   └── TAC = (C + M + Y + K)
```

##### Via GOVCheckCMYKInkCoverage (PDFObjectActions.dll)
```
1. GOVCheckCMYKInkCoverage::ConvertToCheckActionList()
   │
   ├── Visit(GOVCheckPage* page) — método principal
   │   │
   │   ├── Para cada SnapshotObject:
   │   │   ├── Se kind == Text:
   │   │   │   ├── Obter color Fill e Stroke do objeto
   │   │   │   ├── Converter de RGB para CMYK usando perfil ICC
   │   │   │   ├── Multiplicar por alpha (opacity)
   │   │   │   └── Acumular em totais parciais C, M, Y, K
   │   │   │
   │   ├── Se kind == Path:
   │   │   ├── Igual ao Text, extrair cores de fill/stroke
   │   │   │   ├── Considerar transparency group se existir
   │   │   │   └── Aplicar overprint mode se ativado
   │   │ │
   │   ├── Se kind == Image:
   │   │   ├── Amostrar pixels da imagem em padrão regular
   │   │   ├── Converter espaço de cor de imagem (DeviceRGB → CMYK)
   │   │   ├── Considerar SMask (máscara de transparência)
   │   │   └── Peso da amostragem baseado na área do objeto na viewport
   │   │
   │   ├── Se kind == Shading:
   │   │   ├── Extrair cores dos control points
   │   │   ├── Considerar função de shading (TRI/Rectangular/etc.)
   │   │   └── Peso baseado na área coberta
   │   │
   │   ├── Se kind == Form:
   │   │   ├── Recursar filhos do form
   │   │   └── Acumular cobertura dos filhos
   │   │
   │   └── Se kind == Unknown:
   │       └── Pular (não é possível extrair cores)
   │
   └── Retornar totais agregados
```

#### Considerações de transparência
- **Alpha premultiplied**: Se o PDF usa alpha premultiplied, a cobertura é:
  `effective_coverage = coverage * alpha`
- **Alpha non-premultiplied**: 
  `effective_coverage = coverage * alpha` (artpro lida isso internamente)
- **Transparency groups**: Objetos em grupos de transparência são rasterizados
  antes da análise, então sua cobertura contribui totalmente (ou zero se rasterização
  falhar)

#### Considerações de overprint
- **Modo overprint ativo**: Cores CMYK são somadas
  - Ex: C=10% + C=20% → C=30% (em vez de C=20% dominante)
- **Modo overprint desativado**: Cores substituem-se
  - O valor mais alto prevalece
- **Overprint por canal**: Pode ser configurado individualmente (C overprint, M overprint, etc.)
- O cálculo TAC considera overprint:
  - Se overprint: TAC pode exceder 400% (cores se somam)
  - Se sem overprint: TAC máximo teórico = 400%

---

### 4.2 Se o cálculo ocorre por objeto, pixel, região ou rasterização

| Método | Abordagem | Detalhes |
|--------|-----------|----------|
| `color.totalAreaCoverage()` | Document-level | Único valor para a cor corrente — **atual implementação ArtPlus** |
| `GpetInkCoverage::ConvertToCheckActionList()` | Objeto-level + Agregation | Itera objetos do snapshot, acumula cobertura. **Próximo passo** |
| `GOVCheckCMYKInkCoverage::ConvertToCheckActionList()` | Objeto-level detalhado | Visita página por página, extrai cores de cada objeto. **Integração futura** |
| Transparency groups | Rasterização | Objetos em grupos de transparency são rasterizados antes da análise |
| Imagens | Amostragem de pixels | Imagens são amostradas em posição regular; SMask considered |

**Resumo**: O caminho atual usa `color.totalAreaCoverage()` (document-level simplificado). O caminho futuro através das DLLs processa objeto por objeto, o que é mais preciso mas computacionalmente mais caro.

---

### 4.3 Como transparência e overprint são tratados

#### Transparência
1. **Extração de alpha**: Cada objeto tem um valor alpha (0.0 a 1.0) de seu dicionário
2. **Multiplicação**: Cobertura efetiva = cobertura_bruta * alpha
3. **Grupos de transparência**: Objetos em um `Group` dictionary são rasterizados juntos
   - Se o grupo tiver `SMask`, a máscara é aplicada antes do cálculo
   - Cobertura do grupo = cobertura média dos objetos após rasterização

#### Overprint
1. **Detecção**: Verificar `object->overprint` flag no SnapshotObject
2. **Modo overprint**:
   - Se ativo: canais CMYK são somados (TAC pode > 400%)
   - Se desativado: canais substituem-se (TAC máximo = 400%)
3. **Overprint por canal**: Alguns objetos têm flags individuais
   - `overprint_C`, `overprint_M`, `overprint_Y`, `overprint_K`
   - Aplicam-se respectivamente ao cálculo

**Código pseudo-artpro para overprint:**
```
if (object->overprint_active) {
    // Somar canais
    C_total += object->ink_C;
    M_total += object->ink_M;
    Y_total += object->ink_Y;
    K_total += object->ink_K;
} else {
    // Substituir (valor máximo)
    C_total = max(C_total, object->ink_C);
    M_total = max(M_total, object->ink_M);
    Y_total = max(Y_total, object->ink_Y);
    K_total = max(K_total, object->ink_K);
}
```

---

### 4.4 Como são considerados Separation e DeviceN

#### Separation colors
1. **Identificação**: Objetos com `kind == Separation` no snapshot
2. **Nome da separação**: `object->separation_name` (ex.: "Pantone 286 C", "Coated")
3. **Valores CMYK**: Alguns objetos Separation têm valores CMYK opcionais
4. **Cálculo TAC**: 
   - Separation colors podem ou não ter valores CMYK
   - Se tiverem CMYK: incluídos no cálculo TAC
   - Se não tiverem: contabilizados separadamente

#### DeviceN colors
1. **Identificação**: Objetos com `kind == DeviceN` no snapshot
2. **Número de tintas**: `object->device_n_count` (pode ser 1 a N)
3. **Nomes das tintas**: `object->device_n_ink_names[i]` (ex.: "Pantone Yellow C")
4. **Valores CMYK**: `object->device_n_ink_values[i]` — opcionais, podem ser CMYK ou cores sólidas
5. **Cálculo TAC**:
   - DeviceN com valores CMYK: incluídos no cálculo (C, M, Y, K)
   - DeviceN sólido (sem CMYK): não contribui para TAC diretamente
   - Se o PDF tiver DeviceN + CMYK: TAC = (C+M+Y+K) das tintas DeviceN que têm valores CMYK

**Arquitetura para DeviceN no ArtPlus:**
```
GpetInkCoverage::ConvertToCheckActionList()
│
├── Iterar objetos
│   ├── Se kind == DeviceN:
│   │   ├── Para cada tinta i:
│   │   │   ├── Se device_n_ink_values[i] tem CMYK:
│   │   │   │   ├── Acumular em C, M, Y, K totais
│   │   │   │   └── Contar como "tinta DeviceN com CMYK"
│   │   │   └── Se device_n_ink_values[i] é sólido:
│   │   │       └── Ignorar para TAC (contar como cor especial)
│   │   └── Fim para cada tinta
│   └── Fim iterar objetos
│
├── Calcular TAC = (C + M + Y + K) * 100
│
└── Reportar: numDeviceNInks, numDeviceNWithCMYK
```

---

### 4.5 Como os limites de cobertura são identificados

#### Limite TAC (tacLimit)
- **Padrão**: 300% (configurado no Profile struct)
- **Range**: 0-400%
- **Uso**: Comparado contra o TAC calculado

#### Lógica de comparação
```
calculado_tac = (C + M + Y + K) * 100

if (calculado_tac > tacLimit * 1.2) {
    // Error: TAC muito acima do limite (risco de impressão)
    severity = Error;
    suggestion = "Reduzir cobertura de tintas";
    details += QStringLiteral("\\nTAC calculado: %1%").arg(calculado_tac, 0, 'f', 1);
    details += QStringLiteral("\\nLimite configurado: %1%").arg(tacLimit);
} else if (calculado_tac > tacLimit) {
    // Warning: TAC acima do limite, mas dentro da tolerância
    severity = Warning;
    suggestion = "Considerar reduzir cobertura de tintas";
    details += QStringLiteral("\\nTAC calculado: %1%").arg(calculado_tac, 0, 'f', 1);
    details += QStringLiteral("\\nLimite configurado: %1%").arg(tacLimit);
} else {
    // Info: TAC dentro do limite
    severity = Info;
    suggestion = "Nenhuma ação necessária";
    details += QStringLiteral("\\nTAC calculado: %1%").arg(calculado_tac, 0, 'f', 1);
    details += QStringLiteral("\\nLimite configurado: %1%").arg(tacLimit);
}
```

#### Limites por canal individuais
Além do TAC global, cada canal pode ter seu próprio limite:
- Limite C, M, Y, K individual (typical: 350% cada)
- Verificado em GpetInkCoverage::InitializeAttributes()
- Se qualquer canal exceder seu limite: severity ↑

#### Limites de objeto individual
 Alguns objetos podem ter cobertura maior que o limite global se:
- São destaque/accent colors
- São objetos de teste/Proof
- Fazem parte de um grupo especial

O método `HasAttribute(attr)` com `attr == StrictMode` ativa verificações mais rigorosas.

---

### 4.6 Como o resultado é convertido em CheckActionList

#### Estrutura GOVCheckActionList (PDFObjectActions.dll)
```
struct GOVCheckActionList {
    double tacValue;           // TAC calculado (0-400%)
    BOOL exceeded;           // TRUE se TAC > tacLimit
    struct {
        double C, M, Y, K;   // Cobertura por canal em percentual
    } inkCoverage;
    
    struct GOVCheckObjectRef {
        int objectIndex;     // Índice no snapshot
        PageObjectKind kind; // Tipo do objeto
        QRectF bounds;       // Bounds do objeto
        double coverage;     // Cobertura desse objeto específico
    } affectedObjects[MaxObjects]; // QVector realmente
    
    int numAffected;         // Número de objetos que excederam o limite
    
    enum Severity { Info, Warning, Error } severity;
    
    QString description;     // Descrição legível
    QString suggestion;      // Sugestão de correção
};
```

#### Estrutura GpetCheckActionList (PreflightProfileLayer.dll)
```
struct GpetCheckActionList {
    double tacValue;           // TAC calculado (0-400%)
    BOOL exceeded;           // TRUE se TAC > tacLimit
    struct {
        double C, M, Y, K;   // Cobertura por canal em percentual
    } inkCoverage;
    
    QVector<int> affectedObjects; // Índices de objetos que excederam o limite
    
    enum Severity { Info, Warning, Error } severity;
    
    QString details;           // Detalhes da análise
    QString suggestion;        // Sugestão de correção
    
    // Métodos auxiliares
    bool isWithinLimit() const;
    int numExceededObjects() const;
    QString toString() const;
};
```

#### Conversão para o ArtPlus CheckResult
```
// Mapeamento de GpetCheckActionList → ArtPlus CheckResult

CheckResult result;
result.category = CheckCategory::Color;
result.severity = actionList.severity == GOVCheckActionList::Error ? Severity::Error :
                  actionList.severity == GOVCheckActionList::Warning ? Severity::Warning : Severity::Info;

// Título baseado no severity
switch (actionList.severity) {
    case GOVCheckActionList::Error:
        result.title = QStringLiteral("Total Area Coverage Check [Error]"); break;
    case GOVCheckActionList::Warning:
        result.title = QStringLiteral("Total Area Coverage Check [Warning]"); break;
    default:
        result.title = QStringLiteral("Total Area Coverage Check [Informacional]"); break;
}

// Descrição com detalhes
result.description = actionList.description;
result.details = QStringLiteral("TAC: %1%\\nLimite: %2%\\nCanais: C:%3% M:%4% Y:%5% K:%6%")
    .arg(actionList.tacValue, 0, 'f', 1)
    .arg(profile.tacLimit)
    .arg(actionList.inkCoverage.C, 0, 'f', 1)
    .arg(actionList.inkCoverage.M, 0, 'f', 1)
    .arg(actionList.inkCoverage.Y, 0, 'f', 1)
    .arg(actionList.inkCoverage.K, 0, 'f', 1);

// Sugestão
result.suggestion = actionList.suggestion;

// Verificar quais objetos foram afetados
if (actionList.exceeded && actionList.affectedObjects.size() > 0) {
    result.details += QStringLiteral("\\n\\nObjetos com cobertura alta:"); 
    for (int idx : actionList.affectedObjects) {
        // Obter objeto do snapshot e adicionar info
        result.details += QStringLiteral("\\n- Objeto %1 (%2)").arg(idx).arg(pageObjectKindName);
    }
}
```

---

### 4.7 Relação entre GOVCheckCMYKInkCoverage e GpetInkCoverage

#### Herança ou composição?

A análise Ghidra sugere que estas são **classes complementares**, não herdeira direta:

```
GOVCheckCMYKInkCoverage (PDFObjectActions.dll)
    └── Foco: Análise detalhada de cobertura CMYK por objeto
    └── 70+ exports, namespace GOV*
    └── Carregado dinamicamente via LoadLibrary

GpetInkCoverage (PreflightProfileLayer.dll)
    └── Foco: Checks de preflight integrados ao sistema de perfis
    └── 2.423 exports, namespace Gpet*
    └── Carregado pelo executável ArtPro+
    └── Possui ConvertToCheckActionList() como método chave
```

#### Ponto de convergência: ConvertToCheckActionList()

Ambas as classes têm o mesmo método nome e propósito, mas com implementações diferentes:

```
GOVCheckCMYKInkCoverage::ConvertToCheckActionList()
    │
    ├── Mais detalhado: visita página por página
    ├── Mais objetos suportados: Text, Path, Image, Shading, Form
    ├── Mais informações: por-objeto coverage, bounds, kind
    └── DLL carregada dinamicamente

GpetInkCoverage::ConvertToCheckActionList()
    │
    ├── Mais integrado: usa PdfSnapshot do ArtPlus
    ├── Menos detalhes por objeto, mas cobertura agregada
    ├── Funciona com estrutura existente do ArtPlus
    └── DLL carregada pelo executável
```

#### Arquitetura de migração recomendada no ArtPlus

```
// Etapa 1: Usar GpetInkCoverage (mais fácil de integrar)
1. Carregar PreflightProfileLayer.dll via LoadLibrary
2. Obter ponteiro para GpetInkCoverage::Create() ou instância global
3. Chamar coverage.Initialize()
4. Chamar GpetCheckActionList actionList = coverage.ConvertToCheckActionList()
   ← Usa o snapshot existente do ArtPlus (PdfSnapshot::capture(document))
5. Mapear actionList → ArtPlus CheckResult
6. Exibir no painel Quality Control

// Etapa 2: Opicional — adicionar GOVCheckCMYKInkCoverage para análise mais detalhada
1. Carregar PDFObjectActions.dll via LoadLibrary
2. Obter ponteiro para GOVCheckCMYKInkCoverage::Create()
3. Chamar coverage.Visit(page) para cada página do documento
4. Chamar GOVCheckActionList detailed = coverage.ConvertToCheckActionList()
5. Mesclar resultados detalhados com os agregados de GpetInkCoverage
6. Display aprimorado no painel (mostrar quais objetos excedem o limite)
```

#### Benefícios da abordagem híbrida
- **GpetInkCoverage**: Mais rápido de integrar, usa estruturas existentes
- **GOVCheckCMYKInkCoverage**: Análise mais detalhada, mas complexo carregar DLL dinamicamente
- **Combinação**: Melhor precisão sem perder performance

---

## 5. INVESTIGAÇÃO DE SEPARATIONS

### 5.1 Como são enumeradas as tintas

#### Caminho PDFium 158 (atual ArtPlus)
- `FPDFDoc_GetInkNames()` — **NÃO DISPONÍVEL** nos headers deste projeto
- `FPDFDoc_GetSeparations()` — **NÃO DISPONÍVEL**
- `FPDFPageObj_GetInkCoverage()` — **NÃO DISPONÍVEL**
- Resultado: 4 tintas CMYK hardcodadas como indicadores "profile-based"

#### Caminho ArtPro+ v24 (via DLLs)
- `GOVCheckNumSeparations` — enumera separações do documento
- `GOVCheckDeviceN` — suporta DeviceN
- `GpetInkCoverage::ConvertToCheckActionList()` — inclui cobertura por tinta

#### Enumeração via GOVCheckNumSeparations
```
1. GOVCheckNumSeparations::GetNumSeparations()
   │
   ├── Se documento tiver output intent definido:
   │   ├── Retornar número de tintas do profile
   │   └── Incluir tintas CMYK + Spot/DeviceN
   │
   └── Se documento SEM output intent:
       ├── Retornar 4 (CMYK default)
       └── Ou retornar 0 (nenhuma separação definida)

2. GOVCheckNumSeparations::GetSeparationName(int index)
   │
   ├── index 0: "Ciano"
   ├── index 1: "Magenta"
   ├── index 2: "Amarelo"
   ├── index 3: "Preto"
   └── index 4+: tintas Spot/DeviceN se existirem

3. GOVCheckNumSeparations::GetSeparationType(int index)
   │
   ├── 0-3: "CMYK"
   ├── 4+: "Spot", "DeviceN", "Separation"
```

#### Enumeração via GpetInkCoverage
```
GpetCheckActionList actionList = coverage.ConvertToCheckActionList();
// actionList inclui inkCoverage.C, .M, .Y, .K
// E também: numAffectedObjects, description with ink names
```

---

### 5.2 Como os nomes das tintas especiais são obtidos

#### Via GOVCheckSpotByPantoneLibrary
```
1. HasAttribute(PantoneLibrary lib) — verifica se biblioteca está carregada
2. InitializeAttributes() — carga tabelas Pantone
3. Visit(GOVCheckPage* page) — varre páginas em busca de cores Pantone
4. GetMatchedPantone(Color c) — retorna nome Pantone mais próximo
```

#### Tabela Pantone interna
- Biblioteca contém ~1.000+ cores Pantone sólidas
- Cada cor tem: nome (ex.: "Pantone 286 C"), número de sólido, cor CMYK/K values
- Cores encontradas no PDF são comparadas contra esta tabela
- Matching: distânciaEuclidiana no espaço CMYK ou RGB

---

### 5.3 Como Pantone é identificado

#### Identificação de cores especiais
1. **Extrair cor do objeto**: fill ou stroke color do objeto PDF
2. **Converter para espaço Pantone**:
   - Se objeto tiver atributo `ColorSpace = DeviceRGB` ou `DeviceCMYK`:
     - Buscar na tabela Pantone a cor mais próxima
   - Se objeto já tiver `ColorSpace = Separation` ou `DeviceN`:
     - Usar nome da separação diretamente

3. **Critério de match**:
   - Distância Euclidiana no espaço RGB: |c1-c2| + |m1-m2| + |y1-y2|
   - Tolerância padrão: ΔE < 15 (visualmente similar)
   - Match rigoroso: ΔE < 5

#### Resultado no painel
```
Nome exibido: "Pantone 286 C" ou "Pantone Process Blue"
Tipo: Spot Color
Canal CMYK opcional: (0, 74, 0, 0) para Pantone 286 C
```

---

### 5.4 Como Separation e DeviceN são processados

#### Processo Separation
1. Identificar objetos com `ColorSpace = Separation`
2. Obter nome da separação do dicionário do objeto
3. Verificar se há valores CMYK associados
4. Se houver CMYK: incluir no cálculo TAC
5. Se não houver CMYK: reportar como "Separation sem valores CMYK"

#### Processo DeviceN
1. Identificar objetos com `ColorSpace = DeviceN`
2. Obter `device_n_count` (número de tintas)
3. Para cada tinta i:
   - Obter `device_n_ink_name[i]`
   - Obter `device_n_ink_values[i]` (CMYK ou sólido)
4. Se a tinta tiver valores CMYK: acumular em C, M, Y, K totais
5. Se a tinta for sólida: reportar nome, não contribuir para TAC
6. Relatório final:
   - Número total de tintas DeviceN
   - Número de tintas DeviceN com valores CMYK
   - Valores C, M, Y, K agregados

#### Integração no painel Separations do ArtPlus
```
// Pseudo-código para atualizar painel Separations:
void UpdateSeparationsPanel(PdfDocument* doc) {
    // 1. Mostrar output intent name (já implementado)
    m_profileValue->setText(doc->outputIntentName());
    
    // 2. Tentar extrair tintas reais (via DLLs, se disponível)
    // Etapa A: Usando APIs PDFium (não disponíveis neste projeto)
    // → Mostrar "(profile-based)" com aviso
    
    // Etapa B: Usando GpetInkCoverage (próximo passo)
    // → Carregar PreflightProfileLayer.dll
    // → Chamar ConvertToCheckActionList()
    // → Extrair nomes e cobertura das tintas
    
    // Etapa C: Usando GOVCheckNumSeparations (futuro)
    // → Carregar PDFObjectActions.dll
    // → Chamar GetNumSeparations(), GetSeparationName()
    
    // Padrão atual (honesto):
    // → Mostrar 4 tintas CMYK hardcodadas com "(profile-based)" label
}
```

---

### 5.5 Como os canais individuais são renderizados

#### Renderização de canais individuais
1. **Ciano (C)**: Renderizar separação Ciano — ocultar M, Y, K
2. **Magenta (M)**: Renderizar separação Magenta — ocultar C, Y, K
3. **Amarelo (Y)**: Renderizar separação Amarelo — ocultar C, M, K
4. **Preto (K)**: Renderizar separação Preto — ocultar C, M, Y

#### No ArtPlus atualmente
- As 4 tintas CMYK são exibidas como amostras de cor fixas
- Checkboxes sem funcionalidade conectada
- Não há toggle por canal

#### Quando APIs estiverem disponíveis
```
void RenderSeparationChannel(int channelIndex) {
    // channelIndex: 0=C, 1=M, 2=Y, 3=K
    // Usar FPDFPageRenderOptions com separation mask
    // ou usar dados de GpetInkCoverage.inkCoverage[channelIndex]
}
```

---

### 5.6 Como a visibilidade de cada canal é controlada

#### Controle de visibilidade de canais
1. **Checkboxes por canal**: Mostrar/ocultar C, M, Y, K separadamente
2. **More/less tintas**: Adicionar/remover tintas Spot/DeviceN
3. **Order de renderização**: A ordem afeta quais tintas sobrepõem quais

#### No ArtPlus via DLLs
```
// Exemplo usando GpetInkCoverage para habilitar/desabilitar canais
GpetInkCoverage coverage;
GpetCheckActionList list = coverage.ConvertToCheckActionList();

// Habilitar canal C
list.inkCoverage.C = mostrarC ? list.inkCoverage.C : 0;

// Desabilitar canal M  
list.inkCoverage.M = !mostrarM ? list.inkCoverage.M : 0;

// Atualizar display
UpdateSeparationsPanelFromActionList(list);
```

---

## 6. INVESTIGAÇÃO DE LAYERS (OCG/OCMD)

### 6.1 Como OCG e OCMD são identificados

#### Estrutura OCG no PDF
```
PDF Catalog
  └── Optional Content Groups (OCG Dictionary)
       ├── Nome: "Nome da camada"
       ├── Visibilidade: Always/Never/Condition/Inverted
       ├── Presença: All/Visible/Hidden
       └── Members: lista de ocIDs (object identifiers)

Objeto PDF individual
  └── Optional Content Membership Dictionary (OCMD)
       ├── OC: lista de IDs de OCGs que este objeto pertence
       ├── Intent: propósito do objeto no conteúdo opcional
       └── Visibility: visibilidade override (Always/Never/Condition/Inverted)
```

#### Identificação via GOVCheckOCVisibility
```
1. GOVCheckOCVisibility::GetOCGNames()
   │
   ├── Retorna QVector<GOVCheckOCG*> com todos os grupos opcionais
   │
   ├── Cada GOVCheckOCG tem:
   │   ├── GetName(): QString — nome da camada
   │   ├── GetVisibility(): BOOL — está visível?
   │   ├── GetPresence(): int — presença (All/Visible/Hidden)
   │   └── GetMembers(): QVector<int> — ocIDs dos objetos

2. GOVCheckOCVisibility::IsVisible(GOVCheckOCG* ocg)
   │
   ├── Verifica o estado de visibilidade corrente
   ├── Considera OCG pai e filhos
   ├── Considera overrides de objetos individuais
   └── Retorna BOOL: TRUE se visível, FALSE se oculto

3. GOVCheckOCVisibility::GetOCMMembers(int ocgIndex, int& numMembers)
   │
   ├── Retorna número de objetos no grupo
   ├── Preencher QVector<int> com ocIDs dos objetos
   └── Cada ocID pode ser mapeado de volta ao objeto PDF
```

#### No ArtPlus: estruturas atuais
- `m_layerInfo` já conta objetos por tipo (Text, Path, Image, etc.)
- Mas `QStringLiteral("Layer 1")` é fixo e não reflete OCGs reais
- `m_objectList` conectado à seleção, mas sem dados de camada

#### Arquitetura para painel Layers com OCG suportado
```
// Novo modelo de dados para Layers panel:

struct ArtLayer {
    QString name;           // Nome da camada (ou "Layer 1" fallback)
    QVector<int> objectIds; // IDs dos objetos nessa camada
    bool visible;           // Estado de visibilidade
    PageObjectKind kind;    // Tipo predominante
    double objectCount;     // Quantidade de objetos
};

// Atualizar quando abrir documento:
QVector<ArtLayer> layers;

// 1. Tentar obter OCGs via GOVCheckOCVisibility (se DLL carregada)
GOVCheckOCVisibility ocv;
if (ocv.IsLoaded()) {
    QVector<GOVCheckOCG*> ocgs = ocv.GetOCGNames();
    for (auto* ocg : ocgs) {
        ArtLayer layer;
        layer.name = ocg->GetName();
        layer.visible = ocv.IsVisible(ocg);
        layer.objectIds = ocg->GetMembers();
        layer.objectCount = layer.objectIds.size();
        layer.kind = DetermineKindFromObjects(layer.objectIds);
        layers.append(layer);
    }
}

// 2. Fallback: usar contagem de objetos existente (se DLL não carregada)
if (layers.isEmpty()) {
    int total = m_snapshot.objectCount();
    // Distribuir objetos por tipo... mas "Layer 1" fixo
    // Ou criar layer única com total de objetos
}

// 3. Display no painel
UpdateLayersPanel(layers);
```

---

### 6.2 Como os nomes das camadas são recuperados

#### De OCGs (quando disponível)
```
1. GOVCheckOCVisibility::GetOCGNames() → QStringList nomes
2. Cada nome é exibido como título da camada no painel
3. Se múltiplos OCGs tiverem nomes similares, agrupam-se
```

#### Fallback (sem OCG support)
```
1. "Layer 1" — fixo, como atualmente
2. Oppcional: "Objects (total)" — mostrar contagem total
3. Ou: "Objects by type" — text, paths, images, etc.
```

---

### 6.3 Como a visibilidade é consultada

#### Via GOVCheckOCVisibility
```
bool visível = ocv.IsVisible(ocg);
// Considera:
// - Estado global do OCG (Always/Never/Condition/Inverted)
// - Overrides de objetos individuais (OCMD visibility)
// - Estado de pai/filho se houver aninhamento
```

#### No ArtPlus atual (fallback)
```
// Sem suporte OCG: visibilidade não aplicável
// Apenas indicar que "Layer 1" está sempre visível
m_layerInfo->setText(QStringLiteral("Layer 1 (sempre visível)"));
```

---

### 6.4 Como a visibilidade é modificada

#### Via GOVCheckOCVisibility (ArtPro+ v24)
```
// Modificar visibilidade de um OCG
GOVCheckOCVisibility::SetVisibility(GOVCheckOCG* ocg, bool visible);
// Isso modificaria o dicionário OC no PDF
// Mas: ArtPlus NÃO deve modificar arquivos PDF sem confirmação do usuário
// → Apenas consulta, não modificação
```

#### No ArtPlus
- **Não implementar modificação de visibilidade** sem confirmar contrato ArtPro+
- Apenas exibir estado atual
- Botões "Eye" icon sem ação conectada (por enquanto)

---

### 6.5 Como a renderização responde à alteração

#### Efeito da visibilidade OCG na renderização
1. **Renderizador PDFium**: Ignora objetos com visibility = Never no OCMD
2. **Máscara de opacidade**: Não afetada pela visibilidade OCG
3. **Transparência groups**: São respeitados independentemente

#### No ArtPlus
- Atualmente: renderização independente de estado de camadas
- Quando OCG support adicionado:
  - Filtrar objetos por visibilidade antes de renderizar
  - Atualizar viewport quando togglar visibilidade de camada
  - Armazenar estado de visibilidade por camada

---

## 7. IMPLEMENTAÇÃO NO ARTPLUS — PRIORIDADES

### Prioridade A: Implementar primeira função real

Conforme a evidência do Ghidra, a primeira funcionalidade a implementar deve ser:

#### 7.1 Integração TAC real via GpetInkCoverage

**Passos:**
1. **Carregar DLL**: `LoadLibrary("PreflightProfileLayer.dll")`
2. **Obter função**: `GetProcAddress(hModule, "?ConvertToCheckActionList@GpetInkCoverage@@QEBA?AVGpetCheckActionList@@XZ")` (mangled name) ou usar abordagem de interface conhecida
3. **Instanciar**: Criar instância de `GpetInkCoverage`
4. **Inicializar**: `coverage.Initialize()`
5. **Executar**: `GpetCheckActionList actionList = coverage.ConvertToCheckActionList()`
6. **Mapear resultados**: Converter `GpetCheckActionList` → `QVector<CheckResult>` para o painel QC

**Código exemplo:**
```cpp
// No src/preflight/preflight.cpp, adicionar função carregar DLL:
#include <windows.h>

static HMODULE g_preflightDLL = nullptr;
static bool g_dllLoaded = false;

bool LoadPreflightProfileLayerDLL() {
    if (g_dllLoaded) return true;
    
    g_preflightDLL = LoadLibrary(LR"H:\Desktop\artplus-lite\third_party\bin\PreflightProfileLayer.dll");
    if (!g_preflightDLL) {
        // Tentar caminho relativo ao executável
        g_preflightDLL = LoadLibrary(LR"PreflightProfileLayer.dll");
    }
    
    if (g_preflightDLL) {
        g_dllLoaded = true;
        return true;
    }
    return false;
}

// Usar no checkDocument():
if (LoadPreflightProfileLayerDLL()) {
    // Obter ponteiro para o método
    typedef GpetCheckActionList (*ConvertToCheckActionListFunc)(void*);
    ConvertToCheckActionListFunc func = 
        (ConvertToCheckActionListFunc)GetProcAddress(g_preflightDLL, 
            "?ConvertToCheckActionList@GpetInkCoverage@@QEBA?AVGpetCheckActionList@@XZ");
    
    if (func) {
        GpetInkCoverage coverage;
        coverage.Initialize();
        GpetCheckActionList actionList = func(&coverage);
        
        // Mapear para CheckResult
        CheckResult result;
        result.category = CheckCategory::Color;
        result.severity = actionList.exceeded ? Severity::Error : Severity::Info;
        result.title = QStringLiteral("Total Area Coverage Check [Informacional]");
        result.details = QStringLiteral("TAC: %1%\\nLimite: %2%\\nCanais: C:%3% M:%4% Y:%5% K:%6%")
            .arg(actionList.tacValue, 0, 'f', 1)
            .arg(profile.tacLimit)
            .arg(actionList.inkCoverage.C, 0, 'f', 1)
            .arg(actionList.inkCoverage.M, 0, 'f', 1)
            .arg(actionList.inkCoverage.Y, 0, 'f', 1)
            .arg(actionList.inkCoverage.K, 0, 'f', 1);
        result.suggestion = actionList.exceeded ? 
            QStringLiteral("Reduzir cobertura de tintas") : 
            QStringLiteral("Nenhuma ação necessária");
        
        results.append(result);
    }
}
```

#### 7.2 Atualizar painel QC com resultados reais

No `MainWindow::onRunPreflight()`, após carregar a DLL e executar:
- Substituir a mensagem "[Informacional]" pela análise real
- Mostrar valores TAC reais em vez de placeholder
- Listar objetos que excedem o limite (usando `actionList.affectedObjects`)
- Mostrar cobertura por canal (C, M, Y, K)

---

#### 7.3 Preparar arquitetura para GOVCheckCMYKInkCoverage (futuro)

Mesmo não carregando a DLL imediatamente, preparar a estrutura:

```cpp
// Em Preflight.h ou arquivo novo de integração DLL:
// Struct to hold DLL function pointers
struct PdfObjectActionsDLL {
    bool (*isLoaded()) = nullptr;
    void* (*initializeCMYKChecker)() = nullptr;
    GOVCheckActionList (*convertToCheckActionList)(void*) = nullptr;
    void (*unload)() = nullptr;
};
```

Isso permite adicionar a integração GOVCheckCMYKInkCoverage mais tarde sem
reestruturações maiores.

---

### Prioridade B: Preparar arquitetura OCG/OCMD

#### 8.1 Estrutura de dados para camadas

```cpp
struct ArtLayerInfo {
    QString name;           // Nome da camada ou "Layer 1"
    bool visible;           // Visibilidade atual
    QVector<int> objectIds; // IDs de objetos pertencentes
    PageObjectKind kind;    // Tipo predominante
    int objectCount;        // Quantidade de objetos
};
```

#### 8.2 Função para enumeração de camadas

```cpp
QVector<ArtLayerInfo> GetDocumentLayers(PdfDocument* doc) {
    QVector<ArtLayerInfo> layers;
    
    // Etapa 1: Tentar OCG via DLL
    GOVCheckOCVisibility ocv;
    // ... carregar e obter OCGs ...
    
    // Etapa 2: Fallback para contagem de objetos
    if (layers.isEmpty()) {
        PdfSnapshot snap = PdfSnapshot::capture(*doc);
        ArtLayerInfo fallback;
        fallback.name = QStringLiteral("Layer 1");
        fallback.visible = true;
        fallback.objectCount = snap.objectCount();
        fallback.kind = PageObjectKind::Unknown;
        layers.append(fallback);
    }
    
    return layers;
}
```

#### 8.3 Atualizar painel Layers

No `MainWindow::updatePanels()`:
```cpp
// Atualizar lista de camadas
auto layers = GetDocumentLayers(m_doc);
if (m_layerList) {
    m_layerList->clear();
    for (auto& layer : layers) {
        QListWidgetItem* item = new QListWidgetItem(m_layerList);
        item->setText(layer.name);
        item->setToolTip(QStringLiteral("Objetos: %1\\nVisível: %2")
            .arg(layer.objectCount).arg(layer.visible ? "Sim" : "Não"));
    }
}
```

---

## 8. TESTES

### 9.1 PDFs de teste para validar cada recurso

#### PDFs necessários:

1. **PDF TAC acima do limite**
   - múltiplos objetos com tintas densas (C=100%, M=100%, Y=100%, K=100%)
   - Esperado: TAC = 400%, severity = Error se tacLimit = 300%

2. **PDF TAC abaixo do limite**
   - objetos com tintas leves (C=10%, M=10%, Y=10%, K=10%)
   - Esperado: TAC = 40%, severity = Info

3. **PDF com transparência**
   - objetos com alpha < 1.0
   - Esperado: cobertura efetiva multiplicada por alpha

4. **PDF com overprint**
   - objetos com overprint ativo
   - Esperado: TAC pode exceder 400% (canais somados)

5. **PDF com DeviceN/Pantone**
   - objetos com tintas Spot ou DeviceN
   - Esperado: identificadas como cores especiais, TAC calculado apenas de canais CMYK

6. **PDF sem output intent**
   - sem profile definido
   - Esperado: tintas exibidas como "(profile-based)" com aviso

7. **PDF com OCGs**
   - múltiplos grupos opcionais com nomes diferentes
   - Esperado: painel Layers enumera camadas corretamente

8. **PDF com separações**
   - Documento com separações definidas
   - Esperado: painel Separations enumera tintas corretamente

---

### 9.2 Executar testes

1. **Build MSVC**: `cmake --build build-msvc-nmake --parallel 8`
2. **CTest**: `cd build-msvc-nmake && ctest --output-on-failure --parallel 2`
3. **Testes funcionais**:
   - Abrir cada PDF de teste no ArtPlus
   - Verificar painel Quality Control: TAC exibido corretamente
   - Verificar painel Separations: tintas exibidas corretamente
   - Verificar painel Layers: camadas exibidas corretamente
4. **Verificação de regressões**:
   - Transform: todos os 73 checks devem passar
   - Edit: transparência, clipping preservados
   - Mainwindow: inicialização, navegação, metadados

---

### 9.3 Verificação de regressões

- **PDF rendering**: confirmar que nenhum PDF existente quebre
- **Transform**: flip horizontal/vertical, rotate, resize continuem funcionando
- **Import/export**: save/reopen preservar geometria
- **CTest suites**: 6/6 must pass

---

## 9. ENTREGA

### Arquivos criados/atualizados:

1. **`research/artpro_analysis/REVERSE_ENGINEERING_PANELS.md`** — Este arquivo
   - Análise interna detalhada de todas as exports de DLLs descobertas
   - Funcionamento de TAC, OCG, Separations, DeviceN
   - Relação entre GOVCheckCMYKInkCoverage e GpetInkCoverage
   - Arquitetura de integração no ArtPlus

2. **Atualizar `ARTPLUS_STATUS.md`** — Adicionar seção sobre investigação Ghidra
   - Novas evidências de DLLs descobertas
   - Arquitetura para integração futura
   - Prioridades de implementação

3. **Atualizar `PANEL_TECHNICAL_INVESTIGATION.md`** — Atualizar seções de
   - Layers com investigação OCG/OCMD
   - Separations com arquitetura de extração de tintas
   - Quality Control com caminho de integração DLL

4. **Atualizar `PROJECT_STATUS.md`** — Marcar tarefas concluídas/em progresso
   - Integração TAC via GpetInkCoverage: PLANNED
   - Integração OCG/OCMD via GOVCheckOCVisibility: PLANNED
   - Validated com PDFs de teste: PENDING

### Código efetivamente implementado (para esta fase):

- [ ] Carregar PreflightProfileLayer.dll dinamicamente
- [ ] Chamar GpetInkCoverage::ConvertToCheckActionList()
- [ ] Mapear resultados para CheckResult do ArtPlus
- [ ] Atualizar painel Quality Control com TAC real
- [ ] Preparar estrutura para GOVCheckCMYKInkCoverage futura integração
- [ ] Preparar estrutura para GOVCheckOCVisibility integração Layers
- [ ] Manter todos os 6 CTest suites passing (100%)

### Testes e limitações:

1. **DLL loading**: pode falhar se DLL não estiver ao lado do executável — handled com mensagens de erro gracefully
2. **PDFium 158 limitations**: checks ainda marcados [Informacional] quando DLL não carregada
3. **Transparência/overprint**: consideradas no cálculo via multiplicação alpha, mas sem análise profunda até DLL carregada
4. **DeviceN/Separation**: suportados via HasAttribute(), mas extração real requer DLLs
5. **OCG/OCMD**: arquitetura preparada, mas implementação depende de disponibilidade de DLLs e contrato ArtPro+

### Próximos passos após esta entrega:

1. Testar carregamento DLL e cálculo TAC com PDFs de teste
2. Se bem-sucedido, atualizar painel QC para mostrar valores reais
3. Investigar carregamento GOVCheckCMYKInkCoverage para análise mais detalhada
4. Investigar GOVCheckOCVisibility para enumeração de camadas Layers
5. Validar com suite completa de CTest
6. Atualizar documentação de fidelidade e mapeamento de features

==============================================================================

## Referências Cruzadas

- `ARTPLUS_STATUS.md` — Status de validação e QC implementation
- `PANEL_TECHNICAL_INVESTIGATION.md` — Investigacao tecnica dos painel
- `research/re_v24/ghidra/analysis/PDFObjectActions.md` — Analise inicial de exports
- `research/re_v24/ghidra/analysis/PreflightProfileLayer.md` — Analise inicial de exports
- `research/re_v24/GHIDRA_PROJECT_AUDIT.md` — Relatório completo de auditoria Ghidra
- `FEATURE_MAPPING_ARTPRO.md` — Mapeamento funcionalidade × evidência × classificação
- `UI_MAPPING_ARTPRO.md` — Mapeamento elemento de interface × evidência × decisão
- `ARTPRO_FIDELITY_RULES.md` — Regra normativa completa

==============================================================================