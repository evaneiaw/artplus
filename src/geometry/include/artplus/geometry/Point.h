#pragma once

// Qt-free geometric primitives.
//
// Conventions used by the whole artplus::geometry core:
//  * Units are PDF points (1 pt = 1/72 inch). No unit conversion happens here.
//  * Coordinates are an abstract 2D Cartesian frame. ArtPlus renders PDF page
//    space, which is y-down, so a rotation that is mathematically
//    counter-clockwise appears clockwise on screen. Every helper in this
//    library documents which convention it uses instead of assuming one.
//  * Tolerances are absolute distances in points unless a function says
//    otherwise. kGeometryEpsilon (1e-9 pt) is ~3.5e-10 mm: far below any
//    coordinate precision a PDF page can carry, so it only absorbs
//    floating-point noise and never hides geometric error.

namespace artplus::geometry {

// Absolute distance tolerance shared by the core primitives.
inline constexpr double kGeometryEpsilon = 1e-9;

struct Point {
    double x = 0.0;
    double y = 0.0;

    constexpr Point() = default;
    constexpr Point(double x_, double y_) : x(x_), y(y_) {}

    constexpr Point operator+(const Point& o) const { return {x + o.x, y + o.y}; }
    constexpr Point operator-(const Point& o) const { return {x - o.x, y - o.y}; }
    constexpr Point operator*(double s) const { return {x * s, y * s}; }
    constexpr Point operator/(double s) const { return {x / s, y / s}; }
    Point& operator+=(const Point& o) { x += o.x; y += o.y; return *this; }
    Point& operator-=(const Point& o) { x -= o.x; y -= o.y; return *this; }

    constexpr bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Point& o) const { return !(*this == o); }

    double length() const;
    double lengthSquared() const;
    Point normalized() const;
};

struct Rect {
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;

    constexpr Rect() = default;
    constexpr Rect(double l, double t, double r, double b) : left(l), top(t), right(r), bottom(b) {}
    static constexpr Rect fromXYWH(double x, double y, double w, double h) { return {x, y, x + w, y + h}; }

    constexpr double width() const { return right - left; }
    constexpr double height() const { return bottom - top; }
    // A rect is empty when it has no area in either axis. Note that the default
    // Rect {} is therefore empty *and* anchored at the origin; use united()/
    // expandToInclude() only after seeding with the first point.
    constexpr bool isEmpty() const { return right <= left || bottom <= top; }
    constexpr Point topLeft() const { return {left, top}; }
    constexpr Point topRight() const { return {right, top}; }
    constexpr Point bottomLeft() const { return {left, bottom}; }
    constexpr Point bottomRight() const { return {right, bottom}; }
    constexpr Point center() const { return {(left + right) / 2.0, (top + bottom) / 2.0}; }

    constexpr bool operator==(const Rect& o) const
    {
        return left == o.left && top == o.top && right == o.right && bottom == o.bottom;
    }
    constexpr bool operator!=(const Rect& o) const { return !(*this == o); }

    void expandToInclude(const Point& p);
    void expandBy(double d);
    Rect united(const Rect& o) const;
    Rect intersected(const Rect& o) const;
    bool contains(const Point& p) const;
    bool contains(const Rect& r) const;
    bool intersects(const Rect& r) const;
    Rect normalized() const;
};

// 2D affine transform in PDF convention:
//   x' = a*x + c*y + e
//   y' = b*x + d*y + f
// Composition order: (A * B) applied to a point means "B first, then A".
struct Matrix {
    double a = 1.0, b = 0.0, c = 0.0, d = 1.0, e = 0.0, f = 0.0;

    constexpr Matrix() = default;
    constexpr Matrix(double a_, double b_, double c_, double d_, double e_, double f_)
        : a(a_), b(b_), c(c_), d(d_), e(e_), f(f_) {}

    static Matrix identity() { return {}; }
    static Matrix translation(double tx, double ty) { return {1, 0, 0, 1, tx, ty}; }
    static Matrix scaling(double sx, double sy) { return {sx, 0, 0, sy, 0, 0}; }
    static Matrix rotationDegrees(double degrees);
    static Matrix rotationRadians(double radians);

    Point map(const Point& p) const;
    Rect mapRect(const Rect& r) const;

    // this * other: applies other first, then this.
    Matrix operator*(const Matrix& other) const;
    Matrix inverted(bool* ok = nullptr) const;
    double determinant() const { return a * d - b * c; }
    bool isIdentity() const;
};

// `AffineTransform` is the name used by the geometry engine documentation for
// exactly the transform above. It is an alias rather than a second type so that
// callers can pick the clearer name without duplicating the implementation.
using AffineTransform = Matrix;

double distance(const Point& a, const Point& b);
Point lerp(const Point& a, const Point& b, double t);

} // namespace artplus::geometry
