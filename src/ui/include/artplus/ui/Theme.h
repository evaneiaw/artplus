#pragma once

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPalette>
#include <QString>

class QApplication;

namespace artplus::ui {

// Visual language reverse-engineered from the ArtPro+ v24/v250 workspace
// (layout and colours only; all artwork here is original).
namespace theme {

// Chrome / workspace colours.
inline const QColor kChrome(0xF0, 0xF0, 0xF0);
inline const QColor kChromeDark(0xE4, 0xE4, 0xE4);
inline const QColor kBorder(0xD0, 0xD0, 0xD0);
inline const QColor kCanvas(0xDC, 0xDC, 0xDC);
inline const QColor kPanelHeader(0x59, 0x59, 0x59);
inline const QColor kPanelHeaderText(Qt::white);
inline const QColor kRowAlt(0xF7, 0xF7, 0xF7);
inline const QColor kHeaderRow(0xEC, 0xEC, 0xEC);
inline const QColor kText(0x1E, 0x1E, 0x1E);
inline const QColor kTextDisabled(0xA6, 0xA6, 0xA6);
inline const QColor kAccent(0xCF, 0xE3, 0xF7);
inline const QColor kAccentBorder(0x9C, 0xC3, 0xE8);
inline const QColor kError(0xD9, 0x3A, 0x2B);
inline const QColor kWarning(0xE0, 0xA8, 0x00);

inline const QColor kMediaBox(0xF0, 0x8A, 0x24);   // orange
inline const QColor kTrimBox(0x6A, 0x4C, 0xB8);    // purple
inline const QColor kBleedBox(0x2E, 0x9E, 0x5B);   // green
inline const QColor kArtBox(0x2F, 0x6F, 0xD0);     // blue

QFont uiFont(int pointSize = 9, bool bold = false);
QFont captionFont(); // small caption under toolbar icons

// Applies the Fusion style, palette and global stylesheet.
void apply(QApplication& app);

QString styleSheet();

// Tool icons are drawn so the project ships no proprietary artwork.
enum class IconKind {
    ObjectBrowser,
    Colors,
    Images,
    PlacedArt,
    Messages,
    ActionLists,
    ShareApprove,
    Clean,
    Select,
    Recolor,
    Shaper,
    Barcode,
    Screening,
    Trapping,
    Wrap,
    ThreeD,
    Preview,
    Eye,
    EyeClosed,
    Lock,
    LockOpen,
    Printer,
    Check,
    SquareOutline,
    Plus,
    Minus,
    LayersOrder,
    Gear,
    Info,
    Warning
};

QIcon icon(IconKind kind, const QColor& color = kText, int size = 24);

} // namespace theme
} // namespace artplus::ui
