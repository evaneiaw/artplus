#include "artplus/geometry/Point.h"

#include <algorithm>
#include <cmath>

namespace artplus::geometry {

double Point::length() const
{
    return std::sqrt(x * x + y * y);
}

double Point::lengthSquared() const
{
    return x * x + y * y;
}

Point Point::normalized() const
{
    const double len = length();
    if (len <= 1e-12)
        return {};
    return {x / len, y / len};
}

void Rect::expandToInclude(const Point& p)
{
    left = std::min(left, p.x);
    top = std::min(top, p.y);
    right = std::max(right, p.x);
    bottom = std::max(bottom, p.y);
}

void Rect::expandBy(double d)
{
    left -= d;
    top -= d;
    right += d;
    bottom += d;
}

Rect Rect::united(const Rect& o) const
{
    if (isEmpty())
        return o;
    if (o.isEmpty())
        return *this;
    return {std::min(left, o.left), std::min(top, o.top),
            std::max(right, o.right), std::max(bottom, o.bottom)};
}

Rect Rect::intersected(const Rect& o) const
{
    const Rect r{std::max(left, o.left), std::max(top, o.top),
                 std::min(right, o.right), std::min(bottom, o.bottom)};
    if (r.isEmpty())
        return {};
    return r;
}

bool Rect::contains(const Point& p) const
{
    return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
}

bool Rect::contains(const Rect& r) const
{
    return r.left >= left && r.right <= right && r.top >= top && r.bottom <= bottom;
}

bool Rect::intersects(const Rect& r) const
{
    return !(r.left >= right || r.right <= left || r.top >= bottom || r.bottom <= top);
}

Rect Rect::normalized() const
{
    Rect r{std::min(left, right), std::min(top, bottom), std::max(left, right), std::max(top, bottom)};
    return r;
}

Matrix Matrix::rotationDegrees(double degrees)
{
    return rotationRadians(degrees * 3.14159265358979323846 / 180.0);
}

Matrix Matrix::rotationRadians(double radians)
{
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return {c, s, -s, c, 0.0, 0.0};
}

Point Matrix::map(const Point& p) const
{
    return {a * p.x + c * p.y + e, b * p.x + d * p.y + f};
}

Rect Matrix::mapRect(const Rect& r) const
{
    const Rect n = r.normalized();
    Rect out;
    if (n.isEmpty())
        return out;
    const Point p0 = map({n.left, n.top});
    const Point p1 = map({n.right, n.top});
    const Point p2 = map({n.right, n.bottom});
    const Point p3 = map({n.left, n.bottom});
    out = Rect(p0.x, p0.y, p0.x, p0.y);
    out.expandToInclude(p1);
    out.expandToInclude(p2);
    out.expandToInclude(p3);
    return out;
}

Matrix Matrix::operator*(const Matrix& o) const
{
    return {a * o.a + c * o.b,
            b * o.a + d * o.b,
            a * o.c + c * o.d,
            b * o.c + d * o.d,
            a * o.e + c * o.f + e,
            b * o.e + d * o.f + f};
}

Matrix Matrix::inverted(bool* ok) const
{
    const double det = determinant();
    if (std::abs(det) < 1e-12) {
        if (ok)
            *ok = false;
        return {};
    }
    if (ok)
        *ok = true;
    const double inv = 1.0 / det;
    return {d * inv, -b * inv, -c * inv, a * inv,
            (c * f - d * e) * inv, (b * e - a * f) * inv};
}

bool Matrix::isIdentity() const
{
    return std::abs(a - 1.0) < 1e-12 && std::abs(b) < 1e-12 && std::abs(c) < 1e-12
        && std::abs(d - 1.0) < 1e-12 && std::abs(e) < 1e-12 && std::abs(f) < 1e-12;
}

double distance(const Point& a, const Point& b)
{
    return (a - b).length();
}

Point lerp(const Point& a, const Point& b, double t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

} // namespace artplus::geometry
