# artplus::geometry — núcleo geométrico (Milestone 24, etapa 1)

Biblioteca de geometria 2D do ArtPlus. A **etapa 1** entrega o núcleo de
primitivas e curvas, sem operações booleanas e sem offset.

## Independência de Qt e PDFium

O núcleo vive no alvo CMake `artplus_geometry_core`, que **não** liga Qt,
PDFium nem `artplus_core`. A suíte `artplus.geometry` liga apenas a esse alvo,
portanto qualquer `#include` de Qt que entre no núcleo quebra o *link* do teste
e é detectado imediatamente.

```
artplus_geometry_core   (Qt-free)  Point, Rect, Matrix, Line, Bezier, Polygon
        ^
artplus_geometry        (Qt)       Path (QVector/QString) — camada histórica
```

`Path::bounds()` e `flattenCubic(QVector&)` delegam no núcleo, de modo que o
solver de extremos de cúbicas e o algoritmo de subdivisão existem numa única
implementação.

## Convenções

| Tópico | Convenção |
|---|---|
| Unidades | Pontos PDF (1 pt = 1/72 pol). Nenhuma conversão de unidade acontece aqui. |
| Frame | Cartesiano abstrato. O espaço de página do ArtPlus é **y-down**, então uma rotação matematicamente anti-horária aparece horária na tela. |
| Transformação afim | `x' = a·x + c·y + e`, `y' = b·x + d·y + f`. `A * B` aplica **B primeiro**. |
| Área assinada | Shoelace: `A = ½·Σ(xᵢ·yᵢ₊₁ − xᵢ₊₁·yᵢ)`. Positiva ⇒ sentido anti-horário em y-up. Ver nota de y-down acima. |
| `kGeometryEpsilon` | **Distância** absoluta de 1e-9 pt (~3,5e-10 mm) para testes de borda/paralelismo. Só absorve ruído de ponto flutuante. |
| `Polygon::orientation(epsilon)` | `epsilon` é **fator relativo** (área do polígono vs. área da sua bounding box), não distância. Mantém o resultado independente da magnitude das coordenadas. |
| `Polygon::classify(p, epsilon)` | `epsilon` é **distância** absoluta. `Boundary` tem precedência sobre o ray casting. |
| Tolerância de flattening | Desvio máximo em pontos; valores ≤ 0 são limitados a 1e-6 para garantir terminação. |

### Limites numéricos: relativo vs. absoluto

Um limite absoluto (`|x| < 1e-12`) é **invariante de escala apenas quando a
grandeza comparada é ela própria adimensional**. Por isso cada teste usa o
critério adequado:

| Teste | Critério | Justificativa |
|---|---|---|
| `Matrix::inverted()` | `\|sin θ\| = \|det\| / (\|col₀\|·\|col₁\|) > 1e-12` | Adimensional. `\|det\|` absoluto escalava com a transformação e rejeitava escalas < ~1e-6, que são perfeitamente invertíveis em `double`. |
| `Point::normalized()` | divisor = maior componente; zero só quando ambas são 0 | Elimina overflow (`1e300`) e underflow (`1e-320`); nenhum vetor não nulo é descartado. |
| Extremos de Bézier (`Bezier.cpp`) | `64 * ULP(maxCoord)` | Os extremos são calculados a partir dos **diferenciais** da curva, que variam com a escala. Usar ULP local garante robustez sem constantes mágicas como `1e-12`. **Etapa 1.2.3**: tolerâncias fixas `100.0` e `0.9` removidas; todas as comparações agora usam limites baseados em `effectiveUlp()` da magnitude da curva. **Etapa 1.2.4**: denominação do solver agora usa diferenças locais `d1 - d0` (onde `d0 = c - p0`, `d1 = p1 - c`) em vez de coordenadas absolutas `p0 - 2*c + p1`, evitando cancelamento catastrófico sob translação.
| `cubicIsFlat()` (colapso da corda) | `1e-12` relativo à extensão da curva | Indica "corda degenerada", que é adimensional em intenção. |
| `Polygon::orientation()` | `1e-12` relativo à área da bounding box | Ver acima. |
| `kGeometryEpsilon` (bordas, paralelismo) | **distância** absoluta `1e-9` pt | Aqui a grandeza *é* uma distância física; comparar distâncias com distâncias é correto. |

Precisão típica obtida: round-trip `inv(m·p)` fiel a ~1e-9 relativo, inclusive a
escala `1e-8`; normalização exata em `(3e-15, 4e-15)` → `(0.6, 0.8)` e em
`(3e300, 4e300)` sem overflow.

### Degenerado ≠ ausente

`Matrix::mapRect()` transforma os quatro cantos mesmo quando a largura ou a
altura é zero: um segmento vertical/horizontal ou um único ponto mantém a sua
posição após a transformação. `Rect::isEmpty()` continua a reportar esses rects
como vazios (semântica histórica usada por `intersects`/`united`), mas
`mapRect()` **não** usa esse predicado. Só `NaN` produz um rect vazio.

### Invariância à translação

Uma curva não deve mudar de forma só porque foi movida para longe da origem.
Por isso **nenhum limiar de `Bezier.cpp` usa coordenadas absolutas**:

| Cálculo | Como é feito |
|---|---|
| Coeficientes da derivada | Calculados a partir dos **vetores de aresta** `d0 = c1-p0`, `d1 = c2-c1`, `d2 = p1-c2`. Uma translação comum cancela-se exactamente em cada diferença. |
| Magnitude da curva (`scale`) | `max(\|d0\|,\|d1\|,\|d2\|,\|a\|,\|b\|,\|c\|)` — a **extensão geométrica local**, nunca `\|v0\|`/`\|p0\|`. |
| Extremos cúbicos | `B'(t)/3 = A t² + B t + C` com `A = d0-2d1+d2`, `B = 2(d1-d0)`, `C = d0` (verificado simbolicamente). |
| Extremos quadráticos | `B'(t)/2 = d0 + t(d1-d0)` ⇒ `t = -d0/(d1-d0)`. |
| Raízes da quadrática | Fórmula sem cancelamento (`q = -½(b + sign(b)·√Δ)`, segunda raiz por `c/q`) e caso simétrico explícito quando `b == 0`. |
| Discriminante | Comparado com a sua própria magnitude (`b² + \|4ac\|`), não com uma constante. |
| Flatness / subdivisão | Já era relativo a `p0`; passou a usar `localExtent()` em vez de uma expressão ad-hoc. |

### Limite real do IEEE 754

A `double` tem 52 bits de mantissa. Para uma coordenada de magnitude `d`, o
espaçamento entre valores representáveis (ULP) é `≈ d · 2⁻⁵²`:

| Deslocamento | ULP | Um curva de 100 pt ocupa |
|---|---|---|
| 1e6 | 1,2e-10 pt | ~8,6e11 valores |
| 1e9 | 1,2e-7 pt | ~8,4e8 valores |
| 1e12 | 1,2e-4 pt | ~8,2e5 valores |
| 1e15 | **0,125 pt** | **~800 valores** |

Consequência documentada: a `1e15` o ULP (0,125 pt) é **maior** que a tolerância
de flattening usual (0,05 pt). Aí a própria posição dos pontos de controlo está
quantizada mais grosseiramente do que a tolerância pedida, pelo que a forma do
polígono não é reproduzível. Os testes **verificam a invariância só onde ela é
representável** (`ULP ≤ tolerância/2`, isto é, até 1e12) e nas magnitudes
maiores verificam apenas ausência de `NaN`/`infinito`. Esta limitação é um facto
do IEEE 754, não um defeito do algoritmo, e não é mascarada.



## API

### `Point`, `Rect`, `Matrix`

```cpp
using namespace artplus::geometry;

Point p{3.0, 4.0};
double len = p.length();                    // 5
Point unit = p.normalized();                // (0.6, 0.8)

Rect r = Rect::fromXYWH(-10.0, -20.0, 30.0, 40.0);
Rect u = r.united(Rect{0, 0, 10, 10});      // envolvente comum
Rect i = r.intersected(Rect{0, 0, 10, 10}); // Rect vazio quando disjuntos

Matrix m = Matrix::translation(10, -4) * Matrix::rotationDegrees(37)
         * Matrix::scaling(2.0, 0.5);       // escala, depois rotação, depois translação
Point mapped = m.map(Point{3, 7});
bool ok = false;
Matrix inv = m.inverted(&ok);               // ok == false para determinante 0

AffineTransform  t = AffineTransform::translation(1, 2); // alias de Matrix
```

### `Line` e interseções

```cpp
Line a{{0, 0}, {10, 0}};
Line b{{0, 5}, {10, 5}};

LineIntersection hit = intersectSegments(a, b);   // kind == None (paralelas)
LineIntersection cross = intersectSegments({{0,0},{10,10}}, {{0,10},{10,0}});
// cross.kind == Point, cross.point == (5,5), cross.t == 0.5, cross.u == 0.5

LineIntersection over = intersectSegments({{0,0},{10,0}}, {{5,0},{15,0}});
// over.kind == Overlap, over.overlapLength == 5
```

Comportamento garantido para: cruzamento próprio, toque em extremidade,
junção em T, sobreposição colinear, segmentos colineares disjuntos e segmentos
de comprimento zero (tratados como pontos).

### Bézier e flattening

```cpp
CubicBezier c{{0,0}, {0,100}, {100,100}, {100,0}};
Point mid = c.evaluate(0.5);        // (50, 75)
Rect  bb  = c.bounds();             // bottom == 75: extremo da curva, não do polígono de controlo
double arc = c.length(0.01);        // comprimento por flattening

std::vector<Point> polyline;
flattenCubic(c, 0.05, polyline);    // desvio máximo ≤ 0.05 pt
// o primeiro ponto emitido é o fim da curva; empurre c.p0 antes se precisar do início
```

`flattenQuadratic` faz elevação de grau exata para cúbica e reutiliza
`flattenCubic`, portanto há apenas um algoritmo de subdivisão.

### `Polygon`

```cpp
Polygon square{{ {0,0}, {1,0}, {1,1}, {0,1} }};
double a = square.signedArea();          // +1
Orientation o = square.orientation();    // CounterClockwise
Point c = square.centroid();             // (0.5, 0.5)
double per = square.perimeter();         // 4

square.classify({0.5, 0.5});             // Inside
square.classify({0.5, 0.0});             // Boundary (aresta)
square.classify({0.0, 0.0});             // Boundary (vértice)
square.classify({1.5, 0.5});             // Outside
square.contains({0.5, 0.0});             // true: Inside ou Boundary
```

Fallback documentado: para polígonos degenerados (área desprezável face à
bounding box) o centroide é a média aritmética dos vértices e a orientação é
`Degenerate`.

## Robustez

Casos cobertos pela suíte `artplus.geometry` (793 checks):

- coordenadas negativas;
- polígonos degenerados (colineares, 1 e 2 vértices, vazio);
- segmentos paralelos, colineares coincidentes e apenas encostados;
- curvas com pontos de controlo coincidentes (incluindo todos coincidentes);
- polígonos horários e anti-horários;
- pontos exatamente sobre arestas e vértices, e a `kGeometryEpsilon` de distância;
- transformações não invertíveis (determinante 0) e **invertibilidade em escala**
  (`1e-8`, `1e-6`, `1`, `1e6`) com round-trip verificado;
- `mapRect` de rects com largura zero, altura zero e de um único ponto, sob
  translação, rotação e escala;
- `normalized()` para vetor zero, `(3,4)`, `(3e-15, 4e-15)`, `(3e300, 4e300)` e
  subnormal `(1e-320, 0)`, com verificação de ausência de NaN/infinito;
- curvas quadráticas e cúbicas em escalas `1e-21 … 1e9`, verificando bounds,
  avaliação, flattening (dentro da tolerância) e comprimento de arco;
- magnitudes muito pequenas (1e-6 pt) e muito grandes (1e9 pt);
- **invariância à translação** (etapa 1.2): seis famílias de curvas quadráticas e
  cúbicas — controlos distintos com extremos interiores, quase retas, dimensões de
  1 pt, controlos coincidentes, laço com extremos coincidentes, colineares e
  extremo sub-ponto (0,115 pt) — comparadas em `(0,0)`, `1e6`, `1e9`, `1e12` e
  `1e15` quanto a bounds, extremos, avaliação, forma achatada, comprimento e
  ausência de `NaN`/`infinito`. Todas as comparações são feitas em valores
  **relativos ao início da própria curva**, nunca em coordenadas absolutas.

Tolerâncias são explícitas e documentadas; não são usadas para esconder erro
numérico. Comparações de área usam um critério **relativo** à bounding box para
não declarar degenerado um polígono legitimamente pequeno.

## Etapa 1.2.4 — Novos aprimoramentos

- **Ordem de p0**: todos os flattening de curvas agora iniciam com `p0` no início do vetor (`p0 → pontos intermediários → p1`), usando `insert(begin(), p0)` em vez de `push_back`. Curvas transladadas usam `cubic.p0`/`quad.p0` (já traduzidas).
- **Comprimento quadrático**: `baseLen` e `movedLen` recalculados com sequência correta p0→p1, incluindo o primeiro segmento.
- **Testes de polilinhas vazias**: `bidirectionalPolylineDistance()` testado com casos vazia/vazia, vazia/non-vazia, non-vazia/vazia, ponto/ponto, ponto/polilinha, polilinha/ponto e polilinhas com shapes diferentes mas bounding boxes iguais. Entradas vazias retornam -1.0 (comparação inválida) e são rejeitadas nas asserções.
- **Solver quadrático estável**: denominador computado usando diferenças locais (`d1 - d0`) em vez de `p0 - 2*c + p1`, evitando cancelamento catastrófico sob translação.
- **Solver cúbico**: ramo `q == 0` removido contendo comparação impossível `0.0 > 0.0`. Tratamento matemático correto sem constantes arbitrárias `1e-300`. Raízes garantidas finitas, ordenadas e estritamente internas em `(0,1)`.

Tolerâncias são explícitas e documentadas; não são usadas para esconder erro
numérico. Comparações de área usam um critério **relativo** à bounding box para
não declarar degenerado um polígono legitimamente pequeno.

## Não incluído nesta etapa

- Operações booleanas (union, difference, intersection, XOR).
- Offset/inset com miter/bevel/round.
- Limpeza de auto-interseções.
- Geração geométrica de bleed.

## Como compilar e testar

```bat
build-msvc-nmake\rebuild.bat
build-msvc-nmake\runtests.bat
:: apenas o núcleo:
ctest --test-dir build-msvc-nmake -R artplus.geometry --output-on-failure
```
