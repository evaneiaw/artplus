#pragma once

#include "artplus/tags/TagSet.h"
#include "artplus/geometry/Path.h"
#include "artplus/objects/Paint.h"

#include <QImage>
#include <QString>
#include <QVector>
#include <memory>
#include <vector>

namespace artplus::objects {

using ObjectId = quint64;

enum class ObjectType { Path, Text, Image, Group };

class GraphicObject
{
public:
    explicit GraphicObject(ObjectType type);
    virtual ~GraphicObject() = default;

    ObjectType type() const { return m_type; }
    ObjectId id() const { return m_id; }
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }

    const geometry::Matrix& transform() const { return m_transform; }
    void setTransform(const geometry::Matrix& m) { m_transform = m; }

    const Paint& fill() const { return m_fill; }
    Paint& fill() { return m_fill; }
    void setFill(const Paint& p) { m_fill = p; }
    const Paint& stroke() const { return m_stroke; }
    Paint& stroke() { return m_stroke; }
    void setStroke(const Paint& p) { m_stroke = p; }
    double strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(double w) { m_strokeWidth = w; }

    bool fillOverprint() const { return m_fillOverprint; }
    void setFillOverprint(bool v) { m_fillOverprint = v; }
    bool strokeOverprint() const { return m_strokeOverprint; }
    void setStrokeOverprint(bool v) { m_strokeOverprint = v; }
    bool knockout() const { return m_knockout; }
    void setKnockout(bool v) { m_knockout = v; }

    tags::TagSet& tags() { return m_tags; }
    const tags::TagSet& tags() const { return m_tags; }

    virtual geometry::Rect localBounds() const = 0;
    virtual geometry::Path outline() const = 0;
    geometry::Rect bounds() const { return m_transform.mapRect(localBounds()); }

    virtual std::unique_ptr<GraphicObject> clone() const = 0;

protected:
    GraphicObject(const GraphicObject&) = default;

private:
    ObjectType m_type;
    ObjectId m_id;
    QString m_name;
    bool m_visible = true;
    bool m_locked = false;
    geometry::Matrix m_transform;
    Paint m_fill;
    Paint m_stroke;
    double m_strokeWidth = 1.0;
    bool m_fillOverprint = false;
    bool m_strokeOverprint = false;
    bool m_knockout = true;
    tags::TagSet m_tags;
};

class PathObject : public GraphicObject
{
public:
    PathObject();
    explicit PathObject(const geometry::Path& path);

    const geometry::Path& path() const { return m_path; }
    geometry::Path& path() { return m_path; }
    void setPath(const geometry::Path& p) { m_path = p; }

    geometry::Rect localBounds() const override { return m_path.bounds(); }
    geometry::Path outline() const override { return m_path; }
    std::unique_ptr<GraphicObject> clone() const override;

private:
    geometry::Path m_path;
};

enum class TextAlignment { Left, Center, Right };

class TextObject : public GraphicObject
{
public:
    TextObject();

    QString text() const { return m_text; }
    void setText(const QString& text) { m_text = text; }
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& f) { m_fontFamily = f; }
    double fontSize() const { return m_fontSize; }
    void setFontSize(double size) { m_fontSize = size; }
    geometry::Point position() const { return m_position; }
    void setPosition(const geometry::Point& p) { m_position = p; }
    TextAlignment alignment() const { return m_alignment; }
    void setAlignment(TextAlignment a) { m_alignment = a; }

    geometry::Rect localBounds() const override;
    geometry::Path outline() const override;
    std::unique_ptr<GraphicObject> clone() const override;

private:
    QString m_text;
    QString m_fontFamily = QStringLiteral("Helvetica");
    double m_fontSize = 12.0;
    geometry::Point m_position;
    TextAlignment m_alignment = TextAlignment::Left;
};

class ImageObject : public GraphicObject
{
public:
    ImageObject();

    QString sourcePath() const { return m_sourcePath; }
    void setSourcePath(const QString& path) { m_sourcePath = path; }
    QSizeF placedSize() const { return m_placedSize; }
    void setPlacedSize(const QSizeF& size) { m_placedSize = size; }
    const QImage& preview() const { return m_preview; }
    void setPreview(const QImage& image) { m_preview = image; }

    geometry::Rect localBounds() const override;
    geometry::Path outline() const override;
    std::unique_ptr<GraphicObject> clone() const override;

private:
    QString m_sourcePath;
    QSizeF m_placedSize{100.0, 100.0};
    QImage m_preview;
};

class GroupObject : public GraphicObject
{
public:
    GroupObject();

    void addChild(std::unique_ptr<GraphicObject> child);
    const std::vector<std::unique_ptr<GraphicObject>>& children() const { return m_children; }
    std::vector<std::unique_ptr<GraphicObject>>& children() { return m_children; }
    int childCount() const { return static_cast<int>(m_children.size()); }

    geometry::Rect localBounds() const override;
    geometry::Path outline() const override;
    std::unique_ptr<GraphicObject> clone() const override;

private:
    std::vector<std::unique_ptr<GraphicObject>> m_children;
};

QString objectTypeName(ObjectType type);

} // namespace artplus::objects
