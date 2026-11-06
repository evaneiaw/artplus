#pragma once

// Line segments and segment/segment intersections (Qt-free core).

#include "artplus/geometry/Point.h"

namespace artplus::geometry {

// A finite segment from p0 to p1. Parameter t runs 0 -> p0, 1 -> p1.
struct Line {
    Point p0;
    Point p1;

    constexpr Line() = default;
    constexpr Line(const Point& a, const Point& b) : p0(a), p1(b) {}

    constexpr Point direction() const { return p1 - p0; }
    double length() const;
    double lengthSquared() const;
    Rect bounds() const;
    Point pointAt(double t) const;
    // Unclamped projection parameter of p on the infinite support line.
    // Returns 0 for a degenerate (zero-length) segment.
    double parameterAt(const Point& p) const;
    // Distance from p to the segment (clamped to the endpoints), not the line.
    double distanceToPoint(const Point& p) const;
};

enum class IntersectionKind {
    None,    // disjoint, or parallel and distinct
    Point,   // exactly one intersection point (crossing or endpoint touch)
    Overlap  // collinear and overlapping over a non-zero length
};

struct LineIntersection {
    IntersectionKind kind = IntersectionKind::None;
    Point point;              // valid when kind == Point
    double t = 0.0;           // parameter on the first segment
    double u = 0.0;           // parameter on the second segment
    double overlapT0 = 0.0;   // valid when kind == Overlap (first segment)
    double overlapT1 = 0.0;
    double overlapLength = 0.0;

    bool hasPoint() const { return kind == IntersectionKind::Point; }
    bool isOverlap() const { return kind == IntersectionKind::Overlap; }
};

// Intersects two finite segments.
//
// Handles: proper crossings, endpoint touches, T-junctions, collinear overlaps
// (reported as Overlap with the shared interval on the first segment),
// collinear segments that only touch at one endpoint (reported as Point), and
// zero-length segments (treated as a point tested against the other segment).
// `epsilon` is an absolute distance tolerance in points.
LineIntersection intersectSegments(const Line& a, const Line& b,
                                   double epsilon = kGeometryEpsilon);

// Intersects the two infinite support lines. Parallel lines (including
// collinear ones) report kind == None because an overlap has no bounded
// interval. The returned t/u may lie outside [0, 1].
LineIntersection intersectInfiniteLines(const Line& a, const Line& b,
                                        double epsilon = kGeometryEpsilon);

// Distance from p to the segment, clamped to the endpoints.
double pointSegmentDistance(const Point& p, const Line& segment);

} // namespace artplus::geometry
