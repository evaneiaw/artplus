#pragma once

#include "artplus/core/Units.h"

#include <QByteArray>
#include <QImage>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QVector>
#include <memory>

namespace artplus::pdf {

class PdfRuntime
{
public:
    static void initialize();
    static void shutdown();
    static QString lastErrorText(unsigned long errorCode);
    static QString lastErrorText();
};

struct PageRenderResult {
    QImage image;
    QSizeF pageSizePoints; // unrotated
    int pageRotation = 0;  // intrinsic /Rotate of the page
    double effectiveScale = 0.0;
    bool scaleReduced = false;
};

// Page boxes in PDF points, top-left origin (y grows downwards), matching the
// rest of the application.
struct PageBoxes {
    QRectF media;
    QRectF crop;
    QRectF trim;
    QRectF bleed;
    QRectF art;
    bool hasCrop = false;
    bool hasTrim = false;
    bool hasBleed = false;
    bool hasArt = false;
};

enum class PageObjectKind { Text, Path, Image, Shading, Form, Unknown };

QString pageObjectKindName(PageObjectKind kind);

struct PageObjectSummary {
    PageObjectKind kind = PageObjectKind::Unknown;
    QRectF bounds; // PDF points, top-left origin
};

class PdfDocument
{
public:
    static std::unique_ptr<PdfDocument> open(const QString& filePath, QString* errorMessage = nullptr);
    static std::unique_ptr<PdfDocument> createBlank(const QSizeF& sizePoints,
                                                    QString* errorMessage = nullptr);

    ~PdfDocument();
    PdfDocument(const PdfDocument&) = delete;
    PdfDocument& operator=(const PdfDocument&) = delete;

    bool isValid() const;
    bool isNew() const { return m_isNew; }
    QString filePath() const { return m_filePath; }
    QString displayName() const;
    int pageCount() const;
    bool hasPage(int index) const;

    QSizeF pageSize(int index) const;
    QSizeF pageDisplaySize(int index) const; // after intrinsic rotation
    int pageRotation(int index) const;
    QString pageLabel(int index) const;
    double pageWidth(int index) const;
    double pageHeight(int index) const;

    // Page boxes (Media/Crop/Trim/Bleed/Art) in points, top-left origin.
    PageBoxes pageBoxes(int index) const;

    // Top-level page objects, in content order (back to front).
    QVector<PageObjectSummary> pageObjects(int index) const;

    QString title() const;
    QString author() const;
    QString subject() const;
    QString keywords() const;
    QString creator() const;
    QString producer() const;
    QString creationDate() const;
    QString modificationDate() const;
    QString outputIntentName() const { return m_outputIntent; }
    int fileVersion() const; // 14 for PDF 1.4... 20 for 2.0
    bool isEncrypted() const;
    quint32 permissions() const;

    // SHA-256 of the file as it was opened (empty for created documents).
    QByteArray sourceSha256() const { return m_sourceHash; }

    // scale is pixels per PDF point. viewRotation is added to the page's own
    // /Rotate value. The result is always upright for the requested rotation.
    PageRenderResult renderPage(int index, double scale, int viewRotation = 0,
                                bool renderAnnotations = true) const;
    QImage renderThumbnail(int index, int maxPixels = 160, int viewRotation = 0) const;

    QString extractText(int index) const;

    bool save(const QString& filePath, QString* errorMessage = nullptr);

    // Milestone 22 preservation path: writes a byte-identical copy of the
    // source document. Editing that rewrites the PDF arrives with the
    // document model.
    bool saveCopyPreserving(const QString& filePath, QString* errorMessage = nullptr) const;

    bool setPageRotation(int index, int degrees);
    bool deletePage(int index);
    bool movePage(int from, int to);

    bool isModified() const { return m_modified; }
    void setModified(bool modified) { m_modified = modified; }

    // Implementation handle for modules that need low-level PDFium access.
    void* nativeHandle() const;

private:
    PdfDocument();
    struct Impl;
    std::unique_ptr<Impl> d;
    QString m_filePath;
    bool m_isNew = false;
    bool m_modified = false;
    QString m_outputIntent;
    QByteArray m_sourceHash;
};

} // namespace artplus::pdf
