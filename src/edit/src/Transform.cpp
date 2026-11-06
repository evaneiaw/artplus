#include "artplus/edit/Transform.h"

#include <algorithm>
#include <cmath>

namespace artplus::edit {

namespace {

constexpr double kPi = 3.14159265358979323846;

pdf::PdfMatrix multiply(const pdf::PdfMatrix& a, const pdf::PdfMatrix& b)
{
    // Applies b first, then a.
    pdf::PdfMatrix r;
    r.a = a.a * b.a + a.c * b.b;
    r.b = a.b * b.a + a.d * b.b;
    r.c = a.a * b.c + a.c * b.d;
    r.d = a.b * b.c + a.d * b.d;
    r.e = a.a * b.e + a.c * b.f + a.e;
    r.f = a.b * b.e + a.d * b.f + a.f;
    return r;
}

pdf::PdfMatrix translation(double tx, double ty)
{
    pdf::PdfMatrix m;
    m.e = tx;
    m.f = ty;
    return m;
}

} // namespace

QPointF anchorPoint(const QRectF& bounds, TransformAnchor anchor)
{
    const double x = (static_cast<int>(anchor) % 3 == 0)
                         ? bounds.left()
                         : (static_cast<int>(anchor) % 3 == 1 ? bounds.center().x() : bounds.right());
    const int row = static_cast<int>(anchor) / 3;
    const double y = (row == 0) ? bounds.top() : (row == 1 ? bounds.center().y() : bounds.bottom());
    return QPointF(x, y);
}

pdf::PdfMatrix pageToPdfMatrix(double pageHeight)
{
    pdf::PdfMatrix m;
    m.d = -1.0;
    m.f = pageHeight;
    return m;
}

pdf::PdfMatrix pdfToPageMatrix(double pageHeight)
{
    return pageToPdfMatrix(pageHeight);
}

pdf::PdfMatrix pageTransformMatrix(const TransformParams& params, const QPointF& anchor)
{
    pdf::PdfMatrix scale;
    scale.a = params.scaleX;
    scale.d = params.scaleY;

    pdf::PdfMatrix flip;
    flip.a = params.flipHorizontal ? -1.0 : 1.0;
    flip.d = params.flipVertical ? -1.0 : 1.0;

    pdf::PdfMatrix shear;
    shear.c = -std::tan(params.shearDegrees * kPi / 180.0);

    const double rad = params.rotateDegrees * kPi / 180.0;
    pdf::PdfMatrix rotate;
    rotate.a = std::cos(rad);
    rotate.b = std::sin(rad);
    rotate.c = -std::sin(rad);
    rotate.d = std::cos(rad);

    // Applied to a point: scale, then flip, then shear, then rotate, then move.
    const pdf::PdfMatrix linear = multiply(multiply(multiply(rotate, shear), flip), scale);
    const pdf::PdfMatrix around =
        multiply(multiply(translation(anchor.x(), anchor.y()), linear),
                 translation(-anchor.x(), -anchor.y()));
    return multiply(translation(params.moveX, params.moveY), around);
}

pdf::PdfMatrix objectTargetMatrix(const pdf::PdfMatrix& original, const TransformParams& params,
                                  const QPointF& anchor, double pageHeight, int repeat)
{
    const pdf::PdfMatrix toPdf = pageToPdfMatrix(pageHeight);
    const pdf::PdfMatrix toPage = pdfToPageMatrix(pageHeight);
    const pdf::PdfMatrix transform =
        multiply(multiply(toPdf, pageTransformMatrix(params, anchor)), toPage);

    pdf::PdfMatrix result = original;
    for (int i = 0; i < std::max(0, repeat); ++i)
        result = multiply(transform, result);
    return result;
}

double strokeScaleFactor(const TransformParams& params)
{
    const double mean = (std::abs(params.scaleX) + std::abs(params.scaleY)) / 2.0;
    return std::pow(mean, std::max(0, params.repetitions));
}

} // namespace artplus::edit
