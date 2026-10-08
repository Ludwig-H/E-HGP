# Session G4 L1t : scènes LiDAR réelles entières sur le produit avec T1-d (`MES-B`, régime (b))

8 octobre 2026. Session gardée `v12.20261008.mesb1t` (`gcp-migration/v12_session.py`, commit `caf9585e4`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
15:19:33 à 15:41:54 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 15:42:20 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris ; K10 sur deux scènes)
quantification=quantized_u21_input_only (sites distincts au millimètre, D8)
public_status=not_claimed
```

**Produit mesuré** : la Session recouverte, voie par défaut, avec le cache de blocs, le lot B2, T1-d et R1. Plans de
l'agent du chantier C, pilote [`pilote_b.py`](../../microbancs/mes_b_scenes/pilote_b.py). Les scènes sont **entières**,
captations telles quelles, sans découpe ni sous-échantillonnage ; ce sont les 15 scènes des sessions L1r et L1p. K5,
48 fils, budget de l'hôte 160 Gio.

| Commande | État | Durée |
| --- | --- | ---: |
| `mes_b_flux_identite` (Boreas 10 trames sans sol, appareil sous 8 Gio puis CPU) | ok | 113 s |
| `mes_b_l1` (17 cas, budget propre de l'appareil 88 Gio) | ok, 12 calculés, 5 refus `memory_budget` | 839 s |

## Identité de la voie en flux

Boreas 10 trames sans sol (1 513 483 sites) passe sous un budget de l'appareil de 8 Gio, contre 21,5 Gio gardés par la
voie complète. Le pic de l'appareil tombe à 5,0 Gio et le mur chaud vaut 8,2 s, contre 7,2 s pour la voie complète. Son
empreinte FUL1 `49f90d23…` est celle de la voie complète (sessions L1, L1r et L1p) et de la voie CPU jouée ici
(19,5 s) : l'identité est établie entre passes et entre voies.

## Scènes entières ([tableaux](resultats/cmd/001_mes_b_l1/files/b/tableaux_b.md))

| Scène entière | sites | K | état | mur chaud (s) ou froid | s par million | pic appareil (Gio) |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| Boreas, 1 trame, sans sol | 146 316 | 5 | ok | 0,31 | 2,1 | 2,4 |
| Boreas, 1 trame | 215 665 | 5 | ok | 0,44 | 2,0 | 2,8 |
| Boreas, 10 trames, sans sol | 1 513 483 | 5 | ok | 7,18 | 4,7 | 22,0 |
| Boreas, 10 trames | 2 153 342 | 5 | ok | 8,83 | 4,1 | 29,1 |
| IGN Marseille, sans sol | 2 465 285 | 5 | ok | 8,09 | 3,3 | 23,6 |
| FOR-instance SCION 61, sans sol | 3 439 371 | 5 | ok | 36,22 | 10,5 | 81,3 |
| FOR-instance SCION 61 | 3 589 247 | 5 | ok | 30,37 (froid) | 8,5 | 81,8 |
| FOR-instance TUWIEN, sans sol | 5 199 758 | 5 | **passe 0 ok (32,9 s), passe 1 refusée** | — | 6,3 | 87,6 |
| ETH3D meadow, station 1 | 6 181 091 | 5 | ok | 29,62 | 4,8 | 48,1 |
| FOR-instance TUWIEN | 6 236 167 | 5 | **ok (nouveau)** | 45,54 (froid) | 7,3 | 45,5 |
| IGN Marseille | 6 709 045 | 5 | ok | 25,08 (froid) | 3,7 | 55,6 |
| NIBIO 12, sans sol et entière | 7,79 et 7,83 M | 5 | refus `memory_budget` | — | — | 46,3 |
| Boreas, 50 trames, sans sol et entière | 7,86 et 10,77 M | 5 | refus `memory_budget` | — | — | 46,0 et 49,3 |
| Boreas, 10 trames, sans sol | 1 513 483 | 10 | **ok (nouveau)** | 38,62 (froid) | 25,5 | 45,8 |
| IGN Marseille, sans sol | 2 465 285 | 10 | **ok (nouveau)** | 38,52 (froid) | 15,6 | 45,8 |

**Ce que T1-d change.** Dans la [session L1p](../g4_mesb1p_20261008/README.md), la mémoire de l'appareil refusait :

- toute scène de plus de 5 M de sites à K5 ;
- K10 dès 1,5 M de sites.

Ici, plusieurs de ces cas passent :

- TUWIEN entière (6,24 M), par la voie en flux, avec 0,1 Gio gardé sur l'appareil ;
- K10 sur Boreas 10 trames sans sol (1,51 M) ;
- K10 sur Marseille sans sol (2,47 M).

Les refus de NIBIO 12 et de Boreas 50 trames tombent en passe 0 avec un pic de l'appareil de 46 à 49 Gio : ce n'est
plus la carte qui limite. Sur ces scènes denses, la tour coûte 15 à 22 Ko par site à l'hôte, ce qui dépasse le budget
de 160 Gio vers 7 à 10 M de sites. C'est l'hypothèse retenue, à confirmer : les journaux ne nomment pas le budget du
refus.

**Défaut ouvert.** TUWIEN sans sol réussit sa passe 0 par la voie complète, en gardant 80 Gio de l'appareil. Puis la
passe 1 de la même Session est refusée (`memory_budget`). Une Session ne doit pas refuser au second passage une trame
qu'elle vient de calculer. Le défaut est confié au chantier C : diagnostic, fixture minimale, correctif, porte et
mutant.

**Temps.** À l'échelle du million de sites, la queue après G reste lourde : 18,9 s sur ETH3D meadow, 15,2 s sur
Marseille entière. Elle est l'objet de A6b. Verdicts `MES-B` : B1, B2 et B4 non tenus, B3 non évalué.

## Ce que cette session n'établit pas

Ni la cause exacte des refus restants, ni les scènes au-delà de 10 M de sites (session L2 à suivre), ni un gain isolé
de T1-d sur les temps. GCP utilisé pour cette seule session, arrêt certifié.
