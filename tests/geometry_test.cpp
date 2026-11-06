// Qt-free tests for the ArtPlus geometry engine core (Milestone 24, stage 1).
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

    std::printf("\n%s\n", g_failures == 0 ? "ALL PASSED" : "FAILURES PRESENT");
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
