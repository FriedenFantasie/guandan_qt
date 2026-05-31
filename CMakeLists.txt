cmake_minimum_required(VERSION 3.21)

project(GuandanQt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(MINGW)
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

add_library(guandan_core
    src/AiPlayer.cpp
    src/Card.cpp
    src/GameEngine.cpp
    src/HandAnalyzer.cpp
)

target_include_directories(guandan_core PUBLIC src)

find_package(Qt6 COMPONENTS Widgets Svg REQUIRED)

set(APP_ICON_RESOURCE "")
if(WIN32)
    enable_language(RC)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_BINARY_DIR}/app_icon_res.o")
    add_custom_command(
        OUTPUT "${APP_ICON_RESOURCE}"
        COMMAND "${CMAKE_COMMAND}" -E chdir
                "${CMAKE_CURRENT_SOURCE_DIR}/resources"
                "${CMAKE_RC_COMPILER}" -O coff -i app_icon.rc -o "${APP_ICON_RESOURCE}"
        DEPENDS
            resources/app_icon.rc
            resources/icons/app_icon.ico
        VERBATIM
    )
    set_source_files_properties("${APP_ICON_RESOURCE}" PROPERTIES
        GENERATED TRUE
        EXTERNAL_OBJECT TRUE
    )
endif()

add_executable(GuandanQt WIN32
    ${APP_ICON_RESOURCE}
    resources/cards.qrc
    src/MainWindow.cpp
    src/TableWidget.cpp
    src/main.cpp
)

target_link_libraries(GuandanQt PRIVATE Qt6::Widgets Qt6::Svg guandan_core)

enable_testing()

add_executable(guandan_rules_tests
    tests/rules_tests.cpp
)

target_link_libraries(guandan_rules_tests PRIVATE guandan_core)
add_test(NAME guandan_rules_tests COMMAND guandan_rules_tests)
