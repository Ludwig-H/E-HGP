# Session G4 L1p : scènes LiDAR réelles entières sur le produit (`MES-B`, régime (b)), référence avant T1-d

8 octobre 2026. Session gardée `v12.20261008.mesb1p` (`gcp-migration/v12_session.py`, commit `c648b3857`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 10:50:07
à 11:05:12 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 11:05:49 UTC. Reçu sans identité
de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris ; K10 sur deux scènes)
quantification=quantized_u21_input_only (sites distincts au millimètre, D8)
public_status=not_claimed
```

**Produit mesuré** : la Session recouverte, voie par défaut, avec le cache de blocs de 8 Gio, le lot T2-d-C, le Pool à
équipe, T2-d-A et le bras « census » de T2-d-B. Le pilote est [`pilote_b.py`](../../microbancs/mes_b_scenes/pilote_b.py),
au schéma recouvert et avec le lecteur strict partagé. Cas, séries, budgets et verdicts B1 à B4 sont ceux de la
[session L1r](../g4_mesb1r_20261008/README.md) : 15 scènes **entières**, captations telles quelles, sans découpe ni
sous-échantillonnage. K5, 48 fils, budget de l'hôte 160 Gio, budget propre de l'appareil 88 Gio. Durée : 531 s.

## Verdict de `MES-B` : non tenu (B1, B2, B4 non tenus ; B3 non évalué)

| Scène entière | sites | voie | mur chaud L1r → L1p | s par million (L1p) | hôte, Ko par site | appareil gardé |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| Boreas, 1 trame, sans sol | 146 316 | appareil | 0,52 → **0,32 s** | 2,2 | 10,1 | 1,8 Gio |
| Boreas, 1 trame | 215 665 | appareil | 0,70 → **0,45 s** | 2,1 | 9,2 | 2,2 Gio |
| Boreas, 10 trames, sans sol | 1 513 483 | appareil | 10,0 → **7,3 s** | 4,8 | 14,1 | 21,5 Gio |
| Boreas, 10 trames, sans sol | 1 513 483 | CPU | 22,3 → 20,1 s | 13,3 | 17,1 | — |
| Boreas, 10 trames | 2 153 342 | appareil | 13,0 → **9,4 s** | 4,4 | 12,6 | 28,5 Gio |
| IGN Marseille, sans sol | 2 465 285 | appareil | 11,0 → **8,4 s** | 3,4 | 8,8 | 23,0 Gio |
| FOR-instance SCION 61, sans sol | 3 439 371 | appareil | 42,0 → **31,0 s** | 9,0 | 22,5 | 80,7 Gio |
| FOR-instance SCION 61 | 3 589 247 | appareil | 42,8 → 31,8 s (froides) | 8,9 | 22,1 | 81,2 Gio |
| ETH3D meadow, station 1 | 6 181 091 | appareil | 35,3 → **30,6 s** | 4,9 | 7,3 | 47,5 Gio |
| IGN Marseille | 6 709 045 | appareil | 31,6 → 26,6 s (froides) | 4,0 | 7,9 | 55,0 Gio |

**Refus inchangés (`resource_exhausted/memory_budget`, au budget de l'appareil, pics de 73 à 88 Gio).** FOR-instance
TUWIEN (5,20 et 6,24 M sites), NIBIO 12 (7,79 et 7,83 M), Boreas 50 trames (7,86 et 10,77 M ; le second, au-delà de
10 M, est toléré par B1), et K10 dès Boreas 10 trames sans sol (1,51 M) et IGN Marseille sans sol (2,47 M). Aucun
échec, aucune sortie illisible. Empreintes FUL1 identiques entre passes et entre les deux voies sur Boreas 10 trames
sans sol (`49f90d23…`, la même qu'en L1r).

## Lecture

D'une session à l'autre, les murs à chaud baissent de 13 à 39 % sur la voie appareil et de 10 % sur la voie CPU
(comparaison descriptive : plusieurs changements séparent L1r et L1p). Le régime (b) reste limité par la **mémoire de l'appareil**, qui refuse toute scène de plus de
5 millions de sites à K5 et dès 1,5 million à K10. C'est l'objet du catalogue en flux T1-d, en cours. Sur les scènes
calculées, la queue après G pèse lourd : 19,8 s sur ETH3D meadow et 16,6 s sur Marseille entière, contre 7,6 et 6,3 s
pour G (étages de la passe chaude dans [`tableaux_b.md`](resultats/cmd/000_mes_b/files/b/tableaux_b.md)). C'est le
noyau séquentiel de l'ordre 5, objet de A6. Hors du mur, l'empreinte FUL1 d'une scène de 1,5 M sites coûte 18 s sur
un seul fil.

## Ce que cette session n'établit pas

Ni le gain isolé de chaque changement depuis L1r, ni les scènes de plus de 5 M sites à K5, ni K10 au-delà d'un million
de sites. GCP utilisé pour cette seule session, arrêt certifié.
