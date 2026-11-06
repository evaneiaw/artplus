#include "artplus/pdf/PdfDocument.h"

#include "artplus/core/Logger.h"

#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QSaveFile>
#include <QStringList>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>
#include <fpdf_text.h>
#include <fpdf_doc.h>
#include <fpdf_transformpage.h>

namespace artplus::pdf {

namespace {

struct GlobalRuntime {
    bool initialized = false;
    int refCount = 0;
};

GlobalRuntime& runtime()
{
    static GlobalRuntime r;
    return r;
}

QString readMetaText(FPDF_DOCUMENT doc, const char* tag)
{
    if (!doc)
        return {};
    const unsigned long len = FPDF_GetMetaText(doc, tag, nullptr, 0);
    if (len <= 2)
        return {};
    std::vector<unsigned short> buffer(len / sizeof(unsigned short) + 1);
    const unsigned long written = FPDF_GetMetaText(doc, tag, buffer.data(),
                                                   static_cast<unsigned long>(buffer.size() * sizeof(unsigned short)));
    if (written <= 2)
        return {};
    return QString::fromUtf16(buffer.data());
}

int normalizedRotation(int degrees)
{
    int r = degrees % 360;
    if (r < 0)
        r += 360;
    // Snap to the four right angles supported by the viewer.
    return ((r + 45) / 90 * 90) % 360;
}

// PDF boxes use a bottom-left origin; the application uses top-left.
QRectF boxToTopLeft(float l, float b, float r, float t, double referenceTop)
{
    const double left = std::min(l, r);
    const double right = std::max(l, r);
    const double topPdf = std::max(b, t);
    const double bottomPdf = std::min(b, t);
    return QRectF(left, referenceTop - topPdf, right - left, topPdf - bottomPdf);
}

PageObjectKind kindFromPdfium(int type)
{
    switch (type) {
    case FPDF_PAGEOBJ_TEXT: return PageObjectKind::Text;
    case FPDF_PAGEOBJ_PATH: return PageObjectKind::Path;
    case FPDF_PAGEOBJ_IMAGE: return PageObjectKind::Image;
    case FPDF_PAGEOBJ_SHADING: return PageObjectKind::Shading;
    case FPDF_PAGEOBJ_FORM: return PageObjectKind::Form;
    default: return PageObjectKind::Unknown;
    }
}

class SaveFileWriter : public FPDF_FILEWRITE
{
public:
    explicit SaveFileWriter(QSaveFile& file)
        : m_file(file)
    {
        version = 1;
        WriteBlock = &SaveFileWriter::writeBlock;
    }

    static int writeBlock(FPDF_FILEWRITE* self, const void* data, unsigned long size)
    {
        auto* writer = static_cast<SaveFileWriter*>(self);
        const qint64 written = writer->m_file.write(static_cast<const char*>(data),
                                                    static_cast<qint64>(size));
        return written == static_cast<qint64>(size) ? 1 : 0;
    }

private:
    QSaveFile& m_file;
};

// Reads a PDF literal string starting at `pos` (which must point at '(').
QString pdfLiteralStringAt(const QByteArray& data, int pos)
{
    if (pos < 0 || pos >= data.size() || data.at(pos) != '(')
        return {};
    QByteArray out;
    int depth = 1;
    for (int i = pos + 1; i < data.size(); ++i) {
        const char c = data.at(i);
        if (c == '\\') {
            if (i + 1 < data.size()) {
                const char n = data.at(++i);
                switch (n) {
                case 'n': out.append('\n'); break;
                case 'r': out.append('\r'); break;
                case 't': out.append('\t'); break;
                case 'b': out.append('\b'); break;
                case 'f': out.append('\f'); break;
                case '(': out.append('('); break;
                case ')': out.append(')'); break;
                case '\\': out.append('\\'); break;
                default: out.append(n); break;
                }
            }
            continue;
        }
        if (c == '(') {
            ++depth;
            out.append(c);
            continue;
        }
        if (c == ')') {
            if (--depth == 0)
                break;
            out.append(c);
            continue;
        }
        out.append(c);
    }
    return QString::fromLatin1(out).trimmed();
}

// PDF output intents are stored as /OutputConditionIdentifier (or
// /OutputCondition) in the document catalog.
QString extractOutputIntentName(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray data = file.read(64 * 1024 * 1024);
    if (data.isEmpty())
        return {};

    const char* keys[] = {"OutputConditionIdentifier", "OutputCondition"};
    for (const char* key : keys) {
        const int keyLen = static_cast<int>(std::strlen(key));
        int idx = data.indexOf(key);
        while (idx >= 0) {
            const int after = idx + keyLen;
            const int paren = data.indexOf('(', after);
            if (paren > 0 && paren - after < 32) {
                const QString value = pdfLiteralStringAt(data, paren);
                if (!value.isEmpty() && value.size() < 200)
                    return value;
            }
            idx = data.indexOf(key, idx + 1);
        }
    }
    return {};
}

} // namespace

struct PdfDocument::Impl {
    FPDF_DOCUMENT document = nullptr;
};

void PdfRuntime::initialize()
{
    auto& r = runtime();
    if (r.refCount++ == 0) {
        FPDF_LIBRARY_CONFIG config{};
        config.version = 2;
        config.m_pUserFontPaths = nullptr;
        config.m_pIsolate = nullptr;
        config.m_v8EmbedderSlot = 0;
        FPDF_InitLibraryWithConfig(&config);
        r.initialized = true;
        logInfo(QStringLiteral("PDFium initialized"));
    }
}

void PdfRuntime::shutdown()
{
    auto& r = runtime();
    if (r.refCount > 0)
        --r.refCount;
    if (r.refCount == 0 && r.initialized) {
        FPDF_DestroyLibrary();
        r.initialized = false;
        logInfo(QStringLiteral("PDFium shut down"));
    }
}

QString PdfRuntime::lastErrorText(unsigned long errorCode)
{
    switch (errorCode) {
    case FPDF_ERR_SUCCESS: return QStringLiteral("No error");
    case FPDF_ERR_UNKNOWN: return QStringLiteral("Unknown PDFium error");
    case FPDF_ERR_FILE: return QStringLiteral("File not found or could not be opened");
    case FPDF_ERR_FORMAT: return QStringLiteral("Not a PDF or corrupted file");
    case FPDF_ERR_PASSWORD: return QStringLiteral("Password required or incorrect password");
    case FPDF_ERR_SECURITY: return QStringLiteral("Unsupported security scheme");
    case FPDF_ERR_PAGE: return QStringLiteral("Page not found or content error");
    default: return QStringLiteral("PDFium error %1").arg(errorCode);
    }
}

QString PdfRuntime::lastErrorText()
{
    return lastErrorText(FPDF_GetLastError());
}

PdfDocument::PdfDocument()
    : d(std::make_unique<Impl>())
{
    PdfRuntime::initialize();
}

PdfDocument::~PdfDocument()
{
    if (d && d->document)
        FPDF_CloseDocument(d->document);
    PdfRuntime::shutdown();
}

std::unique_ptr<PdfDocument> PdfDocument::open(const QString& filePath, QString* errorMessage)
{
    auto doc = std::unique_ptr<PdfDocument>(new PdfDocument);
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("File does not exist: %1").arg(filePath);
        return nullptr;
    }
    doc->d->document = FPDF_LoadDocument(filePath.toUtf8().constData(), nullptr);
    if (!doc->d->document) {
        if (errorMessage)
            *errorMessage = PdfRuntime::lastErrorText();
        return nullptr;
    }
    doc->m_filePath = info.absoluteFilePath();
    doc->m_isNew = false;
    doc->m_modified = false;
    doc->m_outputIntent = extractOutputIntentName(doc->m_filePath);
    {
        QFile hashFile(doc->m_filePath);
        if (hashFile.open(QIODevice::ReadOnly)) {
            QCryptographicHash hash(QCryptographicHash::Sha256);
            hash.addData(&hashFile);
            doc->m_sourceHash = hash.result().toHex();
        }
    }
    logInfo(QStringLiteral("Opened PDF: %1 (%2 pages)").arg(doc->m_filePath).arg(doc->pageCount()));
    return doc;
}

std::unique_ptr<PdfDocument> PdfDocument::createBlank(const QSizeF& sizePoints, QString* errorMessage)
{
    auto doc = std::unique_ptr<PdfDocument>(new PdfDocument);
    doc->d->document = FPDF_CreateNewDocument();
    if (!doc->d->document) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not create a PDF document");
        return nullptr;
    }
    const double w = std::max(1.0, sizePoints.width());
    const double h = std::max(1.0, sizePoints.height());
    FPDF_PAGE page = FPDFPage_New(doc->d->document, 0, w, h);
    if (!page) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not create the first page");
        return nullptr;
    }
    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);
    doc->m_isNew = true;
    doc->m_modified = true;
    if (errorMessage)
        errorMessage->clear();
    logInfo(QStringLiteral("Created blank PDF: %1 x %2 pt").arg(w).arg(h));
    return doc;
}

bool PdfDocument::isValid() const
{
    return d && d->document;
}

QString PdfDocument::displayName() const
{
    if (!m_filePath.isEmpty())
        return QFileInfo(m_filePath).fileName();
    return QStringLiteral("Untitled.pdf");
}

int PdfDocument::pageCount() const
{
    if (!isValid())
        return 0;
    return std::max(0, FPDF_GetPageCount(d->document));
}

bool PdfDocument::hasPage(int index) const
{
    return index >= 0 && index < pageCount();
}

QSizeF PdfDocument::pageSize(int index) const
{
    if (!isValid() || !hasPage(index))
        return {};
    FS_SIZEF size{};
    if (!FPDF_GetPageSizeByIndexF(d->document, index, &size))
        return {};
    return QSizeF(size.width, size.height);
}

QSizeF PdfDocument::pageDisplaySize(int index) const
{
    const QSizeF size = pageSize(index);
    const int rotation = pageRotation(index);
    if (rotation == 90 || rotation == 270)
        return QSizeF(size.height(), size.width());
    return size;
}

int PdfDocument::pageRotation(int index) const
{
    if (!isValid() || !hasPage(index))
        return 0;
    FPDF_PAGE page = FPDF_LoadPage(d->document, index);
    if (!page)
        return 0;
    int rotation = FPDFPage_GetRotation(page) * 90;
    FPDF_ClosePage(page);
    return normalizedRotation(rotation);
}

QString PdfDocument::pageLabel(int index) const
{
    if (!isValid() || !hasPage(index))
        return {};
    const unsigned long len = FPDF_GetPageLabel(d->document, index, nullptr, 0);
    if (len <= 2)
        return QString::number(index + 1);
    std::vector<unsigned short> buffer(len / 2 + 1);
    FPDF_GetPageLabel(d->document, index, buffer.data(),
                      static_cast<unsigned long>(buffer.size() * sizeof(unsigned short)));
    const QString label = QString::fromUtf16(buffer.data());
    return label.isEmpty() ? QString::number(index + 1) : label;
}

double PdfDocument::pageWidth(int index) const
{
    return pageSize(index).width();
}

double PdfDocument::pageHeight(int index) const
{
    return pageSize(index).height();
}

PageBoxes PdfDocument::pageBoxes(int index) const
{
    PageBoxes boxes;
    if (!isValid() || !hasPage(index))
        return boxes;

    FPDF_PAGE page = FPDF_LoadPage(d->document, index);
    if (!page)
        return boxes;

    float ml = 0.f, mb = 0.f, mr = 0.f, mt = 0.f;
    if (FPDFPage_GetMediaBox(page, &ml, &mb, &mr, &mt)) {
        boxes.media = boxToTopLeft(ml, mb, mr, mt, std::max(mb, mt));
    } else {
        const QSizeF size = pageSize(index);
        boxes.media = QRectF(0.0, 0.0, size.width(), size.height());
    }
    const double referenceTop = boxes.media.top() + boxes.media.height();

    float l = 0.f, b = 0.f, r = 0.f, t = 0.f;
    if (FPDFPage_GetCropBox(page, &l, &b, &r, &t)) {
        boxes.crop = boxToTopLeft(l, b, r, t, referenceTop);
        boxes.hasCrop = true;
    } else {
        boxes.crop = boxes.media;
    }

    // Per PDF spec: Bleed/Trim/Art default to Crop when absent.
    if (FPDFPage_GetBleedBox(page, &l, &b, &r, &t)) {
        boxes.bleed = boxToTopLeft(l, b, r, t, referenceTop);
        boxes.hasBleed = true;
    } else {
        boxes.bleed = boxes.crop;
    }
    if (FPDFPage_GetTrimBox(page, &l, &b, &r, &t)) {
        boxes.trim = boxToTopLeft(l, b, r, t, referenceTop);
        boxes.hasTrim = true;
    } else {
        boxes.trim = boxes.crop;
    }
    if (FPDFPage_GetArtBox(page, &l, &b, &r, &t)) {
        boxes.art = boxToTopLeft(l, b, r, t, referenceTop);
        boxes.hasArt = true;
    } else {
        boxes.art = boxes.crop;
    }

    FPDF_ClosePage(page);
    return boxes;
}

QVector<PageObjectSummary> PdfDocument::pageObjects(int index) const
{
    QVector<PageObjectSummary> result;
    if (!isValid() || !hasPage(index))
        return result;

    const PageBoxes boxes = pageBoxes(index);
    const double referenceTop = boxes.media.top() + boxes.media.height();

    FPDF_PAGE page = FPDF_LoadPage(d->document, index);
    if (!page)
        return result;

    const int count = FPDFPage_CountObjects(page);
    result.reserve(std::max(0, count));
    for (int i = 0; i < count; ++i) {
        FPDF_PAGEOBJECT object = FPDFPage_GetObject(page, i);
        if (!object)
            continue;

        PageObjectSummary summary;
        summary.kind = kindFromPdfium(FPDFPageObj_GetType(object));

        float l = 0.f, b = 0.f, r = 0.f, t = 0.f;
        if (FPDFPageObj_GetBounds(object, &l, &b, &r, &t))
            summary.bounds = boxToTopLeft(l, b, r, t, referenceTop);

        result.append(summary);
    }

    FPDF_ClosePage(page);
    return result;
}

QString PdfDocument::title() const { return readMetaText(d->document, "Title"); }
QString PdfDocument::author() const { return readMetaText(d->document, "Author"); }
QString PdfDocument::subject() const { return readMetaText(d->document, "Subject"); }
QString PdfDocument::keywords() const { return readMetaText(d->document, "Keywords"); }
QString PdfDocument::creator() const { return readMetaText(d->document, "Creator"); }
QString PdfDocument::producer() const { return readMetaText(d->document, "Producer"); }
QString PdfDocument::creationDate() const { return readMetaText(d->document, "CreationDate"); }
QString PdfDocument::modificationDate() const { return readMetaText(d->document, "ModDate"); }

int PdfDocument::fileVersion() const
{
    int version = 0;
    if (!isValid() || !FPDF_GetFileVersion(d->document, &version))
        return 0;
    return version;
}

bool PdfDocument::isEncrypted() const
{
    if (!isValid())
        return false;
    return FPDF_GetSecurityHandlerRevision(d->document) >= 0;
}

quint32 PdfDocument::permissions() const
{
    if (!isValid())
        return 0;
    return static_cast<quint32>(FPDF_GetDocPermissions(d->document));
}

PageRenderResult PdfDocument::renderPage(int index, double scale, int viewRotation,
                                         bool renderAnnotations) const
{
    PageRenderResult result;
    if (!isValid() || !hasPage(index))
        return result;

    FPDF_PAGE page = FPDF_LoadPage(d->document, index);
    if (!page)
        return result;

    const double rawWidth = FPDF_GetPageWidthF(page);
    const double rawHeight = FPDF_GetPageHeightF(page);
    result.pageSizePoints = QSizeF(rawWidth, rawHeight);
    result.pageRotation = normalizedRotation(FPDFPage_GetRotation(page) * 90);

    const double requestedScale = std::clamp(scale, 0.01, 32.0);
    double effectiveScale = requestedScale;
    // Safety cap: avoid allocating multi-gigabyte bitmaps on extreme zoom.
    constexpr double kMaxPixels = 64.0 * 1024.0 * 1024.0;
    const double pixels = rawWidth * rawHeight * effectiveScale * effectiveScale;
    if (pixels > kMaxPixels) {
        effectiveScale = std::sqrt(kMaxPixels / (rawWidth * rawHeight));
        result.scaleReduced = true;
    }

    const int width = std::max(1, static_cast<int>(std::ceil(rawWidth * effectiveScale)));
    const int height = std::max(1, static_cast<int>(std::ceil(rawHeight * effectiveScale)));

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    if (!bitmap) {
        FPDF_ClosePage(page);
        return result;
    }
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);

    int flags = FPDF_ANNOT | FPDF_NO_NATIVETEXT;
    if (!renderAnnotations)
        flags = FPDF_NO_NATIVETEXT;
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, flags);

    const auto* buffer = static_cast<const uchar*>(FPDFBitmap_GetBuffer(bitmap));
    const int stride = FPDFBitmap_GetStride(bitmap);
    QImage image(buffer, width, height, stride, QImage::Format_ARGB32);
    result.image = image.copy();

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);

    const int totalRotation = normalizedRotation(result.pageRotation + viewRotation);
    if (totalRotation != 0) {
        QTransform transform;
        transform.rotate(totalRotation);
        result.image = result.image.transformed(transform, Qt::SmoothTransformation);
    }
    result.effectiveScale = effectiveScale;
    return result;
}

QImage PdfDocument::renderThumbnail(int index, int maxPixels, int viewRotation) const
{
    const QSizeF size = pageSize(index);
    const int rotation = normalizedRotation(pageRotation(index) + viewRotation);
    const bool swap = rotation == 90 || rotation == 270;
    const double w = swap ? size.height() : size.width();
    const double h = swap ? size.width() : size.height();
    if (w <= 0.0 || h <= 0.0)
        return {};
    const double scale = std::max(1.0, static_cast<double>(maxPixels)) / std::max(w, h);
    return renderPage(index, scale, viewRotation, false).image;
}

QString PdfDocument::extractText(int index) const
{
    if (!isValid() || !hasPage(index))
        return {};
    FPDF_PAGE page = FPDF_LoadPage(d->document, index);
    if (!page)
        return {};
    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    QString text;
    if (textPage) {
        const int count = FPDFText_CountChars(textPage);
        if (count > 0) {
            std::vector<unsigned short> buffer(static_cast<size_t>(count) + 1);
            const int written = FPDFText_GetText(textPage, 0, count, buffer.data());
            if (written > 0)
                text = QString::fromUtf16(buffer.data(), written);
        }
        FPDFText_ClosePage(textPage);
    }
    FPDF_ClosePage(page);
    return text;
}

QString pageObjectKindName(PageObjectKind kind)
{
    switch (kind) {
    case PageObjectKind::Text: return QStringLiteral("Text");
    case PageObjectKind::Path: return QStringLiteral("Path");
    case PageObjectKind::Image: return QStringLiteral("Image");
    case PageObjectKind::Shading: return QStringLiteral("Shading");
    case PageObjectKind::Form: return QStringLiteral("Form");
    case PageObjectKind::Unknown: return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

bool PdfDocument::saveCopyPreserving(const QString& filePath, QString* errorMessage) const
{
    if (filePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No target file name");
        return false;
    }
    if (m_isNew || m_filePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("The document has no source file to preserve");
        return false;
    }
    if (QFileInfo(filePath).absoluteFilePath() == QFileInfo(m_filePath).absoluteFilePath()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Refusing to overwrite the source document");
        return false;
    }

    QFile source(m_filePath);
    if (!source.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot read the source document: %1")
                                .arg(source.errorString());
        return false;
    }

    QSaveFile target(filePath);
    if (!target.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot write %1: %2").arg(filePath, target.errorString());
        return false;
    }

    const qint64 total = source.size();
    qint64 written = 0;
    while (!source.atEnd()) {
        const QByteArray chunk = source.read(1 << 20);
        if (chunk.isEmpty())
            break;
        if (target.write(chunk) != chunk.size()) {
            target.cancelWriting();
            if (errorMessage)
                *errorMessage = QStringLiteral("Write failed for %1").arg(filePath);
            return false;
        }
        written += chunk.size();
    }

    if (written != total) {
        target.cancelWriting();
        if (errorMessage)
            *errorMessage = QStringLiteral("Short write: %1 of %2 bytes").arg(written).arg(total);
        return false;
    }
    if (!target.commit()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot commit %1: %2").arg(filePath, target.errorString());
        return false;
    }
    return true;
}

bool PdfDocument::save(const QString& filePath, QString* errorMessage)
{
    if (!isValid()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No document is loaded");
        return false;
    }
    if (filePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No target file name");
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot write %1: %2").arg(filePath, file.errorString());
        return false;
    }

    SaveFileWriter writer(file);
    const FPDF_BOOL ok = FPDF_SaveAsCopy(d->document, &writer, FPDF_NO_INCREMENTAL);
    if (!ok) {
        file.cancelWriting();
        if (errorMessage)
            *errorMessage = QStringLiteral("PDFium could not save the document");
        return false;
    }
    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot finalize %1: %2").arg(filePath, file.errorString());
        return false;
    }

    m_filePath = QFileInfo(filePath).absoluteFilePath();
    m_isNew = false;
    m_modified = false;
    logInfo(QStringLiteral("Saved PDF: %1").arg(m_filePath));
    return true;
}

bool PdfDocument::setPageRotation(int index, int degrees)
{
    if (!hasPage(index))
        return false;
    FPDF_PAGE page = FPDF_LoadPage(d->document, index);
    if (!page)
        return false;
    const int normalized = normalizedRotation(degrees);
    FPDFPage_SetRotation(page, normalized / 90);
    FPDF_ClosePage(page);
    m_modified = true;
    return true;
}

bool PdfDocument::deletePage(int index)
{
    if (!hasPage(index) || pageCount() <= 1)
        return false;
    FPDFPage_Delete(d->document, index);
    m_modified = true;
    return true;
}

bool PdfDocument::movePage(int from, int to)
{
    if (!hasPage(from) || !hasPage(to))
        return false;
    if (from == to)
        return true;
    const int indices[1] = {from};
    const FPDF_BOOL ok = FPDF_MovePages(d->document, indices, 1, to);
    m_modified = true;
    return ok != 0;
}

void* PdfDocument::nativeHandle() const
{
    return d ? d->document : nullptr;
}

} // namespace artplus::pdf
