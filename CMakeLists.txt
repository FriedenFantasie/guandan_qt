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

add_executable(GuandanQt WIN32
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
