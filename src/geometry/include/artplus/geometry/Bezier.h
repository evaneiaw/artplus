#pragma once

// Quadratic and cubic Bezier curves plus tolerance-driven flattening.
// Qt-free core: results are returned through std::vector.

#include "artplus/geometry/Point.h"
#include "artplus/geometry/Line.h"

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

// Intersect a quadratic Bezier with a line segment.
enum class QuadBezierIntersectionKind {
    None,
    Point,     // exactly one intersection point
    Overlap,   // collinear overlap
};

struct QuadBezierIntersection {
    QuadBezierIntersectionKind kind = QuadBezierIntersectionKind::None;
    Point point;              // valid when kind == Point
    double t = 0.0;           // parameter on the quadratic Bezier (0..1)
    double u = 0.0;           // parameter on the line segment (0..1)
};

/// Intersect a quadratic Bezier curve with a line segment.
/// \param bezier   The quadratic Bezier curve.
/// \param line     The line segment.
/// \param epsilon  Absolute distance tolerance in points (default: kGeometryEpsilon).
QuadBezierIntersection intersectQuadBezierLine(const QuadBezier& bezier,
                                                const Line& line,
                                                double epsilon = kGeometryEpsilon);

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
    QuadBezier toQuadratic() const;
    double length(double tolerance = 0.01) const;
};

// Intersect a cubic Bezier with a line segment.
enum class CubicBezIntersectionKind {
    None,
    Point,
    Overlap,
};

struct CubicBezIntersection {
    CubicBezIntersectionKind kind = CubicBezIntersectionKind::None;
    Point point;
    double t = 0.0;           // parameter on the cubic Bezier (0..1)
    double u = 0.0;           // parameter on the line segment (0..1);
};

/// Intersect a cubic Bezier curve with a line segment.
/// \param bezier   The cubic Bezier curve.
/// \param line     The line segment.
/// \param epsilon  Absolute distance tolerance in points (default: kGeometryEpsilon).
CubicBezIntersection intersectCubicBezierLine(const CubicBezier& bezier,
                                               const Line& line,
                                               double epsilon = kGeometryEpsilon);

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
