add_library(artplus_document STATIC
    src/ArtLayer.cpp
    src/ArtPage.cpp
    src/ArtDocument.cpp
)

target_include_directories(artplus_document PUBLIC include)
target_link_libraries(artplus_document PUBLIC
    artplus_core
    artplus_geometry
    artplus_colors
    artplus_tags
    artplus_objects
    Qt6::Core
    Qt6::Gui
)
