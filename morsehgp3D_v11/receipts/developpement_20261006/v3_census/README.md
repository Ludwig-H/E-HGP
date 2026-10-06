# Census des descentes : bornes entières et arbre radix de Morton (levier V3)

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261006.claudev3ab`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudev3ab/receipt.json`). Source exécutée : `b4665642b`, dont le code est celui de `a841d8c4b` (V3) et
`12ce8f8f0` (placement adopté). Aucune mesure ne promeut un statut public.

## Objet

Le levier V3 de l'audit des transpositions (contrat de la revue indépendante 10) a deux pièces :
1. **bornes sur sites entiers** (`num::LatticeSphere`, `src/num/lattice_bounds.hpp`). Le minorant est la puissance au
   point entier le plus proche du centre, ramené dans la boîte : c'est le minimum exact sur les points entiers de la
   boîte. Le majorant est la puissance au coin éloigné : le maximum exact sur la boîte continue. La voie Wide (q3 non
   certifié en u21 et u24) garde les bornes continues ;
2. **arbre radix de Morton** (`src/index/build.cpp`) : chaque plage est coupée au plus haut bit de clé qui diffère
   entre ses extrémités. Le nombre de nœuds est compté avant toute allocation, et la profondeur est au plus 3B+1.

Les sites restent rencontrés dans l'ordre de Morton : listes intérieures et coquilles, témoins saturés, descentes et
sorties FULL sont inchangés.

## Exactitude

- Onze mutants ciblés, tous tués par le code de sortie de leur porte (`mut_*.json`, `mut_*.txt`) :
  - num : plancher au lieu du plus proche, coin proche, seuil au plancher ;
  - index : retour à la borne continue dans chacun des deux census, compte gonflé, coupe médiane, axe décalé ;
  - tower et api : les trois mutants du placement.
- Trois bancs `conforme`. Toutes les prises de toutes les variantes rendent les mêmes vidages, égaux aux empreintes des
  trames : ng00 `3a2bfb4f9f48`, ng01 `5212a2ced81b`, ng02 `78feb765e21c` à K5 ; `61a4245b91d9`, `838a447e0b92`,
  `81f89995eacc` à K10.
- En local, avant la session : portes num et index aux profils u18, u21 et u24 ; suite `fast` u21 sans les jumelles
  `_opt` ni les identités à 16 000 et 32 000 sites, 722 portes conformes sur 726. Les quatre autres :
  - `tower_bench_collector` et `tower_bench_io`, cassées par l'import retiré de `tree_shape`, corrigées puis
    conformes ;
  - `tower_full_campaign`, trop lente sous charge, conforme seule en 77,5 s ;
  - `full_paired_protocol`, dont le reçu manque au checkout partiel du codespace.

  La matrice complète de qualification reste à jouer (session suivante).

## Mesure (règles écrites dans `claudev3ab/plan.json` avant la session)

Variantes construites par `bench/gpu_ab.py --variants` (sommes dans `archives_variantes.sha256`) :
- `new` : la source ;
- `lat` : la source avec `src/index/build.cpp` de `9d10de213`, soit la borne entière seule ;
- `base` : `9d10de213`.

Statistique : `forest_ms` à froid, médiane de 6 processus neufs par trame et par variante, en carré de Williams.

| Bras | Trame | base | lat | new | new ÷ base | new ÷ lat | lat ÷ base |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| K5 CPU, feuilles 16 (278523) | ng00 | 146,0 | 140,1 | 132,3 | 0,906 | 0,944 | 0,959 |
| | ng01 | 117,1 | 111,5 | 107,9 | 0,922 | 0,967 | 0,953 |
| | ng02 | 141,8 | 138,6 | 133,5 | 0,941 | 0,963 | 0,977 |
| K5 GPU+C, feuilles 24 (344059) | ng00 | 148,6 | 140,7 | 134,3 | 0,904 | 0,954 | 0,947 |
| | ng01 | 111,6 | 110,7 | 105,9 | 0,949 | 0,957 | 0,991 |
| | ng02 | 144,5 | 139,7 | 134,3 | 0,930 | 0,962 | 0,967 |

- **R1** (V3 gardé si la moyenne géométrique new/base est ≤ 0,95 et aucun rapport > 1,02) : **0,925**, pire 0,949. Gardé.
- **R2** (arbre radix gardé si la moyenne géométrique new/lat est ≤ 0,98 et aucun rapport > 1,03) : **0,958**, pire 0,967.
  Gardé.
- R3 était sans objet ; la borne seule donne lat/base 0,966.

**K10, GPU+C, feuilles 24 (descriptif, 3 processus par variante).** Forêts à froid :
- ng00 : 1 650 → 1 412 ms (×0,855) ;
- ng01 : 1 184 → 1 009 ms (×0,852) ;
- ng02 : 1 305 → 1 157 ms (×0,887).

Murs chauds de `new` : 2,00, 1,51 et 1,73 s.

À K5 chaud, `new` a un mur de 261 à 339 ms et un étage `domain` de 156 à 209 ms. **Le contrat de 100 ms n'est pas
tenu, ni le jalon de 200 ms.**

## Compteurs déterministes (`compteurs_locaux.json`)

Tests de points du census, K = 5, tous les autres registres de travail identiques :

| Trame | Origine | Borne entière | Borne entière + radix |
| --- | ---: | ---: | ---: |
| ng00 | 21,15 M | 10,48 M (×0,496) | 5,60 M (×0,265) |
| ng01 | 15,52 M | 7,99 M (×0,515) | 4,00 M (×0,258) |
| ng02 | 14,76 M | 7,58 M (×0,513) | 3,81 M (×0,258) |

Profil d'instructions de la résolution régulière (callgrind, ng00, un fil) : 30,71 G → 17,34 G (×0,565). Le census
passe de 20,64 G à 7,27 G, les tests de boîte de 44,6 M à 14,5 M (×0,33, contre ×0,35–0,38 prévus) et les tests de
sites du census de 21,15 M à 5,60 M.

## Lecture et suite

Le temps gagné (×0,925 sur l'étage) est bien moindre que les instructions retirées de la résolution (×0,565). Deux
raisons y contribuent :
- l'étage contient aussi les contextes, les naissances, la publication et les verticales ;
- la résolution elle-même n'est pas proportionnelle à ses instructions.

La chronologie du pipeline (fin des résolveurs, fin du publieur de l'ordre 5) sera relevée sur G4 avant de choisir
entre le publieur (O2) et le coût par pas restant : MEB de la partie 25 % des instructions, bornes de boîte 29 %. Deux
leviers de constante sont prêts : registres sans branche, et coefficients de la sphère copiés une fois par parcours.
Ils retirent encore 9,4 % des instructions de la résolution, avec un vidage identique, et seront mesurés à la session
suivante.

## Pièces

`claudev3ab/` : `plan.json`, `launch.json`, `receipt.json`, rapports `gpu_ab_report_*.json` (trois bras), rapports
de mutants, `archives_variantes.sha256`. `compteurs_locaux.json` : compteurs et profil locaux. Aucune donnée ni
coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers sauf lui-même.
