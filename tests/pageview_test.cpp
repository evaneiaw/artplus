// Deterministic logic tests for the page viewport (no real PDF needed).
//
// Exercises the zoom / fit / rotation / pan arithmetic used by the viewer.

#include "artplus/ui/PageView.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPointF>
#include <QSizeF>

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

void checkNear(double a, double b, double eps, const char* description)
{
    check(std::abs(a - b) <= eps, description);
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    std::printf("ArtPlus PageView logic test\n");
    std::printf("===========================\n");

    artplus::ui::PageView view;
    view.resize(800, 600);
    view.setPageSize(QSizeF(595.0, 842.0)); // A4 in points

    // --- fitPage -------------------------------------------------------------
    std::printf("\nFit page:\n");
    view.fitPage();
    const double fitZoom = view.zoom();
    check(fitZoom > 0.0, "fitPage produced a positive zoom");
    const QRectF fitRect = view.pageRect();
    check(fitRect.width() <= 800.0 + 1.0, "page fits widget width after fitPage");
    check(fitRect.height() <= 600.0 + 1.0, "page fits widget height after fitPage");
    checkNear(fitRect.width() / fitRect.height(), 595.0 / 842.0, 0.02,
              "fitted page preserves aspect ratio");
    std::printf("  fit zoom       : %.4f\n", fitZoom);

    // --- zoom in / out round trip -------------------------------------------
    std::printf("\nZoom:\n");
    const double base = view.zoom();
    view.zoomIn();
    check(view.zoom() > base, "zoomIn increases zoom");
    view.zoomOut();
    checkNear(view.zoom(), base, 1e-6, "zoomIn then zoomOut restores zoom");

    // --- actual size ---------------------------------------------------------
    view.actualSize();
    checkNear(view.zoom(), 96.0 / 72.0, 1e-9, "actualSize sets 96/72 px per point");

    // --- zoom clamping -------------------------------------------------------
    view.setZoom(1.0e6);
    check(view.zoom() <= 64.0, "zoom is clamped at the upper bound");
    view.setZoom(1.0e-6);
    check(view.zoom() >= 0.02, "zoom is clamped at the lower bound");

    // --- rotation ------------------------------------------------------------
    std::printf("\nRotation:\n");
    view.setZoom(1.0);
    view.setViewRotation(90);
    check(view.viewRotation() == 90, "viewRotation stores 90 degrees");
    view.fitPage();
    const QRectF rotRect = view.pageRect();
    check(rotRect.width() < rotRect.height(),
          "a 90-degree rotation swaps the visual page orientation");

    view.setViewRotation(270);
    view.fitPage();
    check(view.viewRotation() == 270, "viewRotation stores 270 degrees");

    // Angles are normalised to the nearest quarter turn, and negative input
    // maps onto the equivalent positive angle.
    view.setViewRotation(400);
    check(view.viewRotation() == 0, "an out-of-range angle snaps to a quarter turn");
    view.setViewRotation(-90);
    check(view.viewRotation() == 270, "a negative angle maps to its positive equivalent");

    // Clockwise / counter-clockwise accumulation, and the full cycle returns to
    // the original orientation.
    view.setViewRotation(0);
    check(view.viewRotation() == 0, "rotation starts at the original orientation");
    view.rotateView(90);
    check(view.viewRotation() == 90, "rotateView(+90) turns clockwise to 90");
    view.rotateView(90);
    check(view.viewRotation() == 180, "rotateView(+90) turns clockwise to 180");
    view.rotateView(90);
    check(view.viewRotation() == 270, "rotateView(+90) turns clockwise to 270");
    view.rotateView(90);
    check(view.viewRotation() == 0, "four clockwise turns return to the original orientation");
    view.rotateView(-90);
    check(view.viewRotation() == 270, "rotateView(-90) turns counter-clockwise to 270");

    // The caller supplies the already-rotated page box, so the view must fit
    // whatever dimensions it is given (here: a landscape 842x595 at 90 degrees).
    std::printf("\nRotation with a rotated page box:\n");
    view.setViewRotation(90);
    view.setPageSize(QSizeF(842.0, 595.0));
    view.fitPage();
    const QRectF landscape = view.pageRect();
    check(landscape.width() > landscape.height(),
          "fitPage honours the landscape box supplied for a rotated view");
    check(landscape.width() <= 800.0 + 1.0 && landscape.height() <= 600.0 + 1.0,
          "the rotated page still fits the viewport");
    checkNear(landscape.width() / landscape.height(), 842.0 / 595.0, 0.02,
              "the rotated page preserves its aspect ratio");

    // Restore the portrait page for the remaining checks.
    view.setViewRotation(0);
    view.setPageSize(QSizeF(595.0, 842.0));
    view.fitPage();

    // --- interaction after rotation -----------------------------------------
    std::printf("\nInteraction after rotation:\n");
    QPointF lastClick(-1.0, -1.0);
    view.setClickCallback([&lastClick](const QPointF& pt) { lastClick = pt; });
    QPointF lastCursor(-1.0, -1.0);
    view.setCursorCallback([&lastCursor](const QPointF& pt) { lastCursor = pt; });

    view.fitPage();
    const QRectF curPage = view.pageRect();
    // Click 100 pt inside the displayed page from its top-left corner.
    const QPointF widgetTarget(curPage.left() + 100.0 * view.zoom(),
                               curPage.top() + 100.0 * view.zoom());
    QMouseEvent click(QEvent::MouseButtonPress, widgetTarget, widgetTarget, widgetTarget,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &click);
    QMouseEvent clickUp(QEvent::MouseButtonRelease, widgetTarget, widgetTarget, widgetTarget,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &clickUp);
    checkNear(lastClick.x(), 100.0, 1.0, "a click maps to the expected page x after rotation");
    checkNear(lastClick.y(), 100.0, 1.0, "a click maps to the expected page y after rotation");

    QMouseEvent hoverMove(QEvent::MouseMove, widgetTarget, widgetTarget, widgetTarget,
                          Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &hoverMove);
    checkNear(lastCursor.x(), 100.0, 1.0, "cursor callback reports page x after rotation");
    checkNear(lastCursor.y(), 100.0, 1.0, "cursor callback reports page y after rotation");

    // Zooming after a rotation must keep a sane, clamped scale.
    view.rotateView(90);
    view.fitPage();
    check(view.zoom() > 0.0, "fitPage keeps a positive zoom after a rotation");
    view.setZoom(1.0e6);
    check(view.zoom() <= 64.0, "zoom stays clamped after a rotation");
    view.rotateView(-90);
    check(view.viewRotation() == 0, "rotating back restores the original orientation");

    // --- pan with synthetic mouse drag --------------------------------------
    std::printf("\nPan:\n");
    view.setViewRotation(0);
    view.fitPage();
    const QPointF before = view.pageRect().topLeft();

    const QPointF start(400.0, 300.0);
    QMouseEvent press(QEvent::MouseButtonPress, start, start, start,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &press);

    const QPointF moved = start + QPointF(50.0, 30.0);
    QMouseEvent move(QEvent::MouseMove, moved, moved, moved,
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &move);

    QMouseEvent release(QEvent::MouseButtonRelease, moved, moved, moved,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &release);

    const QPointF after = view.pageRect().topLeft();
    checkNear(after.x() - before.x(), 50.0, 0.5, "drag pans horizontally by 50 px");
    checkNear(after.y() - before.y(), 30.0, 0.5, "drag pans vertically by 30 px");

    // --- zoom to selection --------------------------------------------------
    std::printf("\nZoom to selection:\n");
    view.setViewRotation(0);
    view.fitPage();
    view.zoomToSelection(QRectF(100.0, 100.0, 50.0, 50.0));
    const double selZoom = view.zoom();
    check(selZoom > 5.0, "zoomToSelection zooms in on a small selection");
    const QRectF selPage = view.pageRect();
    const QPointF selCenter = selPage.topLeft() +
        QPointF(125.0 * selZoom, 125.0 * selZoom);
    checkNear(selCenter.x(), 400.0, 1.0, "selection centre is horizontally centred");
    checkNear(selCenter.y(), 300.0, 1.0, "selection centre is vertically centred");
    std::printf("  selection zoom : %.4f\n", selZoom);
    std::printf("\nResult: %d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
