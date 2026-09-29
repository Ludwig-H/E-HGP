# ehgp.v10.worker_plan.v2
PLAN_NEEDS_PYTHON=1
PLAN_BUILD_TIMEOUT=600
PLAN_RESULTS_CAP=1073741824
PLAN_BUILD_TARGETS=(mhgp10_catalogue mhgp10_tower mhgp10_cluster mhgp10_unit)
PLAN_COUNT=20
PLAN_NAME_0=gates_cpp
PLAN_TIMEOUT_0=300
PLAN_ARGV_0=(ctest --no-tests=error -L fast --output-on-failure)
PLAN_NAME_1=cat_lidar02_k5_w1
PLAN_TIMEOUT_1=420
PLAN_ARGV_1=(./mhgp10_catalogue '{data}/lidar02_full.u32le' --k=5 --threads=1)
PLAN_NAME_2=cat_lidar02_k5_w12
PLAN_TIMEOUT_2=240
PLAN_ARGV_2=(./mhgp10_catalogue '{data}/lidar02_full.u32le' --k=5 --threads=12)
PLAN_NAME_3=cat_lidar02_k5_w24
PLAN_TIMEOUT_3=240
PLAN_ARGV_3=(./mhgp10_catalogue '{data}/lidar02_full.u32le' --k=5 --threads=24)
PLAN_NAME_4=cat_lidar02_k5_w48
PLAN_TIMEOUT_4=240
PLAN_ARGV_4=(./mhgp10_catalogue '{data}/lidar02_full.u32le' --k=5 --threads=48)
PLAN_NAME_5=cat_lidar02_k10_w24
PLAN_TIMEOUT_5=300
PLAN_ARGV_5=(./mhgp10_catalogue '{data}/lidar02_full.u32le' --k=10 --threads=24)
PLAN_NAME_6=cat_lidar02_k10_w48
PLAN_TIMEOUT_6=300
PLAN_ARGV_6=(./mhgp10_catalogue '{data}/lidar02_full.u32le' --k=10 --threads=48)
PLAN_NAME_7=tower_lidar00_k5_w48
PLAN_TIMEOUT_7=300
PLAN_ARGV_7=(./mhgp10_tower '{data}/lidar00_full.u32le' --k=5 --threads=48 --no-points --repeat=3)
PLAN_NAME_8=tower_lidar00_k10_w48
PLAN_TIMEOUT_8=300
PLAN_ARGV_8=(./mhgp10_tower '{data}/lidar00_full.u32le' --k=10 --threads=48 --no-points --repeat=3)
PLAN_NAME_9=tower_lidar01_k5_w48
PLAN_TIMEOUT_9=300
PLAN_ARGV_9=(./mhgp10_tower '{data}/lidar01_full.u32le' --k=5 --threads=48 --no-points --repeat=3)
PLAN_NAME_10=tower_lidar01_k10_w48
PLAN_TIMEOUT_10=300
PLAN_ARGV_10=(./mhgp10_tower '{data}/lidar01_full.u32le' --k=10 --threads=48 --no-points --repeat=3)
PLAN_NAME_11=tower_lidar02_k5_w48
PLAN_TIMEOUT_11=300
PLAN_ARGV_11=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=5 --threads=48 --no-points --repeat=3)
PLAN_NAME_12=tower_lidar02_k10_w48
PLAN_TIMEOUT_12=300
PLAN_ARGV_12=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=10 --threads=48 --no-points --repeat=3)
PLAN_NAME_13=tower_lidar00_k5_cover_w48
PLAN_TIMEOUT_13=240
PLAN_ARGV_13=(./mhgp10_tower '{data}/lidar00_full.u32le' --k=5 --threads=48 --entry=cover --repeat=3)
PLAN_NAME_14=cluster_lidar00_k5_cover_w48
PLAN_TIMEOUT_14=240
PLAN_ARGV_14=(./mhgp10_cluster '{data}/lidar00_full.u32le' '{out}/labels.i32le' --k=5 --mcs=200 --z=3 --selection=eom --entry=cover --threads=48)
PLAN_NAME_15=tower_lidar01_k5_cover_w48
PLAN_TIMEOUT_15=240
PLAN_ARGV_15=(./mhgp10_tower '{data}/lidar01_full.u32le' --k=5 --threads=48 --entry=cover --repeat=3)
PLAN_NAME_16=cluster_lidar01_k5_cover_w48
PLAN_TIMEOUT_16=240
PLAN_ARGV_16=(./mhgp10_cluster '{data}/lidar01_full.u32le' '{out}/labels.i32le' --k=5 --mcs=200 --z=3 --selection=eom --entry=cover --threads=48)
PLAN_NAME_17=tower_lidar02_k5_cover_w48
PLAN_TIMEOUT_17=240
PLAN_ARGV_17=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=5 --threads=48 --entry=cover --repeat=3)
PLAN_NAME_18=cluster_lidar02_k5_cover_w48
PLAN_TIMEOUT_18=240
PLAN_ARGV_18=(./mhgp10_cluster '{data}/lidar02_full.u32le' '{out}/labels.i32le' --k=5 --mcs=200 --z=3 --selection=eom --entry=cover --threads=48)
PLAN_NAME_19=tower_lidar02_k5_w24
PLAN_TIMEOUT_19=240
PLAN_ARGV_19=(./mhgp10_tower '{data}/lidar02_full.u32le' --k=5 --threads=24 --no-points --repeat=3)
