add_library(artplus_automation STATIC
    include/Automation.h
)

target_include_directories(artplus_automation PUBLIC include)
target_link_libraries(artplus_automation PUBLIC artplus_core artplus_tags Qt6::Core)
