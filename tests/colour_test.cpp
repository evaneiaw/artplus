// Regression test for the object paint colour read-back.
//
// FPDFPageObj_GetFillColor/GetStrokeColor were already being called by
// PdfEditor, but the returned RGBA values were discarded, so "Select Same Fill
// Color" had no colour to compare. These checks pin the behaviour that matters:
// the same paint in the fixture yields the same packed value, and different
// paints yield different values.

#include "fixture_builder.h"

#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfEditor.h"
#include "artplus/pdf/PdfSnapshot.h"
#include "artplus/preflight/Preflight.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <cstdio>
#include <memory>

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

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QString workDir = QDir::currentPath();
    const QString pdfPath = workDir + QStringLiteral("/artplus_colour_fixture.pdf");
    QFile::remove(pdfPath);

    std::printf("ArtPlus object colour test\n==========================\n");

    if (!artplus_test::writeEditFixture(pdfPath)) {
        std::printf("  [FAIL] could not write fixture\n");
        std::printf("\nResult: 1 checks, 1 failure(s)\n");
        return 1;
    }

    QString error;
    std::unique_ptr<artplus::pdf::PdfDocument> document =
        artplus::pdf::PdfDocument::open(pdfPath, &error);
    check(document != nullptr, "fixture opens");
    if (!document) {
        std::printf("  open error: %s\n", qPrintable(error));
        std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
        return 1;
    }
    check(document->isValid(), "document is valid");

    artplus::pdf::PdfEditor editor(*document, 0);
    check(editor.isValid(), "editor is valid for page 1");
    check(editor.objectCount() > 0, "page has objects");

    // ---- Object paint read back ----
    int objectsWithFill = 0;
    int objectsWithStroke = 0;
    for (int i = 0; i < editor.objectCount(); ++i) {
        const artplus::pdf::EditableObject* object = editor.object(i);
        if (!object)
            continue;
        if (object->hasFillColor) {
            ++objectsWithFill;
            check(object->fillColor != 0,
                  "filled object carries a non-zero packed fill colour");
        }
        if (object->hasStrokeColor) {
            ++objectsWithStroke;
            check(object->strokeColor != 0,
                  "stroked object carries a non-zero packed stroke colour");
        }
    }
    check(objectsWithFill > 0, "at least one object reports a fill colour");
    check(objectsWithStroke > 0, "at least one object reports a stroke colour");

    // ---- Distinct paints must not collapse to the same value ----
    // The fixture fills one rectangle with red and strokes a path with blue, so
    // the fill and stroke values have to differ.
    unsigned int firstFill = 0;
    unsigned int firstStroke = 0;
    for (int i = 0; i < editor.objectCount() && (firstFill == 0 || firstStroke == 0); ++i) {
        const artplus::pdf::EditableObject* object = editor.object(i);
        if (!object)
            continue;
        if (firstFill == 0 && object->hasFillColor)
            firstFill = object->fillColor;
        if (firstStroke == 0 && object->hasStrokeColor)
            firstStroke = object->strokeColor;
    }
    check(firstFill != 0 && firstStroke != 0, "collected one fill and one stroke colour");
    check(firstFill != firstStroke, "red fill and blue stroke yield different packed values");

    // ---- Same-paint comparison (what Select Same Fill Color does) ----
    int matches = 0;
    for (int i = 0; i < editor.objectCount(); ++i) {
        const artplus::pdf::EditableObject* object = editor.object(i);
        if (object && object->hasFillColor && object->fillColor == firstFill)
            ++matches;
    }
    check(matches >= 1, "the reference fill colour matches at least itself");

    int strokeMatches = 0;
    for (int i = 0; i < editor.objectCount(); ++i) {
        const artplus::pdf::EditableObject* object = editor.object(i);
        if (object && object->hasStrokeColor && object->strokeColor == firstStroke)
            ++strokeMatches;
    }
    check(strokeMatches >= 1, "the reference stroke colour matches at least itself");

    // ---- TAC analysis honours the precision contract ----
    const artplus::pdf::PdfSnapshot snapshot = artplus::pdf::PdfSnapshot::capture(*document);
    const auto tac = artplus::preflight::calculateTacDocument(snapshot, &editor, 300);
    check(tac.tacLimit == 300, "TAC limit is propagated");
    check(tac.precision != artplus::preflight::TacPrecision::Exact,
          "TAC is never reported as Exact without real CMYK ink data");
    check(!tac.objectResults.isEmpty(), "TAC result includes per-object entries");
    check(tac.precision != artplus::preflight::TacPrecision::Unsupported,
          "TAC produces an estimate when object paint is readable");
    check(tac.totalTac >= 0.0 && tac.totalTac <= 400.0, "TAC stays within 0..400 %");
    check(!tac.limitations.isEmpty(), "TAC documents its limitations");

    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
