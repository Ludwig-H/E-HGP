#!/usr/bin/env python3
"""Small AST replay of retained placement diagnostics; no native/process simulation."""
import ast
import difflib
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent

def need(condition, detail):
    if not condition:
        raise ValueError(detail)

manifest = json.loads((HERE / 'sources.json').read_text())
sources = {}
for path, digest in manifest['files'].items():
    raw = subprocess.check_output(['git', 'show', manifest['source_commit'] + ':morsehgp3D_v11/' + path])
    need(hashlib.sha256(raw).hexdigest() == digest, 'source hash: ' + path)
    sources[path] = raw.decode()
old = sources['bench/gpu_ab.py']
anchor = '                batch=dict(batch))'
need(old.count(anchor) == 1, 'one summary anchor')
new = old.replace(anchor, "                pipeline_placement_cores=(full.get('pipeline_tasks') or {}).get('placement_cores'),\n" + anchor)
expected = ''.join(difflib.unified_diff(old.splitlines(True), new.splitlines(True),
    fromfile='a/morsehgp3D_v11/bench/gpu_ab.py', tofile='b/morsehgp3D_v11/bench/gpu_ab.py'))
need(expected == (HERE / 'proposal.patch').read_text(), 'exact proposal')
need('forest_timings.pipeline_placement_cores' in sources['bench/full_probe.cpp'], 'field emitted by source')

def summary(source):
    functions = [f for f in ast.parse(source).body if isinstance(f, ast.FunctionDef) and f.name == 'take_summary']
    need(len(functions) == 1, 'one take_summary')
    namespace = {}
    exec(compile(ast.Module(body=functions, type_ignores=[]), 'take_summary_from_source', 'exec'), namespace)
    return namespace['take_summary']

original, proposed = summary(old), summary(new)
zero = {'full': {'status': 'ok', 'pipeline_tasks': {'placement_cores': 0}}}
active = {'full': {'status': 'ok', 'pipeline_tasks': {'placement_cores': 24}}}
unknown = {'full': {'status': 'ok'}}
need(original(zero) == original(active), 'existing summary erases the distinction')
for event, value in ((zero, 0), (active, 24), (unknown, None)):
    result = proposed(event)
    need(result.pop('pipeline_placement_cores') == value, 'preserved observation including missing value')
    need(result == original(event), 'all other summary fields unchanged')
print(json.dumps({'source_commit': manifest['source_commit'], 'original_zero_and_active_summaries_identical': True,
    'proposal_preserves': [None, 0, 24], 'all_other_fields_unchanged': True,
    'native_executions': 0, 'cloud_actions': 0,
    'scope': 'summary observation only; plan core count is not proof of kernel affinity or performance'}, sort_keys=True))
