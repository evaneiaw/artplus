add_library(artplus_tags STATIC
    src/TagSet.cpp
)

target_include_directories(artplus_tags PUBLIC include)
target_link_libraries(artplus_tags PUBLIC Qt6::Core)
