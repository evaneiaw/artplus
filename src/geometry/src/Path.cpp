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

} // namespace artplus::geometry
