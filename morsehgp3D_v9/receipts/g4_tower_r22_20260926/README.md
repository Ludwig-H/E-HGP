# Reçu : session G4 R22 (catalogue scellé, recensement q2 précoce, bassin épinglé, sonde v28)

26 septembre 2026, 04 h 00 – 04 h 07 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-25T21:00:02.368-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-25T21:07:02.314-07:00`).

**Paquet** : construit depuis `43c5ad25`, protocole au commit. Sonde v28.
Snapshot `ad24cae1…`, manifeste `10bd9382…`, worker `b1d9c65d…`.

**Contenu mesuré** :
- **catalogue scellé** (R-29 de C, `tower_sealed_catalogue`) : la chaîne
  certifie chaque support régulier (forme, positivité) et la tour ne rejoue
  sa passe 1 que sur une boule sur 64, derrière un sceau de type ;
- **recensement des clés q2** côté q2, pendant les appels de l'appareil
  (`q2_early_census`) ;
- **bassin hôte épinglé** pour les enregistrements des voies
  (`q34_lanes_pinned`), réservé par la session d'appareil (256 Mo à K5,
  1,28 Go à K10), avec des chronos qui séparent la copie du travail hôte.

**Plan R22 à 36 cas** :
- les trames sans sol 08/000000, 000100 et 000200 : bras GPU (les trois
  leviers actifs) et jumeau moteur (scellé), à K5 et K10 ;
- à 00 :
  - paires répétées et entrelacées avec `gpu_r21` (les trois leviers
    coupés) ;
  - chaque levier coupé seul, à K5 et à K10 ;
- les trames brutes b00, b01 et b02 : bras GPU et jumeau moteur, à K5 et
  K10.

**Reçus** : hôte et worker **`completed`**. Les 36 cas sont
`complete_relative`. Aucun cas non apparié. Mur utile du worker : 264 s.

## Préflights et objet

- Préflight jugé et jumeau moteur (`73490cf8`), ardoises réduites : 3 404
  arêtes rendues au moteur et 11 993 voies en traîne. Le préflight exerce
  les trois leviers : clés q2 recensées côté q2, sceau et bassin.
- Euler « holds » jusqu'à K−2, les douze épingles reproduites (six sans sol,
  six brutes).
- Les 24 comparaisons entre cas sont égales, condensé des présentations
  compris : les bras scellé et non scellé, avec et sans recensement
  précoce, avec et sans bassin, et le moteur donnent le même objet.
- Sous le sceau, l'échantillon compté vaut bien une boule sur 64 (20 418 à
  00/K5, 182 572 à b02/K10). Côté q2, toutes les clés q2 sont recensées
  (456 919 à 00/K5), sans voie de repli. Le bassin ne grandit dans aucun
  appel (0 allocation).
- Reports du chemin GPU au moteur sur le brut : certificats 22 et 57 à b00,
  voies 8 à b02/K10. Aucun refus de capacité.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | **GPU** | `gpu_r21` | sans sceau | sans recensement précoce | sans bassin | R21 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,180 | **0,760** | | | | | 0,805 |
| 000000 | 5 | 2,770 | **0,926 / 0,940** | 1,002 / 1,008 | 0,961 | 0,951 | 0,964 | 0,972 / 0,998 |
| 000200 | 5 | 3,000 | **0,983** | | | | | 1,026 |
| 000100 | 10 | 6,101 | **2,269** | | | | | 2,446 |
| 000000 | 10 | 8,405 | **2,961 / 2,994** | 3,227 / 3,218 | 3,034 | 3,032 | 3,121 | 3,165 / 3,212 |
| 000200 | 10 | 8,614 | **2,888** | | | | | 3,136 |
| brut 000000 | 5 | 5,614 | **1,991** | | | | | 2,094 |
| brut 000100 | 5 | 4,886 | **1,810** | | | | | 1,914 |
| brut 000200 | 5 | 5,433 | **2,027** | | | | | 2,180 |
| brut 000000 | 10 | 14,877 | **6,010** | | | | | 6,487 |
| brut 000100 | 10 | 12,400 | **5,311** | | | | | 5,711 |
| brut 000200 | 10 | 14,293 | **5,907** | | | | | 6,334 |

Hors chaîne, la session d'appareil prend 121 à 166 ms de contexte, plus la
réservation du bassin : 38 à 40 ms pour 256 Mo à K5, 190 à 201 ms pour
1,28 Go à K10.

## Leviers, à 08/000000 (ms)

| | K5 GPU | K5 sans le levier | K10 GPU | K10 sans le levier |
| --- | ---: | ---: | ---: | ---: |
| sceau : validation (passe 1) | 34 (0) | 50 (16) | 116 (1) | 175 (68) |
| recensement précoce : index + recensement | 0 + 102 | 10 + 106 | 0 + 505 | 10 + 522 |
| bassin : transfert des voies (copie des enregistrements) | 4,8 (1,9) | 30,6 (27,8) | 14,1 (10,5) | 148,5 (144,9) |
| les trois ensemble : chaîne | 926 / 940 | 1 002 / 1 008 | 2 961 / 2 994 | 3 227 / 3 218 |

Côté q2, le recensement prend 66 à 78 ms à K5 et 99 à 122 ms à K10, sans
aucune attente : il est entièrement caché par les appels de l'appareil.
Mais les clés q2 sont les moins chères. Le recensement restant ne baisse
que de 4 ms à K5 et de 17 ms à K10 ; le reste du gain vient de l'index
réutilisé.

## Lecture

- **Seuil d'une seconde à K5 sans sol : atteint sur les trois trames.**
  08/000000 : 0,926 et 0,940 s ; 08/000100 : 0,760 s ; 08/000200 : 0,983 s.
  Ce sont des trames de la séquence 08, sans sol, avec la session
  d'appareil ouverte par le processus. Aucune qualification de contrat
  n'en découle.
- **Les trois leviers** retirent 62 à 82 ms à K5 et 224 à 266 ms à K10 par
  rapport au bras `gpu_r21` apparié. Le bassin épinglé divise la copie des
  enregistrements par 14 environ, à K5 comme à K10 : c'était bien un transfert en
  mémoire pageable (remarque de B), désormais mesuré par ablation.
- **Trames brutes avec sol** : K5 1,81 à 2,03 s, K10 5,31 à 6,01 s. Le pic
  de RSS atteint 10,0 Gio à K10 : il compte le bassin épinglé de 1,28 Go.
- **Tour à 00/K5** (289 ms) : validation 34, fenêtre 186 (phase 0 de
  l'ordre 5 : 38, puis sa phase A : 148), queue 68. Comme C l'a établi,
  la fenêtre est bornée par A(Kmax).
- **Reste à 08/000200/K5** (0,983 s) :
  - tour 0,30 s ;
  - q34 0,51 s : front 0,10, filtre 0,10, certificats 0,13, voies 0,09 ;
  - recensement 0,11 s, fusion 0,03 s.
