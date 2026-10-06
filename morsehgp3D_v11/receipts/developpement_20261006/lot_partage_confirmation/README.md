# Exécuteur partagé du lot de feuilles : 400 ‰ confirmé à chaud, adopté pour la voie GPU à K5

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261006.claudesplitconf`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt
`TERMINATED` certifié (`claudesplitconf/receipt.json`). Source : `c6e294d11`. Prises neuves sur les trames LiDAR
réelles ng00, ng01 et ng02, W48. Suite du reçu `lot_partage/` (session `claudesplit1`), qui reste tel quel. Aucune
mesure ne promeut un statut public.

## Changement de statistique, déclaré dans le plan avant les données

La règle à froid de `claudesplit1` n'a admis aucune part. À froid, l'ouverture du contexte CUDA (environ 75 ms)
domine l'étage, et le partage n'y peut rien. La règle de cette session porte donc sur `domain_ms` à chaud : la médiane
des passes 2 à 10 d'un processus, par trame et par mode, à K5 avec des feuilles de 24. Une part (400 ou 450 ‰) est
admise si la moyenne géométrique des trois rapports `domain(part) / domain(sans partage)` est ≤ 0,95, sans aucun
rapport > 1,00. Est retenue la part admise de plus petite moyenne. Le plan (`claudesplitconf/plan.json`) porte cette
règle et la raison du changement.

## Exactitude

- Les quatre mutants du partage sont tués au code de sortie : `partage_selection_inverse` et `partage_cible_ignoree`
  (manifeste `catalogue`, `mut_catalogue.txt`), `partage_debut_local` et `partage_compteurs_appareil_omis`
  (manifeste `tower`, `mut_tower.txt`). Ce sont les mutants que `claudesplit1` n'avait pas pu juger.
- Les deux bancs sont `conforme`, et toutes les prises rendent les vidages des empreintes des trames.

## Verdict

| Part (pour mille) | Rapports à chaud ng00 / ng01 / ng02 | Moyenne géométrique | Max | Admise |
| --- | --- | ---: | ---: | --- |
| 400 | 0,898 / 0,873 / 0,907 | 0,893 | 0,907 | oui, retenue |
| 450 | 0,940 / 0,897 / 0,917 | 0,918 | 0,940 | oui |

**La voie GPU de référence à K5 devient `344059:400`.** Elle donne un étage `domain` à chaud de
162,5 / 137,9 / 164,7 ms, contre 180,9 / 157,9 / 181,7 ms sans partage. Le comportement par défaut du banc
(`full_probe` sans part, ou `:0`) ne change pas, pour que les plans antérieurs gardent leur sens. Les sessions
suivantes écrivent la part dans leurs modes.

## Lecture (descriptive)

- À froid, 400 ‰ donne 0,962 / 0,903 / 0,964 (gm 0,943), et 450 ‰ donne 0,935 / 0,937 / 0,935 (gm 0,936). C'est un
  gain sur toutes les trames, mais en deçà de l'ancien seuil de 0,90.
- Avec 400 ‰, le mur à chaud passe de 307 / 260 / 314 ms à 284 / 234 / 301 ms.
- À K10, une part de 250 ‰ donne 0,964 / 0,960 / 0,960 sur l'étage à chaud (gm 0,961), et 0,960 à froid, comme dans
  `claudesplit1`. Aucune règle n'était écrite pour K10 : aucune part n'y est adoptée.

## Pièces

`claudesplitconf/` : `plan.json`, `launch.json`, `receipt.json`, `gpu_ab_report_k5_24.json`,
`gpu_ab_report_k10_24.json`, `mut_catalogue.txt`, `mut_tower.txt`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS`
couvre tous les fichiers sauf lui-même.
