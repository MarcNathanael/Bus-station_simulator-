function(add_unit_test TEST_NAME TEST_FILE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE gare_core gare_db)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_unit_test(test_voiture test_voiture.cpp)
#add_unit_test(test_planificateur test_planificateur.cpp)
