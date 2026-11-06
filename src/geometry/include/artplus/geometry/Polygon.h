#pragma once

// Polygon primitives: area, orientation, centroid, perimeter, bounds and
// point classification (inside / outside / exactly on the boundary).
// Qt-free core: vertices live in std::vector.

#include "artplus/geometry/Line.h"
#include "artplus/geometry/Point.h"

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace artplus::geometry {

// Sign convention: signedArea() is the shoelace sum
//   A = 1/2 * SUM(x_i * y_{i+1} - x_{i+1} * y_i)
// In a y-up frame a positive A is counter-clockwise. ArtPlus works in PDF page
// space, which is y-down, so the same positive A appears clockwise on screen.
// The enum below names the *mathematical* (y-up) orientation; callers that care
// about screen appearance must invert it. This is documented in the README.
enum class Orientation { Clockwise, CounterClockwise, Degenerate };

enum class PointLocation { Outside, Inside, Boundary };

class Polygon {
public:
    Polygon() = default;
    explicit Polygon(std::vector<Point> vertices);
    Polygon(std::initializer_list<Point> vertices);

    void add(const Point& p);
    void reserve(std::size_t n) { m_vertices.reserve(n); }
    std::size_t size() const { return m_vertices.size(); }
    bool empty() const { return m_vertices.empty(); }
    void clear() { m_vertices.clear(); }

    const std::vector<Point>& vertices() const { return m_vertices; }
    std::vector<Point>& vertices() { return m_vertices; }

    // Signed shoelace area. The closing edge (last -> first) is implicit.
    double signedArea() const;
    double area() const;
    // `epsilon` is a *relative* factor, not a distance: the outline is reported
    // as Degenerate when its enclosed area is negligible compared with its own
    // bounding-box area. This keeps the answer independent of coordinate
    // magnitude (a 1e-6 pt and a 1e6 pt square are both counter-clockwise),
    // which an absolute area threshold could not do.
    Orientation orientation(double epsilon = kGeometryEpsilon) const;
    // Area centroid for non-degenerate polygons; vertex mean otherwise.
    // Returns the origin for an empty polygon.
    Point centroid() const;
    double perimeter() const;
    // Empty rect when the polygon has no vertices.
    Rect bounds() const;

    // Boundary membership takes precedence over the ray-casting test, so a point
    // exactly on an edge or vertex reports Boundary rather than Inside/Outside.
    // Here `epsilon` *is* an absolute distance tolerance in points; callers that
    // work at an unusual scale should pass a matching value.
    PointLocation classify(const Point& p, double epsilon = kGeometryEpsilon) const;
    // True for Inside and Boundary. Use classify() when the distinction matters.
    bool contains(const Point& p, double epsilon = kGeometryEpsilon) const;

    void reverse();
    // Reverses the winding when needed. Degenerate polygons are left untouched.
    void ensureOrientation(Orientation target, double epsilon = kGeometryEpsilon);
    // Drops consecutive duplicates and a duplicated closing vertex.
    void removeDuplicateVertices(double epsilon = kGeometryEpsilon);

private:
    std::vector<Point> m_vertices;
};

// Free-function equivalents for callers that already hold a vertex vector.
double polygonSignedArea(const std::vector<Point>& polygon);
Point polygonCentroid(const std::vector<Point>& polygon);
Orientation polygonOrientation(const std::vector<Point>& polygon,
                               double epsilon = kGeometryEpsilon);
PointLocation classifyPointInPolygon(const Point& p, const std::vector<Point>& polygon,
                                     double epsilon = kGeometryEpsilon);
Rect polygonBoundsOf(const std::vector<Point>& polygon);

} // namespace artplus::geometry
