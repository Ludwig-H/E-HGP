# Portes du module points (tranche S9 de la sortie parametree : hierarchie de points H^r_{K+1}).
add_executable(mhgp11_points_probe ${CMAKE_CURRENT_LIST_DIR}/points_probe.cpp)
target_link_libraries(mhgp11_points_probe PRIVATE mhgp11)
target_include_directories(mhgp11_points_probe PRIVATE ${PROJECT_SOURCE_DIR}/bench)
# Exportateur MHGP11PH de reference (bench/points_export.cpp) : celui des portes de la tour s'il est declare, sinon la
# meme source sous un nom propre (construction -DMHGP11_MODULES=points des mutants), memes octets.
if(TARGET mhgp11_points_export)
  set(mhgp11_points_reference $<TARGET_FILE:mhgp11_points_export>)
else()
  add_executable(mhgp11_points_export_reference ${PROJECT_SOURCE_DIR}/bench/points_export.cpp)
  target_link_libraries(mhgp11_points_export_reference PRIVATE mhgp11)
  target_include_directories(mhgp11_points_export_reference PRIVATE ${PROJECT_SOURCE_DIR}/bench)
  set(mhgp11_points_reference $<TARGET_FILE:mhgp11_points_export_reference>)
endif()
