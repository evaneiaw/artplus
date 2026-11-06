#include "artplus/geometry/Line.h"

#include <algorithm>
#include <cmath>

namespace artplus::geometry {

namespace {

constexpr double kDegenerateLengthSquared = 1e-300;

double cross(const Point& a, const Point& b)
{
    return a.x * b.y - a.y * b.x;
}

// Perpendicular distance from p to the infinite support line of `line`.
double pointLineDistance(const Point& p, const Line& line, double directionLength)
{
    if (directionLength <= 0.0)
        return distance(p, line.p0);
    return std::abs(cross(line.direction(), p - line.p0)) / directionLength;
}

} // namespace

double Line::length() const
{
    return (p1 - p0).length();
}

double Line::lengthSquared() const
{
    return (p1 - p0).lengthSquared();
}

Rect Line::bounds() const
{
    return {std::min(p0.x, p1.x), std::min(p0.y, p1.y),
            std::max(p0.x, p1.x), std::max(p0.y, p1.y)};
}

Point Line::pointAt(double t) const
{
    return lerp(p0, p1, t);
}

double Line::parameterAt(const Point& p) const
{
    const Point d = p1 - p0;
    const double lenSq = d.lengthSquared();
    if (lenSq <= kDegenerateLengthSquared)
        return 0.0;
    return ((p - p0).x * d.x + (p - p0).y * d.y) / lenSq;
}

double Line::distanceToPoint(const Point& p) const
{
    return pointSegmentDistance(p, *this);
}

double pointSegmentDistance(const Point& p, const Line& segment)
{
    const Point d = segment.p1 - segment.p0;
    const double lenSq = d.lengthSquared();
    if (lenSq <= kDegenerateLengthSquared)
        return distance(p, segment.p0);
    double t = ((p - segment.p0).x * d.x + (p - segment.p0).y * d.y) / lenSq;
    t = std::clamp(t, 0.0, 1.0);
    return distance(p, segment.p0 + d * t);
}

LineIntersection intersectInfiniteLines(const Line& a, const Line& b, double epsilon)
{
    LineIntersection result;
    const Point d1 = a.direction();
    const Point d2 = b.direction();
    const double l1 = d1.length();
    const double l2 = d2.length();
    if (l1 <= epsilon || l2 <= epsilon)
        return result; // a zero-length segment has no support line

    const double denom = cross(d1, d2);
    // sin(angle between the lines) = denom / (l1 * l2); compare against a
    // dimensionless threshold so the test is scale independent.
    if (std::abs(denom) <= epsilon * l1 * l2)
        return result; // parallel (collinear included: no bounded intersection)

    const Point delta = b.p0 - a.p0;
    const double t = cross(delta, d2) / denom;
    const double u = cross(delta, d1) / denom;

    result.kind = IntersectionKind::Point;
    result.t = t;
    result.u = u;
    result.point = a.pointAt(t);
    return result;
}

LineIntersection intersectSegments(const Line& a, const Line& b, double epsilon)
{
    LineIntersection result;

    const Point d1 = a.direction();
    const Point d2 = b.direction();
    const double l1 = d1.length();
    const double l2 = d2.length();

    const bool aDegenerate = l1 <= epsilon;
    const bool bDegenerate = l2 <= epsilon;

    // --- Degenerate segments are treated as points. -----------------------
    if (aDegenerate && bDegenerate) {
        if (distance(a.p0, b.p0) <= epsilon) {
            result.kind = IntersectionKind::Point;
            result.point = a.p0;
        }
        return result;
    }
    if (aDegenerate) {
        if (pointSegmentDistance(a.p0, b) <= epsilon) {
            result.kind = IntersectionKind::Point;
            result.point = a.p0;
            result.u = std::clamp(b.parameterAt(a.p0), 0.0, 1.0);
        }
        return result;
    }
    if (bDegenerate) {
        if (pointSegmentDistance(b.p0, a) <= epsilon) {
            result.kind = IntersectionKind::Point;
            result.point = b.p0;
            result.t = std::clamp(a.parameterAt(b.p0), 0.0, 1.0);
        }
        return result;
    }

    const double denom = cross(d1, d2);
    if (std::abs(denom) <= epsilon * l1 * l2) {
        // Parallel. They can still overlap when collinear within epsilon.
        if (pointLineDistance(b.p0, a, l1) > epsilon
            || pointLineDistance(b.p1, a, l1) > epsilon) {
            return result; // distinct parallel lines
        }

        double t0 = a.parameterAt(b.p0);
        double t1 = a.parameterAt(b.p1);
        double lo = std::max(std::min(t0, t1), 0.0);
        double hi = std::min(std::max(t0, t1), 1.0);

        const double paramEps = epsilon / l1;
        if (hi < lo - paramEps)
            return result; // collinear but disjoint

        if (hi - lo <= paramEps) {
            // Collinear, touching at a single point (endpoint contact).
            const double t = std::clamp((lo + hi) / 2.0, 0.0, 1.0);
            result.kind = IntersectionKind::Point;
            result.t = t;
            result.point = a.pointAt(t);
            result.u = std::clamp(b.parameterAt(result.point), 0.0, 1.0);
            return result;
        }

        result.kind = IntersectionKind::Overlap;
        result.overlapT0 = lo;
        result.overlapT1 = hi;
        result.overlapLength = (hi - lo) * l1;
        result.t = lo;
        result.point = a.pointAt(lo);
        result.u = std::clamp(b.parameterAt(result.point), 0.0, 1.0);
        return result;
    }

    const Point delta = b.p0 - a.p0;
    double t = cross(delta, d2) / denom;
    double u = cross(delta, d1) / denom;

    const double tEps = epsilon / l1;
    const double uEps = epsilon / l2;
    if (t < -tEps || t > 1.0 + tEps || u < -uEps || u > 1.0 + uEps)
        return result; // intersection lies outside at least one segment

    t = std::clamp(t, 0.0, 1.0);
    u = std::clamp(u, 0.0, 1.0);

    result.kind = IntersectionKind::Point;
    result.t = t;
    result.u = u;
    result.point = a.pointAt(t);
    return result;
}

} // namespace artplus::geometry
