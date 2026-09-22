# Contre-audit B — première paire diamétrale pour `anchor_meb`

22 septembre 2026. Lecture du port publié à **`ad2d0ebb`** puis de sa
correction comptable à **`5ab4326c`** : `src/tower/forest/anchor_meb.hpp`,
`full_ball_tower.hpp`, gate MEB et CMake. `src/tower/core/mutants.hpp`
est inchangé. Verdict : **pas de régression géométrique trouvée**, mais
aucune qualification appariée de gain FULL ni de performance G4 du nouveau
noyau à ce stade.

## Correction locale et frontière de preuve

`anchor_meb.hpp:120–143,186–195` remplace l'essai de toutes les paires q2
par un scan des `K(K−1)/2` distances, puis l'essai de la **première** paire
maximale ; les supports q3/q4 restent dans leur ordre lexical. Une boule
diamétrale couvrant tous les sites doit réaliser le diamètre global `D` :
une paire plus courte ne peut couvrir une paire à distance `D`. Si une paire
maximale couvre tous les sites, chaque autre paire maximale est antipodale
dans cette même boule et a donc le même milieu. Garder le premier maximum
par `>` conserve le premier support q2 que l'ancien parcours aurait accepté.
L'ordre de confinement « deux extrêmes puis les autres » est une permutation
complète des sites ; rejet, coquille et clé géométrique ne changent pas.
Les points sont distincts et dans u18 avant le scan (`:99–107`) ; la distance
carrée vaut au plus `3·262143²<2^38`, donc tient dans `i64`. Le port cite la
preuve v7 u16, mais cette dernière ne qualifie ni le coût ni la tour v9.

La primitive locale est jugée contre tous les supports Gram rationnels
(`tests/tower/anchor_meb_gate.cpp:72–154`). Dans un build neuf indépendant
de `ad2d0ebb`, le nominal rend code 0 avec 697 comparaisons ; Release et
Clang 18 ASan/UBSan passent chacun les deux CTests nominal et mutant.
Les trois portes `full_ball_tower`, `full_ball_static_cpu1/4` passent en
Release ; les trois portes publiques `line12`, `shell14`, `spatial12` passent
aussi après compilation du raccord. Les portes FULL existantes recontrôlent
de petits catalogues oracle et les variantes statiques, mais le reçu LiDAR
`first_tower_20260922` reste épinglé à **d2700314**, avant ce changement.
L'usage du support dans l'échange FULL (`full_ball_tower.hpp:894–895`) impose
en outre une comparaison de tour complète, pas seulement du rayon local.

## Mutant et contrat comptable

Le mutant compile-time `>=` choisit la **dernière** paire maximale
(`anchor_meb.hpp:133–137`). Son binaire déjà construit rend code 1 avec
`cause=support.first_positive_containing` ; il est tué par le support
canonique, **pas** par une mauvaise géométrie MEB. À `ad2d0ebb`, la porte
CMake n'exigeait que le préfixe `cause=` ; **`5ab4326c` exige désormais la
cause entière**. Reste un test FULL ciblant l'échange ;
ne pas attribuer cette porte au registre historique `mutants.hpp`, qui n'a
ni diff ni activation dans ce moteur. Le nouveau compteur
`pair_distances` a désormais une porte d'overflow (`UINT64_MAX`) avec
résultat vide à `5ab4326c` ; elle manquait à `ad2d0ebb`.

Le compteur est agrégé dans la voie statique FULL
(`full_ball_tower.hpp:645–650`). `5ab4326c` renomme correctement
`kAnchorMebWorkAccounting` et publie `meb_pair_distances`,
`meb_materializations`, `meb_supports_by_size` dans la sonde v2 ; le temps
de résolution par ordre et les tests de puissance par arité restent absents.
Les anciens et nouveaux nombres de puissance ne sont pas directement
comparables sans cet étiquetage. Le macro de mutant est toutefois testé
directement par `#if defined(MHGP9_MEB_MUTANT_LAST_MAXIMUM)` dans le header,
sans `MHGP9_TESTING`, alors que CMake annonce les cibles produit sans point
d'injection : réserver explicitement ce crochet aux cibles de test.

## Décision de coût et intégration

Le scan ajoute exactement `K(K−1)/2` distances **par appel MEB** avant la
recherche de support : jusqu'à 10 à K5 ou 45 à K10, contre des essais q2
éliminés et un ordre de confinement susceptible de réduire **ou augmenter**
les tests avant rejet. Le reçu antérieur 08/000000 K10 comptait 12,004 M
appels de résolution MEB ; `45×` ce total serait une **borne supérieure**
de 540,18 M distances nouvelles, pas une mesure réalisée. Exiger un essai
apparié ancien/nouveau sur les mêmes trames sans sol 1 mm, K5 **et** K10,
mêmes W/s/compilateur, digest et catalogue exacts, temps q3/q4/FULL séparés,
travail MEB complet et RSS ; répéter et publier aussi les cas défavorables.
Aucun gain ne doit être déduit du seul nombre de supports q2 évités. La
passation annonce **130→109 s** local à K10, mais sans reçu apparié versionné,
conditions de charge ni mesure G4 sur le nouveau noyau ; c'est un signal
exploratoire, pas un gain qualifié.

Point bloquant indépendant avant une **prochaine** session G4 : le reçu G4
R1 utilise le paquet antérieur `e28296bb` et reste valide. `5ab4326c`
reconnaît le nouveau `ledger`, mais son worker exige encore que tous les
champs de `tower_work` soient entiers ; la sonde v2 ajoute une chaîne et un
tableau MEB. Le worker refuse donc `probe counters tower_work` après calcul,
alors que ses 17 selftests passent sur un faux JSON qui omet ces champs.
Voir [l'audit de schéma G4](CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md).
