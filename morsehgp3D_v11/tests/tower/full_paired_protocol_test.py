#!/usr/bin/env python3
"""FULL acceleration and paired protocol gates; tiny data, no native processes or cloud."""
import copy
import hashlib
import io
import json
import os
import subprocess
from pathlib import Path
import sys
import tarfile
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'bench'))
import full_paired as paired
import full_qualification_context as context
from full_campaign_test import events, arguments, request, encode, VALUE

full = paired.full
CHECKS = 0


def check(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(reason)


def refused(call, reason):
    try:
        call()
    except (ValueError, KeyError, OSError):
        check(True, reason)
    else:
        raise ValueError('not refused: ' + reason)


def acceleration():
    value = events(optimizations=16379)[2]
    value.update(population_lookup=True, population_lookup_entries=2, population_lookup_reserved_bytes=64,
                 concurrent_orders=True, phases=dict.fromkeys(full.acceleration.PHASES, 20),
                 wall_ns=300, forest_ns=100, peak_reserved_bytes=value['peak_reserved_bytes'] + 64)
    for k, order in enumerate(value['orders'], 1):
        order['timings'].update(classify_ns=5, births_ns=20, plateaus_ns=20, verticals_ns=20 if k > 1 else 0)
        if k > 1:
            order['vertical_parallel'].update(vertical_batches=1, max_vertical_batch=order['births'],
                                              vertical_resolutions=order['work']['vertical_descents'])
    one, two = [o['work'] for o in value['orders'][:2]]
    one.update(descent_steps=1, population_hits=1, singleton_hits=1, part_meb_presentations=0)
    two.update(descent_steps=2, population_hits=1, catalogue_hits=1, census_calls=1,
               census_point_tests=1, part_meb_presentations=1, part_diameter_pairs=1)
    full.check_order_diagnostics(value, 3)
    check(sum(sum(o['timings'].values()) for o in value['orders']) > value['forest_ns'],
          'overlapping per-order walls accepted, disjoint global walls bounded')
    mutations = [lambda e: e.update(population_lookup=False),
                 lambda e: e.update(population_lookup_reserved_bytes=63),
                 lambda e: e['phases'].update(regular_ns=21),
                 lambda e: e['orders'][1]['timings'].update(births_ns=21),
                 lambda e: e['orders'][1]['work'].update(part_meb_presentations=0),
                 lambda e: e['orders'][1]['work'].update(part_diameter_pairs=2),
                 lambda e: e['orders'][0]['parallel'].update(regular_batches=1),
                 lambda e: e['orders'][1]['vertical_parallel'].update(vertical_batches=0),
                 lambda e: e.update(peak_reserved_bytes=900),
                 lambda e: e['orders'][0]['work'].update(memo_queries=1)]
    for mutate in mutations:
        corrupt = copy.deepcopy(value); mutate(corrupt)
        refused(lambda: full.check_order_diagnostics(corrupt, 3), 'corrupt acceleration ledger')
    # Population-before-memo: one terminal shortcut omits one query and one actual MEB call.
    memo = events(optimizations=4103)[2]
    memo.update(population_lookup=True, population_lookup_entries=2, population_lookup_reserved_bytes=64,
                peak_reserved_bytes=memo['peak_reserved_bytes'] + 64)
    work = memo['orders'][1]['work']
    work.update(population_hits=1, catalogue_hits=1, singleton_hits=0,
                census_calls=work['descent_steps']-1, census_point_tests=(work['descent_steps']-1)*2,
                part_meb_presentations=work['descent_steps']-1,
                memo_queries=work['memo_queries']-1, memo_lookups=work['memo_lookups']-1,
                memo_misses=work['memo_misses']-1, memo_insertions=work['memo_insertions']-1)
    full.check_order_diagnostics(memo, 3); check(True, 'valid memo plus population lookup')
    corrupt = copy.deepcopy(memo); corrupt['orders'][1]['work']['memo_queries'] += 1
    refused(lambda: full.check_order_diagnostics(corrupt, 3), 'memo query restored incorrectly')
    old = events(optimizations=2047)[2]
    for key in full.acceleration.FIELDS:
        old.pop(key)
    for order in old['orders']:
        order['work'].pop('population_hits')
    normalized = full.acceleration.producer_event(old, paired.baseline.BASELINE, full.need)
    full.check_order_diagnostics(normalized, 3)
    check('phases' not in old and all('population_hits' not in o['work'] for o in old['orders']),
          'original baseline stream preserved')
    refused(lambda: full.acceleration.producer_event(old, 'current', full.need), 'current may not omit new metadata')
    refused(lambda: full.acceleration.producer_event(old, 'wrong', full.need), 'unknown old producer')
    for mode in (4096, 4104, 8192+8, 16379, 16383):
        check(full.optimization(mode) == mode, 'public mode accepted')
    for mode in (True, -1, 16384, 8192, 128):
        refused(lambda: full.optimization(mode), 'invalid mode')


def archive(root, corrupt=False):
    source = 'commit:' + '1'*40
    summary = dict(complete=True, conforming=True, exit_code=0, signals=[],
                   configurations=[dict(name=n, status='ok') for n in full.profiles.PROFILES.values()])
    supplement = dict(schema='ehgp.v11.g4_matrix_summary.v1', complete=True, conforming=True, exit_code=0,
                      configurations=[dict(name='gcc_asan_ubsan18', status='ok')],
                      requested=['gcc_asan_ubsan18'], statuses={'gcc_asan_ubsan18':'ok'})
    files = {'results/worker.txt': ('source=%s\nstatus=completed\ncommands_ok=2\ncommands_total=2\n'
              'interrupted=0\noverflow_unresolved=0\n' % source).encode(),
             'results/cmd/000_matrice/files/matrix/summary.json': json.dumps(summary).encode(),
             'results/cmd/000_matrice/files/matrix/bits21/build_provenance.json': b'{}',
             'results/cmd/001_asan18/files/matrix/summary.json': json.dumps(supplement).encode()}
    lines = ['%s  ./%s' % (hashlib.sha256(data).hexdigest(), name.removeprefix('results/'))
             for name, data in files.items()]
    files['results/MANIFEST.sha256'] = ('\n'.join(lines)+'\n').encode()
    if corrupt:
        files['results/worker.txt'] += b'drift\n'
    path = root / ('corrupt.tar.gz' if corrupt else 'prior.tar.gz')
    with tarfile.open(path, 'w:gz') as t:
        for name, data in files.items():
            m = tarfile.TarInfo(name); m.size = len(data); t.addfile(m, io.BytesIO(data))
    return path, source


def protocol(root):
    rows = paired.schedule()
    check(len(rows) == len({paired.identity(r) for r in rows}) == 81, '81 unique requests')
    check(sum(r['build_variant'] == 'baseline' for r in rows) == 27, '27 old and54 new requests')
    for case in sorted({r['case'] for r in rows}):
        for workers in (1, 8, 48):
            group = [r for r in rows if r['case'] == case and r['workers'] == workers]
            check(len(group) == 9 and {r['repetition'] for r in group} == {0, 1, 2}, 'three whole paired repetitions')
            check(all({group[3*r+i]['build_variant'] for r in range(3)} == {v for v, _ in paired.VARIANTS}
                      for i in range(3)), 'counterbalanced positions')
    refs = root / 'refs'; refs.mkdir()
    raw = root / 'output.bin'; raw.write_bytes(b'tiny complete artifact')
    row = dict(case='test', semantic=dict(raw_sha256=full.base.digest(raw)))
    paired.compare_artifact(row, raw, refs)
    paired.compare_artifact(row, raw, refs); check(row['artifact_comparison']['status'] == 'equal_bytes', 'bytes before cleanup')
    raw.write_bytes(b'tiny complete artifacu')
    refused(lambda: paired.compare_artifact(row, raw, refs), 'same-length differing artifact')
    check(raw.exists(), 'failed artifact retained for caller')
    path, source = archive(root)
    with patch.dict(os.environ, V11_SOURCE_PIN=source):
        result = context.capture(path, full.base.digest(path), root / 'prior')
        check(result['source'] == source and len(result['copied']) == 5, 'closed prior archive extracted')
        bad, _ = archive(root, True)
        refused(lambda: context.capture(bad, full.base.digest(bad), root / 'bad'), 'inner manifest corruption')
        refused(lambda: context.capture(path, '0'*64, root / 'bad_hash'), 'archive pin mismatch')
    with patch.dict(os.environ, V11_SOURCE_PIN='commit:'+'2'*40):
        refused(lambda: context.capture(path, full.base.digest(path), root / 'wrong_source'), 'new source differs')
    manifest = full.profiles.load(paired.baseline.ARTIFACT / 'baseline_source_manifest.json')
    extracted = paired.baseline.extract(paired.baseline.ARTIFACT / manifest['archive'], manifest, root / 'old')
    check((extracted / 'bench/full_probe.cpp').is_file() and not (extracted / 'receipts').exists(),
          'explicit old source fixture excludes receipts')


def cache_origins(root):
    args = arguments(root, 'cache'); args.work.mkdir(); args.optimizations = 2047
    case = dict(name='test', count=3, coordinates='xyz', point_ids='ids', sha256='d'*64, ids_sha256='e'*64)
    cache = full.reuse.SummaryCache()
    rows = []
    for variant in ('baseline', 'current2047'):
        def child(argv, **_kwargs):
            values = events(optimizations=2047)
            if variant == 'baseline':
                for key in full.acceleration.FIELDS:
                    values[2].pop(key)
                for order in values[2]['orders']:
                    order['work'].pop('population_hits')
            Path(argv[3]).write_bytes(encode(VALUE, 21)[0])
            return subprocess.CompletedProcess(argv, 0, '\n'.join(json.dumps(e) for e in values).encode(), b'')
        wanted = dict(request(optimizations=2047), build_variant=variant,
                      producer_contract=paired.baseline.BASELINE if variant == 'baseline' else 'current')
        with patch.object(full.subprocess, 'run', side_effect=child):
            row = full.measure(root / 'fake', case, wanted, args, lambda _row: None, semantic_cache=cache)
        check(row['status'] == 'ok', 'fake complete native stream validates actual tiny artifact')
        rows.append(row)
    evidence = rows[-1]['semantic_reuse']
    check(evidence['mode'] == 'reused' and evidence['source_attempt'][0] == 'baseline' and
          evidence['current_attempt'][0] == 'current2047', 'shared cache origin distinguishes producers')
    with patch.object(full.subprocess, 'run', side_effect=child):
        row = full.measure(root / 'fake', case, wanted, args, lambda _row: None, semantic_cache=cache,
            artifact_callback=lambda _row, _path: full.need(False, 'deliberate paired discrepancy'),
            preserve_failed_artifact=True)
    check(row['status'] == 'different_output' and Path(row['retained_artifact']).is_file(),
          'difference checkpoint retains actual dump before cleanup')


def main():
    acceleration()
    with tempfile.TemporaryDirectory(prefix='mhgp11-paired-pure-') as directory:
        protocol(Path(directory)); cache_origins(Path(directory))
    print('full_paired_protocol_verdict conforme checks%d native0' % CHECKS)


if __name__ == '__main__':
    main()
