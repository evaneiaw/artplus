#pragma once

#include <QString>
#include <QtGlobal>
#include <cmath>

namespace artplus {

// All internal geometry is stored in PDF points (1 pt = 1/72 inch).
inline constexpr double kPointsPerInch = 72.0;
inline constexpr double kMillimetersPerInch = 25.4;
inline constexpr double kPointsPerMillimeter = kPointsPerInch / kMillimetersPerInch;

constexpr double mmToPoints(double mm) { return mm * kPointsPerMillimeter; }
constexpr double pointsToMm(double pt) { return pt / kPointsPerMillimeter; }
constexpr double pointsToInches(double pt) { return pt / kPointsPerInch; }
constexpr double inchesToPoints(double in) { return in * kPointsPerInch; }

inline QString formatMm(double points, int decimals = 2)
{
    return QString::number(pointsToMm(points), 'f', decimals);
}

inline QString formatPoints(double points, int decimals = 2)
{
    return QString::number(points, 'f', decimals);
}

inline QString formatSizeMm(double widthPt, double heightPt, int decimals = 2)
{
    return QStringLiteral("%1 x %2 mm").arg(pointsToMm(widthPt), 0, 'f', decimals)
        .arg(pointsToMm(heightPt), 0, 'f', decimals);
}

// Standard page formats in points.
struct StandardFormat {
    const char* name = "";
    double widthPt = 0.0;
    double heightPt = 0.0;
};

constexpr double kA4WidthPt = 595.275590551;
constexpr double kA4HeightPt = 841.889763780;
constexpr double kA3WidthPt = 841.889763780;
constexpr double kA3HeightPt = 1190.551181102;
constexpr double kSRA3WidthPt = 906.141732283;
constexpr double kSRA3HeightPt = 1275.590551181;
constexpr double kSRA2WidthPt = 1275.590551181;
constexpr double kSRA2HeightPt = 1814.173228346;

} // namespace artplus
