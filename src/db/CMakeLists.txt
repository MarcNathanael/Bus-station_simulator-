add_library(gare_db STATIC
    dal_voiture.cpp
    dal_client.cpp
    dal_billet.cpp
    dal_cooperative.cpp
    dal_destination.cpp
)

target_include_directories(gare_db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gare_db
    PUBLIC gare_core
    PRIVATE SQLite::SQLite3
)
