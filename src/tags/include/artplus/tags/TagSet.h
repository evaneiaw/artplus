#pragma once

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace artplus::tags {

class TagSet
{
public:
    TagSet() = default;

    void set(const QString& key, const QVariant& value) { m_values.insert(key, value); }
    QVariant get(const QString& key, const QVariant& fallback = {}) const
    {
        return m_values.value(key, fallback);
    }
    bool contains(const QString& key) const { return m_values.contains(key); }
    void remove(const QString& key) { m_values.remove(key); }
    void clear() { m_values.clear(); }
    bool isEmpty() const { return m_values.isEmpty(); }
    int size() const { return m_values.size(); }
    QStringList keys() const { return m_values.keys(); }
    const QHash<QString, QVariant>& values() const { return m_values; }

    QJsonObject toJson() const;
    static TagSet fromJson(const QJsonObject& json);

private:
    QHash<QString, QVariant> m_values;
};

} // namespace artplus::tags
