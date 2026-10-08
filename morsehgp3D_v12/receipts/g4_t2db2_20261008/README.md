# Session G4 T2-d-B2 : ouverture et coût interne de l'étage G — lot adopté ; profil de G par poste

8 octobre 2026. Session gardée `v12.20261008.t2db2` (`gcp-migration/v12_session.py`, commit `4171b2653`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
13:54:13 à 14:18:52 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 14:19:11 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R ; sonde de G profilée en information)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Leviers jugés** ([rapport de l'agent](../developpement_20261008/t2d_b2_RAPPORT.md)) :

- **B2-T** : les index des naissances de tous les ordres sont construits ensemble à l'ouverture de `build_tower`.
- **B2-S** : la seconde recherche du support est évitée.
- **B2-C** : census à plat.

Pilote [`pilote_t2d_b2.py`](../../microbancs/mes_t2d_b2/pilote_t2d_b2.py). Bras :

- avant : archive de `72f622a55`, produit identique à celui de la source après le retrait de A6 ;
- avant_bis : contrôle A/A ;
- tables, relecture, census, et après (le lot) : substitutions exactes depuis cette archive ; le bras après est
  l'arbre de la source, fichier par fichier.

Mesure : mur FULL de la Session recouverte, appareil, 48 fils, cache de 8 Gio, 10 tours × 8 passes, sans `--digest`.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok, 733 portes sur 733 | 141 s |
| `t2d_b2_pilote` | ok, verdicts rendus, aucun refus | 689 s |
| `mutants_index_num_tour` | ok : num, index, tour (plancher 49) | 342 s |
| `profil_g_info` (information, hors règle) | ok, 6 cas | 66 s |

## Verdicts de `REGLE_T2D_B2` (écrite à 09:44 UTC, révisée à 09:48 et 11:22 UTC, avant toute mesure)

Identité FUL1 établie sur les cinq trames dans tous les bras (ng00 = `3a2bfb4f…`). Rapports bras / avant (moyenne
géométrique, IC 95 %) ; adoption si la borne haute est sous 1 sur chacune des cinq trames :

| Trame | Lot B2 | B2-T tables | B2-S relecture | B2-C census | A/A |
| --- | --- | --- | --- | --- | --- |
| ng00 | **0,944** (0,939–0,948) | 0,970 (0,966–0,974) | 1,000 (0,994–1,005) | 0,975 (0,971–0,978) | 0,999 |
| ng01 | **0,948** (0,944–0,951) | 0,968 (0,962–0,977) | 1,003 (0,997–1,008) | 0,980 (0,975–0,984) | 1,000 |
| ng02 | **0,963** (0,959–0,968) | 0,970 (0,966–0,974) | 1,000 (0,995–1,004) | 0,996 (0,991–1,000) | 1,001 |
| `kitti_ng_02_001606` (médiane) | **0,982** (0,978–0,985) | 0,985 (0,982–0,988) | 0,999 (0,994–1,004) | 0,996 (0,992–1,000) | 1,000 |
| `kitti_ng_08_001176` | **0,984** (0,980–0,988) | 0,980 (0,975–0,984) | 0,998 (0,995–1,001) | 0,995 (0,991–0,999) | 0,999 |
| **Verdict** | **adopté** | **adopté** | rejeté | rejeté (bornes 1,000) | contrôle |

Murs médians (ms), avant → lot : ng00 87,3 → 82,6 ; ng01 71,6 → 67,8 ; ng02 88,1 → 84,9 ; trame médiane
149,9 → 147,5 ; `kitti_ng_08_001176` 175,6 → 172,7.

**Décision.** Le produit reste le **lot**, déjà sur `main` depuis `4171b2653`, mesuré et adopté. B2-T est adopté seul.
B2-S n'a pas d'effet mesurable seul ; il reste dans le produit parce que le produit est le bras mesuré, comme L1 dans
T2-d-B. B2-C est rejeté seul de justesse, avec une borne haute de 1,000 sur ng02 et sur la trame médiane. Sur ng00 et
ng01, il retire 2,5 % ; ajouté à B2-T, B2-S et B2-C retirent encore 2,7 % sur ng00. Aucun bras mesuré ne contient B2-C
sans B2-S : retirer B2-S donnerait un produit non mesuré.

**Pourquoi le mur bouge peu sur les trames moyennes.** La fin de G recule nettement, mais la queue de l'ordre 5 ferme
la tour (A6 rejeté). Fin de G avant → lot :

- ng00 : 54,0 → 45,7 ms (−15 %) ;
- trame médiane : 71,9 → 66,8 ms ;
- `kitti_ng_08_001176` : 87,6 → 80,8 ms ;
- trame maximale `kitti_ng_08_002119` (information) : 148,9 → 140,0 ms, pour un mur de 291,9 → 286,8 ms.

À K10, en information (un tour) : ng00 548,8 → 519,6 ms, ng01 406,3 → 384,9 ms, ng02 465,7 → 444,9 ms.

## Information hors règle : profil de G par poste à 48 fils sur G4

Banc [`mes_g_profil`](../../microbancs/mes_g_profil/README.md), sonde de G profilée (`resolve_tower`, voie CPU) sur la
source (lot B2 compris) ; [tableaux](resultats/cmd/003_profil_g_info/files/profil_g/profil_g.md). Parts des temps-fils
de la résolution à 48 fils :

| Poste | ng00 | Trame médiane | Trame maximale | ns par occurrence, médiane, 1 → 48 fils |
| --- | ---: | ---: | ---: | --- |
| LEM-T1 (`find_support`, puis F dans P_b) | 25 % | 27 % | 28 % | 361 → 520 |
| trace (formation des représentants) | 20 % | 22 % | 20 % | 82 → 111 |
| sonde de la table | 14 % | 14 % | 14 % | 53 → 68 |
| proposition | 10 % | 10 % | 10 % | 166 → 192 |
| arrêt | 9 % | 10 % | 9 % | 184 → 266 |
| census saturé et complet | 18 % | 15 % | 17 % | 854 et 1 513 → 1 181 et 2 019 |
| certificat, pas | 4 % | 3 % | 4 % | |

Résolution à la trame médiane : 2 073 ms à un fil, 60,3 ms à 48 fils (×34). Par occurrence, les postes coûtent
28 à 45 % de plus à 48 fils qu'à un fil : ils attendent la mémoire. LEM-T1 est le premier poste partout. C'est le
prochain levier de G : une recherche du support sans dichotomie indirecte, avec une table S* → boule à accès direct.
Le profil ajoute deux lectures encadrées par `lfence` à chaque occurrence ; ce n'est pas le produit.

## Ce que cette session n'établit pas

Ni le contrat FULL, ni l'effet de B2-C ou de B2-S au-dessus de B2-T avec une règle, ni le profil du produit lui-même.
GCP utilisé pour cette seule session, arrêt certifié.
