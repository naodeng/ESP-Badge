add_executable(codex_data_tests
    codex_data_tests.c
    ../../main/codex_data.c
    ../../managed_components/espressif__cjson/cJSON/cJSON.c)
target_include_directories(codex_data_tests PRIVATE
    ../../main
    ../../managed_components/espressif__cjson/cJSON)
target_compile_options(codex_data_tests PRIVATE -Wall -Wextra -Werror)
add_test(NAME codex_snapshot_parser_and_cache COMMAND codex_data_tests)

add_executable(codex_ui_tests
    codex_ui_tests.c
    ../../main/tools_ui.c
    ../../main/font_tools.c
    ../../main/font_cn16.c
    ../../main/font_codex_cn16.c)
target_include_directories(codex_ui_tests PRIVATE
    ../../main
    ../host)
target_link_libraries(codex_ui_tests PRIVATE lvgl)
target_compile_options(codex_ui_tests PRIVATE -Wall -Wextra -Werror -Wno-missing-field-initializers)
add_test(NAME codex_native_ui_interaction_and_fit COMMAND codex_ui_tests)
