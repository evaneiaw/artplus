// GUI integration test: exercises the real MainWindow open/navigate/zoom/fit
// wiring against a real PDF.
//
// Usage: artplus_mainwindow_test <file.pdf>

#include "artplus/pdf/PdfDocument.h"
#include "artplus/ui/MainWindow.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QMetaObject>
#include <QSizeF>
#include <QString>

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

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    std::printf("ArtPlus MainWindow integration test\n");
    std::printf("===================================\n");

    if (argc < 2) {
        std::printf("usage: artplus_mainwindow_test <file.pdf>\n");
        return 2;
    }

    artplus::pdf::PdfRuntime::initialize();

    int exitCode = 0;
    {
        // The window owns the PDF document; it must be destroyed before the
        // PDFium runtime is shut down.
        artplus::ui::MainWindow window;
        window.resize(1100, 800);

        std::printf("\nInitial state:\n");
        check(!window.hasDocument(), "no document at startup");
        check(window.pageCount() == 0, "page count is zero at startup");

        std::printf("\nOpen:\n");
        const QString path = QString::fromLocal8Bit(argv[1]);
        const bool opened = window.openDocument(path);
        check(opened, "openDocument succeeded");

        if (!opened) {
            exitCode = 1;
        } else {
            check(window.hasDocument(), "window reports a document");
            check(window.pageCount() > 0, "window reports a positive page count");
            check(window.currentPageIndex() == 0, "opens on the first page");
            check(window.zoom() > 0.0, "a fit zoom was applied on open");
            std::printf("  pages          : %d\n", window.pageCount());
            std::printf("  initial zoom   : %.4f\n", window.zoom());

            // Captured now so the rotation block can prove the PDF is untouched.
            const QByteArray hashBeforeRotation = sha256Of(path);
            check(!hashBeforeRotation.isEmpty(), "the opened PDF can be hashed");

            std::printf("\nNavigation:\n");
            const int total = window.pageCount();
            const bool invokedNext = QMetaObject::invokeMethod(&window, "nextPage");
            check(invokedNext, "nextPage slot is invokable");
            if (total > 1)
                check(window.currentPageIndex() == 1, "nextPage advances to page 2");

            const bool invokedPrev = QMetaObject::invokeMethod(&window, "previousPage");
            check(invokedPrev, "previousPage slot is invokable");
            check(window.currentPageIndex() == 0, "previousPage returns to page 1");

            const bool invokedGoto = QMetaObject::invokeMethod(&window, "goToPage", Q_ARG(int, total));
            check(invokedGoto, "goToPage slot is invokable");
            check(window.currentPageIndex() == total - 1, "goToPage(last) selects the last page");

            std::printf("\nZoom / fit:\n");
            QMetaObject::invokeMethod(&window, "actualSize");
            check(std::abs(window.zoom() - 96.0 / 72.0) < 1e-6, "actualSize applies 96/72");

            const double beforeZoomIn = window.zoom();
            QMetaObject::invokeMethod(&window, "zoomIn");
            check(window.zoom() > beforeZoomIn, "zoomIn increases zoom");

            const double beforeZoomOut = window.zoom();
            QMetaObject::invokeMethod(&window, "zoomOut");
            check(window.zoom() < beforeZoomOut, "zoomOut decreases zoom");

            QMetaObject::invokeMethod(&window, "fitPage");
            check(window.zoom() > 0.0, "fitPage keeps a positive zoom");

            QMetaObject::invokeMethod(&window, "fitWidth");
            check(window.zoom() > 0.0, "fitWidth keeps a positive zoom");

            std::printf("\nRotation:\n");
            // Baseline must be taken BEFORE rotating, otherwise every later
            // comparison is against an already-rotated box.
            const QSizeF portrait = window.displayedPageSizeOf(0);
            check(portrait.width() > 0.0 && portrait.height() > 0.0,
                  "the unrotated page box has a positive size");

            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            check(window.zoom() > 0.0, "rotation keeps a positive zoom");
            check(window.userRotation() == 90, "one clockwise turn reports 90 degrees");

            const QSizeF quarter = window.displayedPageSizeOf(0);
            check(std::abs(quarter.width() - portrait.height()) < 1e-6 &&
                      std::abs(quarter.height() - portrait.width()) < 1e-6,
                  "a quarter turn swaps the displayed page box dimensions");

            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            check(window.userRotation() == 180, "two clockwise turns report 180 degrees");
            const QSizeF half = window.displayedPageSizeOf(0);
            check(std::abs(half.width() - portrait.width()) < 1e-6 &&
                      std::abs(half.height() - portrait.height()) < 1e-6,
                  "a half turn keeps the original page box dimensions");
            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            check(window.userRotation() == 270, "three clockwise turns report 270 degrees");
            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            check(window.userRotation() == 0, "four clockwise turns restore 0 degrees");

            const QSizeF restored = window.displayedPageSizeOf(0);
            check(std::abs(restored.width() - portrait.width()) < 1e-6 &&
                      std::abs(restored.height() - portrait.height()) < 1e-6,
                  "a full turn restores the original displayed page box");

            // Coordinate mapping between PDF page space and displayed space.
            // A PDF point must map onto the same physical spot of the page
            // whatever the view rotation, and the inverse must undo it.
            // NOTE: the page sizes in this fixture differ per page, so the
            // reference size must come from the *current* page.
            std::printf("\nCoordinate mapping under rotation:\n");
            const int mappedPage = window.currentPageIndex();
            const QSizeF mappedPortrait = window.displayedPageSizeOf(mappedPage);
            check(window.userRotation() == 0, "mapping starts from the unrotated view");

            const QRectF probeRect(120.0, 300.0, 60.0, 40.0);
            for (int turn = 0; turn < 4; ++turn) {
                const QRectF displayed = window.pdfRectToDisplayed(probeRect);
                // Rotation moves a rect's corners around, so compare the centre
                // (which is invariant) rather than the top-left corner.
                const QPointF back = window.displayedPointToPdf(displayed.center());
                check(std::abs(back.x() - probeRect.center().x()) < 1e-6 &&
                          std::abs(back.y() - probeRect.center().y()) < 1e-6,
                      "displayed rect round-trips back to the PDF rect centre");
                // A quarter turn swaps width and height; 0/180 keep them.
                const bool quarter = (turn % 2) == 1;
                const double expectedW = quarter ? probeRect.height() : probeRect.width();
                const double expectedH = quarter ? probeRect.width() : probeRect.height();
                check(std::abs(displayed.width() - expectedW) < 1e-6 &&
                          std::abs(displayed.height() - expectedH) < 1e-6,
                      "the displayed rect has the dimensions the rotation implies");
                QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            }
            check(window.userRotation() == 0, "mapping probes left the view unrotated");

            // At 90 degrees the PDF top-left corner must land on the displayed
            // top-right corner (clockwise rotation of the page).
            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            const QRectF tl = window.pdfRectToDisplayed(QRectF(0.0, 0.0, 1.0, 1.0));
            check(std::abs(tl.left() - (mappedPortrait.height() - 1.0)) < 1e-6,
                  "at 90 degrees the PDF top-left corner maps to the displayed right edge");
            check(std::abs(tl.top() - 0.0) < 1e-6,
                  "at 90 degrees the PDF top-left corner maps to the displayed top edge");
            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            QMetaObject::invokeMethod(&window, "rotateViewClockwise");
            check(window.userRotation() == 0, "view rotation restored before the hash check");

            // The file on disk must be byte-identical: rotating the view never
            // rewrites the document.
            check(sha256Of(path) == hashBeforeRotation,
                  "view rotation does not modify the PDF on disk");
            check(window.hasDocument(), "the document is still open after rotating");
            check(window.pageCount() == total, "rotation does not change the page count");

            std::printf("\nClose:\n");
            QMetaObject::invokeMethod(&window, "closeDocument");
            check(!window.hasDocument(), "closeDocument clears the document");
            check(window.pageCount() == 0, "page count returns to zero after close");
        }
    }

    artplus::pdf::PdfRuntime::shutdown();

    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return (exitCode == 0 && g_failures == 0) ? 0 : 1;
}
