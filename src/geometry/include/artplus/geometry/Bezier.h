#pragma once

// Quadratic and cubic Bezier curves plus tolerance-driven flattening.
// Qt-free core: results are returned through std::vector.

#include "artplus/geometry/Point.h"

#include <vector>

namespace artplus::geometry {

struct QuadBezier {
    Point p0; // start
    Point c;  // single control point
    Point p1; // end

    constexpr QuadBezier() = default;
    constexpr QuadBezier(const Point& start, const Point& control, const Point& end)
        : p0(start), c(control), p1(end) {}

    Point evaluate(double t) const;
    Point derivative(double t) const;
    void split(double t, QuadBezier& left, QuadBezier& right) const;
    // Exact axis-aligned bounds (curve extrema, not the control polygon).
    Rect bounds() const;
};

struct CubicBezier {
    Point p0; // start
    Point c1; // control 1
    Point c2; // control 2
    Point p1; // end

    constexpr CubicBezier() = default;
    constexpr CubicBezier(const Point& start, const Point& control1, const Point& control2,
                          const Point& end)
        : p0(start), c1(control1), c2(control2), p1(end) {}

    Point evaluate(double t) const;
    Point derivative(double t) const;
    void split(double t, CubicBezier& left, CubicBezier& right) const;
    // Exact axis-aligned bounds (curve extrema, not the control polygon).
    Rect bounds() const;
    // Exact degree elevation is not possible in the other direction; this
    // returns the standard quadratic approximation (midpoint control), which is
    // only intended for API compatibility, never for accurate geometry.
    QuadBezier toQuadratic() const;
    // Arc length approximated through flattening with the given tolerance.
    double length(double tolerance = 0.01) const;
};

// Flattening appends points *after* the curve start: the first appended point is
// the curve end (or an intermediate point). Callers that need the start point
// must push it themselves, which matches Path::flatten().
//
// `tolerance` is the maximum allowed deviation in points and is clamped to a
// minimum of 1e-6 so that a degenerate input cannot recurse without bound.
void flattenQuadratic(const QuadBezier& curve, double tolerance, std::vector<Point>& out);
void flattenCubic(const CubicBezier& curve, double tolerance, std::vector<Point>& out);

// Convenience overload mirroring the historical free function.
void flattenCubic(const Point& p0, const Point& c1, const Point& c2, const Point& p1,
                  double tolerance, std::vector<Point>& out);

// Point on a quadratic curve evaluated with de Casteljau (numerically stable).
Point evaluateQuadratic(const Point& p0, const Point& c, const Point& p1, double t);
Point evaluateCubic(const Point& p0, const Point& c1, const Point& c2, const Point& p1, double t);

} // namespace artplus::geometry
