add_library(artplus_stepandrepeat STATIC
    include/StepAndRepeat.h
)

target_include_directories(artplus_stepandrepeat PUBLIC include)
target_link_libraries(artplus_stepandrepeat PUBLIC artplus_core artplus_geometry artplus_colors artplus_tags Qt6::Core)
