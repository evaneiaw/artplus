add_executable(ArtPlus WIN32 ../app/main.cpp)

target_link_libraries(ArtPlus PRIVATE artplus_ui)

# MainWindow lives in the artplus_ui static library (moc there); main.cpp has no
# Q_OBJECT class, so the executable itself does not need moc.
set_target_properties(ArtPlus PROPERTIES AUTOMOC OFF)

target_compile_definitions(ArtPlus PRIVATE ARTPLUS_VERSION_STRING="${PROJECT_VERSION}")

# ======================================================================
# Deployment: make ArtPlus.exe runnable from build tree AND from Explorer
# Copy all required DLLs: PDFium, Qt6 Core/Gui/Widgets, and platform plugin
# ======================================================================
if(WIN32)
    # Get Qt installation prefix from Qt6::Core target
    get_target_property(_qt_core_loc Qt6::Core LOCATION)
    get_filename_component(_qt_prefix "${_qt_core_loc}" DIRECTORY)
    get_filename_component(_qt_prefix "${_qt_prefix}" DIRECTORY)
    # _qt_prefix is now Qt installation root (e.g., C:/Qt/6.8.3/msvc2022_64)

    # Required Qt DLLs (only those available in Qt 6.8.3 MSVC)
    set(_QT_DLLS
        Qt6Core.dll
        Qt6Gui.dll
        Qt6Widgets.dll
    )

    # Copy Qt DLLs
    foreach(_dll ${_QT_DLLS})
        add_custom_command(TARGET ArtPlus POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_prefix}/bin/${_dll}"
                $<TARGET_FILE_DIR:ArtPlus>
            COMMENT "Copying Qt DLL: ${_dll}"
        )
    endforeach()

    # Copy platform plugin (qwindows.dll)
    add_custom_command(TARGET ArtPlus POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:ArtPlus>/platforms
        COMMENT "Creating platforms directory"
    )
    add_custom_command(TARGET ArtPlus POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_prefix}/plugins/platforms/qwindows.dll"
            $<TARGET_FILE_DIR:ArtPlus>/platforms/
        COMMENT "Copying Qt platform plugin: qwindows.dll"
    )

    # Copy PDFium runtime
    add_custom_command(TARGET ArtPlus POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:pdfium>
            $<TARGET_FILE_DIR:ArtPlus>
        COMMENT "Copying pdfium runtime next to ArtPlus"
    )

    # Set QT_QPA_PLATFORM_PLUGIN_PATH for build-tree execution
    set_target_properties(ArtPlus PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:ArtPlus>/platforms"
    )
endif()