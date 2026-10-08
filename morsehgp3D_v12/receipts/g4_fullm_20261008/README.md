# Session G4 M : contrat FULL sur le produit, cache de blocs adopté, voie séquentielle écartée, profils 21/24/32

8 octobre 2026. Session gardée `v12.20261008.fullm` (`gcp-migration/v12_session.py`, commit `957e9784f`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 09:10:59
à 09:40:31 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 09:41:09 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only (MES-D6 : profils 21, 24 et 32)
public_status=not_claimed
```

**Produit mesuré** : `957e9784f`, c'est-à-dire la Session recouverte (voie par défaut de la sonde), le lot T2-d-C, le
Pool à équipe, T2-d-A et le bras « census » de T2-d-B, sans L4. Le cache de blocs était encore éteint par défaut.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`) | ok, 722 portes sur 722 | 141 s |
| `mes_full` ([`pilote_full.py`](../../microbancs/mes_full/pilote_full.py), schéma recouvert, lecteur strict partagé) | ok, aucun refus | 555 s |
| `apparie_cache_as` ([`pilote_apparie.py`](../../microbancs/mes_apparie/pilote_apparie.py)) | ok, campagne jugée | 372 s |
| `mutants_tour` | ok, 38 mutants de la tour (plancher 38) | 100 s |
| `mes_d6_profils` ([`pilote_d6.py`](../../microbancs/mes_d6_profils/pilote_d6.py), lecteur adapté `e37fd8935`) | ok, contrôles conformes | 230 s |

## 1. `MES-FULL` : contrat **non tenu** (règle écrite d'avance, inchangée depuis la session K)

Mur FULL K5 sur l'appareil, 48 fils : médiane des passes chaudes, empreintes FUL1 identiques entre passes, processus
et voies, aucun refus.

| Trames | Médiane | Maximum |
| --- | ---: | ---: |
| ng00–02 (5 processus × 10 passes) | **94,8 ms** | **96,0 ms** |
| 37 trames `v12set`, six séquences (5 processus, second tour) | 160,6 ms | 358,9 ms |

ng00 / ng01 / ng02 : 94,8 / 78,1 / 94,8 ms. Pour la première fois, les trois trames de référence sont sous 100 ms, mais
le contrat porte aussi sur les 37 trames, où la médiane vaut 1,6 fois le budget et le maximum 3,6 fois (trame
`kitti_ng_08_002119`, 99 099 sites : médiane 319,8 ms, pire médiane de processus 358,9 ms).

Étages sur les grandes trames (Session `v12set`, ms) :

| Trame | Sites | Mur | P | C | G | Queue |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| kitti_ng_08_002119 | 99 099 | 319,8 | 4,9 | 76,9 | 156,9 | 79,1 |
| kitti_ng_00_001896 | 95 586 | 320,2 | 3,8 | 71,4 | 162,9 | 79,9 |
| kitti_ng_08_001193 | 80 433 | 242,8 | 3,4 | 59,4 | 122,9 | 55,5 |
| kitti_ng_00_000648 | 77 965 | 168,1 | 3,0 | 44,8 | 69,0 | 47,6 |
| kitti_ng_02_001606 (médiane des sites) | 64 740 | 160,1 | 2,6 | 45,4 | 76,8 | 35,9 |

**Lecture.** Sur la trame de 99 099 sites, le G du produit finit 12 ms plus tôt que dans la session T2-d-A (156,9
contre 169,0 ms). Mais la queue s'allonge d'autant (79,1 contre 64,2 ms), et le mur ne bouge pas. Sur les grandes
trames, le chemin critique n'est plus G mais le noyau séquentiel de l'ordre 5. Sur cette trame, le G de l'ordre 5 finit
vers 101 ms, son noyau vers 199 ms, son registre vers 228 ms, et la tour vers 232 ms, alors que le dernier calcul de G
(tous ordres) finit vers 156 ms (journaux bruts, quatre des cinq processus). C'est le levier A6, en cours.

Autres régimes (information) : K10 sur l'appareil 593,7 / 443,9 / 504,6 ms ; voie CPU à K5 382,1 / 323,9 / 386,4 ms,
dont le catalogue CPU 319 / 273 / 321 ms.

## 2. Pilote apparié : cache de blocs **adopté**, voie séquentielle **rejetée**

`REGLE_APPARIEE`, écrite avant toute mesure : même binaire, bras par options de la sonde, ng00–02 à K5, appareil,
48 fils, 10 tours × 10 passes, ordre décalé sans inversion, rejeu brut des journaux. Rapport à la référence `ref`
(voie par défaut), moyenne géométrique et IC 95 % :

| Bras | ng00 | ng01 | ng02 | Verdict |
| --- | --- | --- | --- | --- |
| `cache` (`--cache=8 Gio`) | 0,929 (0,922–0,938) | 0,924 (0,920–0,928) | 0,918 (0,912–0,924) | **adopté** |
| `seq` (`--sequentiel`, comparaison A/S de l'auditeur) | 1,515 (1,506–1,525) | 1,465 (1,446–1,482) | 1,529 (1,520–1,537) | **rejeté** |
| `aa` (A/A) | 1,004 (0,993–1,019) | 0,999 (0,987–1,011) | 0,991 (0,980–1,001) | contrôle, dans la fenêtre |

Murs médians (ms), référence → cache : 94,5 → 87,5 (ng00), 77,9 → 72,0 (ng01), 96,6 → 88,4 (ng02). Le temps CPU par
passe baisse de 9 %. Session `v12set`, information : médiane 161,7 → 152,6 ms, maximum 319,1 → 305,9 ms. Sur le même
binaire, la voie séquentielle coûte 47 à 53 % de plus que la Session recouverte. Le cas A/S de l'auditeur est
tranché : la route recouverte, prise en entier, l'emporte. Ce n'est pas une mesure du recouvrement seul.

**Décision appliquée** dans le commit qui publie ce reçu : le cache de blocs, à 8 Gio, devient le défaut de la Session
de la sonde FULL. `--cache=0` l'éteint (témoin, ablation). L'architecture le décrit comme les arènes d'hôte de la
Session résidente (D1, « les arènes et les caches ne se paient qu'une fois »).

## 3. `MES-D6` : coût des profils 21, 24 et 32 (voie CPU, catalogue et G)

Rapport au couple (profil 21, ×1) de la même trame et du même tour, K5, 48 fils, 3 tours (IC dans
[`mes_d6.md`](resultats/cmd/004_mes_d6_profils/files/d6/mes_d6.md)) :

| Trame | 24 ×1 catalogue / G | 32 ×1 catalogue / G | 32 ×8 catalogue / G | 32 ×2048 catalogue / G |
| --- | --- | --- | --- | --- |
| ng00 | 1,011 / 1,014 | 1,014 / 1,038 | 1,057 / 1,047 | **2,987** / 1,104 |
| ng01 | 1,008 / 1,005 | 1,008 / 1,045 | 1,072 / 1,045 | **2,931** / 1,125 |
| ng02 | 1,021 / 1,108 | 1,025 / 1,136 | 1,040 / 1,054 | **2,966** / 1,093 |

Les lignes de G sont admises cette fois : la session K les avait toutes refusées. Sur les mêmes coordonnées, élargir le
profil coûte 1 à 2,5 % au catalogue et 0,5 à 14 % à G. Une grille 8 fois plus fine coûte 3 à 7 % au catalogue. Des
coordonnées qui remplissent 32 bits (grille de 1/2048 mm) triplent le catalogue CPU et ajoutent 9 à 13 % à G. Les
écarts de G à ng02 (1,108 dès le profil 24) restent à confirmer sur plus de tours. La voie appareil aux profils 24 et 32
n'est pas mesurée.

## Ce que cette session n'établit pas

Le contrat n'est pas tenu sur les 37 trames, et le gain du cache n'est pas jugé sur elles (seulement en information).
La voie appareil aux profils 24 et 32 n'est pas mesurée, ni le recouvrement seul hors de la route A. GCP utilisé pour
cette seule session, arrêt certifié.
