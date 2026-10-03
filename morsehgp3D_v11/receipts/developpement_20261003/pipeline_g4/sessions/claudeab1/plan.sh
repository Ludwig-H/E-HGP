# ehgp.v11.worker_plan.v1
PLAN_PYTHON_PINNED=0
PLAN_DEFAULT_BUILD=0
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=67108864
PLAN_BUILD_TARGETS=()
PLAN_COUNT=1
PLAN_NAME_0=ab
PLAN_TIMEOUT_0=3300
PLAN_ARGV_0=(python3 '{src}/morsehgp3D_v11/bench/claude/ab_full_g4.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/ab' --reps 5 --tests '^mhgp11_' --mutants 'num:filtre_seuil_trop_bas,filtre_signe_inverse,filtre_conversion_sans_decalage,filtre_bornes_permutees,bounds_anchor_inside_ignored,bounds_linear_upper_inverted_u24;tower:population_lien_noeud_fixe,population_lien_catalogue_non_compte' --w1 --perf-new)
