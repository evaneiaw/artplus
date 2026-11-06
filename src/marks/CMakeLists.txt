add_library(artplus_marks STATIC
    src/Marks.cpp
)

target_include_directories(artplus_marks PUBLIC include)
target_link_libraries(artplus_marks PUBLIC
    artplus_core
    artplus_geometry
    artplus_colors
    artplus_tags
    artplus_document
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
