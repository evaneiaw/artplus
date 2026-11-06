#include "artplus/document/ArtLayer.h"

#include <algorithm>

namespace artplus::document {

QString layerTypeName(LayerType type)
{
    switch (type) {
    case LayerType::Artwork: return QStringLiteral("Artwork");
    case LayerType::Marks: return QStringLiteral("Marks");
    case LayerType::Trapping: return QStringLiteral("Trapping");
    case LayerType::Dimensions: return QStringLiteral("Dimensions");
    case LayerType::White: return QStringLiteral("White");
    case LayerType::Varnish: return QStringLiteral("Varnish");
    case LayerType::Cut: return QStringLiteral("Cut");
    case LayerType::Technical: return QStringLiteral("Technical");
    case LayerType::Custom: return QStringLiteral("Custom");
    }
    return QStringLiteral("Custom");
}

LayerType layerTypeFromName(const QString& name)
{
    const QString n = name.trimmed().toLower();
    if (n == QLatin1String("artwork")) return LayerType::Artwork;
    if (n == QLatin1String("marks")) return LayerType::Marks;
    if (n == QLatin1String("trapping")) return LayerType::Trapping;
    if (n == QLatin1String("dimensions")) return LayerType::Dimensions;
    if (n == QLatin1String("white")) return LayerType::White;
    if (n == QLatin1String("varnish")) return LayerType::Varnish;
    if (n == QLatin1String("cut")) return LayerType::Cut;
    if (n == QLatin1String("technical")) return LayerType::Technical;
    return LayerType::Custom;
}

ArtLayer::ArtLayer(QString name, LayerType type)
    : m_name(std::move(name))
    , m_type(type)
{
    m_printing = type == LayerType::Artwork || type == LayerType::White
              || type == LayerType::Varnish || type == LayerType::Custom;
}

void ArtLayer::setOpacity(double opacity)
{
    m_opacity = std::clamp(opacity, 0.0, 1.0);
}

void ArtLayer::addObject(std::unique_ptr<objects::GraphicObject> object)
{
    if (object)
        m_objects.push_back(std::move(object));
}

bool ArtLayer::removeObject(objects::ObjectId id)
{
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        if ((*it)->id() == id) {
            m_objects.erase(it);
            return true;
        }
    }
    return false;
}

objects::GraphicObject* ArtLayer::findObject(objects::ObjectId id)
{
    for (auto& object : m_objects) {
        if (object->id() == id)
            return object.get();
    }
    return nullptr;
}

geometry::Rect ArtLayer::bounds() const
{
    geometry::Rect result;
    for (const auto& object : m_objects)
        result = result.united(object->bounds());
    return result;
}

} // namespace artplus::document
