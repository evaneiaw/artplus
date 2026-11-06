#include "artplus/objects/GraphicObject.h"

#include <QFont>
#include <QFontMetricsF>
#include <algorithm>
#include <atomic>

namespace artplus::objects {
using geometry::Rect;

namespace {
std::atomic<ObjectId> g_nextObjectId{1};
ObjectId nextId() { return g_nextObjectId.fetch_add(1); }
}

GraphicObject::GraphicObject(ObjectType type)
    : m_type(type)
    , m_id(nextId())
{
    m_name = QStringLiteral("%1 %2").arg(objectTypeName(type)).arg(m_id);
}

QString objectTypeName(ObjectType type)
{
    switch (type) {
    case ObjectType::Path: return QStringLiteral("Path");
    case ObjectType::Text: return QStringLiteral("Text");
    case ObjectType::Image: return QStringLiteral("Image");
    case ObjectType::Group: return QStringLiteral("Group");
    }
    return QStringLiteral("Object");
}

PathObject::PathObject()
    : GraphicObject(ObjectType::Path)
{
}

PathObject::PathObject(const geometry::Path& path)
    : GraphicObject(ObjectType::Path)
    , m_path(path)
{
}

std::unique_ptr<GraphicObject> PathObject::clone() const
{
    return std::make_unique<PathObject>(*this);
}

TextObject::TextObject()
    : GraphicObject(ObjectType::Text)
{
}

geometry::Rect TextObject::localBounds() const
{
    QFont font(m_fontFamily);
    font.setPointSizeF(std::max(0.1, m_fontSize));
    QFontMetricsF metrics(font);
    const QRectF r = metrics.boundingRect(m_text);
    double x = m_position.x;
    if (m_alignment == TextAlignment::Center)
        x -= r.width() / 2.0;
    else if (m_alignment == TextAlignment::Right)
        x -= r.width();
    return {x, m_position.y - r.height(), x + r.width(), m_position.y};
}

geometry::Path TextObject::outline() const
{
    geometry::Path p;
    const geometry::Rect b = localBounds();
    p.addRectangle(b);
    return p;
}

std::unique_ptr<GraphicObject> TextObject::clone() const
{
    return std::make_unique<TextObject>(*this);
}

ImageObject::ImageObject()
    : GraphicObject(ObjectType::Image)
{
}

geometry::Rect ImageObject::localBounds() const
{
    return Rect(0, 0, m_placedSize.width(), m_placedSize.height());
}

geometry::Path ImageObject::outline() const
{
    geometry::Path p;
    p.addRectangle(localBounds());
    return p;
}

std::unique_ptr<GraphicObject> ImageObject::clone() const
{
    return std::make_unique<ImageObject>(*this);
}

GroupObject::GroupObject()
    : GraphicObject(ObjectType::Group)
{
}

void GroupObject::addChild(std::unique_ptr<GraphicObject> child)
{
    if (child)
        m_children.push_back(std::move(child));
}

geometry::Rect GroupObject::localBounds() const
{
    geometry::Rect r;
    for (const auto& child : m_children)
        r = r.united(child->bounds());
    return r;
}

geometry::Path GroupObject::outline() const
{
    geometry::Path p;
    for (const auto& child : m_children) {
        geometry::Path childPath = child->outline();
        childPath.transform(child->transform());
        for (const auto& sp : childPath.subPaths()) {
            p.moveTo(sp.start);
            for (const auto& seg : sp.segments) {
                if (seg.type == geometry::Path::Segment::Type::Line)
                    p.lineTo(seg.p1);
                else
                    p.cubicTo(seg.p1, seg.p2, seg.p3);
            }
            if (sp.closed)
                p.close();
        }
    }
    return p;
}

std::unique_ptr<GraphicObject> GroupObject::clone() const
{
    auto copy = std::make_unique<GroupObject>();
    copy->setName(name());
    copy->setTransform(transform());
    copy->setFill(fill());
    copy->setStroke(stroke());
    copy->setStrokeWidth(strokeWidth());
    copy->setVisible(isVisible());
    copy->setLocked(isLocked());
    for (const auto& child : m_children)
        copy->addChild(child->clone());
    return copy;
}

} // namespace artplus::objects
