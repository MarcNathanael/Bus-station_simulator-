add_executable(Simulateur
    main.cpp
    Simulateur.cpp
    Planificateur.cpp
)

target_include_directories(Simulateur PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Simulateur
    PRIVATE gare_db
    PRIVATE gare_core
)
