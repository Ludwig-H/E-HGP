# Session G4 N : état du contrat FULL et des petits nuages sur le produit adopté (`MES-FULL`, `MES-C`)

8 octobre 2026. Session gardée `v12.20261008.fulln` (`gcp-migration/v12_session.py`, commit `8a0716e74`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
17:55:35 à 18:25:06 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 18:25:33 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris ; K10 sur ng00–02)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Produit mesuré.** La Session recouverte, voie par défaut, avec le cache de blocs. S'y ajoutent les leviers adoptés
aujourd'hui sur G4 : le lot B2, T1-d et R1. A6 et A6b, rejetés, sont retirés.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok | 142 s |
| `mes_full` ([`pilote_full.py`](../../microbancs/mes_full/pilote_full.py)) | ok, verdict rendu, aucun refus | 530 s |
| `mes_c` ([`pilote_c.py`](../../microbancs/mes_c_petits/pilote_c.py)) | ok, verdict rendu | 813 s |

## Régime (a) : contrat FULL (100 ms à K5) — **non tenu**

Tableaux : [MES-FULL](resultats/cmd/001_mes_full/files/full/tableaux_full.md).

| Mesure, K5, appareil, 48 fils | Session M (matin) | Cette session |
| --- | ---: | ---: |
| ng00 / ng01 / ng02, médiane à chaud | 94,8 / 78,1 / 94,8 ms | **80,3 / 66,6 / 83,3 ms** |
| 37 trames `v12set`, médiane | 160,6 ms | **142,4 ms** |
| 37 trames `v12set`, maximum | 358,9 ms | **288,2 ms** |
| trames du `v12set` sous 100 ms | — | 14 sur 37 |

- **ng00–02 tiennent les 100 ms.**
- **Trame médiane** `kitti_ng_00_003624` (72 836 sites) : P 2,9, C 38,1 (dont transferts 4,2), G 59,5 et queue
  41,7 ms. La queue après G, chaîne de l'ordre 5 comprise, reste le premier levier : c'est l'objet d'A6c. G vient
  ensuite, avec B3 (LEM-T1) en mesure.
- **Trame maximale** `kitti_ng_00_001896` (95 586 sites) : C 64,5, G 144,9 et queue 73,1 ms.
- **K10** (ng00–02) : 495 / 366 / 422 ms, contre 594 / 444 / 505 ms dans la session M.
- **Voie CPU** (bras identifié) : 354 / 299 / 361 ms ; le catalogue CPU (255 à 303 ms) y domine.

## Régime (c) : petits nuages — **non tenu**

Tableaux : [MES-C](resultats/cmd/002_mes_c/files/c/tableaux_c.md).

| Critère | Valeur | Verdict |
| --- | --- | --- |
| C1 : ordonnée ≤ 2 ms (CPU, K5, 48 fils, 132 nuages réels) | 14,3 ms | non tenu |
| C2 : pente ≤ 3,73 µs par site (même configuration) | 9,70 µs par site | non tenu |
| C3 : cohorte difficile complète | quasi-sphère refusée `wide_leaf` (T1-c) | non tenu |

La voie appareil reste la meilleure : K5 à 48 fils, 5,1 ms + 1,98 µs par site sur les nuages réels (contre 5,39 ms +
2,18 µs dans la session C3).

## Ce que cette session n'établit pas

Ni un gain isolé de chaque levier depuis la session M (plusieurs changements les séparent : comparaison descriptive),
ni le régime (b), mesuré dans les [sessions L1t](../g4_mesb1t_20261008/README.md) et
[L2t](../g4_mesb2t_20261008/README.md). GCP utilisé pour cette seule session, arrêt certifié.
