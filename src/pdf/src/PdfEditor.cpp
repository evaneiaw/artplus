#include "artplus/pdf/PdfEditor.h"

#include "artplus/core/Logger.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>
#include <cmath>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>
#include <fpdf_text.h>

namespace artplus::pdf {

namespace {

class Writer : public FPDF_FILEWRITE
{
public:
    explicit Writer(QFile& file)
        : m_file(file)
    {
        version = 1;
        WriteBlock = &Writer::writeBlock;
    }

    static int writeBlock(FPDF_FILEWRITE* self, const void* data, unsigned long size)
    {
        auto* writer = static_cast<Writer*>(self);
        const qint64 written =
            writer->m_file.write(static_cast<const char*>(data), static_cast<qint64>(size));
        return written == static_cast<qint64>(size) ? 1 : 0;
    }

private:
    QFile& m_file;
};

int normalizedRotation(int degrees)
{
    int r = degrees % 360;
    if (r < 0)
        r += 360;
    return ((r + 45) / 90 * 90) % 360;
}

PdfMatrix multiply(const PdfMatrix& A, const PdfMatrix& B)
{
    // A composed with B: apply B first, then A.
    PdfMatrix r;
    r.a = A.a * B.a + A.c * B.b;
    r.b = A.b * B.a + A.d * B.b;
    r.c = A.a * B.c + A.c * B.d;
    r.d = A.b * B.c + A.d * B.d;
    r.e = A.a * B.e + A.c * B.f + A.e;
    r.f = A.b * B.e + A.d * B.f + A.f;
    return r;
}

PdfMatrix inverse(const PdfMatrix& m)
{
    const double det = m.a * m.d - m.b * m.c;
    PdfMatrix r;
    if (std::abs(det) < 1e-12)
        return r; // identity fallback
    r.a = m.d / det;
    r.b = -m.b / det;
    r.c = -m.c / det;
    r.d = m.a / det;
    r.e = -(r.a * m.e + r.c * m.f);
    r.f = -(r.b * m.e + r.d * m.f);
    return r;
}

QString utf16ToQString(const unsigned short* data, int chars)
{
    return QString::fromUtf16(data, chars);
}

bool nearlyEqual(double a, double b, double epsilon = 0.75)
{
    return std::abs(a - b) <= epsilon;
}

bool nearlyEqualRect(const QRectF& a, const QRectF& b, double epsilon = 0.75)
{
    return nearlyEqual(a.left(), b.left(), epsilon) &&
           nearlyEqual(a.top(), b.top(), epsilon) &&
           nearlyEqual(a.width(), b.width(), epsilon) &&
           nearlyEqual(a.height(), b.height(), epsilon);
}

} // namespace

struct PdfEditor::Impl {
    PdfDocument* document = nullptr;
    FPDF_PAGE page = nullptr;
    int pageIndex = 0;
    double referenceTop = 0.0;
    QRectF media;
    QVector<EditableObject> objects;
    bool modified = false;

    double toPageY(double pdfY) const { return referenceTop - pdfY; }
    QPointF toPagePoint(double x, double y) const { return QPointF(x, toPageY(y)); }

    QRectF boundsOf(FPDF_PAGEOBJECT object) const
    {
        float l = 0.f, b = 0.f, r = 0.f, t = 0.f;
        if (!FPDFPageObj_GetBounds(object, &l, &b, &r, &t))
            return {};
        return QRectF(l, toPageY(t), r - l, t - b);
    }

    void importPath(FPDF_PAGEOBJECT object, EditableObject& out) const
    {
        const int count = FPDFPath_CountSegments(object);
        out.path.reserve(std::max(0, count));
        for (int i = 0; i < count; ++i) {
            FPDF_PATHSEGMENT segment = FPDFPath_GetPathSegment(object, i);
            if (!segment)
                continue;

            EditablePathSegment imported;
            switch (FPDFPathSegment_GetType(segment)) {
            case FPDF_SEGMENT_MOVETO: imported.type = EditablePathSegment::Type::MoveTo; break;
            case FPDF_SEGMENT_LINETO: imported.type = EditablePathSegment::Type::LineTo; break;
            case FPDF_SEGMENT_BEZIERTO: imported.type = EditablePathSegment::Type::BezierTo; break;
            default: imported.type = EditablePathSegment::Type::Unknown; break;
            }
            float x = 0.f, y = 0.f;
            if (FPDFPathSegment_GetPoint(segment, &x, &y))
                imported.point = toPagePoint(x, y);
            imported.closes = FPDFPathSegment_GetClose(segment) != 0;
            if (imported.closes)
                out.pathClosed = true;
            out.path.append(imported);
        }

        unsigned int r = 0, g = 0, b = 0, a = 0;
        out.hasFillColor = FPDFPageObj_GetFillColor(object, &r, &g, &b, &a) != 0;
        if (out.hasFillColor)
            out.fillColor = (a << 24) | (r << 16) | (g << 8) | b;
        r = g = b = a = 0;
        out.hasStrokeColor = FPDFPageObj_GetStrokeColor(object, &r, &g, &b, &a) != 0;
        if (out.hasStrokeColor)
            out.strokeColor = (a << 24) | (r << 16) | (g << 8) | b;
    }

    void importText(FPDF_PAGEOBJECT object, EditableObject& out) const
    {
        float size = 0.f;
        if (FPDFTextObj_GetFontSize(object, &size))
            out.fontSize = size;

        if (FPDF_FONT font = FPDFTextObj_GetFont(object)) {
            const unsigned long needed = FPDFFont_GetBaseFontName(font, nullptr, 0);
            if (needed > 1) {
                QByteArray buffer(static_cast<int>(needed), '\0');
                const unsigned long written = FPDFFont_GetBaseFontName(
                    font, buffer.data(), static_cast<unsigned long>(buffer.size()));
                if (written > 1) {
                    buffer.resize(static_cast<int>(written));
                    if (buffer.endsWith('\0'))
                        buffer.chop(1);
                    out.fontName = QString::fromLatin1(buffer);
                }
            }
        }

        if (FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page)) {
            const unsigned long needed = FPDFTextObj_GetText(object, textPage, nullptr, 0);
            if (needed > 2) {
                std::vector<unsigned short> buffer(needed / 2 + 1);
                const unsigned long written = FPDFTextObj_GetText(
                    object, textPage, buffer.data(),
                    static_cast<unsigned long>(buffer.size() * sizeof(unsigned short)));
                if (written > 2) {
                    out.text = utf16ToQString(buffer.data(),
                                              static_cast<int>(written / sizeof(unsigned short)));
                    if (out.text.endsWith(QChar('\0')))
                        out.text.chop(1);
                }
            }
            FPDFText_ClosePage(textPage);
        }
    }

    void importImage(FPDF_PAGEOBJECT object, EditableObject& out) const
    {
        FPDF_IMAGEOBJ_METADATA metadata{};
        if (FPDFImageObj_GetImageMetadata(object, page, &metadata)) {
            out.imageWidth = static_cast<int>(metadata.width);
            out.imageHeight = static_cast<int>(metadata.height);
            out.imageDpiX = metadata.horizontal_dpi;
            out.imageDpiY = metadata.vertical_dpi;
        }
    }

    void importObject(int index, FPDF_PAGEOBJECT object)
    {
        EditableObject imported;
        imported.index = index;
        imported.bounds = boundsOf(object);
        {
            FS_MATRIX matrix{};
            if (FPDFPageObj_GetMatrix(object, &matrix)) {
                imported.matrixA = matrix.a;
                imported.matrixB = matrix.b;
                imported.matrixC = matrix.c;
                imported.matrixD = matrix.d;
                imported.matrixE = matrix.e;
                imported.matrixF = matrix.f;
            }
        }

        switch (FPDFPageObj_GetType(object)) {
        case FPDF_PAGEOBJ_TEXT:
            imported.kind = PageObjectKind::Text;
            importText(object, imported);
            break;
        case FPDF_PAGEOBJ_PATH:
            imported.kind = PageObjectKind::Path;
            importPath(object, imported);
            break;
        case FPDF_PAGEOBJ_IMAGE:
            imported.kind = PageObjectKind::Image;
            importImage(object, imported);
            break;
        case FPDF_PAGEOBJ_FORM:
            imported.kind = PageObjectKind::Form;
            imported.formChildCount = FPDFFormObj_CountObjects(object);
            imported.formBBox = imported.bounds;
            break;
        case FPDF_PAGEOBJ_SHADING:
            imported.kind = PageObjectKind::Shading;
            break;
        default:
            imported.kind = PageObjectKind::Unknown;
            break;
        }

        float strokeWidth = 0.f;
        if (FPDFPageObj_GetStrokeWidth(object, &strokeWidth))
            imported.strokeWidth = strokeWidth;

        objects.append(imported);
    }

    void importAll()
    {
        objects.clear();
        if (!page)
            return;
        const int count = FPDFPage_CountObjects(page);
        objects.reserve(std::max(0, count));
        for (int i = 0; i < count; ++i) {
            FPDF_PAGEOBJECT object = FPDFPage_GetObject(page, i);
            if (!object)
                continue;
            importObject(i, object);
        }
    }

    FPDF_PAGEOBJECT handle(int index) const
    {
        if (!page || index < 0 || index >= FPDFPage_CountObjects(page))
            return nullptr;
        return FPDFPage_GetObject(page, index);
    }

    // Captures kind + bounds for verification after saving.
    QVector<QPair<PageObjectKind, QRectF>> signature() const
    {
        QVector<QPair<PageObjectKind, QRectF>> result;
        result.reserve(objects.size());
        for (const EditableObject& object : objects)
            result.append({object.kind, object.bounds});
        return result;
    }
};

QVector<EditableCurve> EditableObject::curves() const
{
    QVector<EditableCurve> out;
    QPointF current;
    int i = 0;
    while (i < path.size()) {
        const EditablePathSegment& segment = path.at(i);
        if (segment.type == EditablePathSegment::Type::MoveTo) {
            EditableCurve move;
            move.kind = EditableCurve::Kind::Move;
            move.start = segment.point;
            move.end = segment.point;
            out.append(move);
            current = segment.point;
            ++i;
        } else if (segment.type == EditablePathSegment::Type::LineTo) {
            EditableCurve line;
            line.kind = EditableCurve::Kind::Line;
            line.start = current;
            line.end = segment.point;
            out.append(line);
            current = segment.point;
            ++i;
        } else if (segment.type == EditablePathSegment::Type::BezierTo) {
            // A cubic is stored as three consecutive BezierTo points.
            if (i + 2 < path.size() &&
                path.at(i + 1).type == EditablePathSegment::Type::BezierTo &&
                path.at(i + 2).type == EditablePathSegment::Type::BezierTo) {
                EditableCurve cubic;
                cubic.kind = EditableCurve::Kind::Cubic;
                cubic.start = current;
                cubic.control1 = segment.point;
                cubic.control2 = path.at(i + 1).point;
                cubic.end = path.at(i + 2).point;
                out.append(cubic);
                current = cubic.end;
                i += 3;
            } else {
                ++i;
            }
        } else {
            ++i;
        }
    }
    return out;
}
PdfEditor::PdfEditor(PdfDocument& document, int pageIndex)
    : d(std::make_unique<Impl>())
{
    d->document = &document;
    d->pageIndex = pageIndex;
    if (!document.isValid() || !document.hasPage(pageIndex)) {
        m_warnings << QStringLiteral("Invalid document or page index");
        return;
    }

    d->media = document.pageBoxes(pageIndex).media;
    d->referenceTop = d->media.top() + d->media.height();

    d->page = FPDF_LoadPage(static_cast<FPDF_DOCUMENT>(document.nativeHandle()), pageIndex);
    if (!d->page) {
        m_warnings << QStringLiteral("PDFium could not load page %1").arg(pageIndex + 1);
        return;
    }

    d->importAll();

    for (const EditableObject& object : d->objects) {
        if (object.kind == PageObjectKind::Shading) {
            m_warnings << QStringLiteral("Shading object at index %1 is read-only; "
                                         "saving is blocked to avoid data loss")
                              .arg(object.index);
        } else if (object.kind == PageObjectKind::Unknown) {
            m_warnings << QStringLiteral("Unknown object at index %1 cannot be preserved safely; "
                                         "saving is blocked")
                              .arg(object.index);
        }
    }
}

PdfEditor::~PdfEditor()
{
    if (d && d->page) {
        FPDF_ClosePage(d->page);
        d->page = nullptr;
    }
}

bool PdfEditor::isValid() const { return d && d->page != nullptr; }
int PdfEditor::pageIndex() const { return d ? d->pageIndex : -1; }
double PdfEditor::pageHeightPoints() const { return d ? d->referenceTop : 0.0; }
int PdfEditor::objectCount() const { return d ? static_cast<int>(d->objects.size()) : 0; }

const EditableObject* PdfEditor::object(int index) const
{
    if (!d || index < 0 || index >= d->objects.size())
        return nullptr;
    return &d->objects.at(index);
}

const QVector<EditableObject>& PdfEditor::objects() const
{
    static const QVector<EditableObject> empty;
    return d ? d->objects : empty;
}

bool PdfEditor::isModified() const { return d && d->modified; }

PdfMatrix PdfEditor::objectMatrix(int index) const
{
    PdfMatrix result;
    if (!d || !d->page || index < 0 || index >= static_cast<int>(d->objects.size()))
        return result;
    FPDF_PAGEOBJECT object = d->handle(d->objects.at(index).index);
    if (!object)
        return result;
    FS_MATRIX matrix{};
    if (FPDFPageObj_GetMatrix(object, &matrix)) {
        result.a = matrix.a;
        result.b = matrix.b;
        result.c = matrix.c;
        result.d = matrix.d;
        result.e = matrix.e;
        result.f = matrix.f;
    }
    return result;
}

bool PdfEditor::setObjectMatrix(int index, const PdfMatrix& matrix)
{
    if (!d || !d->page || index < 0 || index >= d->objects.size())
        return false;

    EditableObject& target = d->objects[index];
    if (target.kind == PageObjectKind::Shading || target.kind == PageObjectKind::Unknown)
        return false;

    FPDF_PAGEOBJECT object = d->handle(target.index);
    if (!object)
        return false;

    FS_MATRIX current{};
    if (!FPDFPageObj_GetMatrix(object, &current))
        return false;

    PdfMatrix cur;
    cur.a = current.a; cur.b = current.b; cur.c = current.c;
    cur.d = current.d; cur.e = current.e; cur.f = current.f;

    // FPDFPageObj_TransformF composes the object matrix on the left:
    // new = relative * current, therefore relative = target * inverse(current).
    const PdfMatrix relative = multiply(matrix, inverse(cur));
    const FS_MATRIX rel{static_cast<float>(relative.a), static_cast<float>(relative.b),
                        static_cast<float>(relative.c), static_cast<float>(relative.d),
                        static_cast<float>(relative.e), static_cast<float>(relative.f)};
    if (!FPDFPageObj_TransformF(object, &rel))
        return false;

    target.matrixA = matrix.a; target.matrixB = matrix.b; target.matrixC = matrix.c;
    target.matrixD = matrix.d; target.matrixE = matrix.e; target.matrixF = matrix.f;
    target.bounds = d->boundsOf(object);
    target.modified = true;
    d->modified = true;
    return true;
}

bool PdfEditor::translateObject(int index, double dx, double dy)
{
    if (!d || index < 0 || index >= d->objects.size())
        return false;

    // Page coordinates use a top-left origin; PDF uses bottom-left.
    PdfMatrix translation;
    translation.e = dx;
    translation.f = -dy;

    const PdfMatrix target = multiply(translation, objectMatrix(index));
    if (!setObjectMatrix(index, target))
        return false;

    d->objects[index].translation += QPointF(dx, dy);
    logInfo(QStringLiteral("Moved object %1 by (%2, %3) pt").arg(index).arg(dx).arg(dy));
    return true;
}

double PdfEditor::objectStrokeWidth(int index) const
{
    if (!d || index < 0 || index >= d->objects.size())
        return 0.0;
    FPDF_PAGEOBJECT object = d->handle(d->objects.at(index).index);
    if (!object)
        return 0.0;
    float width = 0.f;
    if (!FPDFPageObj_GetStrokeWidth(object, &width))
        return 0.0;
    return width;
}

bool PdfEditor::setObjectStrokeWidth(int index, double width)
{
    if (!d || !d->page || index < 0 || index >= d->objects.size() || width < 0.0)
        return false;
    EditableObject& target = d->objects[index];
    if (target.kind == PageObjectKind::Shading || target.kind == PageObjectKind::Unknown)
        return false;
    FPDF_PAGEOBJECT object = d->handle(target.index);
    if (!object)
        return false;
    if (!FPDFPageObj_SetStrokeWidth(object, static_cast<float>(width)))
        return false;
    target.strokeWidth = width;
    target.modified = true;
    d->modified = true;
    return true;
}

int PdfEditor::duplicateObject(int sourceIndex, const PdfMatrix& matrix, QString* errorMessage)
{
    if (!d || !d->page || sourceIndex < 0 || sourceIndex >= static_cast<int>(d->objects.size())) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Invalid source object");
        return -1;
    }

    const EditableObject& source = d->objects.at(sourceIndex);
    FPDF_PAGEOBJECT src = d->handle(source.index);
    if (!src) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Source object is not available");
        return -1;
    }

    FPDF_PAGEOBJECT copy = nullptr;
    switch (source.kind) {
    case PageObjectKind::Path: {
        QPointF start;
        for (const EditablePathSegment& segment : source.path) {
            if (segment.type == EditablePathSegment::Type::MoveTo ||
                segment.type == EditablePathSegment::Type::LineTo ||
                segment.type == EditablePathSegment::Type::BezierTo) {
                start = segment.point;
                break;
            }
        }
        copy = FPDFPageObj_CreateNewPath(static_cast<float>(start.x()),
                                         static_cast<float>(start.y()));
        if (!copy)
            break;

        int i = 0;
        bool first = true;
        while (i < source.path.size()) {
            const EditablePathSegment& segment = source.path.at(i);
            if (segment.type == EditablePathSegment::Type::MoveTo) {
                if (first) {
                    first = false;
                } else if (!FPDFPath_MoveTo(copy, static_cast<float>(segment.point.x()),
                                            static_cast<float>(segment.point.y()))) {
                    FPDFPageObj_Destroy(copy);
                    copy = nullptr;
                    break;
                }
                ++i;
            } else if (segment.type == EditablePathSegment::Type::LineTo) {
                if (!FPDFPath_LineTo(copy, static_cast<float>(segment.point.x()),
                                     static_cast<float>(segment.point.y()))) {
                    FPDFPageObj_Destroy(copy);
                    copy = nullptr;
                    break;
                }
                ++i;
            } else if (segment.type == EditablePathSegment::Type::BezierTo &&
                       i + 2 < source.path.size() &&
                       source.path.at(i + 1).type == EditablePathSegment::Type::BezierTo &&
                       source.path.at(i + 2).type == EditablePathSegment::Type::BezierTo) {
                if (!FPDFPath_BezierTo(copy,
                                       static_cast<float>(segment.point.x()),
                                       static_cast<float>(segment.point.y()),
                                       static_cast<float>(source.path.at(i + 1).point.x()),
                                       static_cast<float>(source.path.at(i + 1).point.y()),
                                       static_cast<float>(source.path.at(i + 2).point.x()),
                                       static_cast<float>(source.path.at(i + 2).point.y()))) {
                    FPDFPageObj_Destroy(copy);
                    copy = nullptr;
                    break;
                }
                i += 3;
            } else {
                ++i;
            }
        }
        if (!copy)
            break;

        unsigned int r = 0, g = 0, b = 0, a = 0;
        if (FPDFPageObj_GetFillColor(src, &r, &g, &b, &a))
            FPDFPageObj_SetFillColor(copy, r, g, b, a);
        if (FPDFPageObj_GetStrokeColor(src, &r, &g, &b, &a))
            FPDFPageObj_SetStrokeColor(copy, r, g, b, a);
        float strokeWidth = 0.f;
        if (FPDFPageObj_GetStrokeWidth(src, &strokeWidth))
            FPDFPageObj_SetStrokeWidth(copy, strokeWidth);
        int fillMode = 0;
        FPDF_BOOL stroked = 0;
        if (FPDFPath_GetDrawMode(src, &fillMode, &stroked))
            FPDFPath_SetDrawMode(copy, fillMode, stroked);
        if (source.pathClosed)
            FPDFPath_Close(copy);
        break;
    }
    case PageObjectKind::Text: {
        FPDF_FONT font = FPDFTextObj_GetFont(src);
        if (!font) {
            if (errorMessage)
                *errorMessage = QStringLiteral("Text object has no reusable font");
            return -1;
        }
        copy = FPDFPageObj_CreateTextObj(
            static_cast<FPDF_DOCUMENT>(d->document->nativeHandle()), font,
            static_cast<float>(source.fontSize));
        if (!copy)
            break;
        if (!source.text.isEmpty()) {
            FPDFText_SetText(copy,
                             reinterpret_cast<FPDF_WIDESTRING>(source.text.utf16()));
        }
        unsigned int r = 0, g = 0, b = 0, a = 0;
        if (FPDFPageObj_GetFillColor(src, &r, &g, &b, &a))
            FPDFPageObj_SetFillColor(copy, r, g, b, a);
        if (FPDFPageObj_GetStrokeColor(src, &r, &g, &b, &a))
            FPDFPageObj_SetStrokeColor(copy, r, g, b, a);
        float strokeWidth = 0.f;
        if (FPDFPageObj_GetStrokeWidth(src, &strokeWidth))
            FPDFPageObj_SetStrokeWidth(copy, strokeWidth);
        FPDFTextObj_SetTextRenderMode(copy, FPDFTextObj_GetTextRenderMode(src));
        break;
    }
    case PageObjectKind::Image: {
        FPDF_BITMAP bitmap = FPDFImageObj_GetBitmap(src);
        if (!bitmap) {
            if (errorMessage)
                *errorMessage = QStringLiteral("Image data could not be extracted");
            return -1;
        }
        copy = FPDFPageObj_NewImageObj(
            static_cast<FPDF_DOCUMENT>(d->document->nativeHandle()));
        if (copy) {
            FPDF_PAGE pages = d->page;
            if (!FPDFImageObj_SetBitmap(&pages, 1, copy, bitmap)) {
                FPDFPageObj_Destroy(copy);
                copy = nullptr;
            }
        }
        FPDFBitmap_Destroy(bitmap);
        break;
    }
    default:
        if (errorMessage)
            *errorMessage = QStringLiteral("This object type cannot be duplicated yet");
        return -1;
    }

    if (!copy) {
        if (errorMessage)
            *errorMessage = QStringLiteral("PDFium could not create the copied object");
        return -1;
    }

    const FS_MATRIX target{static_cast<float>(matrix.a), static_cast<float>(matrix.b),
                           static_cast<float>(matrix.c), static_cast<float>(matrix.d),
                           static_cast<float>(matrix.e), static_cast<float>(matrix.f)};
    if (!FPDFPageObj_SetMatrix(copy, &target)) {
        FPDFPageObj_Destroy(copy);
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not place the copied object");
        return -1;
    }
    if (!FPDFPage_InsertObject(d->page, copy)) {
        FPDFPageObj_Destroy(copy);
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not insert the copied object");
        return -1;
    }

    const int newIndex = FPDFPage_CountObjects(d->page) - 1;
    d->modified = true;
    return newIndex;
}

bool PdfEditor::removeObject(int index)
{
    if (!d || !d->page)
        return false;
    FPDF_PAGEOBJECT object = d->handle(index);
    if (!object)
        return false;
    if (!FPDFPage_RemoveObject(d->page, object))
        return false;
    FPDFPageObj_Destroy(object);
    d->modified = true;
    return true;
}

void PdfEditor::refresh()
{
    if (!d || !d->page)
        return;
    d->importAll();
}

bool PdfEditor::applyEdits()

{
    if (!d || !d->page)
        return false;
    const bool ok = FPDFPage_GenerateContent(d->page) != 0;
    if (!ok)
        logWarning(QStringLiteral("FPDFPage_GenerateContent failed"));
    return ok;
}

QImage PdfEditor::render(double scale, int viewRotation) const
{
    if (!d || !d->page)
        return {};

    const double rawWidth = FPDF_GetPageWidthF(d->page);
    const double rawHeight = FPDF_GetPageHeightF(d->page);
    if (rawWidth <= 0.0 || rawHeight <= 0.0)
        return {};

    const double effectiveScale = std::clamp(scale, 0.01, 32.0);
    const int width = std::max(1, static_cast<int>(std::ceil(rawWidth * effectiveScale)));
    const int height = std::max(1, static_cast<int>(std::ceil(rawHeight * effectiveScale)));

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    if (!bitmap)
        return {};
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, d->page, 0, 0, width, height, 0,
                          FPDF_ANNOT | FPDF_NO_NATIVETEXT);

    const auto* buffer = static_cast<const uchar*>(FPDFBitmap_GetBuffer(bitmap));
    const int stride = FPDFBitmap_GetStride(bitmap);
    QImage image(buffer, width, height, stride, QImage::Format_ARGB32);
    QImage copy = image.copy();
    FPDFBitmap_Destroy(bitmap);

    const int intrinsic = normalizedRotation(FPDFPage_GetRotation(d->page) * 90);
    const int total = normalizedRotation(intrinsic + viewRotation);
    if (total != 0) {
        QTransform transform;
        transform.rotate(total);
        copy = copy.transformed(transform, Qt::SmoothTransformation);
    }
    return copy;
}

bool PdfEditor::saveAs(const QString& path, QString* errorMessage)
{
    if (!d || !d->page || !d->document) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No page is open");
        return false;
    }

    const QString sourcePath = d->document->filePath();
    if (!sourcePath.isEmpty() &&
        QFileInfo(path).absoluteFilePath() == QFileInfo(sourcePath).absoluteFilePath()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Refusing to overwrite the source document");
        return false;
    }

    if (!m_warnings.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral(
                                "Export blocked because the page contains content that cannot be "
                                "preserved safely:\n%1")
                                .arg(m_warnings.join(QLatin1Char('\n')));
        return false;
    }

    if (!FPDFPage_GenerateContent(d->page)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("PDFium could not regenerate the page content");
        return false;
    }

    const auto expected = d->signature();
    const QString tempPath = path + QStringLiteral(".artplus-tmp");

    auto writeWithFlags = [&](int flags) -> bool {
        QFile output(tempPath);
        if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        Writer writer(output);
        const FPDF_BOOL ok =
            FPDF_SaveAsCopy(static_cast<FPDF_DOCUMENT>(d->document->nativeHandle()),
                            &writer, flags);
        output.flush();
        output.close();
        if (!ok)
            QFile::remove(tempPath);
        return ok != 0;
    };

    auto verify = [&]() -> bool {
        QString openError;
        auto reopened = PdfDocument::open(tempPath, &openError);
        if (!reopened || !reopened->isValid())
            return false;
        if (reopened->pageCount() != d->document->pageCount())
            return false;

        // Verify every page's dimensions, including pages untouched by the
        // editor. This catches accidental page-tree or page-box loss during
        // export without inventing a comparison of unsupported resources.
        for (int page = 0; page < d->document->pageCount(); ++page) {
            const QSizeF expectedSize = d->document->pageSize(page);
            const QSizeF actualSize = reopened->pageSize(page);
            if (std::abs(expectedSize.width() - actualSize.width()) > 0.01 ||
                std::abs(expectedSize.height() - actualSize.height()) > 0.01)
                return false;
        }

        PdfEditor check(*reopened, d->pageIndex);
        if (!check.isValid())
            return false;
        if (check.objectCount() != expected.size())
            return false;

        for (int i = 0; i < expected.size(); ++i) {
            const EditableObject* actual = check.object(i);
            if (!actual)
                return false;
            if (actual->kind != expected[i].first)
                return false;
            if (!nearlyEqualRect(actual->bounds, expected[i].second))
                return false;
        }
        return true;
    };

    bool verified = false;
    bool usedIncremental = false;
    if (writeWithFlags(FPDF_INCREMENTAL) && verify()) {
        verified = true;
        usedIncremental = true;
    } else {
        QFile::remove(tempPath);
        if (writeWithFlags(FPDF_NO_INCREMENTAL) && verify())
            verified = true;
    }
    logInfo(usedIncremental ? QStringLiteral("Save strategy: incremental update")
                            : QStringLiteral("Save strategy: full rewrite"));

    if (!verified) {
        QFile::remove(tempPath);
        if (errorMessage)
            *errorMessage = QStringLiteral(
                                "The saved file did not preserve the expected objects; "
                                "export was aborted");
        return false;
    }

    QFile::remove(path);
    if (!QFile::rename(tempPath, path)) {
        QFile::remove(tempPath);
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not move the verified file to %1").arg(path);
        return false;
    }

    d->modified = false;
    logInfo(QStringLiteral("Saved edited page to %1 (verified)").arg(path));
    return true;
}

// Group/ungroup/lock/visible - not supported in PDFium model
bool PdfEditor::groupSelected() { return false; }
bool PdfEditor::ungroupSelected() { return false; }
bool PdfEditor::setLocked(bool) { return false; }
bool PdfEditor::setVisible(bool) { return false; }

} // namespace artplus::pdf
