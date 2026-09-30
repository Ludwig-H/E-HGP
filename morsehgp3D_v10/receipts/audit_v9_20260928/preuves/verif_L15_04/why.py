import importlib.util, sys
from pathlib import Path
W = Path('/workspaces/E-HGP/build/v9-open-worktree')
spec = importlib.util.spec_from_file_location('rls', W / 'morsehgp3D_v9/bench/run_lidar_scaling.py')
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
r = m.g4_reader()
orig = r.validate_probe
def wrapped(*a, **k):
    try:
        return orig(*a, **k)
    except Exception as e:
        print('WORKER_REFUSAL:', type(e).__name__, e)
        raise
r.validate_probe = wrapped
print('run_lidar_scaling PROBE_SCHEMA', m.PROBE_SCHEMA, 'worker PROBE_SCHEMA', r.PROBE_SCHEMA)
sys.exit(m.selftest(W / 'morsehgp3D_v9/receipts/lidar_scaling_local_20260923/out/s01_k5_w8_r0/s01_k5_s8_w8_r0_piece_quarter_x_nonneg_y_nonneg.json'))
