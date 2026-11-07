#pragma once

#include <QString>
#include <QVector>
#include <cmath>

namespace artplus::color {

// ======================================================================
// Definições de Color Space
// ======================================================================

// Enumeração dos espaços de cor suportados
enum class ColorSpaceType {
    Unknown,
    RGB,        // Red Green Blue — device-dependent
    CMYK,       // Cyan Magenta Yellow Key (Black) — device-dependent
    Lab,        // CIELAB — perceptually uniform
    Gray,       // Grayscale
    Spot,       // Cor especial (Pantone, etc.)
    L*a*b*,     // Alias para Lab
    HSB,        // Hue Saturation Brightness
    HSL,        // Hue Saturation Lightness
    XYZ         // Espaço de referência CIE
};

// ======================================================================
// Struct Color — representação de cor em múltiplos espaços
// ======================================================================

struct Color {
    ColorSpaceType space = ColorSpaceType::RGB;
    double values[4] = {0.0, 0.0, 0.0, 0.0}; // Cmax, Cyan, Magenta, Yellow/Key etc.
    double alpha = 1.0; // Opacidade

    // Construtores
    Color() = default;
    Color(ColorSpaceType s, double r, double g, double b, double a = 1.0);
    Color(ColorSpaceType s, const double v[4], double a = 1.0);

    // Acesso conveniente por índice
    double& operator[](int i) { return values[i]; }
    double operator[](int i) const { return values[i]; }

    // Componentes RGB
    double r() const { return space == ColorSpaceType::RGB ? values[0] : 0.0; }
    double g() const { return space == ColorSpaceType::RGB ? values[1] : 0.0; }
    double b() const { return space == ColorSpaceType::RGB ? values[2] : 0.0; }

    // Componentes CMYK
    double c() const { return space == ColorSpaceType::CMYK ? values[0] : 0.0; }
    double m() const { return space == ColorSpaceType::CMYK ? values[1] : 0.0; }
    double y() const { return space == ColorSpaceType::CMYK ? values[2] : 0.0; }
    double k() const { return space == ColorSpaceType::CMYK ? values[3] : 0.0; }

    // Componentes Lab
    double l() const { return space == ColorSpaceType::Lab ? values[0] : 0.0; }
    double a() const { return space == ColorSpaceType::Lab ? values[1] : 0.0; }
    double b() const { return space == ColorSpaceType::Lab ? values[2] : 0.0; }

    // Igualdade
    bool operator==(const Color& other) const {
        return space == other.space &&
               std::abs(values[0] - other.values[0]) < 1e-10 &&
               std::abs(values[1] - other.values[1]) < 1e-10 &&
               std::abs(values[2] - other.values[2]) < 1e-10 &&
               std::abs(alpha - other.alpha) < 1e-10;
    }

    bool operator!=(const Color& other) const { return !(*this == other); }
};

// ======================================================================
// ColorConversion — funções de conversão entre espaços de cor
// ======================================================================

class ColorConversion {
public:
    // Converter de RGB para CMYK
    static Color rgbToCmyk(const Color& rgb);

    // Converter de CMYK para RGB
    static Color cmykToRgb(const Color& cmyk);

    // Converter de RGB para Lab
    static Color rgbToLab(const Color& rgb);

    // Converter de Lab para RGB
    static Color labToRgb(const Color& lab);

    // Converter de RGB para Gray
    static Color rgbToGray(const Color& rgb);

    // Converter de Gray para RGB
    static Color grayToRgb(const Color& gray);

    // Converter entre qualquer par de espaços
    static Color convert(const Color& from, ColorSpaceType toSpace);

    // Clamp de valores a [0,1]
    static void clamp(Color& c);

private:
// Matrizes de conversão padrões
    static constexpr double rgbToCmykMatrix[3][4] = {
        {-2.0/3.0, 1.0, 1.0, 1.0},
        {1.0, -2.0/3.0, 1.0, 1.0},
        {1.0, 1.0, -2.0/3.0, 1.0}
    };

    static constexpr double cmykToRgbMatrix[4][3] = {
        {0, 0, 0},      // K=1 => R=G=B=0
        {1, 0, 0},      // K=0, C=1 => R=0, G=1, B=1 (improper)
        {0, 1, 0},      // C=1 => G=0
        {0, 0, 1}       // C=1 => B=0
    };

    // Componentes normalizados CMYK -> RGB
    static void cmykNormalizedToRgb(double c, double m, double y, double k, double& r, double& g, double& b);
};

// ======================================================================
// ICC Profile Wrapper (esqueleto)
// ======================================================================

class IccProfile {
public:
    IccProfile() = default;
    ~IccProfile() = default;

    // Carregar perfil ICC de arquivo
    bool load(const QString& path);

    // Obter espaço de cor do perfil
    ColorSpaceType profileSpace() const;

    // Transformar cor usando o perfil
    Color transform(const Color& input) const;

    // Verificar se perfil é válido
    bool isValid() const { return !m_data.isEmpty(); }

private:
    QVector<uchar> m_data; // Dados do perfil ICC
    QString m_path;
    ColorSpaceType m_space = ColorSpaceType::Unknown;
};

// ======================================================================
// Color Management Functions
// ======================================================================

// Criar cor a partir de valores RGB (0.0 a 1.0)
Color rgbColor(double r, double g, double b, double a = 1.0);

// Criar cor a partir de valores CMYK (0.0 a 1.0)
Color cmykColor(double c, double m, double y, double k, double a = 1.0);

// Criar cor Lab
Color labColor(double l, double a, double b, double alpha = 1.0);

// Criar cor Gray
Color grayColor(double gray, double alpha = 1.0);

// Converter string hexadecimal para Color
Color hexToColor(const QString& hex);

// Converter Color para string hexadecimal
QString colorToHex(const Color& c, int precision = 2);

// Verificar se cor é preta (para key em CMYK)
bool isBlack(const Color& c);

// Retornar nome aproximado da cor (baseado em valores)
QString colorName(const Color& c);

// Espaço de cor padrão para novos objetos
const ColorSpaceType defaultSpace = ColorSpaceType::RGB;

// Limiar para consideração de cor "negativa" ou "zero"
constexpr double colorTolerance = 1e-6;

} // namespace artplus::color