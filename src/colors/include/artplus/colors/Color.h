#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <array>

namespace artplus::colors {

enum class ColorSpace { Gray, RGB, CMYK, Spot };

struct Color {
    ColorSpace space = ColorSpace::CMYK;
    // Components are normalized 0..1. For gray only c is used, for RGB c/m/y.
    double c = 0.0;
    double m = 0.0;
    double y = 0.0;
    double k = 0.0;

    static Color cmyk(double c, double m, double y, double k);
    static Color rgb(double r, double g, double b);
    static Color gray(double g);
    static Color spot(double alternateC, double alternateM, double alternateY, double alternateK);

    QColor toRgb() const;
    QString toString() const;
    std::array<double, 4> components() const { return {c, m, y, k}; }
    double totalAreaCoverage() const; // for CMYK, (c+m+y+k)*100, clamped components

    bool operator==(const Color& o) const;
    bool operator!=(const Color& o) const { return !(*this == o); }
};

enum class InkType { Process, Spot, Technical };

struct Ink {
    QString name;
    QString book;
    InkType type = InkType::Process;
    Color preview = Color::cmyk(0, 0, 0, 0);
    Color alternate = Color::cmyk(0, 0, 0, 0);
    bool isPrinting = true;
    double dotGain = 0.0;
};

// Distance in a normalized CMYK space; used by trapping and ink tools.
double colorDistance(const Color& a, const Color& b);

class InkSet
{
public:
    InkSet();

    static InkSet processCmyk();

    const QVector<Ink>& inks() const { return m_inks; }
    QVector<Ink>& inks() { return m_inks; }
    int count() const { return m_inks.size(); }
    bool isEmpty() const { return m_inks.isEmpty(); }

    void addInk(const Ink& ink);
    bool removeInk(const QString& name);
    Ink* findInk(const QString& name);
    const Ink* findInk(const QString& name) const;
    bool contains(const QString& name) const;

    double totalAreaCoverage(const QVector<Color>& colors) const;
    QStringList names() const;

private:
    QVector<Ink> m_inks;
};

enum class PrintingState { Printing, NonPrinting };
enum class PrintingMethod { Normal, Opacity, Varnish, White, Technical, Custom };

struct Separation {
    QString name;
    InkType type = InkType::Process;
    PrintingState printingState = PrintingState::Printing;
    PrintingMethod method = PrintingMethod::Normal;
    Color preview;
    int plateOrder = 0;
};

class SeparationSet
{
public:
    SeparationSet();

    static SeparationSet processCmyk();

    const QVector<Separation>& separations() const { return m_separations; }
    QVector<Separation>& separations() { return m_separations; }
    int count() const { return m_separations.size(); }

    void add(const Separation& separation);
    bool remove(const QString& name);
    Separation* find(const QString& name);
    const Separation* find(const QString& name) const;

    QVector<Separation> printingSeparations() const;
    QVector<Separation> spotSeparations() const;

private:
    QVector<Separation> m_separations;
};

} // namespace artplus::colors
