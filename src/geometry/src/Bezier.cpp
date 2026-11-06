#include "artplus/geometry/Bezier.h"

#include <algorithm>
#include <cmath>

namespace artplus::geometry {

namespace {

constexpr double kMinFlattenTolerance = 1e-6;

Point lerp3(const Point& a, const Point& b, const Point& c, double t)
{
    return lerp(lerp(a, b, t), lerp(b, c, t), t);
}

Point lerp4(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    return lerp(lerp3(a, b, c, t), lerp3(b, c, d, t), t);
}

// Flatness test: the sum of both control points' perpendicular distances from
// the chord is compared against tolerance * chordLength. This is the classic
// "distance of control polygon to chord" bound and over-estimates the true
// deviation, so it never accepts a segment that is further off than tolerance.
bool cubicIsFlat(const Point& p0, const Point& c1, const Point& c2, const Point& p1,
                 double tolerance)
{
    const double dx = p1.x - p0.x;
    const double dy = p1.y - p0.y;
    const double lenSq = dx * dx + dy * dy;

    if (lenSq < 1e-18) {
        // Degenerate chord: the curve collapses towards a point. Measure the
        // control point spread directly.
        return std::max(distance(p0, c1), distance(p0, c2)) <= tolerance;
    }

    const double d1 = std::abs((c1.x - p1.x) * dy - (c1.y - p1.y) * dx);
    const double d2 = std::abs((c2.x - p1.x) * dy - (c2.y - p1.y) * dx);
    const double sum = d1 + d2;
    return sum * sum <= tolerance * tolerance * lenSq;
}

double clampTolerance(double tolerance)
{
    return std::max(tolerance, kMinFlattenTolerance);
}

} // namespace

Point evaluateQuadratic(const Point& p0, const Point& c, const Point& p1, double t)
{
    return lerp3(p0, c, p1, t);
}

Point evaluateCubic(const Point& p0, const Point& c1, const Point& c2, const Point& p1, double t)
{
    return lerp4(p0, c1, c2, p1, t);
}

// --- Quadratic -------------------------------------------------------------

Point QuadBezier::evaluate(double t) const
{
    return evaluateQuadratic(p0, c, p1, t);
}

Point QuadBezier::derivative(double t) const
{
    const Point a = (c - p0) * (2.0 * (1.0 - t));
    const Point b = (p1 - c) * (2.0 * t);
    return a + b;
}

void QuadBezier::split(double t, QuadBezier& left, QuadBezier& right) const
{
    const Point ac = lerp(p0, c, t);
    const Point cb = lerp(c, p1, t);
    const Point mid = lerp(ac, cb, t);
    left = {p0, ac, mid};
    right = {mid, cb, p1};
}

Rect QuadBezier::bounds() const
{
    Rect r(p0.x, p0.y, p0.x, p0.y);
    r.expandToInclude(p1);

    // Extrema per axis: B'(t) = 0 with B'(t) = 2[(1-t)(c-p0) + t(p1-c)].
    for (int axis = 0; axis < 2; ++axis) {
        const double v0 = axis == 0 ? p0.x : p0.y;
        const double vc = axis == 0 ? c.x : c.y;
        const double v1 = axis == 0 ? p1.x : p1.y;
        const double den = v0 - 2.0 * vc + v1;
        if (std::abs(den) < 1e-18)
            continue; // control point is the chord midpoint: no interior extremum
        const double t = (v0 - vc) / den;
        if (t > 0.0 && t < 1.0)
            r.expandToInclude(evaluate(t));
    }
    return r;
}

// --- Cubic -----------------------------------------------------------------

Point CubicBezier::evaluate(double t) const
{
    return evaluateCubic(p0, c1, c2, p1, t);
}

Point CubicBezier::derivative(double t) const
{
    const double mt = 1.0 - t;
    const Point a = (c1 - p0) * (3.0 * mt * mt);
    const Point b = (c2 - c1) * (6.0 * mt * t);
    const Point c = (p1 - c2) * (3.0 * t * t);
    return a + b + c;
}

void CubicBezier::split(double t, CubicBezier& left, CubicBezier& right) const
{
    const Point p01 = lerp(p0, c1, t);
    const Point p12 = lerp(c1, c2, t);
    const Point p23 = lerp(c2, p1, t);
    const Point p012 = lerp(p01, p12, t);
    const Point p123 = lerp(p12, p23, t);
    const Point mid = lerp(p012, p123, t);
    left = {p0, p01, p012, mid};
    right = {mid, p123, p23, p1};
}

Rect CubicBezier::bounds() const
{
    Rect r(p0.x, p0.y, p0.x, p0.y);
    r.expandToInclude(p1);

    // Per-axis derivative: 3(a t^2 + b t + c) with
    //   a = -v0 + 3 v1 - 3 v2 + v3
    //   b = 2 (v0 - 2 v1 + v2)
    //   c = v1 - v0
    for (int axis = 0; axis < 2; ++axis) {
        const double v0 = axis == 0 ? p0.x : p0.y;
        const double v1 = axis == 0 ? c1.x : c1.y;
        const double v2 = axis == 0 ? c2.x : c2.y;
        const double v3 = axis == 0 ? p1.x : p1.y;

        const double a = -v0 + 3.0 * v1 - 3.0 * v2 + v3;
        const double b = 2.0 * (v0 - 2.0 * v1 + v2);
        const double c = v1 - v0;

        auto consider = [&](double t) {
            if (t > 0.0 && t < 1.0)
                r.expandToInclude(evaluate(t));
        };

        if (std::abs(a) < 1e-12) {
            if (std::abs(b) > 1e-12)
                consider(-c / b);
        } else {
            const double disc = b * b - 4.0 * a * c;
            if (disc >= 0.0) {
                const double sq = std::sqrt(disc);
                consider((-b + sq) / (2.0 * a));
                consider((-b - sq) / (2.0 * a));
            }
        }
    }
    return r;
}

QuadBezier CubicBezier::toQuadratic() const
{
    // Least-squares style approximation for a cubic: q.c = (3(c1 + c2) - (p0 + p1)) / 4.
    const Point sum = c1 + c2;
    const Point ends = p0 + p1;
    return {p0, (sum * 3.0 - ends) / 4.0, p1};
}

double CubicBezier::length(double tolerance) const
{
    std::vector<Point> points;
    flattenCubic(*this, tolerance, points);

    double total = 0.0;
    Point previous = p0;
    for (const Point& p : points) {
        total += distance(previous, p);
        previous = p;
    }
    return total;
}

// --- Flattening ------------------------------------------------------------

void flattenQuadratic(const QuadBezier& curve, double tolerance, std::vector<Point>& out)
{
    // Degree elevation to a cubic is exact: a quadratic is a cubic whose control
    // points sit 2/3 of the way towards the quadratic control point.
    const Point c1 = curve.p0 + (curve.c - curve.p0) * (2.0 / 3.0);
    const Point c2 = curve.p1 + (curve.c - curve.p1) * (2.0 / 3.0);
    flattenCubic(curve.p0, c1, c2, curve.p1, tolerance, out);
}

void flattenCubic(const CubicBezier& curve, double tolerance, std::vector<Point>& out)
{
    flattenCubic(curve.p0, curve.c1, curve.c2, curve.p1, tolerance, out);
}

void flattenCubic(const Point& p0, const Point& c1, const Point& c2, const Point& p1,
                  double tolerance, std::vector<Point>& out)
{
    const double tol = clampTolerance(tolerance);
    if (cubicIsFlat(p0, c1, c2, p1, tol)) {
        out.push_back(p1);
        return;
    }

    CubicBezier left;
    CubicBezier right;
    CubicBezier{p0, c1, c2, p1}.split(0.5, left, right);
    flattenCubic(left.p0, left.c1, left.c2, left.p1, tol, out);
    flattenCubic(right.p0, right.c1, right.c2, right.p1, tol, out);
}

} // namespace artplus::geometry
