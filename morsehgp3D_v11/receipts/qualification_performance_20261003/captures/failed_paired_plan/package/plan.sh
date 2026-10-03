# ehgp.v11.worker_plan.v1
PLAN_PYTHON_PINNED=0
PLAN_DEFAULT_BUILD=0
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=134217728
PLAN_BUILD_TARGETS=()
PLAN_COUNT=3
PLAN_NAME_0=prior
PLAN_TIMEOUT_0=90
PLAN_ARGV_0=(python3 '{src}/morsehgp3D_v11/bench/full_paired_prepare.py' --archive /home/louis_hauseux_gmail_com/ehgp-v11/ehgp-v11.DGDqVcxKfB/results.tar.gz --archive-sha256 9c4288f6721015ad6cac302e7536064a74f3458acccc5d6cc7d0c13f4e285421 --out '{out}')
PLAN_NAME_1=bits21
PLAN_TIMEOUT_1=450
PLAN_ARGV_1=(python3 '{src}/morsehgp3D_v11/tools/g4_matrix.py' --src '{src}' --out '{out}' --data '{data}' --work '{build}/matrix' --matrix '{src}/morsehgp3D_v11/bench/full_paired_bits21_matrix.json' --budget-seconds 420)
PLAN_NAME_2=paired_full
PLAN_TIMEOUT_2=1380
PLAN_ARGV_2=(python3 '{src}/morsehgp3D_v11/bench/full_paired.py' --source '{src}' --builds '{build}/matrix' --data '{data}' --out '{out}' --work '{build}/full_paired_measurements' --qualification '{out}/../../001_bits21/files/matrix/summary.json' --prior-context '{out}/../../000_prior/files/prior_context.json' --budget-seconds 1320 --build-timeout-seconds 180 --build-threads 16)
