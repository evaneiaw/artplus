// Milestone 22 import test: PDF -> internal snapshot -> preserved copy.
//
// Verifies:
//   * the snapshot mirrors page count, page size and Media Box;
//   * page object summaries are captured (text/path/image);
//   * missing Trim/Bleed/Art boxes default to the Crop Box;
//   * Save-As-Copy produces a byte-identical file and can be reopened;
//   * overwriting the source document is refused.
//
// Usage: artplus_import_test <file.pdf>

#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfSnapshot.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <cmath>
#include <cstdio>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* description)
{
    ++g_checks;
    if (condition) {
        std::printf("  [PASS] %s\n", description);
    } else {
        ++g_failures;
        std::printf("  [FAIL] %s\n", description);
    }
}

QByteArray sha256Of(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    return hash.result().toHex();
}

bool nearly(double a, double b)
{
    return std::abs(a - b) < 0.5;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    std::printf("ArtPlus import (M22) test\n");
    std::printf("=========================\n");

    if (argc < 2) {
        std::printf("usage: artplus_import_test <file.pdf>\n");
        return 2;
    }

    const QString path = QString::fromLocal8Bit(argv[1]);
    artplus::pdf::PdfRuntime::initialize();

    int result = 1;
    {
        QString error;
        auto doc = artplus::pdf::PdfDocument::open(path, &error);
        check(doc != nullptr, "document opens");
        if (!doc) {
            std::printf("  error: %s\n", qPrintable(error));
        } else {
            const auto snapshot = artplus::pdf::PdfSnapshot::capture(*doc);

            std::printf("\nSnapshot:\n");
            check(snapshot.pageCount() == doc->pageCount(), "snapshot page count matches");
            check(!snapshot.sourceSha256.isEmpty(), "source hash captured");
            std::printf("  pages          : %d\n", snapshot.pageCount());
            std::printf("  objects        : %d\n", snapshot.objectCount());
            std::printf("  text/path/img  : %d / %d / %d\n",
                        snapshot.countOf(artplus::pdf::PageObjectKind::Text),
                        snapshot.countOf(artplus::pdf::PageObjectKind::Path),
                        snapshot.countOf(artplus::pdf::PageObjectKind::Image));
            std::printf("  form/shading   : %d / %d\n",
                        snapshot.countOf(artplus::pdf::PageObjectKind::Form),
                        snapshot.countOf(artplus::pdf::PageObjectKind::Shading));

            const auto* first = snapshot.page(0);
            check(first != nullptr, "first page present");
            if (first) {
                const QSizeF size = doc->pageSize(0);
                check(nearly(first->boxes.media.width(), size.width()) &&
                          nearly(first->boxes.media.height(), size.height()),
                      "Media Box matches page size");
                check(nearly(first->boxes.media.left(), 0.0) &&
                          nearly(first->boxes.media.top(), 0.0),
                      "Media Box origin is the page origin");
                std::printf("  media box      : %.2f x %.2f pt\n",
                            first->boxes.media.width(), first->boxes.media.height());

                if (!first->boxes.hasTrim)
                    check(first->boxes.trim == first->boxes.crop,
                          "Trim Box defaults to Crop Box");
                if (!first->boxes.hasBleed)
                    check(first->boxes.bleed == first->boxes.crop,
                          "Bleed Box defaults to Crop Box");
                if (!first->boxes.hasArt)
                    check(first->boxes.art == first->boxes.crop,
                          "Art Box defaults to Crop Box");
            }

            std::printf("\nObjects:\n");
            check(snapshot.objectCount() > 0, "page objects were imported");
            int classified = 0;
            for (artplus::pdf::PageObjectKind kind :
                 {artplus::pdf::PageObjectKind::Text, artplus::pdf::PageObjectKind::Path,
                  artplus::pdf::PageObjectKind::Image, artplus::pdf::PageObjectKind::Shading,
                  artplus::pdf::PageObjectKind::Form})
                classified += snapshot.countOf(kind);
            check(classified > 0, "page objects were classified by kind");

            std::printf("\nPreserved copy:\n");
            // %TEMP% is unreliable in this environment; use a folder in cwd.
            const QString workDir =
                QDir::current().filePath(QStringLiteral("artplus_m22_import_out"));
            QDir().mkpath(workDir);
            const bool dirOk = QDir(workDir).exists();
            check(dirOk, "working directory created");
            if (dirOk) {
                const QString copyPath =
                    QDir(workDir).filePath(QStringLiteral("copy.pdf"));
                QString copyError;
                const bool saved = doc->saveCopyPreserving(copyPath, &copyError);
                check(saved, "saveCopyPreserving succeeded");
                if (!saved) {
                    std::printf("  error: %s\n", qPrintable(copyError));
                } else {
                    const QByteArray sourceHash = sha256Of(path);
                    const QByteArray copyHash = sha256Of(copyPath);
                    check(!copyHash.isEmpty(), "copy is readable");
                    check(copyHash == sourceHash, "copy is byte-identical to the source");

                    auto reopened = artplus::pdf::PdfDocument::open(copyPath, &copyError);
                    check(reopened != nullptr, "preserved copy reopens");
                    if (reopened)
                        check(reopened->pageCount() == doc->pageCount(),
                              "reopened copy keeps the page count");
                }

                QString refuseError;
                check(!doc->saveCopyPreserving(path, &refuseError),
                      "overwriting the source is refused");

                QDir(workDir).removeRecursively();
            }

            result = g_failures == 0 ? 0 : 1;
        }
    }

    artplus::pdf::PdfRuntime::shutdown();

    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return result;
}
