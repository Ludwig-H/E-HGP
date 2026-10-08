# Session G4 v12.20261008.t1bi : voie appareil du catalogue (T1-b), **adoptée**

8 octobre 2026. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à `d2f39fe82`), cible
`us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM démarrée à 00:13:29 UTC, worker de
00:15:24 à 00:22:29 (code 0), **arrêt certifié `TERMINATED`** par le lanceur (clôture `stopped`) et relu indépendamment
à 00:24:57 UTC (`lastStopTimestamp` 00:24:12 UTC). Reçu sans identité de compte : [`receipt.json`](receipt.json) ;
rapport et journaux du pilote sous `resultats/cmd/000_t1b_catalogue_appareil/files/t1b/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue, voie hybride) ; cpu_reference pour l'identité
objet=full_pi0 (catalogue Cat_K)
quantification=quantized_u21_input_only (profil 21 seul joué sur l'appareil)
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `c52a6450ad36cb0b…` |
| plan | `0b14810a63d54efd…` |
| données (trames ng00–02, uniformes de 8 000 à 32 000 sites, archives v10 et v11) | manifeste `fb6b79d2cd45f691…` |
| GPU | NVIDIA RTX PRO 6000 Blackwell Server Edition, pilote 580.178.04, 97 887 Mio, isolé avant et après les temps |

Pilote et juge : [`bench/g4_catalogue_device.py`](../../bench/g4_catalogue_device.py), règle écrite le 7 octobre avant
la session (contrat du catalogue § 10), admission stricte de l'auditeur Codex intégrée en `781fbe8d1`.

## Verdict : **adopté** (aucun refus, aucun rejet)

- **Construction CUDA** du produit au profil 21 (`MHGP12_ENABLE_CUDA=ON`, `sm_120`, nvcc 12.9, GCC 11.4, CMake 3.22.1) et
  **665 portes rapides** de cette construction : vertes. La porte `device_open` joue la **vraie voie appareil** contre la
  voie CPU sur neuf témoins : 217 contrôles, aucun échec.
- **Identité à l'octet** de l'export `MHGP12DP` voie appareil contre voie CPU (48 fils) sur ng00, ng01, ng02 à K5 et K10
  et sur les uniformes de 8 000, 16 000 et 32 000 sites à K5 ; grand livre, comptes, diagnostics physiques, reprises de
  la voie hybride par cause et réécritures égaux ; déterminisme sur trois passes d'un même processus ; empreintes de la
  voie CPU égales à celles de la session F2.
- **Trois mutants appareil tués** : feuille non résolue admise sans rejeu, fin d'étage sans départage exact, un fil par
  feuille.

## Étage C sur l'appareil, à chaud (K5 : 5 processus × 10 passes, 45 valeurs par trame ; ms)

| Trame | médiane | max des médianes par processus | max | 1re passe | parcours | feuilles | émission | fin d'étage | transferts | publication |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | **35,07** | 35,46 | 36,92 | 61,4 | 4,36 | 11,71 | 3,59 | 2,56 | 9,09 | 3,72 |
| ng01 | **30,82** | 30,95 | 32,30 | 53,3 | 3,96 | 9,47 | 3,83 | 2,38 | 7,86 | 3,24 |
| ng02 | **37,58** | 37,71 | 38,51 | 69,7 | 4,45 | 10,86 | 3,89 | 4,08 | 10,40 | 3,74 |

Budget de l'étage C : 35 à 45 ms ; règle : médiane **et** maximum des médianes par processus au plus 45 ms sur chaque
trame. Tenu sur les trois. À K10 (publié, 3 processus × 5 passes) : 137,4 / 112,9 / 146,2 ms, dont transferts 46 / 37 /
51 ms et feuilles 40 / 32 / 37 ms.

## Voie CPU après la fin d'étage partagée (48 fils, 10 passes ; publiée, ne décide rien)

| Cas | médiane (ms) | session F2 | rapport | parcours | feuilles | émission | fin d'étage |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5, feuille 16 / 24 | 330,7 / 327,8 | 465 / 452 | 0,71 / 0,73 | 116,0 / 74,4 | 129,2 / 156,8 | 13,6 / 24,9 | 68,1 / 66,9 |
| ng01 K5, feuille 16 / 24 | 289,4 / 280,9 | 374 / 373 | 0,77 / 0,75 | 98,8 / 64,2 | 103,7 / 126,9 | 14,6 / 24,6 | 67,4 / 60,9 |
| ng02 K5, feuille 16 / 24 | 333,8 / 332,0 | 468 / 470 | 0,71 / 0,71 | 115,8 / 76,7 | 120,7 / 148,2 | 15,6 / 29,0 | 76,2 / 72,6 |
| ng00 / ng01 / ng02 K10, feuille 24 | 1 066,8 / 889,4 / 1 054,3 | 1 750 / 1 380 / 1 699 | 0,61 / 0,64 / 0,62 | | | | |
| uniformes 8 000 / 16 000 / 32 000 K5 | 118,5 / 224,2 / 436,6 | 158 / 327 / 671 | 0,75 / 0,69 / 0,65 | | | | |

La fin d'étage partagée divise par deux ou trois celle de F2 (130 à 170 ms à K5) ; la voie CPU reste au-dessus de celle de
la v11 (200 / 163 / 195 ms à K5) : elle sert la référence et les petits nuages, pas les trames.

## Lecture

L'étage C tient son budget sur l'appareil : 31 à 38 ms à K5 à chaud, dont 8 à 10 ms de transferts. Avec l'étage G
mesuré sur la VM la veille (63 à 80 ms à K5, [session H](../g4_t2h_20261007/README.md)), la tour de bout en bout reste
au-dessus de 100 ms tant que G n'est pas divisé par deux à trois (T2-c en cours) et que T, M, V et R ne sont pas
mesurés. À K10, les transferts (37 à 51 ms) deviennent le premier poste de l'étage C.

## Ce que cette session n'établit pas

Ni les profils 24 et 32 sur l'appareil, ni la tour, ni le budget de 100 ms ; une seule séquence de trois trames. GCP
utilisé pour cette seule session, arrêt certifié.
