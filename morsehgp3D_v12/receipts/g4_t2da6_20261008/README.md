# Sessions G4 A6 : la chaîne de l'ordre K de la Session recouverte — rejetée, retirée du produit

8 octobre 2026. Session décisive gardée `v12.20261008.t2da6b` (`gcp-migration/v12_session.py`, commit `6497ed3b5`,
preuve `pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
12:59:55 à 13:28:00 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 13:28:38 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Levier jugé.** A6, intégré sur `main` avant la mesure (`30a69104a`, mutants corrigés `fd84039c0`), puis ponté
(`6497ed3b5`, publication release/acquire des indices, `CST-0242`) : numérotation par morceaux de cohorte (N), indices
de racine calculés par une tâche d'aide, noyau inchangé (I), historique parallèle (H). Pilote
[`pilote_t2d_a6.py`](../../microbancs/mes_t2d_a6/pilote_t2d_a6.py), juge fermé de l'auditeur ; bras avant = archive
de `bdfca8fb1` (produit identique à `72f622a55`), avant_bis (A/A), après = paquet ; cache de blocs de 8 Gio dans les
deux bras ; K5, appareil, 48 fils.

| Commande | État | Durée |
| --- | --- | ---: |
| `archive_t2da6`, `archive_t2da6r` (archives des deux sessions précédentes, restées sur la VM) | ok | 0,02 s |
| `socle_ctest` (`ctest -LE long`) | ok, 734 portes sur 734 | 142 s |
| `t2da6_pilote` | ok, verdict rendu, aucun refus | 377 s |
| `mutants_tour` | ok, 48 mutants de la tour tués (plancher 48) | 120 s |
| `lidar_ctest` (`MES-M0` sémantique compris) | ok, 7 sur 7 | 764 s |

## Verdict de `REGLE_T2D_A6` (écrite à 09:11 UTC, avant toute mesure) : **rejeté**

Identité FUL1 établie. Rapports après / avant (moyenne géométrique, IC 95 % par bootstrap sur les tours) :

| Cohorte | Tours | Rapport | IC 95 % | Seuil | A/A |
| --- | ---: | ---: | --- | ---: | ---: |
| 21 grandes trames du `v12set` (plus de 60 000 sites) | 6 | **0,914** | 0,910 – 0,917 | < 0,97 | 0,999 |
| ng00 | 5 | 1,033 | 1,031 – 1,035 | < 1,02 | 0,998 |
| ng01 | 5 | 1,043 | 1,040 – 1,047 | < 1,02 | 0,999 |
| ng02 | 5 | 1,012 | 1,009 – 1,015 | < 1,02 | 0,998 |

A6 gagne 8,6 % sur les grandes trames, mais ralentit ng00 et ng01 au-delà du seuil. La règle le rejette.

**Grandes trames** ([tableaux](resultats/cmd/003_t2da6_pilote/files/t2da6/tableaux_t2d_a6.md)) :

- la queue de la Session (tour sans G) passe de 27–72 ms à 12–20 ms ;
- l'écart entre la fin du noyau de l'ordre 5 et celle de G(5) passe de 39–96 ms à 0,1–18 ms ;
- trame médiane `kitti_ng_02_001606` : 149,9 → 138,0 ms ; trame maximale `kitti_ng_08_002119` : 290,2 → 265,0 ms.

**Petites trames** (médianes des tours ; passes 2 à 10 ; ms, sauf le CPU) :

| Trame | Mur avant → après | Queue | CPU par passe | Fins de l'ordre 5 avant (G, noyau, M, V, R) | Après |
| --- | --- | --- | --- | --- | --- |
| ng00 | 87,5 → 90,4 | 5,7 → 6,7 | 2 571 → 2 670 | 32,5 ; 40,7 ; 43,1 ; 52,0 ; 52,6 | 33,0 ; 38,8 ; 41,5 ; 53,5 ; 46,8 |
| ng01 | 71,8 → 75,0 | 7,7 → 7,9 | 1 975 → 2 064 | 25,7 ; 32,8 ; 35,0 ; 41,1 ; 42,7 | 26,1 ; 32,6 ; 34,8 ; 43,8 ; 38,9 |
| ng02 | 87,9 → 89,1 | 14,0 → 9,7 | 2 424 → 2 550 | 29,7 ; 47,5 ; 49,9 ; 51,8 ; 58,6 | 31,4 ; 40,9 ; 43,4 ; 53,7 ; 49,0 |

Sur ces trames, le noyau et le registre de l'ordre 5 finissent plus tôt. En revanche, le CPU par passe augmente de
4 à 5 %, la fin des verticales recule de 1,5 à 2,7 ms et le mur s'allonge. La chaîne de l'ordre K coûte donc plus
qu'elle ne rend quand G est le chemin critique. Le diagnostic (lequel de N, I, H) est confié au chantier A, pour une
A6b qui garderait le gain des grandes trames sans cette perte, avec une règle nouvelle écrite d'avance.

## Les deux sessions précédentes, rapatriées ici

- **`v12.20261008.t2da6`** (commit `30a69104a`, VM de 11:54:59 à 12:22:51 UTC, arrêt certifié) : première mesure,
  **avant le pont** de publication. Même verdict, rejeté : grandes 0,911 (0,906 – 0,915), ng00 1,040, ng01 1,037,
  ng02 1,000. Le pont ne change donc pas les temps
  ([tableaux](premiere_mesure_t2da6/tableaux_t2d_a6.md), [commandes](premiere_mesure_t2da6/commands_t2da6.tsv)).
  La campagne de mutants avait échoué (code 8) : deux mutants A6 ne compilaient pas sous `-Werror`. Ils ont été
  remplacés dans `fd84039c0`. Le rapatriement avait été refusé faute de place locale ; l'archive (SHA-256
  `1ee399cd…5eecb`, la valeur que l'auditeur avait relevée) est récupérée par `archive_t2da6`.
- **`v12.20261008.t2da6r`** (commit `fd84039c0`, VM de 12:27:29 à 12:33:44 UTC, arrêt certifié) : campagne de mutants
  de la tour, 48 tués ([commandes](premiere_mesure_t2da6/commands_t2da6r.tsv)). Le rapatriement avait été refusé de
  12 Mo. L'archive (SHA-256 `786a3a2c…d232`) est récupérée par `archive_t2da6r`.

## Conséquence : A6 retiré de `main`

Les fichiers de `src/tower/`, `tests/tower/` et `tests/mutants/tower.json` touchés par `30a69104a`, `fd84039c0` et
`6497ed3b5` reviennent à leur état de `bdfca8fb1` (plancher des mutants de la tour : 40). Le pilote
`microbancs/mes_t2d_a6/` reste comme banc. Le pont `CST-0242` n'a plus d'objet dans le produit ; il reste acquis pour
une A6b qui reprendrait la tâche d'aide.

## Ce que ces sessions n'établissent pas

Ni le contrat FULL, ni le gain isolé de N, I ou H. GCP utilisé pour ces trois sessions seulement, arrêts certifiés.
