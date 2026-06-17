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
    # ${ARGN} va capturer tous les fichiers .cpp qu'on ajoutera à la suite
    add_executable(${TEST_NAME} ${TEST_FILE} ${ARGN})
    
    # On lie les librairies statiques (core et db) -> il ajoute automatiquement /src/core
    target_link_libraries(${TEST_NAME} PRIVATE gare_core gare_db )
    
    # On indique où trouver les headers (.h) spécifiques au simulateur 
    #COMME CA PLUS BESOIN D"INDIQUER ,c'est une liste de path ou chercher 
    target_include_directories(${TEST_NAME} PRIVATE 
        ${CMAKE_SOURCE_DIR}/src/simulateur
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/db
    )
    
    # LIGNE CRUCIALE : On force l'exécution depuis la racine du projet 
    # pour que config.charger("data") trouve bien ton dossier de CSV !
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
add_unit_test(test_client
    test_client.cpp
    # Moteur de simulation
    ${CMAKE_SOURCE_DIR}/src/simulateur/Simulateur.cpp
    ${CMAKE_SOURCE_DIR}/src/simulateur/Planificateur.cpp
    ${CMAKE_SOURCE_DIR}/src/simulateur/Billetterie.cpp
    ${CMAKE_SOURCE_DIR}/src/simulateur/Generateur.cpp
    
    # Couche Données (DAL)
    ${CMAKE_SOURCE_DIR}/src/db/dal_voiture.cpp
    ${CMAKE_SOURCE_DIR}/src/db/dal_convoi.cpp
    ${CMAKE_SOURCE_DIR}/src/db/dal_client.cpp
    ${CMAKE_SOURCE_DIR}/src/db/dal_billet.cpp
    ${CMAKE_SOURCE_DIR}/src/db/dal_destination.cpp
    ${CMAKE_SOURCE_DIR}/src/db/dal_cooperative.cpp
    ${CMAKE_SOURCE_DIR}/src/db/dal_plage_interdite.cpp
    ${CMAKE_SOURCE_DIR}/src/db/dal_configuration.cpp
    
    # Gestionnaire de base de données
    ${CMAKE_SOURCE_DIR}/src/db/DatabaseManager.cpp
    
    # Modèles métier (nécessaires à la compilation)
    ${CMAKE_SOURCE_DIR}/src/core/Voiture.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Convoi.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Client.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Billet.cpp
    ${CMAKE_SOURCE_DIR}/src/core/PlageInterdite.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Configuration.cpp
)