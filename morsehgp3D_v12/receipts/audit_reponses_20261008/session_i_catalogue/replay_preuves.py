#!/usr/bin/env python3
"""Relecture Git/JSON seulement ; aucun moteur, cloud ou fichier de donnees."""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def replay(repo):
    pins = json.loads((HERE / 'pins.json').read_text())
    cache = {}

    def git(pin, path):
        key = pin + ':' + path
        if key not in cache:
            cache[key] = subprocess.check_output(['git', 'show', key], cwd=repo)
        return cache[key]

    def receipt(path):
        return git(pins['receipt_commit'], pins['receipt_path'] + '/' + path)

    for path, expected in pins['inputs'].items():
        require(sha(receipt(path)) == expected, 'entree differente : ' + path)
    lines = receipt('SHA256SUMS').decode().splitlines()
    for line in lines:
        expected, path = line.split('  ', 1)
        require(sha(receipt(path)) == expected, 'manifeste differente : ' + path)
    host = json.loads(receipt('receipt.json'))
    base = 'resultats/cmd/000_t1b_catalogue_appareil/files/t1b/'
    report = json.loads(receipt(base + 'report.json'))
    manifest = {item['path']: item for item in host['source']['manifest']}
    important = [path for path in manifest if path.startswith('morsehgp3D_v12/src/')]
    important += ['morsehgp3D_v12/CMakeLists.txt', 'morsehgp3D_v12/bench/catalogue_probe.cpp',
                  'morsehgp3D_v12/tests/catalogue/device_pipeline_test.cpp']
    modules = ['g4_catalogue_device.py', 'g4_catalogue_judge.py', 'g4_catalogue_schema.py']
    important += ['morsehgp3D_v12/bench/' + name for name in modules]
    for path in important:
        require(sha(git(pins['source_commit'], path)) == manifest[path]['sha256'],
                'source du snapshot differente du commit : ' + path)

    def log(name):
        raw = receipt(base + 'logs/' + name + '.log').decode()
        command, body = raw.split('\n', 1)
        out, err = body.split('\n--- stderr ---\n', 1)
        require(command.startswith('$ '), 'commande absente : ' + name)
        return out, err

    with tempfile.TemporaryDirectory(prefix='audit_t1bi_') as tmp:
        for name in modules:
            (Path(tmp) / name).write_bytes(git(pins['source_commit'], 'morsehgp3D_v12/bench/' + name))
        sys.path.insert(0, tmp)
        import g4_catalogue_device as driver
        import g4_catalogue_judge as judge
        verdict = judge.judge(report)
        require(verdict == report['verdict'] and verdict['verdict'] == 'adopte', 'verdict different')
        matched = []

        def match(name, summary):
            out, err = log(name)
            require(not err.strip(), 'stderr non vide : ' + name)
            require(driver.summary(driver.parse_probe(out)) == summary, 'resume different : ' + name)
            matched.append(name)

        identity = []
        for item in report['steps']['identity']:
            for role, suffix in [('cpu', 'cpu'), ('device', 'dev')]:
                match('id_%s_k%d_%s' % (item['case'], item['k'], suffix), item[role])
            digests = item['cpu']['digests'] + item['device']['digests']
            require(len(set(digests + [judge.F2_DIGESTS[item['case'] + ':' + str(item['k'])]])) == 1,
                    'identite F2 differente')
            row = item['device']['passes'][0]
            identity.append(dict(case=item['case'], k=item['k'], cpu_digests=len(item['cpu']['digests']),
                                 device_digests=len(item['device']['digests']), equal_to_f2=True,
                                 replayed_leaves=row['device']['replayed_leaves'],
                                 leaves_rewritten=row['diagnostics']['leaves_rewritten']))
        for group in ['cpu_timing', 'device_timing_k5', 'device_timing_k10']:
            for item in report['steps'][group]:
                name = ('cpu_%s_k%d_f%d' % (item['case'], item['k'], item['leaf'])) if group == 'cpu_timing' else (
                    'dev_k%d_%s_p%d' % (5 if group.endswith('k5') else 10, item['frame'], item['process']))
                match(name, item['run'])
        mutants = []
        for item in report['steps']['mutants']:
            name = 'mutant_' + item['id']
            if item['critere'] == 'identite':
                match(name + '_ng00', item['ng00'])
                out, err = log(name + '_unit')
                require(item['unit_code'] == 1 and 'ECHEC' in out + err, 'mutant unite non tue')
                mutants.append(dict(id=item['id'], criterion='identite', unit_code=1,
                                    ng00_digest_changed=item['ng00_digest'] != item['reference_digest']))
            else:
                match(name + '_temps', item['run'])
                warm = [row['wall_ns'] for row in item['run']['passes'][1:]]
                mutants.append(dict(id=item['id'], criterion='temps',
                                    warm_median_ms=statistics.median(warm) / 1e6))
        out, err = log('device_open')
        require(out.rstrip() == report['steps']['gates']['device_open_stdout'].rstrip() and
                'controles=217 echecs=0' in out and not err.strip(), 'preuve device_open differente')
        for name in ['gpu_apps_avant', 'gpu_apps_apres']:
            out, err = log(name)
            require(not out.strip() and not err.strip(), 'GPU non isole')
        out, _ = log('ctest')
        require('100% tests passed, 0 tests failed out of 665' in out, 'portes differentes')

    require(host['closure'] == 'stopped' and host['targeted_shutdown_certified'] is True and
            host['observed_after']['status'] == 'TERMINATED' and host['stop_exit_code'] == 0 and
            host['worker_exit_code'] == 0 and host['results_verified'] is True and not host['errors'],
            'cloture non certifiee')
    return dict(verdict='adopte', archived_verdict_exact=True, manifest_entries=len(lines),
                source_files_equal_to_git=len(important), source_kind=host['source_kind'],
                worker_source=host['worker_source'], raw_summaries_matched=len(matched),
                identity=identity, mutants=mutants, device_open=dict(witnesses=9, controls=217, failures=0),
                fast_gates=665, gpu_quiet_before_after=True,
                stop=dict(closure=host['closure'], status=host['observed_after']['status'],
                          certified=host['targeted_shutdown_certified'],
                          last_stop=host['observed_after']['lastStopTimestamp']),
                cloud_or_native_run_by_auditor=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo-root', type=Path, default=HERE.parents[3])
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = replay(args.repo_root)
    if args.check:
        require(result == json.loads((HERE / 'preuves.json').read_text()), 'resultats differents')
        print('session_i_preuves_ok: sources, 57 journaux, juge, GPU et arret archives')
    else:
        print(json.dumps(result, indent=1, sort_keys=True))
