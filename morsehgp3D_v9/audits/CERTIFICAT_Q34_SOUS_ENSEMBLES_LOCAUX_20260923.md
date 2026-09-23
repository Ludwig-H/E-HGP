# q3/q4 : le certificat n'exige pas un k-NN global exact

23 septembre 2026. Lecture **du WIP mutable**, avant tout reçu v17. Cette
note porte sur le choix des témoins du certificat de voies mortes, pas sur
la complétude du générateur ni sur un gain déjà mesuré.

Pour chaque cellule du domaine des centres admissibles, `prove` ferme q3
avec au moins `K−1` sites distincts strictement intérieurs et q4 avec au
moins `K−2` (`q34_dead_lanes.cpp:128–249`). Chaque forme négative au maximum
de la cellule certifie l'intérieur pour **tous** ses centres. Si la preuve
réussit avec un sous-ensemble `S` du nuage, elle réussit donc avec de vrais
sites du nuage ; l'absence d'autres sites dans `S` ne peut qu'empêcher une
fermeture. Le classement exact des k plus proches voisins globaux n'entre
pas dans cet argument. L'ordre de parcours des IDs influe seulement sur
le coût et les arrêts précoces. L'unicité, elle, est impérative : deux
occurrences du même site ne sont pas deux témoins. `load_sites` l'exige
encore par contrat plutôt que par contrôle ; la voie d'appel WIP trie et
dédoublonne ses IDs.

Cela autorise une **recherche bornée** sur l'index immuable : récupérer
quelques sites locaux pour l'arête, même sans garantir qu'ils soient ses
voisins les plus proches ; dédoublonner leurs IDs, essayer `prove`, puis
exécuter la voie exacte actuelle lorsque la preuve échoue. Un arrêt de
recherche à budget fixe ne compromet que le taux de fermeture. Une
alternative est de conserver seulement une petite liste par site obtenue
par parcours interrompu, sans terminer chaque requête k-NN ; le
contre-exemple du shadow « voisins du cœur » montre toutefois qu'une
liste globale autour des extrémités peut manquer les témoins utiles à
l'arête. Il faut comparer les deux choix, voire une sélection orientée
vers l'intérieur du disque diamétral, sans payer le cœur complet pour
fabriquer cette sélection.

La sélection par arête doit rester **conditionnelle** : B compte environ
9,59 M tentatives possibles avant le filtre ponctuel sur le brut
08/000000/K5 v12, contre 3,99 M cœurs effectivement construits ; sur
R11 sans sol, 7,16 M contre 2,04 M. Faire une nouvelle requête spatiale
pour chaque tentative déplacerait facilement le verrou. Comparer une
petite liste réutilisable par site, une récupération locale seulement
après les filtres bon marché, et un cache borné par worker ; garder le
repli exact dans chaque variante.

Le port WIP fixe `near_sites=16` et réserve `n×16` IDs `u32` plus `n`
compteurs `u8` dans `Q34NearSites::build` (`q34_near_sites.cpp:93–97`) :
**au moins 65 octets/site, soit 2,42 Gio à 40 millions de sites**, avant
index, catalogue, FULL et buffers des workers. La recherche exacte sur
boîtes peut elle-même parcourir beaucoup de nœuds et points ; sa
construction précède les jobs q3/q4, même si la durée murale de q3/q4
l'englobe. Une liste paresseuse ou bornée est donc une piste de mémoire
et de latence, non un gain acquis.

Porte utile avant capture G4 : sur les mêmes entrées, mesurer séparément
construction et mémoire des listes, requêtes, visites de nœuds **et de
points** (`near_list_point_tests` existe dans le moteur mais n'est pas
encore exporté par la chaîne), taux de fermeture, replis, formes/cover
évités, sorties et FULL identiques, CPU, mur et RSS. Faire les ablations
sur les trames entières puis sur les sept secteurs physiques à densités
1/4, 1/2 et entière. Ajouter un test direct de `load_sites` avec doublon
refusé, et le gate mixte cache q3/voisins q4 (et inverse) déjà signalé
par B avant tout reçu v17. Aucune pente sous-quadratique ne découle du
seul caractère local de la sélection.
