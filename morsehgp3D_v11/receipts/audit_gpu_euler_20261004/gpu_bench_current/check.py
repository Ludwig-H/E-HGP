#!/usr/bin/env python3
"""Pinned published GPU A/B protocol, synthetic streams and fake processes only."""
import ast
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile

root = Path(__file__).resolve().parent
source = root / 'source/morsehgp3D_v11/bench/gpu_ab.py'
checks = 0


def need(value, message):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(message)


metadata = json.loads((root / 'SOURCE.json').read_text())
for name, digest in metadata['source_sha256'].items():
    need(hashlib.sha256((root / 'source' / name).read_bytes()).hexdigest() == digest, 'source hash')
tree = ast.parse(source.read_bytes())
tree.body = [node for node in tree.body if not isinstance(node, ast.If)]
env = {'__name__': 'frozen_gpu_protocol'}
exec(compile(tree, str(source), 'exec'), env)
need(env['FRAMES'] == ('lidar_ng00', 'lidar_ng01', 'lidar_ng02'), 'three frames')
fixtures = {}
for fixture, repetitions, warm_passes in (
    ('complete', 1, 3), ('warm_rows_missing', 1, 3),
    ('no_cold_repetitions', 0, 3), ('no_later_warm_pass', 1, 1)
):
    commands = []

    def fake_run(command, timeout):
        commands.append(command)
        need(timeout == 900, 'unchanged timeout argument')
        Path(command[3]).write_bytes(b'synthetic equal FULL dump; never a native result')
        passes = int(command[-1]) if len(command) == 13 else 1
        events = []
        if passes > 1 and fixture != 'warm_rows_missing':
            for index in range(1, passes + 1):
                events.append(dict(phase='pass', pass_=index, status='ok', wall_ns=100000000,
                                   domain_ns=70000000, forest_ns=30000000,
                                   batch_executor_ns=10000000, batch_device_init_ns=1000000))
                events[-1]['pass'] = events[-1].pop('pass_')
        events.extend([
            dict(phase='domain', single_pass_ns=50000000, catalogue_work={'prefixes': 7, 'judged': 3},
                 leaf_batch=dict(jobs=1, unresolved=0, records=1, population=2)),
            dict(phase='full', status='ok', wall_ns=100000000, domain_ns=70000000,
                 forest_ns=30000000, cpu_seconds=1.0),
            dict(phase='exit', status='ok'),
        ])
        return 0, '\n'.join(json.dumps(event) for event in events), '', 0.0

    env['run'] = fake_run
    with tempfile.TemporaryDirectory(prefix='ehgp-fake-gpu-protocol-') as temporary:
        folder = Path(temporary)
        bench = folder / 'fake_bench'; bench.write_text('never executed\n')
        data = folder / 'data'; data.mkdir()
        out = folder / 'out'
        old_argv = sys.argv
        sys.argv = ['gpu_ab.py', '--bench', str(bench), '--data', str(data), '--out', str(out),
                    '--reps', str(repetitions), '--warm-passes', str(warm_passes)]
        try:
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                code = env['main']()
        finally:
            sys.argv = old_argv
        report = json.loads((out / 'gpu_ab_report.json').read_text())
    need(code == 0 and report['verdict'] == 'conforme', 'current protocol acceptance')
    need(not report['refusals'] and len(report['identity']) == 3, 'final dumps matched')
    need(len(report['ledger']) == 3 and all(x == {'prefixes': 7, 'judged': 3} for x in report['ledger'].values()), 'final ledgers matched')
    need(len(report['cold']) == 6 * repetitions and len(report['warm']) == 6, 'scheduled rows')
    missing = sum(not row['passes'] for row in report['warm'])
    no_later = sum(row['wall_ms'] is None for row in report['warm_medians_ms'].values())
    if fixture in ('warm_rows_missing', 'no_later_warm_pass'):
        need(missing == no_later == 6, 'conforme with no measured later warm pass')
    else:
        need(missing == no_later == 0, 'complete later passes retained')
    if fixture == 'no_cold_repetitions':
        need(not report['cold_medians_ms'], 'conforme without any cold take')
    fixtures[fixture] = dict(verdict=report['verdict'], cold_rows=len(report['cold']),
                            warm_rows=len(report['warm']), warm_rows_without_passes=missing,
                            warm_metrics_without_later_pass=no_later,
                            fake_invocations=len(commands), requested_repetitions=repetitions,
                            requested_warm_passes=warm_passes)

probe = (root / 'source/morsehgp3D_v11/bench/full_probe.cpp').read_text()
need('const bool detailed = pass == passes;' in probe, 'only final pass detailed')
need('if (!detailed) return tower.outcome();' in probe, 'previous passes return before serialization')
need('return serialize(argv[3], tower.value());' in probe, 'last pass canonical dump')
need('word(out, kCoordBits)' in probe and 'integer(out, level.numerator())' in probe,
     'dump retains exact profile and levels')
print(json.dumps({'status': 'PASS', 'checks': checks, 'source_kind': 'published_commit',
                  'fixtures': fixtures, 'native_runs': 0, 'gcp_actions': 0}, sort_keys=True, indent=2))
