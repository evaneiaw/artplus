#include "ColorSpace.h"

#include <QStringList>
#include <algorithm>
#include <cmath>

namespace artplus::color {

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
// Conversões RGB <-> CMYK
// ======================================================================

Color ColorConversion::rgbToCmyk(const Color& rgb)
{
    Color result(ColorSpaceType::CMYK);

    // RGB normalizado já está em [0,1]
    double r = rgb.values[0];
    double g = rgb.values[1];
    double b = rgb.values[2];

    // Fórmula padrão de conversão CMYK
    double k = 1.0 - std::max({r, g, b});
    if (k < 0.0) k = 0.0;

    result.values[0] = 1.0 - r - k; // Ciano
    result.values[1] = 1.0 - g - k; // Magenta
    result.values[2] = 1.0 - b - k; // Amarelo
    result.values[3] = k;             // Key (black)
    result.alpha = rgb.alpha;

    // Se o preto for 100%, as outras componentes são 0
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

    // Fórmula de conversão CMYK -> RGB
    result.values[0] = 1.0 - c - k; // R
    result.values[1] = 1.0 - m - k; // G
    result.values[2] = 1.0 - y - k; // B
    result.alpha = cmyk.alpha;

    // Clamp para [0,1]
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
    // Fórmula padrão: R = 1 - C - K, G = 1 - M - K, B = 1 - Y - K
    r = 1.0 - c - k;
    g = 1.0 - m - k;
    b = 1.0 - y - k;

    // Clamp
    if (r < 0.0) r = 0.0; if (r > 1.0) r = 1.0;
    if (g < 0.0) g = 0.0; if (g > 1.0) g = 1.0;
    if (b < 0.0) b = 0.0; if (b > 1.0) b = 1.0;
}

// ======================================================================
// Conversões RGB <-> Lab
// ======================================================================

// Espaço de referência D65 white point
static constexpr double whiteX = 95.047;
static constexpr double whiteY = 100.000;
static constexpr double whiteZ = 108.883;

Color ColorConversion::rgbToLab(const Color& rgb)
{
    Color result(ColorSpaceType::Lab);

    // Converter RGB de [0,1] para XYZ
    double r = rgb.values[0];
    double g = rgb.values[1];
    double b = rgb.values[2];

    // Transformação linear RGB -> XYZ (iluminante D65)
    double x = r * 0.4124 + g * 0.3576 + b * 0.1805;
    double y = r * 0.2126 + g * 71.52 + b * 0.0722;  // Corrigido
    double z = r * 0.0193 + g * 1.119 + b * 0.9505;

    // Normalizar para white point D65
    x /= whiteX;
    y /= whiteY;
    z /= whiteZ;

    // Aplicar função f (cube root ou linear)
    auto f = [](double t) -> double {
        if (t > 216.0 / 2436.0)
            return std::cbrt(t);
        else
            return t * (2436.0 / 27.0) / 16.0;
    };

    double xr = f(x);
    double yr = f(y);
    double zr = f(z);

    // Calcular L*, a*, b*
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
    double b = lab.values[2];

    // Inverter a transformação Lab -> XYZ
    double yr = (l + 16.0) / 116.0;
    double xr = yr + a / 500.0;
    double zr = yr - b / 200.0;

    // Função inversa f^{-1}
    auto fx = [](double t) -> double {
        if (t > 216.0 / 2436.0)
            return t * t * t;
        else
            return t / (2436.0 / 27.0) * 16.0;
        };

    double x = fx(xr);
    double y = fx(yr);
    double z = fx(zr);

    // Denormalizar para XYZ
    x *= 95.047;
    y *= 100.000;
    z *= 108.883;

    // Transformação XYZ -> RGB
    result.values[0] =  3.2406 * x + -1.5372 * y +  0.4986 * z;
    result.values[1] = -0.9689 * x +  1.8758 * y +  0.0415 * z;
    result.values[2] =  0.0557 * x + -0.2040 * y +  1.0570 * z;

    // Clamp para [0,1]
    for (int i = 0; i < 3; ++i) {
        if (result.values[i] < 0.0) result.values[i] = 0.0;
        if (result.values[i] > 1.0) result.values[i] = 1.0;
    }

    result.alpha = lab.alpha;
    result.space = ColorSpaceType::RGB;
    return result;
}

// ======================================================================
// Conversões RGB <-> Gray
// ======================================================================

Color ColorConversion::rgbToGray(const Color& rgb)
{
    Color result(ColorSpaceType::Gray);
    // Fórmula de luminância padrão (Rec. 709)
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
// Conversão geral
// ======================================================================

Color ColorConversion::convert(const Color& from, ColorSpaceType toSpace)
{
    // Caminhos de conversão mais comuns
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

    // Se já estiver no espaço solicitado
    if (from.space == toSpace)
        return from;

    // Default: retorna cor RGB preta
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
// Funções de criação de cor
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

Color labColor(double l, double a, double b, double alpha)
{
    Color col(ColorSpaceType::Lab, l, a, b, alpha);
    // Normalizar L* para [0,100], a* e b* para aproximadamente [-128,127]
    // O normalização completa seria feita na conversão
    return col;
}

Color grayColor(double gray, double alpha)
{
    Color col(ColorSpaceType::Gray, gray, 0.0, 0.0, alpha);
    // Clamp
    if (col.values[0] < 0.0) col.values[0] = 0.0;
    if (col.values[0] > 1.0) col.values[0] = 1.0;
    return col;
}

Color hexToColor(const QString& hex)
{
    // Remover '#' se presente
    QString h = hex;
    if (h.startsWith('#')) h = h.mid(1);

    // Normalizar para 6 dígitos
    while (h.length() < 6) h = '0' + h;
    if (h.length() > 6) h = h.left(6);

    // Parse dos componentes
    bool okR, okG, okB;
    int r = h.mid(0, 2).toInt(&okR, 16);
    int g = h.mid(2, 2).toInt(&okG, 16);
    int b = h.mid(4, 2).toInt(&okB, 16);

    if (okR && okG && okB) {
        return rgbColor(r / 255.0, g / 255.0, b / 255.0);
    }

    // Fallback: preto
    return rgbColor(0.0, 0.0, 0.0);
}

QString colorToHex(const Color& c, int precision)
{
    // Converter para RGB se necessário
    Color rgb = ColorConversion::convert(c, ColorSpaceType::RGB);

    // Formatar como #RRGGBB
    QString r = QString::number(qRound(rgb.values[0] * 255), 16).rightJustified(2, '0').toUpper();
    QString g = QString::number(qRound(rgb.values[1] * 255), 16).rightJustified(2, '0').toUpper();
    QString b = QString::number(qRound(rgb.values[2] * 255), 16).rightJustified(2, '0').toUpper();

    return '#' + r + g + b;
}

bool isBlack(const Color& c)
{
    // Verificar se a cor é essencialmente preta (RGB: 0,0,0 ou CMYK: K=1)
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
    // Nome aproximado baseado em ranges de valores
    // (implementação simplificada — em produção usaria tabela de lookup ou biblioteca)
    if (c.space != ColorSpaceType::RGB) {
        return QString("Cor(%1)").arg(static_cast<int>(c.space));
    }

    double r = c.values[0];
    double g = c.values[1];
    double b = c.values[2];

    // Verificar tons de cinza
    if (std::abs(r - g) < 0.1 && std::abs(g - b) < 0.1) {
        if (r < 0.1) return QString("Preto");
        if (r > 0.9) return QString("Branco");
        return QString("Cinza %1").arg(QString::number(r, 'f', 2));
    }

    // Verificar cores básicas (range amplo)
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

// Valor padrão
const ColorSpaceType defaultSpace = ColorSpaceType::RGB;

// Tolerância
constexpr double colorTolerance = 1e-6;

// ======================================================================
// Funções auxiliares adicionais
// ======================================================================

// Converter de HSB para RGB
Color hsbColor(double h, double s, double b, double a)
{
    Color result(ColorSpaceType::RGB);
    // Normalizar HSB
    h = fmod(h, 360.0); // Garantir que h esteja em [0,360)
    if (h < 0) h += 360.0;
    s = std::max(0.0, std::min(1.0, s));
    b = std::max(0.0, std::min(1.0, b));
    a = std::max(0.0, std::min(1.0, a));

    // Conversão HSB -> RGB
    int i = (int)std::floor(h / 60.0) % 6;
    double f = h / 60.0 - i;
    double p = b * (1.0 - s);
    double q = b * (1.0 - s * f);
    double t = b * (1.0 - s * (1.0 - f));

    double r, g, vb;
    switch (i) {
        case 0: r = b; g = t; vb = p; break;
        case 1: r = q; g = b; vb = p; break;
        case 2: r = p; g = b; vb = t; break;
        case 3: r = p; g = q; vb = b; break;
        case 4: r = t; g = p; vb = b; break;
        case 5: r = b; g = p; vb = q; break;
        default: r = b; g = vb; vb = p; break;
    }
    result.values[0] = r;
    result.values[1] = g;
    result.values[2] = vb;
    result.alpha = a;
    return result;
}

// Converter de RGB para HSB
void rgbToHsb(const Color& rgb, double& h, double& s, double& b, double& a) {
    a = rgb.alpha;
    double r = rgb.values[0];
    double g = rgb.values[1];
    double b_ = rgb.values[2];

    double maxVal = std::max({r, g, b_});
    double minVal = std::min({r, g, b_});
    double delta = maxVal - minVal;

    // Brilho
    b = maxVal;

    // Saturação
    if (maxVal > 0.0)
        s = delta / maxVal;
    else {
        s = 0.0;
        h = 0.0;
        return;
    }

    // Matiz
    if (r == maxVal)
        h = (g - b_) / delta;
    else if (g == maxVal)
        h = 2.0 + (b_ - r) / delta;
    else
        h = 4.0 + (r - g) / delta;

    h *= 60.0;
    if (h < 0) h += 360.0;
}

// Converter de HSL para RGB
Color hslColor(double h, double s, double l, double a) {
    Color result(ColorSpaceType::RGB);
    // Normalizar
    h = fmod(h, 360.0);
    if (h < 0) h += 360.0;
    s = std::max(0.0, std::min(1.0, s));
    l = std::max(0.0, std::min(1.0, l));
    a = std::max(0.0, std::min(1.0, a));

    double p = l <= 0.5 ? l * (1.0 + s) : l + s - l * s;
    double q = 2.0 * l - p;
    double hue = h / 360.0;

    auto hueToColor = [&](double h) -> double {
        if (h < 0) h += 1.0;
        if (h > 1.0) h -= 1.0;
        if (h < 1.0 / 6.0) return q + (p - q) * 6.0 * h;
        if (h < 1.0 / 2.0) return p;
        if (h < 2.0 / 3.0) return q + (p - q) * (2.0 / 3.0 - h) * 6.0;
        return q;
    };

    result.values[0] = hueToColor(hue + 1.0 / 3.0); // R
    result.values[1] = hueToColor(hue);             // G
    result.values[2] = hueToColor(hue - 1.0 / 3.0); // B
    return result;
}

// Converter de RGB para HSL
void rgbToHsl(const Color& rgb, double& h, double& s, double& l, double& a) {
    a = rgb.alpha;
    double r = rgb.values[0];
    double g = rgb.values[1];
    double b_ = rgb.values[2];

    double maxVal = std::max({r, g, b_});
    double minVal = std::min({r, g, b_});
    double delta = maxVal - minVal;

    // Luminância
    l = (maxVal + minVal) / 2.0;

    // Saturação
    if (delta == 0.0) {
        s = 0.0;
        h = 0.0;
        return;
    }
    if (l > 0.5) s = delta / (2.0 - maxVal - minVal);
    else s = delta / (maxVal + minVal);

    // Matiz
    if (r == maxVal)
        h = (g - b_) / delta;
    else if (g == maxVal)
        h = 2.0 + (b_ - r) / delta;
    else
        h = 4.0 + (r - g) / delta;

    h *= 60.0;
    if (h < 0) h += 360.0;
}

// ======================================================================
// Funções de gerenciamento de perfil ICC
// ======================================================================

// Carregar perfil ICC de arquivo
bool loadIccProfile(const QString& path, IccProfile& profile) {
    return profile.load(path);
}

// Obter espaço de cor de um perfil
ColorSpaceType profileSpace(const IccProfile& profile) {
    return profile.profileSpace();
}

// Transformar cor usando perfil ICC
Color transformColor(const IccProfile& profile, const Color& input) {
    return profile.transform(input);
}

// Verificar se perfil é válido
bool isValidProfile(const IccProfile& profile) {
    return profile.isValid();
}

// Função auxiliar: fmod que funciona como o fmod matemático (resultado tem mesmo sinal do divisor)
double fmod(double x, double y) {
    double r = x - y * std::floor(x / y);
    if ((y > 0 && r < 0) || (y < 0 && r > 0)) r += y;
    return r;
}

// Converter string hexadecimal para Color (já definido acima, mantido aqui para compatibilidade)
Color hexToColor(const QString& hex) {
    // Remover '#' se presente
    QString h = hex;
    if (h.startsWith('#')) h = h.mid(1);

    // Normalizar para 6 dígitos
    while (h.length() < 6) h = '0' + h;
    if (h.length() > 6) h = h.left(6);

    // Parse dos componentes
    bool okR, okG, okB;
    int r = h.mid(0, 2).toInt(&okR, 16);
    int g = h.mid(2, 2).toInt(&okG, 16);
    int b = h.mid(4, 2).toInt(&okB, 16);

    if (okR && okG && okB) {
        return rgbColor(r / 255.0, g / 255.0, b / 255.0);
    }

    // Fallback: preto
    return rgbColor(0.0, 0.0, 0.0);
}

// Função para verificar se dois colors são "iguais" considerando tolerância
bool colorsEqual(const Color& a, const Color& b) {
    if (a.space != b.space) return false;
    return std::abs(a.values[0] - b.values[0]) < colorTolerance &&
           std::abs(a.values[1] - b.values[1]) < colorTolerance &&
           std::abs(a.values[2] - b.values[2]) < colorTolerance &&
           std::abs(a.alpha - b.alpha) < colorTolerance;
}

// Formatar valor double para string com precisão especificada
QString doubleToString(double val, int precision) {
    return QString::number(val, 'f', precision);
}

} // namespace artplus::color