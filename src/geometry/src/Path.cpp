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
    if (!m_subpaths.empty() && !m_subpaths.back().closed && !m_subpaths.back().segments.isEmpty()) {
        m_subpaths.back().closed = true;
    }
}

void Path::addRectangle(const Rect& r)
{
    moveTo(r.topLeft());
    lineTo(r.topRight());
    lineTo(r.bottomRight());
    lineTo(r.bottomLeft());
    close();
}

void Path::addRoundedRectangle(const Rect& r, double radius)
{
    double w = r.width();
    double h = r.height();
    double rAdjusted = std::min(radius, std::min(w, h) / 2.0);
    if (rAdjusted <= 0.0) {
        addRectangle(r);
        return;
    }
    Point tl = r.topLeft();
    Point tr = r.topRight();
    Point br = r.bottomRight();
    Point bl = r.bottomLeft();

    moveTo({tl.x + rAdjusted, tl.y});
    lineTo({tr.x - rAdjusted, tr.y});
    cubicTo({tr.x - rAdjusted * (1.0 - kKappa), tr.y},
            {tr.x, tr.y + rAdjusted * (1.0 - kKappa)},
            {tr.x, tr.y + rAdjusted});
    lineTo({br.x, br.y - rAdjusted});
    cubicTo({br.x, br.y - rAdjusted * (1.0 - kKappa)},
            {br.x - rAdjusted * (1.0 - kKappa), br.y},
            {br.x - rAdjusted, br.y});
    lineTo({bl.x + rAdjusted, bl.y});
    cubicTo({bl.x + rAdjusted * (1.0 - kKappa), bl.y},
            {bl.x, bl.y - rAdjusted * (1.0 - kKappa)},
            {bl.x, bl.y - rAdjusted});
    lineTo({tl.x, tl.y + rAdjusted});
    cubicTo({tl.x, tl.y + rAdjusted * (1.0 - kKappa)},
            {tl.x + rAdjusted * (1.0 - kKappa), tl.y},
            {tl.x + rAdjusted, tl.y});
    close();
}

void Path::addCircle(const Point& center, double radius)
{
    double c = radius * kKappa;
    moveTo({center.x + radius, center.y});
    cubicTo({center.x + radius, center.y + c},
            {center.x + c, center.y + radius},
            {center.x, center.y + radius});
    cubicTo({center.x - c, center.y + radius},
            {center.x - radius, center.y + c},
            {center.x - radius, center.y});
    cubicTo({center.x - radius, center.y - c},
            {center.x - c, center.y - radius},
            {center.x, center.y - radius});
    cubicTo({center.x + c, center.y - radius},
            {center.x + radius, center.y - c},
            {center.x + radius, center.y});
    close();
}

void Path::addEllipse(const Rect& box)
{
    Point center = box.center();
    double rx = box.width() / 2.0;
    double ry = box.height() / 2.0;
    double cx = center.x;
    double cy = center.y;
    double c = kKappa; // 0.5522847498

    moveTo({cx + rx, cy});
    cubicTo({cx + rx, cy + ry * c},
            {cx + rx * c, cy + ry},
            {cx, cy + ry});
    cubicTo({cx - rx * c, cy + ry},
            {cx - rx, cy + ry * c},
            {cx - rx, cy});
    cubicTo({cx - rx, cy - ry * c},
            {cx - rx * c, cy - ry},
            {cx, cy - ry});
    cubicTo({cx + rx * c, cy - ry},
            {cx + rx, cy - ry * c},
            {cx + rx, cy});
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
    if (m_subpaths.empty())
        return Rect{};

    bool first = true;
    Rect out{};
    for (const auto& sp : m_subpaths) {
        Point current = sp.start;
        if (first) {
            out = Rect{current.x, current.y, current.x, current.y};
            first = false;
        } else {
            out.expandToInclude(current);
        }
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                out.expandToInclude(seg.p1);
            } else {
                out.expandToInclude(seg.p1);
                out.expandToInclude(seg.p2);
                out.expandToInclude(seg.p3);
            }
            current = (seg.type == Segment::Type::Line) ? seg.p1 : seg.p3;
        }
    }
    return out;
}

double Path::area() const
{
    double total = 0.0;
    for (const auto& sp : m_subpaths) {
        if (sp.segments.isEmpty())
            continue;
        Point current = sp.start;
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                total += (current.x * seg.p1.y - seg.p1.x * current.y) / 2.0;
                current = seg.p1;
            } else {
                // Approximate cubic area using the shoelace formula on flattened points
                QVector<Point> flat;
                flattenCubic(current, seg.p1, seg.p2, seg.p3, 0.2, flat);
                flat.insert(flat.begin(), current);
                for (int i = 0, j = flat.size() - 1; i < (int)flat.size(); j = i++) {
                    total += (flat[j].x * flat[i].y - flat[i].x * flat[j].y) / 2.0;
                }
                current = seg.p3;
            }
        }
    }
    return total;
}

Path Path::transformed(const Matrix& m) const
{
    Path result;
    for (const auto& sp : m_subpaths) {
        Point mappedStart = m.map(sp.start);
        result.moveTo(mappedStart);
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                result.lineTo(m.map(seg.p1));
            } else {
                result.cubicTo(m.map(seg.p1), m.map(seg.p2), m.map(seg.p3));
            }
        }
        if (sp.closed)
            result.close();
    }
    return result;
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
            }
        }
    }
}

QVector<QVector<Point>> Path::flatten(double tolerance) const
{
    QVector<QVector<Point>> all;
    for (const auto& sp : m_subpaths) {
        if (sp.segments.isEmpty())
            continue;
        QVector<Point> flat;
        Point current = sp.start;
        flat.append(current);
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                current = seg.p1;
                flat.append(current);
            } else {
                QVector<Point> segFlat;
                flattenCubic(current, seg.p1, seg.p2, seg.p3, tolerance, segFlat);
                for (const auto& p : segFlat)
                    flat.append(p);
                current = seg.p3;
            }
        }
        all.append(flat);
    }
    return all;
}

QString Path::toSvgPath() const
{
    QString svg;
    for (const auto& sp : m_subpaths) {
        if (sp.segments.isEmpty())
            continue;
        svg += QString("M %1 %2 ").arg(sp.start.x).arg(sp.start.y);
        for (const auto& seg : sp.segments) {
            if (seg.type == Segment::Type::Line) {
                svg += QString("L %1 %2 ").arg(seg.p1.x).arg(seg.p1.y);
            } else {
                svg += QString("C %1 %2 %3 %4 %5 %6 ")
                    .arg(seg.p1.x).arg(seg.p1.y)
                    .arg(seg.p2.x).arg(seg.p2.y)
                    .arg(seg.p3.x).arg(seg.p3.y);
            }
        }
        if (sp.closed)
            svg += "Z ";
    }
    return svg.trimmed();
}

Path Path::fromPolygon(const QVector<Point>& points, bool closed)
{
    Path path;
    if (points.isEmpty())
        return path;
    path.moveTo(points.first());
    for (int i = 1; i < points.size(); ++i)
        path.lineTo(points[i]);
    if (closed)
        path.close();
    return path;
}

Path Path::fromPolygons(const QVector<QVector<Point>>& polygons)
{
    Path path;
    for (const auto& poly : polygons)
        path = fromPolygon(poly, true);
    return path;
}

// Find all intersections between all segments of all subpaths in a Path.
// Minimal implementation: returns empty vector for now.
QVector<Intersection> artplus::geometry::buildTopology(Path& path)
{
    Q_UNUSED(path);
    return QVector<Intersection>();
}

// Divide um subpath específico em seus pontos de interseção com outros subpaths.
// Minimal implementation: returns empty vector for now.
QVector<Intersection> artplus::geometry::subdivideSubPath(Path::SubPath& subpath, const Path& path)
{
    Q_UNUSED(subpath);
    Q_UNUSED(path);
    return QVector<Intersection>();
}

} // namespace artplus::geometry