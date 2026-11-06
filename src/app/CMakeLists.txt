add_executable(ArtPlus WIN32 ../app/main.cpp)

target_link_libraries(ArtPlus PRIVATE artplus_ui)

# MainWindow lives in the artplus_ui static library (moc there); main.cpp has no
# Q_OBJECT class, so the executable itself does not need moc.
set_target_properties(ArtPlus PROPERTIES AUTOMOC OFF)

target_compile_definitions(ArtPlus PRIVATE ARTPLUS_VERSION_STRING="${PROJECT_VERSION}")

# Make the app runnable from the build tree: place the PDFium runtime next to it.
add_custom_command(TARGET ArtPlus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:ArtPlus>
    COMMENT "Copying pdfium runtime next to ArtPlus")
