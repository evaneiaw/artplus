#include "artplus/core/Logger.h"
#include "artplus/core/Version.h"
#include "artplus/pdf/PdfDocument.h"
#include "artplus/ui/MainWindow.h"
#include "artplus/ui/Theme.h"

#include <QApplication>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QTimer>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ArtPlus"));
    app.setApplicationVersion(QStringLiteral(ARTPLUS_VERSION_STRING));
    app.setOrganizationName(QStringLiteral("ArtPlus Project"));

    artplus::ui::theme::apply(app);

    artplus::Logger::instance().setMinimumLevel(artplus::LogLevel::Info);
    artplus::pdf::PdfRuntime::initialize();

    int result = 0;
    {
        // The window owns the open PdfDocument; it MUST be destroyed before
        // PdfRuntime::shutdown(), otherwise the document is closed after the
        // PDFium library has already been destroyed.
        artplus::ui::MainWindow window;
        window.show();

        // Optional PDF path from the command line: ArtPlus.exe file.pdf
        const QStringList args = app.arguments();
        if (args.size() > 1) {
            const QString candidate = args.at(1);
            if (!candidate.startsWith(QLatin1Char('-'))) {
                QTimer::singleShot(0, &window, [&window, candidate]() {
                    window.openDocument(candidate);
                });
            }
        }

        result = app.exec();
    }

    artplus::pdf::PdfRuntime::shutdown();
    return result;
}
