#pragma once

#include "artplus/document/ArtLayer.h"

#include <QSizeF>
#include <QString>
#include <memory>
#include <vector>

namespace artplus::document {

class ArtPage
{
public:
    ArtPage(int index, const QSizeF& size);

    int index() const { return m_index; }
    void setIndex(int index) { m_index = index; }
    QSizeF size() const { return m_size; }
    void setSize(const QSizeF& size);
    int rotation() const { return m_rotation; }
    void setRotation(int degrees);
    bool isLandscape() const { return m_size.width() > m_size.height(); }

    // Page boxes in points, origin at top-left of the page.
    geometry::Rect pageBox() const;
    geometry::Rect bleedBox() const;
    geometry::Rect trimBox() const;
    void setBleedBox(const geometry::Rect& box);
    void setTrimBox(const geometry::Rect& box);

    ArtLayer* addLayer(const QString& name, LayerType type = LayerType::Artwork);
    bool removeLayer(const QString& name);
    ArtLayer* findLayer(const QString& name);
    const ArtLayer* findLayer(const QString& name) const;
    const std::vector<std::unique_ptr<ArtLayer>>& layers() const { return m_layers; }
    std::vector<std::unique_ptr<ArtLayer>>& layers() { return m_layers; }
    int layerCount() const { return static_cast<int>(m_layers.size()); }

    std::vector<objects::GraphicObject*> allObjects();
    int objectCount() const;
    geometry::Rect contentBounds() const;

    tags::TagSet& tags() { return m_tags; }
    const tags::TagSet& tags() const { return m_tags; }

private:
    int m_index = 0;
    QSizeF m_size{595.275590551, 841.889763780};
    int m_rotation = 0;
    geometry::Rect m_bleedBox;
    geometry::Rect m_trimBox;
    bool m_hasBleedBox = false;
    bool m_hasTrimBox = false;
    std::vector<std::unique_ptr<ArtLayer>> m_layers;
    tags::TagSet m_tags;
};

} // namespace artplus::document
