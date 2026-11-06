#include "artplus/geometry/Polygon.h"

#include <algorithm>
#include <cmath>

namespace artplus::geometry {

namespace {

double shoelace(const std::vector<Point>& polygon)
{
    if (polygon.size() < 3)
        return 0.0;
    double sum = 0.0;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
        sum += polygon[j].x * polygon[i].y - polygon[i].x * polygon[j].y;
    return sum / 2.0;
}

Point vertexMean(const std::vector<Point>& polygon)
{
    if (polygon.empty())
        return {};
    Point sum;
    for (const Point& p : polygon) {
        sum.x += p.x;
        sum.y += p.y;
    }
    const double n = static_cast<double>(polygon.size());
    return {sum.x / n, sum.y / n};
}

// A polygon encloses a meaningful area when its |signed area| is not negligible
// compared with its own bounding box. `relative` is dimensionless, so the test
// is independent of the coordinate magnitude.
bool hasNegligibleArea(double signedAreaValue, const Rect& boundingBox, double relative)
{
    const double bboxArea = std::abs(boundingBox.width() * boundingBox.height());
    if (!(bboxArea > 0.0))
        return true; // zero width or height: the outline encloses no area
    return std::abs(signedAreaValue) <= relative * bboxArea;
}

} // namespace

Polygon::Polygon(std::vector<Point> vertices) : m_vertices(std::move(vertices)) {}

Polygon::Polygon(std::initializer_list<Point> vertices) : m_vertices(vertices) {}

void Polygon::add(const Point& p)
{
    m_vertices.push_back(p);
}

double Polygon::signedArea() const
{
    return shoelace(m_vertices);
}

double Polygon::area() const
{
    return std::abs(signedArea());
}

Orientation Polygon::orientation(double epsilon) const
{
    const double a = signedArea();
    if (hasNegligibleArea(a, polygonBoundsOf(m_vertices), epsilon))
        return Orientation::Degenerate;
    return a > 0.0 ? Orientation::CounterClockwise : Orientation::Clockwise;
}

Point Polygon::centroid() const
{
    return polygonCentroid(m_vertices);
}

double Polygon::perimeter() const
{
    if (m_vertices.size() < 2)
        return 0.0;
    double sum = 0.0;
    const std::size_t n = m_vertices.size();
    for (std::size_t i = 0; i < n; ++i)
        sum += distance(m_vertices[i], m_vertices[(i + 1) % n]);
    return sum;
}

Rect Polygon::bounds() const
{
    return polygonBoundsOf(m_vertices);
}

PointLocation Polygon::classify(const Point& p, double epsilon) const
{
    return classifyPointInPolygon(p, m_vertices, epsilon);
}

bool Polygon::contains(const Point& p, double epsilon) const
{
    return classify(p, epsilon) != PointLocation::Outside;
}

void Polygon::reverse()
{
    std::reverse(m_vertices.begin(), m_vertices.end());
}

void Polygon::ensureOrientation(Orientation target, double epsilon)
{
    const Orientation current = orientation(epsilon);
    if (current == Orientation::Degenerate || current == target)
        return;
    reverse();
}

void Polygon::removeDuplicateVertices(double epsilon)
{
    std::vector<Point> out;
    out.reserve(m_vertices.size());
    for (const Point& p : m_vertices) {
        if (out.empty() || distance(out.back(), p) > epsilon)
            out.push_back(p);
    }
    while (out.size() > 1 && distance(out.front(), out.back()) <= epsilon)
        out.pop_back();
    m_vertices = std::move(out);
}

// --- Free functions --------------------------------------------------------

double polygonSignedArea(const std::vector<Point>& polygon)
{
    return shoelace(polygon);
}

Point polygonCentroid(const std::vector<Point>& polygon)
{
    if (polygon.empty())
        return {};
    if (polygon.size() < 3)
        return vertexMean(polygon);

    const double a = shoelace(polygon);
    // A degenerate outline has no area centroid; the vertex mean is the only
    // well-defined answer and is documented as the fallback.
    if (hasNegligibleArea(a, polygonBoundsOf(polygon), kGeometryEpsilon))
        return vertexMean(polygon);

    double cx = 0.0;
    double cy = 0.0;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const double cross = polygon[j].x * polygon[i].y - polygon[i].x * polygon[j].y;
        cx += (polygon[j].x + polygon[i].x) * cross;
        cy += (polygon[j].y + polygon[i].y) * cross;
    }
    const double factor = 1.0 / (6.0 * a);
    return {cx * factor, cy * factor};
}

Orientation polygonOrientation(const std::vector<Point>& polygon, double epsilon)
{
    const double a = shoelace(polygon);
    if (hasNegligibleArea(a, polygonBoundsOf(polygon), epsilon))
        return Orientation::Degenerate;
    return a > 0.0 ? Orientation::CounterClockwise : Orientation::Clockwise;
}

Rect polygonBoundsOf(const std::vector<Point>& polygon)
{
    if (polygon.empty())
        return {};
    Rect r(polygon.front().x, polygon.front().y, polygon.front().x, polygon.front().y);
    for (const Point& p : polygon)
        r.expandToInclude(p);
    return r;
}

PointLocation classifyPointInPolygon(const Point& p, const std::vector<Point>& polygon,
                                     double epsilon)
{
    if (polygon.empty())
        return PointLocation::Outside;

    if (polygon.size() == 1) {
        return distance(p, polygon.front()) <= epsilon ? PointLocation::Boundary
                                                       : PointLocation::Outside;
    }

    // Boundary membership is tested first so that a point exactly on an edge or
    // vertex never depends on the ray-casting tie-breaking rules.
    const std::size_t n = polygon.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        if (pointSegmentDistance(p, Line{polygon[j], polygon[i]}) <= epsilon)
            return PointLocation::Boundary;
    }

    if (n < 3)
        return PointLocation::Outside; // a non-degenerate polygon needs 3 vertices

    // Crossing-number test against the horizontal ray towards +x.
    bool inside = false;
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const Point& a = polygon[i];
        const Point& b = polygon[j];
        if ((a.y > p.y) != (b.y > p.y)) {
            const double xCross = a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (p.x < xCross)
                inside = !inside;
        }
    }
    return inside ? PointLocation::Inside : PointLocation::Outside;
}

} // namespace artplus::geometry
