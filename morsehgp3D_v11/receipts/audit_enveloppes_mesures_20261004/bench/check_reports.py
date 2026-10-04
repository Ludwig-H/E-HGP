#!/usr/bin/env python3
"""Exact frozen Python AST; only synthetic JSON/files and fake subprocess.run.

No native executable, fit, build, cloud access or product import is performed.
"""
import ast
import contextlib
from fractions import Fraction
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'source/morsehgp3D_v11/bench'
PIN = '66372e621dcee58daaa7d7309875ab157894acf4'
checks = 0


def need(value, message):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(message)


def load_exact(name, fake_subprocess=None):
    path = SOURCE / name
    tree = ast.parse(path.read_bytes(), filename=str(path))
    # Exclude only the module's __main__ invocation and, for timing, bind a fake subprocess.
    body = [n for n in tree.body if not isinstance(n, ast.If)]
    if fake_subprocess is not None:
        body = [n for n in body if not isinstance(n, ast.Import) or
                not any(a.name == 'subprocess' for a in n.names)]
    env = {'__name__': 'frozen_auditor', 'subprocess': fake_subprocess}
    exec(compile(ast.Module(body=body, type_ignores=[]), str(path), 'exec'), env)
    return env


def invoke(main, argv):
    old = sys.argv
    out, err = io.StringIO(), io.StringIO()
    sys.argv = argv
    try:
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = main()
        return {'exit_code': code, 'stdout': out.getvalue(), 'stderr': err.getvalue()}
    finally:
        sys.argv = old


ab = load_exact('ab_summary.py')
producer_ast = ast.parse((SOURCE / 'ab_g4.py').read_bytes())
producer_schemas = [v.value for n in ast.walk(producer_ast) if isinstance(n, ast.Dict)
                   for k, v in zip(n.keys, n.values) if isinstance(k, ast.Constant) and k.value == 'schema'
                   and isinstance(v, ast.Constant) and isinstance(v.value, str)]
need(producer_schemas == ['ehgp.v11.claude_ab.v3'], 'unique literal schema matches frozen producer domain')
report_schema = producer_schemas[0]
need(ab['median']([2, 4]) == 3 and ab['median']([3, 1, 2]) == 2, 'paired median handles odd/even')
need(ab['sign_test'](5, 0) == float(Fraction(1, 16)), 'p minimum with five pairs')
need(ab['sign_test'](6, 0) == float(Fraction(1, 32)), 'six-pair p minimum')
need(ab['sign_test'](0, 0) is None and ab['sign_test'](2, 2) == 1, 'ties and balanced signs')
for n in range(1, 13):
    for wins in range(n + 1):
        losses = n - wins
        need(ab['sign_test'](wins, losses) == ab['sign_test'](losses, wins), 'two-sided symmetry')


def take(rep, variant, duration, good=True, dump='a' * 64):
    return dict(frame='ng00', workers='48', rep=rep, variant=variant, code=0 if good else 1,
                quiet=True, dump_sha256=dump, summary=dict(status='ok' if good else 'resource_exhausted',
                exit='ok' if good else 'refus', wall_ns=duration, forest_ns=duration // 2,
                domain_ns=duration // 2, domain_detail=dict(single_pass_ns=duration // 3)))


ab_results = {}
for fixture in ('valid_all_five', 'canonical_output_refused', 'three_new_takes_failed'):
    timings = []
    for rep in range(5):
        timings.append(take(rep, 'base', 200_000_000))
        timings.append(take(rep, 'new', 100_000_000,
                            good=fixture != 'three_new_takes_failed' or rep < 2,
                            dump='b' * 64 if fixture == 'canonical_output_refused' else 'a' * 64))
    report = dict(schema=report_schema, status='done', verdict='conforme' if fixture == 'valid_all_five' else 'refus',
                  refusals=[] if fixture == 'valid_all_five' else ['native output identity or a take failed'],
                  identity={'ng00': 'a' * 64}, timings=timings)
    with tempfile.TemporaryDirectory(prefix='ehgp-synthetic-ab-') as td:
        path = Path(td) / ('fixture_' + fixture + '.json')
        path.write_text(json.dumps(report, indent=2) + '\n')
        execution = invoke(ab['main'], ['ab_summary.py', str(path), '--pairs', 'base:new', '--metrics', 'wall'])
        paired = json.loads(Path(str(path) + '.paired.json').read_text())
    need(execution['exit_code'] == 0 and not execution['stderr'], 'reader documents code zero as read, not qualification')
    need(len(paired['rows']) == 1, 'one descriptive row')
    row = paired['rows'][0]
    need(row['median_ratio'] == 0.5 and row['ratio_of_medians'] == 0.5, 'paired ratios and ns-to-ms units')
    need(row['median_a_ms'] == 200 and row['median_b_ms'] == 100, 'native wall nanoseconds to milliseconds')
    need(row['pairs'] == (2 if fixture == 'three_new_takes_failed' else 5), 'successful subset silently selected')
    need(row['sign_p'] == (0.5 if fixture == 'three_new_takes_failed' else 0.0625), 'exact sign p on retained subset')
    need(set(paired) == {'schema', 'rows'}, 'paired artifact retains no parent verdict or exclusions')
    ab_results[fixture] = {'input_verdict': report['verdict'], 'input_refusals_count': len(report['refusals']),
                           'input_takes': 10, 'failed_takes': sum(t['code'] != 0 for t in timings),
                           'reader_exit': execution['exit_code'], 'row': row,
                           'paired_fields': sorted(paired)}


successful_stdout = '\n'.join(json.dumps(x) for x in [
    dict(phase='domain', single_pass_ns=70_000_000, catalogue_balls=12),
    dict(phase='full', status='ok', wall_ns=100_000_000, domain_ns=80_000_000, forest_ns=20_000_000, cpu_seconds=0.8),
    dict(phase='exit', status='ok')])
refused_stdout = '\n'.join(json.dumps(x) for x in [
    dict(phase='full', status='resource_exhausted'), dict(phase='exit', status='resource_exhausted')])
timing_results = {}
for fixture in ('second_native_refusal', 'second_timeout'):
    calls = []

    def fake_run(cmd, capture_output, text, timeout):
        calls.append(cmd)
        need(capture_output and text and timeout == 600, 'source wrapper subprocess options preserved')
        if len(calls) == 1:
            return SimpleNamespace(returncode=0, stdout=successful_stdout, stderr='')
        if fixture == 'second_timeout':
            raise subprocess.TimeoutExpired(cmd, timeout, output='partial stdout', stderr='partial stderr')
        return SimpleNamespace(returncode=1, stdout=refused_stdout, stderr='named refusal')

    timing = load_exact('full_timing.py', SimpleNamespace(run=fake_run))
    timing['time'] = SimpleNamespace(monotonic=lambda: 0.0)  # deterministic fake process elapsed time
    with tempfile.TemporaryDirectory(prefix='ehgp-synthetic-timing-') as td:
        tmp = Path(td)
        bench = tmp / 'fake_bench'; bench.write_text('never executed\n')
        xyz = tmp / 'one_site.u32le'; xyz.write_bytes(bytes(12))
        ids = tmp / 'one_site.ids.u32le'; ids.write_bytes(bytes(4))
        out = tmp / 'out.json'
        argv = ['full_timing.py', '--bench', str(bench), '--work', str(tmp / 'work'), '--out', str(out),
                '--extra', 'first=' + str(xyz) + ':' + str(ids),
                '--extra', 'second=' + str(xyz) + ':' + str(ids)]
        old = sys.argv; captured_out, captured_err = io.StringIO(), io.StringIO(); sys.argv = argv
        try:
            with contextlib.redirect_stdout(captured_out), contextlib.redirect_stderr(captured_err):
                try:
                    code = timing['main'](); exception = None
                except subprocess.TimeoutExpired as error:
                    code = None
                    exception = {'class': type(error).__name__, 'timeout_seconds': error.timeout,
                                 'output': error.output, 'stderr': error.stderr}
        finally:
            sys.argv = old
        need(len(calls) == 2, 'first completed and second attempted, both fake')
        need('first' in captured_out.getvalue(), 'first completion is observable on stdout')
        payload = json.loads(out.read_text()) if out.exists() else None
        if fixture == 'second_timeout':
            need(exception is not None and payload is None, 'timeout loses final report including first take')
        else:
            need(code == 1 and exception is None and payload['failures'] == 1, 'ordinary native refusal is reported')
            need(len(payload['rows']) == 2 and payload['rows'][0]['takes'][0]['ok'], 'completed first take persists on ordinary refusal')
        timing_results[fixture] = dict(main_exit_code=code, exception=exception,
                                       final_report_present=out.exists(), fake_process_calls=len(calls),
                                       stdout=captured_out.getvalue(), stderr=captured_err.getvalue(),
                                       final_report=payload)

out = {'pin': PIN, 'checks': checks, 'scope': 'exact frozen wrapper/reader AST; synthetic reports; subprocess.run faked',
       'native_or_cloud_or_fit_run': False, 'ab_fixtures': ab_results, 'full_timing_fixtures': timing_results,
       'sign_test_limit': {'five_nontied_pairs_p_min': 0.0625, 'six_nontied_pairs_p_min': 0.03125,
                           'independent_or_exchangeable_signs': 'required for inferential interpretation; not established by this replay'},
       'sources_sha256': {str(p.relative_to(HERE)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sorted((HERE / 'source').rglob('*')) if p.is_file()}}
print(json.dumps(out, indent=2, sort_keys=True))
