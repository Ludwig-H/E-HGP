# Session G4 v12.20261007.t1f2 : catalogue CPU de la v12 sur la VM

7 octobre 2026. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à `4eda10eb3`, catalogue CPU
`671072339` compris), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, **construction par défaut**
du produit au profil 21 sur la chaîne de la VM (GCC 11.4, CMake 3.22). VM démarrée à 19:21:04 UTC, worker de 19:22:48 à
19:27:27 (code 0), **arrêt certifié `TERMINATED`** (clôture `stopped`), relu indépendamment (`lastStopTimestamp` 19:29:01).
Reçu sans identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

Une tentative précédente (`v12.20261007.t1f`, 19:14 UTC) a démarré la VM puis échoué au téléversement des données : ses
333 fichiers (dont les 320 petits nuages de `MES-P`) ont dépassé le délai calculé sur leur taille ; aucun worker lancé,
VM arrêtée par le lanceur (arrêt certifié), état `TERMINATED` relu. `MES-P` voyagera en une seule archive.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (produit v12, voie CPU du catalogue)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `c4f073273a322363…` |
| plan | `0d4e3a953934d4c5…` |
| données (14 fichiers : trames ng00–02, uniformes, archives v10 et v11) | manifeste `fb6b79d2cd45f691…` |

## Portes sur la chaîne de la VM

`ctest --no-tests=error -LE long -j 40` : **607 sur 607** en 103 s (portes du catalogue comprises ; les portes
`diff_v10` n'ont pas de binaire figé sur la VM et ne sont pas enregistrées).

## Temps de la voie CPU du catalogue (`mhgp12_catalogue_probe`, 48 fils, 10 passes, publié)

Médianes des passes 2 à 10 (millisecondes) ; une seule empreinte `catalogue_digest` par cas sur les dix passes :

| Cas | sites | boules | froid | chaud (médiane) | chaud (max) | parcours | comptage des feuilles | écriture | tri | assemblage | table $S^{*}$ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5, feuille 16 | 39 885 | 1 306 696 | 469 | 465 | 466 | 115 | 142 | 23 | 9 | 106 | 49 |
| ng00 K5, feuille 24 | 39 885 | 1 306 696 | 461 | 452 | 455 | 74 | 170 | 28 | 9 | 104 | 50 |
| ng00 K10, feuille 24 | 39 885 | 5 512 670 | 1 761 | 1 750 | 1 763 | 209 | 599 | 94 | 40 | 441 | 286 |
| ng01 K5, feuille 16 | 35 551 | 1 095 926 | 375 | 374 | 375 | 97 | 114 | 15 | 7 | 82 | 40 |
| ng01 K5, feuille 24 | 35 551 | 1 095 926 | 379 | 373 | 373 | 63 | 137 | 27 | 8 | 82 | 40 |
| ng01 K10, feuille 24 | 35 551 | 4 383 302 | 1 391 | 1 380 | 1 387 | 174 | 475 | 83 | 29 | 338 | 223 |
| ng02 K5, feuille 16 | 45 845 | 1 407 885 | 467 | 468 | 473 | 113 | 132 | 17 | 12 | 121 | 52 |
| ng02 K5, feuille 24 | 45 845 | 1 407 885 | 479 | 470 | 472 | 76 | 160 | 32 | 12 | 120 | 52 |
| ng02 K10, feuille 24 | 45 845 | 5 483 320 | 1 710 | 1 699 | 1 705 | 204 | 546 | 104 | 44 | 449 | 273 |
| uniforme 8 000 K5 | 8 000 | 597 998 | 160 | 158 | 158 | 19 | 57 | 9 | 3 | 37 | 24 |
| uniforme 16 000 K5 | 16 000 | 1 233 046 | 336 | 327 | 329 | 30 | 120 | 17 | 7 | 80 | 55 |
| uniforme 32 000 K5 | 32 000 | 2 536 732 | 675 | 671 | 674 | 52 | 242 | 37 | 14 | 175 | 116 |

**Lecture.** La voie CPU de la v12 coûte environ **2,3 fois** celle de la v11 à 48 fils sur les trames (domaine de la v11 :
200 / 163 / 195 ms à K5 sur ng00 / ng01 / ng02, `MESURE.md`). Deux causes, toutes deux attendues : la **fin d'étage**
(assemblage et table $S^{*}\to$ boule, 130 à 170 ms à K5, 560 à 730 ms à K10) n'est presque pas parallèle, et la feuille J3
sur warp simulé (114 à 170 ms à K5) coûte plus que la DFS de la v11. Conformément au contrat (§ 2 : « en cas d'échec, la
décision de conception est révisée ouvertement »), cela ne change pas la voie produit des trames, qui est la voie
appareil (T1-b, même source, fin d'étage sur l'appareil), mais impose deux suites sur la voie CPU : paralléliser la fin
d'étage (assemblage par blocs et préfixes, comme l'assemblage de la v11 ; construction parallèle de la table), puis
juger le seuil des petits nuages par `MES-P`.

## Ce que cette session n'établit pas

Ni la voie appareil du catalogue, ni la tour, ni le budget de 100 ms. GCP utilisé pour ces deux tentatives seulement,
arrêts certifiés.
