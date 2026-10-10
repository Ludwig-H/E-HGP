# Session G4 O : état du contrat FULL et des petits nuages sur le produit avec A6c (`MES-FULL`, `MES-C`)

10 octobre 2026. Session gardée `v12.20261010.fullo` (`gcp-migration/v12_session.py`, commit `aa6338ee8`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
18:44:38 à 19:13:02 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 19:13:27 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris ; K10 sur ng00–02)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Produit mesuré.** La Session recouverte, voie par défaut, avec le cache de blocs. Les leviers adoptés sur G4 y
sont : le lot B2, T1-d, R1 et, depuis la [session A6c](../g4_a6c_20261010/README.md), la chaîne de l'ordre K engagée
au-delà de 43 900 sites. A6, A6b et B3 sont rejetés et retirés.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok | 141 s |
| `mes_full` ([`pilote_full.py`](../../microbancs/mes_full/pilote_full.py)) | ok, verdict rendu, aucun refus | 521 s |
| `mes_c` ([`pilote_c.py`](../../microbancs/mes_c_petits/pilote_c.py)) | ok, verdict rendu | 815 s |

## Régime (a) : contrat FULL (100 ms à K5) — **non tenu**

Tableaux : [MES-FULL](resultats/cmd/001_mes_full/files/full/tableaux_full.md).

| Mesure, K5, appareil, 48 fils | Session M (8 oct., matin) | Session N (8 oct., soir) | Cette session |
| --- | ---: | ---: | ---: |
| ng00 / ng01 / ng02, médiane à chaud (ms) | 94,8 / 78,1 / 94,8 | 80,3 / 66,6 / 83,3 | **80,4 / 66,9 / 79,3** |
| 37 trames `v12set`, médiane (ms) | 160,6 | 142,4 | **116,6** |
| 37 trames `v12set`, maximum (ms) | 358,9 | 288,2 | **241,9** |
| trames du `v12set` sous 100 ms | — | 14 sur 37 | 15 sur 37 |

**Trame médiane** `kitti_ng_00_003624` (72 836 sites) :

| P | C | dont transferts | G | queue | total |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2,9 ms | 37,8 ms | 4,1 ms | 64,0 ms | 11,5 ms | 116,6 ms |

Il manque 17 ms pour le contrat.

- **G est désormais le premier poste.** B3 est en mesure sur ce produit.
- **Le catalogue C vient ensuite**, avec les leviers C1, C3 et C5 de l'étude du chantier C.
- **La queue tombe à 11,5 ms.**

**Trame maximale** `kitti_ng_00_001896` (95 586 sites) : 239,8 ms. K10 (ng00–02) : 493 / 366 / 423 ms. Bras CPU :
355 / 299 / 355 ms, dominé par le catalogue CPU.

## Régime (c) : petits nuages — **non tenu**

Tableaux : [MES-C](resultats/cmd/002_mes_c/files/c/tableaux_c.md).

| Critère | Valeur | Verdict |
| --- | --- | --- |
| C1 : ordonnée ≤ 2 ms (CPU, K5, 48 fils, 132 nuages réels) | 13,8 ms | non tenu |
| C2 : pente ≤ 3,73 µs par site (même configuration) | 9,56 µs par site | non tenu |
| C3 : cohorte difficile complète | quasi-sphère refusée `wide_leaf` (T1-c) | non tenu |

Voie appareil, K5, 48 fils, nuages réels : 5,1 ms + 1,96 µs par site. A6c n'agit pas ici, car ces nuages sont sous le
seuil de 43 900 sites.

## Ce que cette session n'établit pas

Ni un gain isolé de chaque levier : la comparaison entre sessions est descriptive. Ni le régime (b), mesuré dans les
sessions L1t et L2t, avant A6c. GCP utilisé pour cette seule session, arrêt certifié.
