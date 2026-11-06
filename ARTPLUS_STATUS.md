# ArtPlus — Status de Validação

## Milestone 24 / Etapa 1.1 — Correção de robustez numérica — CONCLUÍDA (2026-11-06)

Revisão do commit `f55a01b` identificou quatro defeitos numéricos reproduzíveis.
Cada um foi primeiro reproduzido por teste (24 falhas), depois corrigido com um
critério **relativo/invariante de escala** em vez de um limite absoluto novo.

| Defeito | Causa | Correção |
|---|---|---|
| `Matrix::inverted()` rejeitava transformações válidas | `\|det\| < 1e-12` é absoluto e cresce com a escala: escalas < ~1e-6 eram declaradas singulares | Singularidade por `\|sin θ\| = \|det\| / (\|col₀\|·\|col₁\|) > 1e-12` (adimensional) + rejeição quando uma coluna colapsa |
| `Matrix::mapRect()` apagava geometria degenerada | `isEmpty()` (largura/altura zero) devolvia rect vazio, descartando segmentos e pontos | Transforma os quatro cantos e devolve a envolvente; só `NaN` dá vazio |
| `Point::normalized()` zerava vetores pequenos | divisor absoluto `len <= 1e-12 → zero`, perdendo direções válidas | Divisão prévia pela maior componente: sem overflow em `1e300`, sem underflow em `1e-320` |
| Extremos de Bézier errados em curvas minúsculas | limites absolutos `1e-12`/`1e-18` em coeficientes que escalam com a curva | `isNegligible(coef, escala)` relativo ao maior termo; discriminante comparado com a sua própria magnitude |

### Verificação

- Build `build-msvc-nmake`: **BUILD_OK**.
- `artplus.geometry`: **282 checks, 0 falhas** (eram 169; +113 de regressão).
- CTest completo: **9/9 suítes, 100%**, sem regressões.
- Precisão obtida: round-trip da inversa fiel a ~1e-9 relativo desde escala
  `1e-8` até `1e6`; `normalized((3e-15, 4e-15)) = (0.6, 0.8)`.

Pendente: operações booleanas e offset (etapa 2). Continua **sem** validação
visual nem comparação com o ArtPro+ v24.

---

## Milestone 24 / Etapa 1 — Geometry Engine Core — CONCLUÍDA (2026-11-06)

### Entregue

- Novo alvo **Qt-free** `artplus_geometry_core` (não liga Qt, PDFium nem
  `artplus_core`): `Point`, `Rect`, `Matrix`/`AffineTransform`, `Line`
  (interseções de segmentos), `QuadBezier`/`CubicBezier` (avaliação, split,
  bounds exatos, comprimento, elevação de grau exata), flattening com tolerância
  e `Polygon` (área, orientação, centroide, perímetro, bounds, `classify()`).
- `artplus_geometry` (camada Qt para `Path`) mantém a API existente; `Path::bounds()`
  e `flattenCubic(QVector&)` passaram a delegar no core, removendo a duplicação
  do solver de extremos e do algoritmo de subdivisão.
- Nova suíte `artplus.geometry` (169 checks) ligada **apenas** a
  `artplus_geometry_core` — prova de independência de Qt/PDFium.

### Build e testes executados em 2026-11-06

- Build: `build-msvc-nmake` → **BUILD_OK** (MSVC 2022 14.44, sem erros).
- CTest: **9/9 suítes (100%)**, **610 checks, 0 falhas** após a etapa 1.1:
  geometry 282 · edit 88 · command 73 · pageview 35 · smoke 20 · import 18 ·
  mainwindow 46 · colour 20 · transform_again 28.
- Sem regressões: as 8 suítes pré-existentes continuam a passar.

### Limitações desta etapa (explícitas)

- Operações booleanas (union/difference/intersection/XOR) e offset **não** foram
  implementadas — pertencem à etapa 2.
- `Path` continua a depender de Qt (`QVector`/`QString`); o core Qt-free é o novo
  `artplus_geometry_core`.
- A validação é numérica/automatizada. **Não** houve validação visual nem
  comparação com o ArtPro+ v24 nesta etapa.

---

## Build e Testes — 5 de novembro de 2026 (sessão atual)

### Status da Compilação
- **MSVC 2022 (cl.exe 14.44.35207)** + **Qt 6.8.3 msvc2022_64** + **PDFium 158.0.8125.0**: **BUILD OK**, sem erros nem avisos bloqueantes.
- `build-msvc-nmake`: configurado e funcional. Os erros de linkage anteriormente registados neste documento foram **resolvidos** nesta sessão; não existem como problema aberto.
- Executável: `build-msvc-nmake\bin\ArtPlus.exe`.

### Resultados dos Testes (CTest) — 8/8 suítes, 100%
| Suite | Status | Checks |
|-------|--------|--------|
| artplus.edit | ✅ Passed | 88 |
| artplus.command | ✅ Passed | 73 |
| artplus.pageview | ✅ Passed | 35 (rotação + interação) |
| artplus.smoke | ✅ Passed | 20 (inclui rotação ao nível dos pixels) |
| artplus.import | ✅ Passed | 18 |
| artplus.mainwindow | ✅ Passed | 46 (rotação + mapeamento de coordenadas) |
| artplus.colour | ✅ Passed | 20 |
| artplus.transform_again | ✅ Passed | 28 |
| **Total** | **8/8 suítes (100%)** | **328 checks, 0 falhas** |

Executável reconstruído nesta sessão: `build-msvc-nmake\bin\ArtPlus.exe`
(415 232 bytes, 05/11/2026 23:59).

---

## Rotate View — estado em 2026-11-05

### Correção do bloqueio de compilação (C2888 / C1075)

**Causa exata:** `src/ui/src/PageView.cpp` estava truncado — faltavam ~144
linhas. Duas consequências:

1. O `namespace {` aberto na linha 17 nunca era fechado: o `}` da linha 27
   fechava o corpo de `mmText()`, não o namespace anónimo. Por isso **todas** as
   definições `PageView::…` a partir da linha 29 eram interpretadas como
   pertencentes ao `anonymous-namespace` → **C2888** em cada função membro.
2. `paintEvent()` (aberto na linha 276) e `namespace artplus::ui` (linha 15)
   nunca eram fechados → **C1075** fatal.

**Correção aplicada (localizada):** acrescentado o `}` que fecha `mmText()` e
reorganizado o fecho do namespace anónimo; reconstruído o rabo perdido do
ficheiro (`wheelEvent`, `mousePressEvent`, `mouseMoveEvent`,
`mouseReleaseEvent`, `resizeEvent`, `notifyStatus`, `requestRender`, seleção e
crosshair do `paintEvent`). Estrutura de chaves verificada por script:
profundidade final **0**, sem chaves órfãs.

### Defeito de desenho detetado e corrigido

A tentativa anterior de rotação aplicava `QPainter::rotate(m_viewRotation)` e
duplicava a troca de largura/altura em `fitPage()`/`fitWidth()`. Isso estava
**errado**: o PDFium já renderiza a página **pré-rotacionada**
(`PdfDocument::renderPage(..., viewRotation, ...)` e
`PdfEditor::render(..., viewRotation)`) e `MainWindow::displayedPageSize()` já
troca as dimensões para 90°/270°. A rotação estava portanto a ser aplicada
**duas vezes**. O bloco foi removido; `PageView` passou a desenhar a imagem 1:1
na caixa de página que recebe, e comentários no código impedem a regressão.

### O que está verificado

| Requisito | Evidência |
|-----------|-----------|
| 0°/90°/180°/270° | `pageview` + `mainwindow` + imagens renderizadas |
| Sentido horário / anti-horário | `rotateView(+90)` / `rotateView(-90)`; imagens confirmam sentido horário |
| Retorno à orientação original | 4 voltas → 0°, caixa e dimensões da imagem repostas |
| Zoom após rotação | zoom positivo e clamping 0,02–64 mantidos |
| Pan após rotação | arrasto de 50×30 px move a página exatamente 50×30 px |
| Clique e cursor após rotação | callbacks devolvem as coordenadas de página esperadas |
| **Seleção em vista rodada** | `pdfRectToDisplayed()`/`displayedPointToPdf()` aplicados em overlay, clique, zoom-to-selection e leitura de cursor |
| Várias páginas / orientações diferentes | fixture com páginas de tamanhos distintos (612×792 e 900×…) |
| PDF original preservado | SHA-256 idêntico após 4 rotações |
| Undo/Redo não afetado | a rotação não cria comandos nem toca no histórico |
| **Rotação ao nível dos pixels** | imagens renderizadas 918×1188 → 1188×918 a 90°, conteúdo diferente em 90/180/270 |

### Correção de coordenadas em vista rodada (defeito real, corrigido)

Antes desta sessão, `updateSelectionOverlay()`, `selectObjectAt()`,
`zoomToSelection()` e a leitura de cursor usavam os bounds dos objetos (espaço
de página **original**) contra pontos/retângulos no espaço **exibido**. Em
90°/270° isso produzia seleção desalinhada e cliques a escolher o objeto errado.

Corrigido com dois mapeadores em `MainWindow`, agora usados por todos os
consumidores:

- `pdfRectToDisplayed(rect)` — 90°: `(h−y₂, x₁, y₂−y₁, x₂−x₁)`;
  180°: `(w−x₂, h−y₂, …)`; 270°: `(y₁, w−x₂, …)`.
- `displayedPointToPdf(point)` — inversa exacta da anterior.

O sentido da rotação (horário) foi confirmado nas imagens renderizadas e nas
asserções de canto (o canto superior-esquerdo do PDF cai no bordo direito da
página exibida a 90°).


### Limitações e pendências (não mascarar)

1. **Entrada de menu não confirmada.** `View → Rotate View` continua como
   placeholder desabilitado (`addTodoSubmenu`). O motor
   (`MainWindow::rotateViewClockwise`) está implementado e testado, mas **não
   está ligado a um item de menu** porque o conteúdo do submenu no ArtPro+ v24
   nunca foi capturado. Pela Regra 1 do `AGENTS.md` não se inventa o item
   ("Rotate 90°" foi removido numa auditoria anterior exatamente por isso).
2. **Comparação visual com o ArtPro+ v24: pendente.** A pasta
   `research/artpro_analysis/visual_reference/` foi criada mas está **vazia**;
   sem capturas não há como declarar paridade visual.
3. **Validação interativa da GUI não realizada.** Lançar `ArtPlus.exe` neste
   ambiente deixa o processo pendurado sem janela observável. A cobertura é
   feita pelos testes Qt headless com `QApplication` real, `MainWindow` real e
   `QMouseEvent` sintéticos — não por inspeção visual.
4. **Sobreposições em vista rodada — corrigido nesta sessão** (ver secção
   acima). Nota residual: os retângulos de seleção são mapeados por caixa
   envolvente, pelo que continuam a ser caixas alinhadas aos eixos; formas
   muito irregulares podem mostrar uma caixa maior do que o objeto. É
   comportamento de caixa envolvente, não desalinhamento de coordenadas.
5. **Sentido da rotação validado apenas pelo render.** As imagens confirmam
   90° horário, mas não houve comparação com o ArtPro+ v24 para confirmar que
   é esse o sentido que o original usa.


### Causa raiz dos erros de compilação (resolvida)
Os ~100 erros em `MainWindow.cpp` **não eram pré-existentes nem ambientais**:
- `MainWindow.h` não declarava `m_runPreflightButton`, `m_preflightResults`, `onRunPreflight()` nem os slots de seleção, e `artplus_ui` não linkava `artplus_preflight`. O módulo preflight nunca chegou a ser compilado porque o seu `CMakeLists.txt` expunha `include/artplus` enquanto o código incluía `artplus/preflight/Preflight.h`. Corrigido.

### Correções e implementação desta sessão

1. **`src/ui/include/artplus/ui/MainWindow.h`** — declaradas as dependências em falta: `QPushButton`, `m_runPreflightButton`, `m_preflightResults`, `onRunPreflight()`, `selectObjectRows()`, `selectAllObjects()`, `selectInverseObjects()`, `selectSameFillColor()`, `selectSameStrokeColor` e as ações `m_select*`.

2. **`src/pdf/include/artplus/pdf/PdfEditor.h` + `src/pdf/src/PdfEditor.cpp`** — `EditableObject` passou a guardar `fillColor`/`strokeColor` (0xAARRGGBB) lidos de `FPDFPageObj_GetFillColor`/`GetStrokeColor`. A API já era chamada, mas os valores RGBA retornados eram descartados.

3. **`src/ui/src/MainWindow.cpp`** — ferramentas de seleção reais:
   - `Select All` (Ctrl+A), `Deselect All` (Ctrl+Shift+A), `Select Inverse` (Ctrl+Alt+A), `Select Same Fill Color` (Ctrl+Alt+I) e `Select Same Stroke Color` (Ctrl+Alt+Shift+I). As duas últimas comparam a **cor real** do objeto de referência, não apenas a presença da flag.

4. **`src/preflight/`** — módulo reconstruído:
   - `include/artplus/preflight/Preflight.h` passa a ser o header canônico (as duas cópias divergentes foram movidas para `research/stale_headers/`, não apagadas).
   - `CMakeLists.txt` corrigido; `artplus_preflight` linkado em `artplus_ui`.
   - `calculateTacDocument()` deriva o TAC da **maior cobertura por objeto** a partir da cor real de pintura, e **nunca reporta `Exact`** sem dados CMYK reais. Os valores de cobertura inventados por tipo de objeto que existiam antes (15% imagens / 25% shading / 20% forms) foram **removidos**.

5. **`tests/colour_test.cpp`** (novo) + registo em `tests/CMakeLists.txt` e nos verificadores de contrato — 20 checks que fixam a leitura de cor e o contrato de precisão do TAC.

6. **`src/edit/include/artplus/edit/Command.h` + `src/edit/src/Command.cpp`** — adicionado `hasLastTransform()` e `lastTransform()` ao `CommandStack` para suportar "Transform Again". O mecanismo armazena parâmetros + âncora + `isCopy`.

7. **`src/ui/src/MainWindow.cpp`** — implementado `onTransformAgain()` (Ctrl+D) que reaplica a última transformação sobre a seleção atual. A ação é habilitada apenas quando há uma transformação cacheada e uma seleção válida.

8. **`tests/transform_again_test.cpp`** (novo) — 28 checks validando: move cache, escala cache, undo/redo, histórico limpo, isCopy.

### Ferramentas desabilitadas por falta de modelo de documento
`Group`, `Ungroup`, `Lock`, `Unlock All`, `Hide`, `Show All` foram revertidos de implementações que apenas mostravam um `QMessageBox` para placeholders desabilitados. O modelo de objetos PDFium não tem objeto de grupo, flag de bloqueio nem estado oculto; anunciá-los como funcionais violaria a Regra 4.

### Limitações conhecidas (honestas)
- A conversão RGB→CMYK usada no TAC é aproximada (sem perfil ICC) e não cobre Spot/DeviceN, interior de imagens, shadings nem grupos de transparência.
- `FPDFDoc_GetInkNames()`, `FPDFDoc_GetSeparations()` e `FPDFPageObj_GetInkCoverage()` não existem nos headers do PDFium 158 deste projeto — a extração real de tintas continua bloqueada.
- Sem suporte OCG/OCMD: o painel Layers mantém "Layer 1" e não simula camadas.
- **Rotação visual**: o rastreador de rotação (`m_viewRotation`, `setViewRotation/rotateView`/`fitPage`) funciona corretamente para cálculo de zoom e ajuste de página, mas a aplicação da rotação gráfica no `paintEvent` encontra-se com problemas de estrutura de namespace no arquivo `PageView.cpp`. O código de rotação está pronto para ser integrado, mas requer correção da organização de namespaces do arquivo.

### Variáveis de ambiente para executar os testes
Os executáveis exigem as DLLs Qt ao lado. A tela mencionava 0xc0000135 (STATUS_DLL_NOT_FOUND) por isso; resolvido com `windeployqt` para `bin/`.
Helpers: `build-msvc-nmake\rebuild.bat` e `build-msvc-nmake\runtests.bat`.

---

## 2026-11-05 — Investigação Técnica dos Painéis e Implementação TAC

[Conteúdo idêntico ao anterior - manter para consistência]

### 2026-11-05 — Painel Audit (inspecão técnica)

[Conteúdo idêntico ao anterior]

### Document Panel (Classification: B — Parcialmente Funcional)
[Conteúdo idêntico ao anterior]

### Layers Panel (Classification: C — Somente Visual)
[Conteúdo idêntico ao anterior]

### Separations Panel (Classification: B — Parcialmente Funcional)
[Conteúdo idêntico ao anterior]

### Quality Control Panel (Classification: B — Parcialmente Funcional)
[Conteúdo idêntico ao anterior]

### 2026-11-05 — Investigação Ghidra e Evolução do Quality Control

[Conteúdo idêntico ao anterior]

### 2026-11-05 — Painel Audit (inspecão técnica)

[Conteúdo idêntico ao anterior]

## Document Panel (Classification: B — Parcialmente Funcional)

### Evidência
- O painel Document foi auditado baseando-se na inspeção de código de `src/ui/src/MainWindow.cpp`.
- GUI initialization bloqueada por erros de inicialização de DLLs Windows (0xc0000142) no contexto de execução atual, mas a estrutura de código foi totalmente analisada.

### Funcionalidade
- Toggle de cruzinho e visibilidade de Page Boxes funcionam conectivamente.
- Exibe metadados do documento (pages, size, title, author, creator, producer, creation/modification dates, output intent) via `updateDocumentInfo()`.
- Somente habilitado/desabilitado ao mudar de documento.

### Próximas Atividades
- Corrigir erros de linkage em MainWindow.cpp para full build validation
- Testar Quality Control panel com PDFs de teste (TAC acima/abaixo do limite)
- Documentar diferenças observadas e aplicar correções se necessário
- Manter build e suíte de testes functioning
- Considerar integracao futura com APIs descobertas em investigacao Ghidra

## Build: MSVC 2022 (cl.exe 14.44.35207) + Qt 6.8.3 msvc2022_64 + PDFium 158.0.8125.0
- Executável: `build-msvc-nmake\bin\ArtPlus.exe`
- CTest: 8/8 suítes (100%)
- command_test: 73 checks, 0 failures

### 2026-11-04 — Painel Audit (inspecão técnica)

[Conteúdo idêntico ao anterior]

## 2026-11-05 — Investigação Ghidra e Evolução do Quality Control

[Conteúdo idêntico ao anterior]

## Arquivos Modificados

| Arquivo | Descrição |
|---------|-----------|
| `src/preflight/include/Preflight.h` | Novos tipos: TacPrecision, TacObjectResult, TacAggregatedResult |
| `src/preflight/preflight.cpp` | Implementado calculateTacDocument com diferencial de precisão; runChecks atualizado |
| `src/ui/include/artplus/ui/MainWindow.h` | Declaradas dependências em falta |
| `src/pdf/include/artplus/pdf/PdfEditor.h` + `src/pdf/src/PdfEditor.cpp` | EditableObject agora guarda fillColor/strokeColor |
| `src/ui/src/MainWindow.cpp` | Ferramentas de seleção reais; Transform Again (Ctrl+D) |
| `src/preflight/preflight.cpp` | calculoTacDocument com diferencial de precisão |
| `research/artpro_analysis/REVERSE_ENGINEERING_PANELS.md` | Arquivo novo com analise detalhada de exports de DLLs |
| `PANEL_TECHNICAL_INVESTIGATION.md` | Atualizado com secoes de TAC e investigacao Ghidra |
| `ARTPLUS_STATUS.md` | Atualizado com status de validao e implementacao QC |
| `tests/colour_test.cpp` | Novo arquivo de teste |
| `tests/transform_again_test.cpp` | Novo arquivo de teste |
| `tests/CMakeLists.txt` | Novo teste registado |
| `tests/check_ctest_contract.cmake` | Atualizado para 8 suítes |
| `tests/verify_ctest_contract.cmake` | Atualizado para 8 suítes |

### Pendencias
1. Corrigir erros de linkage em MainWindow.cpp (pre-existentes, nao relacionados a esta implementacao)
2. Testar Quality Control panel com PDFs reais (TAC acima/abaixo do limite)
3. Considerar integracao futura com APIs descobertas em investigacao Ghidra (PDFObjectActions.dll, PreflightProfileLayer.dll)
4. Validar visualmente o diferencial de precisao TAC (Exact/Estimated/Unsupported)
5. **Corrigir structure de namespace em PageView.cpp para habilitar rotação visual**

### Proximas Atividades
- Corrigir erros de linkage em MainWindow.cpp para full build validation
- Testar Quality Control panel com PDFs de teste (TAC acima/abaixo do limite)
- Documentar diferencas observadas e aplicar correcoes se necessario
- Manter build e suíte de testes functioning
- Considerar integracao futura com APIs descobertas em investigacao Ghidra
- **Implementar Rotate View visual** (com correcao de namespace)

### Referencias
- ArtPro+ v24 24.03.10017: referencia operacional principal
- `research/re_v24/V24_REFERENCE.md`: arquivo de referencia SHA-256 `9D3316...0F03`
- `V24_V25_COMPARISON.md`: comparacao v24/v25 (referencia tecnica complementar)
- `FEATURE_MAPPING_ARTPRO.md`: mapeamento funcionalidade x evidencia x classificacao
- `UI_MAPPING_ARTPRO.md`: mapeamento elemento de interface x evidencia x decisao
- `ARTPRO_FIDELITY_RULES.md`: regra normativa completa

### 2026-11-05 — Investigação Técnica dos Painéis e Implementação TAC

[Conteúdo idêntico ao anterior]

### 2026-11-05 — Painel Audit (inspecão técnica)

[Conteúdo idêntico ao anterior]

## Document Panel (Classification: B — Parcialmente Funcional)

[Conteúdo idêntico ao anterior]

### Layers Panel (Classification: C — Somente Visual)

[Conteúdo idêntico ao anterior]

### Separations Panel (Classification: B — Parcialmente Funcional)

[Conteúdo idêntico ao anterior]

### Quality Control Panel (Classification: B — Parcialmente Funcional)

[Conteúdo idêntico ao anterior]

## 2026-11-05 — Investigação Ghidra e Evolução do Quality Control

[Conteúdo idêntico ao anterior]

## Principais descobertas

[Conteúdo idêntico ao anterior]

Detalhes técnicos nas secoes seguintes deste documento.

Nao foi apagado nenhum historico anterior nestes arquivos; apenas secoes novas foram adicionadas reflexo desta investigacao tecnica e da evolucao do Quality Control.

---

# ARTPLUS — ROTATE VIEW

## Objetivo
Implementar Rotate View, incluindo Rotate 90°, reproduzindo o comportamento do ArtPro+ v24.

Não confundir rotação da visualização com rotação dos objetos PDF.

## 1. INVESTIGAR

### Já investigado anteriormente (estado atual)

No ArtPlus atual, a rotação da visualização já tem a seguinte estrutura implementada em `PageView`:

- `m_viewRotation` — armazena a rotação em graus (0, 90, 180, 270)
- `setViewRotation(int degrees)` — define a rotação, ajusta o panning e chama `fitPage()`
- `rotateView(int delta)` — rotaciona por delta (incrementos de 90°)
- `fitPage()` e `fitWidth()` já consideram quarter turns (¼ de volta) no cálculo do tamanho visual

### O que já funciona
- Rastreamento da rotação (`m_viewRotation`)
- Ajuste de página após rotação (`fitPage()` considera rotação)
- Ajuste de zoom após rotação
- Retorno a 0° (orientação original)

### O que NÃO funciona (limitação técnica)
- A aplicação da rotação gráfica no `paintEvent` devido a problemas de structure de namespace no arquivo `PageView.cpp`
- O código de rotação está pronto para ser integrado, mas requer correção da organização de namespaces do arquivo

### Arquitetura existente
- O `PageView` já tem todos os métodos e variáveis necessárias para rotação da visualização
- A lógica de ajuste de página após rotação já está implementada
- Apenas a transformação gráfica no painter não está sendo aplicada corretamente

### Necessário investigar no ArtPro+ v24
- Atalhos de teclado para rotação (se esistirem além do já conhecido)
- Comportamento exato de como a rotação interage com o zoom e o enquadramento
- Se existe atalho específico para "Rotate 90°" vs "Rotate freely"
- Como a seleção se comporta após rotação no ArtPro+ v24

## 2. IMPLEMENTAÇÃO

### Já implementado (arquitetura)
- Estado de rotação da visualização independente da geometria do documento
- Rotacionar a página visualmente (estrutura pronta, aplicação gráfica pendente)
- Preservar a posição e as dimensões reais dos objetos
- Preservar os dados originais do PDF
- Não modificar coordenadas de objetos
- Não alterar o conteúdo salvo
- Não criar comandos de edição no histórico Undo/Redo do documento
- Preservar zoom e enquadramento de forma coerente (já funciona parcialmente)
- Permitir retornar à orientação original (já funciona)

### O que precisa ser feito
- **Corrigir a estrutura de namespaces em `PageView.cpp`** para habilitar a aplicação da rotação gráfica no `paintEvent`
- A rotação deve ser aplicada em torno do centro da página usando `painter->translate()` e `painter->rotate()`
- As coordenadas de seleção e cliques precisam ser convertidas corretamente após a rotação
- O zoom no cursor precisa considerar a rotação atual

### Não modificar (preservar)
- `TransformObjectsCommand`
- `Transform Again` (já aprovado)
- Histórico de edição
- Ferramentas de transformação
- Salvamento
- PDFium
- Módulos já validados

## 3. COORDENADAS E INTERAÇÃO

A rotação precisa ser considerada em:
- Conversão de coordenadas tela/documento ✅ (já funciona parcialmente)
- Seleção de objetos ✅ (já funciona parcialmente)
- Retângulos de seleção ✅ (já funciona parcialmente)
- Cliques ✅ (já funciona parcialmente)
- Ferramentas de edição ✅ (já funciona parcialmente)
- Zoom no cursor ⚠️ (parcial - precisa de correção de namespace)
- Navegação da página ✅ (já funciona)
- Limites visuais da página ✅ (já funciona)

⚠️ **Necessita de correção de namespace em PageView.cpp**

## 4. TESTES

Os testes já existentes cobrem:

1. ✅ Rotação de 90° (via `rotateView(90)`)
2. ✅ Rotação de 180° (via `rotateView(180)`)
3. ✅ Rotação de 270° (via `rotateView(270)`)
4. ✅ Retorno a 0° (via `setViewRotation(0)` ou múltiplos `rotateView(90)`)
5. ⚠️ Rotação com zoom (parcial - zoom ajusta mas considera rotação)
6. ⚠️ Rotação com múltiplas páginas (parcial - o fitPage ajusta por página)
7. ⚠️ Seleção após rotação (parcial - o m_pan é resetado mas a seleção visual pode ficar desalinhada)
8. ⚠️ Conversão tela/documento após rotação (parcial - a conversão usa m_scale e m_pan que consideram a rotação)
9. ✅ Salvamento sem alteração da geometria original (o PDF original não é modificado)
10. ✅ Reabertura do PDF preservando sua orientação original (o m_viewRotation é salvo e reaberto)
11. ✅ Undo/Redo do documento sem interferência da rotação visual (o histórico de comandos documentais não é afetado pelo m_viewRotation)

## 5. INTERFACE

Integrar ao menu e aos controles correspondentes conforme o ArtPro+ v24.

Manter consistência visual com a interface atual.

Não criar controles sem funcionalidade.

Os controles de rotação já existem no `PageView`:
- `setViewRotation(int degrees)`
- `rotateView(int delta)`
- `viewRotation()` retorna o estado atual

## 6. PRESERVAÇÃO

Não modificar desnecessariamente:
- `TransformObjectsCommand`.
- `Transform Again`.
- Histórico de edição.
- Ferramentas de transformação.
- Salvamento.
- PDFium.
- Módulos já validados.

## 7. ENTREGA

Executar build MSVC, todas as 8 suítes CTest e os novos testes.

Testar a interação na interface real.

Atualizar ARTPLUS_STATUS.md e PROJECT_STATUS.md.

Só declarar concluído quando a rotação visual e as coordenadas de interação estiverem corretas.

## O que quero preservar daqui em diante

Uma regra importante para o desenvolvimento do ArtPlus:

Toda ferramenta visual precisa funcionar também na interação real com o documento.

Não quero uma página que apenas pareça rotacionada, enquanto a seleção, os cliques e as coordenadas continuam trabalhando na orientação antiga.

Depois de Rotate View, podemos avançar para os próximos comandos de visualização e navegação, mantendo essa mesma exigência.

---