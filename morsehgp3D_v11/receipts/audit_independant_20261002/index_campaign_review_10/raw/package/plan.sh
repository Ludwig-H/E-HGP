# ehgp.v11.worker_plan.v1
PLAN_PYTHON_PINNED=0
PLAN_DEFAULT_BUILD=0
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=67108864
PLAN_BUILD_TARGETS=()
PLAN_COUNT=3
PLAN_NAME_0=matrice
PLAN_TIMEOUT_0=600
PLAN_ARGV_0=(python3 '{src}/morsehgp3D_v11/tools/g4_matrix.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/matrix' --budget-seconds 550)
PLAN_NAME_1=asan18
PLAN_TIMEOUT_1=150
PLAN_ARGV_1=(python3 '{src}/morsehgp3D_v11/tools/g4_matrix.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/asan18_matrix' --matrix '{src}/morsehgp3D_v11/bench/index_asan18_matrix.json' --budget-seconds 130)
PLAN_NAME_2=index
PLAN_TIMEOUT_2=800
PLAN_ARGV_2=(python3 '{src}/morsehgp3D_v11/bench/index_g4.py' --builds '{build}/matrix' --data '{data}' --out '{out}' --work '{build}/index_measurements' --qualification '{out}/../../000_matrice/files/matrix/summary.json' --supplement '{out}/../../001_asan18/files/matrix/summary.json')
