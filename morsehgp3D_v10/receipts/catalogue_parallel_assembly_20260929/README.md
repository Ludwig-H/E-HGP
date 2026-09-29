# Reçu : assemblage parallèle du catalogue (29 septembre 2026)

`public_status=not_claimed`. Changement d'implémentation sans changement d'objet.

## Motif

La première session G4 (`receipts/g4_session1_20260929`) a montré que le catalogue plafonne à ×7,7 entre 1 et 48 fils
sur la trame LiDAR 02 à K = 5. Après l'énumération parallèle des boîtes de centres, tout l'assemblage était
séquentiel :

- collecte des enregistrements ;
- tri canonique (niveau exact, S*) avec un comparateur indirect ;
- réparation exacte des bandes où les approximations double des niveaux se touchent ;
- comparaisons exactes des niveaux voisins, rangs, décalages ;
- copie des populations.

## Ce qui change

- Collecte à des positions fixées par des préfixes, avec une clé plate (approximation, S*, origine).
- Tri parallèle par échantillonnage régulier (`src/sched/sort.hpp`). La clé (approximation, S*) est un ordre total,
  sauf pour une boule émise deux fois, qui est refusée plus loin. Le résultat est donc celui du tri séquentiel, quel
  que soit le nombre de fils.
- Bandes flottantes repérées en série, puis triées en exact en parallèle : chaque bande ne touche que ses positions.
- Comparaisons exactes des voisins en parallèle. Le premier défaut dans l'ordre décide du refus, qui reste donc
  déterministe.
- Rangs, niveaux distincts et décalages par sommes préfixes par blocs ; copie en parallèle.
- Chronomètres par étage publiés dans `catalogue_stages` par `mhgp10_catalogue` et `mhgp10_tower`, avec le nombre
  de bandes réparées.

## Contrôles

- `differentiel_final.txt` : dumps canoniques complets du catalogue (niveau, rang, support, I, U), binaire
  `8b8d66f6e` contre le nouveau à 8 et à 3 fils. Cinq entrées (deux trames LiDAR entières, un quart de trame, deux
  synthétiques), K = 5 et 10 : identiques. `differentiel_v1.txt` est le même contrôle sur une première version.
- Portes v10 locales vertes, 7 sur 7.

## Profil (trame LiDAR 02, K = 5, codespace chargé par d'autres calculs)

Fichiers `lidar02_k5_w1.json` et `lidar02_k5_w8.json` :

| Étage | 1 fil (s) | 8 fils (s) |
| --- | ---: | ---: |
| frontière en largeur | 0,125 | 0,069 |
| boîtes (énumération des feuilles) | 14,25 | 3,62 |
| ordre : collecte, tri, bandes | 0,278 | 0,178 |
| assemblage : comparaisons, rangs, copie | 0,549 | 0,249 |

- 68 645 bandes flottantes, qui regroupent 376 949 boules sur 1,4 million : sur la grille d'1 mm, beaucoup de boules
  partagent un même niveau exact.
- L'énumération des boîtes fait 93 % du temps à 1 fil. Sur la G4, elle ne gagne presque plus au-delà des 24 cœurs
  physiques. C'est le prochain levier : réduire le travail par feuille, ou porter les feuilles sur GPU.
