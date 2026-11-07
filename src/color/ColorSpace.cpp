#include "ColorSpace.h"

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
    double y = r * 0.2126 + g * 0.7152 + b * 0.0722;
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
    double b_val = lab.values[2];

    // Inverter a transformação Lab -> XYZ
    double yr = (l + 16.0) / 116.0;
    double xr = yr + a_val / 500.0;
    double zr = yr - b_val / 200.0;

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

Color labColor(double l, double a_val, double b_val, double alpha)
{
    Color col(ColorSpaceType::Lab, l, a_val, b_val, alpha);
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
// Overprint e Knockout
// ======================================================================

// Verificar se a cor deve fazer overprint (sobreposição)
// Retorna true se a cor de primeiro plano for clara o bastante para ser visível sobre o fundo
bool shouldOverprint(const Color& foreground, const Color& background) {
    // Simplificação: overprint é normalmente aplicado quando a cor de primeiro plano
    // tem alpha < 1.0 ou quando a cor é clara o bastante
    // Em CMYK, overprint é definido pelos valores de tinta
    if (foreground.space == ColorSpaceType::CMYK && background.space == ColorSpaceType::CMYK) {
        // Overprint é comum quando a soma das tintas não excede o TAC (Total Ink Coverage)
        // Para simplificação, consideramos overprint se o canal K do foreground for baixo
        return foreground.k < 0.5;
    }
    // Para RGB com alpha, overprint quando alpha < 1.0
    return foreground.alpha < 1.0;
}

// Aplicar overprint a uma cor (afeta como a cor se mistura com o que está por baixo)
Color applyOverprint(const Color& base, const Color& overprint, bool knockout) {
    Color result = base;
    // Em uma implementação completa, isso usaria a fórmula de sobreposição de tintas
    // CMYK overprint: C1 + C2, M1 + M2, Y1 + Y2, K1 + K2 (com limites)
    // Aqui fazemos uma mistura simples baseada no princípio do overprint
    if (overprint.space == ColorSpaceType::CMYK && base.space == ColorSpaceType::CMYK) {
        // Mistura simples: somar as componentes, mas limitar a 1.0
        result.values[0] = std::min(1.0, base.values[0] + overprint.values[0] - base.values[0] * overprint.values[0]); // Ciano
        result.values[1] = std::min(1.0, base.values[1] + overprint.values[1] - base.values[1] * overprint.values[1]); // Magenta
        result.values[2] = std::min(1.0, base.values[2] + overprint.values[2] - base.values[2] * overprint.values[2]); // Amarelo
        result.values[3] = std::min(1.0, base.values[3] + overprint.values[3]); // Key
    } else if (overprint.space == ColorSpaceType::RGB && base.space == ColorSpaceType::RGB) {
        // Mistura RGB sobreposta (tela)
        double ba = 1.0 - base.alpha;
        double oa = overprint.alpha;
        result.values[0] = 1.0 - (1.0 - ba * base.values[0]) * (1.0 - oa * overprint.values[0]);
        result.values[1] = 1.0 - (1.0 - ba * base.values[1]) * (1.0 - oa * overprint.values[1]);
        result.values[2] = 1.0 - (1.0 - ba * base.values[2]) * (1.0 - oa * overprint.values[2]);
        result.alpha = ba + oa * (1.0 - ba);
    }
    // Aplicar knockout se solicitado
    if (knockout) {
        // Remover a cor base onde o foreground está
        // Implementação simplificada
    }
    ColorConversion::clamp(result);
    return result;
}

// Verificar se knockout deve ser aplicado (remover a cor subjacente)
// Retorna true quando o foreground deve "recortar" o background
bool shouldKnockout(const Color& foreground, const Color& background) {
    // Knockout é aplicado quando queremos que o foreground "corte" o background
    // Normalmente usado com overprint: o fundo é removido onde o foreground está
    // Critério simples: foreground tem alpha significativo e cores contrastantes
    if (foreground.alpha < 0.1) return false;
    // Se as cores são muito diferentes, knockout tem mais efeito
    double diff = 0.0;
    if (foreground.space == ColorSpaceType::RGB && background.space == ColorSpaceType::RGB) {
        for (int i = 0; i < 3; ++i) diff += std::abs(foreground.values[i] - background.values[i]);
    }
    return diff > 0.5;
}

// Aplicar knockout: remover a cor subjacente onde o foreground está
Color applyKnockout(const Color& base, const Color& foreground) {
    // Remover a cor base onde o foreground está visível
    // Em overprint + knockout: o fundo é removido (tornado transparente/ branco)
    // onde o foreground está.
    Color result = foreground; // Começamos com o foreground
    if (foreground.space == ColorSpaceType::CMYK && base.space == ColorSpaceType::CMYK) {
        // Em CMYK com knockout: as áreas cobertas pelo foreground ficam com a cor de fundo removida
        // (normalmente substituído por branco ou transparência)
        // Aqui simplificamos: mantemos o foreground e "limpamos" o background
        result.k = foreground.k; // Manter o K do foreground
    }
    ColorConversion::clamp(result);
    return result;
}

// ======================================================================
// Rendering Intent
// ======================================================================

enum class RenderingIntent {
    Perceptual,              // Preservar proporções de cor (fotográfico)
    RelativeColorimetric,    // Pontos brancos/pretos mapeados, outras cores podem mudar
    AbsoluteColorimetric,    // Como RGB absolute, sem ajustar para white point
    Saturation               // Preservar saturação, ignorando brilho
};

// Converter cor com intent especificado
Color convertWithIntent(const Color& from, ColorSpaceType toSpace, RenderingIntent intent) {
    Color result = ColorConversion::convert(from, toSpace);
    // Ajustes finos baseados no intent
    switch (intent) {
        case RenderingIntent::Perceptual:
            // Já vem com preservação de proporções por padrão nas conversões
            break;
        case RenderingIntent::RelativeColorimetric:
            // Ajustar para manter o ponto branco e preto consistentes
            // (já implementado parcialmente na conversão Lab/RGB)
            break;
        case RenderingIntent::AbsoluteColorimetric:
            // Usar a cor absoluta sem ajustar o white point
            // Isso requer perfis ICC com white point explícito
            break;
        case RenderingIntent::Saturation:
            // Preservar saturação, pode mudar brilho
            // Ajustar as componentes para realçar saturação
            if (result.space == ColorSpaceType::RGB) {
                // Boost saturação simples
                double m = 1.2; // factor de saturação
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

// Obter intent recomendado para um uso específico
RenderingIntent recommendedIntent(ColorSpaceType fromSpace, ColorSpaceType toSpace, bool isProofing) {
    // Recomendações baseadas em parâmetros
    if (isProofing) {
        // Em prova de cor, usar RelativeColorimetric para manter a fidelidade de cores
        return RenderingIntent::RelativeColorimetric;
    }
    // Converter de RGB para CMYK geralmente usa Perceptual ou RelativeColorimetric
    if (fromSpace == ColorSpaceType::RGB && toSpace == ColorSpaceType::CMYK) {
        // Para proofing, use RelativeColorimetric; para workflow geral, use Perceptual
        return RenderingIntent::Perceptual; // default safe choice
    }
    // Outras combinações
    return RenderingIntent::Perceptual;
}

// ======================================================================
// Funções auxiliares adicionais
// ======================================================================

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

// ======================================================================
// Finalização
// ======================================================================

} // namespace artplus::color