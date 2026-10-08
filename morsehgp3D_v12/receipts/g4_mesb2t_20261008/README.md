# Session G4 L2t : les plus grandes scènes entières sur la voie appareil avec T1-d (`MES-B`, régime (b))

8 octobre 2026. Session gardée `v12.20261008.mesb2t` (`gcp-migration/v12_session.py`, commit `8b9eab40a`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
15:44:49 à 15:55:16 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 15:55:57 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only (sites distincts au millimètre, D8)
public_status=not_claimed
```

**Produit mesuré** : la Session recouverte, voie par défaut, avec le cache de blocs, le lot B2, T1-d et R1, sur la voie
appareil (budget propre de l'appareil 88 Gio, hôte 160 Gio), une passe par scène. Les scènes sont **entières** : ce
sont les grandes captations de la [session L2](../g4_mesb2_20261008/README.md), jouée ce matin sur la voie CPU. ETH3D
courtyard est omise : elle refuse `wide_leaf` sur les deux voies, et relève de T1-c.

| Commande | État | Durée |
| --- | --- | ---: |
| `mes_b_l2` | ok, 1 calculée, 3 refus `memory_budget` | 217 s |

| Scène entière | sites | état | mur (froid) | s par million | appareil gardé / pic | hôte (budget, pic) |
| --- | ---: | --- | ---: | ---: | --- | ---: |
| IGN Paris, sans sol | 9 111 422 | **ok** | **46,5 s** | 5,1 | 0,1 / 45,6 Gio (voie en flux) | 82,1 Gio |
| IGN Paris | 14 551 520 | refus `memory_budget` | — | — | — / 46,9 Gio | — |
| IGN Lyon, sans sol | 24 016 862 | refus `memory_budget` | — | — | — / 65,8 Gio | — |
| IGN Lyon | 32 412 887 | refus `memory_budget` | — | — | — / 71,1 Gio | — |

Étages de Paris sans sol, en secondes ([tableaux](resultats/cmd/000_mes_b_l2/files/b/tableaux_b.md)) :

| P | C | dont transferts | G | queue |
| ---: | ---: | ---: | ---: | ---: |
| 0,7 | 13,9 | 2,4 | 14,3 | 17,4 |

**Lecture.**

- **Gain sur Paris sans sol.** Le catalogue en flux sur l'appareil calcule la scène en 46,5 s, contre 112,9 s ce
  matin sur la voie CPU (×2,4). La queue après G (17,4 s) est le premier poste, objet de A6b.
- **Régression sur Paris entière.** Ce matin, la voie CPU (sonde séquentielle de l'époque) la calculait sous le même
  budget de 160 Gio, en 165,8 s, avec un pic de l'hôte de 137,0 Gio. Ici, elle est refusée, avec un pic de
  l'appareil de 46,9 Gio seulement : c'est donc le budget de l'hôte qui refuse, plus tôt. Trois explications
  possibles :
  - la Session recouverte, voie par défaut depuis `86d7e39d8`, admettrait plus que son pic ; c'est un constat de
    l'agent du chantier C ;
  - le cache de blocs de 8 Gio entre au budget ;
  - les tableaux de la voie en flux s'ajoutent sur l'hôte.

  Le diagnostic est confié au chantier C, avec le refus de seconde passe de la [session L1t](../g4_mesb1t_20261008/README.md).
- **Lyon** refuse comme ce matin sur la voie CPU (au-delà de 10 M de sites, refus toléré par B1). La limite du régime
  (b) est désormais la mémoire de l'hôte pendant la tour, pas la carte : 8 à 22 Ko par site à K5.

Verdicts `MES-B` : B1 non tenu, B2 tenu (une scène jouée sous 10 M de sites), B3 et B4 non évalués.

## Ce que cette session n'établit pas

Ni la cause exacte des refus, ni un temps chaud, puisque chaque scène n'a qu'une passe. GCP utilisé pour cette seule
session, arrêt certifié.
