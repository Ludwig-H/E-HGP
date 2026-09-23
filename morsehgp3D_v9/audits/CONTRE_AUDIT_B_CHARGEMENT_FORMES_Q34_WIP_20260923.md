# Contre-audit B — chargement des formes q3/q4 en WIP

23 septembre 2026, lecture seule du diff de
`q34_dead_lanes.{cpp,hpp}` après `684d8fc7`, publié ensuite sous
**`47f8a5da`** avec les mêmes octets (`cpp` SHA-256 `f1b23ac4…`,
`hpp` `474e9b11…`). Il préordonne les points
selon le rang spatial une fois par `Q34DeadLaneProver`, réutilise le
préfixe d'IDs de frontière et écrit les formes sans lookup d'ID dans
chaque cover. Le produit scalaire spécialisé aux deux coordonnées non
nulles de chaque vecteur de base est algébriquement identique à l'ancien
`dot`. Les formes des deux extrémités, désormais incluses, valent
identiquement zéro : elles ne fournissent aucun témoin strict. Les
comptes `form_sites` restent exprimés hors extrémités, tandis que
`site_count` formes sont physiquement **calculées** ; les tests réels
sur ces deux formes supplémentaires doivent, eux aussi, rester payés.
Ajouter `forms_computed` ou distinguer clairement ce compteur logique
dans toute ablation du chargement. Ces
lectures ne prouvent pas de gain de temps.

Deux défauts d'API/exactitude sur ce WIP :

1. `load()` peut allouer/remplir `ordered_` **avant** `loaded_=false`.
   Après un `load` réussi, un échec d'allocation de la préparation d'un
   autre index laisse donc `loaded_==true` ; `prove()` peut encore utiliser
   l'ancienne preuve. Cela contredit le contrat commenté « failed load
   leaves no usable state ». Invalider dès l'entrée, avant toute opération
   qui puisse échouer, puis ne réarmer qu'à la fin.
2. `ordered_owner_` est l'adresse nue de l'index, sans possession.
   Un nouvel index alloué à la même adresse après destruction du précédent
   ne déclenche pas la reconstruction de `ordered_`. À taille différente,
   l'accès par rang peut sortir du buffer ; à taille égale, les formes
   peuvent porter les **mauvais points** et certifier à tort une voie
   morte. Le moteur `Engine` garde son index stable pendant un appel, mais
   l'API publique du prover accepte plusieurs covers successifs sans
   ce contrat. Garder un `Q2CensusIndexPtr` propriétaire du cache, ou
   réinitialiser par une identité de génération non réutilisable.

Le coût du nouvel `ordered_` est **O(n) par worker**, pas O(n) partagé.
`sizeof(Point3)=12` octets a été confirmé sur l'ABI de build ; il ajoute
au minimum `12·n·W` octets : 28,8 Mo à 50k/W48, 576 Mo à 1 M/W48 et
17,28 Go (16,1 Gio) à 30 M/W48, avant surcapacité, formes, index,
catalogue et forêts. Ce poste
peut être raisonnable à 50k mais rédhibitoire dans le contrat des
plusieurs dizaines de millions de points ; mesurer RSS couplé et coût de
préparation, comparer à un tampon spatial immuable partagé entre workers.
`peak_edge_buffer_bytes` ne donne que le maximum d'un worker ; la somme
des maxima privés est publiée séparément mais n'est pas un pic
simultané. Après le chargement, `observe` ne recombine pas toujours
`ordered_` avec les buffers ultérieurs d'atlas/q3 ; le RSS réel est
indispensable.
`forms_.resize(site_count)` peut aussi initialiser une croissance du
vecteur avant de réécrire ses formes : qualifier le gain par ablation
réelle, sans supposer qu'une boucle sans branche est plus rapide. Le
message du commit mentionne 74→46 Gcycles de chargement sur une trame
K5/W8 avec instrumentation de brouillon et même digest : indication
locale, **pas un reçu apparié versionné** ni une mesure de toute la
tour/G4. Aucun test nouveau n'est ajouté par ce commit.

Porte minimale : deux index construits successivement, échec injecté au
début du second `load`, et réutilisation d'adresse contrôlée ; oracle de
formes/résultats, ASan/UBSan/TSan si pertinent, puis W1/W8/W48 sur les
trois trames 1 mm avec même sortie complète, temps de chargement,
`dead_form_sites`, tests, RSS et pics de workers. Ne pas relancer G4
uniquement pour ce WIP avant correction des deux points de sûreté.

## Correctif produit `aae9da0e`

Le développeur a publié une réponse structurelle aux deux défauts :
`load()` met maintenant `loaded_=false` **avant** tout accès/allocation,
et l'index immuable construit puis possède `spatial_points()` une fois.
Le prover ne garde plus l'adresse nue de l'index ni sa propre copie des
points. À la lecture, cela supprime le risque d'adresse réutilisée et
ramène le nouvel octet spatial à **12·n partagé** (360 Mo à 30 M), non
`12·n·W`. L'index `Q2CensusIndex` paie cette copie O(n) même si la
certification q3/q4 n'est pas activée ; `retained_bytes()` l'inclut.
Les deux défauts ci-dessus sont donc **historiques**, pas un verdict sur
le produit courant. La représentation exacte des formes et des IDs ne
change pas ; aucune porte ni mesure de trame n'est ajoutée par ce
commit. Refaire les gates q3/q4 et les chronos/RSS appariés au même
snapshot avant de transférer le gain de brouillon au pipeline complet.

Attention aux compteurs lors de cette ablation : le remplissage de
`spatial_points()` ajoute une passe de `n` lectures/écritures à la
construction de l'index, mais `Q2IndexWork.point_visits` ne les ajoute
pas. De même, une baisse de `edge_buffer_bytes_sum` peut seulement
signifier que les `12·n` octets ont migré des workers vers l'index.
Mesurer `gen_index`, la résidence de l'index et le RSS total, y compris
sur q2 seul où cette copie est aujourd'hui payée sans utilisation.
`spatial_points()` reste une vue de mémoire hôte : ce changement ne
constitue pas une voie GPU ni une qualification des transferts.

Contrôle indépendant sur `aae9da0e` : configuration Release dans
`build/v9-audit-current/`, reconstruction des cibles
`mhgp9_gen_wspd_q34_gate` et `mhgp9_gen_q2_census_gate`, puis leurs deux
CTests : **2/2 PASS** (11,15 s au total). Le gate q3/q4 couvre les modes
et leurs oracles existants ; il ne contient pas encore une sonde causale
de réutilisation d'adresse, de `load()` interrompu, ni la mesure du nouveau
coût de copie globale. Le contrôle local ne vaut ni reçu de trame ni gate
GPU/G4. Les deux unités modifiées compilent également en C++20 avec
`-Wall -Wextra -Wpedantic -fsyntax-only` (contrôle indépendant léger).
La cible de chaîne `mhgp9_chain_static_paths_gate` a ensuite été
reconstruite sur le même snapshot et son CTest passe **1/1** (3,42 s).

La disparition de la copie de coordonnées par worker ne retire pas
les autres états privés. `Q34DeadLaneProver` conserve `forms_` (24 octets
par site), `all_` et les frontières `levels_` à chaque profondeur ; leurs
capacités persistent d'une arête à l'autre. Au réglage usuel de profondeur
maximale 6, une borne structurelle prudente est **48·n octets par worker**
pour ces seuls tableaux, hors surallocation et autres covers (jusqu'à
environ 69 Go décimaux à 30 M points et 48 workers). C'est un plafond,
**pas un RSS observé sur LiDAR** : publier les pics co-résidents par
worker et du processus avant de conclure sur le régime massif.

### Porte propriétaire publiée `84c74a5e`

La nouvelle porte `mhgp9_gen_q34_dead_lanes_owner` compare un prouveur
réutilisé à un neuf sur deux nuages et 372 arêtes, force un `bad_alloc`
pendant un second `load()` puis vérifie que `prove()` refuse l'ancien
état. Elle couvre un couple de nuages où une preuve périmée changerait
le masque. Le mutant `dead_load_keeps_loaded` est tué par réponse
d'état incorrecte. Reconstruction Release indépendante et CTests
ciblés : **2/2 PASS** ; exécution directe affiche
`aba_same_address=1`, `stale_mask=2`, `fresh_mask=0`,
`interrupted_loads=1` sur l'allocateur local. Le produit actuel ne
garde toujours aucune adresse nue d'index dans le prouveur.

La porte **n'exige pas** `same_address=1` pour passer : si l'allocateur
ne réutilise pas cette adresse, l'essai ABA particulier devient
conditionnel (l'alternance et l'échec de chargement restent exercés).
Pour une preuve reproductible indépendante de l'allocateur, forcer la
réutilisation d'adresse via placement ou injection dédiée, ou exiger
ce plancher dans un environnement qualifié. Les deux CTests ciblés ne
mesurent pas le RSS ni le coût de la copie `12·n` de l'index.

### Travail géométrique non visible dans `dead_uniform_tests`

Dans `Q34DeadLaneProver::cell`, chaque `uniform_tests` compte une
évaluation du **maximum** de forme sur la cellule. Si ce maximum n'est
pas strictement négatif, le code évalue aussi le **minimum** de forme,
sans compteur propre. Une cellule peut créditer au plus `K−1` maxima
strictement négatifs avant de retourner ; ainsi le nombre de minima
évalués est au moins
`dead_uniform_tests − (K−1)·dead_cells`. Sur les six premières
répétitions G4 R5, ce minorant vaut **3,92–7,44 milliards** à K5 et
**9,62–20,41 milliards** à K10. Pour 08/000200/K10, les
21 728 323 453 maxima publiés impliquent **au moins 20 405 518 285
minima supplémentaires**, soit au moins 42,13 milliards
d'évaluations affines dans cette seule preuve, avant les 65,8 M tests
ponctuels. Ce sont des comptes de travail, **pas** une attribution du
temps mur q3/q4. Instrumenter `minimum_tests`, `frontier_ids_copied`
et `forms_computed` (y compris les deux extrémités) pour juger une
architecture par blocs et son coût réel.

La largeur du noyau de forme actuel a été contre-vérifiée pour u18 :
avec `M=262143`, `Q=2^20`, les coefficients vérifient
`|constant|≤12QM²`, `|x|,|y|≤8M²` et les coordonnées cellulaires
`|α|,|β|≤2Q`. Une évaluation affine et ses sommes intermédiaires sont
donc bornées par `44QM²≈3,17·10^18<2^62`, dans `i64` signé. Cette
preuve dépend de la grille u18 et des cellules actuelles ; elle ne
qualifie ni une nouvelle mise à l'échelle dyadique ni le profil
float32 exact.
