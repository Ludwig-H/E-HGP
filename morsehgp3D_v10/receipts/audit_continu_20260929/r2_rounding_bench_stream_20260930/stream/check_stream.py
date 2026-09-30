"""Tiny adversarial comparison; no engine/native calls or file writes."""
from collections import Counter
from pathlib import Path
import hashlib
import importlib.util
import json
import sys
import time

ROOT = Path(__file__).resolve().parent
def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

paths = [Path(__file__).resolve()] + sorted((ROOT/'sources').rglob('*.py')) + sorted((ROOT/'fixtures').glob('*.txt'))
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
before = hashes()
started = time.monotonic()
stream = load('stream', ROOT/'sources/invariants_echelle.py')
strict = load('strict', ROOT/'sources/tests/oracle/test_tower_oracle.py')
P = [(0,0,0),(2,0,0),(5,0,0)]
rows = {}
for name in ('baseline','complete_order_prefix','foreign_point_ids','false_header_counts','missing_point_control'):
    path = ROOT/'fixtures'/f'{name}.txt'
    counts = Counter()
    weak = stream.tower_stream(path,len(P),counts)
    strong = strict.judge(P,strict.parse(path),2)
    rows[name] = {'stream_error':weak,'strict_error':strong,'stream_counts':dict(counts)}
    if name == 'baseline' and (weak is not None or strong is not None):
        raise RuntimeError('valid baseline rejected')
    if name in ('complete_order_prefix','foreign_point_ids') and not (weak is None and strong is not None):
        raise RuntimeError('counterexample not reproduced '+name)
    if name == 'false_header_counts' and weak is not None:
        raise RuntimeError('header probe changed')
    if name == 'missing_point_control' and (weak is None or strong is None):
        raise RuntimeError('positive control missed')
after = hashes()
if before != after:
    raise RuntimeError('frozen files changed')
print(json.dumps({'status':'STREAM_SCOPE_COUNTEREXAMPLES_REPRODUCED',
                  'scope':'tiny_archived_core_dump_K2_comparison_not_new_producer',
                  'optimized':sys.flags.optimize,'rows':rows,
                  'hashes_before':before,'hashes_after':after,
                  'engine_calls':0,'engine_modified':False,'GCP_used':False,
                  'wall_seconds':time.monotonic()-started},indent=2))
