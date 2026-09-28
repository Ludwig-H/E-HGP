import importlib.util, sys, traceback
from pathlib import Path
sys.dont_write_bytecode = True
W = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9')
spec = importlib.util.spec_from_file_location('rls', W / 'bench/run_lidar_scaling.py')
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
new = sys.argv[1]
m.PROBE_SCHEMA = new
m.KNOWN_SCHEMAS = ('mhgp9_tower_probe_v12', new)
real = m.g4_reader()
class Wrap:
    def __getattr__(self, k): return getattr(real, k)
    def validate_probe(self, *a, **kw):
        try:
            r = real.validate_probe(*a, **kw); print('worker ->', r); return r
        except Exception as e:
            print('worker raised', type(e).__name__, e); raise
w = Wrap()
m.g4_reader = lambda: w
case = W / 'receipts/lidar_scaling_local_20260923/out/s01_k5_w8_r0/s01_k5_s8_w8_r0_piece_quarter_x_nonneg_y_nonneg.json'
print('rc', m.selftest(case))
