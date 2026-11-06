#include "artplus/ui/Theme.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QStyleFactory>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace artplus::ui::theme {

QFont uiFont(int pointSize, bool bold)
{
    QFont f(QStringLiteral("Segoe UI"));
    f.setPointSize(pointSize);
    f.setBold(bold);
    return f;
}

QFont captionFont()
{
    QFont f(QStringLiteral("Segoe UI"));
    f.setPointSize(8);
    return f;
}

void apply(QApplication& app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette p;
    p.setColor(QPalette::Window, kChrome);
    p.setColor(QPalette::WindowText, kText);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, kRowAlt);
    p.setColor(QPalette::Text, kText);
    p.setColor(QPalette::Button, kChrome);
    p.setColor(QPalette::ButtonText, kText);
    p.setColor(QPalette::BrightText, Qt::white);
    p.setColor(QPalette::Highlight, kAccent);
    p.setColor(QPalette::HighlightedText, kText);
    p.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xDC));
    p.setColor(QPalette::ToolTipText, kText);
    p.setColor(QPalette::Disabled, QPalette::Text, kTextDisabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, kTextDisabled);
    p.setColor(QPalette::Disabled, QPalette::WindowText, kTextDisabled);
    app.setPalette(p);
    app.setFont(uiFont(9));

    app.setStyleSheet(styleSheet());
}

QString styleSheet()
{
    return QStringLiteral(R"(
QMainWindow, QDialog { background: #F0F0F0; }
QMenuBar { background: #F0F0F0; border-bottom: 1px solid #D0D0D0; }
QMenuBar::item { padding: 3px 8px; background: transparent; }
QMenuBar::item:selected { background: #E4E4E4; }
QMenu { background: #FFFFFF; border: 1px solid #C8C8C8; padding: 2px; }
QMenu::item { padding: 4px 24px 4px 22px; }
QMenu::item:selected { background: #CFE3F7; color: #1E1E1E; }
QMenu::item:disabled { color: #A6A6A6; }
QMenu::separator { height: 1px; background: #E0E0E0; margin: 3px 6px; }

QToolBar { background: #F0F0F0; border: 0px; border-bottom: 1px solid #D0D0D0; spacing: 0px; padding: 2px 4px; }
QToolBar::separator { width: 1px; background: #D0D0D0; margin: 4px 6px; }

QToolButton { border: 0px; padding: 2px 5px; color: #1E1E1E; background: transparent; }
QToolButton:hover { background: #E4E4E4; border-radius: 3px; }
QToolButton:checked { background: #CFE3F7; border: 1px solid #9CC3E8; border-radius: 3px; }
QToolButton:disabled { color: #A6A6A6; }

QStatusBar { background: #F0F0F0; border-top: 1px solid #D0D0D0; }
QStatusBar::item { border: 0px; }

QDockWidget { border: 0px; titlebar-close-icon: none; titlebar-normal-icon: none; }
QDockWidget::title { background: #E8E8E8; padding: 2px; border-bottom: 1px solid #D0D0D0; }

QScrollArea { border: 0px; background: #F0F0F0; }
QScrollBar:vertical { background: #F0F0F0; width: 12px; margin: 0px; }
QScrollBar::handle:vertical { background: #C4C4C4; min-height: 24px; border-radius: 3px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar:horizontal { background: #F0F0F0; height: 12px; }
QScrollBar::handle:horizontal { background: #C4C4C4; min-width: 24px; border-radius: 3px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }

QComboBox { background: #FFFFFF; border: 1px solid #C8C8C8; padding: 1px 4px; }
QSpinBox { background: #FFFFFF; border: 1px solid #C8C8C8; padding: 1px 2px; }

QTabBar::tab { background: #E8E8E8; border: 1px solid #D0D0D0; padding: 3px 10px; }
QTabBar::tab:selected { background: #FFFFFF; border-bottom-color: #FFFFFF; }

QTreeWidget, QTreeView, QListWidget { background: #FFFFFF; border: 0px; }
QTreeView::item { height: 22px; }
QHeaderView::section {
    background: #ECECEC; border: 0px; border-right: 1px solid #D8D8D8;
    border-bottom: 1px solid #D8D8D8; padding: 2px 4px; font-weight: 600;
}
)");
}

namespace {

void drawEye(QPainter& p, const QRectF& r, bool open)
{
    QPen pen(p.pen());
    pen.setWidthF(1.2);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath eye;
    eye.moveTo(r.left(), r.center().y());
    eye.cubicTo(r.left() + r.width() * 0.25, r.top(),
                r.left() + r.width() * 0.75, r.top(),
                r.right(), r.center().y());
    eye.cubicTo(r.left() + r.width() * 0.75, r.bottom(),
                r.left() + r.width() * 0.25, r.bottom(),
                r.left(), r.center().y());
    p.drawPath(eye);
    if (open) {
        p.setBrush(p.pen().color());
        p.drawEllipse(r.center(), r.width() * 0.14, r.height() * 0.20);
    } else {
        p.drawLine(r.topLeft(), r.bottomRight());
    }
}

void drawGear(QPainter& p, const QRectF& r, const QColor& color)
{
    const QPointF c = r.center();
    const double outer = r.width() * 0.42;
    const double inner = r.width() * 0.30;
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    const int teeth = 8;
    QPainterPath path;
    for (int i = 0; i < teeth * 2; ++i) {
        const double angle = (M_PI * i) / teeth;
        const double radius = (i % 2 == 0) ? outer : inner;
        const QPointF pt(c.x() + std::cos(angle) * radius,
                         c.y() + std::sin(angle) * radius);
        if (i == 0)
            path.moveTo(pt);
        else
            path.lineTo(pt);
    }
    path.closeSubpath();
    p.drawPath(path);
    // Punch the hub out so the icon reads as a gear on any background.
    p.setCompositionMode(QPainter::CompositionMode_Clear);
    p.setBrush(Qt::black);
    p.drawEllipse(c, inner * 0.42, inner * 0.42);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
}

} // namespace

QIcon icon(IconKind kind, const QColor& color, int size)
{
    const int s = size;
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color);
    pen.setWidthF(std::max(1.2, s / 18.0));
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const QRectF box(s * 0.16, s * 0.16, s * 0.68, s * 0.68);

    switch (kind) {
    case IconKind::ObjectBrowser: {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        for (int i = 0; i < 3; ++i) {
            const double y = s * (0.24 + i * 0.20);
            p.drawRect(QRectF(s * 0.16, y, s * 0.10, s * 0.10));
            p.drawRect(QRectF(s * 0.32, y + s * 0.02, s * 0.52, s * 0.06));
        }
        break;
    }
    case IconKind::Colors: {
        p.setPen(Qt::NoPen);
        const double cell = s * 0.18;
        const QColor swatches[9] = {
            QColor(0x00, 0x9B, 0xD4), QColor(0xE5, 0x00, 0x7E), QColor(0xFF, 0xE8, 0x00),
            QColor(0x00, 0x00, 0x00), QColor(0x8A, 0x8A, 0x8A), QColor(0xFF, 0xFF, 0xFF),
            QColor(0xE0, 0x4A, 0x2A), QColor(0x2E, 0x9E, 0x5B), QColor(0x2F, 0x6F, 0xD0)};
        for (int i = 0; i < 9; ++i) {
            const int cx = i % 3;
            const int cy = i / 3;
            p.setBrush(swatches[i]);
            p.drawRect(QRectF(s * 0.16 + cx * cell, s * 0.16 + cy * cell, cell - 1, cell - 1));
        }
        break;
    }
    case IconKind::Images: {
        p.drawRect(box);
        p.setBrush(color);
        QPainterPath mountain;
        mountain.moveTo(box.left() + 2, box.bottom() - 2);
        mountain.lineTo(box.left() + box.width() * 0.35, box.top() + box.height() * 0.45);
        mountain.lineTo(box.left() + box.width() * 0.55, box.bottom() - 2);
        p.drawPath(mountain);
        p.drawEllipse(QPointF(box.left() + box.width() * 0.75, box.top() + box.height() * 0.28),
                      s * 0.05, s * 0.05);
        break;
    }
    case IconKind::PlacedArt: {
        QPainterPath page;
        page.moveTo(box.left(), box.top());
        page.lineTo(box.right() - box.width() * 0.3, box.top());
        page.lineTo(box.right(), box.top() + box.height() * 0.3);
        page.lineTo(box.right(), box.bottom());
        page.lineTo(box.left(), box.bottom());
        page.closeSubpath();
        p.drawPath(page);
        p.drawLine(QPointF(box.right() - box.width() * 0.3, box.top()), box.topRight());
        p.drawLine(box.topRight(), QPointF(box.right() - box.width() * 0.3, box.top() + box.height() * 0.3));
        break;
    }
    case IconKind::Messages: {
        p.setPen(Qt::NoPen);
        p.setBrush(kError);
        p.drawEllipse(box);
        p.setPen(QPen(Qt::white, std::max(1.6, s / 12.0)));
        const QRectF x = box.adjusted(box.width() * 0.28, box.height() * 0.28,
                                      -box.width() * 0.28, -box.height() * 0.28);
        p.drawLine(x.topLeft(), x.bottomRight());
        p.drawLine(x.topRight(), x.bottomLeft());
        break;
    }
    case IconKind::ActionLists: {
        drawGear(p, box, color);
        break;
    }
    case IconKind::ShareApprove: {
        p.setBrush(color);
        QPainterPath cloud;
        cloud.addEllipse(QPointF(box.center().x() - box.width() * 0.18, box.center().y() + box.height() * 0.05),
                         box.width() * 0.26, box.height() * 0.20);
        cloud.addEllipse(QPointF(box.center().x() + box.width() * 0.18, box.center().y() + box.height() * 0.05),
                         box.width() * 0.26, box.height() * 0.20);
        cloud.addEllipse(QPointF(box.center().x(), box.center().y() - box.height() * 0.12),
                         box.width() * 0.30, box.height() * 0.24);
        p.drawPath(cloud.simplified());
        break;
    }
    case IconKind::Clean: {
        QPolygonF star;
        const QPointF c = box.center();
        const double R = box.width() * 0.45;
        const double r = R * 0.28;
        for (int i = 0; i < 8; ++i) {
            const double a = M_PI * i / 4.0 - M_PI / 2.0;
            const double rad = (i % 2 == 0) ? R : r;
            star << QPointF(c.x() + std::cos(a) * rad, c.y() + std::sin(a) * rad);
        }
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawPolygon(star);
        break;
    }
    case IconKind::Select: {
        QPen dashed(color, std::max(1.2, s / 18.0), Qt::DashLine);
        p.setPen(dashed);
        p.drawRect(box);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const double h = s * 0.07;
        const QPointF corners[4] = {box.topLeft(), box.topRight(), box.bottomLeft(), box.bottomRight()};
        for (const QPointF& c : corners)
            p.drawRect(QRectF(c.x() - h / 2, c.y() - h / 2, h, h));
        break;
    }
    case IconKind::Recolor: {
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(box.center().x() - box.width() * 0.14, box.center().y()),
                      box.width() * 0.30, box.height() * 0.30);
        p.drawEllipse(QPointF(box.center().x() + box.width() * 0.14, box.center().y()),
                      box.width() * 0.30, box.height() * 0.30);
        break;
    }
    case IconKind::Shaper: {
        p.drawRect(QRectF(box.left(), box.top(), box.width() * 0.65, box.height() * 0.65));
        p.drawEllipse(QPointF(box.right() - box.width() * 0.25, box.bottom() - box.height() * 0.25),
                      box.width() * 0.32, box.height() * 0.32);
        break;
    }
    case IconKind::Barcode: {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const int bars = 9;
        for (int i = 0; i < bars; ++i) {
            const double w = (i % 3 == 0) ? s * 0.055 : s * 0.028;
            const double x = box.left() + i * (box.width() / bars);
            p.drawRect(QRectF(x, box.top(), w, box.height()));
        }
        break;
    }
    case IconKind::Screening: {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                const double d = s * (0.035 + 0.012 * (3 - y));
                p.drawEllipse(QPointF(box.left() + x * (box.width() / 3.4),
                                      box.top() + y * (box.height() / 3.4)), d, d);
            }
        }
        break;
    }
    case IconKind::Trapping: {
        p.drawRoundedRect(QRectF(box.left(), box.top(), box.width() * 0.62, box.height() * 0.62), 2, 2);
        p.drawRoundedRect(QRectF(box.left() + box.width() * 0.34, box.top() + box.height() * 0.34,
                                 box.width() * 0.62, box.height() * 0.62), 2, 2);
        break;
    }
    case IconKind::Wrap: {
        p.drawEllipse(box);
        p.drawArc(box, 0, 180 * 16);
        p.drawArc(box, 180 * 16, 180 * 16);
        p.drawLine(box.left(), box.center().y(), box.right(), box.center().y());
        break;
    }
    case IconKind::ThreeD: {
        const QRectF front(box.left(), box.top() + box.height() * 0.25,
                           box.width() * 0.68, box.height() * 0.68);
        p.drawRect(front);
        p.drawLine(front.topLeft(), QPointF(box.right() - box.width() * 0.18, box.top()));
        p.drawLine(front.topRight(), QPointF(box.right(), box.top() + box.height() * 0.18));
        p.drawLine(front.bottomRight(), QPointF(box.right(), box.top() + box.height() * 0.86));
        p.drawLine(QPointF(box.right() - box.width() * 0.18, box.top()),
                   QPointF(box.right(), box.top() + box.height() * 0.18));
        break;
    }
    case IconKind::Preview: {
        drawEye(p, box, true);
        break;
    }
    case IconKind::Eye: {
        drawEye(p, box, true);
        break;
    }
    case IconKind::EyeClosed: {
        drawEye(p, box, false);
        break;
    }
    case IconKind::Lock: {
        p.drawRect(QRectF(box.left() + box.width() * 0.15, box.center().y() - box.height() * 0.05,
                          box.width() * 0.7, box.height() * 0.55));
        p.drawArc(QRectF(box.left() + box.width() * 0.28, box.top() + box.height() * 0.10,
                         box.width() * 0.44, box.height() * 0.5), 0, 180 * 16);
        break;
    }
    case IconKind::LockOpen: {
        p.drawRect(QRectF(box.left() + box.width() * 0.15, box.center().y() - box.height() * 0.05,
                          box.width() * 0.7, box.height() * 0.55));
        p.drawArc(QRectF(box.left() + box.width() * 0.28, box.top() + box.height() * 0.10,
                         box.width() * 0.44, box.height() * 0.5), 30 * 16, 150 * 16);
        break;
    }
    case IconKind::Printer: {
        p.drawRect(QRectF(box.left(), box.top() + box.height() * 0.22,
                          box.width(), box.height() * 0.38));
        p.setBrush(Qt::white);
        p.drawRect(QRectF(box.left() + box.width() * 0.22, box.top(),
                          box.width() * 0.56, box.height() * 0.28));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(box.left() + box.width() * 0.22, box.top() + box.height() * 0.60,
                          box.width() * 0.56, box.height() * 0.30));
        break;
    }
    case IconKind::Check: {
        p.setPen(QPen(color, std::max(1.6, s / 10.0)));
        p.drawLine(QPointF(box.left() + box.width() * 0.10, box.center().y()),
                   QPointF(box.left() + box.width() * 0.38, box.bottom() - box.height() * 0.12));
        p.drawLine(QPointF(box.left() + box.width() * 0.38, box.bottom() - box.height() * 0.12),
                   QPointF(box.right(), box.top() + box.height() * 0.15));
        break;
    }
    case IconKind::SquareOutline: {
        p.drawRect(box);
        break;
    }
    case IconKind::Plus: {
        p.setPen(QPen(color, std::max(1.4, s / 12.0)));
        p.drawLine(QPointF(box.center().x(), box.top()), QPointF(box.center().x(), box.bottom()));
        p.drawLine(QPointF(box.left(), box.center().y()), QPointF(box.right(), box.center().y()));
        break;
    }
    case IconKind::Minus: {
        p.setPen(QPen(color, std::max(1.4, s / 12.0)));
        p.drawLine(QPointF(box.left(), box.center().y()), QPointF(box.right(), box.center().y()));
        break;
    }
    case IconKind::LayersOrder: {
        p.setPen(QPen(color, std::max(1.0, s / 20.0)));
        p.drawLine(QPointF(box.left(), box.top() + box.height() * 0.2),
                   QPointF(box.left() + box.width() * 0.5, box.top() + box.height() * 0.2));
        p.drawLine(QPointF(box.left() + box.width() * 0.1, box.center().y()),
                   QPointF(box.right(), box.center().y()));
        p.drawLine(QPointF(box.left(), box.bottom() - box.height() * 0.2),
                   QPointF(box.left() + box.width() * 0.6, box.bottom() - box.height() * 0.2));
        break;
    }
    case IconKind::Gear: {
        drawGear(p, box, color);
        break;
    }
    case IconKind::Info: {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x2F, 0x6F, 0xD0));
        p.drawEllipse(box);
        p.setPen(QPen(Qt::white, std::max(1.4, s / 12.0)));
        p.drawPoint(QPointF(box.center().x(), box.top() + box.height() * 0.28));
        p.drawLine(QPointF(box.center().x(), box.top() + box.height() * 0.44),
                   QPointF(box.center().x(), box.bottom() - box.height() * 0.18));
        break;
    }
    case IconKind::Warning: {
        p.setPen(Qt::NoPen);
        p.setBrush(kWarning);
        QPolygonF tri;
        tri << QPointF(box.center().x(), box.top())
            << QPointF(box.right(), box.bottom())
            << QPointF(box.left(), box.bottom());
        p.drawPolygon(tri);
        p.setPen(QPen(Qt::black, std::max(1.3, s / 14.0)));
        p.drawLine(QPointF(box.center().x(), box.top() + box.height() * 0.35),
                   QPointF(box.center().x(), box.bottom() - box.height() * 0.32));
        p.drawPoint(QPointF(box.center().x(), box.bottom() - box.height() * 0.18));
        break;
    }
    }

    p.end();
    return QIcon(pm);
}

} // namespace artplus::ui::theme
