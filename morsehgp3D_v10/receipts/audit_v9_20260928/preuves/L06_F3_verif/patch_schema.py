import importlib.util, sys
from pathlib import Path
sys.dont_write_bytecode = True
W = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9')
spec = importlib.util.spec_from_file_location('rls', W / 'bench/run_lidar_scaling.py')
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
new = sys.argv[1]
m.PROBE_SCHEMA = new
m.KNOWN_SCHEMAS = ('mhgp9_tower_probe_v12', new)
case = W / 'receipts/lidar_scaling_local_20260923/out/s01_k5_w8_r0/s01_k5_s8_w8_r0_piece_quarter_x_nonneg_y_nonneg.json'
print('rc', m.selftest(case))
