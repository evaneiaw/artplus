# Qt-free geometry core: primitives, segment intersections, Bezier curves and
# polygons. It deliberately does not link Qt, PDFium or artplus_core so that it
# can be reused by tools and by the future boolean/offset engine without pulling
# in the GUI stack (see ARTPLUS_STATUS.md, Milestone 24 / stage 1).
add_library(artplus_geometry_core STATIC
    src/Point.cpp
    src/Line.cpp
    src/Bezier.cpp
    src/Polygon.cpp
)
target_include_directories(artplus_geometry_core PUBLIC include)

# Qt-facing path builder. Path is consumed by the document, object, marks, PDF
# and UI layers, which already use QVector/QString; it stays in a separate
# target on top of the core instead of forcing a wide refactor outside this
# milestone.
add_library(artplus_geometry STATIC
    src/Path.cpp
)
target_include_directories(artplus_geometry PUBLIC include)
target_link_libraries(artplus_geometry PUBLIC artplus_geometry_core artplus_core Qt6::Core)
