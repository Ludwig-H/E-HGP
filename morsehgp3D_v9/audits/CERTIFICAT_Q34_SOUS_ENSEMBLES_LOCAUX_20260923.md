# q3/q4 : le certificat n'exige pas un k-NN global exact

23 septembre 2026. Preuve formulée pendant l'essai des voisins proches.
Ce [port a été retiré après mesure négative](../receipts/near_sites_negative_20260923/README.md) ;
le lemme ci-dessous reste valable pour toute future sélection de vrais sites
distincts. Il ne démontre ni la complétude du générateur ni un gain de coût.

Pour chaque cellule du domaine des centres admissibles, `prove` ferme q3
avec au moins `K−1` sites distincts strictement intérieurs et q4 avec au
moins `K−2` (`q34_dead_lanes.cpp:128–249`). Chaque forme négative au maximum
de la cellule certifie l'intérieur pour **tous** ses centres. Si la preuve
réussit avec un sous-ensemble `S` du nuage, elle réussit donc avec de vrais
sites du nuage ; l'absence d'autres sites dans `S` ne peut qu'empêcher une
fermeture. Le classement exact des k plus proches voisins globaux n'entre
pas dans cet argument. L'ordre de parcours des IDs influe seulement sur
le coût et les arrêts précoces. L'unicité, elle, est impérative : deux
occurrences du même site ne sont pas deux témoins. Dans le port retiré,
`load_sites` l'exigeait par contrat ; sa voie d'appel triait et
dédoublonnait les IDs. Toute nouvelle API doit vérifier cette précondition.

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

La variante retirée fixait `near_sites=16` : sa table de `n×16` IDs `u32`
et `n` compteurs `u8` réservait **au moins 65 octets/site**, soit
2,42 Gio à 40 millions de sites, avant index, catalogue, FULL et buffers
des workers. Elle construisait les listes exactes avant les jobs q3/q4 ;
le [reçu négatif](../receipts/near_sites_negative_20260923/README.md)
mesure seulement −2 % de CPU q3/q4 à K5 et une régression de +3,7 % à
K10. Une liste paresseuse ou bornée reste une hypothèse, pas un gain acquis.

Pour une future variante, mesurer séparément préparation, nœuds **et
points** visités, fermetures, replis, formes et covers évités, sorties
et FULL identiques, CPU, mur et RSS. Comparer les trames entières puis
les sept secteurs physiques aux densités 1/4, 1/2 et entière. Une API
publique qui accepte une liste de témoins doit refuser les IDs répétés ;
les voies mixtes cache q3/proof q4 et inverse demandent une porte causale.
Aucune pente sous-quadratique ne découle du seul caractère local de la
sélection.
