#include "artplus/geometry/Point.h"

#include <algorithm>
#include <cmath>

namespace artplus::geometry {

namespace {

// Dimensionless threshold on |sin(theta)| between the two column vectors of the
// linear part. Because det = |col0| * |col1| * sin(theta), comparing |det| with
// |col0| * |col1| measures *linear dependence* and is invariant under a uniform
// scale of any magnitude. 1e-12 leaves about 4.5e3 * DBL_EPSILON of head-room:
// below it the inverse would amplify rounding error beyond what a double can
// represent meaningfully, so the matrix is numerically singular.
constexpr double kSingularSineEpsilon = 1e-12;

} // namespace

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
    // Divide by the largest component first. This is the standard scale-invariant
    // formulation: it cannot overflow on huge vectors and does not underflow on
    // tiny ones, so every non-zero vector keeps its direction. The previous
    // absolute cut-off (len <= 1e-12 -> zero) silently zeroed valid small
    // vectors such as (3e-15, 4e-15).
    const double magnitude = std::max(std::abs(x), std::abs(y));
    if (magnitude == 0.0)
        return {};
    const double sx = x / magnitude;
    const double sy = y / magnitude;
    const double scaledLength = std::sqrt(sx * sx + sy * sy);
    return {sx / scaledLength, sy / scaledLength};
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
    // Degenerate geometry is still geometry. A rect with zero width (vertical
    // segment), zero height (horizontal segment) or zero area (single point) is
    // transformed corner by corner and its envelope is returned, so the position
    // and orientation survive the transform. Rect::isEmpty() deliberately treats
    // those as empty, which is why it is NOT used here: an inside-out rect is
    // normalised first, and only truly invalid (NaN) input yields an empty rect.
    const Rect n = r.normalized();
    if (std::isnan(n.left) || std::isnan(n.top) || std::isnan(n.right) || std::isnan(n.bottom))
        return {};

    const Point p0 = map({n.left, n.top});
    const Point p1 = map({n.right, n.top});
    const Point p2 = map({n.right, n.bottom});
    const Point p3 = map({n.left, n.bottom});

    Rect out(p0.x, p0.y, p0.x, p0.y);
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
    // A 2D affine transform is invertible iff its 2x2 linear part is. Singularity
    // is therefore tested on the (dimensionless) sine of the angle between the
    // two column vectors instead of on |det|, whose magnitude scales with the
    // transform: an absolute |det| < 1e-12 rejected every scale below ~1e-6 even
    // though those transforms are perfectly invertible in double precision.
    const double col0Length = std::sqrt(a * a + b * b);   // image of (1, 0)
    const double col1Length = std::sqrt(c * c + d * d);   // image of (0, 1)

    bool singular = false;
    if (col0Length == 0.0 || col1Length == 0.0) {
        singular = true; // a collapsed axis can never be inverted
    } else {
        // |sin(theta)| = |det| / (|col0| * |col1|); normalising by the column
        // lengths removes the scale factor entirely.
        const double sine = std::abs(determinant()) / (col0Length * col1Length);
        singular = !(sine > kSingularSineEpsilon);
    }

    if (singular) {
        if (ok)
            *ok = false;
        return {};
    }
    if (ok)
        *ok = true;

    const double det = determinant();
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
