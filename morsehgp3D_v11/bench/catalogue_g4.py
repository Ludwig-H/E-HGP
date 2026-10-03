#!/usr/bin/env python3
"""Mesures sequentielles du catalogue natif apres qualification, sans sous-echantillonnage LiDAR.

Une commande worker distincte ferme le groupe de la matrice avant ces mesures.
Chaque enfant est le binaire natif mono-processus, sans shell ni nouvelle session.
La tour FULL est absente : aucun succes de ce banc ne valide son contrat.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

import catalogue_diagnostics as diagnostics


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def save(path, value):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
    temporary.replace(path)


def checked_inputs(data):
    manifest_path = data / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    cases = manifest['cases']
    if not isinstance(cases, list) or len(cases) != 6:
        raise ValueError('six entrees entieres attendues')
    names = set()
    for case in cases:
        name = case['name']
        if name in names or not isinstance(name, str) or not name.replace('_', '').isalnum():
            raise ValueError('nom de cas invalide ou repete')
        names.add(name)
        for key, hash_key, width in [('coordinates', 'sha256', 12), ('point_ids', 'ids_sha256', 4)]:
            name = Path(case[key])
            if name.is_absolute() or len(name.parts) != 1:
                raise ValueError('fichier de donnees hors staging')
            path = data / name
            if (path.stat().st_size != width * case['count'] or digest(path) != case[hash_key]):
                raise ValueError('donnees ou nombre de retours divergents : ' + str(path))
    return manifest, digest(manifest_path)


def attempt_error(row, stage, error, status):
    """Keep the first process verdict and every subsequent collection error."""
    row['errors'].append({'stage': stage, 'type': type(error).__name__, 'message': str(error)})
    if row['status'] in ('exited', 'ok'):
        row['status'] = status


def json_value(text):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError('cle JSON repetee : ' + key)
            result[key] = value
        return result

    def constant(value):
        raise ValueError('constante JSON non finie : ' + value)

    return json.loads(text, object_pairs_hook=pairs, parse_constant=constant)


def event_json(line):
    result = json_value(line)
    if not isinstance(result, dict):
        raise ValueError('evenement JSON non objet')
    return result


def collect_success(row, case, output):
    try:
        phases = [event['phase'] for event in row['events']]
        if phases != ['cloud', 'catalogue', 'exit'] or any(
                event.get('status', 'ok') != 'ok' for event in row['events']):
            raise ValueError('succes natif sans toutes ses etapes')
        cloud, catalogue, _ = row['events']
        if row['stderr']:
            raise ValueError('stderr natif non vide sur succes')
        requested = diagnostics.check_request(row, catalogue)
        for event, fields in ((cloud, ('points', 'sites', 'cloud_ns', 'read_ns')),
                              (catalogue, ('balls', 'wall_ns'))):
            if any(type(event[key]) is not int or event[key] < 0 for key in fields):
                raise ValueError('compte ou duree natif non entier positif ou nul')
        if cloud['points'] != case['count'] or cloud['sites'] != case['count'] or catalogue['balls'] == 0:
            raise ValueError('entree tronquee, fusion inattendue ou catalogue vide')
        if requested:
            row['diagnostic_summary'] = diagnostics.check(catalogue, case['count'], catalogue['coord_bits'])
        row.update(catalogue_ms=catalogue['wall_ns'] / 1e6,
                   cloud_ms=cloud['cloud_ns'] / 1e6, read_ms=cloud['read_ns'] / 1e6,
                   catalogue_within_100ms=catalogue['wall_ns'] < 100_000_000)
    except (ValueError, KeyError, TypeError, OverflowError) as error:
        attempt_error(row, 'success', error, 'invalid_output')
        return
    try:
        row['canonical_sha256'] = digest(output)
        row['canonical_bytes'] = output.stat().st_size
        if row['canonical_bytes'] == 0:
            raise ValueError('sortie canonique vide')
        row['status'] = 'ok'
    except (OSError, ValueError) as error:
        attempt_error(row, 'artifact', error, 'artifact_error')


def measure(exe, case, k, repetition, args):
    output = args.work / ('%s_k%d_r%d.bin' % (case['name'], k, repetition))
    argv = [str(exe), str(args.data / case['coordinates']), str(args.data / case['point_ids']), str(output),
            str(k), str(args.leaf_size), '256', '0', str(2**32 - 1), str(8 * 1024**3)]
    workers, mode, diagnostic = getattr(args, 'workers', 0), getattr(args, 'optimizations', 0), getattr(args, 'diagnostics', False)
    if workers:
        argv.append(str(workers))
    if mode or diagnostic:
        argv.append(str(mode))
    if diagnostic:
        argv.append('1')
    row = {'case': case['name'], 'kmax': k, 'repetition': repetition, 'argv': argv,
           'timeout_seconds': args.timeout, 'whole_input': True, 'count': case['count'],
           'exit_code': None, 'stdout': '', 'stderr': '', 'events': [], 'errors': [], 'status': 'exited',
           'optimizations': mode, 'diagnostics': diagnostic}
    if workers:
        row['workers'] = workers
    started = time.monotonic()
    try:
        result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=args.timeout, check=False)
        row.update(exit_code=result.returncode, stdout=result.stdout.decode('utf-8', 'backslashreplace'),
                   stderr=result.stderr.decode('utf-8', 'backslashreplace'),
                   status='exited' if result.returncode == 0 else 'refused' if result.returncode == 2 else 'failed')
    except subprocess.TimeoutExpired as error:
        row.update(exit_code=None, stdout=(error.stdout or b'').decode('utf-8', 'backslashreplace'),
                   stderr=(error.stderr or b'').decode('utf-8', 'backslashreplace'), status='timeout')
        attempt_error(row, 'process', error, 'timeout')
    except OSError as error:
        attempt_error(row, 'launch', error, 'launch_error')
    row['process_wall_seconds'] = time.monotonic() - started
    for line in row['stdout'].splitlines():
        if not line.strip():
            continue
        try:
            row['events'].append(event_json(line))
        except (ValueError, TypeError) as error:
            attempt_error(row, 'events', error, 'invalid_output')
    if row['status'] == 'exited':
        collect_success(row, case, output)
    try:
        output.unlink(missing_ok=True)
    except OSError as error:
        attempt_error(row, 'cleanup', error, 'artifact_error')
    return row


def run(args):
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    qualification = json.loads(args.qualification.read_text())
    if qualification.get('conforming') is not True or qualification.get('exit_code') != 0:
        raise ValueError('matrice non conforme : mesures interdites')
    provenance_path = args.qualification.parent / 'gcc_release' / 'build_provenance.json'
    provenance = json.loads(provenance_path.read_text())
    executable_hash = digest(args.exe)
    recorded = [row for row in provenance['files'] if row['path'] == args.exe.name]
    if provenance.get('complete') is not True or len(recorded) != 1 or recorded[0]['sha256'] != executable_hash:
        raise ValueError('binaire different de celui de la qualification')
    manifest, manifest_hash = checked_inputs(args.data)
    report = {'schema': 'ehgp.v11.catalogue_benchmark.v1', 'scope': 'CPU catalogue, sans FULL ni GPU',
              'attempt_schema': 'ehgp.v11.catalogue_attempt.v2',
              'full_contract': 'not_testable_missing_tower', 'manifest_sha256': manifest_hash,
              'manifest': manifest, 'executable_sha256': executable_hash, 'runs': [], 'complete': False,
              'qualification_sha256': digest(args.qualification), 'repetitions_requested': 3,
              'requested_runs': 36, 'not_run': [], 'leaf_size': args.leaf_size,
              'workers': getattr(args, 'workers', 0), 'optimizations': getattr(args, 'optimizations', 0),
              'diagnostics': getattr(args, 'diagnostics', False),
              'timing_scope': 'catalogue_ms: deux passes, tri, sorties en memoire ; process: lecture et serialisation incluses',
              'memory_scope': 'reservations Buffer, Cloud vivant compris ; pas RSS'}
    report_path = args.out / 'catalogue.json'
    save(report_path, report)
    # Syntheses croissantes puis trois trames entieres ; aucune entree spatiale ou prefixe LiDAR.
    cases = sorted(manifest['cases'], key=lambda c: (c['name'].startswith('lidar'), c['count']))
    for case in cases:
        for k in (5, 10):
            hashes = set()
            for repetition in range(3):
                row = measure(args.exe, case, k, repetition, args)
                report['runs'].append(row)
                print('%s K%d r%d %s %s' % (case['name'], k, repetition, row['status'],
                                            row.get('catalogue_ms', 'sans temps complet')), flush=True)
                if row['status'] == 'ok':
                    hashes.add(row['canonical_sha256'])
                    if len(hashes) != 1:
                        row['status'] = 'nondeterministic'
                save(report_path, report)
                # Un echec ou une duree lourde se conserve une fois ; aucun rejeu jusqu'au vert.
                if row['status'] != 'ok' or row['process_wall_seconds'] > 10:
                    for omitted in range(repetition + 1, 3):
                        report['not_run'].append({'case': case['name'], 'kmax': k, 'repetition': omitted,
                                                 'reason': 'echec ou premiere execution superieure a10s'})
                    break
            if row['status'] != 'ok':
                if k == 5:
                    for omitted in range(3):
                        report['not_run'].append({'case': case['name'], 'kmax': 10, 'repetition': omitted,
                                                 'reason': 'premier refus/echec K5 conserve'})
                break
    report['complete'] = True
    report['all_attempted_ok'] = all(row['status'] == 'ok' for row in report['runs'])
    report['full_schedule_completed'] = len(report['runs']) == report['requested_runs'] and report['all_attempted_ok']
    save(report_path, report)
    return 0 if report['full_schedule_completed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('exe', 'data', 'out', 'work', 'qualification'):
        parser.add_argument('--' + option, type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=30)
    parser.add_argument('--leaf-size', type=int, default=32)
    parser.add_argument('--workers', type=int, default=0)
    parser.add_argument('--optimizations', type=int, default=0)
    parser.add_argument('--diagnostics', action='store_true')
    args = parser.parse_args()
    if not 1 <= args.timeout <= 60:
        parser.error('timeout entre 1 et 60 s')
    if not 13 <= args.leaf_size <= 256:
        parser.error('leaf-size entre 13 et 256 pour les lots K5/K10')
    if not 0 <= args.workers <= 256 or not 0 <= args.optimizations <= 7 or ((args.optimizations or args.diagnostics) and not args.workers):
        parser.error('masque0..7 et diagnostics exigent workers1..256')
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError) as error:
        print('catalogue_benchmark_refused: ' + str(error), flush=True)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
