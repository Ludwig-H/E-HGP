import importlib.util, sys, json
from pathlib import Path
sys.dont_write_bytecode = True
p = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/bench/run_lidar_scaling.py')
spec = importlib.util.spec_from_file_location('rls', p)
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
case = Path(sys.argv[1])
w = m.g4_reader()
print('worker PROBE_SCHEMA', w.PROBE_SCHEMA, 'reader', m.PROBE_SCHEMA)
# capture worker error on forged v13 value
orig = m.validate_probe
def wrapped(value, expected):
    if value.get('schema') != 'mhgp9_tower_probe_v12':
        case_ = dict(scene='lidar', file='lidar.u32le', n=expected['sites'], k=expected['k'], s=expected['s'],
                     workers=expected['workers'], static_threads=expected['static_threads'], levers=dict(m.DEFAULT_LEVERS), repeat=0)
        try:
            r = w.validate_probe(value, case_, 0, inputs={'lidar': dict(n=expected['sites'], fnv=expected['fnv'])})
            print('worker verdict:', r)
        except Exception as e:
            print('worker error:', type(e).__name__, e)
    return orig(value, expected)
m.validate_probe = wrapped
mode = sys.argv[2]
if mode == 'patch':
    m.PROBE_SCHEMA = w.PROBE_SCHEMA
    m.KNOWN_SCHEMAS = ('mhgp9_tower_probe_v12', w.PROBE_SCHEMA)
print('selftest rc =', m.selftest(case))
