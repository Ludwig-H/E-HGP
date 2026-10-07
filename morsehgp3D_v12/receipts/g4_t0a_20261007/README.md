# Session G4 v12.20261007.t0a : socle, MES-M6, MES-M2

7 octobre 2026, 11 h 12 à 11 h 25 UTC. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à
`26b53648c`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM démarrée à
11:12:57 UTC, **arrêt certifié `TERMINATED` à 11:24:50 UTC** (`targeted_shutdown_certified = true`). Reçu de la session
sans identité de compte : [`receipt.json`](receipt.json) (commandes de reprise masquées) ; résultats choisis sous
`resultats/` (dossier personnel de la VM réécrit `$HOME`), fabriqués par `microbancs/outils/recu_session.py` ;
empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 (microbancs)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet (instantané `morsehgp3D_v12/`) | `a5d1ff2383b230eb…` |
| plan | `643e6fcbd7fa505a…` |
| données (14 fichiers : ng00–02, uniformes, v10 figée, archive des sources v11 `6f3454ad…`) | manifeste `fb6b79d2cd45f691…` |
| résultats rapatriés | `44594dc6f7e9b64b…` |
| VM | 48 fils, 177 Gio, RTX PRO 6000 Blackwell Server Edition (pilote 580.178.04, 188 SM, 96 Gio), GCC 11.4.0, CMake 3.22.1, Python 3.10.12 |

## Commandes

| Commande | État | Durée |
| --- | --- | ---: |
| `source_v11` : sources de la v11 gelée déballées et vérifiées, `libmhgp11.a` construite (GCC 11.4 : `8a27ce2f…`) | ok | 5 s |
| `socle` : `ctest -LE long` sur la construction Release u21 de la v12 | ok, **389 sur 389** (sentinelle LiDAR jouée) | 25 s |
| `m6` : MES-M6, 3 processus par mode d'attente | ok | 14 s |
| `m2` : MES-M2, 9 cas, 5 processus, 15 répétitions dont 3 d'échauffement | ok | 457 s |
| `m2_publier` | ok, 122 fichiers (vidages exclus) | 0 s |

## MES-M2 : feuille du catalogue data-parallèle

Règle écrite avant la mesure ([`microbancs/mes_m2_feuille/README.md`](../../microbancs/mes_m2_feuille/README.md) § 9) :
identité sur les neuf cas et borne haute de l'IC 95 % au plus 1/3 du témoin sur chaque cas qui décide (feuilles de 24,
K5 et K10). **Verdict : adoptée, variante `j3_r168`** (J3 par phases, 168 registres). Identité exacte sur tous les cas,
0 feuille non résolue ; Compute Sanitizer (memcheck, racecheck, synccheck) : 0 erreur ; isolation du GPU vérifiée.

| Variante | Verdict | Moyenne géométrique (cas qui décident) | Pire borne haute | Feuilles de 16 (publié) |
| --- | --- | ---: | ---: | --- |
| `j3_r168` | **adoptée, choisie** | 0,176 | 0,203 | 0,30 à 0,31 |
| `j3_r128` | adoptée | 0,196 | 0,223 | 0,32 à 0,33 |
| `j3` | adoptée | 0,239 | 0,276 | 0,42 à 0,43 |
| `coherent_r128` | rejetée | 0,360 | 0,440 | 0,56 à 0,58 |
| `coherent_r168` | rejetée | 0,363 | 0,444 | 0,57 à 0,58 |
| `coherent` | rejetée | 0,492 | 0,602 | 0,78 à 0,80 |

Temps du noyau seul (médiane des médianes de 5 processus, millisecondes ; ni transferts, ni fin d'étage) :

| Cas | Témoin v11 (un fil par feuille) | `j3_r168` | Gain |
| --- | ---: | ---: | ---: |
| ng00 K5/24 | 69,6 | 11,6 | ×6,0 |
| ng01 K5/24 | 64,9 | 9,3 | ×7,0 |
| ng02 K5/24 | 67,9 | 10,8 | ×6,3 |
| ng00 K10/24 | 183,9 | 37,2 | ×4,9 |
| ng01 K10/24 | 149,4 | 29,4 | ×5,1 |
| ng02 K10/24 | 170,6 | 33,8 | ×5,0 |
| ng00 K5/16 | 30,9 | 9,6 | ×3,2 |

La prédiction écrite avant G4 (J3 entre 0,1 et 0,4 du témoin, forme cohérente probablement rejetée) est confirmée. Le
témoin mesure 70 et 184 ms sur ng00, contre environ 60 et 196 ms prédits d'après la v11.

## MES-M6 : coût de la Session résidente

Médianes d'un processus par mode (microsecondes ; trois processus par mode dans `resultats/`) :

| Mesure | spin | yield | blocking |
| --- | ---: | ---: | ---: |
| ouverture du contexte (une fois par processus) | 357 945 | 115 928 | 115 958 |
| premier lancement | 579 | 87 | 128 |
| noyau vide, lancement et synchronisation | 8,1 | 8,1 | 13,0 |
| dix lancements | 24,6 | 24,6 | 29,1 |
| graphe de dix noyaux | 11,4 | 11,5 | 24,5 |
| réservation puis libération de 256 Mio, pool chaud | 2,9 | 2,9 | 1,1 |
| copie épinglée de 720 Kio (une trame de 60 000 sites), hôte vers appareil | 19,7 | 20,5 | 40,5 |
| copie épinglée de 256 Mio | 4 727 (56,8 Go/s) | 4 726 | 4 801 |
| lecture et écriture de 256 Mio sur l'appareil | 368 (1,46 To/s) | 369 | 403 |

Lecture : le contexte coûte 116 ms, payés une fois par la Session résidente (décision D1) ; ensuite les coûts fixes par
trame se comptent en microsecondes. L'attente bloquante double le coût des petites copies et des synchronisations ;
proposition pour la Session : attendre en `yield` (même coût que `spin`, premier lancement plus court) et grouper les
suites de lancements en graphes (coût divisé par deux).

## Ce que cette session n'établit pas

Aucun temps de la tour, ni du catalogue complet (transferts, frontière, fin d'étage), ni du chemin produit : ce sont des
microbancs hors produit. GCP utilisé pour cette seule session, arrêt certifié.
