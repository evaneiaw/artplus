add_library(artplus_export STATIC src/Export.cpp)
target_include_directories(artplus_export PUBLIC include)
target_link_libraries(artplus_export PUBLIC artplus_core Qt6::Core)
