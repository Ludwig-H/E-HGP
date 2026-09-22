# Contre-audit B — première paire diamétrale pour `anchor_meb`

22 septembre 2026. Lecture seule des diffs **non commis** du worktree
développeur sur `e28296bb` : `src/tower/forest/anchor_meb.hpp`,
`full_ball_tower.hpp`, gate MEB et CMake. `src/tower/core/mutants.hpp`
est inchangé. Verdict : **pas de régression géométrique trouvée**, mais
aucune qualification de gain FULL ni de performance G4 à ce stade.

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
(`tests/tower/anchor_meb_gate.cpp:72–154`) ; le binaire nominal déjà présent
rend code 0 avec 697 comparaisons. Les portes FULL existantes recontrôlent
de petits catalogues oracle et les variantes statiques, mais le reçu LiDAR
`first_tower_20260922` reste épinglé à **d2700314**, avant ce changement.
L'usage du support dans l'échange FULL (`full_ball_tower.hpp:894–895`) impose
en outre une comparaison de tour complète, pas seulement du rayon local.

## Mutant et contrat comptable

Le mutant compile-time `>=` choisit la **dernière** paire maximale
(`anchor_meb.hpp:133–137`). Son binaire déjà construit rend code 1 avec
`cause=support.first_positive_containing` ; il est tué par le support
canonique, **pas** par une mauvaise géométrie MEB. La porte CMake
`mhgp9_tower_anchor_meb_mutant_last_maximum` exige seulement le préfixe
`cause=` (`CMakeLists.txt:141–147`) : une autre exception contrôlée de code 1
serait acceptée. Exiger la cause entière et un test FULL ciblant l'échange ;
ne pas attribuer cette porte au registre historique `mutants.hpp`, qui n'a
ni diff ni activation dans ce moteur. Le nouveau compteur
`pair_distances` mérite aussi sa porte d'overflow (`UINT64_MAX`) avec
résultat vide et travail payé ; les refus actuels ne la couvrent pas
(`anchor_meb_gate.cpp:228–253`).

`kAnchorMebWorkAccounting` annonce toujours
`anchor_meb_real_lexicographic_supports_and_powers_v1` (`anchor_meb.hpp:18–20`),
alors que la recherche de supports, l'ordre des puissances et le ledger ont
changé. Le compteur est agrégé dans la voie statique FULL
(`full_ball_tower.hpp:645–650`), mais le JSON du probe n'émet encore que
`meb_calls` et `meb_power_tests` (`bench/tower_probe.cpp:181–187`). Versionner
le contrat et publier, par K, appels, supports essayés par taille,
`pair_distances`, `power_tests` et temps de résolution ; les anciens et
nouveaux nombres de puissance ne sont pas directement comparables sans cet
étiquetage.

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
Aucun gain ne doit être déduit du seul nombre de supports q2 évités.

Point bloquant indépendant avant toute session G4 : le diff récent du probe
ajoute un champ top-level `ledger` (`bench/tower_probe.cpp:169–179`), tandis
que `gcp-migration/tower_worker_v9.py:93–94,291` exige toujours les anciennes
clés JSON exactes. Un probe neuf et ce worker refuseraient **chaque** cas
après l'avoir exécuté et facturé (`probe JSON fields`). Épingler ensemble
probe, worker, selftest et schéma du reçu avant la campagne ; aucun G4 n'a
été lancé par cet audit.
