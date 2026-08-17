# Addendum — le cœur de seed bascule sur le cover aplati (×12 sur t_core, tranché par la mesure)

Date : 17 août 2026 (après redémarrage du conteneur). Base : `9549659`
(parallélisme) ; code livré dans le commit portant ce reçu.

## La mesure qui tranche

Le poste dominant restant était la descente d'antichaîne du cœur de
seed (~127 nœuds/seed, bornes `axis_min`/`axis_max` i128 par nœud).
L'audit b8c4a4d § 3 suggérait le traitement groupé par ancre ; sa forme
la plus simple est de compter les témoins sur le COVER APLATI de
l'ancre (déjà matérialisé par `anchor_cover_from_handles`), avec sortie
anticipée à $h_4$ — une puissance q3 par site examiné, aucun nœud,
aucune borne par boîte. Les deux variantes ont tourné côte à côte :

| eight_clusters n=1000 axial, 1 fil | arbre (avant) | cover aplati |
|---|---|---|
| t_core | 27,0 s | **2,24 s** (×12) |
| seeds tués par le cœur | 3 994 641 | **3 994 641 (identique)** |
| sites/nœuds examinés | 562 M nœuds | 134 M sites |
| sortie (émissions, boules, événements) | — | **identique champ par champ** |

uniform n=1600 : t_core 16,0 → 1,40 s, kills identiques (2 495 175),
532 181 événements. Le sous-univers du cover (⊂ handles) n'a rien
coûté : PAS UN SEUL kill perdu sur les deux familles — les témoins de
Jung vivent dans le cover de l'ancre. Le minorant reste fail-open par
construction (un sous-compte ne tue jamais à tort).

La variante d'arbre est SUPPRIMÉE (un seul chemin, pas d'espace de
configuration non testé) ; l'en-tête « historique des variantes » de
`ball_stream.hpp` grave les trois tentatives perdantes (descente à
boîtes ×12 plus lente, budget d'atteignabilité neutre, ordre de visite
négatif). Le compteur devient `seed_core_sites` (`sites_core=` dans le
probe).

## Validation à l'échelle d'intérêt : n=8000 au compte près

uniform n=8000, s=8, smax=11, seed=3, `--threads=4` — contre le run
gravé de la campagne locale (`cov_uniform_n8000_smax11.txt`) :

- IDENTIQUES : candidats 313095/1425847/1396285, boules uniques
  3 134 427, mortes de profondeur 31 176, census 16 052 179 /
  10 447 608, événements 3 126 158, fusions 19 465 140, nœuds
  1 974 086.
- `gen_tues` q4 : 139 744 809 → 15 084 458 — le cœur tue 17,3 M de
  seeds AVANT que leurs complétions ne soient évaluées ; comme les
  émissions n'ont pas bougé, c'est la preuve à l'échelle que les
  complétions supprimées étaient exactement celles que `depth_dead`
  aurait tuées une à une.
- `t_gen` : 240,1 s (1 fil, sans cœur) → **58,8 s** (4 fils, cœur
  aplati — ×4,1), 105 CTest verts, 7 mutants axiaux re-tués.

## Le nouveau goulot est l'AVAL

À n=8000 la génération n'est plus dominante : `t_fold = 69,2 s`,
`t_census = 27,5 s`, `t_prefiltre = 22,8 s` (séquentiels) contre
`t_gen = 58,8 s`. Prochaine unité : paralléliser l'aval — préfiltre et
census sont indépendants PAR CLÉ de boule, les folds par K sont des
consommateurs du même flux trié. Puis l'écriture GPU des noyaux
réguliers (cœur aplati — désormais un scan pur, encore plus
GPU-friendly que la descente —, primitive de sweep).
