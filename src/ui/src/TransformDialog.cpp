#include "artplus/ui/TransformDialog.h"

#include "artplus/ui/Theme.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPushButton>
#include <QSpinBox>

#include <algorithm>
#include <cmath>

namespace artplus::ui {

namespace {

constexpr double kPointsPerMm = 72.0 / 25.4;

// --- Anchor picker -----------------------------------------------------

class AnchorPicker : public QWidget
{
public:
    explicit AnchorPicker(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(41, 41);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
    }

    artplus::edit::TransformAnchor anchor() const
    {
        return static_cast<artplus::edit::TransformAnchor>(m_index);
    }

    void setAnchor(artplus::edit::TransformAnchor anchor)
    {
        const int index = std::clamp(static_cast<int>(anchor), 0, 8);
        if (index == m_index)
            return;
        m_index = index;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);

        QPen line(QColor(0x46, 0x46, 0x46));
        line.setWidth(1);
        p.setPen(line);

        // Connecting lines along the centre of each row and column.
        for (int i = 0; i < 3; ++i) {
            const int c = i * kPitch + kCell / 2;
            p.drawLine(kCell / 2, c, 2 * kPitch + kCell / 2, c);
            p.drawLine(c, kCell / 2, c, 2 * kPitch + kCell / 2);
        }

        for (int i = 0; i < 9; ++i) {
            const QRect cell = cellRect(i);
            if (i == m_index) {
                p.fillRect(cell, QColor(0x20, 0x20, 0x20));
                p.drawRect(cell.adjusted(0, 0, -1, -1));
            } else {
                p.fillRect(cell, i == m_hover ? QColor(0xE0, 0xEE, 0xF9) : Qt::white);
                p.drawRect(cell.adjusted(0, 0, -1, -1));
            }
        }
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            const int col = std::clamp(static_cast<int>(event->position().x()) / kPitch, 0, 2);
            const int row = std::clamp(static_cast<int>(event->position().y()) / kPitch, 0, 2);
            m_index = row * 3 + col;
            update();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const int col = std::clamp(static_cast<int>(event->position().x()) / kPitch, 0, 2);
        const int row = std::clamp(static_cast<int>(event->position().y()) / kPitch, 0, 2);
        const int hover = row * 3 + col;
        if (hover != m_hover) {
            m_hover = hover;
            update();
        }
        QWidget::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        if (m_hover != -1) {
            m_hover = -1;
            update();
        }
        QWidget::leaveEvent(event);
    }

private:
    static constexpr int kCell = 9;
    static constexpr int kGap = 5;
    static constexpr int kPitch = kCell + kGap;

    static QRect cellRect(int index)
    {
        return QRect((index % 3) * kPitch, (index / 3) * kPitch, kCell, kCell);
    }

    int m_index = 0; // ArtPro+ v24 defaults to the top-left anchor.
    int m_hover = -1;
};

// --- Small painted glyphs ---------------------------------------------

enum class Glyph {
    RightArrow,
    DownArrow,
    HorizontalScale,
    VerticalScale,
    Rotate,
    Shear,
    Info,
};

class GlyphWidget : public QWidget
{
public:
    GlyphWidget(Glyph glyph, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_glyph(glyph)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.translate(width() / 2.0, height() / 2.0);

        switch (m_glyph) {
        case Glyph::RightArrow:
            drawArrow(p, true, true);
            break;
        case Glyph::DownArrow:
            drawArrow(p, false, true);
            break;
        case Glyph::HorizontalScale:
            drawArrow(p, true, true);
            drawArrow(p, true, false);
            break;
        case Glyph::VerticalScale:
            drawArrow(p, false, true);
            drawArrow(p, false, false);
            break;
        case Glyph::Rotate:
            drawRotate(p);
            break;
        case Glyph::Shear:
            drawShear(p);
            break;
        case Glyph::Info:
            drawInfo(p);
            break;
        }
    }

private:
    // Draws a 12 px arrow. `forward` chooses the positive direction,
    // `withHead` adds the triangular head on that side.
    static void drawArrow(QPainter& p, bool horizontal, bool forward)
    {
        QPen pen(QColor(0x14, 0x14, 0x14));
        pen.setWidthF(1.2);
        pen.setCapStyle(Qt::FlatCap);
        p.setPen(pen);
        p.setBrush(QColor(0x14, 0x14, 0x14));

        const double dir = forward ? 1.0 : -1.0;
        if (horizontal) {
            p.drawLine(QPointF(-6.0 * dir, 0.0), QPointF(2.5 * dir, 0.0));
            QPolygonF head;
            head << QPointF(6.0 * dir, 0.0) << QPointF(1.5 * dir, -3.0)
                 << QPointF(1.5 * dir, 3.0);
            p.drawPolygon(head);
        } else {
            p.drawLine(QPointF(0.0, -6.0 * dir), QPointF(0.0, 2.5 * dir));
            QPolygonF head;
            head << QPointF(0.0, 6.0 * dir) << QPointF(-3.0, 1.5 * dir)
                 << QPointF(3.0, 1.5 * dir);
            p.drawPolygon(head);
        }
    }

    static void drawRotate(QPainter& p)
    {
        QPen pen(QColor(0x14, 0x14, 0x14));
        pen.setWidthF(1.3);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        // Open circle with the arrow at the upper-right end.
        p.drawArc(QRectF(-6.5, -6.5, 13.0, 13.0), 45 * 16, 285 * 16);
        p.setBrush(QColor(0x14, 0x14, 0x14));
        QPolygonF head;
        head << QPointF(6.8, -3.0) << QPointF(2.0, -4.6) << QPointF(4.4, -0.4);
        p.drawPolygon(head);
        p.drawLine(QPointF(4.6, -4.2), QPointF(6.8, -6.2));
    }

    static void drawShear(QPainter& p)
    {
        QPen pen(QColor(0x14, 0x14, 0x14));
        pen.setWidthF(1.2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPolygonF shape;
        shape << QPointF(0.5, -5.0) << QPointF(7.0, -5.0) << QPointF(0.0, 5.0)
              << QPointF(-6.5, 5.0);
        p.drawPolygon(shape);
    }

    static void drawInfo(QPainter& p)
    {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x0F, 0x6C, 0xBD));
        p.drawEllipse(QRectF(-7.0, -7.0, 14.0, 14.0));

        p.setPen(Qt::white);
        p.setBrush(Qt::white);
        p.drawEllipse(QRectF(-1.1, -4.4, 2.2, 2.2));
        p.drawRect(QRectF(-0.8, -1.2, 1.6, 5.4));
    }

    Glyph m_glyph;
};

// --- Painted flat checkbox --------------------------------------------

class FlatCheckBox : public QAbstractButton
{
public:
    explicit FlatCheckBox(const QString& text, QWidget* parent = nullptr)
        : QAbstractButton(parent)
    {
        setText(text);
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFont(theme::uiFont(8));
        setAttribute(Qt::WA_Hover, true);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF box(0.5, (height() - 13.0) / 2.0, 13.0, 13.0);
        const bool checked = isChecked();

        if (!isEnabled()) {
            p.setPen(QPen(QColor(0xC0, 0xC0, 0xC0), 1.0));
            p.setBrush(checked ? QColor(0xBF, 0xBF, 0xBF) : QColor(0xF4, 0xF4, 0xF4));
        } else if (checked) {
            p.setPen(QPen(QColor(0x00, 0x5F, 0xB8), 1.0));
            p.setBrush(QColor(0x00, 0x5F, 0xB8));
        } else {
            p.setPen(QPen(underMouse() ? QColor(0x00, 0x5F, 0xB8) : QColor(0x8A, 0x8A, 0x8A), 1.0));
            p.setBrush(Qt::white);
        }
        p.drawRoundedRect(box, 3.0, 3.0);

        if (checked) {
            QPen check(Qt::white);
            check.setWidthF(1.8);
            check.setCapStyle(Qt::RoundCap);
            check.setJoinStyle(Qt::RoundJoin);
            p.setPen(check);
            p.setBrush(Qt::NoBrush);
            QPolygonF tick;
            tick << QPointF(box.left() + 3.2, box.center().y())
                 << QPointF(box.left() + 5.8, box.bottom() - 3.2)
                 << QPointF(box.right() - 2.7, box.top() + 3.4);
            p.drawPolyline(tick);
        }

        p.setPen(isEnabled() ? QColor(0x1E, 0x1E, 0x1E) : QColor(0xA6, 0xA6, 0xA6));
        p.drawText(QRectF(18.0, 0.0, width() - 18.0, height()),
                   Qt::AlignLeft | Qt::AlignVCenter, text());
    }
};

// --- Painted toggle button for the link / flip glyphs ------------------

enum class ButtonGlyph { Link, FlipHorizontal, FlipVertical };

class GlyphButton : public QAbstractButton
{
public:
    GlyphButton(ButtonGlyph glyph, QWidget* parent = nullptr)
        : QAbstractButton(parent)
        , m_glyph(glyph)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setFixedSize(22, 22);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        if (underMouse() || isChecked()) {
            p.setPen(Qt::NoPen);
            p.setBrush(isChecked() ? QColor(0xC7, 0xE0, 0xF7) : QColor(0xE0, 0xEE, 0xF9));
            p.drawRoundedRect(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0), 3.0, 3.0);
        }

        p.translate(width() / 2.0, height() / 2.0);
        switch (m_glyph) {
        case ButtonGlyph::Link:
            drawLink(p);
            break;
        case ButtonGlyph::FlipHorizontal:
            drawFlip(p, true);
            break;
        case ButtonGlyph::FlipVertical:
            drawFlip(p, false);
            break;
        }
    }

private:
    static void drawLink(QPainter& p)
    {
        QPen pen(QColor(0x14, 0x14, 0x14));
        pen.setWidthF(1.4);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.save();
        p.rotate(-45.0);
        p.drawRoundedRect(QRectF(-7.0, -2.6, 8.0, 5.2), 2.2, 2.2);
        p.drawRoundedRect(QRectF(-1.0, -2.6, 8.0, 5.2), 2.2, 2.2);
        p.restore();
    }

    static void drawFlip(QPainter& p, bool horizontal)
    {
        QPen pen(QColor(0x14, 0x14, 0x14));
        pen.setWidthF(1.2);
        p.setPen(pen);
        p.setBrush(QColor(0x14, 0x14, 0x14));

        if (horizontal) {
            p.drawLine(QPointF(0.0, -6.0), QPointF(0.0, 6.0)); // mirror axis
            p.drawLine(QPointF(-6.0, 0.0), QPointF(6.0, 0.0)); // shaft
            QPolygonF left;
            left << QPointF(-7.0, 0.0) << QPointF(-3.0, -3.0) << QPointF(-3.0, 3.0);
            QPolygonF right;
            right << QPointF(7.0, 0.0) << QPointF(3.0, -3.0) << QPointF(3.0, 3.0);
            p.drawPolygon(left);
            p.drawPolygon(right);
        } else {
            p.drawLine(QPointF(-6.0, 0.0), QPointF(6.0, 0.0));
            p.drawLine(QPointF(0.0, -6.0), QPointF(0.0, 6.0));
            QPolygonF top;
            top << QPointF(0.0, -7.0) << QPointF(-3.0, -3.0) << QPointF(3.0, -3.0);
            QPolygonF bottom;
            bottom << QPointF(0.0, 7.0) << QPointF(-3.0, 3.0) << QPointF(3.0, 3.0);
            p.drawPolygon(top);
            p.drawPolygon(bottom);
        }
    }

    ButtonGlyph m_glyph;
};

// --- Widget factory helpers -------------------------------------------

QDoubleSpinBox* makeField(QWidget* parent, const QString& suffix, double minimum,
                          double maximum)
{
    auto* field = new QDoubleSpinBox(parent);
    field->setObjectName(QStringLiteral("transformField"));
    field->setRange(minimum, maximum);
    field->setDecimals(2);
    field->setSuffix(suffix);
    field->setButtonSymbols(QAbstractSpinBox::NoButtons);
    field->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    field->setKeyboardTracking(false);
    field->setFont(theme::uiFont(8));
    field->setFrame(false);
    field->setFixedSize(80, 20);
    return field;
}

QLabel* makeLabel(QWidget* parent, const QString& text, const QRect& geometry,
                  Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter)
{
    auto* label = new QLabel(text, parent);
    label->setFont(theme::uiFont(8));
    label->setAlignment(alignment);
    label->setGeometry(geometry);
    return label;
}

} // namespace

TransformDialog::TransformDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("TransformDialog"));
    setWindowTitle(QStringLiteral("Transform"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedSize(700, 248);
    setFont(theme::uiFont(8));

    buildUi();
    resetToDefaults();
}

void TransformDialog::buildUi()
{
    setStyleSheet(QStringLiteral(R"(
QDialog#TransformDialog { background: #F0F0F0; }
QDoubleSpinBox#transformField, QSpinBox#transformCopies {
    background: #FFFFFF;
    border: 1px solid #DCDCDC;
    border-bottom-color: #838383;
    color: #1E1E1E;
    padding-left: 3px;
    selection-background-color: #0078D4;
    selection-color: #FFFFFF;
}
QPushButton#transformButton, QPushButton#cancelButton {
    background: #FDFDFD;
    border: 1px solid #0078D4;
    border-radius: 3px;
    color: #1E1E1E;
    font: 8pt "Segoe UI";
    padding: 0px;
}
QPushButton#transformButton:hover, QPushButton#cancelButton:hover {
    background: #E0EEF9;
}
QPushButton#transformButton:pressed, QPushButton#cancelButton:pressed {
    background: #CCE4F7;
}
)"));

    // Anchor (11, 23, 41x41).
    auto* anchor = new AnchorPicker(this);
    anchor->setGeometry(11, 23, 41, 41);
    anchor->setToolTip(QStringLiteral("Anchor point"));
    m_anchor = anchor;

    // Move.
    auto* moveRight = new GlyphWidget(Glyph::RightArrow, this);
    moveRight->setGeometry(82, 11, 22, 40);
    auto* moveDown = new GlyphWidget(Glyph::DownArrow, this);
    moveDown->setGeometry(82, 58, 22, 22);

    m_moveX = makeField(this, QStringLiteral(" mm"), -100000.0, 100000.0);
    m_moveX->setGeometry(114, 21, 80, 20);
    m_moveX->setToolTip(QStringLiteral("Horizontal move"));
    m_moveY = makeField(this, QStringLiteral(" mm"), -100000.0, 100000.0);
    m_moveY->setGeometry(114, 59, 80, 20);
    m_moveY->setToolTip(QStringLiteral("Vertical move (downwards)"));

    // Scale.
    auto* scaleHorizontal = new GlyphWidget(Glyph::HorizontalScale, this);
    scaleHorizontal->setGeometry(266, 20, 40, 22);
    auto* scaleVertical = new GlyphWidget(Glyph::VerticalScale, this);
    scaleVertical->setGeometry(266, 58, 40, 22);

    m_scaleX = makeField(this, QStringLiteral(" %"), 0.01, 100000.0);
    m_scaleX->setGeometry(309, 21, 80, 20);
    m_scaleX->setToolTip(QStringLiteral("Horizontal scale"));
    m_scaleY = makeField(this, QStringLiteral(" %"), 0.01, 100000.0);
    m_scaleY->setGeometry(309, 59, 80, 20);
    m_scaleY->setToolTip(QStringLiteral("Vertical scale"));

    auto* link = new GlyphButton(ButtonGlyph::Link, this);
    link->setGeometry(399, 34, 22, 22);
    link->setToolTip(QStringLiteral("Link scale factors"));
    link->setAccessibleName(QStringLiteral("Link scale factors"));
    m_link = link;
    connect(link, &QAbstractButton::toggled, this, &TransformDialog::onLinkToggled);
    connect(m_scaleX, &QDoubleSpinBox::valueChanged, this, [this](double) {
        syncLinkedScale(m_scaleX);
    });
    connect(m_scaleY, &QDoubleSpinBox::valueChanged, this, [this](double) {
        syncLinkedScale(m_scaleY);
    });

    // Rotate and shear.
    auto* rotateGlyph = new GlyphWidget(Glyph::Rotate, this);
    rotateGlyph->setGeometry(486, 20, 40, 22);
    auto* shearGlyph = new GlyphWidget(Glyph::Shear, this);
    shearGlyph->setGeometry(486, 58, 40, 22);

    m_rotate = makeField(this, QStringLiteral(" \u00B0"), -3600.0, 3600.0);
    m_rotate->setGeometry(527, 21, 80, 20);
    m_rotate->setToolTip(QStringLiteral("Rotation"));
    m_shear = makeField(this, QStringLiteral(" \u00B0"), -3600.0, 3600.0);
    m_shear->setGeometry(527, 59, 80, 20);
    m_shear->setToolTip(QStringLiteral("Shear"));

    // Flips.
    auto* flipH = new GlyphButton(ButtonGlyph::FlipHorizontal, this);
    flipH->setGeometry(667, 20, 22, 22);
    flipH->setToolTip(QStringLiteral("Flip horizontally"));
    flipH->setAccessibleName(QStringLiteral("Flip horizontally"));
    m_flipHorizontal = flipH;
    auto* flipV = new GlyphButton(ButtonGlyph::FlipVertical, this);
    flipV->setGeometry(667, 58, 22, 22);
    flipV->setToolTip(QStringLiteral("Flip vertically"));
    flipV->setAccessibleName(QStringLiteral("Flip vertically"));
    m_flipVertical = flipV;

    // Separator between the geometric controls and the options.
    auto* separator = new QFrame(this);
    separator->setGeometry(11, 104, 678, 1);
    separator->setStyleSheet(QStringLiteral("background: #C8C8C8;"));

    // Include options.
    m_includeLabel = makeLabel(this, QStringLiteral("Include:"), QRect(62, 121, 42, 22),
                               Qt::AlignRight | Qt::AlignVCenter);

    auto* gradients = new FlatCheckBox(QStringLiteral("Gradients & Patterns"), this);
    gradients->setGeometry(114, 121, 340, 18);
    gradients->setChecked(true);
    gradients->setEnabled(false);
    gradients->setToolTip(QStringLiteral(
        "Not available in ArtPlus yet: PDFium cannot transform shading or pattern data."));
    m_gradients = gradients;

    auto* strokes = new FlatCheckBox(QStringLiteral("Stroke Weights & Effects"), this);
    strokes->setGeometry(114, 154, 340, 18);
    strokes->setToolTip(QStringLiteral("Scale stroke widths with the transform"));
    m_strokes = strokes;

    auto* masks = new FlatCheckBox(QStringLiteral("Opacity Masks"), this);
    masks->setGeometry(114, 186, 340, 18);
    masks->setChecked(true);
    masks->setEnabled(false);
    masks->setToolTip(QStringLiteral(
        "Not available in ArtPlus yet: PDFium exposes no opacity-mask transform API."));
    m_masks = masks;

    // Repetitions.
    m_transformLabel = makeLabel(this, QStringLiteral("Transform:"), QRect(461, 121, 55, 22));
    m_repetitions = new QSpinBox(this);
    m_repetitions->setObjectName(QStringLiteral("transformCopies"));
    m_repetitions->setRange(1, 999);
    m_repetitions->setValue(1);
    m_repetitions->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_repetitions->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_repetitions->setFont(theme::uiFont(8));
    m_repetitions->setFrame(false);
    m_repetitions->setGeometry(527, 121, 40, 22);
    m_copiesLabel = makeLabel(this, QStringLiteral("x"), QRect(573, 121, 20, 22));

    auto* each = new FlatCheckBox(QStringLiteral("Transform each"), this);
    each->setGeometry(527, 155, 160, 18);
    each->setToolTip(QStringLiteral("Transform every selected object around its own anchor"));
    m_transformEach = each;

    // Information line.
    auto* info = new GlyphWidget(Glyph::Info, this);
    info->setGeometry(10, 212, 16, 16);
    m_hintIcon = info;
    m_hintText = makeLabel(this, QStringLiteral("Hold Alt key to reset and copy."),
                           QRect(32, 211, 360, 20));

    // Buttons.
    m_transformButton = new QPushButton(QStringLiteral("Transform"), this);
    m_transformButton->setObjectName(QStringLiteral("transformButton"));
    m_transformButton->setGeometry(533, 213, 75, 24);
    m_transformButton->setDefault(true);
    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancelButton->setObjectName(QStringLiteral("cancelButton"));
    m_cancelButton->setGeometry(614, 213, 75, 24);
    m_cancelButton->setAutoDefault(false);

    connect(m_transformButton, &QPushButton::clicked, this,
            &TransformDialog::onTransformClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &TransformDialog::onCancelClicked);
}

void TransformDialog::setInitial(const artplus::edit::TransformParams& params,
                                 artplus::edit::TransformAnchor anchor)
{
    auto* anchorPicker = static_cast<AnchorPicker*>(m_anchor);
    anchorPicker->setAnchor(anchor);

    auto* gradients = static_cast<FlatCheckBox*>(m_gradients);
    auto* strokes = static_cast<FlatCheckBox*>(m_strokes);
    auto* masks = static_cast<FlatCheckBox*>(m_masks);
    auto* each = static_cast<FlatCheckBox*>(m_transformEach);
    gradients->setChecked(true);
    strokes->setChecked(params.includeStrokeWeightsAndEffects);
    masks->setChecked(true);
    each->setChecked(params.transformEach);

    m_moveX->setValue(params.moveX / kPointsPerMm);
    m_moveY->setValue(params.moveY / kPointsPerMm);
    m_scaleX->setValue(params.scaleX * 100.0);
    m_scaleY->setValue(params.scaleY * 100.0);
    m_rotate->setValue(params.rotateDegrees);
    m_shear->setValue(params.shearDegrees);
    m_repetitions->setValue(std::max(1, params.repetitions));
    static_cast<GlyphButton*>(m_flipHorizontal)->setChecked(params.flipHorizontal);
    static_cast<GlyphButton*>(m_flipVertical)->setChecked(params.flipVertical);
    static_cast<GlyphButton*>(m_link)->setChecked(false);
}

artplus::edit::TransformParams TransformDialog::params() const
{
    artplus::edit::TransformParams result;
    result.moveX = m_moveX->value() * kPointsPerMm;
    result.moveY = m_moveY->value() * kPointsPerMm;
    result.scaleX = m_scaleX->value() / 100.0;
    result.scaleY = m_scaleY->value() / 100.0;
    result.flipHorizontal = static_cast<const GlyphButton*>(m_flipHorizontal)->isChecked();
    result.flipVertical = static_cast<const GlyphButton*>(m_flipVertical)->isChecked();
    result.shearDegrees = m_shear->value();
    result.rotateDegrees = m_rotate->value();
    result.repetitions = std::max(1, m_repetitions->value());
    result.transformEach = static_cast<const FlatCheckBox*>(m_transformEach)->isChecked();
    result.includeGradientsAndPatterns = true;
    result.includeStrokeWeightsAndEffects =
        static_cast<const FlatCheckBox*>(m_strokes)->isChecked();
    result.includeOpacityMasks = true;
    return result;
}

artplus::edit::TransformAnchor TransformDialog::anchor() const
{
    return static_cast<const AnchorPicker*>(m_anchor)->anchor();
}

void TransformDialog::resetToDefaults()
{
    m_copyRequested = false;

    auto* gradients = static_cast<FlatCheckBox*>(m_gradients);
    auto* strokes = static_cast<FlatCheckBox*>(m_strokes);
    auto* masks = static_cast<FlatCheckBox*>(m_masks);
    auto* each = static_cast<FlatCheckBox*>(m_transformEach);
    gradients->setChecked(true);
    strokes->setChecked(false);
    masks->setChecked(true);
    each->setChecked(false);

    m_moveX->setValue(0.0);
    m_moveY->setValue(0.0);
    m_scaleX->setValue(100.0);
    m_scaleY->setValue(100.0);
    m_rotate->setValue(0.0);
    m_shear->setValue(0.0);
    m_repetitions->setValue(1);
    static_cast<GlyphButton*>(m_flipHorizontal)->setChecked(false);
    static_cast<GlyphButton*>(m_flipVertical)->setChecked(false);
    static_cast<GlyphButton*>(m_link)->setChecked(false);
    static_cast<AnchorPicker*>(m_anchor)->setAnchor(
        artplus::edit::TransformAnchor::TopLeft);
}

void TransformDialog::onTransformClicked()
{
    m_copyRequested = QApplication::keyboardModifiers().testFlag(Qt::AltModifier);
    accept();
}

void TransformDialog::onCancelClicked()
{
    if (QApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
        resetToDefaults();
        return;
    }
    reject();
}

void TransformDialog::onLinkToggled(bool checked)
{
    if (!checked)
        return;
    m_scaleY->setValue(m_scaleX->value());
}

void TransformDialog::syncLinkedScale(QDoubleSpinBox* changed)
{
    if (m_syncingScale || !static_cast<GlyphButton*>(m_link)->isChecked())
        return;
    m_syncingScale = true;
    QDoubleSpinBox* other = (changed == m_scaleX) ? m_scaleY : m_scaleX;
    other->setValue(changed->value());
    m_syncingScale = false;
}

void TransformDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && !(event->modifiers() & Qt::AltModifier)) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

} // namespace artplus::ui
