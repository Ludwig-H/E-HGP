# Reçu : session G4 de performance après J1, CPU seul (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. **Aucun calcul sur GPU.**

## Session

- Code au commit `f8e78ad94`, qui contient l'assemblage parallèle du catalogue (`2c7b8130e`) et la frontière pilotée
  par la charge (J1, `b58479b99`). Plan `plan_s2.json` : 20 commandes, toutes réussies.
- Données : les trois trames LiDAR entières sans sol, 39 885, 35 551 et 45 845 sites.
- VM `g4-standard-48` SPOT : AMD EPYC 9B45, 24 cœurs et 48 fils.
- Arrêt certifié par `stop_and_verify.sh` (code 0), puis TERMINATED sur la cible exacte. Génération
  `2026-09-29T04:27:22.115-07:00`, arrêt `04:31:52.732-07:00`.
- Statut `failed_remote` seulement parce que la VM n'a pas pip (le plan contient `ctest`, qui déclenche l'étape
  Python). La porte C++ `mhgp10_unit` est verte.
- `receipt.json` et `preflight.json` copiés avec l'adresse du compte masquée ; sha256 des originaux dans
  `ORIGINAUX.sha256`.

## Résultats (dernière passe chaude, 48 fils sauf mention)

**Catalogue de la trame 02 à K = 5, selon le nombre de fils :**

| Fils | Catalogue (s) | Boîtes (s) | Ordre (s) | Assemblage (s) | Tâches | Sites de la plus lourde |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 8,514 | 7,979 | 0,164 | 0,296 | 765 | 716 |
| 12 | 0,782 | 0,653 | 0,039 | 0,064 | 6 578 | 59 |
| 24 | 0,481 | 0,369 | 0,034 | 0,050 | 18 980 | 29 |
| 48 | 0,314 | 0,214 | 0,028 | 0,041 | 27 651 | 14 |

Accélération de 1 à 48 fils : ×27, contre ×7,7 en session 1 (`receipts/g4_session1_20260929`).

**Trames entières (catalogue + tour, sans attaches) :**

| Trame | K | Session 1 (s) | Session p2 (s) | Catalogue (s) | Tour (s) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 00 | 5 | 0,818 | 0,373 | 0,289 | 0,084 |
| 01 | 5 | 0,584 | 0,305 | 0,236 | 0,069 |
| 02 | 5 | 1,190 | 0,373 | 0,285 | 0,088 |
| 00 | 10 | 3,542 | 1,541 | 1,113 | 0,428 |
| 01 | 10 | 2,453 | 1,233 | 0,900 | 0,333 |
| 02 | 10 | 4,638 | 1,498 | 1,086 | 0,412 |

**Chaîne complète jusqu'aux étiquettes** (`mhgp10_cluster`, K = 5, couverture, EOM z = 3, mcs = 200) : 0,547 s, 0,459 s
et 0,558 s. Elle rend 66, 45 et 46 amas.

## Lecture

- Le contrat de la v9 (1 s à K = 5 sur G4) est tenu avec de la marge sur les trois trames, en CPU seul.
- À K = 10, 1,2 à 1,5 s.
- L'énumération des boîtes reste l'étage dominant (0,21 s sur 0,31 s à K = 5). Prochain levier : le coût par test du
  filtre de l'arbre (J2, environ 33 cycles par test contre 2,4 possibles), puis le portage sur GPU, pour viser 100 ms
  à K = 5 et 1 s à K = 10.
