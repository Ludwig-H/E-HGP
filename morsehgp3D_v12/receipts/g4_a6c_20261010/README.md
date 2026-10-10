# Session G4 A6c : la chaîne de l'ordre K, engagée au-delà de 43 900 sites — adoptée

10 octobre 2026. Session gardée `v12.20261010.a6c` (`gcp-migration/v12_session.py`, commit `aa6338ee8`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de
18:13:56 à 18:42:28 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 18:43:05 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Levier.** A6c, intégré sur `main` avant la mesure (`aa6338ee8`). C'est le correctif de l'agent du chantier A, pris
tel quel ; ses vérifications locales, interrompues par la limite d'API, ont été finies par le développeur.

- **La chaîne de l'ordre K d'A6b** est numérotation par morceaux, aides après G, historique parallèle et pont
  `CST-0242`. Elle n'est engagée que si la trame compte au moins 43 900 sites, un compte connu à l'admission.
- **Sous ce seuil**, chaque ordre suit le chemin de la base.
- **Le diagnostic du rejet d'A6b.** Sur ng00 et ng01, le noyau de l'ordre 5 de la base finissait déjà avant G.
  L'avancer ne rapportait rien et retardait G.
- **Le seuil** vient d'un modèle du retard du noyau de la base sur la fin de G : retard = −32,53 + 0,8093 × sites/1000
  ms. Erreur quadratique 2,84 ms, et 3,02 ms en validation croisée « laisser un dehors », sur les 37 trames et
  ng00–02.

Pilote [`pilote_t2d_a6c.py`](../../microbancs/mes_t2d_a6c/pilote_t2d_a6c.py), juge fermé hérité d'A6. Bras avant :
archive de `8a0716e74`. Mesure : K5, appareil, 48 fils.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok, 755 sélectionnées | 141 s |
| `t2da6c_pilote` | ok, verdict rendu, aucun refus | 368 s |
| `lidar_ctest` (`MES-M0` sémantique compris) | ok, 7 sur 7 | 760 s |
| `mutants_tour` | ok, 72 mutants de la tour tués (plancher 72) | 217 s |

## Verdict de `REGLE_T2D_A6C` : **adopté**

La règle a été écrite le 8 octobre à 18:18 UTC, avec les seuils d'A6b. Une précision a été ajoutée le 10 octobre à
17:51 UTC, avant la mesure. Identité FUL1 établie.

| Cohorte | Tours | Rapport après / avant | IC 95 % | Seuil | A/A |
| --- | ---: | ---: | --- | ---: | ---: |
| 21 grandes trames (toutes engagées) | 6 | **0,854** | 0,852 – 0,856 | < 0,95 | 1,004 |
| ng00 (39 885 sites, non engagée) | 5 | **1,003** | 0,999 – 1,006 | < 1,01 | 1,000 |
| ng01 (35 551 sites, non engagée) | 5 | **0,998** | 0,991 – 1,004 | < 1,01 | 0,990 |
| ng02 (45 845 sites, engagée) | 5 | **0,961** | 0,959 – 0,963 | < 1,01 | 0,996 |

La chaîne est engagée sur 32 des 40 trames. Effet sur les grandes trames
([tableaux](resultats/cmd/001_t2da6c_pilote/files/t2da6c/tableaux_t2d_a6c.md)) :

- la queue passe de 29–73 ms à 11–21 ms ;
- trame médiane `kitti_ng_00_003624` : 142,0 → 117,9 ms ;
- `kitti_ng_02_001606` : 143,2 → 124,2 ms ;
- trame maximale `kitti_ng_00_001896` : 286,3 → 241,2 ms.

Petites trames (médianes des tours, ms) :

| Trame | Mur avant → après | Queue avant → après |
| --- | --- | --- |
| ng00 | 79,9 → 80,0 | 6,6 → 6,7 |
| ng01 | 66,3 → 66,0 | 5,7 → 5,8 |
| ng02 | 82,8 → 79,5 | 13,1 → 7,6 |

**Calibration et indépendance.** Le seuil a été calibré sur les mesures des sessions a6b et a6b2, qui portent sur les
mêmes trames. La mesure est nouvelle, mais ses trames ne sont pas indépendantes de la calibration. Trames proches du
seuil, de 35 000 à 50 000 sites ([critère](../../microbancs/mes_t2d_a6c/critere_a6c.json)) :

| Trame | Sites | Retard réel de la base (ms) | Retard prédit, LOO (ms) | Chaîne | Rapport d'A6b (a6b2) |
| --- | ---: | ---: | ---: | --- | ---: |
| `kitti_ng_08_000100` (ng01) | 35 551 | −5,9 | −3,5 | non | 1,012 |
| `kitti_ng_06_000798` | 37 543 | 0,5 | −2,4 | non | 0,997 |
| `kitti_ng_06_000800` | 38 341 | 1,4 | −1,8 | non | 0,985 |
| `kitti_ng_08_000000` (ng00) | 39 885 | −7,0 | 0,3 | non | 1,007 |
| `kitti_ng_08_000246` | 41 691 | −4,6 | 1,6 | non | 0,978 |
| `kitti_ng_08_000200` (ng02) | 45 845 | 3,6 | 4,6 | oui | 0,955 |
| `kitti_ng_06_000770` | 46 561 | 8,2 | 5,0 | oui | 0,921 |
| `kitti_ng_06_000804` | 47 658 | 6,5 | 6,0 | oui | 0,930 |
| `kitti_ng_08_001302` | 48 925 | 6,0 | 7,1 | oui | 0,941 |
| `kitti_ng_06_000780` | 49 628 | 8,5 | 7,6 | oui | 0,912 |
| `kitti_ng_06_000772` | 49 668 | 7,3 | 7,7 | oui | 0,927 |

Seuls ng00–02 de cette bande sont jugés ici. Les autres trames de la bande ne sont mesurées que par `MES-FULL`. Deux
trames sous le seuil (0,985 et 0,978 avec A6b) laissent un petit gain de côté ; aucune trame engagée de la bande
n'avait perdu avec A6b.

**Vérifications locales avant la session** (développeur, 10 octobre) :

- `ctest -LE long` : 755 tests sélectionnés, 754 passés, 1 sauté (la sentinelle LiDAR), 0 en échec ;
- ThreadSanitizer (`RelWithDebInfo`, `setarch -R`) : 13 exécutions, code 0, aucun avertissement. Portes jouées : les
  leviers `numerotation`, `indices`, `historique`, `priorite`, `tranches` et `bascule` ; la région `terminaison` et
  `fermeture` ; le pipeline `admission`, `determinisme`, `graphe`, `identite` et `refus`.

**Décision.** A6c reste dans le produit. Les leviers de G masqués par la queue, B2-C et B3, sont à rejuger sur ce
produit.

## Ce que cette session n'établit pas

Ni le contrat FULL, mesuré ensuite dans la session O sur ce produit, ni l'effet d'A6c sur les trames de 43 900 à
60 000 sites, jugées par aucune cohorte ici. GCP utilisé pour cette seule session, arrêt certifié.
