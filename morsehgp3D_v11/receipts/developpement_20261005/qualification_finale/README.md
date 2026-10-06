# Qualification G4 finale de la sortie paramétrée : S8, S9, L2b et S10

5 et 6 octobre 2026. Douze sessions gardées, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt
`TERMINATED` certifié sur cette cible exacte après chacune (`targeted_shutdown_certified`). Données : six trames
LiDAR et trois nuages `uniform_u18` hors dépôt (`data_complet`). Cadre : `exploration_v11_hors_registre /
cpu_reference / quantized_u21_input_only / not_claimed`. Aucune mesure ne promeut un statut public.

Source : `38b76701b` (chaîne finale, huit sessions), puis `98a009550` (reprise ciblée, quatre sessions), qui ajoute
seulement les réparations de qualification `be05bfad8` et `98a009550` : empreintes de route gravées par profil,
mutant `sp_masque_16379` raccordé à `mhgp11_cli_points`, découpage des portes d'échelle Release en deux lots par
profil, campagnes de mutants retirées de `release_long`, décision historique du banc désactivée, exigence LiDAR
retirée du lot court. Aucun fichier du moteur ne change entre les deux commits.

## Résultat

| Session | Commit | Configurations | Résultat |
| --- | --- | --- | --- |
| `clauderepriser1` | `98a009550` | Release u18, u21, u24, empoisonnement (lots courts) | **conformes** : 873/873, 783/783, 783/783, 784/784 |
| `clauderepriser2` | `98a009550` | portes d'échelle et LiDAR des quatre profils, deux lots chacun | **conformes** : 52/52 et 66/66 dans chacun des quatre profils |
| `clauderepriser3` | `98a009550` | campagne complète des mutants (u18) | **conforme** : 485 mutants tués sur 485 dans 13 modules, dont `head` 10/10, `api` 23/23, `cli` 28/28, `tower` 143/143 |
| `clauderepriser4` | `98a009550` | ASan+UBSan u24, quatre lots d'échelle et LiDAR | **conformes** : 11/11, 24/24, 10/10, 35/35 |
| `claudefinb` | `38b76701b` | ASan+UBSan u24 et TSan u21, portes ordinaires | **conformes** : 783/783 et 783/783 |
| `claudefins` | `38b76701b` | onze lots d'échelle et LiDAR sous sanitizers | TSan u21 **conforme** (80/80, sept lots) ; ASan : seuls échecs, les six portes de route aux empreintes u21, rejouées conformes dans `clauderepriser4` |
| `claudefinl` | `38b76701b` | `release_long` | **conforme** : 35/35 portes `long` réelles ; les six campagnes de mutants étiquetées `long`, non jouées, sont dans `clauderepriser3` |
| `claudefinp10` | `38b76701b` | différentiels S10, Python épinglé | **conformes** : `mhgp11_head_vs_python` et les trois trames entières |
| `claudefinp9` | `38b76701b` | différentiels S9, Python épinglé | **conformes** : `mhgp11_points_vs_python` et les trois trames entières |
| `claudefinmesure` | `38b76701b` | mesure FULL contre `supports` (L2b) et deux contrôles W48 | 52 appels concordants, contrôles W48 conformes |
| `claudefina2` | `38b76701b` | Release, quatre profils, configurations entières | remplacée par les reprises 1 et 2 : six échecs de route (empreintes u21 en u18 et u24), jumelles `_opt` d'échelle coupées par l'échéance |
| `claudefinm` | `38b76701b` | mutants | remplacée par la reprise 3 : témoin `api` refusé (mêmes empreintes), `sp_masque_16379` survivant |

**Périmètre.** Toutes les portes d'échelle et LiDAR sont qualifiées en Release (trois profils et empoisonnement),
sous ASan+UBSan u24 et sous TSan u21, sans jumelles `_opt` sous sanitizer (même commande native, seul le juge Python
passe sous `-O`, qualifié en Release). Clang est absent de la VM.

## Mesure FULL contre `supports` après L2b (`claudefinmesure`)

Étage `tree`, médiane de trois prises, K = 5 ; les deux bras construisent les forêts 1 à 5 au masque 16 379,
`supports` pose en plus le journal des graines et extrait l'ordre 5.

| Trame | W1 FULL | W1 supports | W48 FULL | W48 supports | rapport W48 |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 | 3,76 s | 3,78 s | 164 ms | 171 ms | 1,045 |
| ng01 | 2,80 s | 2,79 s | 139 ms | 139 ms | 0,999 |
| ng02 | 3,24 s | 3,25 s | 151 ms | 159 ms | 1,055 |

Avant L2b, la même mesure donnait 1,21, 1,18 et 1,09 (reçu `qualification_sorties`). **Le contrat de 100 ms
n'est pas tenu** : l'étage `tree` dépasse 100 ms sur les 18 prises chaudes à W48 (constat de l'auditeur). Les sorties
`points` et `plat` ne sont pas mesurées par ce banc.

**Erreurs de métadonnée déclarées, pièce non modifiée** (audit `ef91a7f46`). Le fichier `sorties_g4.json` porte
`provenance.commit = b319efc84`, étiquette recopiée de l'ancien plan : la source exécutée est `38b76701b` (reçu du
contrôleur). Il porte aussi `decision = build_order_par_defaut` : cette décision historique est sans objet depuis L2b,
les deux bras mesurant la voie FULL ; elle est désactivée dans le banc depuis `be05bfad8` (`sans_objet_post_l2b`).

## Pièces

Par session : `receipt.json` (reçu du contrôleur), `launch.json` ; pour une matrice, `matrix_summary.json` et
`result_<configuration>.json` ; pour les différentiels, les lignes de verdict CTest de chaque commande
(`stdout_*.txt`) ; pour la mesure, `sorties_g4.json`. `summary.json` résume les douze sessions. Aucune donnée ni
coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers sauf lui-même.
