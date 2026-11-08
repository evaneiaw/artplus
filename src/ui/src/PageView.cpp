#include "artplus/ui/PageView.h"

#include "artplus/ui/Theme.h"

#include <QEnterEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace artplus::ui {

namespace {
constexpr double kMinZoom = 0.02;
constexpr double kMaxZoom = 64.0;
constexpr int kMargin = 24;
constexpr int kChipHeight = 20;

QString mmText(double points)
{
    const double mm = points * 25.4 / 72.0;
    return QString::number(mm, 'f', 2).replace(QLatin1Char('.'), QLatin1Char(','));
}
} // namespace

PageView::PageView(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAutoFillBackground(false);
    setMinimumSize(200, 200);
}

void PageView::setPageSize(const QSizeF& sizePoints)
{
    if (sizePoints.width() <= 0.0 || sizePoints.height() <= 0.0)
        return;
    m_pageSizePoints = sizePoints;
    clampPan();
    update();
}

void PageView::setPageImage(const QImage& image)
{
    m_image = image;
    update();
}

void PageView::clearPage()
{
    m_image = QImage();
    update();
}

void PageView::setZoom(double scale)
{
    const double clamped = std::clamp(scale, kMinZoom, kMaxZoom);
    if (std::abs(clamped - m_scale) < 1e-9)
        return;
    m_scale = clamped;
    clampPan();
    update();
    notifyStatus();
    requestRender();
}

QRectF PageView::centeredPageRect(double scale) const
{
    const double pageW = m_pageSizePoints.width() * scale;
    const double pageH = m_pageSizePoints.height() * scale;
    const double x = (width() - pageW) / 2.0 + m_pan.x();
    const double y = (height() - pageH) / 2.0 + m_pan.y();
    return QRectF(x, y, pageW, pageH);
}

QRectF PageView::pageRect() const
{
    return centeredPageRect(m_scale);
}

void PageView::fitPage()
{
    const double availW = std::max(1.0, static_cast<double>(width() - 2 * kMargin));
    const double availH = std::max(1.0, static_cast<double>(height() - 2 * kMargin - kChipHeight));
    const double pageW = std::max(1e-6, m_pageSizePoints.width());
    const double pageH = std::max(1e-6, m_pageSizePoints.height());

    // m_pageSizePoints is the *displayed* page size: MainWindow already swaps
    // the width/height for 90/270 degree view rotation (displayedPageSize()),
    // and PDFium renders the image pre-rotated. Swapping again here would
    // double-apply the rotation.
    double scale = std::min(availW / pageW, availH / pageH);
    scale = std::clamp(scale, kMinZoom, kMaxZoom);

    m_scale = scale;
    m_pan = QPointF(0.0, 0.0);
    m_fitOnResize = true;
    update();
    notifyStatus();
    requestRender();
}

void PageView::fitWidth()
{
    const double availW = std::max(1.0, static_cast<double>(width() - 2 * kMargin));
    // Already the displayed (post-rotation) size; see the note in fitPage().
    const double visualW = m_pageSizePoints.width();
    if (visualW <= 1e-6)
        return;
    m_scale = std::clamp(availW / visualW, kMinZoom, kMaxZoom);
    m_pan = QPointF(0.0, 0.0);
    m_fitOnResize = false;
    update();
    notifyStatus();
    requestRender();
}

void PageView::actualSize()
{
    m_scale = 96.0 / 72.0;
    m_pan = QPointF(0.0, 0.0);
    m_fitOnResize = false;
    update();
    notifyStatus();
    requestRender();
}

void PageView::zoomToSelection(const QRectF& selection)
{
    if (selection.width() <= 1e-6 || selection.height() <= 1e-6)
        return;

    const double margin = 0.85;
    const double sx = (width() * margin) / selection.width();
    const double sy = (height() * margin) / selection.height();
    m_scale = std::clamp(std::min(sx, sy), kMinZoom, kMaxZoom);
    m_fitOnResize = false;

    // Centre the selection in the viewport.
    const QRectF page = centeredPageRect(m_scale);
    const QPointF centerInWidget =
        page.topLeft() + QPointF(selection.center().x() * m_scale,
                                  selection.center().y() * m_scale);
    m_pan += QPointF(width() / 2.0, height() / 2.0) - centerInWidget;

    clampPan();
    update();
    notifyStatus();
    requestRender();
}
void PageView::setViewRotation(int degrees)
{
    int r = degrees % 360;
    if (r < 0)
        r += 360;
    r = ((r + 45) / 90 * 90) % 360;
    if (r == m_viewRotation)
        return;
    m_viewRotation = r;
    m_pan = QPointF(0.0, 0.0);
    if (m_fitOnResize)
        fitPage();
    else {
        update();
        requestRender();
    }
}

void PageView::rotateView(int delta)
{
    setViewRotation(m_viewRotation + delta);
}

void PageView::setSelectionRect(const QRectF& pageRect)
{
    setSelectionRects({pageRect});
}

void PageView::setSelectionRects(const QVector<QRectF>& pageRects)
{
    m_selections = pageRects;
    m_hasSelection = !pageRects.isEmpty();
    update();
}

void PageView::clearSelection()
{
    if (!m_hasSelection)
        return;
    m_hasSelection = false;
    update();
}

void PageView::setPageBoxesVisible(bool visible)
{
    if (m_pageBoxes == visible)
        return;
    m_pageBoxes = visible;
    update();
}

void PageView::setCrosshairEnabled(bool enabled)
{
    if (m_crosshair == enabled)
        return;
    m_crosshair = enabled;
    updateCursorShape();
}

void PageView::updateCursorShape()
{
    if (m_dragging) {
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    setCursor(m_crosshair ? Qt::CrossCursor : Qt::ArrowCursor);
}

void PageView::enterEvent(QEnterEvent* event)
{
    updateCursorShape();
    QWidget::enterEvent(event);
}

void PageView::clampPan()
{
    const QRectF rect = centeredPageRect(m_scale);
    const double maxDx = (rect.width() + width()) / 2.0 + kMargin;
    const double maxDy = (rect.height() + height()) / 2.0 + kMargin;
    m_pan.setX(std::clamp(m_pan.x(), -maxDx, maxDx));
    m_pan.setY(std::clamp(m_pan.y(), -maxDy, maxDy));
}

void PageView::drawPageBoxChips(QPainter& p, const QRectF& page) const
{
    QFont f = theme::uiFont(8, true);
    p.setFont(f);
    const QFontMetrics fm(f);

    struct Chip {
        QString text;
        QColor color;
    };
    const Chip chips[2] = {{QStringLiteral("Media Box"), theme::kMediaBox},
                           {QStringLiteral("Trim Box"), theme::kTrimBox}};

    const QString sizeText = QStringLiteral("%1 x %2 mm")
                             .arg(mmText(m_pageSizePoints.width()),
                                  mmText(m_pageSizePoints.height()));

    const double barTop = page.top() - kChipHeight;
    double x = page.left();
    const double h = kChipHeight;

    for (const Chip& chip : chips) {
        const double w = fm.horizontalAdvance(chip.text) + 18;
        const QRectF r(x, barTop, w, h);
        p.fillRect(r, chip.color);
        p.setPen(Qt::white);
        p.drawText(r, Qt::AlignCenter, chip.text);
        x += w;
    }

    const double sizeW = fm.horizontalAdvance(sizeText) + 20;
    const QRectF sizeRect(x, barTop, sizeW, h);
    p.fillRect(sizeRect, QColor(0x3C, 0x3C, 0x3C));
    p.setPen(Qt::white);
    p.drawText(sizeRect, Qt::AlignCenter, sizeText);
}

void PageView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), theme::kCanvas);

    if (m_pageSizePoints.width() <= 0.0 || m_pageSizePoints.height() <= 0.0)
        return;

    const QRectF page = pageRect();

    // Box labels live above the page, drawn first so the page covers overlap.
    if (m_pageBoxes)
        drawPageBoxChips(p, page);

    // Page background and image. The image arrives already rotated for the
    // current view rotation (PDFium renders it that way), and `page` already
    // has the rotated dimensions, so it is drawn 1:1 into the page rectangle.
    p.fillRect(page, Qt::white);
    if (!m_image.isNull()) {
        p.save();
        p.setClipRect(page);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawImage(page, m_image);
        p.restore();
    }

    // Selection highlight (page points -> widget coordinates).
    if (m_hasSelection) {
        QPen pen(QColor(0x1F, 0x6F, 0xD0));
        pen.setWidthF(1.8);
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        for (const QRectF& rect : m_selections) {
            const QRectF widget(page.left() + rect.left() * m_scale,
                                page.top() + rect.top() * m_scale,
                                rect.width() * m_scale,
                                rect.height() * m_scale);
            p.drawRect(widget);
        }
    }

    // Crosshair follows the cursor across the whole viewport.
    if (m_crosshair && m_lastMousePos.x() >= 0 && m_lastMousePos.y() >= 0) {
        QPen pen(QColor(0x60, 0x60, 0x60, 0xB0));
        pen.setWidthF(1.0);
        p.setPen(pen);
        const double cx = m_lastMousePos.x();
        const double cy = m_lastMousePos.y();
        p.drawLine(QPointF(cx, 0.0), QPointF(cx, height()));
        p.drawLine(QPointF(0.0, cy), QPointF(width(), cy));
    }
}

void PageView::wheelEvent(QWheelEvent* event)
{
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        event->ignore();
        return;
    }

    const double target =
        std::clamp(m_scale * std::pow(1.0015, static_cast<double>(delta)), kMinZoom, kMaxZoom);
    if (std::abs(target - m_scale) < 1e-9) {
        event->accept();
        return;
    }

    // Keep the page point under the cursor pinned while zooming.
    const QRectF before = pageRect();
    const QPointF cursor = event->position();
    const double u = before.width() > 1e-9 ? (cursor.x() - before.left()) / before.width() : 0.5;
    const double v = before.height() > 1e-9 ? (cursor.y() - before.top()) / before.height() : 0.5;

    m_scale = target;
    const double w = m_pageSizePoints.width() * m_scale;
    const double h = m_pageSizePoints.height() * m_scale;
    m_pan.setX(cursor.x() - u * w - (width() - w) / 2.0);
    m_pan.setY(cursor.y() - v * h - (height() - h) / 2.0);
    m_fitOnResize = false;

    clampPan();
    update();
    notifyStatus();
    requestRender();
    event->accept();
}

void PageView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_draggedSincePress = false;
        m_pressPos = event->position();
        m_lastMousePos = event->position().toPoint();
        updateCursorShape();
        update();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void PageView::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF pos = event->position();

    if (m_dragging) {
        const QPointF delta = pos - m_lastMousePos;
        m_pan += delta;
        m_lastMousePos = pos.toPoint();
        if ((pos - m_pressPos).manhattanLength() > 8.0)
            m_draggedSincePress = true;
        clampPan();
        update();
        notifyStatus();
        event->accept();
        return;
    }

    m_lastMousePos = pos.toPoint();
    if (m_cursorCb && m_pageSizePoints.width() > 0.0) {
        const QRectF page = pageRect();
        m_cursorCb(QPointF((pos.x() - page.left()) / m_scale,
                           (pos.y() - page.top()) / m_scale));
    }
    if (m_crosshair)
        update();
    event->accept();
}

void PageView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        updateCursorShape();
        update();
        if (!m_draggedSincePress && m_clickCb && m_pageSizePoints.width() > 0.0) {
            const QRectF page = pageRect();
            const QPointF pt((event->position().x() - page.left()) / m_scale,
                             (event->position().y() - page.top()) / m_scale);
            m_clickCb(pt);
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void PageView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_fitOnResize && m_pageSizePoints.width() > 0.0) {
        fitPage();
    } else {
        clampPan();
        update();
    }
}

void PageView::notifyStatus()
{
    if (m_statusCb)
        m_statusCb();
}

void PageView::requestRender()
{
    if (m_requestRender)
        m_requestRender();
}

} // namespace artplus::ui

