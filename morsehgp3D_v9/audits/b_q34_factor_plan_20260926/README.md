# Plan q34 par facteurs — contre-audit avant raccord

26 septembre 2026. Base `92c709bc8`, moteur et défauts inchangés.
`phase=exploration_v9_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_q34_factor_plan`,
`public_status=not_claimed`. GCP non utilisé dans cette tranche.

## Question testée

Après le filtre universel de rectangle existant, peut-on préparer les
crédits des deux facteurs une fois, puis ne représenter que les produits
résiduels, sans allouer ni parcourir un tableau de toutes les paires ?
Le [diagnostic FULL précédent](../../receipts/q3_payload_local_20260926/README.md)
mesure 28,35 / 112,77 / 449,65 millions de paires q34 sur amas
8k/16k/32k. Une réduction de constante ne suffira pas à corriger ce régime.

Le prototype porte explicitement les idées de `local_credits.cpp` et
`q2_node_pool.hpp`, pas leurs qualifications. Chaque plan est attaché au
même index global. Les témoins de a sont dans A privé de a ; ceux de b
sont dans B privé de b. Les facteurs sont disjoints, les crédits s'ajoutent
donc pour une voie donnée. Les seuils restent K−1 pour q3 et K−2 pour q4.
Les crédits extérieurs du filtre universel ne sont **pas** récupérés :
h=0, aucun compte implicite ni préchargement du census futur.

La projection ne choisit que des propositions. Les huit coins exacts de
`universal_witness` décident chaque crédit ; contacts et inconnus restent
dans le résidu. Les deux voies sont regroupées conjointement afin qu'une
paire ne soit pas matérialisée deux fois. Tous les termes de préparation,
F=Σ(|A|+|B|), classes réellement occupées, descripteurs et masses résiduelles
doivent être publiés. Une sonde sans aval ne constitue pas un gain FULL.

## Résultats clos et décision

Le [reçu local](../../receipts/q34_factor_plan_20260926/README.md) ferme
42 commandes, 24 mesures, Release/ASan/UBSan, trois mutants dans chaque
build et l'oracle Fraction normal/−O. Le contrôle complémentaire de
recette et de provenance passe aussi normal/−O.

Sur les trois trames sans sol K5/s8, le plan enlève 52,7 à 61,5 % des
paires avant développement. Sur 00 : 23 686 751 → 9 122 704, mais
45,15 millions de tests de coins, 675,9 ms de préparation CPU mono locale
et 125,2 Mo de capacités cumulées de plans. Ce n'est pas un gain GPU/FULL.
Sur amas8k/16k/32k, résidu 2,09/7,79/30,70 millions : grand gain de
constante, mais dernière croissance ×3,942, problème presque quadratique
non résolu. Sur les six relations LiDAR quart→moitié→trame, pentes de
résidu 1,004–1,820 ; un compteur de coins atteint toutefois 2,230.

**Décision :** garder cette brique et ses preuves, ne pas activer le plan
CPU dans le moteur sans gain net mesuré. Les [propositions suivantes](NEXT.md)
visent une représentation compacte avant tout préfixe/allocation de
paires, et une sélection de témoins mieux répartie. Le [raisonnement](math.md)
et son oracle explicitent également les contre-exemples à une sélection
Pool supposée complète. Aucun statut moteur ni contrat n'est promu.

## Périmètre effectivement couvert

- Petits cas et mutants : comparer les décisions et identités, pas seulement
  un nombre de paires ; Release puis ASan/UBSan.
- Synthétiques 8k/16k/32k : uniforme, terrain et huit amas, sans renommer ces
  recettes u16 « grille LiDAR 1 mm ».
- LiDAR : trames entières sans sol ; les sept morceaux capteur de 08/000000
  servent au diagnostic spatial. Leur masque est figé avant les coupes,
  leur origine est commune, aucun tirage ni plafonnement d'effectif.
- K5/K10 et s8/10/12 séparés ; entrées brutes avec sol distinctes.

Les sources, commandes, sorties et hashes avant/après sont conservés.
`run.py` est le collecteur et lecteur principal ; `inputs.py` vérifie les
trames et coupes existantes sans copier leurs octets ; `validate_recipe.py`
ferme les recettes de compilation et les mappings. `check_capture.py`
capture les relectures dans un dossier neuf. Les notes suivantes restent
des propositions, pas des ports GPU qualifiés.
