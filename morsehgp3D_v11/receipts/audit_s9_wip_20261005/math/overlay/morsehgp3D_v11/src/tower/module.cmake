# MEB, domaine ferme, cellules, descentes et forets FULL avec verticales ; arbre d'ordre K seul et rattachement de
# W_K (build_order) ; attaches de points distinctes ; index d'ancetres par pointeurs de saut (hierarchie de points, S9).
mhgp11_module_sources(meb.cpp full_domain.cpp cells.cpp cells_classify.cpp locate.cpp canonical.cpp descent.cpp descent_memo.cpp
                     population_lookup.cpp
                     forest_build.cpp forest_plateau.cpp forest_vertical.cpp forest_parallel.cpp forest_vertical_parallel.cpp
                     forest_concurrent.cpp forest_pipeline.cpp
                     order_tree.cpp attachment.cpp ancestor_index.cpp)
