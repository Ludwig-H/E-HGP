#!/usr/bin/env python3
"""Pinned old/new CPU FULL comparisons: three whole LiDARs, W1/8/48, three fresh process repetitions."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import statistics
import sys
import time

import full_campaign as full
import full_baseline_source as baseline
import full_qualification_context as qualification

base, need = full.base, full.need
SCHEMA = 'ehgp.v11.full_paired_campaign.v1'
VARIANTS = (('baseline', 2047), ('current2047', 2047), ('current16379', 16379))


def schedule():
    rows = []
    lidar = sorted(name for name in full.profiles.COUNTS if name.startswith('lidar'))
    for repetition in range(3):
        for case in lidar:
            for workers in (1, 8, 48):
                # Rotate all three producers so each occupies every position once for each case/W.
                order = VARIANTS[repetition:] + VARIANTS[:repetition]
                rows += [dict(build_variant=name, case=case, coord_bits=21, kmax=5, workers=workers,
                              repetition=repetition, optimizations=mode) for name, mode in order]
    return rows


def identity(row):
    return (row['build_variant'],) + full.identity(row)


def source_inventory(source):
    files = [source / 'CMakeLists.txt']
    for name in ('src', 'cmake', 'tests', 'reference', 'bench', 'tools'):
        files += [p for p in (source / name).rglob('*') if p.is_file() and '__pycache__' not in p.parts and
                  p.suffix not in ('.pyc', '.pyo')]
    files.sort()
    rows = [dict(path=str(p.relative_to(source)), bytes=p.stat().st_size, sha256=base.digest(p)) for p in files]
    digest = hashlib.sha256(
        '\n'.join('%s %s %d' % (r['sha256'], r['path'], r['bytes']) for r in rows).encode()).hexdigest()
    return dict(files=rows, sha256=digest)


def byte_equal(left, right):
    if left.stat().st_size != right.stat().st_size:
        return False
    with left.open('rb') as a, right.open('rb') as b:
        while True:
            x, y = a.read(1 << 20), b.read(1 << 20)
            if x != y:
                return False
            if not x:
                return True


def compare_artifact(row, output, references):
    reference = references / (row['case'] + '.bin')
    if not reference.exists():
        shutil.copyfile(output, reference)
        row['artifact_comparison'] = dict(status='reference', reference=str(reference),
                                         raw_sha256=base.digest(reference))
    else:
        need(byte_equal(reference, output), 'paired FULL artifact differs byte for byte before cleanup')
        row['artifact_comparison'] = dict(status='equal_bytes', reference=str(reference),
                                         raw_sha256=base.digest(reference))
    need(row['artifact_comparison']['raw_sha256'] == row['semantic']['raw_sha256'],
         'semantic inspection and paired artifact disagree')


def comparisons(rows, requested):
    wanted, actual = [identity(r) for r in requested], [identity(r) for r in rows]
    need(len(set(wanted)) == len(wanted) and len(set(actual)) == len(actual) and set(actual) <= set(wanted),
         'paired campaign inventory')
    result = []
    for case in sorted({r['case'] for r in requested}):
        expected = [r for r in requested if r['case'] == case]
        found = [r for r in rows if r['case'] == case and r['status'] == 'ok']
        equal = len({(r['semantic']['sha256'], r['semantic']['raw_sha256']) for r in found}) <= 1
        timings = [dict(build_variant=variant, workers=workers, repetitions=len(values),
                        full_ms=values, median_full_ms=statistics.median(values))
                   for variant, _ in VARIANTS for workers in (1, 8, 48)
                   if (values := [r['full_ms'] for r in found if r['build_variant'] == variant and r['workers'] == workers])]
        result.append(dict(case=case, requested=len(expected), successful=len(found), semantic_and_bytes_equal=equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete',
                           timings=timings))
    return result


def run(args):
    started = time.monotonic()
    need(180 <= args.budget_seconds <= 2400, 'paired campaign budget')
    source = os.environ.get('V11_SOURCE_PIN', '')
    need(re.fullmatch(r'commit:[0-9a-f]{40}', source) is not None, 'current session source pin')
    need(args.source.resolve() == Path(os.environ['V11_SRC']).resolve() and
         args.builds.resolve().is_relative_to(Path(os.environ['V11_BUILD']).resolve()),
         'new same-session targeted build/source directories')
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    current, prior = qualification.checked_current(args)
    supplement = prior['supplement_sha256']
    source_before = source_inventory(args.source / 'morsehgp3D_v11')
    base.save(args.out / 'source_before.json', source_before)
    current.update(source_commit=source.split(':')[1], source_manifest_sha256=base.digest(args.out / 'source_before.json'),
                   source_manifest_path=str(args.out / 'source_before.json'),
                   source_root=str(args.source / 'morsehgp3D_v11'),
                   cache_path=str(Path(current['path']).parent / 'CMakeCache.txt'),
                   provenance_path=str(args.qualification.parent / current['configuration'] / 'build_provenance.json'))
    baseline.checked_pin(current)
    manifest, manifest_hash = full.profiles.inputs(args.data)
    cases = {c['name']: c for c in manifest['cases']}
    requested = schedule()
    report = dict(schema=SCHEMA, complete=False, conforming=False, source=source,
                  package_sha256=os.environ.get('V11_PACKAGE_SHA256'), generation=os.environ.get('V11_GENERATION'),
                  prior_context_sha256=base.digest(args.prior_context), prior_qualification=prior,
                  qualification_context='closed earlier same-source matrix; new binary built and targeted gates played in current session',
                  qualification_sha256=base.digest(args.qualification), supplement_sha256=supplement,
                  manifest=manifest, manifest_sha256=manifest_hash, requested=requested, requested_runs=len(requested),
                  runs=[], launch_intents=[], not_run=[], comparisons=[], build_status='pending',
                  budget_seconds=args.budget_seconds, timeout_seconds=full.TIMEOUT,
                  scope='CPU FULL K1..5, unit weights, three whole nonground SemanticKITTI frames; u21 same 1mm inputs',
                  cold_scope='fresh process and native owners each attempt; OS caches not dropped or claimed cold',
                  timing_scope='native FULL wall=index+domain+forest; input IO/Cloud/Pool/process/decoder separate',
                  excluded='baseline compilation, grid/ground preparation, projection, head, GPU',
                  memory_scope='Buffer reservations plus Cloud, not RSS; compare all reported phases and CPU separately',
                  semantic_reuse_scope='one shared campaign cache; current complete payload rehashed every attempt',
                  byte_comparison_scope='every complete artifact compared byte for byte to first case artifact before cleanup',
                  work_comparison_scope='report actual work; no invariant across different algorithms or worker counts')
    path = args.out / 'full_paired.json'

    def save():
        report['comparisons'] = comparisons(report['runs'], requested)
        base.save(path, report)

    save()
    try:
        old = baseline.build(args.work, args.out, current, args.build_timeout_seconds, args.build_threads)
        report.update(build_status='ok', builds=[old, current]); save()
        cache = full.reuse.SummaryCache()
        references = args.work / 'references'; references.mkdir()
        for variant, _ in VARIANTS:
            (args.work / variant).mkdir()
        for request in requested:
            if time.monotonic() - started + full.TIMEOUT + 20 >= args.budget_seconds:
                report['not_run'].append(dict(request, reason='campaign_budget_before_launch')); save(); continue
            record = old if request['build_variant'] == 'baseline' else current
            case = cases[request['case']]
            need(base.digest(args.data / case['coordinates']) == case['sha256'] and
                 base.digest(args.data / case['point_ids']) == case['ids_sha256'], 'current invocation input hash')
            call = argparse.Namespace(**vars(args)); call.optimizations = request['optimizations']
            call.work = args.work / request['build_variant']
            pin = baseline.checked_pin(record)
            intent = full.launch_intent(Path(record['path']), cases[request['case']], 21, 5, call,
                                       request['workers'], request['repetition'], full.TIMEOUT, request['optimizations'])
            intent.update(build_variant=request['build_variant'], build_pin=pin)
            report['launch_intents'].append(intent); save()
            ordinal = len(report['runs'])
            enriched = dict(request, build_pin=pin, producer_contract=baseline.BASELINE if
                            request['build_variant'] == 'baseline' else 'current')

            def checkpoint(row):
                need(len(report['runs']) == ordinal and identity(row) == identity(request), 'paired checkpoint')
                report['runs'].append(row); save()

            row = full.measure(Path(record['path']), cases[request['case']], enriched, call, checkpoint,
                semantic_cache=cache, artifact_callback=lambda row, output: compare_artifact(row, output, references),
                preserve_failed_artifact=True)
            row['build_pin_after'] = baseline.checked_pin(record)
            need(row['build_pin_after'] == pin, 'binary/cache changed during attempt')
            need(base.digest(args.data / case['coordinates']) == case['sha256'] and
                 base.digest(args.data / case['point_ids']) == case['ids_sha256'], 'input changed during attempt')
            report['runs'][ordinal] = row; save()
            print('%s %s W%d r%d %s' % (request['case'], request['build_variant'], request['workers'],
                                       request['repetition'], row['status']), flush=True)
        source_after = source_inventory(args.source / 'morsehgp3D_v11')
        base.save(args.out / 'source_after.json', source_after)
        need(source_after == source_before, 'current source changed during paired benchmark')
        report['complete'] = True
        report['conforming'] = not report['not_run'] and len(report['runs']) == len(requested) and all(
            r['status'] == 'ok' for r in report['runs']) and all(c['status'] == 'equal' for c in report['comparisons'])
    except BaseException as error:
        report['failure'] = dict(type=type(error).__name__, message=str(error))
        raise
    finally:
        report['campaign_wall_seconds'] = time.monotonic() - started
        save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('builds', 'data', 'out', 'work', 'qualification', 'prior-context', 'source'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--budget-seconds', type=int, default=1200)
    parser.add_argument('--build-timeout-seconds', type=int, default=180)
    parser.add_argument('--build-threads', type=int, choices=range(1, 49), default=16)
    args = parser.parse_args()
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError) as error:
        print('full_paired_refused: ' + str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
