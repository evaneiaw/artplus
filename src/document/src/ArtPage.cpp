#include "artplus/document/ArtPage.h"

#include <algorithm>

namespace artplus::document {
using geometry::Rect;

ArtPage::ArtPage(int index, const QSizeF& size)
    : m_index(index)
    , m_size(size)
{
}

void ArtPage::setSize(const QSizeF& size)
{
    m_size = size;
    m_size.setWidth(std::max(1.0, m_size.width()));
    m_size.setHeight(std::max(1.0, m_size.height()));
}

void ArtPage::setRotation(int degrees)
{
    m_rotation = ((degrees % 360) + 360) % 360;
}

geometry::Rect ArtPage::pageBox() const
{
    return Rect(0, 0, m_size.width(), m_size.height());
}

geometry::Rect ArtPage::bleedBox() const
{
    if (m_hasBleedBox)
        return m_bleedBox;
    return pageBox();
}

geometry::Rect ArtPage::trimBox() const
{
    if (m_hasTrimBox)
        return m_trimBox;
    return pageBox();
}

void ArtPage::setBleedBox(const geometry::Rect& box)
{
    m_bleedBox = box;
    m_hasBleedBox = true;
}

void ArtPage::setTrimBox(const geometry::Rect& box)
{
    m_trimBox = box;
    m_hasTrimBox = true;
}

ArtLayer* ArtPage::addLayer(const QString& name, LayerType type)
{
    if (findLayer(name))
        return findLayer(name);
    auto layer = std::make_unique<ArtLayer>(name, type);
    ArtLayer* raw = layer.get();
    m_layers.push_back(std::move(layer));
    return raw;
}

bool ArtPage::removeLayer(const QString& name)
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        if ((*it)->name().compare(name, Qt::CaseInsensitive) == 0) {
            m_layers.erase(it);
            return true;
        }
    }
    return false;
}

ArtLayer* ArtPage::findLayer(const QString& name)
{
    for (auto& layer : m_layers) {
        if (layer->name().compare(name, Qt::CaseInsensitive) == 0)
            return layer.get();
    }
    return nullptr;
}

const ArtLayer* ArtPage::findLayer(const QString& name) const
{
    for (const auto& layer : m_layers) {
        if (layer->name().compare(name, Qt::CaseInsensitive) == 0)
            return layer.get();
    }
    return nullptr;
}

std::vector<objects::GraphicObject*> ArtPage::allObjects()
{
    std::vector<objects::GraphicObject*> result;
    for (auto& layer : m_layers) {
        for (const auto& object : layer->objects())
            result.push_back(object.get());
    }
    return result;
}

int ArtPage::objectCount() const
{
    int count = 0;
    for (const auto& layer : m_layers)
        count += layer->objectCount();
    return count;
}

geometry::Rect ArtPage::contentBounds() const
{
    geometry::Rect result;
    for (const auto& layer : m_layers) {
        if (!layer->isVisible())
            continue;
        result = result.united(layer->bounds());
    }
    return result;
}

} // namespace artplus::document
