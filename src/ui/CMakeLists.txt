add_executable(GareUI
    main.cpp
    mainwindow.cpp
)

target_link_libraries(GareUI
    PRIVATE gare_core
    PRIVATE gare_db
    PRIVATE Qt6::Widgets
)

target_include_directories(GareUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
