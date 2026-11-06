#pragma once
#include <QString>
namespace artplus::exporter { enum class Format { PDF, TIFF, JDF, CIP3, ZIP }; bool exportDocument(const QString&, const QString&, int formatInt=0); }
