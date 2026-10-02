#!/usr/bin/env python3
"""Bounded MEB/census measurements after qualification; attempt protocol ported explicitly from index_g4."""
import argparse
from pathlib import Path
import subprocess
import sys
import time

from index_g4 import PROFILES, QUALIFIED, base, previous, load, summary, provenance
import meb_semantic as semantic

SCHEMA = 'ehgp.v11.meb_benchmark.v1'


def checked_builds(args):
    summary(args.qualification, QUALIFIED)
    outputs = {}
    for bits, name in PROFILES.items():
        path = args.qualification.parent / name / 'build_provenance.json'
        files = provenance(path, dict(MHGP11_COORD_BITS=str(bits), MHGP11_SANITIZE='OFF', MHGP11_TSAN='OFF',
                                      MHGP11_POISON='OFF'))
        exe = args.builds / name / 'build' / 'mhgp11_meb_bench'
        record = files[exe.name]
        semantic.need(record['sha256'] == base.digest(exe) and record['size'] == exe.stat().st_size,
                      'binaire non qualifie')
        outputs[bits] = dict(coord_bits=bits, configuration=name, path=str(exe), sha256=record['sha256'],
                             bytes=record['size'], provenance_sha256=base.digest(path))
    summary(args.supplement, {'gcc_asan_ubsan18'})
    path = args.supplement.parent / 'gcc_asan_ubsan18/build_provenance.json'
    files = provenance(path, dict(MHGP11_COORD_BITS='18', MHGP11_SANITIZE='ON', MHGP11_TSAN='OFF',
                                 MHGP11_POISON='OFF', MHGP11_MODULES='num;index;tower'))
    semantic.need({'mhgp11_tower_probe', 'mhgp11_meb_bench', 'libmhgp11.a'} <= set(files), 'cibles ASan18 tower')
    return outputs, base.digest(path)


def comparisons(rows):
    out = []
    for name in sorted(previous.COUNTS):
        matches = [row for row in rows if row['case'] == name and row['status'] == 'ok']
        same_output = len({r['semantic']['sha256'] for r in matches}) <= 1
        same_work = len({semantic.work_signature(r) for r in matches}) <= 1
        out.append(dict(case=name, successful_profiles=[r['coord_bits'] for r in matches],
                        semantic_equal=same_output, geometric_work_equal=same_work,
                        status='different' if not same_output or not same_work else
                        'equal' if len(matches) == 3 else 'incomplete'))
    return out


def measure(exe, case, bits, args, checkpoint):
    output = args.work / ('%s_b%d.bin' % (case['name'], bits))
    argv = [str(exe), str(args.data / case['coordinates']), str(args.data / case['point_ids']), str(output),
            str(semantic.BUDGET)]
    row = dict(case=case['name'], coord_bits=bits, repetition=0, count=case['count'], whole_input=True,
               argv=argv, timeout_seconds=30, status='running', exit_code=None, stdout='', stderr='',
               events=[], errors=[])
    checkpoint(row)
    started = time.monotonic()
    row['status'] = 'exited'
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
    checkpoint(row)
    if row['status'] == 'pending_semantic':
        row['status'] = 'exited'
        started = time.monotonic()
        try:
            semantic.need(row['stderr'] == '', 'diagnostic stderr natif inattendu')
            semantic.validate_events(row['events'], bits, case['count'])
        except (ValueError, KeyError, TypeError, OverflowError) as error:
            base.attempt_error(row, 'success', error, 'invalid_output')
        if row['status'] == 'exited':
            try:
                value = semantic.inspect(output, bits, case['count'], row['events'])
                row.update(semantic=value, canonical_sha256=value['raw_sha256'], canonical_bytes=value['bytes'],
                           status='ok')
            except (OSError, ValueError, KeyError, TypeError, OverflowError) as error:
                base.attempt_error(row, 'artifact', error, 'artifact_error')
        row['semantic_wall_seconds'] = time.monotonic() - started
    try:
        output.unlink(missing_ok=True)
    except OSError as error:
        base.attempt_error(row, 'cleanup', error, 'artifact_error')
    checkpoint(row)
    return row


def run(args):
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    builds, supplement_provenance = checked_builds(args)
    manifest, manifest_hash = previous.inputs(args.data)
    cases = sorted(manifest['cases'], key=lambda c: (c['name'].startswith('lidar'), c['count']))
    schedule = [(case, bits) for case in cases for bits in PROFILES]
    report = dict(schema=SCHEMA, meb_search=semantic.SEARCH, complete=False, conforming=False, requested_runs=18, leaf_size=8,
                  queries_per_run=48, thresholds=[5, 10, 13], repetitions_requested=1, timeout_seconds=30,
                  native_schedule_bound_seconds=540, manifest=manifest, manifest_sha256=manifest_hash,
                  qualification_sha256=base.digest(args.qualification), supplement_sha256=base.digest(args.supplement),
                  supplement_provenance_sha256=supplement_provenance, builds=list(builds.values()), runs=[],
                  not_run=[dict(case=c['name'], coord_bits=b, repetition=0, reason='pending') for c, b in schedule],
                  comparisons=[], full_schedule_completed=False,
                  scope='CPU bounded MEB and 48 chosen census queries; no descent/FULL/GPU/segmentation timing',
                  reference_scope='MEB inclusion/strict support and global scan share num; Fraction gate is independent',
                  timing_scope='read/cloud/index/MEB/census/wrapper/reference separate; process includes all and serialization',
                  memory_scope='Buffer Cloud/index plus census; wrapper retains first census (two results); no RSS/Python heap',
                  query_scope='four parts per size1..12 (two Morton-local/two dispersed); all input sites remain in the index')
    path = args.out / 'meb.json'
    base.save(path, report)
    for case, bits in schedule:
        ordinal = len(report['runs'])
        report['not_run'].pop(0)

        def checkpoint(row):
            if len(report['runs']) == ordinal:
                report['runs'].append(row)
            else:
                semantic.need(len(report['runs']) == ordinal + 1, 'checkpoint duplique')
                report['runs'][ordinal] = row
            report['comparisons'] = comparisons(report['runs'])
            base.save(path, report)

        row = measure(Path(builds[bits]['path']), case, bits, args, checkpoint)
        semantic.need(report['runs'][ordinal] is row, 'checkpoint absent')
        print('%s B%d %s' % (case['name'], bits, row['status']), flush=True)
    report['complete'] = True
    report['full_schedule_completed'] = len(report['runs']) == 18 and all(r['status'] == 'ok' for r in report['runs'])
    report['conforming'] = report['full_schedule_completed'] and all(c['status'] == 'equal' for c in report['comparisons'])
    base.save(path, report)
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('builds', 'data', 'out', 'work', 'qualification', 'supplement'):
        parser.add_argument('--' + option, type=Path, required=True)
    args = parser.parse_args()
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        print('meb_benchmark_refused: ' + type(error).__name__, flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
