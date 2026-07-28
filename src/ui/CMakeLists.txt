file(GLOB UI_SOURCES "*.cpp" "*.h"
     "core/*.cpp" "core/*.h"
     "panels/*.cpp" "panels/*.h"
     "widgets/*.cpp" "widgets/*.h"
     "overlays/*.cpp" "overlays/*.h"
)

add_library(gare_ui STATIC ${UI_SOURCES})

target_include_directories(gare_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Liaison de ImGui-SFML, du Simulateur, et des composants SFML requis
target_link_libraries(gare_ui
    PUBLIC
        ImGui-SFML::ImGui-SFML
        gare_simulateur
        sfml-graphics
        sfml-window
        sfml-system
)