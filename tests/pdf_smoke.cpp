// Headless smoke test for the ArtPlus PDF engine.
//
// Verifies, against a real PDF file:
//   * the document opens and reports page count / dimensions / metadata;
//   * page 0 and the last page render to non-empty images;
//   * the source file is byte-identical before and after opening/rendering
//     (the viewer must never modify the source document);
//   * invalid input is rejected.
//
// Usage: artplus_pdf_smoke <file.pdf> [output-directory]

#include "artplus/pdf/PdfDocument.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QString>
#include <QTemporaryFile>

#include <cmath>
#include <cstdio>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const QString& description)
{
    ++g_checks;
    if (condition) {
        std::printf("  [PASS] %s\n", qPrintable(description));
    } else {
        ++g_failures;
        std::printf("  [FAIL] %s\n", qPrintable(description));
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

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    std::printf("ArtPlus PDF engine smoke test\n");
    std::printf("=============================\n");

    if (argc < 2) {
        std::printf("usage: artplus_pdf_smoke <file.pdf> [output-directory]\n");
        return 2;
    }

    const QString pdfPath = QString::fromLocal8Bit(argv[1]);
    const QString outDir = argc >= 3 ? QString::fromLocal8Bit(argv[2]) : QString();

    // --- Preservation: hash before opening -----------------------------------
    const QByteArray hashBefore = sha256Of(pdfPath);
    check(!hashBefore.isEmpty(), QStringLiteral("source file is readable"));
    if (hashBefore.isEmpty())
        return 1;

    // --- Open -----------------------------------------------------------------
    std::printf("\nOpen:\n");
    QString error;
    auto doc = artplus::pdf::PdfDocument::open(pdfPath, &error);
    check(doc != nullptr, QStringLiteral("PdfDocument::open returned a document"));
    if (!doc) {
        std::printf("  error: %s\n", qPrintable(error));
        return 1;
    }
    check(doc->isValid(), QStringLiteral("document is valid"));
    check(doc->pageCount() > 0, QStringLiteral("page count is greater than zero"));
    check(!doc->isEncrypted(), QStringLiteral("document is not encrypted"));

    std::printf("  pages          : %d\n", doc->pageCount());
    std::printf("  file version   : %d\n", doc->fileVersion());
    std::printf("  title          : %s\n", qPrintable(doc->title()));
    std::printf("  author         : %s\n", qPrintable(doc->author()));

    const QSizeF size0 = doc->pageSize(0);
    std::printf("  page 1 size    : %.2f x %.2f pt\n", size0.width(), size0.height());
    check(size0.width() > 0.0 && size0.height() > 0.0,
          QStringLiteral("page 1 has positive dimensions"));

    // --- Render ---------------------------------------------------------------
    std::printf("\nRender:\n");
    const double scale = 1.5; // pixels per point
    const artplus::pdf::PageRenderResult first = doc->renderPage(0, scale);
    check(!first.image.isNull(), QStringLiteral("page 1 rendered a non-null image"));
    check(first.image.width() > 0 && first.image.height() > 0,
          QStringLiteral("page 1 image has a positive size"));
    std::printf("  page 1 image   : %d x %d px\n", first.image.width(), first.image.height());

    const int lastIndex = doc->pageCount() - 1;
    const artplus::pdf::PageRenderResult last = doc->renderPage(lastIndex, scale);
    check(!last.image.isNull(), QStringLiteral("last page rendered a non-null image"));

    const QImage thumb = doc->renderThumbnail(0, 120);
    check(!thumb.isNull(), QStringLiteral("thumbnail render succeeded"));

    // --- View rotation rendering ---------------------------------------------
    // The viewer rotates the rendered page (PDFium renders, then the image is
    // rotated). These checks prove the rotation reaches the actual pixels and
    // changes only the presentation: dimensions swap for quarter turns, the
    // full turn comes back to the original size, and the file on disk is
    // untouched.
    std::printf("\nView rotation rendering:\n");
    const artplus::pdf::PageRenderResult r0 = doc->renderPage(0, scale, 0);
    const artplus::pdf::PageRenderResult r90 = doc->renderPage(0, scale, 90);
    const artplus::pdf::PageRenderResult r180 = doc->renderPage(0, scale, 180);
    const artplus::pdf::PageRenderResult r270 = doc->renderPage(0, scale, 270);
    const artplus::pdf::PageRenderResult r360 = doc->renderPage(0, scale, 360);

    check(!r90.image.isNull() && !r180.image.isNull() && !r270.image.isNull(),
          QStringLiteral("rotated pages render non-null images"));
    std::printf("  image sizes    : 0=%dx%d 90=%dx%d 180=%dx%d 270=%dx%d\n",
                r0.image.width(), r0.image.height(), r90.image.width(), r90.image.height(),
                r180.image.width(), r180.image.height(), r270.image.width(),
                r270.image.height());

    const int dimTolerance = 2; // rounding when the image is transformed
    check(std::abs(r90.image.width() - r0.image.height()) <= dimTolerance &&
              std::abs(r90.image.height() - r0.image.width()) <= dimTolerance,
          QStringLiteral("a 90-degree rotation swaps the rendered image dimensions"));
    check(std::abs(r270.image.width() - r0.image.height()) <= dimTolerance &&
              std::abs(r270.image.height() - r0.image.width()) <= dimTolerance,
          QStringLiteral("a 270-degree rotation swaps the rendered image dimensions"));
    check(std::abs(r180.image.width() - r0.image.width()) <= dimTolerance &&
              std::abs(r180.image.height() - r0.image.height()) <= dimTolerance,
          QStringLiteral("a 180-degree rotation keeps the rendered image dimensions"));
    check(std::abs(r360.image.width() - r0.image.width()) <= dimTolerance &&
              std::abs(r360.image.height() - r0.image.height()) <= dimTolerance,
          QStringLiteral("a full turn restores the rendered image dimensions"));

    // Content must actually change, otherwise only the size would differ.
    check(r0.image != r90.image, QStringLiteral("90-degree output differs from the unrotated page"));
    check(r0.image != r180.image,
          QStringLiteral("180-degree output differs from the unrotated page"));
    check(r0.image != r270.image,
          QStringLiteral("270-degree output differs from the unrotated page"));

    // --- Invalid input --------------------------------------------------------
    std::printf("\nInvalid input:\n");
    const QString badPath = QDir::current().filePath(QStringLiteral("artplus_invalid_input.pdf"));
    bool wroteBad = false;
    QString badWriteError;
    {
        QFile badFile(badPath);
        if (badFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            badFile.write("this is definitely not a PDF file");
            badFile.close();
            wroteBad = true;
        } else {
            badWriteError = badFile.errorString();
        }
    }
    if (wroteBad) {
        QString badError;
        auto bad = artplus::pdf::PdfDocument::open(badPath, &badError);
        check(!bad || !bad->isValid(),
              QStringLiteral("non-PDF file is rejected"));
        std::printf("  rejection msg  : %s\n", qPrintable(badError));
        QFile::remove(badPath);
    } else {
        check(false, QStringLiteral("could not create temporary invalid file: %1 (%2)")
                         .arg(badPath, badWriteError));
    }

    // --- Preservation: hash after --------------------------------------------
    std::printf("\nPreservation:\n");
    const QByteArray hashAfter = sha256Of(pdfPath);
    check(hashAfter == hashBefore,
          QStringLiteral("source file bytes are unchanged after open/render"));
    std::printf("  sha256         : %s\n", hashBefore.constData());

    // --- Optional PNG output --------------------------------------------------
    if (!outDir.isEmpty()) {
        QDir().mkpath(outDir);
        const QString p1 = QDir(outDir).filePath(QStringLiteral("render_page01.png"));
        const QString pl = QDir(outDir).filePath(
            QStringLiteral("render_page%1.png").arg(lastIndex + 1, 2, 10, QLatin1Char('0')));
        const bool ok1 = first.image.save(p1, "PNG");
        const bool okl = last.image.save(pl, "PNG");
        check(ok1, QStringLiteral("saved page 1 PNG"));
        check(okl, QStringLiteral("saved last page PNG"));
        std::printf("  wrote          : %s\n", qPrintable(p1));
        std::printf("  wrote          : %s\n", qPrintable(pl));

        // One PNG per view rotation, so the rotation can be inspected by eye.
        const int rotations[4] = {0, 90, 180, 270};
        const QImage* images[4] = {&r0.image, &r90.image, &r180.image, &r270.image};
        for (int i = 0; i < 4; ++i) {
            const QString path = QDir(outDir).filePath(
                QStringLiteral("render_page01_rot%1.png").arg(rotations[i], 3, 10,
                                                             QLatin1Char('0')));
            const bool ok = images[i]->save(path, "PNG");
            check(ok, QStringLiteral("saved rotation PNG %1 degrees").arg(rotations[i]));
            std::printf("  wrote          : %s\n", qPrintable(path));
        }
    }

    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
