# Session G4 v12.20261007.t0d : MES-M3 répliquée et MES-M4 avec ses preuves

7 octobre 2026, 13 h 23 à 14 h 01 UTC. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à
`bf70e8b99`, juges et pilotes durcis de `320db4a12`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`,
`--max-run-seconds 4200`, sans construction par défaut. VM démarrée à 13:24:32 UTC, **arrêt certifié `TERMINATED` à
14:00:29 UTC**. Reçu sans identité de compte : [`receipt.json`](receipt.json) ; résultats choisis sous `resultats/` ;
empreintes : `SHA256SUMS` ; journaux bruts laissés dans le dossier de session local.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `9a592e8844162440…` |
| plan | `479c1ff57b0d7cdc…` |
| données (mêmes 14 fichiers que les sessions précédentes) | manifeste `fb6b79d2cd45f691…` |
| résultats rapatriés | `676a51e5a663be10…` |

| Commande | État | Durée |
| --- | --- | ---: |
| `source_v11` | ok | 5 s |
| `m34_k10_base` : construction, portes, vidages, MES-M4 à K10, 5 processus | ok | 100 s |
| `m34_k10_resolution_ng00`, `ng01`, `ng02` : 5 processus neufs par trame, une passe chacun | ok | 375, 268, 302 s |
| `m34_k10_m3` : microbanc MES-M3 à K10, 3 processus | ok | 552 s |
| `m34_k5` : tout le pilote à K5, 5 processus | ok | 319 s |
| publications (vidages exclus) | ok | 0 s |

## MES-M3 : adoptée définitivement

Juge durci (`CST-0213`) : moyenne géométrique des rapports réplique v12 sur réplique v11 par processus, IC 95 % par
bootstrap, seuil 0,60 (au moins 40 % de réduction) sur ng00–02 à K10, cinq prises par cas exigées. **Verdict :
adoptée.**

| Trame, K10 | Prises | Moyenne géométrique | IC 95 % | Réduction |
| --- | ---: | ---: | --- | ---: |
| ng00 | 5 | 0,5450 | [0,5444 ; 0,5457] | 45,5 % |
| ng01 | 5 | 0,5388 | [0,5378 ; 0,5397] | 46,1 % |
| ng02 | 5 | 0,5476 | [0,5464 ; 0,5489] | 45,2 % |

La prise unique de la session B (0,542, 0,541, 0,547) est confirmée à moins d'un point. Le rapport du lot K5 rend
« refusé » pour M3, comme prévu : la règle ne décide qu'à K10.

## MES-M4 : conforme, preuves comprises

Juge durci (`CST-0214`) : forêts identiques à la v11, `LEM-T6` sur toutes les naissances des ordres 2 à K, mutant tué,
porte conforme, dans chacun des cinq processus. Temps (médianes des processus, millisecondes ; noyau à un fil,
contraction parallèle à 48 fils) :

| Trame, ordre | Noyau | Seuil | Contraction parallèle | Seuil |
| --- | ---: | ---: | ---: | ---: |
| ng00, ordre 5 | 8,85 | 10 | 1,75 | 3 |
| ng01, ordre 5 | 7,23 | 10 | 1,54 | 3 |
| ng02, ordre 5 | 9,50 | 10 | 1,96 | 3 |
| ng00, ordre 10 | 26,61 | 35 | **4,25** | 3 |
| ng01, ordre 10 | 19,87 | 35 | **3,24** | 3 |
| ng02, ordre 10 | 24,68 | 35 | **4,10** | 3 |

Même lecture qu'en session B : règle tenue à K5 ; à K10, contraction au-dessus du seuil, publiée sans réécrire la règle.
La forêt sans lots est adoptée pour K5 ; la contraction à K10 reste un objectif.

GCP utilisé pour cette seule session, arrêt certifié.
