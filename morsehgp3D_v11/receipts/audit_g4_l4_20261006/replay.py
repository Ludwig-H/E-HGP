#!/usr/bin/env python3
"""Lecture de métadonnées/Git, aucun build ni calcul HGP."""
import argparse
from collections import Counter
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

PIN = 'cf28afb04040eacf1eb92080d75e8a0ebb58a4eb'
RETIRED = '830473218e1b35fa725209df14b7855667f4c6d4'
BASE = '8ee28873f2b9e59166b73a36e7b2302fa2a3a7a4'
BEFORE_L4 = 'c1675e4c94958ba7e4b5a0d0327d54b3c466ad5c'
PUBLISHED = 'morsehgp3D_v11/receipts/developpement_20261006/l4_recouvrement/'
EXPECTED = {5: {'lidar_ng00': '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
                 'lidar_ng01': '5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
                 'lidar_ng02': '78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207'},
            10: {'lidar_ng00': '61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295',
                 'lidar_ng01': '838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de',
                 'lidar_ng02': '81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e'}}
MODES = {'cpu': '16379', 'gpu': '81915', 'gpu_recouvert': '212987'}

def need(ok, why):
    if not ok:
        raise ValueError(why)

def sha(b):
    return hashlib.sha256(b).hexdigest()

def sha_file(p):
    with p.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def member(t, n):
    m = t.getmember(n)
    need(m.isfile() and m.size <= 16 * 1024 * 1024, 'membre non borné')
    return t.extractfile(m).read()

def git(repo, path, pin=PIN):
    return subprocess.check_output(['git', '-C', str(repo), 'show', pin + ':' + path])

def source_tree(repo, package):
    paths = ['morsehgp3D_v11/' + p for p in ['src', 'tests', 'cmake', 'bench', 'reference', 'tools', 'cli']]
    exact = ['morsehgp3D_v11/CMakeLists.txt', 'gcp-migration/v11_worker.sh']
    b = subprocess.check_output(['git', '-C', str(repo), 'archive', PIN, '--', *paths, *exact])
    prefixes = tuple(p + '/' for p in paths)
    hashes = []
    with tarfile.open(fileobj=io.BytesIO(b)) as g, tarfile.open(package) as a:
        wanted = {m.name for m in g.getmembers() if m.isfile()}
        actual = {m.name for m in a.getmembers() if m.isfile() and
                  (m.name.startswith(prefixes) or m.name in exact)}
        need(wanted == actual, 'inventaire de source')
        for n in sorted(wanted):
            v = member(g, n)
            need(v == member(a, n), 'source différente')
            hashes.append(sha(v) + '  ' + n + '\n')
    return {'files': len(wanted), 'manifest_sha256': sha(''.join(hashes).encode()), 'equal_to_git': True}

def derive(repo, session):
    receipt_raw = (session / 'receipt.json').read_bytes()
    r = json.loads(receipt_raw)
    after = r['observed_after']
    need((session / 'DONE').read_text().strip() == '0' and r['status'] == 'completed' and
         r['closure'] == 'stopped' and r['targeted_shutdown_certified'] and r['start_certified'], 'session non close')
    need(after['status'] == 'TERMINATED' and after['name'] == r['target']['instance'] and
         r['generation'] == r['closing_generation'] == after['lastStartTimestamp'], 'cible/génération')
    need(r['private_key_deleted'] and r['reserve_released'] and r['oslogin_key_removed'], 'gardes')
    need(r['commit'] == PIN and r['worker_source'] == 'commit:' + PIN and
         r['source_kind'] == 'commit' and r['evidence_grade'] == 'pushed_commit', 'source')
    need(r['results_verified'] and r['data_verified_remote'] and not r['results_skipped_members'] and
         not r['overflow']['evicted'] and not r['overflow']['truncated_streams'], 'capture incomplète')
    arc = session / 'results/results.tar.gz'
    package = session / 'package/package.tar.gz'
    plan_raw = (session / 'package/plan.json').read_bytes()
    need(sha_file(arc) == r['results_sha256'] and sha_file(package) == r['package_sha256'] and
         sha(plan_raw) == r['plan_sha256'], 'SHA archives/plan')
    source = source_tree(repo, package)
    need(not subprocess.check_output(['git', '-C', str(repo), 'diff', '--name-only', BASE, RETIRED,
                                      '--', 'morsehgp3D_v11/src']).strip(), 'retrait produit différent de la base')
    restored_paths = ['morsehgp3D_v11/src', 'morsehgp3D_v11/bench/full_probe.cpp',
                      'morsehgp3D_v11/tests', 'morsehgp3D_v11/CMakeLists.txt']
    need(not subprocess.check_output(['git', '-C', str(repo), 'diff', '--name-only', BEFORE_L4, RETIRED,
                                      '--', *restored_paths]).strip(), 'retrait différent du parent avant L4')
    for name, value in [('receipt.json', receipt_raw), ('launch.json', (session / 'launch.json').read_bytes()),
                        ('plan.json', plan_raw)]:
        need(git(repo, PUBLISHED + name, RETIRED) == value, 'pièce publiée différente')
    for line in git(repo, PUBLISHED + 'SHA256SUMS', RETIRED).decode().splitlines():
        h, name = line.split(None, 1)
        need(sha(git(repo, PUBLISHED + name.lstrip('*').removeprefix('./'), RETIRED)) == h, 'SHA publié')
    judge_raw = git(repo, 'morsehgp3D_v11/bench/gpu_ab.py')
    judge_text = judge_raw.decode()
    need('work == report[\'ledger\'].get(frame)' in judge_text and
         "report['verdict'] = 'conforme' if not report['refusals'] else 'refus'" in judge_text, 'juge changé')
    results, comparisons, ranges = [], [], []
    processes = dumps = intermediate = constructed = 0
    with tarfile.open(arc) as t:
        for index, (k, reps, passes, leaf) in enumerate([(5, 5, 12, 16), (10, 3, 8, 24)]):
            prefix = f'results/cmd/{index:03d}_gpu_k{k}/'
            raw = member(t, prefix + f'files/gpu_k{k}/gpu_ab_report.json')
            need(git(repo, PUBLISHED + f'gpu_ab_report_k{k}.json', RETIRED) == raw, 'rapport publié différent')
            q = json.loads(raw)
            need(q['verdict'] == 'conforme' and not q['refusals'] and q['modes'] == MODES and
                 q['workers'] == '48' and q['reps'] == reps and q['warm_passes'] == passes and
                 q['kmax'] == k and q['leaf'] == leaf and q['identity'] == EXPECTED[k], 'contrat rapport')
            need('derniere passe' in q['scope'], 'portée absente')
            seen = Counter()
            for regime in ['cold', 'warm']:
                rows = q[regime]
                need(len(rows) == (3 * 3 * reps if regime == 'cold' else 9), 'inventaire processus')
                for x in rows:
                    need(x['code'] == 0 and x['summary']['status'] == 'ok' and x['summary']['exit'] == 'ok' and
                         x['dump_sha256'] == EXPECTED[k][x['frame']], 'sortie différente')
                    need(x['summary']['batch']['unresolved'] == 0, 'unresolved')
                    ps = x['passes']
                    need(not ps if regime == 'cold' else len(ps) == passes and
                         [p['pass'] for p in ps] == list(range(1, passes + 1)) and
                         all(p['status'] == 'ok' for p in ps), 'passes incomplètes')
                    seen[regime, x['frame'], x['mode']] += 1
                    processes += 1
                    dumps += 1
                    constructed += 1 if regime == 'cold' else passes
                    intermediate += 0 if regime == 'cold' else passes - 1
            for f in EXPECTED[k]:
                for mode in MODES:
                    need(seen['cold', f, mode] == reps and seen['warm', f, mode] == 1, 'inventaire du mode')
            warm = {(x['frame'], x['mode']): x for x in q['warm']}
            for f in EXPECTED[k]:
                cpu = warm[f, 'cpu']['passes'][1:]
                serial_passes = warm[f, 'gpu']['passes'][1:]
                covered = warm[f, 'gpu_recouvert']['passes'][1:]
                cpu_max = max(p['domain_ns'] for p in cpu)
                covered_min = min(p['domain_ns'] for p in covered)
                threshold = 1000 if k == 5 else 850
                need(covered_min * 1000 > cpu_max * threshold, 'conclusion de performance différente')
                ranges.append({'k': k, 'frame': f, 'cpu_domain_ns': [min(p['domain_ns'] for p in cpu), cpu_max],
                    'serial_gpu_domain_ns': [min(p['domain_ns'] for p in serial_passes), max(p['domain_ns'] for p in serial_passes)],
                    'covered_domain_ns': [covered_min, max(p['domain_ns'] for p in covered)],
                    'ratio_lower_bound_permille': covered_min * 1000 // cpu_max,
                    'target_permille': threshold, 'target_failed_on_all_observed_resident_passes': True})
                serial = warm[f, 'gpu']['summary']['batch']
                cover = warm[f, 'gpu_recouvert']['summary']['batch']
                keys = ['count_ns', 'fill_ns', 'executor_ns', 'overlap_chunks', 'overlap_tail_ns',
                        'overlap_wait_ns', 'device_pool_used_high', 'device_pool_reserved_high', 'fill_jobs']
                delta_fill = cover['fill_ns'] - serial['fill_ns']
                delta_executor = cover['executor_ns'] - serial['executor_ns']
                comparisons.append({'k': k, 'frame': f, 'scope': 'dernière passe chaude seulement',
                    'serial': {n: serial[n] for n in keys}, 'covered': {n: cover[n] for n in keys},
                    'fill_delta_ns': delta_fill, 'executor_delta_ns': delta_executor,
                    'fill_share_of_executor_delta_permille': delta_fill * 1000 // delta_executor})
            results.append({'k': k, 'leaf': leaf, 'reps_cold_per_mode_and_frame': reps,
                'resident_passes_per_process': passes, 'cold_processes': len(q['cold']),
                'resident_processes': len(q['warm']), 'verdict': q['verdict'], 'refusals': q['refusals'],
                'report_sha256': sha(raw), 'bench_sha256': q['bench_sha256'], 'canonical': q['identity'],
                'ledger_reference_sha256': sha(json.dumps(q['ledger'], sort_keys=True).encode()),
                'ledger_equality_evidence': 'inférence du juge épinglé + refusals vide ; lignes natives par prise non conservées',
                'scope_declared': q['scope']})
    need(results[0]['bench_sha256'] == results[1]['bench_sha256'], 'binaires différents')
    return {'schema': 'audit_g4_l4_metadata_v1', 'source_pin': PIN,
        'receipt_status': r['status'], 'worker_exit_code': r['worker_exit_code'], 'closed_certified': True,
        'last_stop_timestamp': after['lastStopTimestamp'], 'receipt_sha256': sha(receipt_raw),
        'archive_sha256': r['results_sha256'], 'package_sha256': r['package_sha256'], 'plan_sha256': r['plan_sha256'],
        'source_equality': source, 'judge_sha256': sha(judge_raw), 'results': results,
        'retirement': {'published_pin': RETIRED, 'product_src_equal_to_baseline_pin': BASE,
                       'restored_parent_before_L4': BEFORE_L4, 'restored_paths': restored_paths,
                       'lifetime_defect_closed_by_removal_not_by_refusal_qualification': True},
        'developer_publication_pieces_identical': True, 'developer_publication_SHA256SUMS_valid': True,
        'totals': {'processes': processes, 'constructed_passes': constructed, 'canonical_dumps_checked': dumps,
                   'intermediate_passes_without_dump_or_ledger': intermediate},
        'performance_criteria': ranges, 'last_warm_fill_count_comparisons': comparisons,
        'scope': 'Sonde FULL, u21, K5 feuille16 et K10 feuille24, W48, 3 trames entières, 3 modes. Aucun API/CLI ni sanitizer qualifié.',
        'independent_refusal_lifetime_defect': 'Audit130b83534 reste indépendant des succès ; cette source cf28 ne qualifie pas les sorties de refus.',
        'native_or_cloud_runs_by_auditor': 0}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    p.add_argument('--session', type=Path, default=Path('/workspaces/.ehgp-sessions/v11.20261006.claudeL4'))
    p.add_argument('--capture', action='store_true')
    a = p.parse_args()
    result = derive(a.repo, a.session)
    target = Path(__file__).with_name('summary.json')
    text = json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2) + '\n'
    if a.capture:
        target.write_text(text)
    else:
        need(json.loads(target.read_text()) == result, 'résumé différent')
    print(text, end='')

if __name__ == '__main__':
    main()
