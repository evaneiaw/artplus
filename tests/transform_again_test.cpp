// Test for Transform Again (Ctrl+D) functionality.
// This test verifies that the last transform parameters are cached and
// can be reapplied on a new selection.

#include "fixture_builder.h"

#include "artplus/edit/Command.h"
#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfEditor.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <cmath>
#include <cstdio>
#include <memory>
#include <tuple>
#include <vector>

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

    std::printf("ArtPlus Transform Again test\n");
    std::printf("==============================\n");

    artplus::pdf::PdfRuntime::initialize();

    int result = 1;
    {
        const QString fixture = QDir::current().filePath(QStringLiteral("artplus_transform_fixture.pdf"));
        QFile::remove(fixture);
        check(artplus_test::writeEditFixture(fixture), "fixture PDF written");

        QString error;
        auto doc = artplus::pdf::PdfDocument::open(fixture, &error);
        check(doc != nullptr, "fixture opens");
        if (doc) {
            artplus::pdf::PdfEditor editor(*doc, 0);
            artplus::edit::CommandStack stack;

            check(editor.isValid(), "editor opened page 1");

            const int rect = findPathNear(editor, QRectF(100, 192, 150, 100));
            check(rect >= 0, "rectangle found");
            if (rect < 0) {
                std::printf("  [SKIP] rectangle not found\n");
                return 1;
            }

            QRectF original = editor.object(rect)->bounds;

            // Test 1: Apply a move transformation and verify it's cached.
            std::vector<int> indices{rect};
            artplus::edit::TransformParams moveParams;
            moveParams.moveX = 100;
            moveParams.moveY = 50;
            auto moveCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                editor, indices, moveParams, artplus::edit::TransformAnchor::TopLeft, false);
            check(moveCmd->hasEffect() && stack.execute(std::move(moveCmd)),
                  "initial transform (move) applied successfully");

            auto cached = stack.lastTransform();
            check(cached.has_value(), "transform params are cached for again");
            auto [params, anchor, isCopy] = cached.value();
            check(!isCopy, "isCopy is false for normal transform");
            check(nearRect(editor.object(rect)->bounds,
                           QRectF(original.x() + 100, original.y() + 50, original.width(),
                                  original.height()),
                           1.0),
                  "move transformation applied correctly");

            // Test 2: Undo should restore the rectangle.
            check(stack.undo(), "undo works");
            check(nearRect(editor.object(rect)->bounds, original, 1.0),
                  "undo restored original bounds");

            // Test 3: Redo should restore the transformed rectangle.
            check(stack.redo(), "redo works");
            check(nearRect(editor.object(rect)->bounds,
                           QRectF(original.x() + 100, original.y() + 50, original.width(),
                                  original.height()),
                           1.0),
                  "redo restored transformed bounds");

            // Test 4: Clear history, verify cache state.
            QRectF afterMove = editor.object(rect)->bounds;
            stack.clear();
            check(!stack.hasLastTransform(), "cleared history has no cached transform");

            // Test 5: Apply scale and verify it's cached.
            artplus::edit::TransformParams scaleParams;
            scaleParams.scaleX = 2.0;
            scaleParams.scaleY = 2.0;
            auto scaleCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                editor, indices, scaleParams, artplus::edit::TransformAnchor::Center, false);
            check(scaleCmd->hasEffect() && stack.execute(std::move(scaleCmd)),
                  "scale transform applied");

            cached = stack.lastTransform();
            check(cached.has_value(), "scale params cached");
            std::tie(params, anchor, isCopy) = cached.value();
            check(!isCopy, "isCopy is false for scale transform");
            check(std::abs(editor.object(rect)->bounds.width() - afterMove.width() * 2) < 2.0,
                  "scale transform doubled the width");
            check(std::abs(editor.object(rect)->bounds.height() - afterMove.height() * 2) < 2.0,
                  "scale transform doubled the height");

            // Test 6: Apply Transform Again with the cached scale params.
            auto againScale = std::make_unique<artplus::edit::TransformObjectsCommand>(
                editor, indices, params, anchor, isCopy);
            check(againScale->hasEffect() && stack.execute(std::move(againScale)),
                  "transform again (scale) applied");
            check(std::abs(editor.object(rect)->bounds.width() - afterMove.width() * 4) < 3.0,
                  "repeated scale resulted in 4x width");
            check(std::abs(editor.object(rect)->bounds.height() - afterMove.height() * 4) < 3.0,
                  "repeated scale resulted in 4x height");

            // Test 7: Undo and redo the Transform Again.
            check(stack.undo(), "undo after transform again");
            check(std::abs(editor.object(rect)->bounds.width() - afterMove.width() * 2) < 2.0,
                  "undo restored to 2x width");
            check(stack.redo(), "redo after transform again");
            check(std::abs(editor.object(rect)->bounds.width() - afterMove.width() * 4) < 3.0,
                  "redo restored to 4x width");

            // Test 8: Transform with isCopy=true (Alt+Transform behavior).
            artplus::edit::TransformParams copyParams;
            copyParams.moveX = 200;
            auto copyCmd = std::make_unique<artplus::edit::TransformObjectsCommand>(
                editor, indices, copyParams, artplus::edit::TransformAnchor::TopLeft, true);
            check(copyCmd->hasEffect() && stack.execute(std::move(copyCmd)),
                  "copy transform applied");

            cached = stack.lastTransform();
            check(cached.has_value(), "copy params cached");
            std::tie(params, anchor, isCopy) = cached.value();
            check(isCopy, "isCopy is true for copy transform");

            // Clean up for subsequent tests.
            stack.clear();
            result = g_failures == 0 ? 0 : 1;
        }
    }

    artplus::pdf::PdfRuntime::shutdown();
    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return result;
}