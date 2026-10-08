#!/usr/bin/env python3
"""Relit uniquement sources Git et métadonnées/log de résultats, aucun moteur."""
import argparse
import ast
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tarfile

HERE = Path(__file__).resolve().parent


def need(ok, text):
    if not ok:
        raise ValueError(text)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--repo', default='/workspaces/E-HGP')
    p.add_argument('--session', type=Path, default=Path('/workspaces/.ehgp-sessions/v12.20261008.t2da'))
    a = p.parse_args()
    c = json.loads((HERE / 'capture.json').read_text())
    sources = {}
    for path, digest in c['sources'].items():
        raw = subprocess.check_output(['git', '-C', a.repo, 'show', c['commit'] + ':' + path])
        need(sha(raw) == digest, 'source ' + path)
        sources[path] = raw.decode()
    plan = (a.session / 'package/plan.json').read_bytes()
    need(sha(plan) == c['plan_sha256'], 'plan')
    cmd = json.loads(plan)['commands'][2]
    need(cmd == c['command'], 'commande LiDAR')
    archive = a.session / 'results/results.tar.gz'
    need(sha(archive.read_bytes()) == c['archive_sha256'], 'archive résultats')
    with tarfile.open(archive, 'r:gz') as t:
        meta = t.extractfile(c['meta_member']).read()
        log = t.extractfile(c['log_member']).read()
    need(sha(meta) == c['meta_sha256'] and sha(log) == c['log_sha256'], 'métadonnées/log')
    pairs = dict(line.split('=', 1) for line in meta.decode().splitlines() if '=' in line)
    selected = {key: pairs[key] for key in c['metadata']}
    need(selected == c['metadata'], 'champs de statut')
    # Liste blanche : aucune ligne contenant le répertoire distant n'est émise.
    passed = re.findall(r'Test\s+#\d+: (mhgp12_\w+)\s+\.+\s+Passed\s+([0-9.]+) sec', log.decode())
    starts = re.findall(r'Start\s+\d+: (mhgp12_\w+)', log.decode())
    need(len(passed) == 6 and len(starts) == 7 and starts[-1] == 'mhgp12_tower_chain_m0', 'cohorte commencée')
    need(not any(name == starts[-1] for name, _ in passed), 'pas de verdict chain')
    need(not re.search(r'Test\s+#\d+: mhgp12_tower_chain_m0', log.decode()), 'verdict final inattendu')
    def src(name):
        return sources['morsehgp3D_v12/' + name]
    module = ast.parse(src('tests/tower/mes_m0.py'))
    cases = next(ast.literal_eval(n.value) for n in module.body if isinstance(n, ast.Assign)
                 and any(isinstance(x, ast.Name) and x.id == 'CASES' for x in n.targets))
    chain = src('tests/tower/tower_chain.cpp')
    need('auto resolution = resolve_tower(' in chain and 'auto forests = tower::build_forests(' in chain,
         'route séquentielle')
    need('build_tower(' not in chain and '--recouvert' not in chain, 'route recouverte inattendue')
    cmake = src('tests/tower/tests.cmake')
    gate = re.search(r'mhgp12_python_gate\(mhgp12_tower_chain_m0(.*?)\)', cmake, re.S).group(1)
    need('LABELS lidar long TIMEOUT 7200' in gate and '--chaine' in gate, 'porte prévue')
    need('lidar mutant)' in src('cmake/gates.cmake') and 'PROPERTY RUN_SERIAL TRUE' in src('cmake/gates.cmake'),
         'sérialisation des portes LiDAR')
    need('OUTPUT_VARIABLE run_stdout ERROR_VARIABLE run_stderr' in src('cmake/run_expect.cmake'),
         'sorties retenues')
    elapsed = sum(Decimal(seconds) for _, seconds in passed)
    print(json.dumps({'passed_tests': [name for name, _ in passed], 'started_without_verdict': starts[-1],
                      'displayed_passed_seconds': str(elapsed),
                      'external_command_limit_seconds': cmd['timeout_seconds'],
                      'nominal_seconds_left_ignoring_overheads_and_rounding': str(Decimal(180) - elapsed),
                      'chain_ctest_limit_seconds': 7200, 'planned_cases': list(cases),
                      'planned_native_calls': 2 * len(cases), 'planned_semantic_reads': len(cases),
                      'native_path': 'CPU resolve_tower puis build_forests, W1/W8',
                      'metadata': selected, 'chain_passed': False, 'deadlock_proved': False,
                      'source_or_native_execution': False}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
