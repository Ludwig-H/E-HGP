# Session G4 T2-d-A : Session recouverte (D-F2) — adopté ; ng00–02 sous 100 ms

8 octobre 2026. Session gardée `v12.20261008.t2da` (`gcp-migration/v12_session.py`, commit `5f5c0c83f`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 07:27:16
à 07:43:08 UTC, **arrêt certifié `TERMINATED`**. Reçu sans identité de compte : [`receipt.json`](receipt.json) ;
sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Bras.** Le bras « avant » est l'archive de `27eca166b` (SHA-256 `929c3744…`), c'est-à-dire `main` juste avant
l'intégration du patch T2-d-A, avec le lot T2-d-C et le Pool à équipe. Le bras « après » est le produit joué
(`5f5c0c83f`), sonde lancée avec `--recouvert`. Le développeur a choisi ce bras « avant » **avant la session**, à la
place de l'archive de `902041f66` prévue par l'agent, pour que les deux bras ne diffèrent que par T2-d-A : même Pool,
même catalogue, même SHA-256. La règle `REGLE_T2D_A` est inchangée.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (construction par défaut, `ctest -LE long`) | ok, 719 portes sur 719 | 141 s |
| `t2da_pilote` | ok, verdict « adopté », aucun refus | 298 s |
| `lidar_ctest` | **expiré à 180 s** : six portes LiDAR passées, la septième (`mhgp12_tower_chain_m0`, différentiel `MES-M0` contre la v11) encore en cours au délai du plan | 180 s |
| `mutants_tour` | ok, `mhgp12_mutants_tower` (37 mutants, plancher 37) | 97 s |

L'expiration de `lidar_ctest` vient du délai du plan, trop court pour `MES-M0`, que la construction par défaut joue sans
parallélisme interne. Elle met la session en `failed_remote` ; elle ne touche pas la décision du pilote, qui contrôle
lui-même l'identité FUL1. `MES-M0` reste à rejouer avec un délai suffisant.

## Verdict de `REGLE_T2D_A` : adopté

Mur FULL K5 (médiane des passes 2 à 10, 5 tours, processus alternés, voie appareil, 48 fils), identité FUL1 vérifiée :

| Trame | avant | après | rapport (IC 95 %) |
| --- | ---: | ---: | --- |
| ng00 | 147,5 ms | **99,7 ms** | 0,676 (0,670–0,684) |
| ng01 | 117,2 ms | **81,0 ms** | 0,694 (0,688–0,699) |
| ng02 | 150,4 ms | **97,8 ms** | 0,647 (0,636–0,659) |

Partition murale, en ms (après : G jusqu'à la fin du dernier calcul de G ; « queue » = forêt non recouverte) :

| Trame | P | C | G (avant → après) | T, M, V, R (avant) → queue (après) |
| --- | ---: | ---: | ---: | ---: |
| ng00 | 1,9 → 2,2 | 28,4 | 56,7 → 62,0 | 60,6 → 5,5 |
| ng01 | 1,7 → 1,9 | 24,6 | 42,4 → 48,2 | 48,4 → 5,2 |
| ng02 | 1,9 | 28,5 | 51,4 → 52,0 | 68,5 → 13,7 |

Fins par ordre (instants depuis l'ouverture de `build_tower`, ms ; G joue les ordres du plus grand au plus petit) :
le G de l'ordre 5 finit à 39,0 / 30,9 / 35,3 ms (ng00 / ng01 / ng02) et son noyau 5,5 / 4,9 / 16,3 ms plus tard ;
la tour se termine sur les ordres 1 et 2, dont les G finissent les derniers (vers 62 / 48 / 52 ms), suivis de leurs
verticales et de leur registre. Ressources publiées, non jugées : CPU par passe +4 à +6 %, pic du budget +6 à +7 %.

**Session sur les 37 trames `v12set`** (second tour, publiée, non jugée) : médiane 230,8 → **160,6 ms**, maximum
448,2 → **327,3 ms**.

## Lecture

Le recouvrement retire de 36 à 55 ms du mur : presque tout T, M, V, R passe sous G. Sur ng00–02, le mur FULL descend
sous 100 ms. Sur les trames réelles de six séquences, il reste 1,6 fois le budget en médiane et 3,3 fois au maximum :
le contrat de 100 ms n'est pas tenu.

**G est maintenant le chemin critique** : le mur vaut à peu près P + C + G + la queue. Les leviers du coût interne de G
(chantier T2-d-B), puis ceux de C, comptent désormais en entier. La règle écrite d'avance pour paralléliser le noyau
de l'ordre K à l'intérieur de l'ordre (A6 : queue médiane d'au moins 5 ms et fin du noyau de l'ordre K plus de 2 ms
après la fin de G de cet ordre) est **remplie** sur les trois trames. Mais ce noyau finit bien avant la tour : la
queue est faite des ordres 1 et 2, résolus en dernier. A6 seul ne la raccourcirait donc probablement pas. Il faut
d'abord examiner l'ordre de résolution des ordres et ce qui ferme la queue.

## Ce que cette session n'établit pas

Ni le contrat FULL sur toutes les trames, ni `MES-M0` (expiré au délai du plan), ni la voie par défaut de la sonde : la
Session recouverte reste derrière `--recouvert` jusqu'à la bascule, avec ses lecteurs. GCP utilisé pour cette seule
session, arrêt certifié.
