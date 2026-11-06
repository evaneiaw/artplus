#pragma once

#include "artplus/colors/Color.h"
#include "artplus/document/ArtPage.h"

#include <QHash>
#include <QString>
#include <memory>
#include <vector>

namespace artplus::document {

class ArtDocument
{
public:
    ArtDocument();
    ~ArtDocument();

    ArtDocument(const ArtDocument&) = delete;
    ArtDocument& operator=(const ArtDocument&) = delete;

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path);

    int pageCount() const { return static_cast<int>(m_pages.size()); }
    ArtPage* page(int index);
    const ArtPage* page(int index) const;
    ArtPage* addPage(const QSizeF& size);
    bool removePage(int index);
    bool movePage(int from, int to);
    const std::vector<std::unique_ptr<ArtPage>>& pages() const { return m_pages; }

    QHash<QString, QString> metadata() const { return m_metadata; }
    void setMetadata(const QHash<QString, QString>& metadata) { m_metadata = metadata; }
    void setMetadataValue(const QString& key, const QString& value) { m_metadata.insert(key, value); }
    QString metadataValue(const QString& key) const { return m_metadata.value(key); }

    colors::InkSet& inkSet() { return m_inkSet; }
    const colors::InkSet& inkSet() const { return m_inkSet; }
    colors::SeparationSet& separations() { return m_separations; }
    const colors::SeparationSet& separations() const { return m_separations; }

    bool isModified() const { return m_modified; }
    void setModified(bool modified) { m_modified = modified; }
    void touch() { m_modified = true; }

    geometry::Rect contentBounds() const;
    int totalObjectCount() const;
    int totalLayerCount() const;

    tags::TagSet& tags() { return m_tags; }
    const tags::TagSet& tags() const { return m_tags; }

private:
    QString m_name;
    QString m_filePath;
    QHash<QString, QString> m_metadata;
    std::vector<std::unique_ptr<ArtPage>> m_pages;
    colors::InkSet m_inkSet;
    colors::SeparationSet m_separations;
    bool m_modified = false;
    tags::TagSet m_tags;
};

} // namespace artplus::document
