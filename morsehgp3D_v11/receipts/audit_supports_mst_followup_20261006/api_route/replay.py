#!/usr/bin/env python3
"""S1 borne K2 et condition API corrigee, stdlib ; aucune execution native."""
import argparse
import ast
import hashlib
import io
import json
import subprocess
import sys
import tarfile
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--repo', default=str(Path.cwd()))
parser.add_argument('--capture', action='store_true')
args = parser.parse_args()
meta = json.loads((ROOT / 'sources.json').read_text())
captured = {}
for item in meta['wip_captures']:
    data = (ROOT / item['capture_path']).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != item['sha256'] or digest != item['second_capture_sha256']:
        raise ValueError('WIP snapshot hash')
    captured[item['repo_path']] = data.decode()
paths = [item['repo_path'] for item in meta['git_dependencies']]
archive = subprocess.check_output(['git', 'archive', meta['base_pin'], '--'] + paths, cwd=args.repo)
with tarfile.open(fileobj=io.BytesIO(archive), mode='r:') as tar:
    pinned = {name: tar.extractfile(name).read() for name in paths}
for item in meta['git_dependencies']:
    if hashlib.sha256(pinned[item['repo_path']]).hexdigest() != item['sha256']:
        raise ValueError('Git dependency hash')

# Load only the three source files actually required by the exact S1 definition.
package = types.ModuleType('audit_s1')
package.__path__ = []
sys.modules[package.__name__] = package
for name in ('model', 'definition', 'supports'):
    key = 'morsehgp3D_v11/reference/hgp11_ref/' + name + '.py'
    module = types.ModuleType('audit_s1.' + name)
    module.__package__ = 'audit_s1'
    module.__file__ = key
    sys.modules[module.__name__] = module
    exec(compile(pinned[key], key, 'exec'), module.__dict__)

# This named 3-site fixture is already selected at K2 by test_supports.fixtures.
fixture_tree = ast.parse(pinned['morsehgp3D_v11/reference/test_supports.py'])
fixtures = [node for node in ast.walk(fixture_tree) if isinstance(node, ast.Tuple) and len(node.elts) == 4
            and isinstance(node.elts[0], ast.Constant) and node.elts[0].value == 'triangle_equilateral']
if len(fixtures) != 1:
    raise ValueError('fixture anchor')
name, points, orders, description = ast.literal_eval(fixtures[0])
if 2 not in orders or points != [(0, 0, 0), (2, 2, 0), (2, 0, 2)]:
    raise ValueError('fixture changed')
doc = sys.modules['audit_s1.supports'].Supports(points).canonical(2)
births = [b for b in doc['balls'] if b['role'] == 'naissance']
merges = [b for b in doc['balls'] if b['role'] == 'fusion']
if len(doc['balls']) != 4 or len(births) != 3 or len(merges) != 1:
    raise ValueError('S1 count')
if any(b['p'] != 0 or b['m'] != b['qmin'] for b in doc['balls']):
    raise ValueError('regular fixture')

# Translation of the pinned allocation rule for regular balls (not a native run).
journal_source = pinned['morsehgp3D_v11/src/tower/order_tree.cpp'].decode()
for anchor in ('if (ball.m == ball.qmin)', 'if (k == u64{ball.p} + ball.qmin) continue;',
               'MHGP11_TRY(cell_add(cells, 1));', 'MHGP11_TRY(cell_add(seeds, ball.qmin));'):
    if anchor not in journal_source:
        raise ValueError('journal source anchor')
logged = [b for b in doc['balls'] if 2 != b['p'] + b['qmin']]
cells, seeds = len(logged), sum(b['qmin'] for b in logged)
if cells != 1 or seeds != 3:
    raise ValueError('journal count')
probe_source = pinned['morsehgp3D_v11/tests/api/supports_route.cpp'].decode()
if '\"cells\\\":\" << b.diagnostics.log.cells' not in probe_source:
    # Check the actual C++ stream text, whose quotes are escaped in its string literal.
    if '\\"cells\\\":\" << b.diagnostics.log.cells' not in probe_source:
        raise ValueError('probe field source')

# Evaluate the captured condition itself; bytes=1 is a positive sentinel, not a file size measurement.
judge = captured['morsehgp3D_v11/tests/api/supports_route_oracle.py']
tree = ast.parse(judge)
candidates = [node.args[0] for node in ast.walk(tree) if isinstance(node, ast.Call)
              and isinstance(node.func, ast.Attribute) and node.func.attr == 'check' and node.args
              and "row['supports']" in ast.unparse(node.args[0]) and "row['balls']" in ast.unparse(node.args[0])]
if len(candidates) != 1:
    raise ValueError('count condition')
condition = candidates[0]
condition_text = ast.unparse(condition)
if "row['cells']" in condition_text:
    raise ValueError('obsolete comparison still present')
row = dict(balls=4, supports=4, cells=1, multiple=0, bytes=1, nodes=4)
compiled = compile(ast.Expression(condition), 'captured_count_condition', 'eval')
fixed_accepts = eval(compiled, {}, {'row': row})
bad_supports = eval(compiled, {}, {'row': dict(row, supports=3)})
bad_multiple = eval(compiled, {}, {'row': dict(row, multiple=1)})
old_comparison = eval(compile(meta['previous_condition'], 'previous_comparison_observed', 'eval'), {}, {'row': row})
if not fixed_accepts or bad_supports or bad_multiple or old_comparison:
    raise ValueError('condition verdict')
result = dict(schema='audit_api_route_counts_source_corrected_v1', base_pin=meta['base_pin'],
    status=meta['status'], double_capture_equal=True, fixture=name, points=points, k=2,
    s1_balls=4, s1_birth_balls=3, s1_merge_balls=1,
    journal_cells_from_pinned_rule=cells, journal_seeds_from_pinned_rule=seeds,
    s1_ball_summary=[{key: b[key] for key in ('role', 'level', 'p', 'm', 'qmin', 'prior')} for b in doc['balls']],
    captured_judge_sha256=hashlib.sha256(judge.encode()).hexdigest(), actual_condition=condition_text,
    actual_condition_on_s1_counts=fixed_accepts, supports_mismatch_accepted=bad_supports,
    multiple_supports_accepted=bad_multiple, previous_comparison_on_s1_counts=old_comparison,
    previous_condition_provenance=meta['previous_condition_provenance'],
    positive_bytes_field='Sentinel 1 used only for the Python Boolean condition; no native byte size observed.',
    native_runs=0, cloud_actions=0, limit='Source-level closure of the obsolete count check, not native API publication qualification.')
output = json.dumps(result, indent=2, sort_keys=True) + '\n'
if args.capture:
    (ROOT / 'summary.json').write_text(output)
elif (ROOT / 'summary.json').read_text() != output:
    raise ValueError('summary changed')
print(output, end='')
