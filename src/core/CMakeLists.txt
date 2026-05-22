add_library(gare_core STATIC
    Voiture.cpp
    Client.cpp
    Billet.cpp
    Cooperative.cpp
    Destination.cpp
    PlageInterdite.cpp
    configuration.cpp
    convoi.cpp
)

target_include_directories(gare_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
