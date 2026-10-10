# Session G4 T2-d-B3 : LEM-T1 sans dichotomie indirecte — rejeté, retiré du produit

8 octobre 2026 (reçu publié le 10 octobre). Session gardée `v12.20261008.t2db3` (`gcp-migration/v12_session.py`,
commit `545ed987e`, preuve `pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`,
`--max-run-seconds 4200`. VM de 18:27:07 à 18:58:34 UTC, **arrêt certifié `TERMINATED`** par le lanceur ; relu par
l'auditeur à 19:11 UTC ([contrôle](../audit_reponses_20261008/g4_controle_auditeur/README.md)) et de nouveau le
10 octobre à 17:48 UTC. Reçu sans identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ;
empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Leviers.**

- **B3-K, côté catalogue.** La fin d'étage garde les clés S* triées hors de l'export : 16 octets par boule, transmis
  par la voie appareil. `find_support` cherche directement dans ces clés.
- **B3-B, côté tour.** Pour vérifier F dans P_b, la ligne de population est balayée sans relire la fiche de la boule.
- **Lot** : les deux ensemble.

Pilote [`pilote_t2d_b3.py`](../../microbancs/mes_t2d_b3/pilote_t2d_b3.py), sous `REGLE_T2D_B3`. Bras :

- avant : archive de `8a0716e74` ;
- A/A ;
- cles, balayage, et transfert (les clés circulent sans servir) ;
- après.

Mesure : mur FULL de la Session recouverte, K5, appareil, 48 fils, 10 tours × 8 passes.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok | 140 s |
| `t2d_b3_pilote` | ok, verdicts rendus, aucun refus | 697 s |
| `mutants_catalogue_tour` | ok, campagnes du catalogue et de la tour | 775 s |

## Verdicts de `REGLE_T2D_B3` (écrite à 15:09 UTC, base révisée à 17:32 et 17:55 UTC, avant toute mesure) : **tous rejetés**

Identités FUL1 et de la résolution établies ; A/A de 0,999 à 1,001.

| Trame | Lot | Clés | Balayage | Transfert seul |
| --- | --- | --- | --- | --- |
| ng00 | 0,980 (0,977–0,983) | 0,984 | 0,998 (0,996–**1,001**) | 1,000 |
| ng01 | 1,006 (0,981–**1,047**) | 0,980 | 0,997 | 1,002 |
| ng02 | **1,011** (1,006–1,017) | **1,009** (1,006–1,013) | 0,996 | 1,000 |
| `kitti_ng_02_001606` (médiane) | 0,995 | 0,996 | 0,997 (0,992–**1,001**) | 1,002 |
| `kitti_ng_08_001176` | 0,994 | 0,989 | 0,997 | 1,005 |

Un levier est adopté si la borne haute est sous 1 sur les cinq trames : les trois sont rejetés.

## Lecture (information, médianes des tours)

G finit nettement plus tôt avec le lot, mais le mur ne suit pas :

| Trame | Fin de G (ms) | Étage C (ms) | dont transferts (ms) | Mur (ms) |
| --- | --- | --- | --- | --- |
| ng00 | 45,7 → 43,0 | 25,6 → 26,0 | 2,62 → 3,07 | 80,0 → 78,4 |
| ng01 | 35,7 → 33,8 | 22,6 → 23,0 | 2,56 → 3,06 | 66,1 → 65,0 |
| ng02 | 42,0 → 39,0 | 25,8 → 26,3 | 2,86 → 3,35 | 83,4 → 84,2 |
| trame médiane | 67,0 → 62,1 | 39,7 → 40,5 | 4,04 → 4,90 | 144,8 → 144,1 |
| `kitti_ng_08_001176` | 81,1 → 74,8 | 42,8 → 43,8 | 4,42 → 5,25 | 169,2 → 167,9 |
| trame maximale | 139,3 → 127,0 | 68,2 → 69,3 | 6,50 → 7,55 | 281,3 → 279,7 |

Les clés font gagner à G de 6 à 9 %, soit de 2 à 12 ms. Mais le mur ne bouge presque pas, pour deux raisons :

- sur les trames moyennes et grandes, la queue de l'ordre 5, qui suit G, ferme la tour (voir A6c) ;
- le transfert des clés ajoute de 0,4 à 1,1 ms à l'étage C, qui précède G et s'ajoute directement au mur.

Sur ng02, où la queue (13 ms) est aussi critique, le mur s'allonge. B3 est donc un vrai gain de G, que le produit
actuel ne laisse pas voir. Il sera à rejuger après A6c. Les clés pourront alors être construites sur l'hôte, comme le
fait déjà la voie par tranches, plutôt que transférées.

**Décision.** Les trois leviers sont rejetés par la règle. B3 est **retiré de `main`** : `src/` et `tests/` reviennent
à leur état de `8a0716e74`, avec les manifestes tour 55 et catalogue 34. Le pilote reste comme banc. Les contre-lectures
de l'auditeur sont versées dans `audit_reponses_20261008/` :

- [admission de la session](../audit_reponses_20261008/session_t2db3_admission/README.md) ;
- [statistiques](../audit_reponses_20261008/t2db3_stats/README.md) ;
- [mémoire selon la voie](../audit_reponses_20261008/b3_memoire_voies/README.md).

## Ce que cette session n'établit pas

Ni le contrat FULL, ni l'effet de B3 une fois la queue de l'ordre 5 réduite. GCP utilisé pour cette seule session,
arrêt certifié.
