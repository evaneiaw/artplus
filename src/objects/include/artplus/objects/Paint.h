#pragma once

#include "artplus/colors/Color.h"
#include "artplus/geometry/Point.h"

#include <QVector>

namespace artplus::objects {

enum class PaintType { None, Solid, LinearGradient, RadialGradient, Pattern };

struct GradientStop {
    double offset = 0.0;
    colors::Color color;
};

struct Paint {
    PaintType type = PaintType::None;
    colors::Color color;
    QVector<GradientStop> stops;
    geometry::Point start;
    geometry::Point end;
    double opacity = 1.0;

    static Paint none() { return {}; }
    static Paint solid(const colors::Color& c)
    {
        Paint p;
        p.type = PaintType::Solid;
        p.color = c;
        return p;
    }

    bool isVisible() const { return type != PaintType::None; }
};

} // namespace artplus::objects
