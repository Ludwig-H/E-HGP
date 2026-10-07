# Table de vérité des temps G4 de la v10 (CPU seul), extraite des sorties brutes des reçus

Produite par `table_verite.py` à partir de `receipts/g4_session{1..4}_*/results/cmd/*/stdout` ; millisecondes ; VM `g4-standard-48` (AMD EPYC 9B45, 24 cœurs, 48 fils), g++ 11.4, `-O3 -DNDEBUG` sans `-march`. Aucun calcul sur GPU.

## 1. Tour FULL 1..K sans attaches (`mhgp10_tower --no-points --repeat=3`, dernière passe), session 4 (`777406b82`), 48 fils

| Trame | Sites | K | Boules | Préparation (froide, hors passes) | Catalogue | dont frontière | dont boîtes | dont ordre | dont assemblage | dont hors étages | Tour | dont index (`t_prepare`) | dont atlas (`t_local`) | dont semis | dont descentes (`t_resolve`) | dont Kruskal | dont verticales | Catalogue + tour | Trois passes catalogue + tour |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 00 | 39885 | 5 | 1306696 | 7,2 | 163,5 | 21,8 | 106,7 | 11,2 | 16,9 | 6,9 | 88,5 | 2,8 | 4,6 | 3,0 | 28,6 | 31,7 | 17,6 | **252,0** | 259,8 / 256,2 / 252,0 |
| 00 | 39885 | 10 | 5512670 | 7,2 | 652,6 | 37,3 | 471,0 | 48,5 | 75,1 | 20,7 | 472,0 | 11,1 | 29,9 | 21,1 | 242,9 | 90,4 | 64,8 | **1124,6** | 1126,4 / 1111,2 / 1124,6 |
| 01 | 35551 | 5 | 1095926 | 6,7 | 136,9 | 23,2 | 84,8 | 9,2 | 14,8 | 4,9 | 67,3 | 2,3 | 4,4 | 2,6 | 22,2 | 20,9 | 14,8 | **204,2** | 218,6 / 214,3 / 204,2 |
| 01 | 35551 | 10 | 4383302 | 6,6 | 527,5 | 42,0 | 368,2 | 38,2 | 61,4 | 17,7 | 333,9 | 8,3 | 23,4 | 16,3 | 179,3 | 52,4 | 47,5 | **861,4** | 882,9 / 869,8 / 861,4 |
| 02 | 45845 | 5 | 1407885 | 8,1 | 164,3 | 23,0 | 101,5 | 12,6 | 19,5 | 7,7 | 89,3 | 3,3 | 5,0 | 3,2 | 27,1 | 27,5 | 21,5 | **253,6** | 265,5 / 260,3 / 253,6 |
| 02 | 45845 | 10 | 5483320 | 8,3 | 618,1 | 38,3 | 435,0 | 47,3 | 74,7 | 22,8 | 406,2 | 11,0 | 29,7 | 20,7 | 203,4 | 66,4 | 64,1 | **1024,3** | 1043,0 / 1039,2 / 1024,3 |

Lecture : « catalogue + tour » exclut la lecture du fichier et la préparation (tri de Morton, `SiteTree`, création du pool), mesurée une seule fois par processus. Ce sont les seules mesures G4 de la tour FULL 1..K au code courant.

## 2. Étages peu parallèles : plancher hors « boîtes » et « descentes » (session 4, 48 fils)

| Trame | K | Catalogue + tour | Boîtes + descentes | Reste (frontière, ordre, assemblage, hors étages, index, atlas, semis, Kruskal, verticales) | Part du reste | Kruskal de l'ordre K (tâche séquentielle) | Fusions verticales de l'ordre K (tâche séquentielle) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 00 | 5 | 252,0 | 135,3 | 116,7 | 46 % | 31,6 | 15,1 |
| 01 | 5 | 204,2 | 107,0 | 97,2 | 48 % | 20,8 | 12,5 |
| 02 | 5 | 253,6 | 128,6 | 125,0 | 49 % | 27,5 | 18,7 |
| 00 | 10 | 1124,6 | 713,9 | 410,7 | 37 % | 90,3 | 49,6 |
| 01 | 10 | 861,4 | 547,5 | 313,9 | 36 % | 52,3 | 36,4 |
| 02 | 10 | 1024,3 | 638,4 | 385,9 | 38 % | 66,4 | 49,7 |

## 3. Attaches des points (entrée `cover`, `mhgp10_tower --entry=cover --repeat`, dernière passe, 48 fils)

| Session (commit) | Trame | K | Catalogue | Tour avec attaches | dont attaches (`t_points`) | Catalogue + tour avec attaches |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| s4 (`777406b82`) | 00 | 5 | 165,8 | 106,4 | 21,7 | 272,2 |
| s4 (`777406b82`) | 01 | 5 | 135,8 | 84,8 | 18,7 | 220,6 |
| s4 (`777406b82`) | 02 | 5 | 164,8 | 113,2 | 24,3 | 278,0 |
| s1 (`8b8d66f6e`) | 00 | 5 | 741,8 | 102,2 | 21,5 | 844,0 |
| s1 (`8b8d66f6e`) | 01 | 5 | 529,6 | 84,6 | 19,0 | 614,2 |
| s1 (`8b8d66f6e`) | 02 | 5 | 1125,8 | 109,2 | 24,0 | 1235,0 |
| s1 (`8b8d66f6e`) | 00 | 10 | 3125,6 | 499,5 | 63,6 | 3625,1 |
| s1 (`8b8d66f6e`) | 01 | 10 | 2118,8 | 386,7 | 54,6 | 2505,5 |
| s1 (`8b8d66f6e`) | 02 | 10 | 4201,9 | 470,5 | 67,4 | 4672,4 |

L'entrée `core` (C∩X) n'a jamais été mesurée sur G4 sur trame entière. À K = 10, les attaches n'ont été mesurées qu'en session 1 (catalogue d'avant J1, J2 et J2c ; le code de la tour n'a pas changé depuis sur ce chemin).

## 4. Chaîne `mhgp10_cluster` (un seul ordre K = 5, entrée `cover`, tête EOM ; processus froid, préparation comprise dans `catalogue_s`)

| Session (commit) | Trame | `catalogue_s` (préparation + index + catalogue) | `tower_s` (ordre 5 seul, attaches comprises, sans verticales) | `head_s` | Somme | Mur du processus | Amas |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| s2 (`f8e78ad94`) | 00 | 317 | 57 | 173 | 547 | 566 | 66 |
| s2 (`f8e78ad94`) | 01 | 264 | 47 | 148 | 459 | 475 | 45 |
| s2 (`f8e78ad94`) | 02 | 320 | 60 | 178 | 558 | 572 | 46 |
| s3 (`82fc2a6b5`) | 00 | 232 | 57 | 173 | 462 | 480 | 66 |
| s3 (`82fc2a6b5`) | 01 | 186 | 46 | 148 | 380 | 397 | 45 |
| s3 (`82fc2a6b5`) | 02 | 238 | 60 | 177 | 475 | 491 | 46 |
| s4 (`777406b82`) | 00 | 177 | 58 | 24 | 259 | 273 | 66 |
| s4 (`777406b82`) | 01 | 151 | 47 | 20 | 218 | 231 | 45 |
| s4 (`777406b82`) | 02 | 178 | 61 | 24 | 263 | 278 | 46 |

Aucune mesure G4 de la tête à K = 10, ni d'une tête sur les K ordres d'une tour FULL.

## 5. Catalogue seul selon le nombre de fils (trame 02, `mhgp10_catalogue`, une passe froide par processus)

| Session (commit) | K | Fils | Catalogue | Frontière | Boîtes | Ordre | Assemblage | Hors étages | Accélération du catalogue | Accélération des boîtes | Tâches | Mur du processus | RSS max (Kio) | Octets par boule au pic |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| s2 (`f8e78ad94`) | 5 | 1 | 8513,8 | 72,0 | 7978,6 | 163,7 | 295,7 | 3,8 | ×1,0 | ×1,0 | 765 | 8526 | 370144 | 269,2 |
| s2 (`f8e78ad94`) | 5 | 12 | 781,6 | 20,1 | 653,5 | 39,1 | 64,4 | 4,5 | ×10,9 | ×12,2 | 6578 | 795 | 405708 | 295,1 |
| s2 (`f8e78ad94`) | 5 | 24 | 480,8 | 23,2 | 368,8 | 33,7 | 49,8 | 5,3 | ×17,7 | ×21,6 | 18980 | 495 | 410376 | 298,5 |
| s2 (`f8e78ad94`) | 5 | 48 | 313,7 | 25,1 | 214,2 | 27,6 | 41,4 | 5,4 | ×27,1 | ×37,2 | 27651 | 329 | 437680 | 318,3 |
| s2 (`f8e78ad94`) | 10 | 24 | 1629,9 | 35,7 | 1257,8 | 113,9 | 206,1 | 16,4 | — | — | 19012 | 1663 | 1819664 | 339,8 |
| s2 (`f8e78ad94`) | 10 | 48 | 1098,1 | 32,6 | 759,6 | 104,2 | 183,1 | 18,6 | — | — | 27979 | 1132 | 1804804 | 337,0 |
| s3 (`82fc2a6b5`) | 5 | 1 | 4818,7 | 31,3 | 4325,8 | 160,9 | 297,0 | 3,7 | ×1,0 | ×1,0 | 765 | 4833 | 368548 | 268,1 |
| s3 (`82fc2a6b5`) | 5 | 12 | 482,3 | 11,2 | 361,9 | 39,5 | 65,0 | 4,7 | ×10,0 | ×12,0 | 6578 | 496 | 402936 | 293,1 |
| s3 (`82fc2a6b5`) | 5 | 24 | 323,5 | 15,0 | 219,5 | 33,5 | 50,8 | 4,7 | ×14,9 | ×19,7 | 18980 | 338 | 426740 | 310,4 |
| s3 (`82fc2a6b5`) | 5 | 48 | 235,1 | 17,4 | 143,1 | 29,1 | 41,5 | 4,0 | ×20,5 | ×30,2 | 27651 | 253 | 500304 | 363,9 |
| s3 (`82fc2a6b5`) | 10 | 1 | 19160,7 | 50,4 | 17210,0 | 602,2 | 1280,6 | 17,5 | ×1,0 | ×1,0 | 765 | 19184 | 1587572 | 296,5 |
| s3 (`82fc2a6b5`) | 10 | 24 | 1189,0 | 22,9 | 823,2 | 115,3 | 210,9 | 16,7 | ×16,1 | ×20,9 | 19012 | 1221 | 1793432 | 334,9 |
| s3 (`82fc2a6b5`) | 10 | 48 | 840,4 | 26,5 | 516,0 | 100,4 | 179,7 | 17,8 | ×22,8 | ×33,4 | 27979 | 875 | 1819788 | 339,8 |
| s4 (`777406b82`) | 5 | 1 | 4039,6 | 55,9 | 3508,5 | 163,9 | 307,1 | 4,2 | ×1,0 | ×1,0 | 366 | 4052 | 369608 | 268,8 |
| s4 (`777406b82`) | 5 | 12 | 383,9 | 16,6 | 291,3 | 27,0 | 42,5 | 6,5 | ×10,5 | ×12,0 | 4496 | 398 | 401508 | 292,0 |
| s4 (`777406b82`) | 5 | 24 | 258,8 | 21,6 | 180,5 | 21,4 | 27,7 | 7,6 | ×15,6 | ×19,4 | 10287 | 274 | 410580 | 298,6 |
| s4 (`777406b82`) | 5 | 48 | 171,1 | 26,6 | 103,3 | 13,8 | 18,7 | 8,7 | ×23,6 | ×34,0 | 19482 | 189 | 424676 | 308,9 |
| s4 (`777406b82`) | 10 | 1 | 16761,7 | 95,3 | 14740,7 | 621,6 | 1285,8 | 18,3 | ×1,0 | ×1,0 | 359 | 16785 | 1587744 | 296,5 |
| s4 (`777406b82`) | 10 | 24 | 935,6 | 31,2 | 713,0 | 64,7 | 106,2 | 20,5 | ×17,9 | ×20,7 | 10387 | 972 | 1808628 | 337,8 |
| s4 (`777406b82`) | 10 | 48 | 627,9 | 39,6 | 441,6 | 48,1 | 75,7 | 22,9 | ×26,7 | ×33,4 | 19843 | 668 | 1823444 | 340,5 |

## 6. Tour selon le nombre de fils (trame 02, K = 5, sans attaches)

| Session | Fils | Tour | index | atlas | semis | descentes | Kruskal (étage) | Kruskal (somme des ordres) | verticales (étage) | verticales (somme des ordres) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| s1 (`8b8d66f6e`) | 1 | 1154,4 | 36,8 | 33,6 | 50,7 | 851,7 | 77,6 | 77,6 | 102,9 | 44,7 |
| s1 (`8b8d66f6e`) | 24 | 107,7 | 3,9 | 5,7 | 4,3 | 44,5 | 26,5 | 75,2 | 22,7 | 44,8 |
| s1 (`8b8d66f6e`) | 48 | 85,1 | 3,2 | 4,3 | 3,1 | 26,0 | 27,0 | 84,4 | 21,2 | 44,7 |
| s4 (`777406b82`) | 24 | 109,8 | 3,6 | 5,8 | 4,8 | 47,2 | 25,5 | 73,5 | 22,8 | 45,0 |
| s4 (`777406b82`) | 48 | 89,3 | 3,3 | 5,0 | 3,2 | 27,1 | 27,5 | 76,7 | 21,5 | 47,0 |

## 7. Compteurs déterministes du catalogue (trame 02, session 4)

| K | Boules | Niveaux distincts | Nœuds | Feuilles | Candidats (Σm) | m moyen | m max | Tests du filtre | Dominance de feuille | Paires | Triplets | Droites touchant la boîte | Quadruplets | Jugements | Coquilles étendues |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 1407885 | 1099581 | 734083 | 323224 | 4510019 | 13,95 | 16 | 403800945 | 29738790 | 18894584 | 28586816 | 20416010 | 9380910 | 3304711 | 572 |
| 10 | 5483320 | 4908695 | 1065585 | 485833 | 10485734 | 21,58 | 24 | 1287951189 | 108996239 | 57829684 | 133332063 | 94209150 | 75108960 | 10818144 | 1301 |

