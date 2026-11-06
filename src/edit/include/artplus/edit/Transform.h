#pragma once

#include "artplus/pdf/PdfEditor.h"

#include <QPointF>
#include <QRectF>

namespace artplus::edit {

// Anchor points on the selection bounding box, in the order ArtPro+ v24 shows
// them (row-major, starting at the top-left corner).
enum class TransformAnchor {
    TopLeft = 0,
    TopCenter,
    TopRight,
    MiddleLeft,
    Center,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
};

// Numeric transform parameters. All lengths are PDF points and all angles are
// degrees. Move is expressed in page coordinates (y grows downwards), matching
// the values the ArtPro+ dialog accepts in millimetres.
struct TransformParams {
    double moveX = 0.0;
    double moveY = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    bool flipHorizontal = false;
    bool flipVertical = false;
    double shearDegrees = 0.0;
    double rotateDegrees = 0.0;
    int repetitions = 1;
    bool transformEach = false;
    bool includeGradientsAndPatterns = true;
    bool includeStrokeWeightsAndEffects = false;
    bool includeOpacityMasks = true;
};

QPointF anchorPoint(const QRectF& bounds, TransformAnchor anchor);

// Converts between page space (top-left origin, y down) and PDF user space
// (bottom-left origin, y up) of a page whose top edge is at `pageHeight`.
pdf::PdfMatrix pageToPdfMatrix(double pageHeight);
pdf::PdfMatrix pdfToPageMatrix(double pageHeight);

// Builds the page-space transform for one application:
//   scale -> flip -> shear -> rotate -> move
// around `anchor` (page coordinates). This order was measured against
// ArtPro+ v24.
pdf::PdfMatrix pageTransformMatrix(const TransformParams& params, const QPointF& anchor);

// Converts the page-space transform to PDF space and composes it `repeat`
// times onto an object matrix.
pdf::PdfMatrix objectTargetMatrix(const pdf::PdfMatrix& original, const TransformParams& params,
                                  const QPointF& anchor, double pageHeight, int repeat);

// Effective stroke width multiplier for "Stroke Weights & Effects". v24 uses
// the arithmetic mean of the two scale factors and applies the transform once
// per repetition.
double strokeScaleFactor(const TransformParams& params);

} // namespace artplus::edit
