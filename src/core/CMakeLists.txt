add_library(gare_core STATIC
    Voiture.cpp
    Client.cpp
    Billet.cpp
    Cooperative.cpp
    Destination.cpp
    PlageInterdite.cpp
    Configuration.cpp
    Convoi.cpp
)

#quiconque se lie a la gare_core aura automatiquement acces a ses fichier d'entete
target_include_directories(gare_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
