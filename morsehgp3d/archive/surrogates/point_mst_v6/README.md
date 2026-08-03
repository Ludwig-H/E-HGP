# Surrogate point-MST v6

Ce répertoire conserve l'instantané `point_mst_mutual_reachability_surrogate.v6`
et son exécutable de campagne industrielle. Cette implémentation construit des
arbres couvrants sur les points. Pour les ordres supérieurs à un, ces arbres ne
constituent pas la hiérarchie de Hartigan exacte portée par les simplexes et les
facettes de MorseHGP3D. Elle reste donc un artefact de performance et de
falsification, jamais un chemin produit ni un repli de l'implémentation exacte.

Les sources sont volontairement exclues du build, de l'installation et de
l'API publics. Les infrastructures exactes partagées, notamment le LBVH Morton,
restent actives à leur emplacement normal. Les certificats, validateurs,
rapports historiques et fixtures falsificatrices demeurent eux aussi en place
afin de préserver la traçabilité des décisions.

Contenu archivé :

- `include/morsehgp3d/gpu/binary64_lbvh_top_k.hpp` ;
- `src/cuda/phase14_binary64_lbvh_top_k_internal.hpp` ;
- `src/cuda/phase14_binary64_lbvh_top_k.cu` ;
- `src/gpu/binary64_lbvh_top_k.cpp` ;
- `src/tools/gpu_guarded_industrial_e2e.cpp`.
