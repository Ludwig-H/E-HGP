# ehgp.v11.worker_plan.v1
PLAN_PYTHON_PINNED=0
PLAN_DEFAULT_BUILD=0
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=67108864
PLAN_BUILD_TARGETS=()
PLAN_COUNT=1
PLAN_NAME_0=ab
PLAN_TIMEOUT_0=3300
PLAN_ARGV_0=(python3 '{src}/morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/protocol/ab_g4.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/ab' --reps 5 --tests '^mhgp11_' --min-tests 666 --tsan '^mhgp11_tower_(pipeline|population_concurrent)' --min-tsan 7 --mutants tower:pipeline_naissance_sans_cloture,pipeline_bas_ouvert,pipeline_fin_prematuree,pipeline_controles_verticaux_omis,pipeline_graine_reguliere_oubliee,population_lien_noeud_fixe,population_lien_catalogue_non_compte,vertical_parallel_graine_non_elevee,births_blocs_cle_dense,births_blocs_cohortes_omises,births_blocs_etat_partage --w1 --perf-new)
