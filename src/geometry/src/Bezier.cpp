#include "artplus/geometry/Bezier.h"

#include <algorithm>
#include <cmath>

namespace artplus::geometry {

namespace {

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
// This is the geometric size of the curve. Using the absolute coordinate
// magnitude instead (as stage 1.1 did) made every threshold depend on how far
// the curve sits from the document origin: at 1e12 a 100 pt curve has "scale"
// 1e12, so a legitimate 0.25 pt deviation was judged negligible and the extremum
// was dropped, and at 1e15 the flatness test changed the segment count.
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


// Flatness test: the sum of both control points' perpendicular distances from
// the chord is compared against tolerance * chordLength. This is the classic
// "distance of control polygon to chord" bound and over-estimates the true
// deviation, so it never accepts a segment that is further off than tolerance.
bool cubicIsFlat(const Point& p0, const Point& c1, const Point& c2, const Point& p1,
                 double tolerance)
{
    // Everything here is already expressed through differences of control
    // points, so a common translation cancels exactly and the flatness verdict
    // (hence the number of flattened segments) is translation invariant.
    const double dx = p1.x - p0.x;
    const double dy = p1.y - p0.y;
    const double lenSq = dx * dx + dy * dy;

    // Curve-local extent: the largest control point offset from p0.
    const double extent = localExtent(p0, c1, c2, p1);

    if (!(lenSq > 0.0) || (extent > 0.0 && std::abs(dx) <= kRelativeEpsilon * extent
                           && std::abs(dy) <= kRelativeEpsilon * extent)) {
        // Chord collapses towards a point: the deviation from a degenerate curve
        // is the control point spread, measured relative to p0 as well.
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

    // Coefficients are expressed relative to p0 so they describe the curve's
    // shape only. Adding a constant offset to every control point leaves the
    // differences (and therefore the roots) unchanged, which is what makes the
    // result independent of the curve's distance from the origin.
    //   B'(t) = 2[(1-t)(c-p0) + t(p1-c)] = 2[ d0 + t (d1 - d0) ]
    //   with d0 = c-p0 and d1 = p1-c, so the single root is t = -d0 / (d1-d0).
    for (int axis = 0; axis < 2; ++axis) {
        const double d0 = axis == 0 ? c.x - p0.x : c.y - p0.y;
        const double d1 = axis == 0 ? p1.x - c.x : p1.y - c.y;
        const double den = d1 - d0;
        // Scale from the curve-local difference magnitudes, never from |v0|.
        const double scale = std::max(std::abs(d0), std::abs(d1));
        if (isNegligible(den, scale))
            continue; // d0 == d1: the derivative never vanishes (or is identically 0)
        const double t = -d0 / den;
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

    // All coefficients are computed from *relative* control points so that they
    // depend on the curve's shape only. A common translation cancels in every
    // difference, so the roots -- and hence the bounds -- cannot change when the
    // curve is moved far from the origin. Stage 1.1 derived `scale` from the
    // absolute coordinates, which made the degeneracy test depend on position.
    //
    //   B'(t)/3 = A t^2 + B t + C
    //   A = d0 - 2 d1 + d2,  B = 2 (d1 - d0),  C = d0
    //   with d0 = c1-p0, d1 = c2-c1, d2 = p1-c2 (the three edge vectors).
    // Verified symbolically: expanding B'(t) with P0 = 0, P1 = d0,
    // P2 = d0+d1, P3 = d0+d1+d2 gives 3[A t^2 + B t + C].
    for (int axis = 0; axis < 2; ++axis) {
        const double d0 = axis == 0 ? c1.x - p0.x : c1.y - p0.y;
        const double d1 = axis == 0 ? c2.x - c1.x : c2.y - c1.y;
        const double d2 = axis == 0 ? p1.x - c2.x : p1.y - c2.y;

        const double a = d0 - 2.0 * d1 + d2;
        const double b = 2.0 * (d1 - d0);
        const double c = d0;

        // Curve-local magnitude, built from the differences above.
        const double scale = std::max({std::abs(d0), std::abs(d1), std::abs(d2),
                                       std::abs(a), std::abs(b), std::abs(c)});

        double roots[2] = {0.0, 0.0};
        int count = 0;
        appendInteriorQuadraticRoots(a, b, c, scale, roots, count);
        for (int i = 0; i < count; ++i)
            r.expandToInclude(evaluate(roots[i]));
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
