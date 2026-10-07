# Sous-dossier du projet factice : un add_test direct, que le registre des portes doit refuser bien qu'il ne soit pas
# dans le dossier racine (faute "subdirectory" de ../CMakeLists.txt). La porte rendrait 1 et serait dite passee.
add_test(NAME mhgp12_fixture_enfant COMMAND ${CMAKE_COMMAND} -E echo vert)
set_tests_properties(mhgp12_fixture_enfant PROPERTIES PASS_REGULAR_EXPRESSION "vert")
