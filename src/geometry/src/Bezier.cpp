#include "artplus/geometry/Bezier.h"

#include <algorithm>
#include <cmath>

namespace artplus::geometry {

constexpr double kMinFlattenTolerance = 1e-6;

// Relative head-room for the "is the denominator zero?" tests. A coefficient is
// treated as zero when it is negligible compared with the magnitudes it is
// derived from, never against an absolute constant: an absolute 1e-12 silently
// discarded valid extrema on curves at 1e-15 scale.
constexpr double kRelativeEpsilon = 1e-12;

Point lerp3(const Point& a, const Point& b, const Point& c, double t)
{
    return lerp(lerp(a, b, t), lerp(b, c, t), t);
}

Point lerp4(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    return lerp(lerp3(a, b, c, t), lerp3(b, c, d, t), t);
}

// |value| is negligible against `scale`.
bool isNegligible(double value, double scale)
{
    return std::abs(value) <= kRelativeEpsilon * std::abs(scale);
}

// Curve-local magnitude: the largest distance of any control point from `origin`.
double localExtent(const Point& origin, const Point& a, const Point& b, const Point& c)
{
    return std::max({std::abs(a.x - origin.x), std::abs(a.y - origin.y),
                     std::abs(b.x - origin.x), std::abs(b.y - origin.y),
                     std::abs(c.x - origin.x), std::abs(c.y - origin.y)});
}

// Solves a t^2 + b t + c = 0 in a cancellation-safe way and appends the roots
// that fall strictly inside (0, 1) to `out`. `scale` is the curve-local
// magnitude the coefficients were derived from, used to decide degeneracy
// relatively (never against the absolute coordinates).
void appendInteriorQuadraticRoots(double a, double b, double c, double scale,
                                  double (&out)[2], int& count)
{
    if (isNegligible(a, scale)) {
        // Degenerates to the linear equation b t + c = 0.
        if (!isNegligible(b, scale)) {
            const double t = -c / b;
            if (t > 0.0 && t < 1.0)
                out[count++] = t;
        }
        return;
    }

    const double disc = b * b - 4.0 * a * c;
    const double discScale = b * b + std::abs(4.0 * a * c);
    if (!(disc >= 0.0 || isNegligible(disc, discScale)))
        return; // no real root

    const double sq = std::sqrt(std::max(disc, 0.0));
    // Cancellation-safe quadratic formula: compute the root whose numerator does
    // not subtract nearly equal quantities, then recover the other root from the
    // product of the roots (c/a). When b == 0 the two roots are +/- sqrt(-c/a)
    // and q would be zero, so the symmetric form is used instead.
    double t0;
    double t1;
    if (b == 0.0) {
        t0 = sq / (2.0 * a);
        t1 = -sq / (2.0 * a);
    } else {
        const double q = -0.5 * (b + (b > 0.0 ? sq : -sq));
        t0 = q / a;
        t1 = (q != 0.0) ? c / q : -b / (2.0 * a);
    }
    const double roots[2] = {t0, t1};
    for (double t : roots) {
        if (t > 0.0 && t < 1.0 && count < 2)
            out[count++] = t;
    }
}


double clampTolerance(double tolerance)
{
    return std::max(tolerance, kMinFlattenTolerance);
}

// ========================================================================
// Bézier–line segment intersections
// ========================================================================

// Quadratic Bezier × line segment analytical intersection.
QuadBezierIntersection intersectQuadBezierLine(const QuadBezier& bezier,
                                               const Line& line,
                                               double epsilon)
{
    QuadBezierIntersection result;
    result.kind = QuadBezierIntersectionKind::None;

    // Parametric forms:
    // Quadratic Bezier: Q(t) = (1-t)^2 * P0 + 2(1-t)t * C + t^2 * P1, t in [0,1]
    // Line: L(u) = A + u * (B - A), u in [0,1], where A=line.p0, B=line.p1

    // Expand Q(t):
    // Q(t) = P0 + 2t(C-P0) + t^2(P1 - 2C + P0)

    // Set Q(t) = L(u):
    // P0 + 2t(C-P0) + t^2(P1 - 2C + P0) = A + u(B-A)

    // Rearrange: t^2 * D + 2t * E + F = u * G
    // where D = P1 - 2C + P0, E = C - P0, F = P0 - A, G = B - A

    // Take 2D cross product with G to eliminate u:
    // (t^2 * D + 2t * E + F) x G = 0
    // => t^2 * (D x G) + 2t * (E x G) + (F x G) = 0

    auto cross = [](const Point& a, const Point& b) { return a.x * b.y - a.y * b.x; };

    Point D = bezier.p1 - bezier.c * 2.0 + bezier.p0;
    Point E = bezier.c - bezier.p0;
    Point F = bezier.p0 - line.p0;
    Point G = line.p1 - line.p0;

    double a = cross(D, G);
    double b = 2.0 * cross(E, G);
    double c = cross(F, G);

    // Solve quadratic: a*t^2 + b*t + c = 0
    if (std::abs(a) < 1e-300 && std::abs(b) < 1e-300) {
        result.kind = QuadBezierIntersectionKind::None;
        return result;
    }

    double t1, t2;
    if (std::abs(a) < 1e-300) {
        // Linear case: b*t + c = 0
        t1 = -c / b;
        t2 = t1;
    } else {
        double disc = b * b - 4.0 * a * c;
        if (disc < 0.0) {
            result.kind = QuadBezierIntersectionKind::None;
            return result;
        }
        double sq = std::sqrt(std::max(disc, 0.0));
        t1 = (-b + sq) / (2.0 * a);
        t2 = (-b - sq) / (2.0 * a);
    }

    // Try each valid t in [0,1], compute u and verify the point is on the line segment
    auto try_t = [&](double t) {
        if (t < -epsilon || t > 1.0 + epsilon)
            return;

        Point qt = bezier.evaluate(t);

        // Compute u from qt = line.p0 + u * G
        double u;
        if (std::abs(G.x) > std::abs(G.y)) {
            if (std::abs(G.x) > epsilon)
                u = (qt.x - line.p0.x) / G.x;
            else
                u = (qt.y - line.p0.y) / G.y;
        } else {
            if (std::abs(G.y) > epsilon)
                u = (qt.y - line.p0.y) / G.y;
            else
                u = (qt.x - line.p0.x) / G.x;
        }

        // Check u in [0,1] and point within epsilon of the line segment
        if (u >= -epsilon && u <= 1.0 + epsilon) {
            // Verify distance from qt to the line segment is within epsilon
            double dist = pointSegmentDistance(qt, Line{line.p0, line.p1});
            if (dist <= epsilon) {
                result.kind = QuadBezierIntersectionKind::Point;
                result.point = qt;
                result.t = t;
                result.u = u;
            }
        }
    };

    try_t(t1);

    if (std::abs(t2 - t1) > 1e-15) {
        try_t(t2);
    }

    return result;
}

// Cubic Bezier × line segment intersection using flattening approach.
CubicBezIntersection intersectCubicBezierLine(const CubicBezier& bezier,
                                              const Line& line,
                                              double epsilon)
{
    CubicBezIntersection result;
    result.kind = CubicBezIntersectionKind::None;

    // Practical approach: flatten the cubic Bézier to a polyline with tight tolerance,
    // then use the existing robust intersectSegments() function.
    // This is the method used by most CAD systems for Bézier-line intersections.

    // Flatten the cubic Bézier
    std::vector<Point> flat;
    flattenCubic(bezier, epsilon * 0.1, flat);

    // Ensure p0 is at the start
    flat.insert(flat.begin(), bezier.p0);

    // Test each segment of the flattened polyline against the line segment
    for (std::size_t i = 1; i < flat.size(); ++i) {
        Line seg{flat[i - 1], flat[i]};

        LineIntersection hit = intersectSegments(Line{line.p0, line.p1}, seg);

        if (hit.hasPoint()) {
            if (hit.t >= -epsilon && hit.t <= 1.0 + epsilon && hit.u >= -epsilon && hit.u <= 1.0 + epsilon) {
                result.kind = CubicBezIntersectionKind::Point;
                result.point = hit.point;
                result.t = hit.t;
                result.u = hit.u;
                return result;
            }
        }
    }

    return result;
}

} // namespace artplus::geometry