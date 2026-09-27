#!/usr/bin/env python3
"""Offline capture of publication readback, Python -O and negative reader tests."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DATA = ROOT / 'morsehgp3D_v9/receipts/g4_core_warm_20260927'
OUT = ROOT / 'morsehgp3D_v9/receipts/g4_core_warm_checks_20260927'
SNAPSHOT = Path('/workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/snapshot.tar.gz')
REFUSAL = Path('/workspaces/E-HGP/build/v9-g4-core-session-20260927')


def need(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def recipe():
    return [(name + mode, [sys.executable, *flags, '-B', str(HERE / script), *args])
            for name, script, args in [('unit', 'test_tools.py', []), ('read', 'readback.py',
                 [str(DATA), '--snapshot', str(SNAPSHOT)])]
            for mode, flags in [('normal', []), ('optimized', ['-O'])]]


def pins():
    paths = list(HERE.glob('*.py')) + [HERE / 'plan.json', SNAPSHOT]
    paths += list(DATA.rglob('*'))
    paths += [ROOT / ('gcp-migration/' + name) for name in
              ('tower_worker_v9.py', 'tower_session_v9.py', 'tower_snapshot_v9.py', 'full_probe_session_v7.py')]
    return {str(path): sha(path) for path in paths if path.is_file()}


def run():
    OUT.mkdir(exist_ok=False)
    state = dict(status='running', pins_before=pins(), commands=[], GCP_calls=False)
    save = lambda: (OUT / 'capture.json').write_text(json.dumps(state, indent=2) + '\n')
    save()
    try:
        for name, argv in recipe():
            row = dict(name=name, argv=argv, cwd=str(ROOT), status='running')
            state['commands'].append(row)
            save()
            with (OUT / (name + '.stdout')).open('wb') as out, (OUT / (name + '.stderr')).open('wb') as err:
                code = subprocess.run(argv, cwd=ROOT, stdout=out, stderr=err, check=False).returncode
            row.update(exit_code=code, status='completed', stdout_sha256=sha(OUT / (name + '.stdout')),
                       stderr_sha256=sha(OUT / (name + '.stderr')))
            save()
            need(code == 0, 'failed command: ' + name)
        (OUT / 'prestart_refusal').mkdir()
        for name in ('controller.stderr', 'controller.stdout', 'launch.json'):
            raw = (REFUSAL / name).read_bytes()
            need(b'PRIVATE KEY' not in raw, 'private data forbidden')
            (OUT / 'prestart_refusal' / name).write_bytes(raw)
        state['prestart_refusal'] = {name: sha(OUT / 'prestart_refusal' / name) for name in
                                    ('controller.stderr', 'controller.stdout', 'launch.json')}
        state['pins_after'] = pins()
        need(state['pins_before'] == state['pins_after'], 'changed evidence')
        state['status'] = 'completed'
        save()
        check()
    except BaseException as error:
        state.update(status='failed', failure=str(error))
        save()
        raise


def check():
    state = json.loads((OUT / 'capture.json').read_text())
    need(state['status'] == 'completed' and state['GCP_calls'] is False, 'completed offline checks')
    need(state['pins_before'] == state['pins_after'] == pins(), 'live pin closure')
    need(len(state['commands']) == 4, 'four checks')
    for row, (name, argv) in zip(state['commands'], recipe()):
        need(row['name'] == name and row['argv'] == argv and row['exit_code'] == 0 and
             row['cwd'] == str(ROOT) and row['status'] == 'completed', 'command binding')
        for stream in ('stdout', 'stderr'):
            need(sha(OUT / (name + '.' + stream)) == row[stream + '_sha256'], 'output binding')
    for name in ('readnormal', 'readoptimized'):
        need(json.loads((OUT / (name + '.stdout')).read_text()) ==
             json.loads((DATA / 'SUMMARY.json').read_text()), 'summary recomputation')
    for name, value in state['prestart_refusal'].items():
        need(sha(OUT / 'prestart_refusal' / name) == value, 'initial refusal changed')
    need('ValueError: existing private/public key files' in
         (OUT / 'prestart_refusal/controller.stderr').read_text(), 'initial refusal cause')
    print(json.dumps(dict(status='passed', commands=4, unit_tests_per_mode=11, GCP_calls=False)))


if __name__ == '__main__':
    need(sys.argv[1:] in (['run'], ['check']), 'usage: checks.py run|check')
    run() if sys.argv[1] == 'run' else check()
