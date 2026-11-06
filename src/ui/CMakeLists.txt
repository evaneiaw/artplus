add_library(artplus_ui STATIC
    src/MainWindow.cpp
    src/PageView.cpp
    src/Panel.cpp
    src/Theme.cpp
    src/TransformDialog.cpp
    src/DocumentManager.cpp
    src/StatusBarPanel.cpp
    include/artplus/ui/MainWindow.h
    include/artplus/ui/PageView.h
    include/artplus/ui/Panel.h
    include/artplus/ui/Theme.h
    include/artplus/ui/TransformDialog.h
    include/artplus/ui/StatusBarPanel.h
    include/artplus/ui/DocumentManager.h
)

target_include_directories(artplus_ui PUBLIC include)
target_link_libraries(artplus_ui PUBLIC
    artplus_core
    artplus_document
    artplus_pdf
    artplus_edit
    artplus_preflight
    artplus_ui_components
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
add_subdirectory(components)
