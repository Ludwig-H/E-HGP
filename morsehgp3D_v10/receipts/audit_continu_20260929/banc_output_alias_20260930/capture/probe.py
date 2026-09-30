"""Tiny output-alias countertest; no real measurement or child process."""
import contextlib
import csv
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
from types import SimpleNamespace

SOURCE = Path('/workspaces/E-HGP/build/v10-integration-r2/src/morsehgp3D_v10/bench/scaling/scale_run.py')
HERE = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


before = sha(SOURCE)
spec = importlib.util.spec_from_file_location('scale_alias_fixture', SOURCE)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fake_measure(build, path, k, threads):
    calls = []
    for name in ('catalogue', 'tower'):
        native = json.dumps(dict(status='ok', balls=7, fixture=name)) + '\n'
        calls.append(dict(call=name, argv=['NOT_EXECUTED'], code=0, timed_out=False,
                          wall_s=0.01, cpu_s=0.01, max_rss_kb=1, stdout=native, stderr=''))
    return dict(k=k, threads=threads, status='ok', balls=7, tower_s=0.01), calls


module.measure = fake_measure
results = {}
for mode in ('distinct', 'exact_alias'):
    out = HERE / (mode + '.csv')
    calls = out if mode == 'exact_alias' else HERE / (mode + '.jsonl')
    args = SimpleNamespace(data=str(HERE), build='NOT_USED', k='5', threads=1,
                           timeout=None, budget=None, only='', out=str(out), calls=str(calls))
    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        code = module.cmd_run(args)
    try:
        records = [json.loads(s) for s in calls.read_text().splitlines() if s.strip()]
        journal_valid = len(records) == 2 and [c['call'] for c in records] == ['catalogue', 'tower']
    except (ValueError, TypeError, KeyError):
        journal_valid = False
    rows = list(csv.DictReader(io.StringIO(out.read_text())))
    csv_valid = len(rows) == 1 and rows[0].get('status') == 'ok'
    results[mode] = dict(code=code, journal_valid=journal_valid, csv_valid=csv_valid,
                         runner_stdout=captured.getvalue(), file_sha256=sha(out))
require(results['distinct']['code'] == 0 and results['distinct']['journal_valid'] and
        results['distinct']['csv_valid'], 'positive control failed')
require(results['exact_alias']['code'] == 0 and not results['exact_alias']['journal_valid'],
        'exact-alias corruption absent')
after = sha(SOURCE)
require(before == after, 'source changed during countertest')
print(json.dumps(dict(status='OUTPUT_ALIAS_COUNTEREXAMPLE', optimize=sys.flags.optimize,
                     source=str(SOURCE), source_before=before, source_after=after,
                     results=results, native_executions=0, real_measurements=0,
                     scope='actual cmd_run with simulated measure; no HGP or GCP'), sort_keys=True))
