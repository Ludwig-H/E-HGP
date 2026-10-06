#!/usr/bin/env python3
"""Lecture stdlib/Git des métadonnées, sans campagne ni calcul HGP."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

PIN = 'cf5da0e91fb8740c1d748577fefb129c37b02d3d'
REPO = Path('/workspaces/E-HGP')
PREFIX = 'morsehgp3D_v11/receipts/developpement_20261004/gpu_g4/sessions/claudegpu4/'

def git_file(pin, path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', pin + ':' + path])

def evidence(pin, path, needles):
    raw = git_file(pin, path)
    lines = raw.decode().splitlines()
    found = []
    for needle in needles:
        hits = [(i + 1, line) for i, line in enumerate(lines) if needle in line]
        if len(hits) != 1:
            raise ValueError('ancrage non unique: ' + needle)
        found.append({'line': hits[0][0], 'text': hits[0][1].strip()})
    return {'pin': pin, 'path': path, 'sha256': hashlib.sha256(raw).hexdigest(), 'anchors': found}

def derive():
    receipt = json.loads(git_file(PIN, PREFIX + 'receipt.json'))
    records = []
    sources = []
    for name in ['gpu_k5_report.json', 'gpu_k5_leaf24_report.json']:
        raw = git_file(PIN, PREFIX + name)
        report = json.loads(raw)
        rows = [r for r in report['warm'] if r['frame'] == 'lidar_ng00' and r['mode'] == 'gpu']
        if len(rows) != 1:
            raise ValueError('inventaire inattendu')
        row = rows[0]
        ledger = report['ledger']['lidar_ng00']
        records.append({'leaf': report['leaf'], 'nodes': ledger['nodes'],
                        'filter_tests': ledger['filter_tests'], 'leaves': ledger['leaves'],
                        'last_pass_cpu_seconds': row['passes'][-1]['cpu_seconds'],
                        'cpu_seconds_scope': 'index_domain_forests_entire_pass'})
        sources.append({'path': PREFIX + name, 'sha256': hashlib.sha256(raw).hexdigest()})
    a, b = records
    original = evidence(receipt['commit'], 'morsehgp3D_v11/bench/full_probe.cpp',
        ['const auto cpu_start = std::clock()', 'auto index = build_index(',
         'auto domain = prepare_full_domain(', 'auto tower = build_full(',
         'const double cpu_seconds = double('])
    current = evidence(PIN, 'morsehgp3D_v11/src/catalogue/single_pass.cpp',
        ['Outcome generate(u32 ordinal, u32 worker)',
         'MHGP11_TRY(frontier.execute_task(ordinal, run))',
         'if (clock) out.generation_ns = clock->nanoseconds()'])
    always_timings = evidence(PIN, 'morsehgp3D_v11/bench/full_probe.cpp',
        ['auto domain = prepare_full_domain('])
    return {'schema': 'audit_gpu_t1_attribution_metadata_v1', 'evidence_pin': PIN,
        'historical_session': 'v11.20261004.claudegpu4',
        'historical_source_pin': receipt['commit'], 'historical_archive_sha256': receipt['results_sha256'],
        'reports': sources, 'observations': records,
        'simultaneous_work_change': {'removed_nodes': a['nodes'] - b['nodes'],
                                    'removed_filter_tests': a['filter_tests'] - b['filter_tests']},
        'source_scope': original, 'task_boundary_source': current, 'timings_always_requested': always_timings,
        'interpretation': 'Le quotient CPU/nœud entre feuilles 16 et 24 est une différence normalisée de deux travaux. Il ne mesure pas isolément allocations ou atomiques.',
        'necessary_T1_clarifications': [
            'Comparer A/B N1 à feuille 16 fixe, avec registre et tests G1 identiques.',
            'Lire CLOCK_THREAD_CPUTIME_ID au début et à la fin de chaque tâche de génération, plus aux bornes de feuilles ; walk_thread_ns = CPU tâche moins CPU feuilles.',
            'Activer ces microhorloges par un interrupteur de diagnostic distinct de timings, car la sonde demande toujours timings ; les éteindre dans les bras de mur.'
        ],
        'session_budget': 'Non validé : build, qualifications, mutants et contrôles A/A non intégralement chiffrés. Aucun dépassement démontré.',
        'limits': 'Lecture de métadonnées et sources ; aucun test natif, GCP, réseau, donnée ou calcul HGP exécuté.'}

def main():
    result = derive()
    text = json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + '\n'
    target = Path(__file__).with_name('proof.json')
    if '--capture' in sys.argv:
        target.write_text(text)
    elif json.loads(target.read_text()) != result:
        raise ValueError('preuve différente')
    sys.stdout.write(text)

if __name__ == '__main__':
    main()
