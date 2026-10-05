# Executable mhgp11 (docs/ARCHITECTURE.md, paragraphe 2 ; specification de la sortie parametree, paragraphe 5) : cible
# mhgp11_cli, fichier mhgp11 (la bibliotheque statique s'appelle deja mhgp11). cli/mhgp11.cpp n'inclut que
# "api/api.hpp" ; ses portes sont dans tests/cli/tests.cmake.
add_executable(mhgp11_cli ${CMAKE_CURRENT_LIST_DIR}/mhgp11.cpp)
set_target_properties(mhgp11_cli PROPERTIES OUTPUT_NAME mhgp11)
target_link_libraries(mhgp11_cli PRIVATE mhgp11)
