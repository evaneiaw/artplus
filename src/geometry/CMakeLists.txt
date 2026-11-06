add_library(artplus_geometry STATIC
    src/Point.cpp
    src/Path.cpp
)

target_include_directories(artplus_geometry PUBLIC include)
target_link_libraries(artplus_geometry PUBLIC artplus_core Qt6::Core)
