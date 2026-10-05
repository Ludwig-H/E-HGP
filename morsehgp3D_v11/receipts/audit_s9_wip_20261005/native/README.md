# S9 WIP : refus du tri et revue native

Capture du 5 octobre 2026 à16:03:14 UTC, base `53c027fe848b0d890f164eb87ebf347338c58d55`, développeur `build/v11-impl-l3`. Onze sources capturées puis vérifiées inchangées à la fermeture. WIP non qualifié. Aucun build/test natif ni GCP.

**P1 : propager le refus immédiatement pendant le tri des dates.** `point_tree.cpp:80–90` change de comparaison après un refus. Le modèle borné du premier partitionnement GCC11.4 montre que sa sentinelle peut alors manquer :17 éléments, pivot SiteIdx16, refus à la4e comparaison, lecture tentée à l’indice17. Les témoins sans refus et avec refus immédiatement propagé restent dans les bornes.

La capsule contient un blueprint rationnel de17 dates dans(1,2), toutes au même plancher strict, avec d1=1.1,d16=1.5,d8=1.9. Il réalise les valeurs attendues du groupe ; aucun catalogue géométrique ou nuage naturel causant le refus n’est prétendu. Le refus est injecté, aucun crash natif exécuté.

Rejeu stdlib : `python3 -S -B sort_refusal_model.py` et `python3 -O -S -B sort_refusal_model.py`. Les sorties sont identiques ; commandes et SHA dans `sort_replay.json`. `review.json` sépare faits, modèle, interprétation et limites. `source_manifest.json`, `sources/` et `closure.json` épinglent la lecture et les dépendances.

Solution : tri qui rend et propage `Outcome` avant toute comparaison suivante, ou exception de contrôle interceptée à l’intérieur de `entry_order` avant de quitter son `noexcept`. La relation d’ordre doit rester constante. Les autres chemins de capacités, mémoire, parentés et concurrence relus n’ont pas livré de nouveau défaut important.

Source du partitionnement : [GCC11.4 stl_algo.h](https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-11.4.0/libstdc%2B%2B-v3/include/bits/stl_algo.h). Précondition : [C++20 N4861 alg.sorting](https://timsong-cpp.github.io/cppwp/n4861/alg.sorting#3).
