#pragma once

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QVector>
#include <QWidget>

#include <functional>

#include "artplus/objects/GraphicObject.h"

namespace artplus::ui {

// A lightweight PDF page viewport.
//
// The widget owns no PDF document: it displays a QImage prepared by the
// caller (rendered at `zoom()` pixels per PDF point). Zoom, pan, and view
// rotation are handled here so the surrounding window only has to re-render
// the page when asked.
//
// Deliberately not a QObject so it does not require moc; callers receive
// notifications through std::function callbacks.
class PageView : public QWidget
{
public:
    // Interaction mode determines how mouse events are interpreted.
    enum class InteractionMode {
        Selection,    // Select objects, drag to move, marquee select on empty
        Pan,          // Pan the viewport (Hand tool)
        Marquee       // Drag to create selection rectangle (could be merged with Selection)
    };

    explicit PageView(QWidget* parent = nullptr);

    // Page geometry, in PDF points (already account for /Rotate + view rotation).
    void setPageSize(const QSizeF& sizePoints);
    QSizeF pageSize() const { return m_pageSizePoints; }

    // Rendered page image. It is drawn scaled to zoom() * pageSize().
    void setPageImage(const QImage& image);
    const QImage& pageImage() const { return m_image; }
    void clearPage();

    // Scale in pixels per PDF point.
    void setZoom(double scale);
    double zoom() const { return m_scale; }

    void fitPage();
    void fitWidth();
    void actualSize();
    // Zooms so that `pageRect` (page points, top-left origin) fills ~85% of the
    // viewport and centres it. ArtPro+ exposes this as View > Zoom >
    // "Zoom to Selection / Previous" (Ctrl+Alt+0).
    void zoomToSelection(const QRectF& pageRect);
    void zoomIn() { setZoom(m_scale * 1.25); }
    void zoomOut() { setZoom(m_scale / 1.25); }

    void setViewRotation(int degrees);
    int viewRotation() const { return m_viewRotation; }
    void rotateView(int delta);

    // ArtPro+-style overlays.
    void setPageBoxesVisible(bool visible);
    bool pageBoxesVisible() const { return m_pageBoxes; }
    void setCrosshairEnabled(bool enabled);
    bool crosshairEnabled() const { return m_crosshair; }

    // Selection highlight, in page points (top-left origin).
    void setSelectionRect(const QRectF& pageRect);
    void setSelectionRects(const QVector<QRectF>& pageRects);
    void clearSelection();

    // Cursor position in PDF points, relative to the page top-left corner.
    void setCursorCallback(std::function<void(const QPointF&)> cb) { m_cursorCb = std::move(cb); }
    // Called on a left click that did not turn into a pan drag or object drag.
    void setClickCallback(std::function<void(const QPointF&)> cb) { m_clickCb = std::move(cb); }
    // Called when an object is being dragged (object moved).
    void setObjectDragCallback(std::function<void(const QPointF&)> cb) { m_objectDragCb = std::move(cb); }
    // Called when a marquee selection rectangle is completed.
    void setMarqueeSelectCallback(std::function<void(const QRectF&)> cb) { m_marqueeSelectCb = std::move(cb); }
    // Called to determine if a point (in page coordinates) hits an object.
    // Returns the object ID if hit, 0 otherwise.
    void setHitTestCallback(std::function<objects::ObjectId(const QPointF&)> cb) { m_hitTestCb = std::move(cb); }

    void setStatusCallback(std::function<void()> cb) { m_statusCb = std::move(cb); }
    void setRenderRequestCallback(std::function<void()> cb) { m_requestRender = std::move(cb); }

    // Set the interaction mode (Selection, Pan, etc.)
    void setInteractionMode(InteractionMode mode) { m_interactionMode = mode; }
    InteractionMode interactionMode() const { return m_interactionMode; }

    QSize sizeHint() const override { return {900, 700}; }

    // Rectangle (widget coordinates) occupied by the page.
    QRectF pageRect() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEnterEvent* event) override;

private:
    QRectF centeredPageRect(double scale) const;
    void clampPan();
    void notifyStatus();
    void requestRender();
    void updateCursorShape();
    void drawPageBoxChips(QPainter& painter, const QRectF& page) const;

    QImage m_image;
    QVector<QRectF> m_selections; // page points
    bool m_hasSelection = false;
    QSizeF m_pageSizePoints{595.275590551, 841.889763780};
    double m_scale = 1.0;
    int m_viewRotation = 0;
    QPointF m_pan{0.0, 0.0};
    bool m_dragging = false;
    bool m_pageBoxes = true;
    bool m_crosshair = false;
    QPoint m_lastMousePos;
    bool m_fitOnResize = true;
    std::function<void()> m_statusCb;
    std::function<void()> m_requestRender;
    std::function<void(const QPointF&)> m_cursorCb;
    std::function<void(const QPointF&)> m_clickCb;
    std::function<void(const QPointF&)> m_objectDragCb;
    std::function<void(const QRectF&)> m_marqueeSelectCb;
    QPointF m_pressPos;
    bool m_draggedSincePress = false;

    // Interaction state machine
    enum class DragState {
        None,
        Pan,
        ObjectDrag,
        MarqueeSelect
    };
    DragState m_dragState = DragState::None;
    InteractionMode m_interactionMode = InteractionMode::Selection;
    objects::ObjectId m_dragObjectId = 0;
    QPointF m_dragStartPagePos;
    QPointF m_marqueeStartPos;
    QRectF m_marqueeCurrentPos;
    bool m_marqueeActive = false;
    std::function<objects::ObjectId(const QPointF&)> m_hitTestCb;
};

} // namespace artplus::ui