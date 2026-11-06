#pragma once

#include <QRectF>
#include <QString>
#include <QVector>
#include <utility>

#include "artplus/pdf/PdfDocument.h" // PageObjectKind used by the TAC results

namespace artplus::colors { struct Color; }

namespace artplus::pdf {
class PdfEditor;
struct PdfSnapshot;
} // namespace artplus::pdf

namespace artplus::preflight {

enum class CheckCategory {
    PageBoxes,
    Separations,
    Font,
    Image,
    Color,
    Resolution
};

enum class Severity { Warning, Error, Info };

struct CheckResult {
    CheckCategory category;
    Severity severity;
    QString title;
    QString description;
    QString suggestion;
    QString details; // e.g., page number, ink names, TAC value
};

struct Profile {
    int tacLimit = 300; // default TAC limit in %
    QVector<CheckCategory> enabledChecks;
};

// How confident the TAC number is. Only Exact means "real (c+m+y+k)*100 over
// object paint"; the other two must be surfaced to the user as limitations.
enum class TacPrecision {
    Exact,       // Per-object CMYK ink values (needs ArtPro+ DLL integration)
    Estimated,   // Derived from object kinds/areas, not from real CMYK ink
    Unsupported  // No colour information available through the current API
};

// Per-object breakdown of the TAC analysis.
struct TacObjectResult {
    int objectIndex = -1;              // index in the page snapshot
    artplus::pdf::PageObjectKind kind = artplus::pdf::PageObjectKind::Unknown;
    QString name;                      // human readable object kind
    double coverage = 0.0;             // coverage attributed to this object (0-100)
    TacPrecision precision = TacPrecision::Unsupported;
    bool hasAlpha = false;             // transparency channel known
    bool hasOverprint = false;         // overprint flag known
    QRectF bounds;                     // object bounds in page points
};

// Document-level TAC result.
struct TacAggregatedResult {
    double totalTac = 0.0;             // 0-400 %
    int tacLimit = 300;                // configured limit
    TacPrecision precision = TacPrecision::Unsupported;
    QVector<TacObjectResult> objectResults;
    QString calculationMethod;         // what was actually done
    QString limitations;               // what could not be done, and why
};

// (c+m+y+k)*100 for one explicit CMYK colour.
double calculateTac(const artplus::colors::Color& color);

// Document-level TAC analysis. `editor` may be null: without it only the object
// inventory is available and the result reports TacPrecision::Unsupported.
TacAggregatedResult calculateTacDocument(const artplus::pdf::PdfSnapshot& snapshot,
                                         const artplus::pdf::PdfEditor* editor, int tacLimit);

QVector<CheckResult> runChecks(const artplus::pdf::PdfDocument& document,
                               const artplus::pdf::PdfEditor* editor, const Profile& profile);

QVector<CheckResult> runChecks(const artplus::pdf::PdfDocument& document, const Profile& profile);

std::pair<QVector<CheckResult>, Profile> checkDocument(const artplus::pdf::PdfDocument& document,
                                                       const artplus::pdf::PdfEditor* editor,
                                                       Profile profile);

std::pair<QVector<CheckResult>, Profile> checkDocument(const artplus::pdf::PdfDocument& document,
                                                       Profile profile);

} // namespace artplus::preflight
