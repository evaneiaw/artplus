// Milestone 22 (second slice): editable geometry and a real save/reopen cycle.
//
// Builds a controlled fixture (rectangle, cubic Bezier, text, image, form
// XObject), imports it, moves one path, saves, reopens and verifies that the
// move persisted while every other object stayed unchanged.

#include "fixture_builder.h"

#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfEditor.h"
#include "artplus/pdf/PdfSnapshot.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

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

bool nearRect(const QRectF& a, const QRectF& b, double eps = 1.0)
{
    return std::abs(a.left() - b.left()) <= eps && std::abs(a.top() - b.top()) <= eps &&
           std::abs(a.width() - b.width()) <= eps && std::abs(a.height() - b.height()) <= eps;
}

QString rectText(const QRectF& r)
{
    return QStringLiteral("(%1,%2 %3x%4)")
        .arg(r.left(), 0, 'f', 1)
        .arg(r.top(), 0, 'f', 1)
        .arg(r.width(), 0, 'f', 1)
        .arg(r.height(), 0, 'f', 1);
}

void dumpObjects(const artplus::pdf::PdfEditor& editor)
{
    std::printf("  objects imported: %d\n", editor.objectCount());
    for (int i = 0; i < editor.objectCount(); ++i) {
        const auto* object = editor.object(i);
        if (!object)
            continue;
        std::printf("   [%d] %-7s %s", i,
                    qPrintable(artplus::pdf::pageObjectKindName(object->kind)),
                    qPrintable(rectText(object->bounds)));
        if (object->kind == artplus::pdf::PageObjectKind::Path) {
            std::printf(" segs=%d closed=%d fill=%d stroke=%d", object->path.size(),
                        object->pathClosed ? 1 : 0, object->hasFillColor ? 1 : 0,
                        object->hasStrokeColor ? 1 : 0);
            std::printf(" [");
            for (const auto& seg : object->path) {
                const char* t = seg.type == artplus::pdf::EditablePathSegment::Type::MoveTo ? "M"
                              : seg.type == artplus::pdf::EditablePathSegment::Type::LineTo ? "L"
                              : seg.type == artplus::pdf::EditablePathSegment::Type::BezierTo ? "C"
                              : "?";
                std::printf("%s(%.1f,%.1f)%s", t, seg.point.x(), seg.point.y(),
                            seg.closes ? "h " : " ");
            }
            std::printf("] m=(%.1f,%.1f)", object->matrixE, object->matrixF);
            if (!object->path.isEmpty()) {
                int cubics = 0;
                QPointF c1, c2, e;
                for (const auto& curve : object->curves()) {
                    if (curve.kind == artplus::pdf::EditableCurve::Kind::Cubic) {
                        ++cubics; c1 = curve.control1; c2 = curve.control2; e = curve.end;
                    }
                }
                if (cubics > 0)
                    std::printf(" cubics=%d c1=(%.1f,%.1f) c2=(%.1f,%.1f) end=(%.1f,%.1f)",
                                cubics, c1.x(), c1.y(), c2.x(), c2.y(), e.x(), e.y());
            }
        } else if (object->kind == artplus::pdf::PageObjectKind::Text) {
            std::printf(" \"%s\" font=%s size=%.1f", qPrintable(object->text),
                        qPrintable(object->fontName), object->fontSize);
        } else if (object->kind == artplus::pdf::PageObjectKind::Image) {
            std::printf(" %dx%d px", object->imageWidth, object->imageHeight);
        } else if (object->kind == artplus::pdf::PageObjectKind::Form) {
            std::printf(" children=%d", object->formChildCount);
        }
        std::printf("\n");
    }
}

int findPathNear(const artplus::pdf::PdfEditor& editor, const QRectF& target, double eps = 1.5)
{
    for (int i = 0; i < editor.objectCount(); ++i) {
        const auto* object = editor.object(i);
        if (object && object->kind == artplus::pdf::PageObjectKind::Path &&
            nearRect(object->bounds, target, eps))
            return i;
    }
    return -1;
}

int findKind(const artplus::pdf::PdfEditor& editor, artplus::pdf::PageObjectKind kind)
{
    for (int i = 0; i < editor.objectCount(); ++i) {
        const auto* object = editor.object(i);
        if (object && object->kind == kind)
            return i;
    }
    return -1;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    std::printf("ArtPlus editable geometry / round-trip test (M22.2)\n");
    std::printf("==================================================\n");

    artplus::pdf::PdfRuntime::initialize();

    {
        const QString fixture =
            QDir::current().filePath(QStringLiteral("artplus_edit_fixture.pdf"));
        QFile::remove(fixture);
        check(artplus_test::writeEditFixture(fixture), "fixture PDF written");

        const QByteArray sourceHash = sha256Of(fixture);
        check(!sourceHash.isEmpty(), "fixture is readable");

        int beforeCount = 0;
        int rectIndex = -1;
        QRectF rectBounds;
        QRectF bezierBounds;
        int bezierIndex = -1;
        QString textValue;
        double textSize = 0.0;
        int imageWidth = 0, imageHeight = 0;
        int formChildren = 0;

        {
            QString error;
            auto doc = artplus::pdf::PdfDocument::open(fixture, &error);
            check(doc != nullptr, "fixture opens");
            if (!doc) {
                std::printf("  error: %s\n", qPrintable(error));
            } else {
                artplus::pdf::PdfEditor editor(*doc, 0);
                check(editor.isValid(), "editor opened page 1");
                const QImage originalRender = editor.render(1.0);
                check(!originalRender.isNull(), "source fixture renders before editing");
                check(editor.warnings().isEmpty(), "no blocked content warnings");
                beforeCount = editor.objectCount();

                std::printf("\nImport:\n");
                dumpObjects(editor);

                // Rectangle: PDF y 500..600 -> page y 192..292.
                const QRectF expectedRect(100.0, 192.0, 150.0, 100.0);
                rectIndex = findPathNear(editor, expectedRect);
                check(rectIndex >= 0, "rectangle path imported with expected bounds");
                if (rectIndex >= 0) {
                    rectBounds = editor.object(rectIndex)->bounds;
                    const auto& path = editor.object(rectIndex)->path;
                    check(path.size() >= 4, "rectangle has four or more segments");
                    check(editor.object(rectIndex)->hasFillColor,
                          "rectangle fill colour imported");
                    check(std::abs(editor.object(rectIndex)->matrixE) < 0.1 &&
                              std::abs(editor.object(rectIndex)->matrixF) < 0.1,
                          "rectangle starts with an identity translation");
                }

                // The second path is the cubic Bezier stroke.
                for (int i = 0; i < editor.objectCount(); ++i) {
                    if (i != rectIndex && editor.object(i) &&
                        editor.object(i)->kind == artplus::pdf::PageObjectKind::Path) {
                        bezierIndex = i;
                        break;
                    }
                }
                check(bezierIndex >= 0, "Bezier path imported");
                if (bezierIndex >= 0) {
                    const QRectF b = editor.object(bezierIndex)->bounds;
                    check(b.left() > 335.0 && b.left() < 360.0 &&
                              b.top() > 185.0 && b.top() < 200.0 &&
                              b.width() > 190.0 && b.width() < 220.0 &&
                              b.height() > 145.0 && b.height() < 160.0,
                          "Bezier path bounds are in the expected region");
                }
                if (bezierIndex >= 0) {
                    bezierBounds = editor.object(bezierIndex)->bounds;
                    bool hasCubic = false;
                    for (const auto& segment : editor.object(bezierIndex)->path) {
                        if (segment.type == artplus::pdf::EditablePathSegment::Type::BezierTo)
                            hasCubic = true;
                    }
                    check(hasCubic, "Bezier path exposes cubic segments");
                    int cubics = 0;
                    QPointF c1, c2, end;
                    for (const auto& curve : editor.object(bezierIndex)->curves()) {
                        if (curve.kind == artplus::pdf::EditableCurve::Kind::Cubic) {
                            ++cubics; c1 = curve.control1; c2 = curve.control2; end = curve.end;
                        }
                    }
                    check(cubics == 1, "one cubic curve reconstructed");
                    check(std::abs(c1.x() - 400.0) < 1.5 && std::abs(c1.y() - 192.0) < 1.5 &&
                              std::abs(c2.x() - 500.0) < 1.5 && std::abs(c2.y() - 342.0) < 1.5 &&
                              std::abs(end.x() - 550.0) < 1.5 && std::abs(end.y() - 242.0) < 1.5,
                          "cubic control points and end point reconstructed");
                }

                const int textIndex = findKind(editor, artplus::pdf::PageObjectKind::Text);
                check(textIndex >= 0, "text object imported");
                if (textIndex >= 0) {
                    const auto* text = editor.object(textIndex);
                    textValue = text->text;
                    textSize = text->fontSize;
                    check(text->text.contains(QStringLiteral("ArtPlus fixture")),
                          "text content imported");
                    check(std::abs(text->fontSize - 24.0) < 0.6, "font size imported (24)");
                    check(text->fontName.contains(QStringLiteral("Helvetica")),
                          "font name imported");
                }

                const int imageIndex = findKind(editor, artplus::pdf::PageObjectKind::Image);
                check(imageIndex >= 0, "image object imported");
                if (imageIndex >= 0) {
                    imageWidth = editor.object(imageIndex)->imageWidth;
                    imageHeight = editor.object(imageIndex)->imageHeight;
                    check(imageWidth == 8 && imageHeight == 8,
                          "image pixel dimensions imported (8x8)");
                }

                const int formIndex = findKind(editor, artplus::pdf::PageObjectKind::Form);
                check(formIndex >= 0, "form XObject imported");
                if (formIndex >= 0) {
                    formChildren = editor.object(formIndex)->formChildCount;
                    check(formChildren >= 1, "form XObject children counted");
                }

                std::printf("\nEdit:\n");
                check(rectIndex >= 0, "rectangle selected for editing");
                if (rectIndex >= 0) {
                    const QPointF delta(30.0, 20.0);
                    const QRectF expected = rectBounds.translated(delta);
                    check(editor.translateObject(rectIndex, delta.x(), delta.y()),
                          "rectangle translated by (30, 20) pt");
                    check(editor.isModified(), "editor reports the document modified");
                    check(nearRect(editor.object(rectIndex)->bounds, expected, 0.1),
                          "in-memory bounds reflect the translation");
                    const QImage editedRender = editor.render(1.0);
                    check(!editedRender.isNull() && editedRender != originalRender,
                          "translation changes the rendered page");

                    QString selfError;
                    check(!editor.saveAs(fixture, &selfError),
                          "editor refuses to overwrite its own source file");
                    if (selfError.isEmpty())
                        std::printf("  (no refusal message produced)\n");

                    const QString output = fixture + QStringLiteral("-edited.pdf");
                    QFile::remove(output);
                    QString saveError;
                    check(editor.saveAs(output, &saveError), "edited PDF saved");
                    if (!saveError.isEmpty())
                        std::printf("  save error: %s\n", qPrintable(saveError));

                    std::printf("\nReopen (%s):\n", qPrintable(QFileInfo(output).fileName()));
                    auto reopened = artplus::pdf::PdfDocument::open(output);
                    check(reopened != nullptr, "edited PDF reopens");
                    if (reopened) {
                        check(reopened->pageCount() == 1, "reopened keeps one page");
                        artplus::pdf::PdfEditor verify(*reopened, 0);
                        check(verify.isValid(), "reopened page is editable");
                        check(verify.objectCount() == beforeCount,
                              "reopened keeps the same object count");
                        const QImage reopenedRender = verify.render(1.0);
                        check(!reopenedRender.isNull() && reopenedRender == editedRender,
                              "saved page renders pixel-identically to the in-memory edit");

                        // Fixed regions in this generated 612x792 pt fixture, at
                        // one pixel per point. None overlaps the moved rectangle.
                        // Require visible content as well as equality so empty
                        // images or accidentally blank crops cannot pass.
                        const auto unchangedRegion = [&](const QRect& region) {
                            if (originalRender.isNull() || reopenedRender.isNull() ||
                                !originalRender.rect().contains(region) ||
                                !reopenedRender.rect().contains(region))
                                return false;
                            const QImage before = originalRender.copy(region);
                            QImage blank(before.size(), before.format());
                            blank.fill(Qt::white);
                            return before != blank && before == reopenedRender.copy(region);
                        };
                        check(unchangedRegion(QRect(90, 365, 220, 40)),
                              "untouched text appearance preserved after save");
                        check(unchangedRegion(QRect(95, 505, 90, 95)),
                              "untouched image pixels preserved after save");
                        check(unchangedRegion(QRect(295, 525, 110, 75)),
                              "untouched form appearance preserved after save");
                        check(unchangedRegion(QRect(340, 185, 220, 160)),
                              "untouched cubic stroke appearance preserved after save");
                        dumpObjects(verify);

                        const int movedIndex = findPathNear(verify, expected);
                        check(movedIndex >= 0, "moved rectangle found at the new position");
                        if (movedIndex >= 0) {
                            const auto* moved = verify.object(movedIndex);
                            check(std::abs(moved->matrixE - 30.0) < 0.6 &&
                                      std::abs(moved->matrixF + 20.0) < 0.6,
                                  "translation persisted in the object matrix (30, -20) PDF");
                        }
                        const int oldIndex = findPathNear(verify, rectBounds, 1.0);
                        check(oldIndex < 0, "rectangle is no longer at the old position");

                        check(findPathNear(verify, bezierBounds) >= 0,
                              "Bezier path unchanged after save");
                        const int textIndex2 =
                            findKind(verify, artplus::pdf::PageObjectKind::Text);
                        check(textIndex2 >= 0 && verify.object(textIndex2)->text == textValue,
                              "text content preserved after save");
                        check(textIndex2 >= 0 &&
                                  std::abs(verify.object(textIndex2)->fontSize - textSize) < 0.6,
                              "text size preserved after save");
                        const int imageIndex2 =
                            findKind(verify, artplus::pdf::PageObjectKind::Image);
                        check(imageIndex2 >= 0 &&
                                  verify.object(imageIndex2)->imageWidth == imageWidth &&
                                  verify.object(imageIndex2)->imageHeight == imageHeight,
                              "image preserved after save");
                        const int formIndex2 =
                            findKind(verify, artplus::pdf::PageObjectKind::Form);
                        check(formIndex2 >= 0 &&
                                  verify.object(formIndex2)->formChildCount == formChildren,
                              "form XObject preserved after save");

                        const auto snapshot = artplus::pdf::PdfSnapshot::capture(*reopened);
                        check(snapshot.countOf(artplus::pdf::PageObjectKind::Path) == 2,
                              "snapshot still reports both paths");
                        std::printf("  reopened source hash: %s\n",
                                    snapshot.sourceSha256.constData());

                        // The reopened document must keep its own source path.
                        check(QFileInfo(reopened->filePath()).absoluteFilePath() ==
                                  QFileInfo(output).absoluteFilePath(),
                              "reopened document points at the edited file");
                    }
                }

                // The original file must be byte-identical.
                check(sha256Of(fixture) == sourceHash, "source fixture unchanged");
            }
        }

    }

    {
        const QString multi =
            QDir::current().filePath(QStringLiteral("artplus_edit_multipage_fixture.pdf"));
        const QString output = multi + QStringLiteral("-edited.pdf");
        QFile::remove(multi);
        QFile::remove(output);
        check(artplus_test::writeEditFixture(multi, true),
              "multipage fixture PDF written");
        auto document = artplus::pdf::PdfDocument::open(multi);
        check(document && document->pageCount() == 2,
              "multipage fixture opens with two pages");
        if (document && document->pageCount() == 2) {
            const QSizeF untouchedSize = document->pageSize(1);
            check(untouchedSize == QSizeF(700, 900) &&
                      untouchedSize != document->pageSize(0),
                  "untouched page has distinct expected dimensions");
            artplus::pdf::PdfEditor untouched(*document, 1);
            const QImage untouchedRender = untouched.render(1.0);
            QImage blank(untouchedRender.size(), untouchedRender.format());
            blank.fill(Qt::white);
            check(!untouchedRender.isNull() && untouchedRender != blank,
                  "untouched page contains visible reference content");
            const QByteArray originalHash = sha256Of(multi);
            artplus::pdf::PdfEditor editor(*document, 0);
            check(editor.isValid(), "first page of multipage fixture is editable");
            check(editor.translateObject(0, 5.0, 5.0),
                  "first page of multipage fixture accepts an edit");
            QString error;
            check(editor.saveAs(output, &error),
                  "multipage edited PDF saved and verified");
            auto reopened = artplus::pdf::PdfDocument::open(output);
            check(reopened && reopened->pageCount() == 2,
                  "edited multipage PDF keeps both pages");
            if (reopened && reopened->pageCount() == 2) {
                const QSizeF actual = reopened->pageSize(1);
                check(std::abs(actual.width() - untouchedSize.width()) < 0.01 &&
                          std::abs(actual.height() - untouchedSize.height()) < 0.01,
                      "untouched page dimensions are preserved");
                artplus::pdf::PdfEditor verifyUntouched(*reopened, 1);
                check(verifyUntouched.isValid() &&
                          verifyUntouched.objectCount() == untouched.objectCount(),
                      "untouched page object count is preserved");
                check(!untouchedRender.isNull() &&
                          verifyUntouched.render(1.0) == untouchedRender,
                      "untouched page renders pixel-identically after other page edit");
            }
            check(!originalHash.isEmpty() && sha256Of(multi) == originalHash,
                  "multipage source remains byte-identical");
        }
    }

    {
        const QString fixture = QDir::current().filePath(
            QStringLiteral("artplus_edit_resources_fixture.pdf"));
        const QString output = fixture + QStringLiteral("-edited.pdf");
        QFile::remove(fixture);
        QFile::remove(output);
        check(artplus_test::writeEditFixture(fixture, false, true),
              "transparency and clipping fixture written");
        const QByteArray originalHash = sha256Of(fixture);
        auto document = artplus::pdf::PdfDocument::open(fixture);
        check(document != nullptr, "resource fixture opens");
        if (document) {
            artplus::pdf::PdfEditor editor(*document, 0);
            const QImage before = editor.render(1.0);
            check(editor.isValid() && before.size() == QSize(612, 792),
                  "resource fixture renders at expected size");
            if (before.size() == QSize(612, 792)) {
                const QColor purple = before.pixelColor(400, 702);
                check(std::abs(purple.red() - 128) <= 2 &&
                          purple.green() <= 2 && std::abs(purple.blue() - 128) <= 2,
                      "reference alpha blend is half red over blue");
                check(before.pixelColor(120, 732) == QColor(Qt::green) &&
                          before.pixelColor(90, 732) == QColor(Qt::white) &&
                          before.pixelColor(150, 732) == QColor(Qt::white),
                      "reference clip paints inside and excludes both outside samples");
                const int selected = findPathNear(editor, QRectF(100, 192, 150, 100));
                check(selected >= 0 && editor.translateObject(selected, 30, 20),
                      "unrelated rectangle moved in resource fixture");
                const QImage edited = editor.render(1.0);
                const QRect resources(70, 642, 400, 140);
                check(!edited.isNull() && edited != before &&
                          edited.copy(resources) == before.copy(resources),
                      "edit changes page while alpha and clip regions stay identical");
                QString error;
                check(editor.saveAs(output, &error), "resource fixture edited save succeeds");
                if (!error.isEmpty())
                    std::printf("  resource save error: %s\n", qPrintable(error));
                auto reopened = artplus::pdf::PdfDocument::open(output);
                check(reopened != nullptr, "saved resource fixture reopens");
                if (reopened) {
                    artplus::pdf::PdfEditor verify(*reopened, 0);
                    const QImage after = verify.render(1.0);
                    check(!after.isNull() && after == edited,
                          "resource fixture saved render matches entire in-memory edit");
                    check(!after.isNull() && after.copy(resources) == before.copy(resources),
                          "saved transparency and clipping regions match original pixels");
                    check(verify.objectCount() == editor.objectCount(),
                          "resource fixture object count preserved");
                }
            }
        }
        check(!originalHash.isEmpty() && sha256Of(fixture) == originalHash,
              "resource fixture source remains byte-identical");
    }

    {
        const QString fixture = QDir::current().filePath(
            QStringLiteral("artplus_edit_softmask_fixture.pdf"));
        const QString output = fixture + QStringLiteral("-edited.pdf");
        QFile::remove(fixture);
        QFile::remove(output);
        check(artplus_test::writeEditFixture(fixture, false, false, true),
              "soft-mask fixture written");
        const QByteArray originalHash = sha256Of(fixture);
        auto document = artplus::pdf::PdfDocument::open(fixture);
        check(document != nullptr, "soft-mask fixture opens");
        if (document) {
            artplus::pdf::PdfEditor editor(*document, 0);
            const QImage before = editor.render(1.0);
            check(editor.isValid() && before.size() == QSize(612, 792),
                  "soft-mask fixture renders at expected size");
            if (before.size() == QSize(612, 792)) {
                check(before.pixelColor(370, 702) == QColor(Qt::blue),
                      "zero soft-mask samples reveal the blue backdrop");
                const QColor purple = before.pixelColor(430, 702);
                check(std::abs(purple.red() - 128) <= 2 &&
                          purple.green() <= 2 && std::abs(purple.blue() - 127) <= 2,
                      "half soft-mask samples blend red over blue");
                const int selected = findPathNear(editor, QRectF(100, 192, 150, 100));
                check(selected >= 0 && editor.translateObject(selected, 30, 20),
                      "unrelated rectangle moved in soft-mask fixture");
                const QImage edited = editor.render(1.0);
                const QRect resources(70, 642, 400, 140);
                check(!edited.isNull() && edited != before &&
                          edited.copy(resources) == before.copy(resources),
                      "edit changes page while soft-mask regions stay identical");
                QString error;
                check(editor.saveAs(output, &error), "soft-mask fixture edited save succeeds");
                if (!error.isEmpty())
                    std::printf("  resource save error: %s\n", qPrintable(error));
                auto reopened = artplus::pdf::PdfDocument::open(output);
                check(reopened != nullptr, "saved soft-mask fixture reopens");
                if (reopened) {
                    artplus::pdf::PdfEditor verify(*reopened, 0);
                    const QImage after = verify.render(1.0);
                    check(!after.isNull() && after == edited,
                          "soft-mask fixture saved render matches entire in-memory edit");
                    check(!after.isNull() && after.copy(resources) == before.copy(resources),
                          "saved soft-mask regions match original pixels");
                    check(verify.objectCount() == editor.objectCount(),
                          "soft-mask fixture object count preserved");
                }
            }
        }
        check(!originalHash.isEmpty() && sha256Of(fixture) == originalHash,
              "soft-mask fixture source remains byte-identical");
    }

    artplus::pdf::PdfRuntime::shutdown();

    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
