#pragma once

#include "artplus/geometry/Point.h"

#include <QString>
#include <QVector>
#include <vector>

namespace artplus::geometry {

class Path
{
public:
    struct Segment {
        enum class Type { Line, Cubic };
        Type type = Type::Line;
        Point p1; // line end / cubic control 1
        Point p2; // cubic control 2
        Point p3; // cubic end
    };

    struct SubPath {
        Point start;
        QVector<Segment> segments;
        bool closed = false;

        Point currentPoint() const;
        bool isEmpty() const { return segments.isEmpty(); }
    };

    Path() = default;

    void moveTo(const Point& p);
    void moveTo(double x, double y) { moveTo({x, y}); }
    void lineTo(const Point& p);
    void lineTo(double x, double y) { lineTo({x, y}); }
    void cubicTo(const Point& c1, const Point& c2, const Point& end);
    void cubicTo(double c1x, double c1y, double c2x, double c2y, double ex, double ey)
    {
        cubicTo({c1x, c1y}, {c2x, c2y}, {ex, ey});
    }
    void close();
    void addRectangle(const Rect& r);
    void addRoundedRectangle(const Rect& r, double radius);
    void addCircle(const Point& center, double radius);
    void addEllipse(const Rect& box);

    void clear();
    bool isEmpty() const;
    int subPathCount() const { return static_cast<int>(m_subpaths.size()); }
    const std::vector<SubPath>& subPaths() const { return m_subpaths; }
    std::vector<SubPath>& subPaths() { return m_subpaths; }

    Rect bounds() const;
    double area() const; // signed area of flattened outlines

    Path transformed(const Matrix& m) const;
    void transform(const Matrix& m);

    // Flattens every subpath. Closed subpaths are returned without a duplicate
    // closing point (the edge from last to first is implicit).
    QVector<QVector<Point>> flatten(double tolerance = 0.2) const;

    QString toSvgPath() const;
    static Path fromPolygon(const QVector<Point>& points, bool closed = true);
    static Path fromPolygons(const QVector<QVector<Point>>& polygons);

private:
    std::vector<SubPath> m_subpaths;
};

// Splits a cubic bezier into a polyline. Public for geometry tests and offsetting.
void flattenCubic(const Point& p0, const Point& c1, const Point& c2, const Point& p3,
                  double tolerance, QVector<Point>& out);

double signedArea(const QVector<Point>& polygon);
bool pointInPolygon(const Point& p, const QVector<Point>& polygon);
Rect polygonBounds(const QVector<Point>& polygon);
double polygonPerimeter(const QVector<Point>& polygon);

// Offsets a simple closed polygon. Positive delta expands, negative shrinks.
// Uses miter joins with a miter limit. Concave self-intersection cleanup is not
// performed yet; callers should validate the result before destructive use.
QVector<Point> offsetPolygon(const QVector<Point>& polygon, double delta,
                             double miterLimit = 4.0, double tolerance = 1e-6);

// Returns how much boundary length two polygons share (collinear overlapping edges).
double sharedBoundaryLength(const QVector<Point>& a, const QVector<Point>& b,
                            double tolerance = 0.05);

// Distance between two polygons (0 when they touch or overlap).
double polygonDistance(const QVector<Point>& a, const QVector<Point>& b);

QVector<Point> ensureOrientation(const QVector<Point>& polygon, bool counterClockwise);
QVector<Point> simplifyPolygon(const QVector<Point>& polygon, double tolerance = 1e-6);

// Removes collinear spikes and duplicate points.
QVector<Point> cleanPolygon(const QVector<Point>& polygon, double tolerance = 1e-6);

// Circle approximated with four cubic segments.
Path makeCirclePath(const Point& center, double radius);

} // namespace artplus::geometry
