#pragma once

#include "artplus/pdf/PdfDocument.h"

#include <QByteArray>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QVector>

namespace artplus::pdf {

// Immutable representation of a PDF as imported into ArtPlus.
//
// Milestone 22 keeps the source file untouched: the snapshot is read-only
// metadata/geometry captured through PDFium, and saving writes a byte-identical
// copy of the original. Editing that rewrites the PDF comes with the editable
// document model.
struct SnapshotObject {
    PageObjectKind kind = PageObjectKind::Unknown;
    QRectF bounds; // points, top-left origin
};

struct SnapshotPage {
    int index = 0;
    QSizeF sizePoints;
    int rotation = 0;
    PageBoxes boxes;
    QVector<SnapshotObject> objects; // content order (back to front)

    int countOf(PageObjectKind kind) const;
};

struct PdfSnapshot {
    QString path;
    QString title;
    QString author;
    QString subject;
    QString creator;
    QString producer;
    QString outputIntent;
    int pdfVersion = 0;
    bool encrypted = false;
    QByteArray sourceSha256;
    QVector<SnapshotPage> pages;

    int pageCount() const { return static_cast<int>(pages.size()); }
    int objectCount() const;
    int countOf(PageObjectKind kind) const;

    const SnapshotPage* page(int index) const;

    static PdfSnapshot capture(const PdfDocument& document);
};

} // namespace artplus::pdf
