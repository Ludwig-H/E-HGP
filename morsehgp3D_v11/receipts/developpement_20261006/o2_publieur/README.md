# Publieur des forêts, première partie du levier O2 : parents DSU denses et préchargement des graines

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261006.claudeo2a`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudeo2a/receipt.json`). Source : `13a4a0a4c`. Toutes les mesures portent sur les trois trames LiDAR
réelles ng00, ng01 et ng02. Aucune mesure ne promeut un statut public.

## Pourquoi le publieur

La chronologie relevée par `claudev3c` (reçu `constantes_pas_descente`) montrait qu'après V3 les publieurs des ordres
4 et 5 étaient devenus le chemin critique de l'étage des forêts. Ils calculaient pendant tout le pipeline et finissaient
20 à 27 ms après les résolveurs.

Le profil callgrind avec simulation de cache de la publication (ng00, K = 5, un fil, tous ordres) donnait :
- `regular_cell` et ses `find` : 52 % des instructions, 83 % des défauts L1 en lecture et 86 % des défauts du dernier
  niveau (365 000) ;
- la fermeture des plateaux : 16 % des instructions, et presque tous les défauts en écriture, ceux de l'arbre neuf ;
- `cell_add`, appelé hors ligne : 7 % ;
- le balayage des 1,31 M fiches de boules : environ 3 %.

## Changement (`13a4a0a4c`)

- **Parents DSU dans un tableau dense séparé** (`ForestBuilder::parents`, u32) : `find` ne lit plus les champs de
  chaîne. L'état restant (`top`, `head`, `tail`, `next`, `touched`) passe de 24 à 20 octets. Le total par naissance
  (état, parent, entrée `touched`) reste de 28 octets ; le budget est inchangé.
- **Préchargement** des parents et des états des graines du job situé 16 jobs plus loin. Seulement dans un bloc déjà
  confirmé, c'est-à-dire lu après l'acquisition de `await_job`, ou hors pipeline, où aucun résolveur n'écrit en même
  temps. Il est sans effet sur toute décision.
- `cell_add` passe en ligne.
- Mutants : `births_blocs_etat_partage` retargeté sur la nouvelle initialisation, `births_blocs_parent_partage` nouveau.

## Exactitude

- Les deux mutants sont tués (`mut_tower.*`).
- **TSan u21, portes ordinaires : 800/800 conformes** (`result_gcc_tsan.json`). C'est le contrôle de la garde du
  préchargement : le lanceur de mutants construit sans sanitizer et sans `setarch -R`, si bien qu'un mutant TSan de
  cette garde ne serait pas jouable.
- Trois bancs `conforme` ; toutes les prises rendent les vidages des empreintes des trames, à K5 et à K10. Vidages
  vérifiés aussi en local sur les trois trames, au masque 278523.

## Règle écrite dans le plan avant la session, et mesure

Variantes `new` (cette source) et `cst` (archive de `38faaf272`). Statistique : `forest_ms` à froid, médiane de
6 processus neufs par trame et par variante. Règle : gardé si la moyenne géométrique des 6 rapports new/cst à K5 est
≤ 1,00 et si aucun rapport ne dépasse 1,03.

| Bras | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| K5 CPU, feuilles 16 (278523) : cst → new (ms) | 133,1 → 110,7 (0,831) | 106,0 → 102,3 (0,965) | 136,8 → 123,6 (0,903) |
| K5 GPU+C, feuilles 24 (344059) : cst → new (ms) | 128,6 → 118,3 (0,920) | 106,5 → 93,1 (0,874) | 133,3 → 124,5 (0,934) |
| K10 GPU+C, feuilles 24, descriptif (ms) | 1 334 → 1 346 (1,009) | 972 → 954 (0,981) | 1 102 → 1 082 (0,982) |

Moyenne géométrique à K5 : **0,904**, pire rapport **0,965**. **Gardé.** À K10, l'effet est neutre : la résolution
y domine.

## Chronologie après le changement (K5, `new`, médianes à froid, ms depuis le début du pipeline)

| Bras | Trame | Fin des résolveurs | Fin des publieurs 3 / 4 / 5 | CPU des publieurs 3 / 4 / 5 | Fin des verticales 5 |
| --- | --- | ---: | --- | --- | ---: |
| CPU | ng00 | 80,0 | 80,6 / 88,7 / 90,0 | 70,4 / 86,8 / 87,3 | 90,1 |
| CPU | ng01 | 64,9 | 74,8 / 82,8 / 83,8 | 72,9 / 82,5 / 78,4 | 85,2 |
| CPU | ng02 | 76,1 | 89,1 / 93,1 / 95,5 | 81,9 / 92,8 / 94,4 | 99,3 |
| GPU | ng00 | 82,5 | 86,8 / 89,7 / 98,0 | 75,7 / 88,0 / 94,9 | 99,1 |
| GPU | ng01 | 61,8 | 67,4 / 71,1 / 73,5 | 55,0 / 69,5 / 72,7 | 76,3 |
| GPU | ng02 | 76,9 | 92,9 / 95,2 / 99,3 | 75,4 / 94,9 / 94,9 | 99,6 |

La queue des publieurs derrière les résolveurs passe de 20–27 ms à 10–22 ms. Les publieurs 4 et 5 restent occupés
presque tout le pipeline (CPU de 70 à 95 ms) : leur débit reste le premier levier de l'étage, avec la résolution
(62 à 83 ms) et les quelque 20 ms qui entourent le pipeline.

## Pièces

`claudeo2a/` : `plan.json`, `launch.json`, `receipt.json`, rapports `gpu_ab_report_*.json` (chronologie par prise
comprise), `mut_tower.*`, `tsan_matrix_summary.json`, `result_gcc_tsan.json`, `archives_variantes.sha256`. Aucune
donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers sauf lui-même.
