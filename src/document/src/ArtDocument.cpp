#include "artplus/document/ArtDocument.h"

#include <QFileInfo>
#include <algorithm>

namespace artplus::document {

ArtDocument::ArtDocument()
    : m_inkSet(colors::InkSet::processCmyk())
    , m_separations(colors::SeparationSet::processCmyk())
{
}

ArtDocument::~ArtDocument() = default;

void ArtDocument::setFilePath(const QString& path)
{
    m_filePath = path;
    if (m_name.isEmpty())
        m_name = QFileInfo(path).fileName();
}

ArtPage* ArtDocument::page(int index)
{
    if (index < 0 || index >= static_cast<int>(m_pages.size()))
        return nullptr;
    return m_pages[static_cast<size_t>(index)].get();
}

const ArtPage* ArtDocument::page(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_pages.size()))
        return nullptr;
    return m_pages[static_cast<size_t>(index)].get();
}

ArtPage* ArtDocument::addPage(const QSizeF& size)
{
    auto page = std::make_unique<ArtPage>(static_cast<int>(m_pages.size()), size);
    page->addLayer(QStringLiteral("Artwork"), LayerType::Artwork);
    ArtPage* raw = page.get();
    m_pages.push_back(std::move(page));
    m_modified = true;
    return raw;
}

bool ArtDocument::removePage(int index)
{
    if (index < 0 || index >= static_cast<int>(m_pages.size()))
        return false;
    m_pages.erase(m_pages.begin() + index);
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
        m_pages[static_cast<size_t>(i)]->setIndex(i);
    m_modified = true;
    return true;
}

bool ArtDocument::movePage(int from, int to)
{
    if (from < 0 || from >= static_cast<int>(m_pages.size()))
        return false;
    if (to < 0 || to >= static_cast<int>(m_pages.size()))
        return false;
    if (from == to)
        return true;
    auto page = std::move(m_pages[static_cast<size_t>(from)]);
    m_pages.erase(m_pages.begin() + from);
    m_pages.insert(m_pages.begin() + to, std::move(page));
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
        m_pages[static_cast<size_t>(i)]->setIndex(i);
    m_modified = true;
    return true;
}

geometry::Rect ArtDocument::contentBounds() const
{
    geometry::Rect result;
    for (const auto& page : m_pages)
        result = result.united(page->contentBounds());
    return result;
}

int ArtDocument::totalObjectCount() const
{
    int count = 0;
    for (const auto& page : m_pages)
        count += page->objectCount();
    return count;
}

int ArtDocument::totalLayerCount() const
{
    int count = 0;
    for (const auto& page : m_pages)
        count += page->layerCount();
    return count;
}

} // namespace artplus::document
