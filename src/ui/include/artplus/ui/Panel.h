#pragma once

#include <QString>
#include <QWidget>

class QVBoxLayout;

namespace artplus::ui {

// Dark-header collapsible panel, matching the ArtPro+ dock sections
// (Document, Layers, Separations, Quality Control, Messages).
class CollapsiblePanel : public QWidget
{
public:
    explicit CollapsiblePanel(const QString& title, QWidget* parent = nullptr);

    QWidget* body() const { return m_body; }
    QVBoxLayout* bodyLayout() const;
    QVBoxLayout* contentLayout() const;

    void setExpanded(bool expanded);
    bool isExpanded() const { return m_expanded; }
    void setTitle(const QString& title);
    QString title() const { return m_title; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    static constexpr int kHeaderHeight = 24;
    QString m_title;
    QWidget* m_body = nullptr;
    QVBoxLayout* m_bodyLayout = nullptr;
    bool m_expanded = true;
    bool m_hover = false;
};

} // namespace artplus::ui
