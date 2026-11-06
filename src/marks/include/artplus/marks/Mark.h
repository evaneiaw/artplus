#pragma once

#include <QByteArray>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>
#include <QSize>

#include "artplus/colors/Color.h"
#include "artplus/geometry/Path.h"
#include "artplus/document/ArtPage.h"

#include <QString>
#include <QVector>
#include <algorithm>
#include <memory>

namespace artplus::marks {

enum class MarkType {
    RegistrationCross,
    Trim,
    Bleed,
    ColorBar,
    CodeBar,
    Quad,
    CircleOrk,
    Arrow,
    Custom
};

enum class MarkRole { Registration, Trim, Bleed, ColorBar, Decorative };

struct MarkSetId {
    int group = 0;       // User or factory category index (0 = default)
    const char* name = nullptr;
};

class ScaleableMarker
{
public:
    ScaleableMarker();
    void scale(double sx, double sy);
    void translate(double dx, double dy);
    void rotateDegrees(double deg);
    void setSize(double widthPt, double heightPt);
    double width() const;
    double height() const;
    double scaleX() const;
    double scaleY() const;
    int rotation() const;
    geometry::Point origin();
    geometry::Matrix transform() const;

private:
    double m_width = 20.0;
    double m_height = 20.0;
    double m_scaleX = 1.0;
    double m_scaleY = 1.0;
    double m_rotation = 0.0;
    geometry::Point m_origin{0, 0};
};

class GraphicalMark : public ScaleableMarker
{
public:
    GraphicalMark();
    explicit GraphicalMark(const geometry::Path& path);

    const geometry::Path& path() const { return m_path; }
    void setPath(const geometry::Path& p) { m_path = p; }

    bool filled() const { return m_filled; }
    void setFilled(bool v) { m_filled = v; }
    colors::Color fillColor() const { return m_fillColor; }
    void setFillColor(const colors::Color& c) { m_fillColor = c; }
    colors::Color strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const colors::Color& c) { m_strokeColor = c; }
    bool strokeOnly() const { return !m_filled || m_strokeWidth > 0; }
    double strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(double w) { m_strokeWidth = w; }
    double opacity() const { return m_opacity; }
    void setOpacity(double v) { m_opacity = std::clamp(v, 0.0, 1.0); }
    void renderStroke(QPainter* painter, double w) const;

    virtual void paint(QPainter* painter, const geometry::Rect& area) const = 0;

protected:
    geometry::Path m_path;
    bool m_filled = false;
    colors::Color m_fillColor = colors::Color::cmyk(0, 0, 0, 0);
    colors::Color m_strokeColor = colors::Color::cmyk(0, 0, 0, 1);
    double m_strokeWidth = 0.5;
    double m_opacity = 1.0;
};

struct RegistrationCross : public GraphicalMark
{
    RegistrationCross();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

struct TrimMarks : public GraphicalMark
{
    TrimMarks();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

struct BleedMarks : public GraphicalMark
{
    BleedMarks();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

struct ColorBarMarks : public GraphicalMark
{
    ColorBarMarks();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

struct CodeBarMarks : public GraphicalMark
{
    CodeBarMarks();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

struct QuadMarks : public GraphicalMark
{
    QuadMarks();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

struct CircleOrkMarks : public GraphicalMark
{
    CircleOrkMarks();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

struct ArrowMark : public GraphicalMark
{
    ArrowMark();
    using GraphicalMark::GraphicalMark;
    void paint(QPainter* painter, const geometry::Rect& area) const override;
};

class TaskMark
{
public:
    TaskMark() = default;
    TaskMark(QString name, MarkRole role, QString listName);

    QString name() const { return m_name; }
    MarkRole role() const { return m_role; }
    QString listName() const { return m_listName; }
    bool isRegistered() const { return m_role == MarkRole::Registration; }
    bool isTrim() const { return m_role == MarkRole::Trim; }
    bool isBleed() const { return m_role == MarkRole::Bleed; }
    bool isColorBar() const { return m_role == MarkRole::ColorBar; }

    void setName(const QString& name) { m_name = name; }
    void setListName(const QString& listName) { m_listName = listName; }

private:
    QString m_name;
    MarkRole m_role;
    QString m_listName;
};

class TaskMarkSet
{
public:
    static const TaskMarkSet& defaultFactory();

    TaskMarkSet();
    ~TaskMarkSet();

    void loadFromDocument(artplus::document::ArtPage* page);
    void saveToDocument(artplus::document::ArtPage* page);

    const QVector<TaskMark>& marks() const { return m_marks; }
    QVector<TaskMark>& marks() { return m_marks; }
    int count() const { return static_cast<int>(m_marks.size()); }

    TaskMark add(const TaskMark& mark);
    bool remove(const QString& name);
    TaskMark* find(const QString& name);
    const TaskMark* find(const QString& name) const;

    bool hasCrossoverMark() const;
    bool hasVersionGaugeMark() const;
    bool hasRepeatForSeparationMarks() const;
    bool hasRepeatForStationMarks() const;

    void flush();
    void applyRelevant(artplus::document::ArtPage* page, bool withReuse);

private:
    void makeDefaultOnPage(artplus::document::ArtPage* page);

    QVector<TaskMark> m_marks;
};

class SNRGridParams
{
public:
    int gridColumns = 5;
    int gridRows = 5;
    double cellSizeX = 200.0;
    double cellSizeY = 200.0;
    double gapX = 50.0;
    double gapY = 50.0;
    double bleed = 0.0;
    bool useBleed = false;
    double staggeredGap = 0.0;
    bool staggerEnabled = false;
    double rolloverDistance = 5.0;
    double headTurnAngle = 0.0;
    bool sequentialOrdering = true;
    bool overlapLabels = false;
    bool clipOverlaps = false;
    bool oneUpMask = false;
    int maskPattern = 1; // 1 = all / 2 = alternating 1U / 3 = alternating 2U

    double effectiveCellX() const;
    double effectiveCellY() const;
};

} // namespace artplus::marks
