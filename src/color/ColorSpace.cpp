//#include "ColorSpace.h"

#include <QStringList>
#include <algorithm>
#include <cmath>

// ======================================================================
// Construtores de Color
// ======================================================================

Color::Color(ColorSpaceType s, double r, double g, double b, double a)
    : space(s)
{
    if (space == ColorSpaceType::RGB) {
        values[0] = r;
        values[1] = g;
        values[2] = b;
        values[3] = 0.0;
    } else if (space == ColorSpaceType::CMYK) {
        values[0] = r;
        values[1] = g;
        values[2] = b;
        values[3] = 0.0;
    } else {
        values[0] = r;
        values[1] = g;
        values[2] = b;
        values[3] = 0.0;
    }
    alpha = a;
}

Color::Color(ColorSpaceType s, const double v[4], double a)
    : space(s)
{
    for (int i = 0; i < 4; ++i) values[i] = v[i];
    alpha = a;
}

// ======================================================================
// Conversoes RGB <-> CMYK
// ======================================================================

Color ColorConversion::rgbToCmyk(const Color& rgb)
{
    Color result(ColorSpaceType::CMYK);

    double r = rgb.values[0];
    double g = rgb.values[1];
    double b = rgb.values[2];

    double k = 1.0 - std::max({r, g, b});
    if (k < 0.0) k = 0.0;

    result.values[0] = 1.0 - r - k; // Ciano
    result.values[1] = 1.0 - g - k; // Magenta
    result.values[2] = 1.0 - b - k; // Amarelo
    result.values[3] = k;             // Key (black)
    result.alpha = rgb.alpha;

    if (k >= 1.0) {
        result.values[0] = 0.0;
        result.values[1] = 0.0;
        result.values[2] = 0.0;
    }

    clamp(result);
    return result;
}

Color ColorConversion::cmykToRgb(const Color& cmyk)
{
    Color result(ColorSpaceType::RGB);

    double c = cmyk.values[0];
    double m = cmyk.values[1];
    double y = cmyk.values[2];
    double k = cmyk.values[3];

    result.values[0] = 1.0 - c - k; // R
    result.values[1] = 1.0 - m - k; // G
    result.values[2] = 1.0 - y - k; // B
    result.alpha = cmyk.alpha;

    if (result.values[0] < 0.0) result.values[0] = 0.0;
    if (result.values[0] > 1.0) result.values[0] = 1.0;
    if (result.values[1] < 0.0) result.values[1] = 0.0;
    if (result.values[1] > 1.0) result.values[1] = 1.0;
    if (result.values[2] < 0.0) result.values[2] = 0.0;
    if (result.values[2] > 1.0) result.values[2] = 1.0;

    return result;
}

void ColorConversion::cmykNormalizedToRgb(double c, double m, double y, double k, double& r, double& g, double& b)
{
    r = 1.0 - c - k;
    g = 1.0 - m - k;
    b = 1.0 - y - k;

    if (r < 0.0) r = 0.0; if (r > 1.0) r = 1.0;
    if (g < 0.0) g = 0.0; if (g > 1.0) g = 1.0;
    if (b < 0.0) b = 0.0; if (b > 1.0) b = 1.0;
}

// ======================================================================
// Conversoes RGB <-> Lab
// ======================================================================

static constexpr double whiteX = 95.047;
static constexpr double whiteY = 100.000;
static constexpr double whiteZ = 108.883;

Color ColorConversion::rgbToLab(const Color& rgb)
{
    Color result(ColorSpaceType::Lab);

    double r = rgb.values[0];
    double g = rgb.values[1];
    double b = rgb.values[2];

    double x = r * 0.4124 + g * 0.3576 + b * 0.1805;
    double y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    double z = r * 0.0193 + g * 1.119 + b * 0.9505;

    x /= whiteX;
    y /= whiteY;
    z /= whiteZ;

    auto f = [](double t) -> double {
        if (t > 216.0 / 2436.0)
            return std::cbrt(t);
        else
            return t * (2436.0 / 27.0) / 16.0;
    };

    double xr = f(x);
    double yr = f(y);
    double zr = f(z);

    result.values[0] = 116.0 * yr - 16.0;      // L*
    result.values[1] = 500.0 * (xr - yr);       // a*
    result.values[2] = 200.0 * (yr - zr);        // b*

    result.alpha = rgb.alpha;
    return result;
}

Color ColorConversion::labToRgb(const Color& lab)
{
    Color result(ColorSpaceType::RGB);

    double l = lab.values[0];
    double a = lab.values[1];
    double b_val = lab.values[2];

    double yr = (l + 16.0) / 116.0;
    double xr = yr + a / 500.0;
    double zr = yr - b_val / 200.0;

    auto fx = [](double t) -> double {
        if (t > 216.0 / 2436.0)
            return t * t * t;
        else
            return t / (2436.0 / 27.0) * 16.0;
        };

    double x = fx(xr);
    double y = fx(yr);
    double z = fx(zr);

    x *= 95.047;
    y *= 100.000;
    z *= 108.883;

    result.values[0] =  3.2406 * x + -1.5372 * y +  0.4986 * z;
    result.values[1] = -0.9689 * x +  1.8758 * y +  0.0415 * z;
    result.values[2] =  0.0557 * x + -0.2040 * y +  1.0570 * z;

    for (int i = 0; i < 3; ++i) {
        if (result.values[i] < 0.0) result.values[i] = 0.0;
        if (result.values[i] > 1.0) result.values[i] = 1.0;
    }

    result.alpha = lab.alpha;
    result.space = ColorSpaceType::RGB;
    return result;
}

// ======================================================================
// Conversoes RGB <-> Gray
// ======================================================================

Color ColorConversion::rgbToGray(const Color& rgb)
{
    Color result(ColorSpaceType::Gray);
    double gray = 0.2126 * rgb.values[0] + 0.7152 * rgb.values[1] + 0.0722 * rgb.values[2];
    result.values[0] = gray;
    result.alpha = rgb.alpha;
    return result;
}

Color ColorConversion::grayToRgb(const Color& gray)
{
    Color result(ColorSpaceType::RGB);
    double g = gray.values[0];
    result.values[0] = g;
    result.values[1] = g;
    result.values[2] = g;
    result.alpha = gray.alpha;
    return result;
}

// ======================================================================
// Conversao geral
// ======================================================================

Color ColorConversion::convert(const Color& from, ColorSpaceType toSpace)
{
    if (from.space == ColorSpaceType::RGB && toSpace == ColorSpaceType::CMYK)
        return rgbToCmyk(from);
    if (from.space == ColorSpaceType::CMYK && toSpace == ColorSpaceType::RGB)
        return cmykToRgb(from);
    if (from.space == ColorSpaceType::RGB && toSpace == ColorSpaceType::Lab)
        return rgbToLab(from);
    if (from.space == ColorSpaceType::Lab && toSpace == ColorSpaceType::RGB)
        return labToRgb(from);
    if (from.space == ColorSpaceType::RGB && toSpace == ColorSpaceType::Gray)
        return rgbToGray(from);
    if (from.space == ColorSpaceType::Gray && toSpace == ColorSpaceType::RGB)
        return grayToRgb(from);

    if (from.space == toSpace)
        return from;

    return Color(ColorSpaceType::RGB, 0.0, 0.0, 0.0, from.alpha);
}

void ColorConversion::clamp(Color& c)
{
    for (int i = 0; i < 4; ++i) {
        if (c.values[i] < 0.0) c.values[i] = 0.0;
        if (c.values[i] > 1.0) c.values[i] = 1.0;
    }
    if (c.alpha < 0.0) c.alpha = 0.0;
    if (c.alpha > 1.0) c.alpha = 1.0;
}

// ======================================================================
// Funcoes de criacao de cor
// ======================================================================

Color rgbColor(double r, double g, double b, double a)
{
    Color c(ColorSpaceType::RGB, r, g, b, a);
    ColorConversion::clamp(c);
    return c;
}

Color cmykColor(double c, double m, double y, double k, double a)
{
    Color col(ColorSpaceType::CMYK, c, m, y, k);
    ColorConversion::clamp(col);
    return col;
}

Color labColor(double l, double a_val, double b_val, double alpha)
{
    Color col(ColorSpaceType::Lab, l, a_val, b_val, alpha);
    return col;
}

Color grayColor(double gray, double alpha)
{
    Color col(ColorSpaceType::Gray, gray, 0.0, 0.0, alpha);
    if (col.values[0] < 0.0) col.values[0] = 0.0;
    if (col.values[0] > 1.0) col.values[0] = 1.0;
    return col;
}

Color hexToColor(const QString& hex)
{
    QString h = hex;
    if (h.startsWith('#')) h = h.mid(1);
    while (h.length() < 6) h = '0' + h;
    if (h.length() > 6) h = h.left(6);

    bool okR, okG, okB;
    int r = h.mid(0, 2).toInt(&okR, 16);
    int g = h.mid(2, 2).toInt(&okG, 16);
    int b = h.mid(4, 2).toInt(&okB, 16);

    if (okR && okG && okB) {
        return rgbColor(r / 255.0, g / 255.0, b / 255.0);
    }
    return rgbColor(0.0, 0.0, 0.0);
}

QString colorToHex(const Color& c, int precision)
{
    Color rgb = ColorConversion::convert(c, ColorSpaceType::RGB);
    QString r = QString::number(qRound(rgb.values[0] * 255), 16).rightJustified(2, '0').toUpper();
    QString g = QString::number(qRound(rgb.values[1] * 255), 16).rightJustified(2, '0').toUpper();
    QString b = QString::number(qRound(rgb.values[2] * 255), 16).rightJustified(2, '0').toUpper();
    return '#' + r + g + b;
}

bool isBlack(const Color& c)
{
    if (c.space == ColorSpaceType::RGB) {
        return std::abs(c.values[0]) < colorTolerance &&
               std::abs(c.values[1]) < colorTolerance &&
               std::abs(c.values[2]) < colorTolerance;
    }
    if (c.space == ColorSpaceType::CMYK) {
        return std::abs(c.values[3] - 1.0) < colorTolerance;
    }
    return false;
}

QString colorName(const Color& c)
{
    if (c.space != ColorSpaceType::RGB) {
        return QString("Cor(%1)").arg(static_cast<int>(c.space));
    }

    double r = c.values[0];
    double g = c.values[1];
    double b = c.values[2];

    if (std::abs(r - g) < 0.1 && std::abs(g - b) < 0.1) {
        if (r < 0.1) return QString("Preto");
        if (r > 0.9) return QString("Branco");
        return QString("Cinza %1").arg(QString::number(r, 'f', 2));
    }

    if (r > 0.8 && g < 0.2 && b < 0.2) return QString("Vermelho");
    if (r < 0.2 && g > 0.8 && b < 0.2) return QString("Verde");
    if (r < 0.2 && g < 0.2 && b > 0.8) return QString("Azul");
    if (r > 0.8 && g > 0.8 && b < 0.2) return QString("Amarelo");
    if (r > 0.8 && g < 0.2 && b > 0.8) return QString("Magenta");
    if (r < 0.2 && g > 0.8 && b > 0.8) return QString("Ciano");

    return QString("Cor(%1,%2,%3)").arg(QString::number(r, 'f', 2))
                                  .arg(QString::number(g, 'f', 2))
                                  .arg(QString::number(b, 'f', 2));
}

const ColorSpaceType defaultSpace = ColorSpaceType::RGB;
constexpr double colorTolerance = 1e-6;

// ======================================================================
// Overprint e Knockout
// ======================================================================

bool shouldOverprint(const Color& foreground, const Color& background) {
    if (foreground.space == ColorSpaceType::CMYK && background.space == ColorSpaceType::CMYK) {
        return foreground.k < 0.5;
    }
    return foreground.alpha < 1.0;
}

Color applyOverprint(const Color& base, const Color& overprint, bool knockout) {
    Color result = base;
    if (overprint.space == ColorSpaceType::CMYK && base.space == ColorSpaceType::CMYK) {
        result.values[0] = std::min(1.0, base.values[0] + overprint.values[0] - base.values[0] * overprint.values[0]);
        result.values[1] = std::min(1.0, base.values[1] + overprint.values[1] - base.values[1] * overprint.values[1]);
        result.values[2] = std::min(1.0, base.values[2] + overprint.values[2] - base.values[2] * overprint.values[2]);
        result.values[3] = std::min(1.0, base.values[3] + overprint.values[3]);
    } else if (overprint.space == ColorSpaceType::RGB && base.space == ColorSpaceType::RGB) {
        double ba = 1.0 - base.alpha;
        double oa = overprint.alpha;
        result.values[0] = 1.0 - (1.0 - ba * base.values[0]) * (1.0 - oa * overprint.values[0]);
        result.values[1] = 1.0 - (1.0 - ba * base.values[1]) * (1.0 - oa * overprint.values[1]);
        result.values[2] = 1.0 - (1.0 - ba * base.values[2]) * (1.0 - oa * overprint.values[2]);
        result.alpha = ba + oa * (1.0 - ba);
    }
    if (knockout) {
        // Implementacao simplificada
    }
    ColorConversion::clamp(result);
    return result;
}

bool shouldKnockout(const Color& foreground, const Color& background) {
    if (foreground.alpha < 0.1) return false;
    double diff = 0.0;
    if (foreground.space == ColorSpaceType::RGB && background.space == ColorSpaceType::RGB) {
        for (int i = 0; i < 3; ++i) diff += std::abs(foreground.values[i] - background.values[i]);
    }
    return diff > 0.5;
}

Color applyKnockout(const Color& base, const Color& foreground) {
    Color result = foreground;
    if (foreground.space == ColorSpaceType::CMYK && base.space == ColorSpaceType::CMYK) {
        result.k = foreground.k;
    }
    ColorConversion::clamp(result);
    return result;
}

// ======================================================================
// Rendering Intent
// ======================================================================

enum class RenderingIntent {
    Perceptual,
    RelativeColorimetric,
    AbsoluteColorimetric,
    Saturation
};

Color convertWithIntent(const Color& from, ColorSpaceType toSpace, RenderingIntent intent) {
    Color result = ColorConversion::convert(from, toSpace);
    switch (intent) {
        case RenderingIntent::Perceptual:
            break;
        case RenderingIntent::RelativeColorimetric:
            break;
        case RenderingIntent::AbsoluteColorimetric:
            break;
        case RenderingIntent::Saturation:
            if (result.space == ColorSpaceType::RGB) {
                double m = 1.2;
                result.values[0] = std::min(1.0, result.values[0] * m);
                result.values[1] = std::min(1.0, result.values[1] * m);
                result.values[2] = std::min(1.0, result.values[2] * m);
            }
            break;
        default:
            break;
    }
    ColorConversion::clamp(result);
    return result;
}

RenderingIntent recommendedIntent(ColorSpaceType fromSpace, ColorSpaceType toSpace, bool isProofing) {
    if (isProofing) return RenderingIntent::RelativeColorimetric;
    if (fromSpace == ColorSpaceType::RGB && toSpace == ColorSpaceType::CMYK) {
        return RenderingIntent::Perceptual;
    }
    return RenderingIntent::Perceptual;
}

// ======================================================================
// Blend Modes
// ======================================================================

BlendMode blendModeFromString(const QString& modeName) {
    if (modeName == QStringLiteral("Normal")) return BlendMode::Normal;
    if (modeName == QStringLiteral("Multiply")) return BlendMode::Multiply;
    if (modeName == QStringLiteral("Screen")) return BlendMode::Screen;
    if (modeName == QStringLiteral("Overlay")) return BlendMode::Overlay;
    if (modeName == QStringLiteral("SoftLight")) return BlendMode::SoftLight;
    if (modeName == QStringLiteral("HardLight")) return BlendMode::HardLight;
    if (modeName == QStringLiteral("ColorDodge")) return BlendMode::ColorDodge;
    if (modeName == QStringLiteral("ColorBurn")) return BlendMode::ColorBurn;
    if (modeName == QStringLiteral("Darken")) return BlendMode::Darken;
    if (modeName == QStringLiteral("Lighten")) return BlendMode::Lighten;
    if (modeName == QStringLiteral("Difference")) return BlendMode::Difference;
    if (modeName == QStringLiteral("Exclusion")) return BlendMode::Exclusion;
    if (modeName == QStringLiteral("Hue")) return BlendMode::Hue;
    if (modeName == QStringLiteral("Saturation")) return BlendMode::Saturation;
    if (modeName == QStringLiteral("Color")) return BlendMode::Color;
    if (modeName == QStringLiteral("Luminosity")) return BlendMode::Luminosity;
    return BlendMode::Normal;
}

QString blendModeToString(BlendMode mode) {
    switch (mode) {
        case BlendMode::Normal: return QStringLiteral("Normal");
        case BlendMode::Multiply: return QStringLiteral("Multiply");
        case BlendMode::Screen: return QStringLiteral("Screen");
        case BlendMode::Overlay: return QStringLiteral("Overlay");
        case BlendMode::SoftLight: return QStringLiteral("SoftLight");
        case BlendMode::HardLight: return QStringLiteral("HardLight");
        case BlendMode::ColorDodge: return QStringLiteral("ColorDodge");
        case BlendMode::ColorBurn: return QStringLiteral("ColorBurn");
        case BlendMode::Darken: return QStringLiteral("Darken");
        case BlendMode::Lighten: return QStringLiteral("Lighten");
        case BlendMode::Difference: return QStringLiteral("Difference");
        case BlendMode::Exclusion: return QStringLiteral("Exclusion");
        case BlendMode::Hue: return QStringLiteral("Hue");
        case BlendMode::Saturation: return QStringLiteral("Saturation");
        case BlendMode::Color: return QStringLiteral("Color");
        case BlendMode::Luminosity: return QStringLiteral("Luminosity");
        default: return QStringLiteral("Normal");
    }
}

Color applyOpacity(const Color& c, double opacity) {
    Color result = c;
    result.alpha = std::max(0.0, std::min(1.0, c.alpha * opacity));
    ColorConversion::clamp(result);
    return result;
}

bool colorsOverlap(const Color& c1, const Color& c2, double threshold) {
    if (c1.space != c2.space) {
        Color c2c = ColorConversion::convert(c2, c1.space);
        return colorsOverlap(c1, c2c, threshold);
    }
    double diff = 0.0;
    for (int i = 0; i < 3; ++i) diff += std::abs(c1.values[i] - c2.values[i]);
    diff += std::abs(c1.alpha - c2.alpha);
    return diff < threshold;
}

Color overlayColors(const Color& c1, const Color& c2, BlendMode mode, double alpha1, double alpha2) {
    Color base = applyOpacity(c1, alpha1);
    Color blend = applyOpacity(c2, alpha2);
    if (blend.space != base.space) {
        blend = ColorConversion::convert(blend, base.space);
    }
    return BlendOperation::blend(base, blend, mode);
}

// ======================================================================
// BlendOperation
// ======================================================================

void BlendOperation::blendRGB(double baseR, double baseG, double baseB,
                              double blendR, double blendG, double blendB,
                              BlendMode mode, double& outR, double& outG, double& outB) {
    switch (mode) {
        case BlendMode::Normal:
            outR = baseR * 1.0 + blendR * 1.0;
            outG = baseG * 1.0 + blendG * 1.0;
            outB = baseB * 1.0 + blendB * 1.0;
            break;
        case BlendMode::Multiply:
            outR = baseR * blendR;
            outG = baseG * blendG;
            outB = baseB * blendB;
            break;
        case BlendMode::Screen:
            outR = 1.0 - (1.0 - baseR) * (1.0 - blendR);
            outG = 1.0 - (1.0 - baseG) * (1.0 - blendG);
            outB = 1.0 - (1.0 - baseB) * (1.0 - blendB);
            break;
        case BlendMode::Overlay:
            if (baseR > 0.5) {
                outR = 1.0 - (1.0 - baseR) * (1.0 - blendR);
                outG = 1.0 - (1.0 - baseG) * (1.0 - blendG);
                outB = 1.0 - (1.0 - baseB) * (1.0 - blendB);
            } else {
                outR = baseR * blendR;
                outG = baseG * blendG;
                outB = baseB * blendB;
            }
            break;
        case BlendMode::Darken:
            outR = (baseR < blendR) ? baseR : blendR;
            outG = (baseG < blendG) ? baseG : blendG;
            outB = (baseB < blendB) ? baseB : blendB;
            break;
        case BlendMode::Lighten:
            outR = (baseR > blendR) ? baseR : blendR;
            outG = (baseG > blendG) ? baseG : blendG;
            outB = (baseB > blendB) ? baseB : blendB;
            break;
        case BlendMode::Difference:
            outR = std::abs(baseR - blendR);
            outG = std::abs(baseG - blendG);
            outB = std::abs(baseB - blendB);
            break;
        case BlendMode::Exclusion:
            outR = baseR + blendR - 2.0 * baseR * blendR;
            outG = baseG + blendG - 2.0 * baseG * blendG;
            outB = baseB + blendB - 2.0 * baseB * blendB;
            break;
        case BlendMode::Hue:
        case BlendMode::Saturation:
        case BlendMode::Color:
        case BlendMode::Luminosity:
            outR = baseR;
            outG = baseG;
            outB = baseB;
            break;
        default:
            outR = baseR;
            outG = baseG;
            outB = baseB;
            break;
    }
    if (outR < 0.0) outR = 0.0; if (outR > 1.0) outR = 1.0;
    if (outG < 0.0) outG = 0.0; if (outG > 1.0) outG = 1.0;
    if (outB < 0.0) outB = 0.0; if (outB > 1.0) outB = 1.0;
}

void BlendOperation::blendCMYK(double baseC, double baseM, double baseY, double baseK,
                               double blendC, double blendM, double blendY, double blendK,
                               BlendMode mode, double& outC, double& outM, double& outY, double& outK) {
    switch (mode) {
        case BlendMode::Normal:
            outC = baseC; outM = baseM; outY = baseY; outK = baseK;
            break;
        case BlendMode::Multiply:
            outC = std::min(1.0, baseC + blendC);
            outM = std::min(1.0, baseM + blendM);
            outY = std::min(1.0, baseY + blendY);
            outK = std::min(1.0, baseK + blendK);
            break;
        case BlendMode::Screen:
            outC = 1.0 - (1.0 - baseC) * (1.0 - blendC);
            outM = 1.0 - (1.0 - baseM) * (1.0 - blendM);
            outY = 1.0 - (1.0 - baseY) * (1.0 - blendY);
            outK = 1.0 - (1.0 - baseK) * (1.0 - blendK);
            break;
        default:
            outC = baseC; outM = baseM; outY = baseY; outK = baseK;
            break;
    }
    if (outC < 0.0) outC = 0.0; if (outC > 1.0) outC = 1.0;
    if (outM < 0.0) outM = 0.0; if (outM > 1.0) outM = 1.0;
    if (outY < 0.0) outY = 0.0; if (outY > 1.0) outY = 1.0;
    if (outK < 0.0) outK = 0.0; if (outK > 1.0) outK = 1.0;
}

Color BlendOperation::blend(const Color& base, const Color& blendColor, BlendMode mode) {
    Color result;
    if (base.space == blendColor.space) {
        if (base.space == ColorSpaceType::RGB) {
            blendRGB(base.values[0], base.values[1], base.values[2],
                     blendColor.values[0], blendColor.values[1], blendColor.values[2],
                     mode, result.values[0], result.values[1], result.values[2]);
            result.alpha = base.alpha;
            result.space = ColorSpaceType::RGB;
        } else if (base.space == ColorSpaceType::CMYK) {
            blendCMYK(base.values[0], base.values[1], base.values[2], base.values[3],
                      blendColor.values[0], blendColor.values[1], blendColor.values[2], blendColor.values[3],
                      mode, result.values[0], result.values[1], result.values[2], result.values[3]);
            result.alpha = base.alpha;
            result.space = ColorSpaceType::CMYK;
        } else {
            result = base;
        }
    } else {
        Color blendConverted = ColorConversion::convert(blendColor, base.space);
        if (base.space == ColorSpaceType::RGB) {
            blendRGB(base.values[0], base.values[1], base.values[2],
                     blendConverted.values[0], blendConverted.values[1], blendConverted.values[2],
                     mode, result.values[0], result.values[1], result.values[2]);
            result.alpha = base.alpha;
            result.space = ColorSpaceType::RGB;
        } else if (base.space == ColorSpaceType::CMYK) {
            blendCMYK(base.values[0], base.values[1], base.values[2], base.values[3],
                      blendConverted.values[0], blendConverted.values[1], blendConverted.values[2], blendConverted.values[3],
                      mode, result.values[0], result.values[1], result.values[2], result.values[3]);
            result.alpha = base.alpha;
            result.space = ColorSpaceType::CMYK;
        } else {
            result = base;
        }
    }
    ColorConversion::clamp(result);
    return result;
}

// ======================================================================
// Separations (Separações de Cores)
// ======================================================================

Color extractCyan(const Color& cmyk) {
    Color result(ColorSpaceType::Gray);
    if (cmyk.space == ColorSpaceType::CMYK) {
        result.values[0] = cmyk.values[0];
    } else {
        Color cmykConverted = ColorConversion::convert(cmyk, ColorSpaceType::CMYK);
        result.values[0] = cmykConverted.values[0];
    }
    result.alpha = 1.0;
    ColorConversion::clamp(result);
    return result;
}

Color extractMagenta(const Color& cmyk) {
    Color result(ColorSpaceType::Gray);
    if (cmyk.space == ColorSpaceType::CMYK) {
        result.values[0] = cmyk.values[1];
    } else {
        Color cmykConverted = ColorConversion::convert(cmyk, ColorSpaceType::CMYK);
        result.values[0] = cmykConverted.values[1];
    }
    result.alpha = 1.0;
    ColorConversion::clamp(result);
    return result;
}

Color extractYellow(const Color& cmyk) {
    Color result(ColorSpaceType::Gray);
    if (cmyk.space == ColorSpaceType::CMYK) {
        result.values[0] = cmyk.values[2];
    } else {
        Color cmykConverted = ColorConversion::convert(cmyk, ColorSpaceType::CMYK);
        result.values[0] = cmykConverted.values[2];
    }
    result.alpha = 1.0;
    ColorConversion::clamp(result);
    return result;
}

Color extractBlack(const Color& cmyk) {
    Color result(ColorSpaceType::Gray);
    if (cmyk.space == ColorSpaceType::CMYK) {
        result.values[0] = cmyk.values[3];
    } else {
        Color cmykConverted = ColorConversion::convert(cmyk, ColorSpaceType::CMYK);
        result.values[0] = cmykConverted.values[3];
    }
    result.alpha = 1.0;
    ColorConversion::clamp(result);
    return result;
}

Color toSpotColor(const Color& cmyk, const QString& spotName) {
    Color result;
    result.space = ColorSpaceType::Spot;
    result.values[3] = spotName.length();
    result.alpha = 1.0;
    return result;
}

bool requiresSpecialSeparation(const Color& cmyk) {
    if (cmyk.space != ColorSpaceType::CMYK) return true;
    double totalInk = cmyk.values[0] + cmyk.values[1] + cmyk.values[2] + cmyk.values[3];
    if (totalInk > 1.5) return true;
    if (cmyk.k > 0.7) return true;
    return false;
}

// ======================================================================
// Finalização
// ======================================================================

} // namespace artplus::color