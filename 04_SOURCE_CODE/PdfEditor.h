#pragma once

#include "artplus/pdf/PdfDocument.h"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>

#include <memory>

namespace artplus::pdf {

// One path command imported from a PDFium path object.
struct EditablePathSegment {
    enum class Type { MoveTo, LineTo, BezierTo, Unknown };
    Type type = Type::Unknown;
    QPointF point;   // end point, page coordinates (top-left origin)
    bool closes = false;
};

// Affine matrix in PDF convention: (x,y) -> (a*x + c*y + e, b*x + d*y + f).
struct PdfMatrix {
    double a = 1.0, b = 0.0, c = 0.0, d = 1.0, e = 0.0, f = 0.0;
};

// Structured curve reconstructed from the raw segment list.
// PDFium stores a cubic Bezier as three consecutive BezierTo points
// (control 1, control 2, end point), in path-local coordinates.
struct EditableCurve {
    enum class Kind { Move, Line, Cubic };
    Kind kind = Kind::Move;
    QPointF start;
    QPointF control1;
    QPointF control2;
    QPointF end;
};

// Editable view over one PDF page.
//
// The object keeps the PDFium page open so objects can be transformed and the
// page content regenerated. Imported geometry is a snapshot for inspection;
// edits are applied through PDFium so untouched resources stay intact.
struct EditableObject {
    int index = 0;
    PageObjectKind kind = PageObjectKind::Unknown;
    QRectF bounds; // page coordinates, top-left origin

    // Path
    QVector<EditablePathSegment> path;
    bool pathClosed = false;
    bool hasFillColor = false;
    bool hasStrokeColor = false;
    // Actual paint read back with FPDFPageObj_GetFillColor/GetStrokeColor,
    // packed as 0xAARRGGBB. Only meaningful when the matching has*Color flag is
    // set. PDFium normalises the paint to RGBA, so these identify equal paint,
    // not the original CMYK ink composition.
    unsigned int fillColor = 0;
    unsigned int strokeColor = 0;
    double strokeWidth = 0.0;

    // Text
    QString text;
    QString fontName;
    double fontSize = 0.0;

    // Image
    int imageWidth = 0;
    int imageHeight = 0;
    double imageDpiX = 0.0;
    double imageDpiY = 0.0;

    // Form XObject
    int formChildCount = 0;
    QRectF formBBox;

    // Object transform matrix (PDF convention: x' = a*x + c*y + e).
    double matrixA = 1.0, matrixB = 0.0, matrixC = 0.0, matrixD = 1.0,
           matrixE = 0.0, matrixF = 0.0;

    // Edit bookkeeping
    bool modified = false;
    QPointF translation; // accumulated translation, page coordinates

    // Reconstructs lines/cubics from the raw segment list.
    QVector<EditableCurve> curves() const;
};

class PdfEditor
{
public:
    PdfEditor(PdfDocument& document, int pageIndex);
    ~PdfEditor();

    PdfEditor(const PdfEditor&) = delete;
    PdfEditor& operator=(const PdfEditor&) = delete;

    bool isValid() const;
    int pageIndex() const;
    double pageHeightPoints() const;
    int objectCount() const;
    const EditableObject* object(int index) const;
    const QVector<EditableObject>& objects() const;

    bool isModified() const;
    const QStringList& warnings() const { return m_warnings; }

    // Current object matrix in PDF convention.
    PdfMatrix objectMatrix(int index) const;

    // Sets the object matrix exactly (no cumulative drift). Returns false when
    // the object kind is not editable or the index is invalid.
    bool setObjectMatrix(int index, const PdfMatrix& matrix);

    // Moves an object by (dx, dy) in *page* coordinates (top-left origin,
    // points), composing the translation in page space so rotated objects move
    // along the requested direction.
    bool translateObject(int index, double dx, double dy);

    // Stroke width (PDF points) used by "Stroke Weights & Effects".
    double objectStrokeWidth(int index) const;
    bool setObjectStrokeWidth(int index, double width);

    // Creates a copy of `sourceIndex` (path, text or image) with `matrix` as
    // its absolute matrix and appends it to the page. Returns the new object
    // index, or -1 when the kind cannot be duplicated safely.
    int duplicateObject(int sourceIndex, const PdfMatrix& matrix, QString* errorMessage = nullptr);

    // Removes and destroys the page object at `index`.
    bool removeObject(int index);

    // Re-imports the page object list. Needed after inserting/removing objects.
    void refresh();

    // Writes the document to `path`, never touching the source file.
    // Tries an incremental update first and verifies the result by reopening it.
    // Regenerates the page content from the edited objects (needed before
    // rendering or saving).
    bool applyEdits();

    // Renders the (possibly edited) page. scale is pixels per point.
    QImage render(double scale, int viewRotation = 0) const;

    bool saveAs(const QString& path, QString* errorMessage);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
    QStringList m_warnings;
};

} // namespace artplus::pdf
