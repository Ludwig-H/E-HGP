# Session G4 L1o : scènes LiDAR réelles entières sur le produit du 10 octobre (`MES-B`, régime (b))

10 octobre 2026. Session gardée `v12.20261010.mesb1o` (`gcp-migration/v12_session.py`, commit `ae8f8107c`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
20:09:43 à 20:29:16 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 20:29:39 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris ; K10 sur deux scènes)
quantification=quantized_u21_input_only (sites distincts au millimètre, D8)
public_status=not_claimed
```

**Produit mesuré.** La Session recouverte avec le cache de blocs, le lot B2, T1-d, R1, A6c et B3-K. Les sources du
produit sont celles de `2aaed1847` ; les commits suivants ne portent que des notes de l'auditeur.

- **Données.** Ce sont les 15 scènes **entières** des sessions L1, L1r, L1p et L1t, captations telles quelles, sans
  découpe ni sous-échantillonnage. Le paquet a été régénéré le 10 octobre par les mêmes outils : 15 cas,
  45 fichiers, 1 440,4 Mo, la même taille qu'en L1t.
- **Pilote** : [`pilote_b.py`](../../microbancs/mes_b_scenes/pilote_b.py).
- **Paramètres** : K5, 48 fils, budget de l'hôte 160 Gio, budget propre de l'appareil 88 Gio.

| Commande | État | Durée |
| --- | --- | ---: |
| `mes_b_flux_identite` (Boreas 10 trames sans sol, appareil sous 8 Gio puis CPU) | ok | 108 s |
| `mes_b_l1` (17 cas) | ok, 12 calculés, 5 refus `memory_budget` | 744 s |

**Identité.** Sur Boreas 10 trames sans sol, la voie en flux (8 Gio d'appareil, pic 5,0 Gio) et la voie CPU rendent
la même empreinte FUL1 `49f90d23…`, celle de toutes les sessions depuis L1. Mur à chaud de la voie en flux : 8,20 s
en L1t, **6,28 s** ici.

## Scènes entières : L1t (8 octobre) → L1o (10 octobre)

Comparaison descriptive : plusieurs changements séparent les deux sessions (A6c, B3-K). Mur à chaud, sinon à froid
quand une seule passe est jouée. Queue = forêt après le dernier calcul de G.
[Tableaux](resultats/cmd/001_mes_b_l1/files/b/tableaux_b.md).

| Scène entière | sites | K | mur L1t → L1o | queue L1t → L1o |
| --- | ---: | ---: | --- | --- |
| Boreas, 1 trame, sans sol | 146 316 | 5 | 0,31 → **0,26 s** | 0,08 → 0,03 s |
| Boreas, 1 trame | 215 665 | 5 | 0,44 → **0,35 s** | 0,13 → 0,04 s |
| Boreas, 10 trames, sans sol | 1 513 483 | 5 | 7,18 → **4,89 s** | 3,03 → 0,79 s |
| Boreas, 10 trames | 2 153 342 | 5 | 8,83 → **6,23 s** | 3,64 → 1,10 s |
| IGN Marseille, sans sol | 2 465 285 | 5 | 8,09 → **5,17 s** | 3,98 → 0,99 s |
| FOR-instance SCION 61, sans sol | 3 439 371 | 5 | 36,22 → **28,55 s** | 10,38 → 3,33 s |
| FOR-instance SCION 61 | 3 589 247 | 5 | 30,37 → **21,66 s** (froid) | 10,81 → 3,22 s |
| ETH3D meadow, station 1 | 6 181 091 | 5 | 29,62 → **13,50 s** | 18,87 → 1,93 s |
| FOR-instance TUWIEN | 6 236 167 | 5 | 45,54 → **36,11 s** (froid) | 12,94 → 3,96 s |
| IGN Marseille | 6 709 045 | 5 | 25,08 → **13,54 s** (froid) | 15,21 → 2,96 s |
| Boreas, 10 trames, sans sol | 1 513 483 | 10 | 38,62 → **36,25 s** (froid) | 1,15 → 0,99 s |
| IGN Marseille, sans sol | 2 465 285 | 10 | 38,52 → **36,34 s** (froid) | 1,67 → 1,42 s |

**Ce qui progresse.** À l'échelle du million de sites, la queue après G s'effondre : de 15 à 19 s à 2 à 3 s sur ETH3D
et Marseille entière. Ces deux scènes de 6,2 et 6,7 M sites passent sous 14 s. Le temps restant est d'abord G (6 à
17 s), puis C (3 à 15 s sur les forêts denses FOR-instance). À K10, C (13 s) et G (21 s) dominent.

**Refus inchangés** (`resource_exhausted/memory_budget`) :

- TUWIEN sans sol (5,20 M) : passe 0 réussie en 24,8 s, passe 1 refusée ; c'est le défaut de seconde passe
  `CST-0243` ;
- NIBIO 12, entière et sans sol (7,79 et 7,83 M) ;
- Boreas 50 trames, entière et sans sol (7,86 et 10,77 M).

Au-delà de 7 à 8 M sites, la mémoire de l'hôte pendant la tour limite le régime. Les correctifs de l'agent du
chantier C sont en cours : seconde passe, et repli de la Session recouverte quand son admission refuse.

Verdicts `MES-B` : B1, B2 et B4 non tenus (B2 : des scènes de moins de 10 M de sites refusent encore), B3 non évalué.

## Ce que cette session n'établit pas

Ni le gain isolé d'A6c ou de B3-K sur ces scènes, ni les scènes de plus de 10 M de sites (L2), ni une correction des
refus. GCP utilisé pour cette seule session, arrêt certifié.
