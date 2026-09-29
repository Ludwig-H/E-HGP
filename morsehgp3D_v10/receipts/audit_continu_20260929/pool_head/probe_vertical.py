import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys

if len(sys.argv) != 4:
    raise SystemExit('usage: probe_vertical.py TOWER_BINARY V10_SOURCE NEW_OUTPUT_DIRECTORY')
base = Path(sys.argv[2]).resolve()
work = Path(sys.argv[3]).resolve()
work.mkdir(parents=True, exist_ok=False)
source = base / 'tests/oracle/test_tower_oracle.py'
spec = importlib.util.spec_from_file_location('gate', source)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
binary = Path(sys.argv[1])
pts = [(0, 0, 0), (2, 0, 0), (5, 0, 0)]
payload = b''.join(struct.pack('<III', *p) for p in pts)
(work / 'points.u32le').write_bytes(payload)
argv = [str(binary), str(work / 'points.u32le'), '--k=2', '--threads=1', '--dump=' + str(work / 'tower.txt')]
run = subprocess.run(argv, capture_output=True, text=True)
(work / 'tower.stdout').write_text(run.stdout)
(work / 'tower.stderr').write_text(run.stderr)
if run.returncode:
    raise RuntimeError('native failed')
orders = gate.parse(work / 'tower.txt')
levels = sorted({lv for _, lv, _ in orders[2]['nodes']} | {e for _, _, e in orders[2]['points']})
before = gate.vertical_check(orders, 2, pts, levels)
mutated = copy.deepcopy(orders)
bad_v = next(v for v, (_, lv, _) in enumerate(mutated[2]['nodes']) if lv == 1)
wrong_lower = next(v for pt, v, _ in mutated[1]['points'] if pt == (5, 0, 0))
par, lv, low = mutated[2]['nodes'][bad_v]
mutated[2]['nodes'][bad_v] = (par, lv, wrong_lower)
after = gate.vertical_check(mutated, 2, pts, levels)
right_component = gate.top(orders[1]['nodes'], low, lv, True)
wrong_component = gate.top(orders[1]['nodes'], wrong_lower, lv, True)
if before[0] is not None or after[0] is not None or right_component == wrong_component:
    raise RuntimeError('mutation not proved')
report = dict(argv=argv, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
              gate_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
              baseline_gate=before, mutated_gate=after, mutated_order=2, mutated_node=bad_v,
              wrong_lower=wrong_lower, original_lower=low, birth_level=str(lv),
              right_component=right_component, wrong_component=wrong_component,
              outcome='wrong_vertical_survives_published_gate')
(work / 'vertical_report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
