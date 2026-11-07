#include "artplus/geometry/Path.h"

#include "artplus/geometry/Bezier.h"

#include <QStringList>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace artplus::geometry {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kKappa = 0.5522847498307936; // circle approximation constant
}

Point Path::SubPath::currentPoint() const
{
    if (segments.isEmpty())
        return start;
    const Segment& s = segments.last();
    return s.type == Segment::Type::Line ? s.p1 : s.p3;
}

void Path::moveTo(const Point& p)
{
    SubPath sp;
    sp.start = p;
    m_subpaths.push_back(sp);
}

void Path::lineTo(const Point& p)
{
    if (m_subpaths.empty() || m_subpaths.back().closed)
        moveTo(m_subpaths.empty() ? Point{} : m_subpaths.back().currentPoint());
    Segment s;
    s.type = Segment::Type::Line;
    s.p1 = p;
    m_subpaths.back().segments.push_back(s);
}

void Path::cubicTo(const Point& c1, const Point& c2, const Point& end)
{
    if (m_subpaths.empty() || m_subpaths.back().closed)
        moveTo(m_subpaths.empty() ? Point{} : m_subpaths.back().currentPoint());
    Segment s;
    s.type = Segment::Type::Cubic;
    s.p1 = c1;
    s.p2 = c2;
    s.p3 = end;
    m_subpaths.back().segments.push_back(s);
}

void Path::close()
{
    if (!m_subpaths.empty())
        m_subpaths.back().closed = true;
}

void Path::addRectangle(const Rect& r)
{
    const Rect n = r.normalized();
    moveTo({n.left, n.top});
    lineTo({n.right, n.top});
    lineTo({n.right, n.bottom});
    lineTo({n.left, n.bottom});
    close();
}

void Path::addCircle(const Point& center, double radius)
{
    addEllipse(Rect::fromXYWH(center.x - radius, center.y - radius, radius * 2, radius * 2));
}

void Path::addEllipse(const Rect& box)
{
    const Rect r = box.normalized();
    const double rx = r.width() / 2.0;
    const double ry = r.height() / 2.0;
    const Point c = r.center();
    const double kx = rx * kKappa;
    const double ky = ry * kKappa;

    moveTo({c.x + rx, c.y});
    cubicTo({c.x + rx, c.y + ky}, {c.x + kx, c.y + ry}, {c.x, c.y + ry});
    cubicTo({c.x - kx, c.y + ry}, {c.x - rx, c.y + ky}, {c.x - rx, c.y});
    cubicTo({c.x - rx, c.y - ky}, {c.x - kx, c.y - ry}, {c.x, c.y - ry});
    cubicTo({c.x + kx, c.y - ry}, {c.x + rx, c.y - ky}, {c.x + rx, c.y});
    close();
}

void Path::addRoundedRectangle(const Rect& r, double radius)
{
    const Rect n = r.normalized();
    const double rr = std::clamp(radius, 0.0, std::min(n.width(), n.height()) / 2.0);
    if (rr <= 0.0) {
        addRectangle(n);
        return;
    }
    const double k = rr * kKappa;
    moveTo({n.left + rr, n.top});
    lineTo({n.right - rr, n.top});
    cubicTo({n.right - rr + k, n.top}, {n.right, n.top + rr - k}, {n.right, n.top + rr});
    lineTo({n.right, n.bottom - rr});
    cubicTo({n.right, n.bottom - rr + k}, {n.right - rr + k, n.bottom}, {n.right - rr, n.bottom});
    lineTo({n.left + rr, n.bottom});
    cubicTo({n.left + rr - k, n.bottom}, {n.left, n.bottom - rr + k}, {n.left, n.bottom - rr});
    lineTo({n.left, n.top + rr});
    cubicTo({n.left, n.top + rr - k}, {n.left + rr - k, n.top}, {n.left + rr, n.top});
    close();
}

void Path::clear()
{
    m_subpaths.clear();
}

bool Path::isEmpty() const
{
    for (const auto& sp : m_subpaths) {
        if (!sp.segments.isEmpty())
            return false;
    }
    return true;
}

Rect Path::bounds() const
{
    Rect result;
    bool first = true;
    auto includePoint = [&](const Point& p) {
        if (first) {
            result = Rect(p.x, p.y, p.x, p.y);
            first = false;
        } else {
            result.expandToInclude(p);
        }
    };
    auto includeRect = [&](const Rect& r) {
        if (first) {
            result = r;
            first = false;
        } else {
            result = result.united(r);
        }
    };

    for (const auto& sp : m_subpaths) {
        includePoint(sp.start);
        Point current = sp.start;
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                includePoint(seg.p1);
                current = seg.p1;
            } else {
                // Exact cubic extrema come from the shared geometry core so the
                // solver exists in exactly one place.
                includeRect(CubicBezier{current, seg.p1, seg.p2, seg.p3}.bounds());
                current = seg.p3;
            }
        }
    }
    return result;
}

double Path::area() const
{
    double total = 0.0;
    for (const auto& poly : flatten()) {
        total += signedArea(poly);
    }
    return total;
}

Path Path::transformed(const Matrix& m) const
{
    Path out = *this;
    out.transform(m);
    return out;
}

void Path::transform(const Matrix& m)
{
    for (auto& sp : m_subpaths) {
        sp.start = m.map(sp.start);
        for (auto& seg : sp.segments) {
            seg.p1 = m.map(seg.p1);
            if (seg.type == Segment::Type::Cubic) {
                seg.p2 = m.map(seg.p2);
                seg.p3 = m.map(seg.p3);
            } else {
                seg.p3 = seg.p1;
            }
        }
    }
}

void flattenCubic(const Point& p0, const Point& c1, const Point& c2, const Point& p3,
                  double tolerance, QVector<Point>& out)
{
    // The subdivision algorithm lives in the Qt-free core; this overload only
    // adapts the result to QVector so existing callers keep working.
    std::vector<Point> points;
    flattenCubic(p0, c1, c2, p3, tolerance, points);
    out.reserve(out.size() + static_cast<int>(points.size()));
    for (const Point& p : points)
        out.push_back(p);
}

QVector<QVector<Point>> Path::flatten(double tolerance) const
{
    QVector<QVector<Point>> result;
    for (const auto& sp : m_subpaths) {
        if (sp.segments.isEmpty())
            continue;
        QVector<Point> poly;
        poly.push_back(sp.start);
        Point current = sp.start;
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                poly.push_back(seg.p1);
                current = seg.p1;
            } else {
                flattenCubic(current, seg.p1, seg.p2, seg.p3, tolerance, poly);
                current = seg.p3;
            }
        }
        if (sp.closed) {
            if (!poly.isEmpty() && distance(poly.front(), poly.back()) < 1e-9)
                poly.pop_back();
        }
        if (poly.size() >= 2)
            result.push_back(poly);
    }
    return result;
}

QString Path::toSvgPath() const
{
    QStringList parts;
    for (const auto& sp : m_subpaths) {
        parts << QStringLiteral("M %1 %2").arg(sp.start.x, 0, 'f', 3).arg(sp.start.y, 0, 'f', 3);
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                parts << QStringLiteral("L %1 %2").arg(seg.p1.x, 0, 'f', 3).arg(seg.p1.y, 0, 'f', 3);
            } else {
                parts << QStringLiteral("C %1 %2 %3 %4 %5 %6")
                             .arg(seg.p1.x, 0, 'f', 3).arg(seg.p1.y, 0, 'f', 3)
                             .arg(seg.p2.x, 0, 'f', 3).arg(seg.p2.y, 0, 'f', 3)
                             .arg(seg.p3.x, 0, 'f', 3).arg(seg.p3.y, 0, 'f', 3);
            }
        }
        if (sp.closed)
            parts << QStringLiteral("Z");
    }
    return parts.join(QLatin1Char(' '));
}

Path Path::fromPolygon(const QVector<Point>& points, bool closed)
{
    Path p;
    if (points.isEmpty())
        return p;
    p.moveTo(points.front());
    for (int i = 1; i < points.size(); ++i)
        p.lineTo(points[i]);
    if (closed)
        p.close();
    return p;
}

Path Path::fromPolygons(const QVector<QVector<Point>>& polygons)
{
    Path p;
    for (const auto& poly : polygons) {
        if (poly.size() < 2)
            continue;
        p.moveTo(poly.front());
        for (int i = 1; i < poly.size(); ++i)
            p.lineTo(poly[i]);
        p.close();
    }
    return p;
}

double signedArea(const QVector<Point>& polygon)
{
    if (polygon.size() < 3)
        return 0.0;
    double sum = 0.0;
    for (int i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
        sum += (polygon[j].x * polygon[i].y) - (polygon[i].x * polygon[j].y);
    return sum / 2.0;
}

bool pointInPolygon(const Point& p, const QVector<Point>& polygon)
{
    bool inside = false;
    if (polygon.size() < 3)
        return false;
    for (int i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const Point& a = polygon[i];
        const Point& b = polygon[j];
        if (((a.y > p.y) != (b.y > p.y))
            && (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)) {
            inside = !inside;
        }
    }
    return inside;
}

Rect polygonBounds(const QVector<Point>& polygon)
{
    Rect r;
    bool first = true;
    for (const auto& p : polygon) {
        if (first) {
            r = Rect(p.x, p.y, p.x, p.y);
            first = false;
        } else {
            r.expandToInclude(p);
        }
    }
    return r;
}

double polygonPerimeter(const QVector<Point>& polygon)
{
    if (polygon.size() < 2)
        return 0.0;
    double sum = 0.0;
    for (int i = 0; i < polygon.size(); ++i)
        sum += distance(polygon[i], polygon[(i + 1) % polygon.size()]);
    return sum;
}

QVector<Point> cleanPolygon(const QVector<Point>& polygon, double tolerance)
{
    QVector<Point> out;
    out.reserve(polygon.size());
    for (const auto& p : polygon) {
        if (out.isEmpty() || distance(out.back(), p) > tolerance)
            out.push_back(p);
    }
    while (out.size() > 1 && distance(out.front(), out.back()) <= tolerance)
        out.pop_back();
    // Remove collinear points.
    bool changed = true;
    while (changed && out.size() > 3) {
        changed = false;
        for (int i = 0; i < out.size(); ++i) {
            const Point& a = out[(i + out.size() - 1) % out.size()];
            const Point& b = out[i];
            const Point& c = out[(i + 1) % out.size()];
            const Point ab = b - a;
            const Point bc = c - b;
            const double cross = ab.x * bc.y - ab.y * bc.x;
            const double len = ab.length() * bc.length();
            if (len > 1e-12 && std::abs(cross) / len < 1e-9) {
                out.removeAt(i);
                changed = true;
                break;
            }
        }
    }
    return out;
}

QVector<Point> ensureOrientation(const QVector<Point>& polygon, bool counterClockwise)
{
    const double area = signedArea(polygon);
    if ((area < 0.0) == counterClockwise) {
        QVector<Point> reversed = polygon;
        std::reverse(reversed.begin(), reversed.end());
        return reversed;
    }
    return polygon;
}

QVector<Point> offsetPolygon(const QVector<Point>& polygon, double delta,
                             double miterLimit, double tolerance)
{
    QVector<Point> poly = cleanPolygon(polygon, tolerance);
    if (poly.size() < 3 || std::abs(delta) < 1e-12)
        return poly;

    // Work on a CCW polygon so that the outward normal is (dy, -dx).
    // Positive delta then expands and negative delta shrinks.
    poly = ensureOrientation(poly, true);

    QVector<Point> out;
    out.reserve(poly.size());
    for (int i = 0; i < poly.size(); ++i) {
        const Point& prev = poly[(i + poly.size() - 1) % poly.size()];
        const Point& cur = poly[i];
        const Point& next = poly[(i + 1) % poly.size()];

        Point n1 = (cur - prev).normalized();
        n1 = {n1.y, -n1.x}; // outward for CCW
        Point n2 = (next - cur).normalized();
        n2 = {n2.y, -n2.x};

        Point bisector = n1 + n2;
        const double bisLen = bisector.length();
        if (bisLen < 1e-9) {
            // 180-degree turn: offset straight out along n1.
            out.push_back(cur + n1 * delta);
            continue;
        }
        bisector = bisector / bisLen;
        double cosHalf = bisector.x * n1.x + bisector.y * n1.y;
        cosHalf = std::max(cosHalf, 1.0 / std::max(miterLimit, 1.0));
        const double miterLength = delta / cosHalf;
        out.push_back(cur + bisector * miterLength);
    }
    return cleanPolygon(out, tolerance);
}

double sharedBoundaryLength(const QVector<Point>& a, const QVector<Point>& b, double tolerance)
{
    double total = 0.0;
    if (a.size() < 2 || b.size() < 2)
        return 0.0;
    for (int i = 0; i < a.size(); ++i) {
        const Point a0 = a[i];
        const Point a1 = a[(i + 1) % a.size()];
        const Point da = a1 - a0;
        const double la = da.length();
        if (la < tolerance)
            continue;
        const Point ua = da / la;
        for (int j = 0; j < b.size(); ++j) {
            const Point b0 = b[j];
            const Point b1 = b[(j + 1) % b.size()];
            const Point db = b1 - b0;
            const double lb = db.length();
            if (lb < tolerance)
                continue;
            const Point ub = db / lb;
            const double cross = std::abs(ua.x * ub.y - ua.y * ub.x);
            if (cross > 1e-3)
                continue;
            // Check b0 distance from the infinite line a.
            const Point n{-ua.y, ua.x};
            const double distB0 = std::abs((b0 - a0).x * n.x + (b0 - a0).y * n.y);
            const double distB1 = std::abs((b1 - a0).x * n.x + (b1 - a0).y * n.y);
            if (distB0 > tolerance || distB1 > tolerance)
                continue;
            const double t0 = (b0 - a0).x * ua.x + (b0 - a0).y * ua.y;
            const double t1 = (b1 - a0).x * ua.x + (b1 - a0).y * ua.y;
            const double lo = std::max(0.0, std::min(t0, t1));
            const double hi = std::min(la, std::max(t0, t1));
            if (hi > lo)
                total += hi - lo;
        }
    }
    return total;
}

double polygonDistance(const QVector<Point>& a, const QVector<Point>& b)
{
    if (a.size() < 3 || b.size() < 3)
        return std::numeric_limits<double>::max();
    if (pointInPolygon(a.front(), b) || pointInPolygon(b.front(), a))
        return 0.0;

    auto pointSegmentDistance = [](const Point& p, const Point& s0, const Point& s1) {
        const Point d = s1 - s0;
        const double lenSq = d.lengthSquared();
        if (lenSq < 1e-18)
            return distance(p, s0);
        double t = ((p - s0).x * d.x + (p - s0).y * d.y) / lenSq;
        t = std::clamp(t, 0.0, 1.0);
        return distance(p, s0 + d * t);
    };

    double best = std::numeric_limits<double>::max();
    for (const auto& p : a) {
        for (int j = 0; j < b.size(); ++j)
            best = std::min(best, pointSegmentDistance(p, b[j], b[(j + 1) % b.size()]));
    }
    for (const auto& p : b) {
        for (int j = 0; j < a.size(); ++j)
            best = std::min(best, pointSegmentDistance(p, a[j], a[(j + 1) % a.size()]));
    }
    return best;
}

QVector<Point> simplifyPolygon(const QVector<Point>& polygon, double tolerance)
{
    return cleanPolygon(polygon, tolerance);
}

Path makeCirclePath(const Point& center, double radius)
{
    Path p;
    p.addCircle(center, radius);
    return p;
}

// ========================================================================
// Topological subdivision for Path — foundation for boolean operations
// ========================================================================

// Compute the intersection of two line segments.
 // Returns true if the segments intersect, and if so, sets the intersection point
 // and the parameters t1 (on segment 1) and t2 (on segment 2).
static bool lineSegmentIntersection(const Point& p1, const Point& p2,
                                    const Point& q1, const Point& q2,
                                    Point& result,
                                    double& t1, double& t2,
                                    double epsilon = 1e-12)
{
    // Line 1: p1 + t1 * (p2 - p1)
    // Line 2: q1 + t2 * (q2 - q1)
    Point d1 = p2 - p1;
    Point d2 = q2 - q1;

    double cross = d1.x * d2.y - d1.y * d2.x;
    if (std::abs(cross) < epsilon) {
        // Parallel or collinear
        return false;
    }

    double t1_num = (q1 - p1).x * d2.y - (q1 - p1).y * d2.x;
    double t2_num = (q1 - p1).x * d1.y - (q1 - p1).y * d1.x;
    t1 = t1_num / cross;
    t2 = t2_num / cross;

    if (t1 >= -epsilon && t1 <= 1.0 + epsilon && t2 >= -epsilon && t2 <= 1.0 + epsilon) {
        result = p1 + d1 * t1;
        return true;
    }
    return false;
}

// Find all intersections between all segments of all subpaths in a Path.
QVector<Intersection> artplus::geometry::buildTopology(Path& path)
{
    QVector<Intersection> intersections;

    // Collect all segments with their subpath and segment indices
    struct SegmentRef {
        int subpathIdx;
        int segIdx;
        Point p1, p2; // endpoints (for line) or p1/p3 for cubic (using de Casteljau at t=0 and t=1)
        bool isCubic;
    };

    std::vector<SegmentRef> allSegments;
    allSegments.reserve(path.subPathCount() * 10); // estimate

    for (int si = 0; si < path.subPathCount(); ++si) {
        const auto& subpath = path.subPaths()[si];
        for (int si2 = 0; si2 < (int)subpath.segments.size(); ++si2) {
            const auto& seg = subpath.segments[si2];
            SegmentRef ref;
            ref.subpathIdx = si;
            ref.segIdx = si2;
            ref.isCubic = (seg.type == Path::Segment::Type::Cubic);
            if (ref.isCubic) {
                // For cubic segments, use the endpoints p1 (t=0) and p3 (t=1)
                // The cubic is: P(t) = (1-t)^3*P0 + 3(1-t)^2*t*C1 + 3(1-t)*t^2*C2 + t^3*P3
                // At t=0: P0 = current point (subpath.start transformed)
                // At t=1: P3 = end point (seg.p3)
                // For intersection testing, we'll treat cubic segments as polylines
                // or use the analytical method later. For now, use endpoints.
                ref.p1 = subpath.start;  // will be set properly below
                ref.p2 = Point{0, 0};    // placeholder
                ref.p3 = seg.p3;
            } else {
                ref.p1 = seg.p1;
                ref.p2 = Point{0, 0};
                ref.isCubic = false;
            }
            allSegments.push_back(ref);
        }
    }

    // Actually, let's redo this more carefully. We need the actual segment endpoints.
    // Let's iterate over subpaths and segments directly.

    intersections.clear();

    for (int i = 0; i < path.subPathCount(); ++i) {
        const auto& subpathI = path.subPaths()[i];
        for (int j = i + 1; j < path.subPathCount(); ++j) {
            const auto& subpathJ = path.subPaths()[j];

            // Test all segment pairs between subpath i and subpath j
            for (int si = 0; si < (int)subpathI.segments.size(); ++si) {
                const auto& segI = subpathI.segments[si];

                for (int sj = 0; sj < (int)subpathJ.segments.size(); ++sj) {
                    const auto& segJ = subpathJ.segments[sj];

                    Point intersectPt;
                    double t1 = 0.0, t2 = 0.0;

                    // Determine types and compute intersection
                    bool haveIntersection = false;

                    if (!segI.isCubic && !segJ.isCubic) {
                        // Line × Line
                        haveIntersection = lineSegmentIntersection(
                            segI.p1, segI.p3,  // Line endpoints
                            segJ.p1, segJ.p3,
                            intersectPt, t1, t2);
                    } else if (!segI.isCubic && segJ.isCubic) {
                        // Line × Cubic: flatten the cubic and test line × line segments
                        // For now, use a coarse approximation: treat cubic as 4 line segments
                        // via de Casteljau at t=0, 1/3, 2/3, 1
                        // Actually, let's just use the analytical intersectCubicBezierLine
                        // but we need a Line, not a segment pair approach.
                        // For simplicity, skip cubic-cubic and line-cubic for this first pass.
                        continue;
                    } else if (segI.isCubic && !segJ.isCubic) {
                        // Cubic × Line: same as above, skip for now
                        continue;
                    } else {
                        // Cubic × Cubic: skip for now
                        continue;
                    }

                    if (haveIntersection) {
                        Intersection ix;
                        ix.point = intersectPt;
                        // Compute t1 for segment i: parameter along segI
                        // For line segment from p1 to p3: t1 = parameter
                        ix.t1 = t1;
                        ix.t2 = t2;
                        ix.source1 = i;
                        ix.source2 = j;
                        intersections.push_back(ix);
                    }
                }
            }
        }
    }

    // Sort intersections by position (optional, for deterministic order)
    // std::sort(intersections.begin(), intersections.end(), [](const Intersection& a, const Intersection& b) {
    //     return a.point.x < b.point.x || (a.point.x == b.point.x && a.point.y < b.point.y);
    // });

    return intersections;
}

// Divide um subpath específico em seus pontos de interseção com outros subpaths.
QVector<Intersection> artplus::geometry::subdivideSubPath(Path::SubPath& subpath, const Path& path)
{
    QVector<Intersection> result;

    // Find all intersections on this subpath with all other subpaths
    for (int otherIdx = 0; otherIdx < path.subPathCount(); ++otherIdx) {
        if (&path.subPaths()[otherIdx] == &subpath)
            continue;

        auto otherIntersections = buildTopologyInternal(path, subpath, otherIdx);
        result.append(otherIntersections);
    }

    // Sort intersections by parameter t1
    std::sort(result.begin(), result.end(), [](const Intersection& a, const Intersection& b) {
        return a.t1 < b.t1;
    });

    return result;
}

} // namespace artplus::geometry

// ========================================================================
// Topological graph for boolean operations
// ========================================================================

// A node in the topological graph. Either an intersection point, or a segment
// endpoint (start/end of a subpath).
struct TopoNode {
    Point point;          // coordinates
    int id = -1;          // unique node identifier
    bool isIntersection = false;  // true if this node is an intersection
    int sourceSubpath = -1;  // which subpath this node belongs to
    int sourceSegment = -1;  // which segment within the subpath
    double t = 0.0;       // parameter along the source segment (0 = start, 1 = end)
};

// An edge in the topological graph, connecting two nodes.
struct TopoEdge {
    int from;           // node index from
    int to;             // node index to
    int sourceSubpath = -1;  // which subpath this edge belongs to
    int sourceSegment = -1;  // which source segment
    double tFrom = 0.0;   // parameter at from node (0 = start, 1 = end)
    double tTo = 1.0;     // parameter at to node (0 = start, 1 = end)
};

// The complete topological graph resulting from subdivision.
struct TopoGraph {
    QVector<TopoNode> nodes;     // all nodes (intersections + endpoints)
    QVector<TopoEdge> edges;     // all edges connecting nodes
    QVector<QVector<int>> faces; // face outlines (node indices in order)
};

// Build the complete topological graph from a Path.
// This subdivides all segments at their mutual intersection points
// and constructs a graph suitable for boolean operations (Union, Difference,
// Intersection, XOR).
TopoGraph artplus::geometry::buildTopoGraph(Path& path)
{
    TopoGraph graph;

    // Step 1: Find all intersection points between all segment pairs
    QVector<Intersection> allIntersections = buildTopology(path);

    // Step 2: Collect all segment endpoints (start/end of each subpath)
    // and add them as nodes if they're not already at the same position as an intersection.

    // Map from Point -> node index (for snapping intersections to existing nodes)
    // We'll use a simple approach: add all endpoints first, then add intersections.

    // Collect all segment endpoints
    struct EndpointRef {
        Point point;
        int subpathIdx;
        int segIdx;
        bool isStart; // true = start point, false = end point
    };

    QVector<EndpointRef> endpoints;
    endpoints.reserve(path.subPathCount() * 2);

    for (int i = 0; i < path.subPathCount(); ++i) {
        const auto& subpath = path.subPaths()[i];
        if (!subpath.segments.isEmpty()) {
            // Start point (before first segment)
            endpoints.append({subpath.start, i, 0, true});
            // End point (after last segment)
            if (!subpath.closed)
                endpoints.append({subpath.segments.last().isCubic ? subpath.segments.last().p3 : subpath.segments.last().p1, i, segIdx, false});
        }
    }

    // Actually, let me redo this more carefully. The Path structure has subpaths,
    // each with segments. The start of the first segment is the subpath start,
    // and the end of the last segment (or the subpath start if closed) is the end.

    // For now, let's build the graph more directly from the intersections and the
    // existing segment structure.

    // Step 3: Build nodes from intersections + segment endpoints
    // We'll use a map to avoid duplicate nodes at the same position

    // Map: rounded point -> node index
    QMap<QPair<double, double>, int> pointToNode; // (x, y) -> node index

    auto addNode = [&](const Point& pt, bool isInt, int subpathIdx, int segIdx, double t) -> int {
        // Round to avoid floating point issues in the map
        double rx = std::round(pt.x * 1e8) / 1e8;
        double ry = std::round(pt.y * 1e8) / 1e8;
        QPair<double, double> key{rx, ry};

        if (pointToNode.contains(key)) {
            return pointToNode[key];
        }

        int newId = graph.nodes.size();
        graph.nodes.append({pt, newId, isInt, subpathIdx, segIdx, t});
        pointToNode[key] = newId;
        return newId;
    };

    // Add all intersection points as nodes
    for (const auto& ix : allIntersections) {
        addNode(ix.point, true, ix.source1, ix.segIdx, ix.t1);
        // Also add the second intersection point if it's different
        // (for line-segment intersections, t1 and t2 are on different segments)
    }

    // Add segment endpoints as nodes
    for (const auto& ref : endpoints) {
        // Check if this endpoint is close to an existing intersection
        bool isCloseToExisting = false;
        for (const auto& ix : allIntersections) {
            if (distance(ix.point, ref.point) < 1e-6) {
                isCloseToExisting = true;
                break;
            }
        }
        if (!isCloseToExisting) {
            // Determine parameter t: 0 for start, 1 for end
            double t = ref.isStart ? 0.0 : 1.0;
            addNode(ref.point, false, ref.subpathIdx, ref.segIdx, t);
        }
    }

    // Step 4: Build edges connecting nodes along each subpath segment
    // Between consecutive nodes (sorted by parameter t)

    // For each subpath, sort the nodes that belong to it by their parameter t,
    // and create edges between consecutive nodes.

    // First, assign each node to its source subpath and segment
    // (this information was set when the node was added)

    // For simplicity, let's create edges along each subpath's segments,
    // splitting at any intersection points that lie on them.

    // For each subpath:
    for (int i = 0; i < path.subPathCount(); ++i) {
        const auto& subpath = path.subPaths()[i];

        // Collect all nodes that belong to this subpath
        QVector<int> subpathNodes;
        for (int n = 0; n < graph.nodes.size(); ++n) {
            if (graph.nodes[n].sourceSubpath == i) {
                subpathNodes.append(n);
            }
        }

        // Sort nodes by their parameter t
        std::sort(subpathNodes.begin(), subpathNodes.end(), [&](int n1, int n2) {
            return graph.nodes[n1].t < graph.nodes[n2].t;
        });

        // Create edges between consecutive nodes
        for (int k = 0; k < (int)subpathNodes.size() - 1; ++k) {
            int from = subpathNodes[k];
            int to = subpathNodes[k + 1];

            // Skip zero-length edges (same position)
            if (distance(graph.nodes[from].point, graph.nodes[to].point) < 1e-12)
                continue;

            graph.edges.append({from, to, i, /* sourceSegment */ 0, /* tFrom */ graph.nodes[from].t, /* tTo */ graph.nodes[to].t});
        }
    }

    // Step 5: Identify faces using ray casting / edge traversal
    // This is the most complex part. For now, we'll mark the outer boundary
    // and leave hole detection for a later pass.

    // A simple approach: the "outer face" is the one with the largest area.
    // We can compute the signed area of each face outline and the one with
    // the largest absolute area is the outer boundary.

    // For now, we'll construct a basic face from the outer boundary edges.
    // A more complete implementation would use the Bentley-Gray algorithm
    // or half-edge data structure.

    // Compute faces using a half-edge traversal approach:
    // Build adjacency: for each node, find the next edge in counterclockwise order.

    // For each node, find outgoing edges
    QVector<QVector<int>> outgoing(graph.nodes.size());
    for (int e = 0; e < graph.edges.size(); ++e) {
        outgoing[graph.edges[e].from].append(e);
    }

    // For each node, sort outgoing edges by angle (to enable face traversal)
    for (int n = 0; n < (int)graph.nodes.size(); ++n) {
        std::sort(outgoing[n].begin(), outgoing[n].end(), [&](int e1, int e2) {
            const auto& ed1 = graph.edges[e1];
            const auto& ed2 = graph.edges[e2];
            // Compute angle of the edge direction at the node
            Point d1 = graph.nodes[ed1.to].point - graph.nodes[ed1].point;
            Point d2 = graph.nodes[ed2.to].point - graph.nodes[ed2].point;
            // atan2(y, x) comparison: quadrant 1 has larger angle than quadrant 4, etc.
            double angle1 = std::atan2(d1.y, d1.x);
            double angle2 = std::atan2(d2.y, d2.x);
            return angle1 < angle2;
        });
    }

    // Simple face traversal: start from the leftmost-bottommost node and
    // always take the "rightmost" turn (which gives the exterior face in CCW
    // ordering, or interior face in CW ordering).

    // For now, just mark that faces need to be computed properly.
    // We'll add a placeholder face from the convex hull of all nodes.

    if (graph.nodes.size() >= 3) {
        // Compute convex hull as a placeholder outer face
        // (In a full implementation, this would use proper face traversal)
        QVector<int> outerFace;
        // Sort nodes by angle from centroid
        Point centroid{0, 0};
        for (const auto& node : graph.nodes)
            centroid = centroid + node.point;
        centroid = centroid / graph.nodes.size();

        QVector<Point> angles;
        angles.reserve(graph.nodes.size());
        for (const auto& node : graph.nodes) {
            double angle = std::atan2(node.point.y - centroid.y, node.point.x - centroid.x);
            angles.append(angle);
        }

        // Sort node indices by angle
        QVector<int> indices(graph.nodes.size());
        for (int i = 0; i < (int)graph.nodes.size(); ++i)
            indices[i] = i;
        std::sort(indices.begin(), indices.end(), [&](int i1, int i2) {
            return angles[i1] < angles[i2];
        });

        for (int idx : indices)
            outerFace.append(idx);

        if (outerFace.size() >= 3)
            graph.faces.append(outerFace);
    }

    return graph;
}

// Convenience: distance between two points
double distance(const Point& a, const Point& b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Convenience: point-segment distance
double pointSegmentDistance(const Point& p, const Point& s0, const Point& s1)
{
    const Point d = s1 - s0;
    const double lenSq = d.lengthSquared();
    if (lenSq < 1e-18)
        return distance(p, s0);
    double t = ((p - s0).x * d.x + (p - s0).y * d.y) / lenSq;
    t = std::clamp(t, 0.0, 1.0);
    return distance(p, s0 + d * t);
}

// Alias for the global function
double artplus::geometry::pointSegmentDistance(const Point& p, const Point& s0, const Point& s1)
{
    return ::pointSegmentDistance(p, s0, s1);
}

// ========================================================================
// Implementação das funções de conversão de EditablePathSegment <-> Path::Segment
// ========================================================================

// Converte um EditablePathSegment para Path::Segment
inline Path::Segment::Type editableSegmentTypeToPathType(EditablePathSegment::Type type)
{
    switch (type) {
    case EditablePathSegment::Type::MoveTo: return Path::Segment::Type::Line;
    case EditablePathSegment::Type::LineTo: return Path::Segment::Type::Line;
    case EditablePathSegment::Type::BezierTo: return Path::Segment::Type::Cubic;
    default: return Path::Segment::Type::Line;
    }
}

// Converte QVector<EditablePathSegment> em QVector<Path::Segment>
// O parâmetro subpathStart é o ponto inicial do subpath (transformado para coordenadas locais)
QVector<Path::Segment> artplus::geometry::convertEditableSegments(const QVector<EditablePathSegment>& segments,
                                                                 const Point& subpathStart)
{
    QVector<Path::Segment> result;
    result.reserve(segments.size());

    for (const auto& seg : segments) {
        Path::Segment s;
        s.type = editableSegmentTypeToPathType(seg.type);

        // O segmento EditablePathSegment tem 'point' como ponto final.
        // Para LineTo/MoveTo, o ponto é o destino.
        // Para BezierTo, 'point' é o ponto final da curva, e precisamos dos controles.
        // Neste nível de conversão básica, usamos o ponto fornecido como p1 (end point)
        // e definimos p3 como o mesmo para cubicas, ou calculamos baseado no início.

        if (s.type == Path::Segment::Type::Cubic) {
            // Para cubicas, usamos o point como p3 (end), e definimos p1/p2 como placeholders
            // A conversão completa exigiria os controles, que o EditableCurve fornece.
            s.p1 = Point{0, 0};     // control 1 placeholder
            s.p2 = Point{0, 0};     // control 2 placeholder
            s.p3 = Point{seg.point.x, seg.point.y}; // end point from editable
        } else {
            // Para linhas, o point é o destino (p1)
            s.p1 = Point{seg.point.x, seg.point.y}; // end point
            s.p2 = Point{0, 0};     // control 1 placeholder (não usado para linha)
            s.p3 = Point{0, 0};     // control 2 placeholder (não usado para linha)
        }

        // Ajusta o início: se este for o primeiro segmento e o subpath tenha um start,
        // o segmento deve começar no subpathStart. Ajuste de posição será feito pelo chamador.
        result.append(s);
    }

    return result;
}

// Converte QVector<EditableCurve> em QVector<Path::Segment>
// Cada EditableCurve tem start, control1, control2, end em coordenadas page-space.
// Precisamos transformar para o modelo Path::Segment.
QVector<Path::Segment> artplus::geometry::convertEditableCurvesToSegments(const QVector<EditableCurve>& curves,
                                                                         const Point& subpathStart)
{
    QVector<Path::Segment> result;
    result.reserve(curves.size());

    for (const auto& curve : curves) {
        Path::Segment s;
        s.type = Path::Segment::Type::Cubic;

        // O EditableCurve tem start, control1, control2, end em QPointF (page coordinates).
        // Precisamos converter para Point (geometry coordinates) e ajustar o início.
        s.p1 = Point{curve.control1.x, curve.control1.y}; // control 1
        s.p2 = Point{curve.control2.x, curve.control2.y}; // control 2
        s.p3 = Point{curve.end.x, curve.end.y};           // end point

        // Ajustar início: o start do segmento deve ser o 'start' do subpath
        // ou o point do segmento anterior. Para esta conversão básica, registramos
        // os valores e o ajuste de posição é feito pelo chamador.

        result.append(s);
    }

    return result;
}

// ========================================================================
// Conversão inversa: Path::Segment para EditablePathSegment
// ========================================================================

// Converte um Path::Segment para EditablePathSegment
EditablePathSegment::Type artplus::geometry::pathSegmentTypeToEditableType(Path::Segment::Type type)
{
    switch (type) {
    case Path::Segment::Type::Line: return EditablePathSegment::Type::LineTo;
    case Path::Segment::Type::Cubic: return EditablePathSegment::Type::BezierTo;
    default: return EditablePathSegment::Type::Unknown;
    }
}

// Converte QVector<Path::Segment> em QVector<EditablePathSegment>
// O parâmetro subpathStart é o ponto inicial do subpath (para posicionamento correto)
QVector<EditablePathSegment> artplus::geometry::convertPathSegmentsToEditable(const QVector<Path::Segment>& segments,
                                                                             const Point& subpathStart)
{
    QVector<EditablePathSegment> result;
    result.reserve(segments.size());

    for (const auto& seg : segments) {
        EditablePathSegment s;
        s.type = pathSegmentTypeToEditableType(seg.type);

        if (s.type == EditablePathSegment::Type::BezierTo) {
            // Para cubicas, populamos o point (end) e os controles
            s.point = Point{seg.p3.x, seg.p3.y}; // end point
            s.closes = false;
            // control1 e control2 já estão em seg.p1 e seg.p2
            // Nota: a ordem pode precisar de ajuste baseado na convenção PDF vs geometry
            // PDF: cubicTo(c1, c2, end) onde c1 e c2 são control points
            // Geometry: o segment tem p1=p1, p2=c1, p3=c2, p3=end (já definido acima)
            s.point = Point{seg.p3.x, seg.p3.y};
        } else {
            // Para linhas, o point é o destino
            s.point = Point{seg.p1.x, seg.p1.y}; // end point (p1 do segment)
            s.closes = false;
        }

        result.append(s);
    }

    return result;
}

// Converte um EditableCurve para Path::Segment
Path::Segment artplus::geometry::convertEditableCurveToSegment(const EditableCurve& curve)
{
    Path::Segment s;
    s.type = Path::Segment::Type::Cubic;

    // O EditableCurve tem: start, control1, control2, end em QPointF
    // Convertendo para Point (geometry coordinates)
    s.p1 = Point{curve.control1.x, curve.control1.y}; // control 1
    s.p2 = Point{curve.control2.x, curve.control2.y}; // control 2
    s.p3 = Point{curve.end.x, curve.end.y};           // end point

    // O start do segmento é implicitamente o 'start' do subpath onde esta curva está
    // Para esta função standalone, registramos os controles e o end point.

    return s;
}

} // namespace artplus::geometry
