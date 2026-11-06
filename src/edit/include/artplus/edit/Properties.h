#pragma once

#include "artplus/pdf/PdfEditor.h"

#include <QRectF>
#include <QString>

namespace artplus::edit {

// Real, measured properties of a selected object. Nothing here is estimated:
// every value comes from the imported PDFium object or its transform matrix.
struct ObjectProperties {
    bool valid = false;
    int index = -1;
    QString kind;

    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    QRectF bounds;

    bool hasRotation = false;
    double rotationDegrees = 0.0;

    bool modified = false;

    // Text
    QString text;
    QString fontName;
    double fontSize = 0.0;

    // Image
    int pixelWidth = 0;
    int pixelHeight = 0;
    double dpiX = 0.0;
    double dpiY = 0.0;

    // Form XObject
    int formChildren = 0;
};

ObjectProperties describe(const pdf::EditableObject& object);

} // namespace artplus::edit
