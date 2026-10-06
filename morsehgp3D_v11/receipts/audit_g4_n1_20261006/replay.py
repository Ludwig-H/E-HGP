#!/usr/bin/env python3
"""Rejeu de lecture locale/Git, aucun build ni calcul HGP."""
import argparse
from collections import Counter
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import tarfile

PIN = 'cf28afb04040eacf1eb92080d75e8a0ebb58a4eb'
BASE = '8ee28873f2b9e59166b73a36e7b2302fa2a3a7a4'
PUBLISH = 'morsehgp3D_v11/receipts/developpement_20261006/n1_ab/'
PREFIX = 'results/cmd/000_ab/files/ab/'
EXPECTED = {'lidar_ng00': '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
            'lidar_ng01': '5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
            'lidar_ng02': '78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207'}

def need(ok, why):
    if not ok:
        raise ValueError(why)

def sha(b):
    return hashlib.sha256(b).hexdigest()

def sha_file(p):
    with p.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def git(repo, pin, path):
    return subprocess.check_output(['git', '-C', str(repo), 'show', pin + ':' + path])

def member(t, name):
    m = t.getmember(name)
    need(m.isfile() and m.size <= 16 * 1024 * 1024, 'membre non borné')
    return t.extractfile(m).read()

def tree(repo, pin, archive, worker):
    paths = ['morsehgp3D_v11/' + p for p in ['src', 'tests', 'cmake', 'bench', 'reference', 'tools', 'cli']]
    exact = ['morsehgp3D_v11/CMakeLists.txt'] + (['gcp-migration/v11_worker.sh'] if worker else [])
    raw = subprocess.check_output(['git', '-C', str(repo), 'archive', pin, '--', *paths, *exact])
    prefixes = tuple(p + '/' for p in paths)
    with tarfile.open(fileobj=io.BytesIO(raw)) as g, tarfile.open(archive) as a:
        wanted = {m.name for m in g.getmembers() if m.isfile()}
        actual = {m.name for m in a.getmembers() if m.isfile() and
                  (m.name.startswith(prefixes) or m.name in exact)}
        need(actual == wanted, 'inventaire source')
        hashes = []
        for n in sorted(wanted):
            b = member(g, n)
            need(b == member(a, n), 'source différente')
            hashes.append(sha(b) + '  ' + n + '\n')
    return {'pin': pin, 'files': len(wanted), 'manifest_sha256': sha(''.join(hashes).encode())}

def derive(repo, session, baseline):
    receipt_bytes = (session / 'receipt.json').read_bytes()
    r = json.loads(receipt_bytes)
    need((session / 'DONE').is_file(), 'session non close')
    need(r['closure'] == 'stopped' and r['targeted_shutdown_certified'] and r['start_certified'], 'fermeture')
    after = r['observed_after']
    need(after['status'] == 'TERMINATED' and after['name'] == r['target']['instance'], 'cible arrêtée')
    need(r['generation'] == r['closing_generation'] == after['lastStartTimestamp'], 'génération')
    need(r['private_key_deleted'] and r['reserve_released'] and r['oslogin_key_removed'], 'gardes')
    need(r['source_kind'] == 'commit' and r['evidence_grade'] == 'pushed_commit' and
         r['worker_source'] == 'commit:' + r['commit'], 'source worker')
    need(r['results_verified'] and r['data_verified_remote'] and not r['results_skipped_members'] and
         not r['overflow']['evicted'] and not r['overflow']['truncated_streams'], 'capture incomplète')
    arc = session / 'results/results.tar.gz'
    package = session / 'package/package.tar.gz'
    need(sha_file(arc) == r['results_sha256'] and sha_file(package) == r['package_sha256'], 'SHA archives')
    plan = (session / 'package/plan.json').read_bytes()
    need(sha(plan) == r['plan_sha256'], 'SHA plan')
    for name in ['receipt.json', 'launch.json', 'plan.json']:
        actual = plan if name == 'plan.json' else (session / name).read_bytes()
        need(git(repo, PIN, PUBLISH + name) == actual, 'pièce publiée différente')
    sums = git(repo, PIN, PUBLISH + 'SHA256SUMS').decode().splitlines()
    for line in sums:
        h, path = line.split(None, 1)
        need(sha(git(repo, PIN, PUBLISH + path.lstrip('*').removeprefix('./'))) == h, 'SHA publié')
    base_digest = next(x['sha256'] for x in r['data_files'] if x['name'] == 'base_src.tar.gz')
    need(sha_file(baseline) == base_digest, 'base livrée différente')
    sources = [tree(repo, r['commit'], package, True), tree(repo, BASE, baseline, False)]
    with tarfile.open(arc) as t:
        report_raw = member(t, PREFIX + 'ab_report.json')
        need(git(repo, PIN, PUBLISH + 'ab_report.json') == report_raw, 'rapport publié différent')
        report = json.loads(report_raw)
        need(report['archives_sha256']['base'] == base_digest, 'base exécutée')
        need(report['status'] == 'done' and report['verdict'] == 'refus' and
             report['refusals'] == ['ctest count 145 < 150'], 'refus inattendu')
        log = member(t, PREFIX + 'ctest_new.stdout')
        tests = re.findall(r'Test\s+#\d+:\s+(\S+)\s+\.+\s+(Passed|\*\*\*[^\n]+?)\s+\d+(?:\.\d+)? sec', log.decode())
        need(len(tests) == 145 and len({n for n, _ in tests}) == 145 and
             all(s == 'Passed' for _, s in tests), 'CTests')
        mutants_raw = member(t, PREFIX + 'mutants_catalogue.json')
        mutants = json.loads(mutants_raw)['mutants']
        need(len(mutants) == 2 and all(m['verdict'] == 'TUE' and m['detail'] == 'code' for m in mutants), 'mutants')
        rows = report['timings']
        need(len(rows) == 78 and len({x['name'] for x in rows}) == 78, 'prises')
        seen, work, fallbacks, pairs, streams = Counter(), {}, [], {}, {}
        for x in rows:
            need(x['code'] == 0 and x['quiet'] and x['dump_sha256'] == EXPECTED[x['frame']], 'prise refusée')
            need(x['cmd'][-2:] == [x['workers'], '16379'], 'mode différent')
            raw = member(t, PREFIX + x['name'] + '.stdout')
            phases = {}
            for line in raw.decode().splitlines():
                try:
                    value = json.loads(line)
                except ValueError:
                    continue
                if isinstance(value, dict) and 'phase' in value:
                    phases[value['phase']] = value
            d, f = phases['domain'], phases['full']
            need(f['status'] == 'ok' and phases['exit']['status'] == 'ok' and
                 x['summary']['domain_ns'] == f['domain_ns'], 'ligne native différente')
            need('pass' not in phases, 'régime résident inattendu')
            old = work.setdefault(x['frame'], d['catalogue_work'])
            need(old == d['catalogue_work'], 'registre logique différent')
            if x['variant'] == 'new':
                fallbacks.append(d['leaf_batch']['walk_fallbacks'])
            seen[x['frame'], x['workers'], x['variant']] += 1
            if x['workers'] == '48':
                pair = pairs.setdefault((x['frame'], x['rep']), {})
                pair[x['variant']] = f['domain_ns']
            streams[x['name']] = sha(raw)
        for frame in EXPECTED:
            for workers, count in [('48', 12), ('1', 1)]:
                for variant in ['base', 'new']:
                    need(seen[frame, workers, variant] == count, 'périmètre incomplet')
        need(len(fallbacks) == 39 and all(x == 0 for x in fallbacks), 'replis N1')
        negatives = {frame: sum(p['new'] < p['base'] for (f, _), p in pairs.items() if f == frame)
                     for frame in EXPECTED}
        need(all(n < 10 for n in negatives.values()), 'critère de rejet différent')
    return {'schema': 'audit_g4_n1_metadata_v1', 'publication_pin': PIN,
        'source_pin': r['commit'], 'baseline_pin': BASE, 'receipt_status': r['status'],
        'worker_exit_code': r['worker_exit_code'], 'closed_certified': True,
        'certified_last_stop_timestamp': after['lastStopTimestamp'],
        'receipt_sha256': sha(receipt_bytes), 'archive_sha256': r['results_sha256'],
        'package_sha256': r['package_sha256'], 'plan_sha256': r['plan_sha256'],
        'baseline_archive_sha256': base_digest, 'source_equality': sources,
        'report_sha256': sha(report_raw), 'published_pieces_identical': True, 'published_SHA256SUMS_valid': True,
        'benchmark_verdict': report['verdict'], 'benchmark_refusals': report['refusals'],
        'ctest': {'passed': 145, 'failed': 0, 'stdout_sha256': sha(log),
                  'selection': '^mhgp11_catalogue_, exclusion mutant|long, Release u21'},
        'mutants': [{'id': m['id'], 'verdict': m['verdict'], 'detail': m['detail'], 'judge': m['juge']} for m in mutants],
        'mutants_report_sha256': sha(mutants_raw), 'calls': 78, 'N1_calls': 39,
        'all_calls_status_ok': True, 'whole_frames': sorted(EXPECTED), 'canonical_dumps': EXPECTED,
        'ledger_equal_between_calls_and_variants': True, 'N1_walk_fallbacks_all_zero': True,
        'negative_paired_domain_differences_out_of_12': negatives,
        'gain_criterion_rejected_each_frame': True, 'stdout_manifest_sha256': sha(json.dumps(streams, sort_keys=True).encode()),
        'scope': 'CPU FULL/16379, K5, u21, processus neuf, W48 douze paires et W1 une paire par trame ; aucun GPU, TSan ou ASan joué par ce plan.',
        'causal_limit': 'Le rejet du gain N1 est établi. Le reçu ne mesure pas le CPU par tâche/feuille ; une causalité SMT exclusive ne découle pas de cet A/B.',
        'native_or_cloud_runs_by_auditor': 0}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    p.add_argument('--session', type=Path, default=Path('/workspaces/.ehgp-sessions/v11.20261006.claudeN1'))
    p.add_argument('--baseline', type=Path, default=Path('/workspaces/E-HGP/build/v11-persist/n1_ab/data/base_src.tar.gz'))
    p.add_argument('--capture', action='store_true')
    a = p.parse_args()
    result = derive(a.repo, a.session, a.baseline)
    target = Path(__file__).with_name('summary.json')
    text = json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2) + '\n'
    if a.capture:
        target.write_text(text)
    else:
        need(json.loads(target.read_text()) == result, 'résumé différent')
    print(text, end='')

if __name__ == '__main__':
    main()
