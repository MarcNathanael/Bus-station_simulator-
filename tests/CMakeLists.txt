#ne permet de tester qu'un fichier 
#function(add_unit_test TEST_NAME TEST_FILE)
#    add_executable(${TEST_NAME} ${TEST_FILE})
#    target_link_libraries(${TEST_NAME} PRIVATE gare_core gare_db)
#    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#endfunction()

#add_unit_test(test_voiture test_voiture.cpp)
#add_unit_test(test_planificateur test_planificateur.cpp)

# On modifie ta fonction pour accepter des fichiers sources supplémentaires avec ${ARGN}
function(add_unit_test TEST_NAME TEST_FILE)
    # ${ARGN} capture les fichiers additionnels si nécessaire
    add_executable(${TEST_NAME} ${TEST_FILE} ${ARGN})
    
    # ON LIE TOUT ICI : Plus besoin de compiler manuellement les .cpp !
    # 'Simulateur' apporte son code, 'gare_db' apporte les DALs, et 'gare_core' apporte les modèles.
    target_link_libraries(${TEST_NAME} 
        PRIVATE Simulateur 
        PRIVATE gare_db 
        PRIVATE gare_core
        PRIVATE sqlite3 # Très important sous Linux pour DatabaseManager
    )
    
    # On indique où chercher les headers
    target_include_directories(${TEST_NAME} PRIVATE 
        ${CMAKE_SOURCE_DIR}/src/simulateur
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/db
    )
    
    # Forcer l'exécution depuis la racine (parfait pour tes CSV !)
    add_test(NAME ${TEST_NAME} 
             COMMAND ${TEST_NAME} 
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endfunction()


# ---------------------------------------------------------
# 1. TEST VOITURE (Basique)
# ---------------------------------------------------------
#add_unit_test(test_voiture test_voiture.cpp)

# ---------------------------------------------------------
# 2. TEST PLANIFICATEUR (Intégration)
# ---------------------------------------------------------
# On lui passe le fichier de test ET les sources du simulateur dont il dépend
#add_unit_test(test_planificateur 
#    test_planificateur.cpp
#    ${CMAKE_SOURCE_DIR}/src/simulateur/Planificateur.cpp
#    ${CMAKE_SOURCE_DIR}/src/simulateur/Billetterie.cpp
#    ${CMAKE_SOURCE_DIR}/src/simulateur/Generateur.cpp
#)

# 3. Test PLANIFICATEUR 2 
#add_unit_test(test_planificateur
#    test_planificateur2.cpp
#    ${CMAKE_SOURCE_DIR}/src/simulateur/Planificateur.cpp
#    ${CMAKE_SOURCE_DIR}/src/simulateur/Billetterie.cpp
#    ${CMAKE_SOURCE_DIR}/src/simulateur/Generateur.cpp
#)
# 4. Test Simulateur 
# Désormais ton appel est ultra-propre et ultra-rapide à compiler !
#add_unit_test(test_cycle_vie
#    test_cycle_vie.cpp
#)

add_unit_test(test_stree
    test_stress.cpp
)
