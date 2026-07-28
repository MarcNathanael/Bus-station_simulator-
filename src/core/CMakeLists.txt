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

# Tout module lié à gare_core aura automatiquement accès à ce dossier pour les #include
target_include_directories(gare_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})