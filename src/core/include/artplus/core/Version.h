#pragma once

#include <QString>

#ifndef ARTPLUS_VERSION_STRING
#define ARTPLUS_VERSION_STRING "0.1.0"
#endif

namespace artplus {

inline constexpr const char* kApplicationName = "ArtPlus";
inline constexpr const char* kOrganizationName = "ArtPlus Project";
inline constexpr const char* kApplicationVersion = ARTPLUS_VERSION_STRING;

inline QString versionString()
{
    return QString::fromLatin1(kApplicationVersion);
}

} // namespace artplus
