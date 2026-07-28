cmake_minimum_required(VERSION 3.16)
project(GareRoutiere VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Sanitizers & Debug
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O0")

option(BUILD_TESTS "Construire les tests unitaires" ON)
option(BUILD_UI "Construire l'interface graphique SFML/ImGui" ON)

# ----------------------------------------------------
# Dépendances Backend
# ----------------------------------------------------
find_package(SQLite3 REQUIRED)

# ----------------------------------------------------
# Dépendances Frontend (SFML 2 + ImGui)
# ----------------------------------------------------

if(BUILD_UI)
    # 1. Recherche de SFML 2 (fourni par apt)
    find_package(SFML 2 COMPONENTS graphics window system REQUIRED)

    # 2. ImGui & ImGui-SFML
    include(FetchContent)

    # Déclaration de ImGui
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.89.9
    )

    # ÉTAPE CLÉ : On télécharge ImGui manuellement pour contourner l'absence de CMakeLists.txt
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    # On dit explicitement à ImGui-SFML où trouver les fichiers sources de ImGui
    set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Chemin source ImGui" FORCE)

    # Déclaration de ImGui-SFML
    FetchContent_Declare(
        ImGui-SFML
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        v2.6 
    )

    # On interdit à ImGui-SFML de chercher SFML
    set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)

    # On compile UNIQUEMENT ImGui-SFML (qui va englober le code de ImGui)
    FetchContent_MakeAvailable(ImGui-SFML)
endif()
# ----------------------------------------------------
# Inclusion des modules
# ----------------------------------------------------
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

# --- Exécutable Principal (UI) ---
add_executable(GareRoutiere src/main.cpp)
target_link_libraries(GareRoutiere PRIVATE gare_ui)
# S'assurer qu'il s'exécute depuis la racine du projet
set_target_properties(GareRoutiere PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})