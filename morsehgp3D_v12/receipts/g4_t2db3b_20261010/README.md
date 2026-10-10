# Session G4 T2-d-B3, nouvelle mesure sur le produit A6c — clés adoptées, balayage rejeté

10 octobre 2026. Session gardée `v12.20261010.t2db3b` (`gcp-migration/v12_session.py`, commit `81b0883d1`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
19:14:31 à 19:45:13 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 19:45:31 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Pourquoi une nouvelle mesure.** Dans la [session t2db3](../g4_t2db3_20261008/README.md), sur la base `8a0716e74`, les
trois leviers ont été rejetés. G y finissait pourtant 6 à 9 % plus tôt, mais la queue de l'ordre 5 fermait la tour.
Depuis, [A6c](../g4_a6c_20261010/README.md) a réduit cette queue. Le même levier est donc rejugé sur la base
`aa6338ee8`, par la même règle et les mêmes bras. La base a été révisée à 18:45 UTC, avant la mesure ; seuils,
statistiques et trames n'ont pas changé. La mesure précédente reste rejetée et publiée.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok, 755 sélectionnées | 140 s |
| `t2d_b3_pilote` | ok, verdicts rendus, aucun refus | 668 s |
| `mutants_catalogue_tour` | ok, campagnes du catalogue et de la tour | 808 s |

## Verdicts de `REGLE_T2D_B3` : lot et clés **adoptés**, balayage **rejeté**

Identités FUL1 et de la résolution établies ; A/A de 0,998 à 1,004.

| Trame | Lot | Clés seules | Balayage seul | Transfert seul | Clés après transfert | Balayage après clés |
| --- | --- | --- | --- | --- | --- | --- |
| ng00 | **0,983** (0,977–0,989) | **0,982** | 0,994 | 1,006 | 0,976 | 1,001 |
| ng01 | **0,987** (0,980–0,998) | **0,986** | 0,995 | 1,010 | 0,976 | 1,001 |
| ng02 | **0,984** (0,982–0,985) | **0,984** | 1,004 (0,995–**1,022**) | 1,008 | 0,976 | 1,000 |
| trame `02/001606` | **0,981** (0,980–0,983) | **0,980** | 0,996 | 1,009 | 0,971 | 1,001 |
| trame `08/001176` | **0,975** (0,972–0,977) | **0,971** | 0,999 (0,996–**1,002**) | 1,008 | 0,964 | 1,004 |
| **Verdict** | adopté | adopté | rejeté | (information) | (information) | (information) |

Médianes des tours, en ms, avant → clés :

| Trame | Fin de G | Étage C (transferts) | Mur |
| --- | --- | --- | --- |
| ng00 | 45,4 → 43,1 | 25,9 → 26,2 (2,64 → 3,07) | 79,9 → 78,6 |
| ng01 | 35,6 → 33,6 | 22,7 → 23,1 (2,52 → 2,98) | 66,1 → 64,9 |
| ng02 | 43,8 → 41,1 | 26,0 → 26,4 (2,85 → 3,30) | 79,4 → 78,1 |
| `02/001606` | 71,3 → 66,1 | 40,0 → 41,0 (4,05 → 5,07) | 125,3 → 122,9 |
| `08/001176` | 88,1 → 80,7 | 43,4 → 44,2 (4,63 → 5,40) | 146,7 → 142,6 |
| maximum `08/002119` | 151,3 → 138,4 | 68,9 → 70,2 (6,90 → 7,96) | 244,0 → 235,6 |

## Décision : le produit est le bras « clés » seul

Le lot et les clés sont adoptés. Ajouter le balayage aux clés ne fait rien (1,000 à 1,004), et le balayage seul est
rejeté. Les deux bras adoptés sont mesurés ; le produit retient le plus simple, **les clés seules (B3-K)**.

- `src/tower/resolve.cpp` revient à son état de `aa6338ee8`, et le mutant du balayage (`balayage_dernier_oublie`) est
  retiré : plancher de la tour 73, catalogue 38.
- Les sources du produit ont été vérifiées identiques, fichier par fichier, au bras `cles` reconstruit depuis
  l'archive de la base.

**Le coût restant** est le transfert des clés de l'appareil vers l'hôte : 0,8 à 1,0 % du mur, et +0,4 à +1,1 ms sur
l'étage C. Construire les clés sur l'hôte, comme le fait déjà la voie par tranches, est la prochaine variante à mesurer.

## Ce que cette session n'établit pas

Ni le contrat FULL, ni l'effet des clés sur la mémoire des scènes de plusieurs millions de sites (16 octets par boule
du catalogue). GCP utilisé pour cette seule session, arrêt certifié.
