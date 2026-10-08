# Session G4 R1 : raccourci du registre pour les classes à cellule unique — adopté

8 octobre 2026. Session gardée `v12.20261008.r1` (`gcp-migration/v12_session.py`, commit `47feedc96`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
14:46:51 à 15:17:09 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 15:17:38 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris ; K10 en information)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Levier.** R1, intégré sur `main` avant la mesure (`47feedc96`). C'est le critère q = d + 1 de l'auditeur
([preuve](../audit_reponses_20261008/registre_classe_unique/README.md),
[raccord relu](../audit_reponses_20261008/r1_raccord_math/README.md)) : une ligne du registre dont la classe n'a
qu'une cellule ne relit aucun représentant et copie les enfants de sa classe. Seul
`src/tower/registry_branches.cpp` change dans le produit.

Pilote [`pilote_r1.py`](../../microbancs/mes_r1/pilote_r1.py), avec la fermeture de cohorte de l'auditeur. Bras :

- avant : archive de `5f8e777cf` ;
- avant_bis : contrôle A/A ;
- après : paquet de la session.

Les deux bras ont le cache de blocs de 8 Gio. Mesure : K5, appareil, 48 fils, Session recouverte.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok, 747 portes sur 747 | 141 s |
| `r1_pilote` | ok, verdict rendu, aucun refus | 474 s |
| `lidar_ctest` (`MES-M0` sémantique compris) | ok, 7 sur 7 | 751 s |
| `mutants_tour` | ok, 55 mutants de la tour (plancher 55) | 190 s |

## Verdict de `REGLE_R1` (écrite à 12:31 UTC, base amendée à 14:09 UTC, avant toute mesure) : **adopté**

Identité FUL1 établie. Rapports après / avant (moyenne géométrique, IC 95 % par bootstrap sur les tours) :

| Cohorte | Tours | Rapport | IC 95 % | Seuil | A/A |
| --- | ---: | ---: | --- | ---: | ---: |
| 21 grandes trames du `v12set` | 6 | **0,973** | 0,969 – 0,977 | < 0,99 | 0,999 |
| ng00 | 5 | **0,963** | 0,960 – 0,968 | < 1,02 | 1,000 |
| ng01 | 5 | **0,970** | 0,949 – 0,993 | < 1,02 | 0,994 |
| ng02 | 5 | **0,966** | 0,962 – 0,969 | < 1,02 | 1,000 |

À K10, en information (3 tours) : ng00 0,956, ng01 0,949, ng02 0,957.

Sur les grandes trames ([tableaux](resultats/cmd/001_r1_pilote/files/r1/tableaux_r1.md)) :

- le registre de l'ordre 5 finit 2 à 5 ms plus tôt après M et V ;
- la queue passe de 33–81 ms à 29–73 ms ;
- trame médiane `kitti_ng_02_001606` : 148,1 → 144,7 ms ; trame maximale `kitti_ng_08_002119` : 288,9 → 280,7 ms.

R1 reste dans le produit.

## Ce que cette session n'établit pas

Ni le contrat FULL : la queue de l'ordre 5 reste le premier poste des trames moyennes et grandes, objet de l'A6b en
préparation. GCP utilisé pour cette seule session, arrêt certifié.
