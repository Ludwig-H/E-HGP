# ehgp.v11.worker_plan.v1
PLAN_PYTHON_PINNED=0
PLAN_DEFAULT_BUILD=0
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=67108864
PLAN_BUILD_TARGETS=()
PLAN_COUNT=2
PLAN_NAME_0=matrice
PLAN_TIMEOUT_0=1500
PLAN_ARGV_0=(python3 '{src}/morsehgp3D_v11/tools/g4_matrix.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/matrix')
PLAN_NAME_1=catalogue
PLAN_TIMEOUT_1=600
PLAN_ARGV_1=(python3 '{src}/morsehgp3D_v11/bench/catalogue_g4.py' --exe '{build}/matrix/gcc_release/build/mhgp11_catalogue_bench' --data '{data}' --out '{out}' --work '{build}/catalogue_measurements' --qualification '{out}/../../000_matrice/files/matrix/summary.json' --timeout 30 --leaf-size 16)
