#!/usr/bin/env python3
"""One bounded catalogue schedule per compiled profile; CPU only, no FULL or GPU claim."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

import catalogue_g4 as base
import catalogue_semantic as semantic


PROFILES = {18: 'gcc_release', 21: 'bits21', 24: 'bits24'}
COUNTS = dict(lidar_ng00=39885, lidar_ng01=35551, lidar_ng02=45845,
              uniform_u18_n8000=8000, uniform_u18_n16000=16000, uniform_u18_n32000=32000)
SCHEMA = 'ehgp.v11.catalogue_profiles.v2'
WORK_SCHEMA = 'ehgp.v11.catalogue_work.v1'
NATIVE_BUDGET = 36 * 30
LOGICAL = {'nodes', 'leaves', 'filter_tests', 'dominance_tests', 'prefixes', 'judged', 'census_tests',
           'max_leaf', 'max_depth'}


def load(path):
    return base.event_json(path.read_text())


def checked_builds(args):
    summary = load(args.qualification)
    semantic.need(summary.get('conforming') is True and summary.get('exit_code') == 0 and
                  summary.get('complete') is True, 'qualification incomplete/non conforme')
    configs = summary['configurations']
    names = [row['name'] for row in configs]
    semantic.need(len(names) == len(set(names)), 'qualification dupliquee')
    outputs = {}
    for bits, name in PROFILES.items():
        semantic.need(names.count(name) == 1 and next(c for c in configs if c['name'] == name)['status'] == 'ok',
                      'configuration absente/non conforme')
        provenance_path = args.qualification.parent / name / 'build_provenance.json'
        provenance = load(provenance_path)
        semantic.need(provenance['schema'] == 'ehgp.v11.build_provenance.v1' and
                      provenance['complete'] is True and not provenance['errors'], 'provenance incomplete')
        rows = provenance['files']
        paths = [row['path'] for row in rows]
        semantic.need(len(paths) == len(set(paths)), 'provenance dupliquee')
        exe = args.builds / name / 'build' / 'mhgp11_catalogue_bench'
        record = next(row for row in rows if row['path'] == exe.name)
        semantic.need(record['sha256'] == base.digest(exe) and record['size'] == exe.stat().st_size,
                      'binaire different de sa qualification')
        cache = next(row for row in rows if row['path'] == 'CMakeCache.txt')
        text = cache['text'].encode()
        semantic.need(hashlib.sha256(text).hexdigest() == cache['sha256'] and len(text) == cache['size'],
                      'cache de compilation corrompu')
        entries = [line.split('=', 1)[1] for line in cache['text'].splitlines()
                   if line.startswith('MHGP11_COORD_BITS:')]
        semantic.need(entries == [str(bits)], 'profil compile different du profil demande')
        outputs[bits] = {'configuration': name, 'coord_bits': bits, 'path': str(exe),
                         'sha256': record['sha256'], 'bytes': record['size'],
                         'provenance_sha256': base.digest(provenance_path), 'cache_sha256': cache['sha256']}
    return outputs


def inputs(data):
    load(data / 'manifest.json')  # Reject duplicate keys/nonfinite JSON before the historical hash/size checks.
    manifest, sha = base.checked_inputs(data)
    semantic.need(manifest['schema'] == 'mhgp11.catalogue_benchmark_inputs.v1' and
                  {c['name']: c['count'] for c in manifest['cases']} == COUNTS, 'inventaire des six entrees')
    for case in manifest['cases']:
        semantic.need(type(case['count']) is int and case['profile'] == 'quantized_u18_input_only' and
                      case['unit_site_weights'] is True and case['duplicate_sites'] == 0, 'profil entree commun')
    return manifest, sha


def checked_supplement(path):
    value = load(path)
    configurations = value['configurations']
    semantic.need(value['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and value['complete'] is True and
                  value['conforming'] is True and type(value['exit_code']) is int and value['exit_code'] == 0 and
                  not value.get('signals') and len(configurations) == 1 and
                  configurations[0]['name'] == 'gcc_asan_ubsan18' and configurations[0]['status'] == 'ok' and
                  value['requested'] == ['gcc_asan_ubsan18'] and
                  value['statuses'] == {'gcc_asan_ubsan18': 'ok'}, 'supplement ASan18 non conforme')
    return base.digest(path)


def success(row, case, output, bits):
    base.collect_success(row, case, output)
    if row['status'] != 'ok':
        return
    started = time.monotonic()
    try:
        event = row['events'][1]
        semantic.need(event['coord_bits'] == bits and event['kmax'] == row['kmax'] and
                      event['generation_passes'] == 2 and event['peak_reserved_bytes'] <= 8 * 1024**3 and
                      0 <= event['reserved_after_bytes'] <= event['peak_reserved_bytes'], 'profil/parametres natifs')
        work_signature(row)
        value = semantic.inspect(output, bits, row['kmax'], case['count'], arity_counts=True)
        semantic.need(all(value[k] == event[k] for k in ('balls', 'levels', 'incidences')),
                      'comptes JSON/canonique divergents')
        row['qmin_counts'] = value.pop('qmin_counts')
        q4_signature(row, value['balls'])
        row['semantic'] = value
    except (OSError, ValueError, KeyError, TypeError, OverflowError) as error:
        base.attempt_error(row, 'artifact', error, 'artifact_error')
    row['semantic_wall_seconds'] = time.monotonic() - started


def measure(exe, case, bits, kmax, args, checkpoint=None):
    # Same attempt.v2 process/error precedence as catalogue_g4.measure, with decoding before cleanup.
    output = args.work / ('%s_b%d_k%d.bin' % (case['name'], bits, kmax))
    argv = [str(exe), str(args.data / case['coordinates']), str(args.data / case['point_ids']), str(output),
            str(kmax), '16', '256', '0', str(2**32 - 1), str(8 * 1024**3)]
    row = dict(case=case['name'], coord_bits=bits, kmax=kmax, repetition=0, argv=argv, timeout_seconds=30,
               whole_input=True, count=case['count'], exit_code=None, stdout='', stderr='', events=[], errors=[],
               status='exited')
    started = time.monotonic()
    try:
        result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=30, check=False)
        row.update(exit_code=result.returncode, stdout=result.stdout.decode('utf-8', 'backslashreplace'),
                   stderr=result.stderr.decode('utf-8', 'backslashreplace'),
                   status='exited' if result.returncode == 0 else 'refused' if result.returncode == 2 else 'failed')
    except subprocess.TimeoutExpired as error:
        row.update(stdout=(error.stdout or b'').decode('utf-8', 'backslashreplace'),
                   stderr=(error.stderr or b'').decode('utf-8', 'backslashreplace'), status='timeout')
        base.attempt_error(row, 'process', error, 'timeout')
    except OSError as error:
        base.attempt_error(row, 'launch', error, 'launch_error')
    row['process_wall_seconds'] = time.monotonic() - started
    for line in row['stdout'].splitlines():
        if line.strip():
            try:
                row['events'].append(base.event_json(line))
            except (ValueError, TypeError) as error:
                base.attempt_error(row, 'events', error, 'invalid_output')
    if row['status'] == 'exited':
        row['status'] = 'pending_semantic'
    if checkpoint is not None:
        checkpoint(row)
    if row['status'] == 'pending_semantic':
        row['status'] = 'exited'
        success(row, case, output, bits)
    try:
        output.unlink(missing_ok=True)
    except OSError as error:
        base.attempt_error(row, 'cleanup', error, 'artifact_error')
    return row


def work_signature(row):
    event = row['events'][1]
    values = event['logical']
    semantic.need(set(values) == LOGICAL and all(type(v) is int and v >= 0 for v in values.values()),
                  'compteurs geometriques natifs')
    semantic.need(event['generation_passes'] == 2, 'nombre de passes')
    return (event['generation_passes'], *(values[name] for name in sorted(LOGICAL)))


def comparisons(rows):
    out = []
    for name in sorted(COUNTS):
        for kmax in (5, 10):
            matches = [row for row in rows if row['case'] == name and row['kmax'] == kmax and row['status'] == 'ok']
            semantic_equal = len({r['semantic']['sha256'] for r in matches}) <= 1
            work_equal = len({work_signature(r) for r in matches}) <= 1
            out.append({'case': name, 'kmax': kmax, 'successful_profiles': [r['coord_bits'] for r in matches],
                        'semantic_equal': semantic_equal, 'geometric_work_equal': work_equal,
                        'status': ('different' if not semantic_equal or not work_equal
                                   else 'equal' if len(matches) == 3 else 'incomplete')})
    return out


def q4_signature(row, balls=None):
    value = row['events'][1]['work']
    counts = row['qmin_counts']
    semantic.need(set(value) == {'q4_candidates', 'q4_levels'} and
                  all(type(v) is int and v >= 0 for v in value.values()), 'compteurs q4')
    semantic.need(set(counts) == {'2', '3', '4'} and all(type(v) is int and v >= 0 for v in counts.values()) and
                  sum(counts.values()) == (row['semantic']['balls'] if balls is None else balls), 'comptes qmin')
    semantic.need(value['q4_levels'] == counts['4'] <= value['q4_candidates'], 'niveaux q4 differes')
    return value['q4_candidates'], value['q4_levels']


def q4_comparisons(rows):
    out = []
    for name in sorted(COUNTS):
        for kmax in (5, 10):
            matches = [r for r in rows if r['case'] == name and r['kmax'] == kmax and r['status'] == 'ok']
            signatures = {q4_signature(r) for r in matches}
            out.append({'case': name, 'kmax': kmax, 'successful_profiles': [r['coord_bits'] for r in matches],
                        'status': 'different' if len(signatures) > 1 else 'equal' if len(matches) == 3 else 'incomplete'})
    return out


def run(args):
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    builds = checked_builds(args)
    supplement_hash = checked_supplement(args.supplement)
    manifest, manifest_hash = inputs(args.data)
    report = {'schema': SCHEMA, 'attempt_schema': 'ehgp.v11.catalogue_attempt.v2', 'complete': False,
              'scope': 'CPU catalogue only; no FULL/GPU/segmentation timing', 'leaf_size': 16,
              'manifest': manifest, 'manifest_sha256': manifest_hash, 'builds': list(builds.values()),
              'qualification_sha256': base.digest(args.qualification), 'requested_runs': 36,
              'supplement_sha256': supplement_hash, 'work_schema': WORK_SCHEMA,
              'repetitions_requested': 1, 'timeout_seconds': 30, 'native_schedule_bound_seconds': NATIVE_BUDGET,
              'runs': [], 'not_run': [], 'comparisons': [], 'q4_comparisons': [], 'full_schedule_completed': False,
              'q4_work_scope': 'per generation pass; two identical passes; q4_levels equals emitted qmin4 balls',
              'timing_scope': 'API: two passes/sort/output memory; process: input/output included; semantic decode separate',
              'memory_scope': 'native Buffer reservations including live Cloud; not RSS or Python decoder'}
    report_path = args.out / 'profiles.json'
    base.save(report_path, report)
    cases = sorted(manifest['cases'], key=lambda c: (c['name'].startswith('lidar'), c['count']))
    for case in cases:
        for bits in PROFILES:
            for kmax in (5, 10):
                ordinal = len(report['runs'])

                def checkpoint(row):
                    semantic.need(len(report['runs']) == ordinal, 'tentative deja inseree')
                    report['runs'].append(row)
                    report['comparisons'] = comparisons(report['runs'])
                    report['q4_comparisons'] = q4_comparisons(report['runs'])
                    base.save(report_path, report)

                row = measure(Path(builds[bits]['path']), case, bits, kmax, args, checkpoint)
                semantic.need(len(report['runs']) == ordinal + 1, 'checkpoint de tentative absent')
                report['runs'][ordinal] = row
                report['comparisons'] = comparisons(report['runs'])
                report['q4_comparisons'] = q4_comparisons(report['runs'])
                if row['status'] != 'ok' and kmax == 5:
                    report['not_run'].append({'case': case['name'], 'coord_bits': bits, 'kmax': 10,
                                               'repetition': 0, 'reason': 'same_profile_K5_failed'})
                base.save(report_path, report)
                print('%s B%d K%d %s' % (case['name'], bits, kmax, row['status']), flush=True)
                if row['status'] != 'ok':
                    break
    report['complete'] = True
    report['all_attempted_ok'] = all(row['status'] == 'ok' for row in report['runs'])
    report['full_schedule_completed'] = len(report['runs']) == 36 and report['all_attempted_ok']
    report['conforming'] = report['full_schedule_completed'] and all(
        c['status'] == 'equal' for c in report['comparisons'] + report['q4_comparisons'])
    base.save(report_path, report)
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('builds', 'data', 'out', 'work', 'qualification', 'supplement'):
        parser.add_argument('--' + option, type=Path, required=True)
    args = parser.parse_args()
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        print('catalogue_profiles_refused: ' + type(error).__name__, flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
