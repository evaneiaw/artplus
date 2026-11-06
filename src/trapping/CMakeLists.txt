add_library(artplus_trapping STATIC
    include/Trapping.h
)

target_include_directories(artplus_trapping PUBLIC include)
target_link_libraries(artplus_trapping PUBLIC artplus_core artplus_geometry artplus_colors artplus_tags Qt6::Core)
