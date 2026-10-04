#!/usr/bin/env python3
"""Assembly ablation: real tiny decoding, fake children, current checks before cache publication."""
import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from bench_parallel_collector_test import environment, setup
from bench_optimizations_test import events, ready
from bench_semantic_test import fixture
import catalogue_assembly as driver

CHECKS = 0


def need(condition, reason):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(reason)


def native(bits, workers, mode):
    value = events(bits, workers, mode)
    value[1]['diagnostics_requested'] = False
    return value


def inventories():
    requests = driver.schedule()
    need(len(requests) == len(set(map(driver.identity, requests))) == 36, '36 unique requests')
    need([r['optimizations'] for r in requests[:4]] == [3,11,7,15], 'independent frontier/assembly pairs')
    need(all(r['workers'] == 48 and r['kmax'] == 5 and not r['diagnostics'] for r in requests), 'whole K5 W48')
    rows = [ready(r) for r in requests]
    for row in rows:
        row['events'] = native(row['coord_bits'], row['workers'], row['optimizations'])
    need(all(c['status'] == 'equal' for c in driver.comparisons(rows, requests)), 'six exact groups')
    for field in ('semantic', 'raw', 'cache', 'work', 'duplicate', 'unknown', 'boolean'):
        changed = copy.deepcopy(rows)
        if field == 'semantic': changed[1]['semantic']['sha256'] = 'b'*64
        if field == 'raw': changed[1]['canonical_sha256'] = 'b'*64
        if field == 'cache': changed[1]['events'][1]['cache_work']['hits'] += 1
        if field == 'work': changed[1]['events'][1]['logical']['nodes'] += 1
        if field == 'duplicate': changed.append(changed[0])
        if field == 'unknown': changed[1]['optimizations'] = 8
        if field == 'boolean': changed[1]['optimizations'] = True
        if field in ('duplicate','unknown','boolean'):
            try: driver.comparisons(changed, requests)
            except ValueError: pass
            else: raise ValueError('bad comparison inventory accepted')
        else:
            changed[0]['status'] = 'timeout'
            need(driver.comparisons(changed, requests)[0]['status'] == 'different', 'incomplete divergence retained')
    return 7


def campaigns(root):
    modes = ('ok', 'no_cache', 'baseline_failed', 'parallel_failed', 'wrong_mode', 'wrong_workers', 'unsolicited',
             'wrong_cache', 'decode_failed', 'deadline', 'no_budget', 'interrupt', 'decoder_interrupt')
    children = decodes = 0
    for failure in modes:
        args, manifest, builds = environment(root, failure)
        args.budget_seconds = 700; args.reuse_semantic = failure != 'no_cache'
        for i, case in enumerate(manifest['cases']):
            case['sha256'] = format(i + 1, '064x')
        elapsed, ticks, calls, decoded = 0, 0, [], 0
        original_decode = driver.profiles.semantic.decode
        original_digest = driver.base.digest

        def clock():
            nonlocal ticks
            ticks += 1
            return 700 if failure == 'no_budget' and ticks > 1 else elapsed

        def decode(*a, **kw):
            nonlocal decoded
            decoded += 1
            if failure == 'decoder_interrupt':
                raise KeyboardInterrupt
            if failure == 'decode_failed' and decoded == 1:
                raise ValueError('injected semantic refusal')
            return original_decode(*a, **kw)

        def child(argv, **kw):
            nonlocal elapsed
            before = json.loads((args.out/'assembly.json').read_text())
            need(before['launch_intents'][-1]['argv'] == argv, 'intent precedes process')
            need(len(argv) == 12 and kw['timeout'] == 15, 'no diagnostic payload, bounded child')
            mode, workers = int(argv[11]), int(argv[10])
            bits = next(bits for bits, v in builds.items() if v['path'] == argv[0])
            calls.append((bits, mode))
            if failure == 'interrupt':
                raise KeyboardInterrupt
            target = len(calls) == 1 if failure != 'parallel_failed' else len(calls) == 2
            values = native(bits, workers, mode)
            if target:
                if failure == 'wrong_mode': values[1]['optimizations'] = 11
                if failure == 'wrong_workers': values[1]['workers'] = 8
                if failure == 'unsolicited': values[1]['diagnostics'] = {}
                if failure == 'wrong_cache': values[1]['cache_work']['hits'] += 1
            Path(argv[3]).write_bytes(fixture(bits)[0])
            if failure == 'deadline': elapsed += 100
            code = 2 if target and failure in ('baseline_failed', 'parallel_failed') else 0
            return subprocess.CompletedProcess(argv, code, '\n'.join(json.dumps(e) for e in values).encode(), b'')

        with setup(manifest, builds), patch.object(driver.profiles.subprocess, 'run', side_effect=child), \
                patch.object(driver.profiles.semantic, 'decode', side_effect=decode), \
                patch.object(driver.base, 'digest', side_effect=lambda p: original_digest(p) if p.suffix == '.bin' else 'b'*64), \
                patch.object(driver.time, 'monotonic', side_effect=clock):
            try: code = driver.run(args)
            except KeyboardInterrupt:
                need(failure in ('interrupt','decoder_interrupt'), 'only requested interruption')
            else: need(code == (0 if failure in ('ok','no_cache') else 1), 'campaign verdict '+failure)
        report = json.loads((args.out/'assembly.json').read_text())
        need(report['conforming'] is (failure in ('ok','no_cache')), 'conformity requires complete valid schedule')
        need(len(report['launch_intents']) == len(calls), 'intent/child inventory')
        if failure == 'decoder_interrupt':
            need(not report['complete'] and len(report['runs']) == len(calls) == 1, 'native checkpoint retained')
            row = report['runs'][0]
            need(row['status'] == 'pending_semantic' and 'semantic' not in row and
                 Path(row['argv'][3]).exists(), 'interrupted decoder retains unfinished artifact')
        elif failure == 'interrupt':
            need(not report['complete'] and not report['runs'] and len(calls) == 1, 'no invented process result')
        else:
            all_rows = report['runs'] + report['not_run']
            need(len(all_rows) == 36 and set(map(driver.identity, all_rows)) == set(map(driver.identity, driver.schedule())),
                 'every unit has result or explicit omission')
            need(report['complete'], 'closed inventory')
            if failure not in ('deadline','no_budget'):
                need(len(calls) == 36 and not report['not_run'], 'failure does not suppress paired mode')
            if failure == 'no_cache':
                need(decoded == 36 and all('semantic_reuse' not in r for r in report['runs']), 'default branch decodes every artifact')
            if failure == 'ok':
                need(decoded == 12, 'one real decode per input/profile, 24 verified reuse hits')
            if failure in ('wrong_mode','wrong_workers','unsolicited','wrong_cache','decode_failed'):
                first, second = report['runs'][:2]
                need(first['status'] != 'ok' and second['semantic_reuse']['mode'] == 'decoded',
                     'failed current validation cannot publish cache')
            if failure == 'deadline': need(len(calls) == 7 and len(report['not_run']) == 29, 'bounded scheduling')
            if failure == 'no_budget': need(not calls and not decoded, 'no budget means no child or decoder')
        children += len(calls); decodes += decoded
    return len(modes), children, decodes


def main():
    with tempfile.TemporaryDirectory(prefix='mhgp11-assembly-collector-') as folder, contextlib.redirect_stdout(io.StringIO()):
        corruptions = inventories()
        schedules, children, decodes = campaigns(Path(folder))
    need(CHECKS >= 600 and schedules == 13 and corruptions == 7, 'coverage floor')
    print(json.dumps(dict(checks=CHECKS, schedules=schedules, corruptions=corruptions,
                          mock_children=children, tiny_decodes=decodes, native=0), sort_keys=True))
    print('assembly_collector_verdict conforme')


if __name__ == '__main__':
    main()
