# Référence EMST exhaustive indépendante

Ce dossier appartient uniquement aux tests de la phase 1. `exhaustive_emst.py` n'importe aucun module de `reference/morsehgp3d_oracle` et aucun code de production : il normalise séparément les coordonnées, construit les $n(n-1)/2$ arêtes du graphe euclidien complet en `Fraction`, puis applique Kruskal par lots de poids exactement égaux.

La routine `exact_affine_dimension` effectue aussi sa propre élimination gaussienne rationnelle. La campagne l'utilise pour vérifier les classes affines 1D, 2D et 3D sans réutiliser le calcul de rang du générateur ou de l'oracle comparé.

Le niveau HGP d'une arête de longueur carrée $d^2$ est exactement $d^2/4$. Avant chaque lot, les composantes sont figées; toutes les arêtes du lot déterminent les multifusions sémantiques, tandis qu'un sous-ensemble lexicographique de $q-1$ arêtes est retenu pour l'EMST. Les coupes du graphe complet et de cet arbre peuvent être rejouées séparément aux seuils ouverts et fermés.

## Compteurs

`EMSTCounters` expose les quantités suivantes :

- `point_count` : nombre de sites distincts;
- `distance_evaluations` : distances carrées exactes calculées;
- `complete_edge_count` : arêtes du graphe complet, égal à $n(n-1)/2$;
- `distinct_edge_weight_count` : poids d'arêtes exacts distincts;
- `equal_weight_batch_count` : lots contenant au moins deux arêtes;
- `max_equal_weight_batch_size` : plus grand nombre d'arêtes dans un lot;
- `selected_edge_count` : arêtes retenues par Kruskal;
- `redundant_edge_count` : arêtes complètes non retenues;
- `merge_batch_count` : lots qui diminuent le nombre de composantes;
- `merge_event_count` : contractions connexes distinctes sur les composantes pré-lot;
- `multifusion_count` : événements d'arité au moins trois;
- `max_merge_arity` : plus grande arité pré-lot;
- `replay_level_count` : niveau de naissance zéro plus tous les niveaux d'arêtes distincts nécessaires pour rejouer le graphe complet; certains de ces niveaux peuvent être topologiquement redondants.

Ces compteurs décrivent le travail de la référence EMST. Ils ne ferment ni G2, ni la phase 1, et ne constituent pas une mesure de performance du backend cible.
