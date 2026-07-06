cmake_minimum_required(VERSION 3.14)
project(GareRoutiere VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O0")

option(BUILD_TESTS "Construire les tests unitaires" ON)
option(BUILD_UI "Construire l'interface graphique Qt" OFF)

find_package(SQLite3 REQUIRED)

if(BUILD_UI)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(NOT Qt6_FOUND)
        find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)
    endif()
endif()

add_subdirectory(src/core)
add_subdirectory(src/db)
add_subdirectory(src/simulateur)

if(BUILD_UI)
    add_subdirectory(src/ui)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
