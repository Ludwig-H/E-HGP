# Exécuteur partagé du lot de feuilles : mesure G4, règle non atteinte, option gardée à 0

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261006.claudesplit1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudesplit1/receipt.json`). Source : `2045ec27c`. Trames LiDAR réelles ng00, ng01, ng02, W48. Aucune
mesure ne promeut un statut public.

## Objet

Le diagnostic `claudedom1` (reçu `domain_diagnostic`) montrait que le Pool attend pendant tout le lot de feuilles du
GPU. `split_host_permille` lui confie les feuilles les plus lourdes, estimées par m³, jusqu'à cette part du travail
estimé ; le GPU traite les autres en même temps, et les deux résultats sont fusionnés dans l'ordre du lot. Les sorties
ne dépendent pas de la part.

## Exactitude

- Deux bancs `conforme`. Toutes les prises de tous les modes (voie CPU, GPU sans partage, parts de 30, 40 et 50 % à K5,
  de 25 et 35 % à K10) rendent les vidages des empreintes des trames.
- **Le lot de mutants n'a jugé aucun mutant** (`mut_catalogue.txt`). Deux mutants du manifeste `catalogue` citaient la
  porte `mhgp11_tower_full_leaf_lanes`, absente d'une campagne du seul module catalogue. Le témoin sans mutation a
  donc refusé, et la campagne s'est arrêtée avant tout jugement. Ces deux mutants passent au manifeste `tower` ; les
  quatre mutants du partage restent à jouer.

## Règle écrite dans le plan avant la session, et verdict

Une part s est admise si, à K5 avec des feuilles de 24, la moyenne géométrique des rapports
`domain(s) / domain(gpu sans partage)` à froid (médiane des prises par trame) est ≤ 0,90, sans aucun rapport > 1,00.

| Part (pour mille) | Rapports à froid ng00 / ng01 / ng02 | Moyenne géométrique | Admise |
| --- | --- | ---: | --- |
| 300 | 0,938 / 0,944 / 0,929 | 0,937 | non |
| 400 | 1,016 / 0,911 / 1,009 | 0,977 | non |
| 500 | 0,934 / 0,890 / 0,923 | 0,916 | non |

**Aucune part n'est admise : l'option reste disponible, à 0 par défaut.** Le gain existe, mais il est inférieur au
seuil écrit.

## Lecture (descriptive)

- **À chaud**, la part de 400 donne ×0,90 / 0,86 / 0,91 sur l'étage `domain`, soit −16 à −22 ms. À froid, l'ouverture
  du contexte CUDA et les premiers usages ajoutent environ 75 ms au lot GPU (domaine à froid 239 à 259 ms contre 160 à
  182 ms à chaud), ce qui noie l'effet et le rend bruité. La statistique à froid était un mauvais choix pour ce
  levier ; elle n'est pas réécrite après coup.
- Le gain mesuré reste en deçà du modèle `1/(1/T_GPU + 1/T_Pool)`, qui annonçait environ −30 ms : à chaud, le lot passe
  de 75 à 57 ms sur ng02, pour 42 à 48 ms attendus. L'estimation m³ du travail équilibre mal les deux côtés (corrélation
  de rang 0,55 avec les cycles mesurés, contre 0,91 pour les triplets candidats, d'après le rapport D du workflow GPU).
- À K10 : 25 % donne ×0,95 sur l'étage, à froid comme à chaud ; 35 % ne donne rien.
- La voie CPU sans lot a un étage à chaud de 192 à 235 ms, contre 160 à 182 ms pour le GPU sans partage.

**Suite possible.** Une estimation du travail plus fidèle (triplets candidats, ou un comptage préalable bon marché),
puis une confirmation sur une statistique à chaud écrite d'avance, sur données neuves.

## Pièces

`claudesplit1/` : `plan.json`, `launch.json`, `receipt.json`, `gpu_ab_report_k5_24.json`, `gpu_ab_report_k10_24.json`,
`mut_catalogue.txt`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers sauf lui-même.
