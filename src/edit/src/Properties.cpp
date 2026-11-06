#include "artplus/edit/Properties.h"

#include <cmath>

namespace artplus::edit {

namespace {
double normalizeDegrees(double degrees)
{
    double d = std::fmod(degrees, 360.0);
    if (d > 180.0)
        d -= 360.0;
    if (d <= -180.0)
        d += 360.0;
    return d;
}
} // namespace

ObjectProperties describe(const pdf::EditableObject& object)
{
    ObjectProperties p;
    p.valid = true;
    p.index = object.index;
    p.kind = pdf::pageObjectKindName(object.kind);
    p.bounds = object.bounds;
    p.x = object.bounds.left();
    p.y = object.bounds.top();
    p.width = object.bounds.width();
    p.height = object.bounds.height();
    p.modified = object.modified;

    // Rotation is only reported when the matrix actually contains a non-trivial
    // transformation. A matrix close to identity (a≈1, b≈0, c≈0, d≈1) means no rotation.
    const double a = object.matrixA;
    const double b = object.matrixB;
    const double c = object.matrixC;
    const double d = object.matrixD;
    const double identityThreshold = 0.01; // approximate float drift tolerance
    bool matrixIsIdentity = std::abs(a - 1.0) <= identityThreshold &&
                            std::abs(b) <= identityThreshold &&
                            std::abs(c) <= identityThreshold &&
                            std::abs(d - 1.0) <= identityThreshold;
    if (matrixIsIdentity) {
        p.hasRotation = false;
        p.rotationDegrees = 0.0;
    } else {
        // Rotation is only reported when the matrix actually contains one.
        const double angle =
            normalizeDegrees(std::atan2(object.matrixB, object.matrixA) * 180.0 / 3.14159265358979323846);
        p.hasRotation = std::abs(angle) > 0.01;
        p.rotationDegrees = angle;
    }

    switch (object.kind) {
    case pdf::PageObjectKind::Text:
        p.text = object.text;
        p.fontName = object.fontName;
        p.fontSize = object.fontSize;
        break;
    case pdf::PageObjectKind::Image:
        p.pixelWidth = object.imageWidth;
        p.pixelHeight = object.imageHeight;
        p.dpiX = object.imageDpiX;
        p.dpiY = object.imageDpiY;
        break;
    case pdf::PageObjectKind::Form:
        p.formChildren = object.formChildCount;
        break;
    default:
        break;
    }
    return p;
}

} // namespace artplus::edit
