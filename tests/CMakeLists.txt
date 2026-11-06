# Test executables for ArtPlus.

# Opt-in diagnostic, independent of Qt/PDFium and the dynamic MSVC CRT.
if(WIN32 AND MSVC)
    add_executable(artplus_runtime_probe EXCLUDE_FROM_ALL runtime_loader_probe.cpp)
    set_target_properties(artplus_runtime_probe PROPERTIES
        AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(artplus_pdf_smoke pdf_smoke.cpp)
target_link_libraries(artplus_pdf_smoke PRIVATE artplus_pdf Qt6::Core Qt6::Gui)
add_custom_command(TARGET artplus_pdf_smoke POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_pdf_smoke>
    COMMENT "Copying pdfium runtime next to artplus_pdf_smoke")

add_executable(artplus_pageview_test pageview_test.cpp)
target_link_libraries(artplus_pageview_test PRIVATE artplus_ui Qt6::Core Qt6::Gui Qt6::Widgets)

add_executable(artplus_mainwindow_test mainwindow_test.cpp)
target_link_libraries(artplus_mainwindow_test PRIVATE artplus_ui artplus_pdf Qt6::Core Qt6::Gui Qt6::Widgets)
add_custom_command(TARGET artplus_mainwindow_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_mainwindow_test>
    COMMENT "Copying pdfium runtime next to artplus_mainwindow_test")

add_executable(artplus_import_test import_test.cpp)
target_link_libraries(artplus_import_test PRIVATE artplus_pdf Qt6::Core Qt6::Gui)
add_custom_command(TARGET artplus_import_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_import_test>
    COMMENT "Copying pdfium runtime next to artplus_import_test")

add_executable(artplus_edit_test edit_test.cpp)
target_link_libraries(artplus_edit_test PRIVATE artplus_pdf Qt6::Core Qt6::Gui)
add_custom_command(TARGET artplus_edit_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_edit_test>
    COMMENT "Copying pdfium runtime next to artplus_edit_test")
add_executable(artplus_inspect inspect_tool.cpp)
target_link_libraries(artplus_inspect PRIVATE artplus_pdf Qt6::Core Qt6::Gui)
add_custom_command(TARGET artplus_inspect POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_inspect>
    COMMENT "Copying pdfium runtime next to artplus_inspect")
add_executable(artplus_command_test command_test.cpp)
target_link_libraries(artplus_command_test PRIVATE artplus_edit artplus_pdf Qt6::Core Qt6::Gui)
add_custom_command(TARGET artplus_command_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_command_test>
    COMMENT "Copying pdfium runtime next to artplus_command_test")

# Object paint read-back (fill/stroke colour) and the TAC precision contract.
add_executable(artplus_colour_test colour_test.cpp)
target_link_libraries(artplus_colour_test PRIVATE
    artplus_pdf artplus_preflight Qt6::Core Qt6::Gui)
add_custom_command(TARGET artplus_colour_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_colour_test>
    COMMENT "Copying pdfium runtime next to artplus_colour_test")

# Transform Again (Ctrl+D) - repeats the last transform on the current selection.
add_executable(artplus_transform_again_test transform_again_test.cpp)
target_link_libraries(artplus_transform_again_test PRIVATE
    artplus_edit artplus_pdf Qt6::Core Qt6::Gui)
add_custom_command(TARGET artplus_transform_again_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pdfium>
        $<TARGET_FILE_DIR:artplus_transform_again_test>
    COMMENT "Copying pdfium runtime next to artplus_transform_again_test")

# Keep generated PDFs and temporary import output inside the build tree.
# The edit regression generates the licence-clean multipage input needed by
# the smoke/import/window tests. CTest fixtures also arrange this dependency
# when a consumer alone is selected with -R.
set(artplus_test_workdir "${CMAKE_CURRENT_BINARY_DIR}/runtime")
file(MAKE_DIRECTORY "${artplus_test_workdir}")
set(artplus_test_pdf "${artplus_test_workdir}/artplus_edit_multipage_fixture.pdf")

add_test(NAME artplus.edit COMMAND artplus_edit_test)
set_tests_properties(artplus.edit PROPERTIES FIXTURES_SETUP artplus_pdf_fixture)
add_test(NAME artplus.command COMMAND artplus_command_test)
add_test(NAME artplus.pageview COMMAND artplus_pageview_test)
add_test(NAME artplus.smoke COMMAND artplus_pdf_smoke "${artplus_test_pdf}")
add_test(NAME artplus.import COMMAND artplus_import_test "${artplus_test_pdf}")
add_test(NAME artplus.mainwindow COMMAND artplus_mainwindow_test "${artplus_test_pdf}")
add_test(NAME artplus.colour COMMAND artplus_colour_test)
add_test(NAME artplus.transform_again COMMAND artplus_transform_again_test)
set_tests_properties(artplus.smoke artplus.import artplus.mainwindow PROPERTIES
    FIXTURES_REQUIRED artplus_pdf_fixture)
set_tests_properties(
    artplus.edit artplus.command artplus.pageview artplus.colour artplus.transform_again
    artplus.smoke artplus.import artplus.mainwindow PROPERTIES
    WORKING_DIRECTORY "${artplus_test_workdir}"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:artplus_edit_test>"
    TIMEOUT 60)
