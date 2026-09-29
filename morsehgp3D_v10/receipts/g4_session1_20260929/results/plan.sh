# ehgp.v10.worker_plan.v2
PLAN_NEEDS_PYTHON=1
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=1073741824
PLAN_BUILD_TARGETS=()
PLAN_COUNT=20
PLAN_NAME_0=gates
PLAN_TIMEOUT_0=900
PLAN_ARGV_0=(ctest --no-tests=error -L gate -j 16 --output-on-failure)
PLAN_NAME_1=c0_lidar00_k5_w48
PLAN_TIMEOUT_1=240
PLAN_ARGV_1=(./mhgp10_tower '{data}/lidar00_full.u32le' --k=5 --threads=48 --no-points --repeat=3)
PLAN_NAME_2=c0_lidar01_k5_w48
PLAN_TIMEOUT_2=240
PLAN_ARGV_2=(./mhgp10_tower '{data}/lidar01_full.u32le' --k=5 --threads=48 --no-points --repeat=3)
PLAN_NAME_3=c0_lidar02_k5_w48
PLAN_TIMEOUT_3=240
PLAN_ARGV_3=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=5 --threads=48 --no-points --repeat=3)
PLAN_NAME_4=c0_lidar00_k10_w48
PLAN_TIMEOUT_4=420
PLAN_ARGV_4=(./mhgp10_tower '{data}/lidar00_full.u32le' --k=10 --threads=48 --no-points --repeat=2)
PLAN_NAME_5=c0_lidar01_k10_w48
PLAN_TIMEOUT_5=420
PLAN_ARGV_5=(./mhgp10_tower '{data}/lidar01_full.u32le' --k=10 --threads=48 --no-points --repeat=2)
PLAN_NAME_6=c0_lidar02_k10_w48
PLAN_TIMEOUT_6=420
PLAN_ARGV_6=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=10 --threads=48 --no-points --repeat=2)
PLAN_NAME_7=c0_lidar02_k5_w24
PLAN_TIMEOUT_7=240
PLAN_ARGV_7=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=5 --threads=24 --no-points --repeat=3)
PLAN_NAME_8=c0_lidar02_k5_w1
PLAN_TIMEOUT_8=420
PLAN_ARGV_8=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=5 --threads=1 --no-points --repeat=1)
PLAN_NAME_9=c1_lidar00_k5_cover_w48
PLAN_TIMEOUT_9=240
PLAN_ARGV_9=(./mhgp10_tower '{data}/lidar00_full.u32le' --k=5 --threads=48 --entry=cover --repeat=2)
PLAN_NAME_10=c1_lidar00_k10_cover_w48
PLAN_TIMEOUT_10=420
PLAN_ARGV_10=(./mhgp10_tower '{data}/lidar00_full.u32le' --k=10 --threads=48 --entry=cover --repeat=2)
PLAN_NAME_11=c1_lidar01_k5_cover_w48
PLAN_TIMEOUT_11=240
PLAN_ARGV_11=(./mhgp10_tower '{data}/lidar01_full.u32le' --k=5 --threads=48 --entry=cover --repeat=2)
PLAN_NAME_12=c1_lidar01_k10_cover_w48
PLAN_TIMEOUT_12=420
PLAN_ARGV_12=(./mhgp10_tower '{data}/lidar01_full.u32le' --k=10 --threads=48 --entry=cover --repeat=2)
PLAN_NAME_13=c1_lidar02_k5_cover_w48
PLAN_TIMEOUT_13=240
PLAN_ARGV_13=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=5 --threads=48 --entry=cover --repeat=2)
PLAN_NAME_14=c1_lidar02_k10_cover_w48
PLAN_TIMEOUT_14=420
PLAN_ARGV_14=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=10 --threads=48 --entry=cover --repeat=2)
PLAN_NAME_15=c2_lidar00_cluster_k5_cover_w48
PLAN_TIMEOUT_15=240
PLAN_ARGV_15=(./mhgp10_cluster '{data}/lidar00_full.u32le' '{out}/labels.i32le' --k=5 --mcs=200 --z=2 --selection=eom --entry=cover --threads=48)
PLAN_NAME_16=c2_lidar01_cluster_k5_cover_w48
PLAN_TIMEOUT_16=240
PLAN_ARGV_16=(./mhgp10_cluster '{data}/lidar01_full.u32le' '{out}/labels.i32le' --k=5 --mcs=200 --z=2 --selection=eom --entry=cover --threads=48)
PLAN_NAME_17=c2_lidar02_cluster_k5_cover_w48
PLAN_TIMEOUT_17=240
PLAN_ARGV_17=(./mhgp10_cluster '{data}/lidar02_full.u32le' '{out}/labels.i32le' --k=5 --mcs=200 --z=2 --selection=eom --entry=cover --threads=48)
PLAN_NAME_18=scale_lidar_k5_k10_w48
PLAN_TIMEOUT_18=1200
PLAN_ARGV_18=(python3 '{src}/morsehgp3D_v10/bench/scaling/scale_run.py' run --build '{build}' --data '{data}' --out '{out}/scale_lidar.csv' --k 5,10 --threads 48 --only lidar)
PLAN_NAME_19=scale_syn_k5_w48
PLAN_TIMEOUT_19=900
PLAN_ARGV_19=(python3 '{src}/morsehgp3D_v10/bench/scaling/scale_run.py' run --build '{build}' --data '{data}' --out '{out}/scale_syn_k5.csv' --k 5 --threads 48 --only syn_)
