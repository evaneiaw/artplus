add_library(artplus_colors STATIC
    src/Color.cpp
)

target_include_directories(artplus_colors PUBLIC include)
target_link_libraries(artplus_colors PUBLIC artplus_core Qt6::Core Qt6::Gui)
