#!/usr/bin/env python3
"""GPU A/B protocol at Git 22a6, stdlib AST and fake processes only.

The inherited b74 harness is preserved separately. This adaptation executes
the pinned Python protocol with its native runner and build path replaced.
No native executable, CUDA, profile, download or GCP action is possible here.
The fake stream has a final equal dump and ledger even when intermediate
pass metadata is invalid, so each refusal tests the new non-vacuity guards.
"""
import ast
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / 'source/morsehgp3D_v11/bench/gpu_ab.py'
CHECKS = 0


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def forbidden_build(*_args, **_kwargs):
    raise RuntimeError('build path forbidden in this model')


def main():
    metadata = json.loads((ROOT / 'SOURCE.json').read_text())
    for entry in metadata['files']:
        raw = (ROOT / 'source' / entry['path']).read_bytes()
        need(hashlib.sha256(raw).hexdigest() == entry['sha256'], 'source hash')
        need(len(raw) == entry['bytes'], 'source bytes')
    need(hashlib.sha256((ROOT / 'inherited/check_b74.py').read_bytes()).hexdigest() ==
         metadata['inherited']['check_sha256'], 'old harness preserved byte for byte')
    tree = ast.parse(SOURCE.read_bytes())
    tree.body = [node for node in tree.body if not isinstance(node, ast.If)
                 and not (isinstance(node, ast.ImportFrom) and node.module == 'ab_g4')
                 and not (isinstance(node, ast.Expr) and isinstance(node.value, ast.Call)
                          and isinstance(node.value.func, ast.Attribute)
                          and node.value.func.attr == 'insert')]
    order_source = ROOT / 'source/morsehgp3D_v11/bench/ab_g4.py'
    order_tree = ast.parse(order_source.read_bytes())
    order_function = [n for n in order_tree.body if isinstance(n, ast.FunctionDef) and n.name == 'williams']
    need(len(order_function) == 1, 'unique Williams function')
    env = {'__name__': 'frozen_gpu_protocol', '__file__': str(SOURCE)}
    exec(compile(ast.Module(body=order_function, type_ignores=[]), str(order_source), 'exec'), env)
    exec(compile(tree, str(SOURCE), 'exec'), env)
    env['build'] = forbidden_build
    need(env['FRAMES'] == ('lidar_ng00', 'lidar_ng01', 'lidar_ng02'), 'three frames')
    fixtures = {}
    plans = (
        ('complete', 1, 3, True),
        ('minimal_warm', 1, 2, True),
        ('no_cold_repetitions', 0, 3, False),
        ('no_later_warm_pass', 1, 1, False),
        ('warm_rows_missing', 1, 3, False),
        ('middle_pass_missing', 1, 3, False),
        ('pass_duplicate', 1, 3, False),
        ('pass_out_of_order', 1, 3, False),
        ('pass_out_of_range', 1, 3, False),
        ('pass_status_nonok', 1, 3, False),
    )
    for fixture, repetitions, warm_passes, expected_success in plans:
        commands = []

        def fake_run(command, timeout):
            commands.append(command)
            need(timeout == 900, 'unchanged timeout argument')
            Path(command[3]).write_bytes(b'synthetic equal FULL dump; never a native result')
            passes = int(command[-1]) if len(command) == 13 else 1
            events = []
            if passes > 1:
                indices = list(range(1, passes + 1))
                if fixture == 'warm_rows_missing':
                    indices = []
                elif fixture == 'middle_pass_missing':
                    indices = [1, 3]
                elif fixture == 'pass_duplicate':
                    indices = [1, 2, 2, 3]
                elif fixture == 'pass_out_of_order':
                    indices = [1, 3, 2]
                elif fixture == 'pass_out_of_range':
                    indices = [1, 2, 4]
                for index in indices:
                    event = dict(phase='pass', status='ok', wall_ns=100000000,
                                 domain_ns=70000000, forest_ns=30000000,
                                 batch_executor_ns=10000000, batch_device_init_ns=1000000)
                    event['pass'] = index
                    if fixture == 'pass_status_nonok' and index == 2:
                        event['status'] = 'refus'
                    events.append(event)
            events.extend([
                dict(phase='domain', single_pass_ns=50000000, catalogue_work={'prefixes': 7, 'judged': 3},
                     leaf_batch=dict(jobs=1, unresolved=0, records=1, population=2)),
                dict(phase='full', status='ok', wall_ns=100000000, domain_ns=70000000,
                     forest_ns=30000000, cpu_seconds=1.0),
                dict(phase='exit', status='ok'),
            ])
            return 0, '\n'.join(json.dumps(event) for event in events), '', 0.0

        env['run'] = fake_run
        with tempfile.TemporaryDirectory(prefix='ehgp-fake-gpu-protocol22-') as temporary:
            folder = Path(temporary)
            bench = folder / 'fake_bench'
            bench.write_text('never executed\n')
            data = folder / 'data'
            data.mkdir()
            out = folder / 'out'
            old_argv = sys.argv
            sys.argv = ['gpu_ab.py', '--bench', str(bench), '--data', str(data), '--out', str(out),
                        '--reps', str(repetitions), '--warm-passes', str(warm_passes)]
            try:
                with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                    code = env['main']()
            finally:
                sys.argv = old_argv
            report_path = out / 'gpu_ab_report.json'
            report = json.loads(report_path.read_text()) if report_path.is_file() else None
            need(not (out / 'dump.tmp').exists(), 'temporary final dump not retained')
        early = repetitions == 0 or warm_passes == 1
        if early:
            need(code == 1 and report is None and not commands, 'invalid regimes refused before any fake run')
            fixtures[fixture] = dict(code=code, report_written=False, fake_invocations=0,
                                     requested_repetitions=repetitions, requested_warm_passes=warm_passes)
            continue
        need(report is not None, 'report available after simulated runs')
        need(code == (0 if expected_success else 1), 'correct final return code')
        need(report['verdict'] == ('conforme' if expected_success else 'refus'), 'correct final verdict')
        need(len(report['refusals']) == (0 if expected_success else 6), 'all six warm rows checked')
        need(len(report['identity']) == 3, 'final dumps matched even for pass-guard negatives')
        need(len(report['ledger']) == 3 and all(x == {'prefixes': 7, 'judged': 3}
                                               for x in report['ledger'].values()), 'final ledgers matched')
        need(len(report['cold']) == 6 * repetitions and len(report['warm']) == 6, 'scheduled rows')
        need(len(commands) == 6 * repetitions + 6, 'native work entirely simulated')
        need(all(row['code'] == 0 and row['summary']['status'] == 'ok' for row in report['cold'] + report['warm']),
             'pass guards independent of final full success')
        if expected_success:
            for row in report['warm']:
                need([p['pass'] for p in row['passes']] == list(range(1, warm_passes + 1)), 'complete exact pass sequence')
                need(all(p['status'] == 'ok' for p in row['passes']), 'all passes succeeded')
            need(all(value['wall_ms'] == 100.0 and value['first_pass_wall_ms'] == 100.0
                     for value in report['warm_medians_ms'].values()), 'later-pass metrics present')
        elif fixture == 'warm_rows_missing':
            need(all(value['wall_ms'] is None for value in report['warm_medians_ms'].values()),
                 'empty warm metadata cannot claim success')
        need('derniere passe seulement' in report['scope'], 'last-dump identity scope stated')
        fixtures[fixture] = dict(code=code, verdict=report['verdict'], cold_rows=len(report['cold']),
                                 warm_rows=len(report['warm']), refusals=len(report['refusals']),
                                 fake_invocations=len(commands), requested_repetitions=repetitions,
                                 requested_warm_passes=warm_passes,
                                 example_passes=[(p['pass'], p['status']) for p in report['warm'][0]['passes']],
                                 final_dumps_and_ledgers_match=True)

    probe = (ROOT / 'source/morsehgp3D_v11/bench/full_probe.cpp').read_text()
    need('const bool detailed = pass == passes;' in probe, 'only final pass detailed')
    need('if (!detailed) return tower.outcome();' in probe, 'earlier passes return before dump serialization')
    need('return serialize(argv[3], tower.value());' in probe, 'last pass canonical dump')
    need('word(out, kCoordBits)' in probe and 'integer(out, level.numerator())' in probe,
         'dump retains exact profile and levels')
    scope_note = 'passes2..P ne serialisent rien' in SOURCE.read_text()
    need(scope_note, 'captured wording imprecision preserved as evidence')
    receipt = (ROOT / 'source/morsehgp3D_v11/receipts/developpement_20261004/mesures_g4_ab8_diag1/README.md').read_text()
    need('126 prises A/B' in receipt and '48 prises de' in receipt and '/dev/null' in receipt,
         'receipt distinguishes dumps from non-digested timing takes')
    need('sans identité de' in receipt and 'sortie établie' in receipt, 'no identity claim for timing takes')
    need("nombre de fils et l'affinité" in receipt.replace('’', "'") and 'hyperthreading' in receipt,
         'W48/W24 contrast does not isolate hyperthreading')
    print(json.dumps({'status': 'PASS', 'checks': CHECKS, 'pin': metadata['pin'],
                      'source_kind': 'immutable published Git + inherited harness adaptation',
                      'fixtures': fixtures, 'native_runs': 0, 'gcp_actions': 0,
                      'scope_wording_correction': 'Only passes1..P-1 omit dumps; P is the last serialized dump.',
                      'receipt_counts_readonly': {'ab_dumps': 126, 'timing_takes_without_dump_identity': 48}},
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
