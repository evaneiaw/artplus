#include "artplus/preflight/Preflight.h"

#include "artplus/colors/Color.h"
#include "artplus/pdf/PdfDocument.h"
#include "artplus/pdf/PdfEditor.h"
#include "artplus/pdf/PdfSnapshot.h"

#include <algorithm>
#include <cmath>

namespace artplus::preflight {

namespace {

using artplus::colors::Color;
using artplus::pdf::EditableObject;
using artplus::pdf::PageObjectKind;
using artplus::pdf::PdfDocument;
using artplus::pdf::PdfEditor;
using artplus::pdf::PdfSnapshot;

// Naive RGB -> CMYK conversion (no ICC profile available in this build).
// This is the textbook unter-colour-removal mapping and it deliberately inflates
// coverage for saturated colours: it is used only to produce an *estimated*
// number that is always labelled as such, never as a measured TAC.
Color rgbToCmykEstimate(unsigned int packedArgb)
{
    const double r = ((packedArgb >> 16) & 0xFFu) / 255.0;
    const double g = ((packedArgb >> 8) & 0xFFu) / 255.0;
    const double b = (packedArgb & 0xFFu) / 255.0;
    const double alpha = ((packedArgb >> 24) & 0xFFu) / 255.0;

    const double k = 1.0 - std::max({r, g, b});
    double c = 0.0;
    double m = 0.0;
    double y = 0.0;
    if (k < 1.0) {
        const double denom = 1.0 - k;
        c = (1.0 - r - k) / denom;
        m = (1.0 - g - k) / denom;
        y = (1.0 - b - k) / denom;
    }
    const double scale = std::clamp(alpha, 0.0, 1.0);
    return Color::cmyk(std::clamp(c, 0.0, 1.0) * scale, std::clamp(m, 0.0, 1.0) * scale,
                       std::clamp(y, 0.0, 1.0) * scale, std::clamp(k, 0.0, 1.0) * scale);
}

QString kindName(PageObjectKind kind)
{
    switch (kind) {
    case PageObjectKind::Text: return QStringLiteral("texto");
    case PageObjectKind::Path: return QStringLiteral("path");
    case PageObjectKind::Image: return QStringLiteral("imagem");
    case PageObjectKind::Shading: return QStringLiteral("shading");
    case PageObjectKind::Form: return QStringLiteral("form");
    case PageObjectKind::Unknown: break;
    }
    return QStringLiteral("desconhecido");
}

} // namespace

// (c+m+y+k)*100 for an explicit CMYK colour. This is the only exact figure the
// module can produce on its own; it says nothing about the rest of the page.
double calculateTac(const Color& color)
{
    return color.totalAreaCoverage();
}

// Document-level TAC analysis.
//
// With an editor the module has access to the real paint of every object, so it
// reports the highest per-object coverage found on the page (that is the figure
// a print check cares about) and marks the result as `Estimated`, because the
// CMYK values come from a naive RGB->CMYK conversion instead of from the
// document's ink separations. Without an editor only the object inventory is
// available and the result is `Unsupported`.
TacAggregatedResult calculateTacDocument(const PdfSnapshot& snapshot, const PdfEditor* editor,
                                         int tacLimit)
{
    TacAggregatedResult result;
    result.tacLimit = tacLimit;
    result.totalTac = 0.0;
    result.precision = TacPrecision::Unsupported;
    result.calculationMethod = QStringLiteral("inventario de objetos (nenhum valor de tinta CMYK disponivel)");
    result.limitations.clear();

    if (snapshot.pageCount() == 0) {
        result.limitations = QStringLiteral("Nenhuma pagina encontrada.");
        return result;
    }

    int objectCount = 0;
    int objectsWithPaint = 0;
    double maxCoverage = 0.0;
    int maxObjectIndex = -1;
    QString maxObjectKind;

    if (editor && editor->isValid()) {
        for (int i = 0; i < editor->objectCount(); ++i) {
            const EditableObject* object = editor->object(i);
            if (!object)
                continue;
            ++objectCount;

            TacObjectResult objectResult;
            objectResult.objectIndex = i;
            objectResult.kind = object->kind;
            objectResult.name = kindName(object->kind);
            objectResult.bounds = object->bounds;
            objectResult.hasAlpha = false;
            objectResult.hasOverprint = false;

            double coverage = 0.0;
            if (object->hasFillColor) {
                coverage = std::max(coverage, calculateTac(rgbToCmykEstimate(object->fillColor)));
                objectResult.precision = TacPrecision::Estimated;
            }
            if (object->hasStrokeColor) {
                coverage = std::max(coverage, calculateTac(rgbToCmykEstimate(object->strokeColor)));
                objectResult.precision = TacPrecision::Estimated;
            }

            objectResult.coverage = coverage;
            if (objectResult.precision == TacPrecision::Estimated)
                ++objectsWithPaint;
            else
                objectResult.precision = TacPrecision::Unsupported;

            if (coverage > maxCoverage) {
                maxCoverage = coverage;
                maxObjectIndex = i;
                maxObjectKind = objectResult.name;
            }
            result.objectResults.append(objectResult);
        }
    } else {
        for (int p = 0; p < snapshot.pageCount(); ++p) {
            const artplus::pdf::SnapshotPage* page = snapshot.page(p);
            if (!page)
                continue;
            for (const auto& object : page->objects) {
                ++objectCount;
                TacObjectResult objectResult;
                objectResult.objectIndex = objectCount - 1;
                objectResult.kind = object.kind;
                objectResult.name = kindName(object.kind);
                objectResult.bounds = object.bounds;
                objectResult.precision = TacPrecision::Unsupported;
                result.objectResults.append(objectResult);
            }
        }
    }

    result.totalTac = maxCoverage;
    if (objectsWithPaint > 0) {
        result.precision = TacPrecision::Estimated;
        result.calculationMethod =
            QStringLiteral("maior cobertura por objeto a partir da cor de pintura real, "
                           "convertida RGB->CMYK de forma aproximada (sem perfil ICC)");
        result.limitations =
            QStringLiteral("Valor ESTIMADO. Nao considera tintas Spot/DeviceN, o interior de "
                           "imagens, gradientes (shading) nem grupos de transparencia.\n"
                           "Nao substitui a analise de separacoes do ArtPro+.");
    } else {
        result.limitations =
            QStringLiteral("Nao foi possivel obter valores de tinta CMYK: nenhum objeto com cor "
                           "de pintura legivel pela API PDFium deste projeto.\n"
                           "A extracao real de Separation/DeviceN exige as DLLs do ArtPro+ "
                           "(PDFObjectActions.dll / PreflightProfileLayer.dll), que nao estao "
                           "carregadas.");
    }

    result.limitations += QStringLiteral("\nObjetos analisados: %1 (%2 com pintura lida).")
                              .arg(objectCount)
                              .arg(objectsWithPaint);
    if (maxObjectIndex >= 0) {
        result.limitations +=
            QStringLiteral("\nMaior cobertura no objeto %1 (%2).").arg(maxObjectIndex).arg(maxObjectKind);
    }
    return result;
}

QVector<CheckResult> runChecks(const PdfDocument& document, const PdfEditor* editor,
                               const Profile& profile)
{
    QVector<CheckResult> results;

    if (!document.isValid())
        return results;

    const PdfSnapshot snapshot = PdfSnapshot::capture(document);

    // ---- Check 1: Total Area Coverage ----
    if (profile.enabledChecks.contains(CheckCategory::Color)) {
        const TacAggregatedResult tac = calculateTacDocument(snapshot, editor, profile.tacLimit);

        CheckResult item;
        item.category = CheckCategory::Color;
        switch (tac.precision) {
        case TacPrecision::Exact:
            item.severity = Severity::Info;
            item.title = QStringLiteral("Total Area Coverage [Exacto]");
            break;
        case TacPrecision::Estimated:
            item.severity = (tac.totalTac > tac.tacLimit) ? Severity::Warning : Severity::Info;
            item.title = QStringLiteral("Total Area Coverage [Estimado]");
            break;
        case TacPrecision::Unsupported:
        default:
            item.severity = Severity::Info;
            item.title = QStringLiteral("Total Area Coverage [Nao suportado]");
            break;
        }

        item.description = (tac.precision == TacPrecision::Unsupported)
            ? QStringLiteral("TAC nao calculado: sem valores de tinta CMYK acessiveis.")
            : QStringLiteral("Maior cobertura de tinta estimada a partir da cor de pintura dos objetos.");

        QString details;
        if (tac.precision == TacPrecision::Unsupported)
            details += QStringLiteral("TAC calculado: nao disponivel");
        else
            details += QStringLiteral("TAC calculado: %1 %").arg(tac.totalTac, 0, 'f', 1);
        details += QStringLiteral("\nLimite configurado: %1 %").arg(tac.tacLimit);
        details += QStringLiteral("\nMetodo: %1").arg(tac.calculationMethod);
        details += QStringLiteral("\n%1").arg(tac.limitations);
        item.details = details;

        if (tac.precision == TacPrecision::Unsupported)
            item.suggestion = QStringLiteral("Integrar a extracao real de tintas (Separation/DeviceN).");
        else if (tac.totalTac > tac.tacLimit)
            item.suggestion = QStringLiteral("Cobertura acima do limite: reduzir a sobreposicao de tintas.");
        else
            item.suggestion = QStringLiteral("Cobertura dentro do limite configurado.");

        results.append(item);
    }

    // ---- Check 2: Page Boxes ----
    if (profile.enabledChecks.contains(CheckCategory::PageBoxes)) {
        const QString intent = document.outputIntentName();
        results.append({CheckCategory::PageBoxes,
                        intent.isEmpty() ? Severity::Warning : Severity::Info,
                        QStringLiteral("Page Boxes [Informacional]"),
                        intent.isEmpty()
                            ? QStringLiteral("Sem output intent no documento.")
                            : QStringLiteral("Output intent presente: %1").arg(intent),
                        QStringLiteral("Verificar Media/Crop/Trim/Bleed/Art boxes no documento."),
                        QStringLiteral("Analise baseada em metadados; as caixas sao exibidas no "
                                       "viewport com View > Page Boxes.")});
    }

    // ---- Check 3: Separations ----
    if (profile.enabledChecks.contains(CheckCategory::Separations)) {
        const QString intent = document.outputIntentName();
        results.append({CheckCategory::Separations, Severity::Info,
                        QStringLiteral("Separations [Informacional]"),
                        intent.isEmpty()
                            ? QStringLiteral("Sem output intent: separacoes nao podem ser deduzidas.")
                            : QStringLiteral("Output intent presente: %1").arg(intent),
                        QStringLiteral("O output intent nao prova quais tintas existem no PDF."),
                        QStringLiteral("A enumeracao real de tintas (Separation/DeviceN/Spot) "
                                       "exige FPDFDoc_GetInkNames/GetSeparations, ausentes neste "
                                       "build do PDFium 158. O painel Separations mostra apenas "
                                       "as quatro tintas de processo como referencia de perfil.")});
    }

    // ---- Check 4: Fonts ----
    if (profile.enabledChecks.contains(CheckCategory::Font)) {
        results.append({CheckCategory::Font, Severity::Info,
                        QStringLiteral("Fonts [Informacional]"),
                        QStringLiteral("Verificacao de fontes habilitada (nivel de metadados)."),
                        QStringLiteral("Verificar incorporacao e subsetting das fontes."),
                        QStringLiteral("Sem analise de recursos de fonte nesta versao.")});
    }

    // ---- Check 5: Images ----
    if (profile.enabledChecks.contains(CheckCategory::Image)) {
        int images = 0;
        for (int p = 0; p < snapshot.pageCount(); ++p) {
            if (const artplus::pdf::SnapshotPage* page = snapshot.page(p))
                images += page->countOf(PageObjectKind::Image);
        }
        results.append({CheckCategory::Image, Severity::Info,
                        QStringLiteral("Images [Informacional]"),
                        QStringLiteral("%1 imagem(ns) no documento.").arg(images),
                        QStringLiteral("Verificar resolucao e modo de cor das imagens."),
                        QStringLiteral("Resolucao efetiva por imagem ainda nao e calculada.")});
    }

    // ---- Check 6: Resolution ----
    if (profile.enabledChecks.contains(CheckCategory::Resolution)) {
        results.append({CheckCategory::Resolution, Severity::Info,
                        QStringLiteral("Resolution [Informacional]"),
                        QStringLiteral("Verificacao de resolucao habilitada (nivel de metadados)."),
                        QStringLiteral("Referencia usual para impressao: 300 dpi."),
                        QStringLiteral("Sem calculo de resolucao efetiva nesta versao.")});
    }

    return results;
}

QVector<CheckResult> runChecks(const PdfDocument& document, const Profile& profile)
{
    return runChecks(document, nullptr, profile);
}

std::pair<QVector<CheckResult>, Profile> checkDocument(const PdfDocument& document,
                                                       const PdfEditor* editor, Profile profile)
{
    if (profile.enabledChecks.isEmpty()) {
        profile.enabledChecks = {CheckCategory::Color,     CheckCategory::PageBoxes,
                                 CheckCategory::Separations, CheckCategory::Font,
                                 CheckCategory::Image,     CheckCategory::Resolution};
    }
    return {runChecks(document, editor, profile), profile};
}

std::pair<QVector<CheckResult>, Profile> checkDocument(const PdfDocument& document, Profile profile)
{
    return checkDocument(document, nullptr, profile);
}

} // namespace artplus::preflight
