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

Casos cobertos pela suíte `artplus.geometry` (169 checks):

- coordenadas negativas;
- polígonos degenerados (colineares, 1 e 2 vértices, vazio);
- segmentos paralelos, colineares coincidentes e apenas encostados;
- curvas com pontos de controlo coincidentes (incluindo todos coincidentes);
- polígonos horários e anti-horários;
- pontos exatamente sobre arestas e vértices, e a `kGeometryEpsilon` de distância;
- transformações não invertíveis (determinante 0);
- magnitudes muito pequenas (1e-6 pt) e muito grandes (1e9 pt).

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
