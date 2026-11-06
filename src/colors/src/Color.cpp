#include "artplus/colors/Color.h"

#include <QStringList>
#include <algorithm>
#include <cmath>

namespace artplus::colors {

namespace {
double clamp01(double v) { return std::clamp(v, 0.0, 1.0); }
}

Color Color::cmyk(double c_, double m_, double y_, double k_)
{
    Color c;
    c.space = ColorSpace::CMYK;
    c.c = clamp01(c_);
    c.m = clamp01(m_);
    c.y = clamp01(y_);
    c.k = clamp01(k_);
    return c;
}

Color Color::rgb(double r, double g, double b)
{
    Color c;
    c.space = ColorSpace::RGB;
    c.c = clamp01(r);
    c.m = clamp01(g);
    c.y = clamp01(b);
    return c;
}

Color Color::gray(double g)
{
    Color c;
    c.space = ColorSpace::Gray;
    c.c = clamp01(g);
    return c;
}

Color Color::spot(double ac, double am, double ay, double ak)
{
    Color c = cmyk(ac, am, ay, ak);
    c.space = ColorSpace::Spot;
    return c;
}

QColor Color::toRgb() const
{
    switch (space) {
    case ColorSpace::Gray: {
        const int v = static_cast<int>(std::round(clamp01(c) * 255.0));
        return QColor(v, v, v);
    }
    case ColorSpace::RGB:
        return QColor::fromRgbF(clamp01(c), clamp01(m), clamp01(y));
    case ColorSpace::CMYK:
    case ColorSpace::Spot: {
        const double kk = clamp01(k);
        const double r = (1.0 - clamp01(c)) * (1.0 - kk);
        const double g = (1.0 - clamp01(m)) * (1.0 - kk);
        const double b = (1.0 - clamp01(y)) * (1.0 - kk);
        return QColor::fromRgbF(r, g, b);
    }
    }
    return Qt::black;
}

QString Color::toString() const
{
    switch (space) {
    case ColorSpace::Gray:
        return QStringLiteral("Gray %1%").arg(c * 100.0, 0, 'f', 1);
    case ColorSpace::RGB:
        return QStringLiteral("RGB %1/%2/%3")
            .arg(c * 255.0, 0, 'f', 0)
            .arg(m * 255.0, 0, 'f', 0)
            .arg(y * 255.0, 0, 'f', 0);
    case ColorSpace::CMYK:
        return QStringLiteral("CMYK %1/%2/%3/%4")
            .arg(c * 100.0, 0, 'f', 0)
            .arg(m * 100.0, 0, 'f', 0)
            .arg(y * 100.0, 0, 'f', 0)
            .arg(k * 100.0, 0, 'f', 0);
    case ColorSpace::Spot:
        return QStringLiteral("Spot (alt. %1/%2/%3/%4)")
            .arg(c * 100.0, 0, 'f', 0)
            .arg(m * 100.0, 0, 'f', 0)
            .arg(y * 100.0, 0, 'f', 0)
            .arg(k * 100.0, 0, 'f', 0);
    }
    return {};
}

double Color::totalAreaCoverage() const
{
    if (space == ColorSpace::Gray)
        return c * 100.0;
    if (space == ColorSpace::RGB)
        return 0.0;
    return (clamp01(c) + clamp01(m) + clamp01(y) + clamp01(k)) * 100.0;
}

bool Color::operator==(const Color& o) const
{
    return space == o.space && std::abs(c - o.c) < 1e-6 && std::abs(m - o.m) < 1e-6
        && std::abs(y - o.y) < 1e-6 && std::abs(k - o.k) < 1e-6;
}

double colorDistance(const Color& a, const Color& b)
{
    const QColor ca = a.toRgb();
    const QColor cb = b.toRgb();
    // Weighted RGB distance normalized to 0..100. This is a pragmatic preview
    // metric; real ICC DeltaE belongs to the color management module.
    const double dr = ca.redF() - cb.redF();
    const double dg = ca.greenF() - cb.greenF();
    const double db = ca.blueF() - cb.blueF();
    return std::sqrt(0.3 * dr * dr + 0.59 * dg * dg + 0.11 * db * db) * 100.0;
}

InkSet::InkSet() = default;

InkSet InkSet::processCmyk()
{
    InkSet set;
    set.addInk({QStringLiteral("Cyan"), QStringLiteral("Process"), InkType::Process,
                Color::cmyk(1, 0, 0, 0), Color::cmyk(1, 0, 0, 0), true, 0.0});
    set.addInk({QStringLiteral("Magenta"), QStringLiteral("Process"), InkType::Process,
                Color::cmyk(0, 1, 0, 0), Color::cmyk(0, 1, 0, 0), true, 0.0});
    set.addInk({QStringLiteral("Yellow"), QStringLiteral("Process"), InkType::Process,
                Color::cmyk(0, 0, 1, 0), Color::cmyk(0, 0, 1, 0), true, 0.0});
    set.addInk({QStringLiteral("Black"), QStringLiteral("Process"), InkType::Process,
                Color::cmyk(0, 0, 0, 1), Color::cmyk(0, 0, 0, 1), true, 0.0});
    return set;
}

void InkSet::addInk(const Ink& ink)
{
    if (findInk(ink.name))
        return;
    m_inks.push_back(ink);
}

bool InkSet::removeInk(const QString& name)
{
    for (int i = 0; i < m_inks.size(); ++i) {
        if (m_inks[i].name.compare(name, Qt::CaseInsensitive) == 0) {
            m_inks.removeAt(i);
            return true;
        }
    }
    return false;
}

Ink* InkSet::findInk(const QString& name)
{
    for (auto& ink : m_inks) {
        if (ink.name.compare(name, Qt::CaseInsensitive) == 0)
            return &ink;
    }
    return nullptr;
}

const Ink* InkSet::findInk(const QString& name) const
{
    for (const auto& ink : m_inks) {
        if (ink.name.compare(name, Qt::CaseInsensitive) == 0)
            return &ink;
    }
    return nullptr;
}

bool InkSet::contains(const QString& name) const
{
    return findInk(name) != nullptr;
}

double InkSet::totalAreaCoverage(const QVector<Color>& colors) const
{
    double maxCoverage = 0.0;
    for (const auto& color : colors)
        maxCoverage = std::max(maxCoverage, color.totalAreaCoverage());
    return maxCoverage;
}

QStringList InkSet::names() const
{
    QStringList result;
    for (const auto& ink : m_inks)
        result << ink.name;
    return result;
}

SeparationSet::SeparationSet() = default;

SeparationSet SeparationSet::processCmyk()
{
    SeparationSet set;
    const InkSet inks = InkSet::processCmyk();
    int order = 1;
    for (const auto& ink : inks.inks()) {
        Separation sep;
        sep.name = ink.name;
        sep.type = InkType::Process;
        sep.printingState = PrintingState::Printing;
        sep.method = PrintingMethod::Normal;
        sep.preview = ink.preview;
        sep.plateOrder = order++;
        set.add(sep);
    }
    return set;
}

void SeparationSet::add(const Separation& separation)
{
    if (find(separation.name))
        return;
    m_separations.push_back(separation);
}

bool SeparationSet::remove(const QString& name)
{
    for (int i = 0; i < m_separations.size(); ++i) {
        if (m_separations[i].name.compare(name, Qt::CaseInsensitive) == 0) {
            m_separations.removeAt(i);
            return true;
        }
    }
    return false;
}

Separation* SeparationSet::find(const QString& name)
{
    for (auto& sep : m_separations) {
        if (sep.name.compare(name, Qt::CaseInsensitive) == 0)
            return &sep;
    }
    return nullptr;
}

const Separation* SeparationSet::find(const QString& name) const
{
    for (const auto& sep : m_separations) {
        if (sep.name.compare(name, Qt::CaseInsensitive) == 0)
            return &sep;
    }
    return nullptr;
}

QVector<Separation> SeparationSet::printingSeparations() const
{
    QVector<Separation> result;
    for (const auto& sep : m_separations) {
        if (sep.printingState == PrintingState::Printing)
            result.push_back(sep);
    }
    return result;
}

QVector<Separation> SeparationSet::spotSeparations() const
{
    QVector<Separation> result;
    for (const auto& sep : m_separations) {
        if (sep.type == InkType::Spot)
            result.push_back(sep);
    }
    return result;
}

} // namespace artplus::colors
