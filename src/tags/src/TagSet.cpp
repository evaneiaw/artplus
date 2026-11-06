#include "artplus/tags/TagSet.h"

namespace artplus::tags {

QJsonObject TagSet::toJson() const
{
    QJsonObject out;
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        switch (it.value().typeId()) {
        case QMetaType::Bool:
            out.insert(it.key(), it.value().toBool());
            break;
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::UInt:
        case QMetaType::ULongLong:
            out.insert(it.key(), static_cast<qint64>(it.value().toLongLong()));
            break;
        case QMetaType::Double:
        case QMetaType::Float:
            out.insert(it.key(), it.value().toDouble());
            break;
        default:
            out.insert(it.key(), it.value().toString());
            break;
        }
    }
    return out;
}

TagSet TagSet::fromJson(const QJsonObject& json)
{
    TagSet tags;
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        const QJsonValue v = it.value();
        if (v.isBool())
            tags.set(it.key(), v.toBool());
        else if (v.isDouble()) {
            const double d = v.toDouble();
            if (d == static_cast<qint64>(d))
                tags.set(it.key(), static_cast<qlonglong>(d));
            else
                tags.set(it.key(), d);
        } else
            tags.set(it.key(), v.toString());
    }
    return tags;
}

} // namespace artplus::tags
