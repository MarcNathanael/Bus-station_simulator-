add_library(gare_simulateur STATIC
    Simulateur.cpp
    Planificateur.cpp
    Billetterie.cpp
    Generateur.cpp
)

# Seul le dossier courant du simulateur est nécessaire
target_include_directories(gare_simulateur PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Liaison transitive : PUBLIC permet au module UI et aux Tests de se lier
# uniquement à gare_simulateur et de récupérer automatiquement gare_core et gare_db
target_link_libraries(gare_simulateur
    PUBLIC
        gare_db
        gare_core
)