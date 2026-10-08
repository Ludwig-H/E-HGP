# Sessions G4 A6b : la chaîne de l'ordre K, aides après G — rejetée, retirée du produit

8 octobre 2026. Trois sessions gardées (`gcp-migration/v12_session.py`, preuve `pushed_commit`), cible
`us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. Reçus sans identité de compte, un
dossier par session.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Levier.** A6b, intégré sur `main` avant la mesure (`f2c106d93`, correctif v2 de l'agent du chantier A pris tel
quel). C'est A6 avec son pont de publication (`CST-0242`), plus un seul changement d'ordonnancement, tiré du diagnostic
du rejet d'A6. Le noyau passe d'abord, puis les étapes, puis les tranches de G, puis les aides ; aucune aide ne
commence pendant G. Pilote [`pilote_t2d_a6b.py`](../../microbancs/mes_t2d_a6b/pilote_t2d_a6b.py). `REGLE_T2D_A6B`, écrite
à 14:47 UTC avant toute mesure :

- identité FUL1 ;
- borne haute de l'IC des grandes trames sous 0,95 ;
- borne haute de chacune de ng00–02 sous 1,01, plus strict que le 1,02 d'A6 ;
- veto si l'A/A sort de ±1,5 %.

Bras avant : archive de `47feedc96` (SHA-256 `0e812c6b…`).

## Session `v12.20261008.a6b` : refusée (veto A/A), puis récupérée

- **Déroulé.** VM démarrée à 16:01:05 UTC. Le codespace a redémarré vers 16:33 UTC pendant la session, et le
  processus de session a été perdu. La VM s'est arrêtée seule à 16:58:04 UTC, par la garde de l'invité. La reprise
  gardée (`--recover`, [`a6b/recovery.json`](a6b/recovery.json)) a certifié `TERMINATED` à 17:03 UTC.
- **Récupération.** L'archive des résultats était restée sur le disque de la VM. La session de récupération
  `v12.20261008.a6br` (VM de 17:05:00 à 17:08:30 UTC, arrêt certifié et relu à 17:08:46 ; [`a6br/`](a6br/)) l'a
  rapatriée, avec son empreinte vérifiée contre son `SHA256SUMS` (`results.tar.gz`, `9833e053…`).
- **Commandes.** `socle_ctest` ok ; `lidar_ctest` ok ; 66 mutants de la tour tués ; le pilote rend le code 3 (refus).
  Tables et rapport, expurgés : [`a6b/`](a6b/).
- **Verdict : refusé.** L'A/A de ng01 vaut 0,9569, hors de ±1,5 %. Un seul processus est en cause : bras avant, tour
  2, 84,0 ms au lieu d'environ 65,5. En information : grandes trames 0,852 ; ng00 1,007 ; ng02 0,954.

## Session `v12.20261008.a6b2` (même commit, même plan) : **rejeté**

VM de 17:10:44 à 17:39:03 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 17:39:36 UTC.
Commandes, toutes ok :

- `socle_ctest` (141 s) ;
- `t2da6b_pilote` (368 s) ;
- `lidar_ctest` (762 s) ;
- `mutants_tour`, 66 mutants tués (209 s).

Tables : [`a6b2/`](a6b2/resultats/cmd/001_t2da6b_pilote/files/t2da6b/tableaux_t2d_a6b.md).

| Cohorte | Tours | Rapport après / avant | IC 95 % | Seuil | A/A |
| --- | ---: | ---: | --- | ---: | ---: |
| 21 grandes trames | 6 | **0,849** | 0,847 – 0,851 | < 0,95 | 0,997 |
| ng00 | 5 | 1,009 | 1,006 – **1,012** | < 1,01 | 0,996 |
| ng01 | 5 | 1,013 | 1,007 – **1,019** | < 1,01 | 1,001 |
| ng02 | 5 | **0,956** | 0,956 – 0,957 | < 1,01 | 0,995 |

**Sur les grandes trames, le gain est massif.** La queue passe de 29–75 ms à 10–22 ms :

- trame médiane `kitti_ng_02_001606` : 144,4 → 124,6 ms ;
- `kitti_ng_00_001896` : 289,3 → 241,6 ms ;
- trame maximale `kitti_ng_08_002119` : 281,4 → 239,0 ms.

**Les petites trames perdent un peu**, médianes des tours :

| Trame | mur avant → après (ms) | queue avant → après (ms) | CPU par passe |
| --- | --- | --- | --- |
| ng00 | 80,1 → 80,7 | 6,7 → 6,2 | +1,6 % |
| ng01 | 66,0 → 66,7 | 5,7 → 5,5 | +1,1 % |
| ng02 | 83,1 → 79,6 | 13,5 → 7,8 | — |

La queue raccourcit aussi sur ng00 et ng01, mais le mur s'allonge : la perte est avant la queue. Elle tient
vraisemblablement à G, retardé par les étapes de numérotation et d'historique, qui passent avant ses tranches.

**Décision.** La règle rejette A6b. Il est **retiré de `main`** : `src/tower/`, `tests/tower/` et
`tests/mutants/tower.json` reviennent à leur état de `47feedc96`, plancher 55. Le pilote reste comme banc. Une A6c est
confiée au chantier A. Elle devra garder ce gain sans la perte sur ng00–02, avec une règle écrite d'avance et des
seuils qui ne seront pas relâchés.

## Ce que ces sessions n'établissent pas

Ni le contrat FULL, ni la cause exacte de la perte de 1 % sur les petites trames. GCP utilisé pour ces trois sessions
seulement, arrêts certifiés.
