# ehgp.v11.worker_plan.v1
PLAN_PYTHON_PINNED=0
PLAN_DEFAULT_BUILD=0
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=134217728
PLAN_BUILD_TARGETS=()
PLAN_COUNT=2
PLAN_NAME_0=matrice
PLAN_TIMEOUT_0=1450
PLAN_ARGV_0=(python3 '{src}/morsehgp3D_v11/tools/g4_matrix.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/matrix' --budget-seconds 1380)
PLAN_NAME_1=asan18
PLAN_TIMEOUT_1=420
PLAN_ARGV_1=(python3 '{src}/morsehgp3D_v11/tools/g4_matrix.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/asan18_matrix' --matrix '{src}/morsehgp3D_v11/bench/meb_asan18_matrix.json' --budget-seconds 360)
