add_library(artplus_edit STATIC
    src/Command.cpp
    src/Properties.cpp
    src/Transform.cpp
    include/artplus/edit/Command.h
    include/artplus/edit/Properties.h
    include/artplus/edit/Transform.h
)

target_include_directories(artplus_edit PUBLIC include)
target_link_libraries(artplus_edit PUBLIC
    artplus_pdf
    Qt6::Core
    Qt6::Gui
)
