#include "artplus/marks/Mark.h"
#include "artplus/document/ArtPage.h"

#include <algorithm>
#include <cmath>

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>

namespace artplus::marks {

namespace {

geometry::Path unitSquare()
{
    geometry::Path square;
    square.addRectangle({0.0, 0.0, 1.0, 1.0});
    return square;
}

QRectF toQPointRect(const geometry::Rect& r)
{
    return QRectF(r.left, r.top, r.width(), r.height());
}

qreal clampPen(qreal v, qreal fallback)
{
    return v > 0.0 ? v : fallback;
}

} // namespace

// ---------------------------------------------------------------------------
// ScaleableMarker
// ---------------------------------------------------------------------------

ScaleableMarker::ScaleableMarker() = default;

void ScaleableMarker::scale(double sx, double sy)
{
    m_scaleX = std::max(0.01, m_scaleX * sx);
    m_scaleY = std::max(0.01, m_scaleY * sy);
}

void ScaleableMarker::translate(double dx, double dy)
{
    m_origin += geometry::Point(dx, dy);
}

void ScaleableMarker::rotateDegrees(double deg)
{
    m_rotation = std::fmod(deg + 360.0, 360.0);
}

void ScaleableMarker::setSize(double w, double h)
{
    m_width = std::max(1.0, w);
    m_height = std::max(1.0, h);
    m_scaleX = 1.0;
    m_scaleY = 1.0;
}

double ScaleableMarker::width() const { return m_width * m_scaleX; }
double ScaleableMarker::height() const { return m_height * m_scaleY; }
double ScaleableMarker::scaleX() const { return m_scaleX; }
double ScaleableMarker::scaleY() const { return m_scaleY; }
int ScaleableMarker::rotation() const { return static_cast<int>(std::lround(m_rotation)); }

geometry::Point ScaleableMarker::origin() { return m_origin; }

geometry::Matrix ScaleableMarker::transform() const
{
    geometry::Matrix m = geometry::Matrix::translation(m_origin.x, m_origin.y);
    m = m * geometry::Matrix::rotationDegrees(m_rotation);
    m = m * geometry::Matrix::scaling(m_scaleX, m_scaleY);
    return m;
}

// ---------------------------------------------------------------------------
// GraphicalMark
// ---------------------------------------------------------------------------

GraphicalMark::GraphicalMark()
    : m_path(unitSquare())
{
}

GraphicalMark::GraphicalMark(const geometry::Path& path)
    : m_path(path)
{
    setSize(20.0, 20.0);
}

void GraphicalMark::renderStroke(QPainter* painter, double w) const
{
    if (!painter)
        return;
    QPen pen(QColor(0, 0, 0));
    pen.setWidthF(static_cast<qreal>(std::max(0.0, w)));
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
}

// ---------------------------------------------------------------------------
// Concrete marks
// ---------------------------------------------------------------------------

RegistrationCross::RegistrationCross()
{
    m_path = unitSquare();
    m_filled = true;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 0;
}

void RegistrationCross::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    const qreal lineWidth = clampPen(std::min(r.width(), r.height()) / 12.0, 0.25);

    QPen pen(QColor(0, 0, 0));
    pen.setWidthF(lineWidth);
    painter->save();
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    // Horizontal and vertical crosshair through the centre.
    painter->drawLine(QPointF(r.left(), r.center().y()),
                      QPointF(r.right(), r.center().y()));
    painter->drawLine(QPointF(r.center().x(), r.top()),
                      QPointF(r.center().x(), r.bottom()));
    // Registration circle.
    painter->drawEllipse(r.center(), r.width() / 4.0, r.height() / 4.0);
    painter->restore();
}

TrimMarks::TrimMarks()
{
    m_path = unitSquare();
    m_filled = false;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 0);
    m_strokeColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 1.0;
}

void TrimMarks::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    const qreal len = std::max(r.width(), r.height()) / 2.0;
    const qreal lineWidth = clampPen(std::min(r.width(), r.height()) / 20.0, 0.25);

    QPen pen(QColor(0, 0, 0));
    pen.setWidthF(lineWidth);
    painter->save();
    painter->setPen(pen);
    // Trim marks: short lines extending from the right and bottom edges.
    painter->drawLine(QPointF(r.right(), r.center().y()),
                      QPointF(r.right() + len, r.center().y()));
    painter->drawLine(QPointF(r.center().x(), r.bottom()),
                      QPointF(r.center().x(), r.bottom() + len));
    painter->restore();
}

BleedMarks::BleedMarks()
{
    m_path = unitSquare();
    m_filled = true;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 0;
}

void BleedMarks::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    const qreal lineWidth = clampPen(std::min(r.width(), r.height()) / 25.0, 0.2);

    QPen pen(QColor(0, 0, 0));
    pen.setWidthF(lineWidth);
    painter->save();
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    // Outer bleed box and inner trim box.
    painter->drawRect(r.adjusted(-lineWidth * 2.0, -lineWidth * 2.0,
                                 lineWidth * 2.0, lineWidth * 2.0));
    painter->drawRect(r);
    painter->restore();
}

ColorBarMarks::ColorBarMarks()
{
    m_path = unitSquare();
    m_filled = false;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 0);
    m_strokeColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 1.0;
}

void ColorBarMarks::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    const int patches = 6;
    const qreal patchW = r.width() / patches;

    painter->save();
    painter->setPen(Qt::NoPen);
    // A simple process/ink patch ramp. Exact ink values are a later milestone.
    const int levels[6] = {0, 51, 102, 153, 204, 255};
    for (int i = 0; i < patches; ++i) {
        const int v = levels[i];
        painter->setBrush(QColor(v, v, v));
        painter->drawRect(QRectF(r.left() + i * patchW, r.top(), patchW, r.height()));
    }
    painter->restore();
}

CodeBarMarks::CodeBarMarks()
{
    m_path = unitSquare();
    m_filled = false;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 0);
    m_strokeColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 1.0;
}

void CodeBarMarks::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    const int bars = 24;
    const qreal barW = r.width() / bars;

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0));
    for (int i = 0; i < bars; i += 2) {
        painter->drawRect(QRectF(r.left() + i * barW, r.top(), barW, r.height()));
    }
    painter->restore();
}

QuadMarks::QuadMarks()
{
    m_path = unitSquare();
    m_filled = false;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 0);
    m_strokeColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 1.0;
}

void QuadMarks::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    const qreal lineWidth = clampPen(std::min(r.width(), r.height()) / 20.0, 0.25);

    QPen pen(QColor(0, 0, 0));
    pen.setWidthF(lineWidth);
    painter->save();
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    // Four quadrant registration quads.
    painter->drawRect(r.adjusted(0.0, 0.0, -r.width() / 2.0, -r.height() / 2.0));
    painter->drawRect(r.adjusted(r.width() / 2.0, 0.0, 0.0, -r.height() / 2.0));
    painter->drawRect(r.adjusted(0.0, r.height() / 2.0, -r.width() / 2.0, 0.0));
    painter->drawRect(r.adjusted(r.width() / 2.0, r.height() / 2.0, 0.0, 0.0));
    painter->restore();
}

CircleOrkMarks::CircleOrkMarks()
{
    m_path = unitSquare();
    m_filled = false;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 0);
    m_strokeColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 1.0;
}

void CircleOrkMarks::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    const qreal lineWidth = clampPen(std::min(r.width(), r.height()) / 30.0, 0.2);

    QPen pen(QColor(0, 0, 0));
    pen.setWidthF(lineWidth);
    painter->save();
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    // Concentric registration circles with a centre dot.
    painter->drawEllipse(r.center(), r.width() / 2.0, r.height() / 2.0);
    painter->drawEllipse(r.center(), r.width() / 3.0, r.height() / 3.0);
    painter->drawEllipse(r.center(), r.width() / 8.0, r.height() / 8.0);
    painter->restore();
}

ArrowMark::ArrowMark()
{
    m_path = unitSquare();
    m_filled = false;
    m_fillColor = colors::Color::cmyk(0, 0, 0, 1);
    m_strokeWidth = 1.5;
}

void ArrowMark::paint(QPainter* painter, const geometry::Rect& area) const
{
    if (!painter)
        return;

    const QRectF r = toQPointRect(area);
    QPainterPath arrow;
    arrow.moveTo(r.center().x(), r.top());
    arrow.lineTo(r.right(), r.center().y());
    arrow.lineTo(r.center().x() + r.width() / 8.0, r.center().y());
    arrow.lineTo(r.center().x() + r.width() / 8.0, r.bottom());
    arrow.lineTo(r.center().x() - r.width() / 8.0, r.bottom());
    arrow.lineTo(r.center().x() - r.width() / 8.0, r.center().y());
    arrow.lineTo(r.left(), r.center().y());
    arrow.closeSubpath();

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0));
    painter->drawPath(arrow);
    painter->restore();
}

// ---------------------------------------------------------------------------
// SNRGridParams
// ---------------------------------------------------------------------------

double SNRGridParams::effectiveCellX() const
{
    return useBleed ? cellSizeX + 2.0 * bleed : cellSizeX;
}

double SNRGridParams::effectiveCellY() const
{
    return useBleed ? cellSizeY + 2.0 * bleed : cellSizeY;
}

// ---------------------------------------------------------------------------
// TaskMark
// ---------------------------------------------------------------------------

TaskMark::TaskMark(QString name, MarkRole role, QString listName)
    : m_name(std::move(name))
    , m_role(role)
    , m_listName(std::move(listName))
{
}

// ---------------------------------------------------------------------------
// TaskMarkSet
// ---------------------------------------------------------------------------

TaskMarkSet::TaskMarkSet() = default;
TaskMarkSet::~TaskMarkSet() = default;

const TaskMarkSet& TaskMarkSet::defaultFactory()
{
    static TaskMarkSet s;
    static bool initialised = false;
    if (!initialised) {
        initialised = true;
        s.add(TaskMark(QStringLiteral("Registration Cross"), MarkRole::Registration,
                       QStringLiteral("registration")));
        s.add(TaskMark(QStringLiteral("Trim Marks"), MarkRole::Trim, QStringLiteral("trim")));
        s.add(TaskMark(QStringLiteral("Bleed Marks"), MarkRole::Bleed, QStringLiteral("bleed")));
        s.add(TaskMark(QStringLiteral("Color Bar"), MarkRole::ColorBar, QStringLiteral("colorbar")));
        s.add(TaskMark(QStringLiteral("Code Bar"), MarkRole::ColorBar, QStringLiteral("codebar")));
        s.add(TaskMark(QStringLiteral("Quad Marks"), MarkRole::Decorative, QStringLiteral("quad")));
        s.add(TaskMark(QStringLiteral("Circle OrK"), MarkRole::Decorative, QStringLiteral("ork")));
        s.add(TaskMark(QStringLiteral("Arrow"), MarkRole::Decorative, QStringLiteral("arrow")));
    }
    return s;
}

void TaskMarkSet::loadFromDocument(artplus::document::ArtPage* page)
{
    if (!page)
        return;
    m_marks.clear();
    const QString serialised = page->tags().get(QStringLiteral("taskMarks")).toString();
    if (serialised.isEmpty())
        return;
    const QStringList names = serialised.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString& entry : names) {
        const QStringList parts = entry.split(QLatin1Char('|'));
        if (parts.isEmpty())
            continue;
        MarkRole role = MarkRole::Decorative;
        if (parts.size() > 1) {
            if (parts[1] == QLatin1String("registration"))
                role = MarkRole::Registration;
            else if (parts[1] == QLatin1String("trim"))
                role = MarkRole::Trim;
            else if (parts[1] == QLatin1String("bleed"))
                role = MarkRole::Bleed;
            else if (parts[1] == QLatin1String("colorbar"))
                role = MarkRole::ColorBar;
        }
        add(TaskMark(parts[0], role, parts.size() > 2 ? parts[2] : QString()));
    }
}

void TaskMarkSet::saveToDocument(artplus::document::ArtPage* page)
{
    if (!page)
        return;
    QStringList entries;
    entries.reserve(static_cast<int>(m_marks.size()));
    for (const auto& mark : m_marks) {
        QString role = QStringLiteral("decorative");
        switch (mark.role()) {
        case MarkRole::Registration: role = QStringLiteral("registration"); break;
        case MarkRole::Trim: role = QStringLiteral("trim"); break;
        case MarkRole::Bleed: role = QStringLiteral("bleed"); break;
        case MarkRole::ColorBar: role = QStringLiteral("colorbar"); break;
        case MarkRole::Decorative: role = QStringLiteral("decorative"); break;
        }
        entries << QStringLiteral("%1|%2|%3").arg(mark.name(), role, mark.listName());
    }
    page->tags().set(QStringLiteral("taskMarks"), entries.join(QLatin1Char(';')));
}

TaskMark TaskMarkSet::add(const TaskMark& mark)
{
    TaskMark stored(mark.name(), mark.role(), mark.listName());
    if (find(stored.name()))
        return stored;
    m_marks.push_back(stored);
    return stored;
}

bool TaskMarkSet::remove(const QString& name)
{
    for (int i = 0; i < m_marks.size(); ++i) {
        if (m_marks[i].name().compare(name, Qt::CaseInsensitive) == 0) {
            m_marks.removeAt(i);
            return true;
        }
    }
    return false;
}

TaskMark* TaskMarkSet::find(const QString& name)
{
    for (auto& mark : m_marks) {
        if (mark.name().compare(name, Qt::CaseInsensitive) == 0)
            return &mark;
    }
    return nullptr;
}

const TaskMark* TaskMarkSet::find(const QString& name) const
{
    for (const auto& mark : m_marks) {
        if (mark.name().compare(name, Qt::CaseInsensitive) == 0)
            return &mark;
    }
    return nullptr;
}

bool TaskMarkSet::hasCrossoverMark() const
{
    for (const auto& mark : m_marks) {
        if (mark.isRegistered())
            return true;
    }
    return false;
}

bool TaskMarkSet::hasVersionGaugeMark() const
{
    for (const auto& mark : m_marks) {
        if (mark.name().startsWith(QStringLiteral("Version")))
            return true;
    }
    return false;
}

bool TaskMarkSet::hasRepeatForSeparationMarks() const
{
    for (const auto& mark : m_marks) {
        if (mark.isRegistered() && mark.listName().startsWith(QStringLiteral("repeat_")))
            return true;
    }
    return false;
}

bool TaskMarkSet::hasRepeatForStationMarks() const
{
    for (const auto& mark : m_marks) {
        if (mark.listName().startsWith(QStringLiteral("repeat_station")))
            return true;
    }
    return false;
}

void TaskMarkSet::flush() { m_marks.clear(); }

void TaskMarkSet::makeDefaultOnPage(artplus::document::ArtPage* page)
{
    if (!page)
        return;
    if (!page->findLayer(QStringLiteral("Default Marks")))
        page->addLayer(QStringLiteral("Default Marks"), artplus::document::LayerType::Marks);
}

void TaskMarkSet::applyRelevant(artplus::document::ArtPage* page, bool withReuse)
{
    if (!page)
        return;
    (void)withReuse;
}

} // namespace artplus::marks
