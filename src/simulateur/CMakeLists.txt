#ces executable seront donc enfermer ici meme , elle n'est pas accessible dans test
add_library(Simulateur
    #main.cpp
    Simulateur.cpp
    Planificateur.cpp
    Billetterie.cpp
    Generateur.cpp
)

target_include_directories(Simulateur PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/simulateur
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/db
)

#C'est le join
target_link_libraries(Simulateur
    PRIVATE gare_db
    PRIVATE gare_core
)
