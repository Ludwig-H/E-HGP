# Précisions de la relecture performance — 4 octobre 2026

Supplément indépendant de `../deep_performance_20261004/`, dont aucun octet n'est modifié. Les formulations ci-dessous précisent sa lecture ; aucune expérience, build, natif ou GCP ajouté.

**q3 : « réduit le Level » désignait la formule géométrique de degré réduit, pas une réduction par PGCD.** À e02 `src/num/sphere.cpp:46–49`, le niveau est construit immédiatement par

`num = |u|² |v|² |v−u|²`, `den = 4 |u×v|²`.

Le numérateur est de degré 6, le dénominateur de degré 4. `src/num/level.hpp:1` spécifie « non réduit » ; `Level::make:16–23` valide signe/capacité et conserve ces entiers sans normalisation. Le constat de travail inutile avant certains rejets de propriétaire demeure ; aucune fraction de temps n'en est déduite.

**Le seuil de subdivision des centres change réellement entre moteurs.**

| Source épinglée | Marqueur et conséquence |
| --- | --- |
| v11 e02, `catalogue/boxes.cpp:143–149` | Plus grand côté calculé ; `width <= 1` interdit de diviser, dans les coordonnées entières originales. La feuille peut aussi être arrêtée plus tôt par `leaf_size`. |
| v10 `777406b82`, `catalogue/generator.cpp:22`, `:648`, `:597–608` | `kT=6`, sites `X=P<<6`, découpe tant que `side>1` : le minimum possible vaut 1/64 de maille originale. Taille `C.M` et stagnation sous la maille (limite9) peuvent arrêter avant ce seuil. |

Il s'agit de subdivisions de l'espace des **centres**, sans changement des coordonnées physiques des sites. À grille 1 mm, 1/64 de maille vaut 15,625 µm pour ces boîtes ; ce n'est pas une nouvelle précision des données.

La voie v11 arrêtée à width1 passe à l'énumération de la liste conservée (`boxes.cpp:161–168`) ; une feuille au-delà de `max_leaf` retourne `wide_leaf`. Le seuil grossier n'implique donc pas, à lui seul, omission de candidats : il change les listes, préfixes, charge et éventuels refus, pas un droit de tronquer. Les prédicats et propriétaires restent à certifier pour toute partition choisie.

Copier seulement kT6 n'est pas un port numérique sûr : le v10 `generator.cpp:32–33` exige `2*(B+T)+5 <= 63` pour le filtre i64. Avec B24/T6, cela ferait 65 ; ce test ne passe plus. Reprendre les bornes/types de dominance, centres et propriétaire avant toute qualification u24. Aucun coût mesuré ni gain de T6 n'est revendiqué ici.

Cinq sources exactes, recoupées avant/après. Ce reçu conserve uniquement leur lecture et leur hachage ; pas de nouvelles portes. Le ledger inventorie tous les payloads ; `SHA256SUMS` inclut le ledger et exclut seulement sa propre racine.
