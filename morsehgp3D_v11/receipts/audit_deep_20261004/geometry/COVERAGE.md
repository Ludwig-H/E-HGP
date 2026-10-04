# Matrice de portée

| Invariant | Relecture source e02 | Nouvelle preuve bornée / limites |
|---|---|---|
| Centres q1..4, N/D, tags et poids | sphere/geometry/centers/predicates et helpers complets | Cramer/Gram q3, fixture propriétaire ; ancienne preuve q4 et portes natives inchangées, non rejouées |
| Budgets18/21/24, sommes intermédiaires | budgets/integer/wide/certificats/poids | table types symboliques, majorant du cubeu18 et 336 présentations ; pas sizeof/RSS/chrono |
| Puissance/bornes, contact strict | predicates/power_checked/index census | exemple d'UB lâche sûr déjà connu ; tous contacts restent ambigus/collectés |
| Tri exact/F3/F4 et FENV | level/sort_level_key/sort_indices | lecture ; portes FENV anciennes inchangées, aucun nouveau natif |
| Propriétaire/lifetime et saturation index | tous6 fichiers index ; Cloud/Buffer en dépendance | lecture : owned vs callback/workspace, seuil strict/shell entière ; aucun test géant |
| G1/G2 et régions fermées/demi-ouvertes | boxes/center_region/leaf/support | modèle DFS8sites K3 ; témoins sur z=lo conservés ; aucun modèle de gains LiDAR |
| G3, paires et J2/replis | leaf/pairgraph/cache, tests ciblés pairgraph/support/same-work | modèle des sous-préfixes et paires de la fixture ; compte logique explicité |
| Fronts/propriété/jobs, mono/parallèle | frontier/adaptive_prepare/replay/parallel/single_pass/internal/dispatch | lecture : plan possédé, quotas, scratch privé, jonction avant compactage, refus ; pas pthread/TSan joué |
| Deux passes/compaction/tri/assemblage | assemble/assembly_parallel/single_pass_storage | lecture : sorties privées, égalité ledgers deuxpasses, rebasageu64 et capacités ; timings ne deviennent pas CPU/RSS |
| V10 vs v11 | cinq sources v10 pin777 | niveau différé et T6 seulement ; anciennes mesures/XYZ recoupés par evidence_head, aucun dump interversions nouveau |

Les 106 fichiers sous tests/num,index,catalogue sont capturés. Relecture ciblée des juges/pins/gates et tests qui touchent les invariants ci-dessus ; pas assertion que chaque ligne de chaque script a été relue. Le diff Git 0f→e02 des trois modules et de ces tests est vide : la capsule précédente reste la revue complémentaire du même code. Les résultats G4 appartiennent exclusivement à leurs lots et empreintes, pas à ce replay Python.
