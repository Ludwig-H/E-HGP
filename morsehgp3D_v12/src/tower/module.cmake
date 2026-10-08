# Module tower (tranche T2) : etage G (resolution) de la voie CPU de reference ; cellules de fenetre, table de
# populations, resolution des representants. Les etages T, M, V et R ajoutent leurs sources ici.
mhgp12_module_sources(supports.cpp cells.cpp cells_stage.cpp populations.cpp radix.cpp first_probes.cpp resolve.cpp
                      passes.cpp stage.cpp)
# Etages T, M, V, R et export FUL1.
mhgp12_module_sources(forest_births.cpp forest_kernel.cpp forest_contract.cpp forest_build.cpp forest_stages.cpp
                      forest_validate.cpp forest_sources.cpp vertical_images.cpp registry_branches.cpp export_full.cpp)
