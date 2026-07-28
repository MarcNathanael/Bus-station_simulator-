add_library(gare_db STATIC
    DatabaseManager.cpp
    dal_voiture.cpp
    dal_client.cpp
    dal_billet.cpp
    dal_cooperative.cpp
    dal_destination.cpp
    dal_configuration.cpp
    dal_convoi.cpp
    dal_plage_interdite.cpp
)

# Accès automatique aux headers du module BDD
target_include_directories(gare_db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Liens : gare_core est PUBLIC pour propager ses headers aux utilisateurs de gare_db
target_link_libraries(gare_db
    PUBLIC gare_core
    PRIVATE SQLite::SQLite3
)