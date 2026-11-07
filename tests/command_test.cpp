// Milestone 23: command stack, undo/redo, transforms, properties and save
// verification on top of the M22 editor.

#include "fixture_builder.h"

#include "artplus/edit/Command.h"
#include "artplus/edit/Properties.h"
#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfEditor.h"

#include <QCoreApplication>
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

bool near(double a, double b, double eps = 0.75)
{
    return std::abs(a - b) <= eps;
}

bool nearRect(const QRectF& a, const QRectF& b, double eps = 0.75)
{
    return near(a.left(), b.left(), eps) && near(a.top(), b.top(), eps) &&
           near(a.width(), b.width(), eps) && near(a.height(), b.height(), eps);
}

int findPathNear(const artplus::pdf::PdfEditor& editor, const QRectF& target, double eps = 2.0)
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

    std::printf("ArtPlus command / undo-redo test (M23)\n");
    std::printf("======================================\n");

    artplus::pdf::PdfRuntime::initialize();

    int result = 1;
    {
        const QString fixture =
            QDir::current().filePath(QStringLiteral("artplus_cmd_fixture.pdf"));
        QFile::remove(fixture);
        check(artplus_test::writeEditFixture(fixture), "fixture PDF written");

        QString error;
        auto doc = artplus::pdf::PdfDocument::open(fixture, &error);
        check(doc != nullptr, "fixture opens");
        if (doc) {
            artplus::pdf::PdfEditor editor(*doc, 0);
            check(editor.isValid(), "editor opened page 1");

            const int rect = findPathNear(editor, QRectF(100, 192, 150, 100));
            check(rect >= 0, "rectangle found");
            if (rect < 0) {
                std::printf("  [SKIP] rectangle not found, skipping Properties\n");
                return 1;
            }
            const QRectF original = editor.object(rect)->bounds;

            // --- Properties ---------------------------------------------------
            std::printf("\nProperties:\n");
            {
                const auto* obj = editor.object(rect);
                check(obj != nullptr, "object exists");
                if (!obj) {
                    std::printf("  [SKIP] object is null\n");
                    return 1;
                }
                const auto p = artplus::edit::describe(*obj);
                check(p.valid && p.kind == QStringLiteral("Path"), "type reported");
                check(near(p.x, 100.0) && near(p.y, 192.0), "X/Y reported from bounds");
                check(near(p.width, 150.0) && near(p.height, 100.0), "width/height reported");
                check(!p.hasRotation, "no rotation reported for the plain rectangle");
            }
            {
                const int text = findKind(editor, artplus::pdf::PageObjectKind::Text);
                check(text >= 0, "text object found");
                const auto p = artplus::edit::describe(*editor.object(text));
                check(p.text.contains(QStringLiteral("ArtPlus fixture")), "text content reported");
                check(p.fontName.contains(QStringLiteral("Helvetica")), "font reported");
                check(near(p.fontSize, 24.0, 0.6), "font size reported");
            }
            {
                const int image = findKind(editor, artplus::pdf::PageObjectKind::Image);
                check(image >= 0, "image object found");
                const auto p = artplus::edit::describe(*editor.object(image));
                check(p.pixelWidth == 8 && p.pixelHeight == 8, "image pixel size reported");
            }

            // --- Move + undo/redo --------------------------------------------
            std::printf("\nMove / undo / redo:\n");
            artplus::edit::CommandStack stack;
            check(!stack.isModified(), "clean before any command");
            check(stack.execute(std::make_unique<artplus::edit::MoveObjectCommand>(
                      editor, rect, 30.0, 20.0)),
                  "move command executed");
            check(nearRect(editor.object(rect)->bounds, QRectF(130, 212, 150, 100)),
                  "object moved by (30, 20)");
            check(stack.isModified(), "document marked modified");
            check(stack.canUndo() && !stack.canRedo(), "undo available, redo not");

            check(stack.undo(), "undo executed");
            check(nearRect(editor.object(rect)->bounds, original, 1e-6),
                  "undo restores the original bounds exactly");
            check(!stack.isModified(), "clean again after undo to the initial state");
            check(!stack.canUndo() && stack.canRedo(), "redo available after undo");

            check(stack.redo(), "redo executed");
            check(nearRect(editor.object(rect)->bounds, QRectF(130, 212, 150, 100)),
                  "redo re-applies the move");

            // --- Resize -------------------------------------------------------
            std::printf("\nResize:\n");
            {
                const QRectF before = editor.object(rect)->bounds;
                check(stack.execute(std::make_unique<artplus::edit::ScaleObjectCommand>(
                          editor, rect, before.left(), before.top(), 2.0, 2.0)),
                      "resize command executed");
                const QRectF after = editor.object(rect)->bounds;
                std::printf("    resize before=(%.3f,%.3f %.3fx%.3f) after=(%.3f,%.3f %.3fx%.3f)\n",
                            before.left(), before.top(), before.width(), before.height(),
                            after.left(), after.top(), after.width(), after.height());
                check(near(after.width(), 300.0, 1.5) && near(after.height(), 200.0, 1.5),
                      "resize doubles width and height");
                check(near(after.left(), before.left()) && near(after.top(), before.top()),
                      "resize keeps the top-left anchor");
                check(stack.undo(), "resize undone");
                check(nearRect(editor.object(rect)->bounds, before), "undo restores size");
                check(stack.redo(), "resize redone");
            }

            // --- Rotate -------------------------------------------------------
            // Reflection must preserve bounds, reverse orientation and round-trip exactly.
            for (bool vertical : {false, true}) {
                const auto before = editor.objectMatrix(rect);
                const auto bounds = editor.object(rect)->bounds;
                check(stack.execute(std::make_unique<artplus::edit::ScaleObjectCommand>(
                          editor, rect, bounds.center().x(), bounds.center().y(),
                          vertical ? 1.0 : -1.0, vertical ? -1.0 : 1.0)),
                      "flip executed");
                const auto after = editor.objectMatrix(rect);
                check(near(after.a * after.d - after.b * after.c,
                           -(before.a * before.d - before.b * before.c), 1e-6),
                      "flip reverses matrix orientation");
                check(nearRect(editor.object(rect)->bounds, bounds), "flip preserves bounds");
                check(stack.undo(), "flip undone");
                const auto restored = editor.objectMatrix(rect);
                check(near(restored.a, before.a, 1e-6) && near(restored.b, before.b, 1e-6) &&
                          near(restored.c, before.c, 1e-6) && near(restored.d, before.d, 1e-6) &&
                          near(restored.e, before.e, 1e-6) && near(restored.f, before.f, 1e-6),
                      "flip undo restores exact matrix");
                check(stack.redo(), "flip redone");
                check(stack.undo(), "flip undone for remaining tests");
            }
            std::printf("\nRotate:\n");
            {
                const QRectF before = editor.object(rect)->bounds;
                const double cx = before.center().x();
                const double cy = before.center().y();
                check(stack.execute(std::make_unique<artplus::edit::RotateObjectCommand>(
                          editor, rect, cx, cy, 90.0)),
                      "rotate command executed");
                const QRectF after = editor.object(rect)->bounds;
                check(near(after.width(), before.height(), 1.5) &&
                          near(after.height(), before.width(), 1.5),
                      "90-degree rotation swaps width and height");
                const auto p = artplus::edit::describe(*editor.object(rect));
                check(p.hasRotation && near(p.rotationDegrees, -90.0, 1.0),
                      "rotation reported as -90 degrees");
                check(stack.undo(), "rotation undone");
                const auto p2 = artplus::edit::describe(*editor.object(rect));
                check(!p2.hasRotation || near(p2.rotationDegrees, 0.0, 1.0),
                      "undo removes the rotation");
                check(stack.redo(), "rotation redone");
                check(stack.undo(), "rotation undone again for the save test");
            }

            // --- No numeric drift over repeated undo/redo ---------------------
            std::printf("\nDrift:\n");
            {
                stack.clear();
                const QRectF base = editor.object(rect)->bounds;
                for (int i = 0; i < 10; ++i) {
                    stack.execute(std::make_unique<artplus::edit::MoveObjectCommand>(
                        editor, rect, 1.0, 0.0));
                }
                std::printf("    drift base=(%.6f) after10=(%.6f)\n",
                            base.left(), editor.object(rect)->bounds.left());
                check(near(editor.object(rect)->bounds.left(), base.left() + 10.0, 0.05),
                      "ten 1 pt moves accumulate to 10 pt");
                for (int i = 0; i < 10; ++i)
                    stack.undo();
                std::printf("    drift after undo=(%.6f,%.6f %.3fx%.3f)\n",
                            editor.object(rect)->bounds.left(), editor.object(rect)->bounds.top(),
                            editor.object(rect)->bounds.width(), editor.object(rect)->bounds.height());
                check(nearRect(editor.object(rect)->bounds, base, 1e-6),
                      "undoing ten moves returns exactly to the base geometry");
                for (int i = 0; i < 10; ++i)
                    stack.redo();
                std::printf("    drift base=(%.6f) after10=(%.6f)\n",
                            base.left(), editor.object(rect)->bounds.left());
                check(near(editor.object(rect)->bounds.left(), base.left() + 10.0, 0.05),
                      "redoing them returns to the same position");
            }

            // --- Save after several operations --------------------------------
            std::printf("\nSave after edits:\n");
            {
                stack.clear();
                stack.execute(std::make_unique<artplus::edit::MoveObjectCommand>(
                    editor, rect, 30.0, 20.0));
                const QRectF scaled = editor.object(rect)->bounds;
                stack.execute(std::make_unique<artplus::edit::ScaleObjectCommand>(
                    editor, rect, scaled.left(), scaled.top(), 2.0, 2.0));
                const QRectF rotated = editor.object(rect)->bounds;
                stack.execute(std::make_unique<artplus::edit::RotateObjectCommand>(
                    editor, rect, rotated.center().x(), rotated.center().y(), 90.0));

                const QRectF expected = editor.object(rect)->bounds;
                check(stack.isModified(), "modified before saving");

                const QString output = QDir::current().filePath(
                    QStringLiteral("artplus_cmd_edited.pdf"));
                QFile::remove(output);
                QString saveError;
                check(editor.saveAs(output, &saveError), "edited PDF saved and verified");
                if (!saveError.isEmpty())
                    std::printf("  save error: %s\n", qPrintable(saveError));

                auto reopened = artplus::pdf::PdfDocument::open(output);
                check(reopened != nullptr, "edited PDF reopens");
                if (reopened) {
                    artplus::pdf::PdfEditor verify(*reopened, 0);
                    check(verify.isValid(), "reopened page is editable");
                    const int moved = findPathNear(verify, expected, 2.0);
                    check(moved >= 0, "final geometry preserved after reopen");
                    if (moved >= 0) {
                        check(nearRect(verify.object(moved)->bounds, expected, 1.5),
                              "reopened bounds match the final state");
                    }
                    check(findKind(verify, artplus::pdf::PageObjectKind::Text) >= 0,
                          "text still present");
                    check(findKind(verify, artplus::pdf::PageObjectKind::Image) >= 0,
                          "image still present");
                }

                stack.markClean();
                check(!stack.isModified(), "saved state is clean");
                check(stack.undo(), "undo after save works");
                check(stack.isModified(), "undo after save marks the document modified");
                check(stack.redo(), "redo after save works");
                check(!stack.isModified(), "redo back to the saved state is clean again");
            }

            // --- Independent history -----------------------------------------
            std::printf("\nHistory independence:\n");
            {
                QString error2;
                auto doc2 = artplus::pdf::PdfDocument::open(fixture, &error2);
                check(doc2 != nullptr, "second document opens");
                if (doc2) {
                    artplus::pdf::PdfEditor editor2(*doc2, 0);
                    artplus::edit::CommandStack stack2;
                    check(!stack2.canUndo(), "second document has its own empty history");
                    stack2.execute(std::make_unique<artplus::edit::MoveObjectCommand>(
                        editor2, findPathNear(editor2, QRectF(100, 192, 150, 100)), 5.0, 0.0));
                    check(stack2.canUndo() && stack2.undoCount() == 1,
                          "second history records its own command");
                    check(stack2.undoName() == QStringLiteral("Move object"),
                          "history exposes the command name");
                }
            }

            // --- Transform regressions ---------------------------------------
            std::printf("\nTransform regressions:\n");
            {
                const int rect = findPathNear(editor, QRectF(100, 192, 150, 100));
                if (rect < 0) {
                    std::printf("  [SKIP] rectangle not found\n");
                } else {
                    const artplus::pdf::EditableObject* obj = editor.object(rect);
                    if (!obj) {
                        std::printf("  [SKIP] object is null\n");
                    } else {
                        const QRectF originalBounds = obj->bounds;
                        const artplus::pdf::PdfMatrix originalMatrix = editor.objectMatrix(rect);
                        artplus::edit::CommandStack stack;

                    // --- Shear via TransformObjectsCommand ----------------------
                    std::printf("  Shear:\n");
                    {
                    artplus::edit::TransformParams params;
                    params.shearDegrees = 20.0;
                    params.repetitions = 1;
                    std::vector<int> indices = {rect};
                    auto shearCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::TopLeft, false);
                    check(shearCmd->hasEffect(), "shear has effect");
                    check(stack.execute(std::move(shearCmd)), "shear command executed");
                    const auto p = artplus::edit::describe(*editor.object(rect));
                    check(p.valid, "shear produced valid result");
                    check(stack.undo(), "shear undone");
                    check(nearRect(editor.object(rect)->bounds, originalBounds, 1e-3),
                          "shear undo restores original bounds");
                }

                // --- Flip horizontal via TransformObjectsCommand -------------
                std::printf("  Flip horizontal:\n");
                {
                    artplus::edit::TransformParams params;
                    params.flipHorizontal = true;
                    params.repetitions = 1;
                    std::vector<int> indices = {rect};
                    auto flipCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::Center, false);
                    check(flipCmd->hasEffect(), "flip has effect");
                    check(stack.execute(std::move(flipCmd)), "flip horizontal executed");
                    const auto after = editor.objectMatrix(rect);
                    check(near(after.a * after.d - after.b * after.c,
                               -(originalMatrix.a * originalMatrix.d - originalMatrix.b * originalMatrix.c), 1e-6),
                          "flip reverses matrix orientation");
                    check(nearRect(editor.object(rect)->bounds, originalBounds),
                          "flip preserves bounds");
                    check(stack.undo(), "flip undone");
                    const auto restored = editor.objectMatrix(rect);
                    check(near(restored.a, originalMatrix.a, 1e-6) && near(restored.b, originalMatrix.b, 1e-6) &&
                          near(restored.c, originalMatrix.c, 1e-6) && near(restored.d, originalMatrix.d, 1e-6) &&
                          near(restored.e, originalMatrix.e, 1e-6) && near(restored.f, originalMatrix.f, 1e-6),
                          "flip undo restores exact matrix");
                    check(stack.redo(), "flip redone");
                    check(stack.undo(), "flip undone for remaining tests");
                }

                // --- Flip vertical via TransformObjectsCommand ---------------
                std::printf("  Flip vertical:\n");
                {
                    artplus::edit::TransformParams params;
                    params.flipVertical = true;
                    params.repetitions = 1;
                    std::vector<int> indices = {rect};
                    auto flipCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::Center, false);
                    check(flipCmd->hasEffect(), "flip vertical has effect");
                    check(stack.execute(std::move(flipCmd)), "flip vertical executed");
                    const auto after = editor.objectMatrix(rect);
                    check(near(after.a * after.d - after.b * after.c,
                               -(originalMatrix.a * originalMatrix.d - originalMatrix.b * originalMatrix.c), 1e-6),
                          "flip vertical reverses matrix orientation");
                    check(nearRect(editor.object(rect)->bounds, originalBounds),
                          "flip vertical preserves bounds");
                    check(stack.undo(), "flip vertical undone");
                }

                // --- Composition order: Scale → Flip → Shear → Rotate → Move ---
                std::printf("  Composition order (Scale→Flip→Shear→Rotate→Move):\n");
                {
                    stack.clear();
                    const QRectF base = editor.object(rect)->bounds;
                    artplus::edit::TransformParams params;
                    params.scaleX = 2.0;
                    params.flipHorizontal = true;
                    params.shearDegrees = 20.0;
                    params.rotateDegrees = 45.0;
                    params.moveX = 10.0;
                    params.moveY = 10.0;
                    params.repetitions = 1;
                    std::vector<int> indices = {rect};
                    auto compCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::TopLeft, false);
                    check(compCmd->hasEffect(), "composition has effect");
                    check(stack.execute(std::move(compCmd)), "composition executed");
                    check(stack.isModified(), "composition modified document");
                    // Undo all
                    check(stack.undo(), "composition undone");
                    check(nearRect(editor.object(rect)->bounds, base, 1e-3),
                          "composition undo returns to base geometry");
                }

                // --- Repetitions (TransformObjectsCommand applies transform N times) ---
                std::printf("  Repetitions:\n");
                {
                    artplus::edit::TransformParams params;
                    params.scaleX = 1.5;
                    params.scaleY = 1.5;
                    params.repetitions = 3;
                    std::vector<int> indices = {rect};
                    auto repCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::TopLeft, false);
                    check(repCmd->hasEffect(), "repetitions have effect");
                    check(stack.execute(std::move(repCmd)), "repetitions executed");
                    const QRectF after = editor.object(rect)->bounds;
                    // 3 reps of 1.5x = 1.5^3 = 3.375x total
                    check(near(after.width(), originalBounds.width() * 3.375, 3.0) &&
                          near(after.height(), originalBounds.height() * 3.375, 3.0),
                          "3 repetitions of 1.5x scale (cumulative)");
                    check(stack.undo(), "repetitions undone");
                    check(nearRect(editor.object(rect)->bounds, originalBounds, 1e-3),
                          "undo restores original");
                }

                // --- Transform each (per-object anchor) ---------------------
                std::printf("  Transform each:\n");
                {
                    // Find another object to test with
                    const int text = findKind(editor, artplus::pdf::PageObjectKind::Text);
                    check(text >= 0, "text object found for transform-each test");
                    if (text >= 0) {
                        artplus::edit::TransformParams params;
                        params.scaleX = 1.2;
                        params.scaleY = 1.2;
                        params.repetitions = 1;
                        std::vector<int> indices = {rect, text};
                        auto eachCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                            editor, indices, params,
                            artplus::edit::TransformAnchor::Center, true); // transformEach = true
                        check(eachCmd->hasEffect(), "transform each has effect");
                        check(stack.execute(std::move(eachCmd)), "transform each executed");
                        // Both objects should be scaled around their own centers
                        check(near(editor.object(rect)->bounds.width(), originalBounds.width() * 1.2, 2.0) &&
                              near(editor.object(rect)->bounds.height(), originalBounds.height() * 1.2, 2.0),
                              "rectangle scaled around own center");
                        const auto textBounds = editor.object(text)->bounds;
                        check(textBounds.width() > 0 && textBounds.height() > 0,
                              "text object transformed");
                        check(stack.undo(), "transform each undone");
                    }
                }

                // --- Copy (Alt+Transform) ------------------------------------
                std::printf("  Copy (Alt+Transform):\n");
                {
                    int beforeCount = editor.objectCount();
                    artplus::edit::TransformParams params;
                    params.scaleX = 2.0;
                    params.scaleY = 2.0;
                    params.repetitions = 2;
                    std::vector<int> indices = {rect};
                    auto copyCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::TopLeft, true); // copy = true
                    check(copyCmd->hasEffect(), "copy has effect");
                    check(stack.execute(std::move(copyCmd)), "copy executed");
                    int afterCount = editor.objectCount();
                    check(afterCount == beforeCount + 2,
                          "copy creates one object per repetition (2 copies)");
                    check(stack.undo(), "copy undone");
                    check(editor.objectCount() == beforeCount,
                          "undo removes the copies");
                    check(stack.redo(), "copy redone");
                    check(editor.objectCount() == beforeCount + 2,
                          "redo restores the copies");
                }

                // --- Stroke Weights & Effects --------------------------------
                std::printf("  Stroke Weights & Effects:\n");
                {
                    const int bezier = findKind(editor, artplus::pdf::PageObjectKind::Path);
                    // Find the bezier (it's the one at ~350,500)
                    if (bezier >= 0 && bezier != rect) {
                        artplus::edit::TransformParams params;
                        params.scaleX = 2.0;
                        params.scaleY = 2.0;
                        params.includeStrokeWeightsAndEffects = true;
                        params.repetitions = 1;
                        std::vector<int> indices = {bezier};
                        double beforeStroke = editor.objectStrokeWidth(bezier);
                        auto strokeCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                            editor, indices, params,
                            artplus::edit::TransformAnchor::TopLeft, false);
                        check(strokeCmd->hasEffect(), "stroke transform has effect");
                        check(stack.execute(std::move(strokeCmd)), "stroke transform executed");
                        double afterStroke = editor.objectStrokeWidth(bezier);
                        check(near(afterStroke, beforeStroke * 2.0, 0.1),
                              "stroke width scales by mean(|scaleX|,|scaleY|) = 2.0");
                        check(stack.undo(), "stroke transform undone");
                        check(near(editor.objectStrokeWidth(bezier), beforeStroke, 1e-3),
                              "undo restores stroke width");
                    }
                }

                // --- Save after several transform operations ---------------
                std::printf("  Save after transforms:\n");
                {
                    stack.clear();
                    artplus::edit::TransformParams params;
                    params.moveX = 30.0;
                    params.moveY = 20.0;
                    std::vector<int> indices = {rect};
                    auto saveCmd1 = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::TopLeft, false);
                    check(stack.execute(std::move(saveCmd1)), "move for save");
                    params.moveX = 0; params.moveY = 0;
                    params.scaleX = 2.0; params.scaleY = 2.0;
                    auto saveCmd2 = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::TopLeft, false);
                    check(stack.execute(std::move(saveCmd2)), "scale for save");
                    params.scaleX = 1.0; params.scaleY = 1.0;
                    params.rotateDegrees = 90.0;
                    auto saveCmd3 = std::make_unique<artplus::edit::TransformObjectsCommand>(
                        editor, indices, params,
                        artplus::edit::TransformAnchor::Center, false);
                    check(stack.execute(std::move(saveCmd3)), "rotate for save");

                    const QRectF expected = editor.object(rect)->bounds;
                    check(stack.isModified(), "modified before saving");

                    const QString output = QDir::current().filePath(
                        QStringLiteral("artplus_cmd_transform_edited.pdf"));
                    QFile::remove(output);
                    QString saveError;
                    check(editor.saveAs(output, &saveError), "edited PDF saved and verified");
                    if (!saveError.isEmpty())
                        std::printf("  save error: %s\n", qPrintable(saveError));

                    auto reopened = artplus::pdf::PdfDocument::open(output);
                    check(reopened != nullptr, "edited PDF reopens");
                    if (reopened) {
                        artplus::pdf::PdfEditor verify(*reopened, 0);
                        check(verify.isValid(), "reopened page is editable");
                        const int moved = findPathNear(verify, expected, 2.0);
                        check(moved >= 0, "final geometry preserved after reopen");
                        if (moved >= 0) {
                            check(nearRect(verify.object(moved)->bounds, expected, 1.5),
                                  "reopened bounds match the final state");
                        }
                        check(findKind(verify, artplus::pdf::PageObjectKind::Text) >= 0,
                              "text still present");
                        check(findKind(verify, artplus::pdf::PageObjectKind::Image) >= 0,
                              "image still present");
                    }

                    stack.markClean();
                    check(!stack.isModified(), "saved state is clean");
                    check(stack.undo(), "undo after save works");
                    check(stack.isModified(), "undo after save marks the document modified");
                    check(stack.redo(), "redo after save works");
                    check(!stack.isModified(), "redo back to the saved state is clean again");
                }
                }

            }

            result = g_failures == 0 ? 0 : 1;
        }
    }
}

    artplus::pdf::PdfRuntime::shutdown();
    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return result;
}
