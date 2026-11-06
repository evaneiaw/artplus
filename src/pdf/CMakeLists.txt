add_library(artplus_pdf STATIC
    src/PdfDocument.cpp
    src/PdfSnapshot.cpp
    src/PdfEditor.cpp
    include/artplus/pdf/PdfDocument.h
    include/artplus/pdf/PdfSnapshot.h
    include/artplus/pdf/PdfEditor.h
)

target_include_directories(artplus_pdf PUBLIC include)
target_link_libraries(artplus_pdf PUBLIC
    artplus_core
    pdfium
    Qt6::Core
    Qt6::Gui
)
