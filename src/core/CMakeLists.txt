add_library(artplus_core STATIC
    src/Logger.cpp
)

target_include_directories(artplus_core PUBLIC include)
target_link_libraries(artplus_core PUBLIC Qt6::Core)
target_compile_definitions(artplus_core PUBLIC ARTPLUS_VERSION_STRING="0.1.0")
