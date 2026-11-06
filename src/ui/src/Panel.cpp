#include "artplus/ui/Panel.h"

#include "artplus/ui/Theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace artplus::ui {

CollapsiblePanel::CollapsiblePanel(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setCursor(Qt::PointingHandCursor);

    m_body = new QWidget(this);
    m_body->setAutoFillBackground(false);
    m_body->setStyleSheet(QStringLiteral("background: #FFFFFF;"));

    m_bodyLayout = new QVBoxLayout(m_body);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(0);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addSpacing(kHeaderHeight);
    outer->addWidget(m_body);

    setStyleSheet(QStringLiteral("background: #FFFFFF;"));
}

QVBoxLayout* CollapsiblePanel::bodyLayout() const
{
    return m_bodyLayout;
}

QVBoxLayout* CollapsiblePanel::contentLayout() const
{
    return m_bodyLayout;
}

void CollapsiblePanel::setExpanded(bool expanded)
{
    if (m_expanded == expanded)
        return;
    m_expanded = expanded;
    m_body->setVisible(expanded);
    updateGeometry();
    update();
}

void CollapsiblePanel::setTitle(const QString& title)
{
    m_title = title;
    update();
}

void CollapsiblePanel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const QRect header(0, 0, width(), kHeaderHeight);
    p.fillRect(header, theme::kPanelHeader);

    // Disclosure triangle.
    p.setPen(Qt::NoPen);
    p.setBrush(m_hover ? QColor(0xE8, 0xE8, 0xE8) : QColor(0xD8, 0xD8, 0xD8));
    const QPointF c(9.0, kHeaderHeight / 2.0);
    QPolygonF tri;
    if (m_expanded)
        tri << QPointF(c.x() - 4, c.y() - 2.5) << QPointF(c.x() + 4, c.y() - 2.5) << QPointF(c.x(), c.y() + 3.5);
    else
        tri << QPointF(c.x() - 2.5, c.y() - 4) << QPointF(c.x() + 3.5, c.y()) << QPointF(c.x() - 2.5, c.y() + 4);
    p.drawPolygon(tri);

    p.setPen(theme::kPanelHeaderText);
    p.setFont(theme::uiFont(9));
    p.drawText(QRect(20, 0, width() - 26, kHeaderHeight),
               Qt::AlignVCenter | Qt::AlignLeft, m_title);

    // Thin separator below the header.
    p.setPen(QColor(0xC8, 0xC8, 0xC8));
    p.drawLine(0, kHeaderHeight - 1, width(), kHeaderHeight - 1);
}

void CollapsiblePanel::mousePressEvent(QMouseEvent* event)
{
    if (event->pos().y() < kHeaderHeight) {
        setExpanded(!m_expanded);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void CollapsiblePanel::mouseMoveEvent(QMouseEvent* event)
{
    const bool hover = event->pos().y() < kHeaderHeight;
    if (hover != m_hover) {
        m_hover = hover;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void CollapsiblePanel::leaveEvent(QEvent* event)
{
    m_hover = false;
    update();
    QWidget::leaveEvent(event);
}

QSize CollapsiblePanel::sizeHint() const
{
    int bodyHeight = m_expanded ? m_body->sizeHint().height() : 0;
    return QSize(m_body->sizeHint().width(), kHeaderHeight + bodyHeight);
}

QSize CollapsiblePanel::minimumSizeHint() const
{
    return QSize(160, kHeaderHeight + (m_expanded ? m_body->minimumSizeHint().height() : 0));
}

} // namespace artplus::ui
