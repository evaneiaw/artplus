add_library(artplus_preflight STATIC
    include/artplus/preflight/Preflight.h
    preflight.cpp
)

# The public header is included as "artplus/preflight/Preflight.h", so the
# include root must be the directory that contains "artplus/".
target_include_directories(artplus_preflight PUBLIC include)
target_link_libraries(artplus_preflight PUBLIC
    artplus_core
    artplus_colors
    artplus_tags
    artplus_pdf
    Qt6::Core
)
