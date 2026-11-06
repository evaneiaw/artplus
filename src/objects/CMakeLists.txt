add_library(artplus_objects STATIC
    src/GraphicObject.cpp
)

target_include_directories(artplus_objects PUBLIC include)
target_link_libraries(artplus_objects PUBLIC
    artplus_core
    artplus_geometry
    artplus_colors
    artplus_tags
    Qt6::Core
    Qt6::Gui
)
