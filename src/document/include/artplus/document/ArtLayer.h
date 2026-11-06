#pragma once

#include "artplus/objects/GraphicObject.h"
#include "artplus/tags/TagSet.h"

#include <QString>
#include <memory>
#include <vector>

namespace artplus::document {

enum class LayerType {
    Artwork,
    Marks,
    Trapping,
    Dimensions,
    White,
    Varnish,
    Cut,
    Technical,
    Custom
};

QString layerTypeName(LayerType type);
LayerType layerTypeFromName(const QString& name);

class ArtLayer
{
public:
    ArtLayer(QString name, LayerType type = LayerType::Artwork);

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    LayerType type() const { return m_type; }
    void setType(LayerType type) { m_type = type; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }
    bool isPrinting() const { return m_printing; }
    void setPrinting(bool printing) { m_printing = printing; }

    double opacity() const { return m_opacity; }
    void setOpacity(double opacity);

    void addObject(std::unique_ptr<objects::GraphicObject> object);
    bool removeObject(objects::ObjectId id);
    objects::GraphicObject* findObject(objects::ObjectId id);
    const std::vector<std::unique_ptr<objects::GraphicObject>>& objects() const { return m_objects; }

    geometry::Rect bounds() const;
    int objectCount() const { return static_cast<int>(m_objects.size()); }

    tags::TagSet& tags() { return m_tags; }
    const tags::TagSet& tags() const { return m_tags; }

private:
    QString m_name;
    LayerType m_type = LayerType::Artwork;
    bool m_visible = true;
    bool m_locked = false;
    bool m_printing = true;
    double m_opacity = 1.0;
    std::vector<std::unique_ptr<objects::GraphicObject>> m_objects;
    tags::TagSet m_tags;
};

} // namespace artplus::document
