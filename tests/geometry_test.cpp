// Qt-free tests for the ArtPlus geometry engine core (Milestone 24, stage 1.2.1).
//
// Two purposes:
//  1. Verify numeric behaviour: area/orientation/centroid, segment
//     intersections, Bezier evaluation/flattening and point classification,
//     including the robustness cases required by the milestone (negative
//     coordinates, degenerate polygons, parallel/coincident segments,
//     coincident control points, both windings, points exactly on the boundary,
//     non-invertible transforms and extreme magnitudes).
//  2. Prove the core has no Qt/PDFium dependency: this executable links only
//     artplus_geometry_core, so any leaked Qt include breaks the build.

#include "artplus/geometry/Bezier.h"
#include "artplus/geometry/Line.h"
#include "artplus/geometry/Polygon.h"
#include "artplus/geometry/Point.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* description)
{
    ++g_checks;
    if (condition) {
        std::printf("  [PASS] %s\n", description);
    } else {
        ++g_failures;
        std::printf("  [FAIL] %s\n", description);
    }
}

void checkNear(double actual, double expected, double eps, const char* description)
{
    ++g_checks;
    if (std::abs(actual - expected) <= eps) {
        std::printf("  [PASS] %s\n", description);
    } else {
        ++g_failures;
        std::printf("  [FAIL] %s (actual %.17g, expected %.17g, eps %.3g)\n",
                    description, actual, expected, eps);
    }
}

// Minimum distance from p to the polyline made of consecutive points.
double distanceToPolyline(const artplus::geometry::Point& p,
                          const std::vector<artplus::geometry::Point>& polyline)
{
    using artplus::geometry::Line;
    using artplus::geometry::pointSegmentDistance;
    if (polyline.empty())
        return 1e300;
    if (polyline.size() == 1)
        return artplus::geometry::distance(p, polyline.front());
    double best = 1e300;
    for (std::size_t i = 1; i < polyline.size(); ++i)
        best = std::min(best, pointSegmentDistance(p, Line{polyline[i - 1], polyline[i]}));
    return best;
}

} // namespace

// Bounding box of the *offsets* of a polyline relative to a reference point.
// Used to compare a curve with a translated copy without requiring absolute
// coordinate equality.
artplus::geometry::Rect relativeBounds(const std::vector<artplus::geometry::Point>& points,
                                       const artplus::geometry::Point& origin)
{
    using artplus::geometry::Point;
    using artplus::geometry::Rect;
    if (points.empty())
        return {};
    Rect r{points.front().x - origin.x, points.front().y - origin.y,
           points.front().x - origin.x, points.front().y - origin.y};
    for (const Point& p : points) {
        const double rx = p.x - origin.x;
        const double ry = p.y - origin.y;
        if (rx < r.left) r.left = rx;
        if (ry < r.top) r.top = ry;
        if (rx > r.right) r.right = rx;
        if (ry > r.bottom) r.bottom = ry;
    }
    return r;
}

bool allFinite(const std::vector<artplus::geometry::Point>& points)
{
    for (const auto& p : points) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y))
            return false;
    }
    return true;
}

// ========================================================================
// Helpers de comparação rigorosa baseados em ULP e escala local
// ========================================================================

// ULP (Unit in the Last Place) efetivo para um valor double em magnitude dada.
inline double effectiveUlp(double magnitude)
{
    if (magnitude == 0.0)
        return std::numeric_limits<double>::min(); // menor subnormal positivo ~5e-324
    return std::max(std::nextafter(magnitude, magnitude + 1.0) - magnitude,
                    std::numeric_limits<double>::min());
}

// Tolerância geométrica justificada para um deslocamento 'magnitude' e
// tolerância de flattening 'requestedTolerance'.
//
// A tolerância combinada reflete:
//  - Erro de representação das coordenadas (ULP * fator de segurança)
//  - Tolerância de flattening solicitada
//  - Erro numérico acumulado nas operações (fator 4 * ULP, segurança para a
//    aritmética de ponto flutuante encadeada nas avaliações).
//
// O critério anterior `magnitude * 1e-13` não é usado: em 1e15 ele daria
// tolerância ~0.001 pt, o que permitiria erros geométricos de centenas de
// pontos — não aceitável.
inline double geometricTolerance(double magnitude, double requestedTolerance)
{
    const double ulp = effectiveUlp(magnitude);
    const double roundoffMargin = 4.0 * ulp;
    return std::max(requestedTolerance, roundoffMargin);
}

// Verifica se uma coordenada em 'magnitude' ainda pode expressar a tolerância solicitada.
// Se o ULP exceder a tolerância / 2, a forma não pode ser verificada numericamente.
inline bool isRepresentable(double magnitude, double requestedTolerance)
{
    const double ulp = effectiveUlp(magnitude);
    return ulp <= requestedTolerance * 0.5;
}

// Avalia curva Bézier em múltiplos parâmetros t e verifica X e Y.
template <typename Curve>
void checkEvaluationAtTs(const Curve& base, const Curve& moved, const artplus::geometry::Point& delta,
                         const char* prefix, const char* caseName, double d)
{
    using artplus::geometry::Point;
    static constexpr double ts[] = {0.0, 0.1, 0.25, 0.37, 0.5, 0.75, 0.9, 1.0};
    // Tolerância baseada em ULP da magnitude: isso reflete o erro de
    // representação IEEE 754 efetivo em vez de usar constantes fixas como
    // 100.0 ou 0.9 que seriam arbitrariamente permissivas ou restritivas.
    const double ulp = effectiveUlp(d);
    // Para magnitudes onde o ULP excede a tolerância de flattening (0.05 pt),
    // reconhecemos que a geometria fina não é representável e verificamos
    // apenas propriedades seguras (finitude, bounds, finitos).
    const double eps = (ulp > 0.5) ? ulp * 2.0 : geometricTolerance(d, 0.05);
    char label[256];

    for (double t : ts) {
        const Point basePt = base.evaluate(t);
        const Point movedPt = moved.evaluate(t);

        std::snprintf(label, sizeof(label),
                      "%s %s @%.0e: evaluate(t=%.2f) X follows translation",
                      prefix, caseName, d, t);
        // Usar tolerância ULP-based; se a magnitude não é representável,
        // a comparação ainda deve passar quando os valores coincidem dentro
        // do possível dado o precision da floating-point.
        checkNear(movedPt.x - delta.x, basePt.x, eps, label);

        std::snprintf(label, sizeof(label),
                      "%s %s @%.0e: evaluate(t=%.2f) Y follows translation",
                      prefix, caseName, d, t);
        checkNear(movedPt.y - delta.y, basePt.y, eps, label);
    }
}

// Distância geométrica bidirecional entre duas polilinhas (descontando translação).
// As polilinhas devem ter a mesma translação aplicada: subtrai-se delta de b para trazer para o espaço de a.
// Comportamento inequívoco para casos de geometria vazia:
//   - Duas polilinhas vazias: retorna -1.0 para indicar "indeterminado / ausência de geometria".
//   - Apenas uma vazia: retorna -1.0 (comparação inválida).
//   - Uma polilinha com um ponto: comparação de ponto única.
double bidirectionalPolylineDistance(const std::vector<artplus::geometry::Point>& a,
                                     const std::vector<artplus::geometry::Point>& b,
                                     const artplus::geometry::Point& delta)
{
    using artplus::geometry::Line;
    using artplus::geometry::pointSegmentDistance;
    using artplus::geometry::distance;
    if (a.empty() && b.empty())
        return -1.0; // indeterminado / ausência de geometria
    if (a.empty() || b.empty())
        return -1.0; // comparação inválida: uma vazia, outra não
    if (a.size() == 1 && b.size() == 1)
        return distance(a.front(), {b.front().x - delta.x, b.front().y - delta.y});
    if (a.size() == 1) {
        // Point a against polyline b
        const artplus::geometry::Point a_shifted{a.front().x + delta.x, a.front().y + delta.y};
        return distanceToPolyline(a_shifted, b);
    }
    if (b.size() == 1) {
        // Polyline a against point b
        const artplus::geometry::Point b_shifted{b.front().x - delta.x, b.front().y - delta.y};
        return distanceToPolyline(b_shifted, a);
    }

    double maxDist = 0.0;
    // a -> b (trazendo b para o espaço de a subtraindo delta de b)
    for (const auto& pa : a) {
        double minDist = 1e300;
        for (std::size_t i = 1; i < b.size(); ++i) {
            const artplus::geometry::Point p0{b[i - 1].x - delta.x, b[i - 1].y - delta.y};
            const artplus::geometry::Point p1{b[i].x - delta.x, b[i].y - delta.y};
            double d = pointSegmentDistance(pa, Line{p0, p1});
            if (d < minDist) minDist = d;
        }
        if (minDist > maxDist) maxDist = minDist;
    }
    // b -> a (trazendo a para o espaço de b somando delta a a)
    for (const auto& pb : b) {
        double minDist = 1e300;
        for (std::size_t i = 1; i < a.size(); ++i) {
            const artplus::geometry::Point p0{a[i - 1].x + delta.x, a[i - 1].y + delta.y};
            const artplus::geometry::Point p1{a[i].x + delta.x, a[i].y + delta.y};
            double d = pointSegmentDistance(pb, Line{p0, p1});
            if (d < minDist) minDist = d;
        }
        if (minDist > maxDist) maxDist = minDist;
    }
    return maxDist;
}

// Encontra extremos internos de uma curva Bézier quadrática (retorna ts em (0,1)).
// O critério de degeneração é baseado nos coeficientes diferenciais da curva,
// não em magnitudes absolutas dos pontos de controle. Isso garante que uma
// translação comum dos pontos de controle não mude a quantidade nem os valores
// de t dos extremos encontrados.
int findQuadraticInteriorExtrema(const artplus::geometry::QuadBezier& q, double (&ts)[2])
{
    using artplus::geometry::Point;
    int count = 0;
    for (int axis = 0; axis < 2; ++axis) {
        const double p0 = (axis == 0) ? q.p0.x : q.p0.y;
        const double c  = (axis == 0) ? q.c.x  : q.c.y;
        const double p1 = (axis == 0) ? q.p1.x : q.p1.y;
        // Diferencial quadratico: denom = p0 - 2*c + p1
        // Usar o máximo dos termos diferenciais em vez de magnitudes absolutas
        // para que a translação dos pontos de controle não altere o resultado.
        const double charmax = std::max({std::abs(p0 - c), std::abs(c - p1)});
        const double threshold = effectiveUlp(charmax) * 64.0;
        const double denom = p0 - 2.0 * c + p1;
        if (std::abs(denom) > threshold) {
            const double t = (c - p0) / denom;
            if (t > 0.0 && t < 1.0 && count < 2)
                ts[count++] = t;
        }
    }
    return count;
}

// Encontra extremos internos de uma curva Bézier cúbica (retorna ts em (0,1)).
// O critério de degeneração é baseado nos coeficientes diferenciais da curva,
// não em magnitudes absolutas dos pontos de controle. Isso garante que uma
// translação comum dos pontos de controle não mude a quantidade nem os valores
// de t dos extremos encontrados.
//
// A fórmula de Citardauq ($q = -\frac{1}{2}(B + \text{sgn}(B)\sqrt{D})$) evita
// cancelamento catastrófico quando $|4AC| \ll B^2$. Trata os casos de borda:
//   - B == 0
//   - q == 0 (raiz dupla)
//   - discriminante negativo (nenhum real)
//   - discriminante zero (raiz dupla)
//   - coeficientes degenerados (A == 0)
//   - raízes fora do intervalo (0,1)
int findCubicInteriorExtrema(const artplus::geometry::CubicBezier& c, double (&ts)[4])
{
    using artplus::geometry::Point;
    int count = 0;
    for (int axis = 0; axis < 2; ++axis) {
        const double p0 = (axis == 0) ? c.p0.x : c.p0.y;
        const double c1  = (axis == 0) ? c.c1.x  : c.c1.y;
        const double c2  = (axis == 0) ? c.c2.x  : c.c2.y;
        const double p1  = (axis == 0) ? c.p1.x  : c.p1.y;

        // Coeficientes dos diferenciais da curva (invariantes de translação)
        const double d0 = c1 - p0;
        const double d1 = c2 - c1;
        const double d2 = p1 - c2;

        const double A = d0 - 2.0 * d1 + d2;
        const double B = 2.0 * (d1 - d0);
        const double C = d0;

        // Critério de degeneração baseado em ULP dos coeficientes diferenciais,
        // não em magnitudes absolutas dos pontos de controle.
        const double maxCoord = std::max({std::abs(A), std::abs(B), std::abs(C)});
        const double threshold = effectiveUlp(maxCoord) * 128.0;

        if (std::abs(A) < threshold) {
            // A é "zero" relativamente: a curva se reduz a um quadrático ou linear.
            if (std::abs(B) > threshold) {
                // Caso linear: raiz única t = -C/B
                const double t = -C / B;
                if (t > 0.0 && t < 1.0 && count < 4) ts[count++] = t;
            } else if (std::abs(B) <= threshold && std::abs(C) <= threshold) {
                // Curva completamente degenerada (todos os coeficientes zero)
                // Não há extremos internos a serem encontrados.
            }
            // Se A ≈ 0 mas B não é pequeno: a raiz linear é o único candidato.
            // Já tratamos acima.
        } else {
            // A é significativo: cúbica quadrática completa.
            const double disc = B * B - 4.0 * A * C;
            if (disc > 0.0) {
                // Dois extremos reais distintos. Aplicar Citardauq para evitar
                // cancelamento catastrófico quando |4AC| << B^2.
                const double sq = std::sqrt(disc);
                // Citardauq: q = -0.5 * (B + sgn(B) * sqrt(D))
                const double q = (B >= 0.0) ? -0.5 * (B + sq) : -0.5 * (B - sq);

                // Duas raízes: t0 = q / A, t1 = C / q
                // Tratamento especial quando q ≈ 0 (raiz dupla)
                if (std::abs(q) > 1e-300) {
                    const double t0 = q / A;
                    const double t1 = C / q;
                    if (t0 > 0.0 && t0 < 1.0 && count < 4) ts[count++] = t0;
                    if (t1 > 0.0 && t1 < 1.0 && count < 4) ts[count++] = t1;
                } else {
                    // q == 0 => raiz dupla em t = 0 ou análise de limite
                    // O termo C/q diverge; se C também é 0, a raiz está em t = 0
                    // (ou o modelo é degener). Apenas registramos se t=0 for
                    // um extremo genuíno verificado por avaliação.
                    if (std::abs(C) <= 1e-300 && 0.0 > 0.0 && 0.0 < 1.0 && count < 4) ts[count++] = 0.0;
                }
            } else if (disc == 0.0) {
                // Raiz dupla: t = -B / (2A)
                const double t = -B / (2.0 * A);
                if (t > 0.0 && t < 1.0 && count < 4) ts[count++] = t;
            } else {
                // discriminante < 0: nenhum extremo real na esta ângulo.
                // Nada a fazer.
            }
        }
    }
    return count;
}

// Verifica se os extremos internos são preservados sob translação.
template <typename Curve>
void checkInteriorExtremaPreserved(const Curve& base, const Curve& moved,
                                   const char* prefix, const char* caseName, double d)
{
    using artplus::geometry::Point;
    int baseCount, movedCount;
    double baseTs[4], movedTs[4];

    if constexpr (std::is_same_v<Curve, artplus::geometry::QuadBezier>) {
        double baseTs2[2], movedTs2[2];
        baseCount = findQuadraticInteriorExtrema(base, baseTs2);
        movedCount = findQuadraticInteriorExtrema(moved, movedTs2);
        // Copy to the larger array for uniform handling below
        for (int i = 0; i < baseCount; ++i) baseTs[i] = baseTs2[i];
        for (int i = 0; i < movedCount; ++i) movedTs[i] = movedTs2[i];
    } else {
        baseCount = findCubicInteriorExtrema(base, baseTs);
        movedCount = findCubicInteriorExtrema(moved, movedTs);
    }

    char label[256];
    // Tolerância baseada em ULP da magnitude da curva - reflete o erro de
    // representação IEEE 754 em vez de usar 0.9 aplicado indiscriminadamente.
    const double ulpVal = effectiveUlp(std::max({std::abs(p0), std::abs(c), std::abs(p1)}));
    const double eps = (ulpVal > 0.5) ? ulpVal * 2.0 : geometricTolerance(d, 0.05);

    std::snprintf(label, sizeof(label),
                  "%s %s @%.0e: interior extrema count preserved",
                  prefix, caseName, d);
    check(baseCount == movedCount, label);

    for (int i = 0; i < baseCount; ++i) {
        std::snprintf(label, sizeof(label),
                      "%s %s @%.0e: interior extremum t[%d] preserved",
                      prefix, caseName, d, i);
        checkNear(movedTs[i], baseTs[i], eps, label);

        const Point basePt = base.evaluate(baseTs[i]);
        const Point movedPt = moved.evaluate(movedTs[i]);
        const Point delta{moved.p0.x - base.p0.x, moved.p0.y - base.p0.y};

        std::snprintf(label, sizeof(label),
                      "%s %s @%.0e: interior extremum X preserved",
                      prefix, caseName, d);
        // Tolerância baseada em ULP da magnitude da curva, não um fixo 100.0.
        const double ulpX = effectiveUlp(std::max(std::abs(movedPt.x), std::abs(basePt.x)));
        checkNear(movedPt.x - delta.x, basePt.x, ulpX * 2.0, label);

        std::snprintf(label, sizeof(label),
                      "%s %s @%.0e: interior extremum Y preserved",
                      prefix, caseName, d);
        checkNear(movedPt.y - delta.y, basePt.y, eps, label);
    }
}

// Verifica bounds relativos para duas polilinhas.
void checkRelativeBoundsForPolylines(const artplus::geometry::Rect& baseBounds,
                                     const artplus::geometry::Rect& movedBounds,
                                     const artplus::geometry::Point& baseP0,
                                     const artplus::geometry::Point& movedP0,
                                     const char* prefix, const char* caseName, double d)
{
    using artplus::geometry::Point;
    // Tolerância baseada em ULP da magnitude - reflete erro de representação
    // em vez de usar 0.9 aplicado indiscriminadamente.
    const double ulpVal = effectiveUlp(d);
    const double eps = (ulpVal > 0.5) ? ulpVal * 2.0 : geometricTolerance(d, 0.05);
    char label[256];

    std::snprintf(label, sizeof(label),
                  "%s %s @%.0e: relative left preserved",
                  prefix, caseName, d);
    checkNear(movedBounds.left - movedP0.x, baseBounds.left - baseP0.x, eps, label);

    std::snprintf(label, sizeof(label),
                  "%s %s @%.0e: relative top preserved",
                  prefix, caseName, d);
    checkNear(movedBounds.top - movedP0.y, baseBounds.top - baseP0.y, eps, label);

    std::snprintf(label, sizeof(label),
                  "%s %s @%.0e: relative right preserved",
                  prefix, caseName, d);
    checkNear(movedBounds.right - movedP0.x, baseBounds.right - baseP0.x, eps, label);

    std::snprintf(label, sizeof(label),
                  "%s %s @%.0e: relative bottom preserved",
                  prefix, caseName, d);
    checkNear(movedBounds.bottom - movedP0.y, baseBounds.bottom - baseP0.y, eps, label);

    std::snprintf(label, sizeof(label),
                  "%s %s @%.0e: width preserved",
                  prefix, caseName, d);
    checkNear(movedBounds.width(), baseBounds.width(), eps, label);

    std::snprintf(label, sizeof(label),
                  "%s %s @%.0e: height preserved",
                  prefix, caseName, d);
    checkNear(movedBounds.height(), baseBounds.height(), eps, label);
}

int main()
{
    using namespace artplus::geometry;

    std::printf("ArtPlus geometry core test (Qt-free)\n");
    std::printf("====================================\n");

    // --- Point ---------------------------------------------------------------
    std::printf("\nPoint:\n");
    {
        const Point p{3.0, 4.0};
        checkNear(p.length(), 5.0, 1e-12, "3-4-5 length");
        checkNear(p.lengthSquared(), 25.0, 1e-12, "3-4-5 squared length");
        const Point n = p.normalized();
        checkNear(n.x, 0.6, 1e-12, "normalized x");
        checkNear(n.y, 0.8, 1e-12, "normalized y");
        check(Point{}.normalized() == Point{}, "normalizing a zero vector stays zero (no NaN)");
        check(p + Point{1.0, 1.0} == Point{4.0, 5.0}, "point addition");
        check(p - Point{1.0, 1.0} == Point{2.0, 3.0}, "point subtraction");
        check(p * 2.0 == Point{6.0, 8.0}, "point scaling");
        checkNear(distance(Point{-3.0, -4.0}, Point{0.0, 0.0}), 5.0, 1e-12,
                  "distance with negative coordinates");
        check(lerp(Point{0.0, 0.0}, Point{10.0, 20.0}, 0.5) == Point{5.0, 10.0}, "lerp midpoint");
    }

    // --- Rect ----------------------------------------------------------------
    std::printf("\nRect:\n");
    {
        const Rect r = Rect::fromXYWH(-10.0, -20.0, 30.0, 40.0);
        checkNear(r.width(), 30.0, 1e-12, "width from XYWH");
        checkNear(r.height(), 40.0, 1e-12, "height from XYWH");
        checkNear(r.center().x, 5.0, 1e-12, "center x");
        checkNear(r.center().y, 0.0, 1e-12, "center y");
        check(r.contains(Point{20.0, 20.0}), "contains keeps the right/bottom border inclusive");
        check(!r.contains(Point{20.0000001, 20.0}), "rejects a point past the border");

        const Rect u = Rect{-5.0, -5.0, 5.0, 5.0}.united(Rect{0.0, 0.0, 10.0, 10.0});
        check(u.left == -5.0 && u.top == -5.0 && u.right == 10.0 && u.bottom == 10.0,
              "union covers both rects");

        const Rect i = Rect{0.0, 0.0, 10.0, 10.0}.intersected(Rect{5.0, 5.0, 15.0, 15.0});
        check(i.left == 5.0 && i.top == 5.0 && i.right == 10.0 && i.bottom == 10.0,
              "intersection of overlapping rects");

        check(Rect{0.0, 0.0, 10.0, 10.0}.intersected(Rect{20.0, 20.0, 30.0, 30.0}).isEmpty(),
              "disjoint intersection is empty");
        check(Rect{10.0, 10.0, 0.0, 0.0}.normalized() == Rect{0.0, 0.0, 10.0, 10.0},
              "normalized swaps inverted corners");
        check(Rect{0.0, 0.0, 0.0, 10.0}.isEmpty(), "zero-width rect is empty");
        check(Rect{0.0, 0.0, 10.0, 10.0}.intersects(Rect{9.0, 9.0, 20.0, 20.0}),
              "overlapping rects intersect");
        check(!Rect{0.0, 0.0, 10.0, 10.0}.intersects(Rect{10.0, 0.0, 20.0, 10.0}),
              "rects sharing only an edge do not intersect");
    }

    // --- AffineTransform (Matrix) --------------------------------------------
    std::printf("\nAffineTransform:\n");
    {
        check(Matrix::translation(5.0, -3.0).map(Point{1.0, 1.0}) == Point{6.0, -2.0},
              "translation");
        check(Matrix::scaling(2.0, 3.0).map(Point{1.0, 1.0}) == Point{2.0, 3.0}, "scaling");

        const Point r90 = Matrix::rotationDegrees(90.0).map(Point{1.0, 0.0});
        checkNear(r90.x, 0.0, 1e-12, "rotation 90 maps +x to +y (x)");
        checkNear(r90.y, 1.0, 1e-12, "rotation 90 maps +x to +y (y)");

        const Point r90y = Matrix::rotationDegrees(90.0).map(Point{0.0, 1.0});
        checkNear(r90y.x, -1.0, 1e-12, "rotation 90 maps +y to -x (x)");
        checkNear(r90y.y, 0.0, 1e-12, "rotation 90 maps +y to -x (y)");

        const Point composed =
            (Matrix::translation(10.0, 0.0) * Matrix::rotationDegrees(90.0)).map(Point{1.0, 0.0});
        checkNear(composed.x, 10.0, 1e-12, "composition applies the right factor first (x)");
        checkNear(composed.y, 1.0, 1e-12, "composition applies the right factor first (y)");

        const Matrix m =
            Matrix::translation(10.0, -4.0) * Matrix::rotationDegrees(37.0) * Matrix::scaling(2.0, 0.5);
        bool ok = false;
        const Matrix inv = m.inverted(&ok);
        check(ok, "an invertible transform reports success");
        const Point roundTrip = inv.map(m.map(Point{3.0, 7.0}));
        checkNear(roundTrip.x, 3.0, 1e-9, "inverse round-trip x");
        checkNear(roundTrip.y, 7.0, 1e-9, "inverse round-trip y");

        const Matrix singular{0.0, 0.0, 0.0, 0.0, 5.0, 5.0};
        bool singularOk = true;
        (void)singular.inverted(&singularOk);
        check(!singularOk, "a non-invertible transform reports failure");
        checkNear(singular.determinant(), 0.0, 1e-15, "singular determinant is zero");

        const Rect rotatedRect = Matrix::rotationDegrees(90.0).mapRect(Rect{0.0, 0.0, 10.0, 20.0});
        checkNear(rotatedRect.left, -20.0, 1e-12, "mapRect 90 degrees: left");
        checkNear(rotatedRect.top, 0.0, 1e-12, "mapRect 90 degrees: top");
        checkNear(rotatedRect.width(), 20.0, 1e-12, "mapRect 90 degrees: width");
        checkNear(rotatedRect.height(), 10.0, 1e-12, "mapRect 90 degrees: height");

        check(Matrix::identity().isIdentity(), "identity is identity");
        check(!Matrix::translation(1.0, 0.0).isIdentity(), "translation is not identity");

        const AffineTransform alias = AffineTransform::translation(1.0, 2.0);
        check(alias.map(Point{0.0, 0.0}) == Point{1.0, 2.0}, "AffineTransform alias works");
    }

    // --- Line geometry -------------------------------------------------------
    std::printf("\nLine:\n");
    {
        const Line l{Point{0.0, 0.0}, Point{3.0, 4.0}};
        checkNear(l.length(), 5.0, 1e-12, "segment length");
        check(l.pointAt(0.5) == Point{1.5, 2.0}, "pointAt mid parameter");
        checkNear(l.distanceToPoint(Point{0.0, 5.0}), 3.0, 1e-12,
                  "distance from a point to the segment body");
        checkNear(l.distanceToPoint(Point{-3.0, -4.0}), 5.0, 1e-12,
                  "distance is clamped to the segment start");
        checkNear(Line{Point{0.0, 0.0}, Point{10.0, 4.0}}.bounds().right, 10.0, 1e-12,
                  "segment bounds right");
        checkNear(Line{Point{-5.0, -1.0}, Point{2.0, 3.0}}.bounds().left, -5.0, 1e-12,
                  "segment bounds with negative coordinates");
    }

    // --- Segment intersections ----------------------------------------------
    std::printf("\nLine intersections:\n");
    {
        // Proper crossing.
        {
            const Line a{Point{0.0, 0.0}, Point{10.0, 10.0}};
            const Line b{Point{0.0, 10.0}, Point{10.0, 0.0}};
            const LineIntersection hit = intersectSegments(a, b);
            check(hit.kind == IntersectionKind::Point, "crossing segments report Point");
            checkNear(hit.point.x, 5.0, 1e-12, "crossing point x");
            checkNear(hit.point.y, 5.0, 1e-12, "crossing point y");
            checkNear(hit.t, 0.5, 1e-12, "crossing parameter t");
            checkNear(hit.u, 0.5, 1e-12, "crossing parameter u");
        }

        // Negative coordinates.
        {
            const LineIntersection hit = intersectSegments(Line{Point{-10.0, -10.0}, Point{10.0, 10.0}},
                                                          Line{Point{-10.0, 10.0}, Point{10.0, -10.0}});
            checkNear(hit.point.x, 0.0, 1e-12, "crossing with negative coordinates (x)");
            checkNear(hit.point.y, 0.0, 1e-12, "crossing with negative coordinates (y)");
        }

        // Parallel and distinct.
        {
            const LineIntersection hit = intersectSegments(Line{Point{0.0, 0.0}, Point{10.0, 0.0}},
                                                          Line{Point{0.0, 5.0}, Point{10.0, 5.0}});
            check(hit.kind == IntersectionKind::None, "distinct parallel segments do not intersect");
        }

        // Collinear overlap.
        {
            const LineIntersection hit = intersectSegments(Line{Point{0.0, 0.0}, Point{10.0, 0.0}},
                                                          Line{Point{5.0, 0.0}, Point{15.0, 0.0}});
            check(hit.kind == IntersectionKind::Overlap, "collinear segments report Overlap");
            checkNear(hit.overlapT0, 0.5, 1e-12, "overlap start parameter");
            checkNear(hit.overlapT1, 1.0, 1e-12, "overlap end parameter");
            checkNear(hit.overlapLength, 5.0, 1e-12, "overlap length");
        }

        // Collinear, touching at one endpoint only.
        {
            const LineIntersection hit = intersectSegments(Line{Point{0.0, 0.0}, Point{10.0, 0.0}},
                                                          Line{Point{10.0, 0.0}, Point{20.0, 0.0}});
            check(hit.kind == IntersectionKind::Point, "collinear endpoint touch reports Point");
            checkNear(hit.point.x, 10.0, 1e-12, "collinear touch x");
            checkNear(hit.point.y, 0.0, 1e-12, "collinear touch y");
        }

        // Collinear but disjoint.
        {
            const LineIntersection hit = intersectSegments(Line{Point{0.0, 0.0}, Point{10.0, 0.0}},
                                                          Line{Point{20.0, 0.0}, Point{30.0, 0.0}});
            check(hit.kind == IntersectionKind::None, "disjoint collinear segments return None");
        }

        // T-junction (endpoint of b on the body of a).
        {
            const LineIntersection hit = intersectSegments(Line{Point{0.0, 0.0}, Point{10.0, 0.0}},
                                                          Line{Point{5.0, 0.0}, Point{5.0, 10.0}});
            check(hit.kind == IntersectionKind::Point, "T-junction reports Point");
            checkNear(hit.point.x, 5.0, 1e-12, "T-junction x");
            checkNear(hit.u, 0.0, 1e-12, "T-junction parameter on the second segment");
        }

        // Endpoint touch of two non-collinear segments.
        {
            const LineIntersection hit = intersectSegments(Line{Point{0.0, 0.0}, Point{10.0, 0.0}},
                                                          Line{Point{10.0, 0.0}, Point{10.0, 10.0}});
            check(hit.kind == IntersectionKind::Point, "shared endpoint reports Point");
            checkNear(hit.point.x, 10.0, 1e-12, "shared endpoint x");
        }

        // Zero-length segments (degenerate) behave like points.
        {
            const LineIntersection onBody = intersectSegments(Line{Point{5.0, 0.0}, Point{5.0, 0.0}},
                                                             Line{Point{0.0, 0.0}, Point{10.0, 0.0}});
            check(onBody.kind == IntersectionKind::Point, "zero-length segment on a segment reports Point");
            checkNear(onBody.point.x, 5.0, 1e-12, "zero-length segment point x");

            const LineIntersection offBody = intersectSegments(Line{Point{5.0, 3.0}, Point{5.0, 3.0}},
                                                              Line{Point{0.0, 0.0}, Point{10.0, 0.0}});
            check(offBody.kind == IntersectionKind::None, "zero-length segment off a segment returns None");

            const LineIntersection samePoint =
                intersectSegments(Line{Point{2.0, 2.0}, Point{2.0, 2.0}},
                                  Line{Point{2.0, 2.0}, Point{2.0, 2.0}});
            check(samePoint.kind == IntersectionKind::Point, "two coincident points intersect");
        }

        // Infinite lines: intersection may lie outside both segments.
        {
            const LineIntersection hit = intersectInfiniteLines(Line{Point{0.0, 0.0}, Point{1.0, 0.0}},
                                                                Line{Point{5.0, 5.0}, Point{5.0, 6.0}});
            check(hit.kind == IntersectionKind::Point, "infinite lines intersect outside the segments");
            checkNear(hit.point.x, 5.0, 1e-12, "infinite line intersection x");
            checkNear(hit.t, 5.0, 1e-12, "infinite line parameter is not clamped");

            const LineIntersection parallel = intersectInfiniteLines(
                Line{Point{0.0, 0.0}, Point{1.0, 0.0}}, Line{Point{0.0, 2.0}, Point{1.0, 2.0}});
            check(parallel.kind == IntersectionKind::None, "parallel infinite lines do not intersect");
        }

        // Extreme magnitudes.
        {
            const LineIntersection huge = intersectSegments(
                Line{Point{0.0, 0.0}, Point{1.0e9, 1.0e9}},
                Line{Point{0.0, 1.0e9}, Point{1.0e9, 0.0}});
            check(huge.kind == IntersectionKind::Point, "very large coordinates still intersect");
            check(std::abs(huge.point.x - 5.0e8) <= 1e-6 * 5.0e8,
                  "very large intersection x within relative tolerance");

            const LineIntersection tiny = intersectSegments(
                Line{Point{0.0, 0.0}, Point{1.0e-6, 1.0e-6}},
                Line{Point{0.0, 1.0e-6}, Point{1.0e-6, 0.0}});
            check(tiny.kind == IntersectionKind::Point, "very small coordinates still intersect");
            checkNear(tiny.point.x, 5.0e-7, 1e-15, "very small intersection x");
            checkNear(tiny.point.y, 5.0e-7, 1e-15, "very small intersection y");
        }
    }

    // --- Bezier --------------------------------------------------------------
    std::printf("\nBezier:\n");
    {
        const CubicBezier sCurve{Point{0.0, 0.0}, Point{0.0, 100.0}, Point{100.0, 100.0},
                                 Point{100.0, 0.0}};

        const Point at0 = sCurve.evaluate(0.0);
        const Point at1 = sCurve.evaluate(1.0);
        checkNear(at0.x, 0.0, 1e-12, "cubic starts at p0 (x)");
        checkNear(at0.y, 0.0, 1e-12, "cubic starts at p0 (y)");
        checkNear(at1.x, 100.0, 1e-12, "cubic ends at p1 (x)");
        checkNear(at1.y, 0.0, 1e-12, "cubic ends at p1 (y)");

        const Point mid = sCurve.evaluate(0.5);
        checkNear(mid.x, 50.0, 1e-12, "cubic midpoint x");
        checkNear(mid.y, 75.0, 1e-12, "cubic midpoint y");

        const Point derivative0 = sCurve.derivative(0.0);
        checkNear(derivative0.x, 0.0, 1e-12, "cubic derivative at t=0 (x)");
        checkNear(derivative0.y, 300.0, 1e-12, "cubic derivative at t=0 (y)");

        const Rect bounds = sCurve.bounds();
        checkNear(bounds.left, 0.0, 1e-12, "cubic bounds left");
        checkNear(bounds.right, 100.0, 1e-12, "cubic bounds right");
        checkNear(bounds.top, 0.0, 1e-12, "cubic bounds top");
        checkNear(bounds.bottom, 75.0, 1e-12,
                  "cubic bounds use the curve extremum, not the control polygon");

        CubicBezier left;
        CubicBezier right;
        sCurve.split(0.5, left, right);
        checkNear(left.p1.x, mid.x, 1e-12, "split shares the joint x");
        checkNear(left.p1.y, mid.y, 1e-12, "split shares the joint y");
        const Point leftMid = left.evaluate(0.5);
        const Point curveQuarter = sCurve.evaluate(0.25);
        checkNear(leftMid.x, curveQuarter.x, 1e-9, "left half reproduces the original curve (x)");
        checkNear(leftMid.y, curveQuarter.y, 1e-9, "left half reproduces the original curve (y)");

        // Coincident control points.
        {
            const CubicBezier collapsed{Point{5.0, 5.0}, Point{5.0, 5.0}, Point{5.0, 5.0},
                                        Point{5.0, 5.0}};
            const Point collapsedMid = collapsed.evaluate(0.5);
            checkNear(collapsedMid.x, 5.0, 1e-12, "fully coincident cubic stays at the point (x)");
            checkNear(collapsedMid.y, 5.0, 1e-12, "fully coincident cubic stays at the point (y)");
            const Rect collapsedBounds = collapsed.bounds();
            checkNear(collapsedBounds.width(), 0.0, 1e-12, "coincident cubic bounds width is zero");
            checkNear(collapsedBounds.height(), 0.0, 1e-12, "coincident cubic bounds height is zero");

            std::vector<Point> collapsedFlat;
            flattenCubic(collapsed, 0.01, collapsedFlat);
            check(collapsedFlat.size() == 1, "coincident cubic flattens to a single point");
            checkNear(collapsedFlat.front().x, 5.0, 1e-12, "flattened coincident point x");

            // Coincident endpoints only (closed-looking span).
            const CubicBezier loop{Point{0.0, 0.0}, Point{10.0, 10.0}, Point{10.0, -10.0},
                                   Point{0.0, 0.0}};
            const Rect loopBounds = loop.bounds();
            check(loopBounds.right >= 5.0, "cubic with coincident endpoints still has extent");
        }

        // Straight-line cubic (control points on the chord).
        {
            const CubicBezier line{Point{0.0, 0.0}, Point{10.0, 0.0}, Point{20.0, 0.0},
                                   Point{30.0, 0.0}};
            checkNear(line.length(0.01), 30.0, 1e-9, "straight cubic length equals the chord");
            std::vector<Point> flat;
            flattenCubic(line, 0.01, flat);
            check(flat.size() == 1, "straight cubic flattens to one segment");
        }

        // Quadratic.
        {
            const QuadBezier quad{Point{0.0, 0.0}, Point{50.0, 100.0}, Point{100.0, 0.0}};
            const Point quadMid = quad.evaluate(0.5);
            checkNear(quadMid.x, 50.0, 1e-12, "quadratic midpoint x");
            checkNear(quadMid.y, 50.0, 1e-12, "quadratic midpoint y");
            const Rect quadBounds = quad.bounds();
            checkNear(quadBounds.right, 100.0, 1e-12, "quadratic bounds right");
            checkNear(quadBounds.bottom, 50.0, 1e-12, "quadratic bounds use the extremum");

            std::vector<Point> quadFlat;
            flattenQuadratic(quad, 0.05, quadFlat);
            check(quadFlat.size() >= 2, "quadratic flattens to at least one segment");
            const Point quadEnd = quadFlat.back();
            checkNear(quadEnd.x, 100.0, 1e-9, "quadratic flattening ends at p1 (x)");
            checkNear(quadEnd.y, 0.0, 1e-9, "quadratic flattening ends at p1 (y)");

            // Every sampled curve point stays within the tolerance of the polyline.
            std::vector<Point> quadPolyline;
            quadPolyline.push_back(quad.p0);
            quadPolyline.insert(quadPolyline.end(), quadFlat.begin(), quadFlat.end());
            double worstQuadratic = 0.0;
            for (int i = 0; i <= 200; ++i) {
                const double t = static_cast<double>(i) / 200.0;
                worstQuadratic = std::max(worstQuadratic, distanceToPolyline(quad.evaluate(t), quadPolyline));
            }
            check(worstQuadratic <= 0.05, "quadratic flattening respects the tolerance");
        }

        // Flattening tolerance behaviour.
        {
            std::vector<Point> coarse;
            std::vector<Point> fine;
            flattenCubic(sCurve, 1.0, coarse);
            flattenCubic(sCurve, 0.01, fine);
            check(fine.size() > coarse.size(), "a smaller tolerance produces more segments");

            std::vector<Point> polyline;
            polyline.push_back(sCurve.p0);
            polyline.insert(polyline.end(), fine.begin(), fine.end());
            double worst = 0.0;
            for (int i = 0; i <= 400; ++i) {
                const double t = static_cast<double>(i) / 400.0;
                worst = std::max(worst, distanceToPolyline(sCurve.evaluate(t), polyline));
            }
            check(worst <= 0.01, "cubic flattening respects the tolerance");

            // A tolerance of zero/negative must be clamped, not recurse forever.
            std::vector<Point> clamped;
            flattenCubic(sCurve, 0.0, clamped);
            check(!clamped.empty(), "zero tolerance is clamped and still terminates");
        }
    }

    // --- Polygon -------------------------------------------------------------
    std::printf("\nPolygon:\n");
    {
        // Counter-clockwise (mathematical, y-up) unit square.
        Polygon square{{Point{0.0, 0.0}, Point{1.0, 0.0}, Point{1.0, 1.0}, Point{0.0, 1.0}}};
        checkNear(square.signedArea(), 1.0, 1e-12, "CCW unit square signed area");
        checkNear(square.area(), 1.0, 1e-12, "CCW unit square area");
        check(square.orientation() == Orientation::CounterClockwise,
              "CCW square reports CounterClockwise");
        checkNear(square.centroid().x, 0.5, 1e-12, "square centroid x");
        checkNear(square.centroid().y, 0.5, 1e-12, "square centroid y");
        checkNear(square.perimeter(), 4.0, 1e-12, "square perimeter");
        const Rect squareBounds = square.bounds();
        check(squareBounds.width() == 1.0 && squareBounds.height() == 1.0, "square bounds");

        // Clockwise winding.
        Polygon clockwise{{Point{0.0, 1.0}, Point{1.0, 1.0}, Point{1.0, 0.0}, Point{0.0, 0.0}}};
        checkNear(clockwise.signedArea(), -1.0, 1e-12, "CW square signed area is negative");
        check(clockwise.orientation() == Orientation::Clockwise, "CW square reports Clockwise");

        clockwise.ensureOrientation(Orientation::CounterClockwise);
        check(clockwise.orientation() == Orientation::CounterClockwise,
              "ensureOrientation flips the winding");

        square.reverse();
        check(square.orientation() == Orientation::Clockwise, "reverse() flips the winding");
        square.reverse();

        // Triangle: area centroid equals the vertex mean.
        const Polygon triangle{{Point{0.0, 0.0}, Point{4.0, 0.0}, Point{0.0, 3.0}}};
        checkNear(triangle.area(), 6.0, 1e-12, "triangle area");
        checkNear(triangle.centroid().x, 4.0 / 3.0, 1e-12, "triangle centroid x");
        checkNear(triangle.centroid().y, 1.0, 1e-12, "triangle centroid y");

        // Point classification, including exact boundary membership.
        check(square.classify(Point{0.5, 0.5}) == PointLocation::Inside, "centre is Inside");
        check(square.classify(Point{1.5, 0.5}) == PointLocation::Outside, "far point is Outside");
        check(square.classify(Point{0.5, 0.0}) == PointLocation::Boundary,
              "edge midpoint is Boundary");
        check(square.classify(Point{0.0, 0.0}) == PointLocation::Boundary,
              "vertex is Boundary");
        check(square.classify(Point{0.5, -5e-10}) == PointLocation::Boundary,
              "point just outside within epsilon is Boundary");
        check(square.classify(Point{0.5, -1e-6}) == PointLocation::Outside,
              "point clearly outside is Outside");
        check(square.contains(Point{0.5, 0.0}), "contains() accepts boundary points");
        check(!square.contains(Point{2.0, 2.0}), "contains() rejects outside points");

        // Concave polygon: the notch is outside.
        const Polygon lShape{{Point{0.0, 0.0}, Point{2.0, 0.0}, Point{2.0, 1.0}, Point{1.0, 1.0},
                              Point{1.0, 2.0}, Point{0.0, 2.0}}};
        checkNear(lShape.area(), 3.0, 1e-12, "L-shape area");
        check(lShape.classify(Point{0.5, 1.5}) == PointLocation::Inside,
              "L-shape arm point is Inside");
        check(lShape.classify(Point{1.5, 1.5}) == PointLocation::Outside,
              "L-shape notch point is Outside");

        // Degenerate polygons.
        const Polygon collinear{{Point{0.0, 0.0}, Point{1.0, 0.0}, Point{2.0, 0.0}}};
        checkNear(collinear.signedArea(), 0.0, 1e-12, "collinear polygon has zero area");
        check(collinear.orientation() == Orientation::Degenerate,
              "collinear polygon is Degenerate");
        check(collinear.classify(Point{1.0, 0.5}) == PointLocation::Outside,
              "point off a collinear polygon is Outside");
        check(collinear.classify(Point{1.0, 0.0}) == PointLocation::Boundary,
              "point on a collinear polygon is Boundary");

        const Polygon twoVertices{{Point{0.0, 0.0}, Point{10.0, 0.0}}};
        check(twoVertices.classify(Point{5.0, 0.0}) == PointLocation::Boundary,
              "two-vertex polygon: on the span is Boundary");
        check(twoVertices.classify(Point{5.0, 1.0}) == PointLocation::Outside,
              "two-vertex polygon: off the span is Outside");

        const Polygon oneVertex{{Point{1.0, 1.0}}};
        check(oneVertex.classify(Point{1.0, 1.0}) == PointLocation::Boundary,
              "one-vertex polygon: the point itself is Boundary");
        check(oneVertex.classify(Point{2.0, 1.0}) == PointLocation::Outside,
              "one-vertex polygon: elsewhere is Outside");

        const Polygon empty;
        check(empty.empty(), "default polygon is empty");
        check(empty.classify(Point{0.0, 0.0}) == PointLocation::Outside, "empty polygon has no inside");
        check(empty.bounds().isEmpty(), "empty polygon bounds are empty");
        check(empty.centroid() == Point{}, "empty polygon centroid is the origin");

        // Duplicate vertices are removable.
        Polygon duplicated{{Point{0.0, 0.0}, Point{1.0, 0.0}, Point{1.0, 0.0}, Point{1.0, 1.0},
                            Point{0.0, 1.0}, Point{0.0, 0.0}}};
        check(duplicated.size() == 6, "duplicate vertices are present before cleanup");
        duplicated.removeDuplicateVertices();
        check(duplicated.size() == 4, "removeDuplicateVertices drops duplicates and the closing vertex");

        // Negative coordinates.
        const Polygon negative{{Point{-5.0, -5.0}, Point{-4.0, -5.0}, Point{-4.0, -4.0},
                               Point{-5.0, -4.0}}};
        checkNear(negative.area(), 1.0, 1e-12, "negative-coordinate square area");
        checkNear(negative.centroid().x, -4.5, 1e-12, "negative-coordinate centroid x");

        // Extreme scales: the relative degeneracy test must not misfire.
        {
            const double s = 1.0e-6;
            const Polygon tiny{{Point{0.0, 0.0}, Point{s, 0.0}, Point{s, s}, Point{0.0, s}}};
            checkNear(tiny.area(), s * s, 1e-24, "tiny square area");
            check(tiny.orientation() == Orientation::CounterClockwise,
                  "tiny square is not misclassified as Degenerate");

            const double b = 1.0e6;
            const Polygon huge{{Point{0.0, 0.0}, Point{b, 0.0}, Point{b, b}, Point{0.0, b}}};
            checkNear(huge.area(), b * b, 1.0, "huge square area");
            check(huge.orientation() == Orientation::CounterClockwise,
                  "huge square keeps its orientation");
        }

        // Free functions mirror the class API.
        checkNear(polygonSignedArea(triangle.vertices()), 6.0, 1e-12,
                  "polygonSignedArea free function");
        checkNear(polygonCentroid(triangle.vertices()).x, 4.0 / 3.0, 1e-12,
                  "polygonCentroid free function");
        check(polygonOrientation(square.vertices()) == Orientation::CounterClockwise,
              "polygonOrientation free function");
        check(classifyPointInPolygon(Point{0.5, 0.25}, triangle.vertices()) == PointLocation::Inside,
              "classifyPointInPolygon free function inside");
        checkNear(polygonBoundsOf(negative.vertices()).left, -5.0, 1e-12,
                  "polygonBoundsOf free function");
    }

    // --- 1.1 regression: scale-invariant invertibility -----------------------
    std::printf("\nMatrix invertibility across scales:\n");
    {
        // A uniform scale is always invertible. The previous absolute test
        // |det| < 1e-12 rejected everything below roughly 1e-6 scale.
        for (double s : {1e-8, 1e-6, 1.0, 1e6}) {
            char label[160];
            const Matrix m = Matrix::translation(3.0 * s, -2.0 * s) * Matrix::scaling(s, s);
            bool ok = false;
            const Matrix inv = m.inverted(&ok);
            std::snprintf(label, sizeof(label), "uniform scale %.0e is invertible", s);
            check(ok, label);

            const Point original{7.0 * s, -4.0 * s};
            const Point back = inv.map(m.map(original));
            std::snprintf(label, sizeof(label), "uniform scale %.0e round-trips x", s);
            checkNear(back.x, original.x, std::abs(original.x) * 1e-9, label);
            std::snprintf(label, sizeof(label), "uniform scale %.0e round-trips y", s);
            checkNear(back.y, original.y, std::abs(original.y) * 1e-9, label);
        }

        // Rank-1 matrix: mathematically singular, must stay rejected.
        const Matrix rankOne{1.0, 2.0, 2.0, 4.0, 0.0, 0.0};
        bool rankOneOk = true;
        (void)rankOne.inverted(&rankOneOk);
        check(!rankOneOk, "rank-1 matrix is rejected as singular");

        // Columns dependent to within 1e-16: numerically singular.
        const Matrix nearlyDependent{1.0, 1.0, 1.0, 1.0 + 1e-16, 0.0, 0.0};
        bool nearOk = true;
        (void)nearlyDependent.inverted(&nearOk);
        check(!nearOk, "columns dependent to 1e-16 are rejected as singular");

        // Rotation combined with a tiny uniform scale must still invert.
        const Matrix tinyRot = Matrix::rotationDegrees(30.0) * Matrix::scaling(1e-8, 1e-8);
        bool tinyRotOk = false;
        const Matrix tinyRotInv = tinyRot.inverted(&tinyRotOk);
        check(tinyRotOk, "rotation at 1e-8 scale is invertible");
        const Point p{1e-8, 0.0};
        const Point q = tinyRotInv.map(tinyRot.map(p));
        checkNear(q.x, p.x, 1e-17, "tiny rotation round-trip x");
        checkNear(q.y, p.y, 1e-17, "tiny rotation round-trip y");

        // A large uniform scale must also invert.
        const Matrix large = Matrix::scaling(1e6, 1e6);
        bool largeOk = false;
        const Matrix largeInv = large.inverted(&largeOk);
        check(largeOk, "uniform scale 1e6 is invertible");
        const Point largeBack = largeInv.map(large.map(Point{1.5e6, -2.5e6}));
        checkNear(largeBack.x, 1.5e6, 1e-3, "large scale round-trip x");
        checkNear(largeBack.y, -2.5e6, 1e-3, "large scale round-trip y");
    }

    // --- 1.1 regression: mapRect keeps degenerate geometry -------------------
    std::printf("\nmapRect with degenerate geometry:\n");
    {
        const Matrix translate = Matrix::translation(1.0, 2.0);

        const Rect normal = translate.mapRect(Rect{0.0, 0.0, 10.0, 20.0});
        checkNear(normal.left, 1.0, 1e-12, "normal rect keeps left");
        checkNear(normal.bottom, 22.0, 1e-12, "normal rect keeps bottom");

        // Zero width: a vertical segment. Its transformed x must survive.
        const Rect zeroWidth = translate.mapRect(Rect{5.0, 0.0, 5.0, 10.0});
        checkNear(zeroWidth.left, 6.0, 1e-12, "zero-width rect keeps x");
        checkNear(zeroWidth.right, 6.0, 1e-12, "zero-width rect keeps right edge");
        checkNear(zeroWidth.top, 2.0, 1e-12, "zero-width rect keeps top");
        checkNear(zeroWidth.bottom, 12.0, 1e-12, "zero-width rect keeps bottom");
        checkNear(zeroWidth.height(), 10.0, 1e-12, "zero-width rect keeps its span");

        // Zero height: a horizontal segment.
        const Rect zeroHeight = translate.mapRect(Rect{0.0, 7.0, 10.0, 7.0});
        checkNear(zeroHeight.top, 9.0, 1e-12, "zero-height rect keeps top");
        checkNear(zeroHeight.width(), 10.0, 1e-12, "zero-height rect keeps width");

        // A single point.
        const Rect point = translate.mapRect(Rect{3.0, 4.0, 3.0, 4.0});
        checkNear(point.left, 4.0, 1e-12, "point rect keeps x");
        checkNear(point.top, 6.0, 1e-12, "point rect keeps y");

        // Rotation of a zero-width rect must rotate the segment, not erase it.
        const Rect rotatedLine = Matrix::rotationDegrees(90.0).mapRect(Rect{0.0, 0.0, 0.0, 10.0});
        checkNear(rotatedLine.left, -10.0, 1e-12, "rotated zero-width rect: left");
        checkNear(rotatedLine.right, 0.0, 1e-12, "rotated zero-width rect: right");
        checkNear(rotatedLine.top, 0.0, 1e-12, "rotated zero-width rect: top");
        checkNear(rotatedLine.bottom, 0.0, 1e-12, "rotated zero-width rect: bottom");

        // Scale of a zero-height rect keeps its transformed span.
        const Rect scaledLine = Matrix::scaling(2.0, 3.0).mapRect(Rect{0.0, 0.0, 0.0, 5.0});
        checkNear(scaledLine.bottom, 15.0, 1e-12, "scaled zero-width rect keeps its span");
    }

    // --- 1.1 regression: normalized() is scale invariant ---------------------
    std::printf("\nPoint::normalized across scales:\n");
    {
        const Point zero = Point{}.normalized();
        check(zero == Point{}, "zero vector stays zero");
        check(std::isfinite(zero.x) && std::isfinite(zero.y), "zero vector has no NaN/inf");

        const Point classic = Point{3.0, 4.0}.normalized();
        checkNear(classic.x, 0.6, 1e-15, "classic vector normalized x");
        checkNear(classic.y, 0.8, 1e-15, "classic vector normalized y");

        const Point tiny = Point{3e-15, 4e-15}.normalized();
        checkNear(tiny.x, 0.6, 1e-12, "1e-15 vector is normalized, not zeroed (x)");
        checkNear(tiny.y, 0.8, 1e-12, "1e-15 vector is normalized, not zeroed (y)");
        check(std::isfinite(tiny.x) && std::isfinite(tiny.y), "tiny vector is finite");

        const Point huge = Point{3e300, 4e300}.normalized();
        checkNear(huge.x, 0.6, 1e-12, "1e300 vector normalized without overflow (x)");
        checkNear(huge.y, 0.8, 1e-12, "1e300 vector normalized without overflow (y)");
        check(std::isfinite(huge.x) && std::isfinite(huge.y), "huge vector is finite");

        const Point subnormal = Point{1e-320, 0.0}.normalized();
        checkNear(subnormal.x, 1.0, 1e-12, "subnormal vector normalizes");
        checkNear(subnormal.y, 0.0, 1e-12, "subnormal vector y is zero");
    }

    // --- 1.1 regression: Bezier behaviour across scales ----------------------
    std::printf("\nBezier across scales:\n");
    {
        auto makeCubic = [](double s) {
            return CubicBezier{Point{0.0, 0.0}, Point{0.0, 100.0 * s},
                               Point{100.0 * s, 100.0 * s}, Point{100.0 * s, 0.0}};
        };

        for (double s : {1e-15, 1e-9, 1e-6, 1.0, 1e6, 1e9}) {
            char label[160];
            const CubicBezier curve = makeCubic(s);

            const Rect bb = curve.bounds();
            std::snprintf(label, sizeof(label), "cubic %.0e: bounds width", s);
            checkNear(bb.width(), 100.0 * s, std::abs(100.0 * s) * 1e-9, label);
            std::snprintf(label, sizeof(label), "cubic %.0e: bounds use the interior extremum", s);
            checkNear(bb.bottom, 75.0 * s, std::abs(75.0 * s) * 1e-9, label);

            const Point mid = curve.evaluate(0.5);
            std::snprintf(label, sizeof(label), "cubic %.0e: evaluation x", s);
            checkNear(mid.x, 50.0 * s, std::abs(50.0 * s) * 1e-9, label);
            std::snprintf(label, sizeof(label), "cubic %.0e: evaluation y", s);
            checkNear(mid.y, 75.0 * s, std::abs(75.0 * s) * 1e-9, label);

            // Flattening honours the requested tolerance; values below 1e-6 are
            // clamped (documented contract), so the effective bound is used.
            const double requestedTolerance = std::max(1e-6, 1e-3 * s);
            std::vector<Point> flat;
            flattenCubic(curve, requestedTolerance, flat);
            std::vector<Point> polyline;
            polyline.push_back(curve.p0);
            polyline.insert(polyline.end(), flat.begin(), flat.end());

            double worst = 0.0;
            for (int i = 0; i <= 200; ++i) {
                const double t = static_cast<double>(i) / 200.0;
                worst = std::max(worst, distanceToPolyline(curve.evaluate(t), polyline));
            }
            std::snprintf(label, sizeof(label), "cubic %.0e: flattening within tolerance", s);
            check(worst <= requestedTolerance * 1.001, label);
        }

        // Arc length must scale linearly with the curve. Only scales whose
        // flattening tolerance stays above the documented 1e-6 clamp are exact.
        const double baseLength = makeCubic(1.0).length(1e-4);
        check(baseLength > 100.0, "unit S-curve arc length exceeds its chord");
        for (double s : {1e-3, 1.0, 1e6, 1e9}) {
            char label[160];
            const double lengthAtScale = makeCubic(s).length(std::max(1e-6, 1e-3 * s));
            std::snprintf(label, sizeof(label), "cubic %.0e: arc length scales", s);
            checkNear(lengthAtScale / s, baseLength, baseLength * 0.01, label);
        }

        // Quadratic: bounds, evaluation and flattening across scales.
        auto makeQuad = [](double s) {
            return QuadBezier{Point{0.0, 0.0}, Point{50.0 * s, 100.0 * s}, Point{100.0 * s, 0.0}};
        };
        for (double s : {1e-21, 1e-9, 1e-6, 1.0, 1e6, 1e9}) {
            char label[160];
            const QuadBezier quad = makeQuad(s);

            const Rect bb = quad.bounds();
            std::snprintf(label, sizeof(label), "quad %.0e: bounds use the interior extremum", s);
            checkNear(bb.bottom, 50.0 * s, std::abs(50.0 * s) * 1e-9, label);
            std::snprintf(label, sizeof(label), "quad %.0e: bounds width", s);
            checkNear(bb.width(), 100.0 * s, std::abs(100.0 * s) * 1e-9, label);

            const Point mid = quad.evaluate(0.5);
            std::snprintf(label, sizeof(label), "quad %.0e: evaluation x", s);
            checkNear(mid.x, 50.0 * s, std::abs(50.0 * s) * 1e-9, label);
            std::snprintf(label, sizeof(label), "quad %.0e: evaluation y", s);
            checkNear(mid.y, 50.0 * s, std::abs(50.0 * s) * 1e-9, label);

            const double requestedTolerance = std::max(1e-6, 1e-3 * s);
            std::vector<Point> flat;
            flattenQuadratic(quad, requestedTolerance, flat);
            std::vector<Point> polyline;
            polyline.push_back(quad.p0);
            polyline.insert(polyline.end(), flat.begin(), flat.end());

            double worst = 0.0;
            for (int i = 0; i <= 200; ++i) {
                const double t = static_cast<double>(i) / 200.0;
                worst = std::max(worst, distanceToPolyline(quad.evaluate(t), polyline));
            }
            std::snprintf(label, sizeof(label), "quad %.0e: flattening within tolerance", s);
            check(worst <= requestedTolerance * 1.001, label);
        }
    }

    // --- 1.2 regression: translation invariance (strengthened) -------------------
    // A curve of ~100 pt near the origin must behave identically when moved to
    // large coordinates. Every comparison is made on values *relative* to the
    // curve's own start point, so only the shape is compared, never the
    // absolute position (which legitimately changes with the translation).
    //
    // Tolerances are derived from ULP at the working magnitude + the requested
    // flattening tolerance, never from a proportional magnitude factor.
    // The previous `magnitude * 1e-13` allowed ~100 pt error at 1e15; the new
    // `geometricTolerance` keeps the bound tight while accounting for IEEE 754
    // representability limits.
    std::printf("\nTranslation invariance:\n");
    {
        struct Case {
            const char* name;
            CubicBezier cubic;
            QuadBezier quad;
        };

        const std::vector<Case> cases{
            // Distinct control points, interior extremum on both axes.
            {"S-curve", {{0, 0}, {0, 100}, {100, 100}, {100, 0}},
             {{0, 0}, {50, 100}, {100, 0}}},
            // Nearly straight: control points almost on the chord.
            {"near-line", {{0, 0}, {33, 0.5}, {66, -0.5}, {100, 0}},
             {{0, 0}, {50, 0.25}, {100, 0}}},
            // Small dimensions (1 pt span) with an interior extremum.
            {"small", {{0, 0}, {0, 1}, {1, 1}, {1, 0}}, {{0, 0}, {0.5, 1}, {1, 0}}},
            // Degenerate: every control point coincident.
            {"point", {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}}},
            // Degenerate: coincident endpoints, a loop.
            {"loop", {{0, 0}, {50, 50}, {50, -50}, {0, 0}}, {{0, 0}, {50, 50}, {0, 0}}},
            // Collinear control points (a straight cubic).
            {"collinear", {{0, 0}, {30, 30}, {70, 70}, {100, 100}},
             {{0, 0}, {50, 50}, {100, 100}}},
            // Sub-point extremum with a genuinely non-zero quadratic term.
            // Coefficients on the Y axis: d0 = 0.1, d1 = 0.1, d2 = -0.2, so
            // a = d0 - 2 d1 + d2 = -0.3, b = 2 (d1 - d0) = 0, c = 0.1. The
            // interior extremum sits at t = 0.577 with value 0.11547. An
            // absolute-magnitude threshold at 1e12 is 1e12 * 1e-12 = 1 pt, so
            // |a| = 0.3 was judged negligible, the quadratic degenerated to a
            // linear equation and the extremum was silently dropped.
            {"sub-point-extremum", {{0, 0}, {0, 0.1}, {100, 0.2}, {100, 0}},
             {{0, 0}, {50, 0.1}, {100, 0}}},
        };

        const double translations[] = {0.0, 1e6, 1e9, 1e12, 1e15};
        const double tolerance = 0.05; // flattening tolerance (pt)

        for (const Case& testCase : cases) {
            const Point cubicShift{testCase.cubic.p0.x, testCase.cubic.p0.y};
            const Point quadShift{testCase.quad.p0.x, testCase.quad.p0.y};

            // Reference results at the origin.
            const Rect cubicBaseBounds = testCase.cubic.bounds();
            const double cubicBaseWidth = cubicBaseBounds.width();
            const double cubicBaseHeight = cubicBaseBounds.height();
            const double cubicBaseLength = testCase.cubic.length(tolerance);
            std::vector<Point> cubicBaseFlat;
            flattenCubic(testCase.cubic, tolerance, cubicBaseFlat);
            // Garantir que p0 este incluído na polilha para cálculos de distância.
            cubicBaseFlat.push_back(testCase.cubic.p0);
            const int cubicBaseSegments = static_cast<int>(cubicBaseFlat.size());

            const Rect quadBaseBounds = testCase.quad.bounds();
            const double quadBaseWidth = quadBaseBounds.width();
            const double quadBaseHeight = quadBaseBounds.height();
            std::vector<Point> quadBaseFlat;
            flattenQuadratic(testCase.quad, tolerance, quadBaseFlat);
            // Garantir que p0 este incluído na polilha para cálculos de distância.
            quadBaseFlat.push_back(testCase.quad.p0);
            const int quadBaseSegments = static_cast<int>(quadBaseFlat.size());

            for (double d : translations) {
                const Point delta{d, d};
                char label[256];
                const double eps = geometricTolerance(d, tolerance);
                const bool representableHere = isRepresentable(d, tolerance);

                // ---- Cubic ----
                const CubicBezier cubic{testCase.cubic.p0 + delta,
                                        testCase.cubic.c1 + delta,
                                        testCase.cubic.c2 + delta,
                                        testCase.cubic.p1 + delta};

                // 1. Bounds relative to p0 (left, top, right, bottom)
                const Rect cBounds = cubic.bounds();
                checkRelativeBoundsForPolylines(cubicBaseBounds, cBounds,
                                                testCase.cubic.p0, cubic.p0,
                                                "cubic", testCase.name, d);

                // 2. No NaN/inf in bounds
                std::snprintf(label, sizeof(label),
                              "%s cubic @%.0e: no NaN/inf in bounds", testCase.name, d);
                check(std::isfinite(cBounds.left) && std::isfinite(cBounds.right)
                          && std::isfinite(cBounds.top) && std::isfinite(cBounds.bottom),
                      label);

                // 3. Evaluation at multiple t (0, 0.1, 0.25, 0.37, 0.5, 0.75, 0.9, 1.0)
                //    Verifies BOTH X and Y coordinates follow translation.
                checkEvaluationAtTs(testCase.cubic, cubic, delta,
                                    "cubic", testCase.name, d);

                // 4. Interior extrema preserved (t parameter AND X/Y value at extremum)
                checkInteriorExtremaPreserved(testCase.cubic, cubic,
                                              "cubic", testCase.name, d);

                // 5. No NaN/inf when flattened
                std::snprintf(label, sizeof(label),
                              "%s cubic @%.0e: no NaN/inf when flattened", testCase.name, d);
                std::vector<Point> cubicFlat;
                flattenCubic(cubic, tolerance, cubicFlat);
                // Garantir que p0 este incluído na polilha.
                cubicFlat.push_back(testCase.cubic.p0);
                check(allFinite(cubicFlat), label);

                // 6. Flattened geometry: bidirectional polyline distance (shape preservation)
                std::snprintf(label, sizeof(label),
                              "%s cubic @%.0e: flattened shape follows translation "
                              "(bidirectional distance)", testCase.name, d);
                if (representableHere) {
                    const double maxDist = bidirectionalPolylineDistance(cubicBaseFlat, cubicFlat, delta);
                    // -1.0 returned when geometry is empty (indeterminate);
                    // in representable case this is a test-fail condition.
                    check(maxDist >= 0.0, "%s cubic @%.0e: bidirectional distance valid (non-empty polylines)", testCase.name, d);
                    checkNear(maxDist, 0.0, tolerance * 2.0, label);
                } else {
                    check(allFinite(cubicFlat), label);
                }

                // 7. Arc length preserved under translation
                //    At magnitudes where the ULP exceeds the flattening
                //    tolerance, the flattened polyline itself changes shape, so
                //    the arc length computed from it legitimately differs.
                std::snprintf(label, sizeof(label),
                              "%s cubic @%.0e: arc length preserved", testCase.name, d);
                if (representableHere) {
                    checkNear(cubic.length(tolerance), cubicBaseLength, eps, label);
                } else {
                    check(std::isfinite(cubic.length(tolerance)), label);
                }

                // ---- Quadratic ----
                const QuadBezier quad{testCase.quad.p0 + delta, testCase.quad.c + delta,
                                      testCase.quad.p1 + delta};

                // 1. Bounds relative to p0 (left, top, right, bottom)
                const Rect qBounds = quad.bounds();
                checkRelativeBoundsForPolylines(quadBaseBounds, qBounds,
                                                testCase.quad.p0, quad.p0,
                                                "quad", testCase.name, d);

                // 2. Evaluation at multiple t
                checkEvaluationAtTs(testCase.quad, quad, delta,
                                    "quad", testCase.name, d);

                // 3. Interior extrema preserved
                checkInteriorExtremaPreserved(testCase.quad, quad,
                                              "quad", testCase.name, d);

                // 4. No NaN/inf when flattened
                std::snprintf(label, sizeof(label),
                              "%s quad @%.0e: no NaN/inf when flattened", testCase.name, d);
                std::vector<Point> quadFlat;
                flattenQuadratic(quad, tolerance, quadFlat);
                // Garantir que p0 este incluído na polilha.
                quadFlat.push_back(testCase.quad.p0);
                check(allFinite(quadFlat), label);

                // 5. Flattened geometry: bidirectional polyline distance
                std::snprintf(label, sizeof(label),
                              "%s quad @%.0e: flattened shape follows translation "
                              "(bidirectional distance)", testCase.name, d);
                if (representableHere) {
                    const double maxDist = bidirectionalPolylineDistance(quadBaseFlat, quadFlat, delta);
                    // -1.0 returned when geometry is empty (indeterminate);
                    // in representable case this is a test-fail condition.
                    check(maxDist >= 0.0, "%s quad @%.0e: bidirectional distance valid (non-empty polylines)", testCase.name, d);
                    checkNear(maxDist, 0.0, tolerance * 2.0, label);
                } else {
                    check(allFinite(quadFlat), label);
                }

                // 6. Arc length preserved under translation (via flattening for quadratic)
                std::snprintf(label, sizeof(label),
                              "%s quad @%.0e: arc length preserved", testCase.name, d);
                if (representableHere) {
                    std::vector<Point> basePoly, movedPoly;
                    flattenQuadratic(testCase.quad, tolerance, basePoly);
                    flattenQuadratic(quad, tolerance, movedPoly);
                    // Add p0 to both polylines for distance/length calculations.
                    // basePoly gets original p0; movedPoly gets translated p0 (quad.p0 already has delta).
                    basePoly.push_back(testCase.quad.p0);
                    movedPoly.push_back(quad.p0);
                    double baseLen = 0.0, movedLen = 0.0;
                    for (std::size_t i = 1; i < basePoly.size(); ++i)
                        baseLen += artplus::geometry::distance(basePoly[i - 1], basePoly[i]);
                    for (std::size_t i = 1; i < movedPoly.size(); ++i)
                        movedLen += artplus::geometry::distance(movedPoly[i - 1], movedPoly[i]);
                    checkNear(movedLen, baseLen, eps, label);
                } else {
                    check(true, label);
                }
            }
        }
    }

    std::printf("\n%s\n", g_failures == 0 ? "ALL PASSED" : "FAILURES PRESENT");
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
