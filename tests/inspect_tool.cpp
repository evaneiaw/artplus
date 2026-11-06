// Prints page-object geometry of a PDF; used as evidence for edit round-trips.
//
// Usage: artplus_inspect <file.pdf> [pageIndex]

#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfEditor.h"

#include <QCoreApplication>
#include <QString>

#include <cstdio>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    if (argc < 2) {
        std::printf("usage: artplus_inspect <file.pdf> [pageIndex]\n");
        return 2;
    }

    const QString path = QString::fromLocal8Bit(argv[1]);
    const int pageIndex = argc >= 3 ? QString::fromLocal8Bit(argv[2]).toInt() : 0;

    artplus::pdf::PdfRuntime::initialize();

    int result = 1;
    {
        QString error;
        auto doc = artplus::pdf::PdfDocument::open(path, &error);
        if (!doc) {
            std::printf("open failed: %s\n", qPrintable(error));
        } else {
            std::printf("file      : %s\n", qPrintable(path));
            std::printf("pages     : %d\n", doc->pageCount());
            std::printf("sha256    : %s\n", doc->sourceSha256().constData());

            artplus::pdf::PdfEditor editor(*doc, pageIndex);
            if (!editor.isValid()) {
                std::printf("editor could not open page %d\n", pageIndex + 1);
            } else {
                std::printf("page      : %d\n", pageIndex + 1);
                std::printf("objects   : %d\n", editor.objectCount());
                for (int i = 0; i < editor.objectCount(); ++i) {
                    const auto* object = editor.object(i);
                    if (!object)
                        continue;
                    std::printf("  [%d] %-7s bounds=(%.2f,%.2f %.2fx%.2f) "
                                "matrix.e=(%.2f) matrix.f=(%.2f)",
                                i, qPrintable(artplus::pdf::pageObjectKindName(object->kind)),
                                object->bounds.left(), object->bounds.top(),
                                object->bounds.width(), object->bounds.height(),
                                object->matrixE, object->matrixF);
                    if (object->kind == artplus::pdf::PageObjectKind::Text)
                        std::printf(" text=\"%s\"", qPrintable(object->text));
                    std::printf("\n");
                }
            }
            result = 0;
        }
    }

    artplus::pdf::PdfRuntime::shutdown();
    return result;
}
