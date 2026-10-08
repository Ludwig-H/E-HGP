# Session G4 L2, voie CPU : la v12 sur les grandes scènes LiDAR réelles entières (`MES-B`, régime (b))

8 octobre 2026. Session gardée `v12.20261008.mesb2` (`gcp-migration/v12_session.py`, commit `a2c2fccfd`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 05:35:28
à 05:53:58 UTC, **arrêt certifié `TERMINATED`**. Reçu sans identité de compte : [`receipt.json`](receipt.json) ;
sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (catalogue et tour sur l'hôte, 48 fils)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

La voie appareil refuse dès 5 M de sites faute de mémoire de la carte ([sessions L1](../g4_mesb1r_20261008/README.md)).
Cette session joue donc les grandes captations entières sur la **voie CPU**, une passe chacune, avec un budget de
l'hôte de 160 Gio. Elle mesure G, T, M, V, R et la mémoire de l'hôte au-delà de 10 M de sites. Les critères B1 à B4
portent sur la voie appareil : ils sont « non évalués » ici.

| Scène entière | sites | issue | mur | s par million | CPU·s | hôte (budget, pic) |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| IGN Paris, sans sol | 9 111 422 | ok | 112,9 s | 12,4 | 4 021 | 93,1 Gio |
| IGN Paris | 14 551 520 | ok | 165,8 s | 11,4 | 5 675 | 137,0 Gio |
| ETH3D courtyard, station 1 | 16 828 368 | refus `unsupported_degeneracy/wide_leaf` (66 s) | — | — | — | — |
| IGN Lyon, sans sol | 24 016 862 | refus `resource_exhausted/memory_budget` (145 s) | — | — | — | — |
| IGN Lyon | 32 412 887 | refus `resource_exhausted/memory_budget` (181 s) | — | — | — | — |

Étages, en secondes (une passe, froide) :

| Scène | P | C | G | T | M | V | R | validation (hors mur) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| IGN Paris sans sol | 0,65 | 67,68 | 14,90 | 20,30 | 2,12 | 1,21 | 5,13 | 20,88 |
| IGN Paris | 1,03 | 94,21 | 20,97 | 35,08 | 3,35 | 1,88 | 7,89 | 38,97 |

Mémoire du budget de l'hôte (Ko par site, en usage à la fin de l'étage / pic pendant l'étage) : Paris sans sol C
2,94 / **10,97**, G 4,97 / 5,89, tour complète 7,26 / 7,69 ; Paris C 2,63 / **10,11**, G 4,54 / 5,40, tour complète
6,73 / 7,13. Tableaux : [`tableaux_b.md`](resultats/cmd/000_mes_b/files/b/tableaux_b.md).

## Lecture

1. **La tour passe 14,6 M de sites réels en une passe**, sans refus ni échec, avec 137 Gio sur l'hôte. Le pic est celui
   de la construction du catalogue CPU (10 à 11 Ko par site, 3,7 à 3,8 fois sa taille finale) ; la tour complète occupe
   6,7 à 7,3 Ko par site. Les refus de Lyon (24 et 32 M) viennent de ce pic.
2. **Au-delà du catalogue**, à 14,6 M de sites : G 21 s (1,4 µs par site), T 35 s (2,4 µs par site, contre 0,8 vers
   40 000 sites), R 7,9 s. Avec un catalogue sur l'appareil en flux, au coût mesuré en L1 sur l'IGN (0,6 à 0,7 s par
   million), cette scène coûterait environ 80 s, soit 5,5 s par million : G et T doivent encore gagner un facteur 3
   pour l'objectif de 2 s par million.
3. **ETH3D courtyard refuse `wide_leaf`** : près de la station du scanner, la densité sous-millimétrique met plus de 256
   sites dans une feuille. Ce n'est donc pas seulement une affaire de nuages synthétiques dégénérés : la voie large
   (T1-c, `CST-0237`) est nécessaire pour des captations réelles telles quelles.
4. La validation hors du mur coûte 21 à 39 s ; l'empreinte n'est pas calculée au-delà de 1,6 M de sites.

## Ce que cette session n'établit pas

Ni la voie appareil, ni l'identité FUL1 au-delà de 1,6 M de sites, ni une mesure à chaud (une passe). Aucun objectif du
régime (b) n'est tenu. GCP utilisé pour cette seule session, arrêt certifié.
