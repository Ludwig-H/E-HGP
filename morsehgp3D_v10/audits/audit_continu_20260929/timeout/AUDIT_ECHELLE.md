# Audit des sessions G4 4 et 5 : coût, objet et passage à l'échelle

29 septembre 2026 ; lecture et recalculs seuls, sans nouveau test G4.
Les chemins ci-dessous sont relatifs à `morsehgp3D_v10`, sauf mention v8.
Les conclusions concernent les sources lues au point de reprise `6206d1d11`.

## Éléments confirmés

- Session 4, commit `777406b82963be5ff95ba33ed153476e26dcea7a` : 21 commandes
  sur 21 closes avec code 0. Session 5, commit
  `882b13286177657c9b56287f2b1bf20b17858528` : 3 commandes sur 3 closes avec code 0.
  Le statut global `failed_remote` conserve l'échec de pip, pas un échec de ces
  calculs C++. Les deux reçus publient `observed_after.status=TERMINATED`, la même
  génération à l'ouverture et à la fermeture, et la clé retirée.
- 157 hashes de `g4_session4_j2c_20260929/SHA256SUMS` et 16 de
  `g4_session5_scale_20260929/SHA256SUMS` recalculés : aucun désaccord.
- Les 88 entrées de session 5 ont leurs tailles et hashes appariés entre
  `MANIFEST_entrees.json` et `session/receipt.json:data_files` : aucun désaccord.
  Aucun doublon retiré déclaré dans ces entrées synthétiques.
- Les 143 lignes d'exposants, soit 1 144 valeurs, ont été recalculées depuis
  `scale.csv` par le logarithme du rapport de travail divisé par le logarithme
  du rapport réel des effectifs : accord à `1e-12` près.
- CPU uniquement : les commandes utilisent les binaires C++ à 48 fils ; le GPU
  est interrogé par `vm_facts.py`, pas utilisé pour le calcul. La VM expose
  24 cœurs et 48 fils, pas 48 cœurs physiques
  (`g4_session4_j2c_20260929/results/env/lscpu.txt`).
- Les trois fichiers d'étiquettes LiDAR des sessions 2, 3 et 4 sont réellement
  identiques par SHA256, pas seulement par nombre d'amas. Les hashes sont
  `95299ba4…17e30`, `93ab506b…d5a` et `2eb6565d…e577`, pour les trames 00/01/02.

## Ne pas confondre FULL et le clustering d'un seul ordre

`mhgp10_tower --no-points` construit bien les ordres `1..K` : les tableaux
`orders` des six commandes de session 4 contiennent tous les K annoncés.
Il ne construit pas les attaches des points. Les trois dernières passes chaudes
du tableau ci-dessous sont les dernières des trois passes mesurées, pas une
médiane indépendante de répétitions complètes.

| Trame sans sol | Sites | Catalogue + FULL K5 (s) | Catalogue + FULL K10 (s) |
| --- | ---: | ---: | ---: |
| 00 | 39 885 | 0,2520 | 1,1246 |
| 01 | 35 551 | 0,2042 | 0,8614 |
| 02 | 45 845 | 0,2536 | 1,0243 |

Sources : `g4_session4_j2c_20260929/results/cmd/008..013_*/stdout`.
Ces sommes excluent `prepare_s` (6,6 à 8,3 ms), lecture et fin du processus.
La segmentation du sol et la quantification des entrées sont également hors
mesure. `cli/mhgp10_tower.cpp:69–107` donne les bornes des chronomètres.

En revanche `mhgp10_cluster` fixe `tp.only_order=kk`
(`cli/mhgp10_cluster.cpp:155–166`). Les verticales sont alors ignorées
(`src/tower/tower.hpp:143–144`). Ses 0,259 / 0,218 / 0,263 s sont la chaîne
catalogue + ordre K5 + tête, **pas** la tour FULL 1..5 suivie de la tête.
Son champ `catalogue_s` inclut préparation et index contrairement au champ
homonyme de la sonde FULL (`cli/mhgp10_cluster.cpp:106–130,239–241`).
Les murs complets de ces trois commandes sont 0,273 / 0,231 / 0,278 s.

Pour un budget FULL 100 ms, ne pas reprendre les 47 à 61 ms de la tour de
`mhgp10_cluster` : FULL seul prend ici 67,3 à 89,3 ms. Le gain restant ne peut
pas être calculé sur l'objet plus petit. K5 est sous une seconde sur ces trois
entrées CPU ; K10 ne l'est que sur la trame 01. Ni 100 ms ni un calcul GPU
ne sont établis par ces reçus.

Les trames 00/01/02 sont `08/000000`, `08/000100` et `08/000200`, donc une seule
séquence SemanticKITTI, sans sol. Source : manifeste v8
`receipts/lidar_ground_20260921/release/ground_fq64xq_6/MANIFEST.json:5–7`.
Ce ne sont ni plusieurs séquences ni le régime avec sol.

## Tailles et mesures réellement closes

`g4_session5_scale_20260929/scale.csv` contient 176 couples entrée/K :
169 `ok`, 7 `skipped_budget`, aucun refus ni timeout. Les 7 non-lancés sont,
à densité ×128 : filaments K10 et shells/terrain/uniform K5 et K10.
À 1 024 000 sites, seuls clusters K5/K10 et filaments K5 ont donc été mesurés.

Le manifeste contient 67 synthétiques et 21 secteurs LiDAR. En plus des 7
couples sautés pour budget, 13 nuages spatiaux ont été exclus **avant**
exécution pour dépassement u18 : uniform/clusters/filaments ×64 et ×128,
shells ×32/64/128, terrain ×16/32/64/128. La formule « jusqu'à ×32 »
ne vaut pas pour toutes les familles spatiales.

Le générateur sépare utilement densité et expansion spatiale, avec un pas fixe
par famille/régime (`bench/scaling/scale_inputs.py:79–135`). Mais il n'y a qu'une
graine de base 3 et un tirage par couple famille/régime/taille ; le facteur
entre dans la graine. Ce ne sont pas des préfixes appariés d'un même tirage.
Une seule exécution complète est mesurée par couple (`scale_run.py:60–85`).
Les 3 passes de session 4 ne remplacent pas des répétitions à chaque grande taille.

## Croissance favorable, mais affirmations trop fortes

Les compteurs soutiennent une croissance empirique sous-quadratique sur ces
entrées. Ils ne donnent ni une borne globale ni un coût strictement constant
par boule. Intervalles des exposants adjacents recalculés :

| Régime | K | Boules | Tests jugés | Pas de tour | Temps de tour |
| --- | ---: | --- | --- | --- | --- |
| spatial | 5 | 0,998–1,047 | 0,988–1,048 | 0,999–1,049 | 0,857–1,201 |
| spatial | 10 | 1,000–1,062 | 1,000–1,073 | 0,999–1,064 | 0,989–1,273 |
| densité | 5 | 1,013–1,294 | 1,013–1,333 | 1,012–1,347 | 0,802–1,528 |
| densité | 10 | 1,019–1,409 | 1,013–1,470 | 1,017–1,483 | 1,047–1,862 |

Exemple terrain K10, 256k → 512k : exposant boules 1,400338, catalogue
1,402461, tour 1,861660. Le résultat reste sous le carré mesuré, mais la
description du temps de tour limité à « 1,1 à 1,3 » ne couvre pas ce régime.

Exemple clusters K10, 8k → 1 024k : tour de 85,52 à 173,51 ns/boule,
soit ×2,03 ; catalogue + tour de 178,73 à 259,91 ns/boule. Uniform K10,
8k → 512k : tour de 88,61 à 178,43 ns/boule. Les formulations « le temps est
linéaire en nombre de boules » et « le coût par boule reste constant »
(`g4_session5_scale_20260929/README.md:46,61`) dépassent donc les observations.

Le maximum d'environ 460 boules/site est un ordre de grandeur empirique,
pas une borne démontrée. Clusters atteint déjà 467,268 à ×128 ; les séries
shells et terrain sont toujours croissantes à leur dernière taille (271,697
et 145,505 à ×64). Une explication par l'épaisseur résolue est plausible,
mais n'établit pas une limite universelle à 460 ni à 120 sur les surfaces.
Ne pas remplacer ce constat par une recherche d'un pire cas hors régime :
la prochaine preuve utile est une répétition dans les régimes visés.

## Mémoire : unités et extrapolation à corriger

Pour clusters densité ×128/K10 (`scale.csv:169`) :

- 478 482 791 boules ; catalogue 41,3419 s ; tour 83,0214 s ; mur 126,82 s ;
- RSS 141 855 788 **KiB**, soit 145,260 **Go** ou 135,284 **Gio** ;
- donc 303,585 **octets/boule**, pas 280.

Le tableau historique affiche des Gio sous un en-tête Go. Aux dernières
tailles K10 des cinq familles, la RSS par boule est de 303,6 à 314,9 octets ;
la valeur 280 ne vient pas de la conversion correcte de ces données.
La tour coûte environ deux fois le catalogue pour clusters/uniform denses,
mais seulement 1,11 fois pour terrain ×64 : ce n'est pas une constante générale.

Le seuil « environ 1,4 million de sites LiDAR » ne découle pas de ces nombres.
La règle de trois 188/135 millions est celle de la densité **3D**, pas celle
du LiDAR à 120 boules/site. Avec 188 Go et 304 octets/boule, 120 boules/site
donneraient environ 5,15 millions de sites dans une extrapolation naïve.
Ce n'est pas une capacité qualifiée : la RSS dépend des sorties, des buffers,
de la concurrence et de la géométrie. Il faut corriger la confusion, sans
promettre que 5 millions passent. Les dizaines de millions restent ouvertes.

## Conservation des preuves et suite conseillée

Les données de session 5 justifient une poursuite : compteurs et temps observés
sont favorables, et les grandes sorties ont été réellement matérialisées en
mémoire. Le CSV ne conserve cependant pas chaque JSON natif ni le catalogue
exporté ; il agrège deux appels distincts, catalogue puis catalogue+tour.
Le compteur `balls` vient du premier appel, le temps/RSS du second, sans
assertion locale d'égalité des deux compteurs (`scale_run.py:60–85`).
La porte distante fast ne comporte que unit + refus des multiplicités,
pas le grand oracle catalogue/tour.

Pour la prochaine campagne : arrêt et récolte des groupes, JSON natifs par
appel et liaison des compteurs, répétitions/plusieurs graines, plusieurs
séquences LiDAR et masque figé, puis mesure explicitement séparée de FULL,
des attaches et de la tête. Le [défaut de délai](README.md) est prospectif :
aucun timeout de la session 5 n'a été observé.
