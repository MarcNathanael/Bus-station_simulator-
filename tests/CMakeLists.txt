# On modifie ta fonction pour accepter des fichiers sources supplémentaires avec ${ARGN}
function(add_unit_test TEST_NAME TEST_FILE)
    # ${ARGN} capture les fichiers additionnels si nécessaire
    add_executable(${TEST_NAME} ${TEST_FILE} ${ARGN})
    
    # ON LIE TOUT ICI : Plus besoin de compiler manuellement les .cpp !
    # 'gare_simulateur' apporte son code, 'gare_db' apporte les DALs, et 'gare_core' apporte les modèles.
    target_link_libraries(${TEST_NAME} 
        PRIVATE gare_simulateur 
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

# =========================================================
# TESTS ACTIFS
# =========================================================

# 1. Test de stress global original (5 jours de simulation en mode console)
add_unit_test(test_stress
    test_stress.cpp
)

# 2. Test Post-Livraison (Validation des fonctionnalités UI-Ready du backend)
# Vérifie le pont temporel, les injections manuelles, le live tuning et le mapping spatial X/Y
add_unit_test(commit
    commit.cpp
)

# =========================================================
# ANCIENS TESTS (Archivés)
# =========================================================
# add_unit_test(test_voiture test_voiture.cpp)
# add_unit_test(test_planificateur test_planificateur.cpp)
# add_unit_test(test_cycle_vie test_cycle_vie.cpp)