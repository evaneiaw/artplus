#include "artplus/pdf/PdfSnapshot.h"

#include <algorithm>

namespace artplus::pdf {

int SnapshotPage::countOf(PageObjectKind kind) const
{
    int count = 0;
    for (const SnapshotObject& object : objects) {
        if (object.kind == kind)
            ++count;
    }
    return count;
}

int PdfSnapshot::objectCount() const
{
    int count = 0;
    for (const SnapshotPage& p : pages)
        count += static_cast<int>(p.objects.size());
    return count;
}

int PdfSnapshot::countOf(PageObjectKind kind) const
{
    int count = 0;
    for (const SnapshotPage& p : pages)
        count += p.countOf(kind);
    return count;
}

const SnapshotPage* PdfSnapshot::page(int index) const
{
    const int count = pageCount();
    if (index < 0 || index >= count)
        return nullptr;
    return &pages.at(index);
}

PdfSnapshot PdfSnapshot::capture(const PdfDocument& document)
{
    PdfSnapshot snapshot;
    if (!document.isValid())
        return snapshot;

    snapshot.path = document.filePath();
    snapshot.title = document.title();
    snapshot.author = document.author();
    snapshot.subject = document.subject();
    snapshot.creator = document.creator();
    snapshot.producer = document.producer();
    snapshot.outputIntent = document.outputIntentName();
    snapshot.pdfVersion = document.fileVersion();
    snapshot.encrypted = document.isEncrypted();
    snapshot.sourceSha256 = document.sourceSha256();

    const int pageCount = document.pageCount();
    snapshot.pages.reserve(pageCount);
    for (int index = 0; index < pageCount; ++index) {
        SnapshotPage page;
        page.index = index;
        page.sizePoints = document.pageSize(index);
        page.rotation = document.pageRotation(index);
        page.boxes = document.pageBoxes(index);

        const QVector<PageObjectSummary> objects = document.pageObjects(index);
        page.objects.reserve(objects.size());
        for (const PageObjectSummary& summary : objects) {
            SnapshotObject object;
            object.kind = summary.kind;
            object.bounds = summary.bounds;
            page.objects.append(object);
        }

        snapshot.pages.append(page);
    }

    return snapshot;
}

} // namespace artplus::pdf
