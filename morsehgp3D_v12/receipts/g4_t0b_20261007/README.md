# Session G4 v12.20261007.t0b : MES-M3 et MES-M4 (tour)

7 octobre 2026, 11 h 27 à 11 h 47 UTC. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à
`5bd963078`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, sans construction
par défaut. VM démarrée à 11:27:33 UTC, **arrêt certifié `TERMINATED` à 11:46:47 UTC**. Reçu sans identité de compte :
[`receipt.json`](receipt.json) ; résultats choisis sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `1d3429b898edc66b…` |
| plan | `d4843d6a8aacc677…` |
| données (mêmes 14 fichiers que la session A) | manifeste `fb6b79d2cd45f691…` |
| résultats rapatriés | `7391a9f1cdbbe405…` |

| Commande | État | Durée |
| --- | --- | ---: |
| `source_v11` (sources v11 épinglées, `libmhgp11.a`) | ok | 5 s |
| `m34_k5` : pilote, ng00–02 à K5, 5 processus, 48 fils (vidage, portes, MES-M3, variante, MES-M4) | ok | 139 s |
| `m34_k10` : ng00–02 à K10, 3 processus | ok | 792 s |
| publications (vidages exclus) | ok | 0 s |

Les vidages réécrits au format `MHGP11FUL1` sont identiques à l'octet aux empreintes de référence ; les graines rejouées
sont identiques au journal de la v11 ; les portes de MES-M3 et MES-M4 sont conformes et leurs mutants tués
(`resultats/.../tableaux.md`, section « Portes et mutants »).

## MES-M3 : plus petite boule proposée puis certifiée

Règle ([`PLAN.md`](../../docs/PLAN.md), T0) : résolution à K10 réduite d'au moins 40 % à un fil, résultats identiques.
**Verdict : adoptée.**

| Cas | Parties | Certifiées par `LEM-T1` | Plus petite boule v12 / référence | Résolution à un fil, tous ordres : réplique v12 / réplique v11 | Ordre 10 seul |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 K10 | 8 856 134 | 76,1 % | 0,315 | **0,542** (24,0 s → 13,0 s) | 0,447 |
| ng01 K10 | 6 557 526 | 76,1 % | 0,311 | **0,541** (16,9 s → 9,1 s) | 0,444 |
| ng02 K10 | 7 148 742 | 79,5 % | 0,317 | **0,547** (18,8 s → 10,3 s) | 0,437 |
| ng00 K5 | 1 175 034 | 75,2 % | 0,972 | 0,934 | — |
| ng01 K5 | 911 687 | 76,2 % | 0,948 | 0,925 | — |
| ng02 K5 | 1 039 136 | 80,0 % | 0,977 | 0,924 | — |

La part certifiée avec le test complet ($S\subseteq F\subseteq P_b$) égale la part des sphères au catalogue à moins de
quelques dizaines de parties par cas. À K5, le gain est faible (7 %) : le recensement domine les pas.

## MES-M4 : forêt sans lots

Règle : noyau au plus 10 ms à K5 et 35 ms à K10 à un fil ; contraction au plus 3 ms ; forêts identiques. Forêts
identiques à la v11 sur tous les ordres ; `LEM-T6` sans écart. Temps à un fil (ms), contraction parallèle à 48 fils :

| Cas, ordre | Naissances | Noyau | Contraction séquentielle | Contraction parallèle |
| --- | ---: | ---: | ---: | ---: |
| ng00, ordre 5 | 341 081 | 8,78 | 7,14 | 1,70 |
| ng01, ordre 5 | 283 207 | 7,21 | 5,80 | 1,47 |
| ng02, ordre 5 | 361 326 | 9,46 | 7,91 | 1,78 |
| ng00, ordre 10 | 979 350 | 26,40 | 21,73 | **4,09** |
| ng01, ordre 10 | 758 514 | 19,66 | 16,30 | **3,14** |
| ng02, ordre 10 | 937 412 | 24,55 | 21,09 | **3,95** |

**Verdict : tenue à K5 sur tous les critères ; à K10, noyau conforme, mais contraction de 3,1 à 4,1 ms au-dessus du
seuil de 3 ms.** La règle, écrite sans préciser K, n'est donc pas tenue à K10 ; elle n'est pas réécrite. La forêt sans
lots est adoptée pour K5, qui porte le contrat de 100 ms ; à K10, la contraction reste à réduire, et l'écart est publié.
Les ordres sont indépendants jusqu'aux verticales : le temps mural de la forêt suit l'ordre le plus coûteux, et non la
somme des ordres (24,2 ms de noyau sur les ordres 1 à 5 de ng00, à un fil).

## Ce que cette session n'établit pas

Aucun temps du chemin produit : ce sont des microbancs hors produit, à un fil sauf la contraction parallèle. `MES-M7`
(profil par route contre la v10 R2) n'est couvert qu'en partie : le profil des descentes de la v11 et la résolution à
trois bras sont dans `tableaux.md`, sans la v10. GCP utilisé pour cette seule session, arrêt certifié.
