#!/usr/bin/env python3
"""Inert capture of 29 pinned functional comparisons; no engine or timing ratio.

Execution needs ROOT GO, this file's SHA, a reviewed matrix PATH=SHA, explicit
closed build/micro/heavy PATH=SHA and the new header SHA. Matrix.jobs contains
the ordered 24 micro and five heavy cases, each with old/new, old_protocol/
new_protocol and old_sources/new_sources PATH=SHA, old_kind, scope and optional
prefix_orders. scope=skip additionally requires reason; an unattempted new arm
uses null new/new_sources. Every real comparison is invoked normally and -O.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import signal
import sys
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_successor_20260905_mono_controller'
IMPORTED = ROOT / 'build/v7_full_lazy_20260905_probe_controller/capture.py'
IMPORTED_SHA = '417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa'
COMPARE = BASE / 'compare.py'
COMPARE_SHA = '15176b19ff7dd6b56c56470a887ecc9438fd673215cdefbd48f1a49953015718'
JUDGE = ROOT / 'morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py'
JUDGE_SHA = '5de5b8d20d6073d5521a4217b8d504a59d255fad514eb8f4ddc257553ee0f032'
HEADER = 'morsehgp3D_v7/src/forest/full_gabriel.hpp'
SINGLETON = ROOT / 'build/v7_singleton_20260905_mono_controller'
LAZY = ROOT / 'build/v7_full_lazy_20260905_probe_controller'
SCHEMA = 'mhgp7-successor-comparison-capture-v1'


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda: stream.read(1048576), b''):
            value.update(part)
    return value.hexdigest()


def module(path: Path, pin: str) -> types.ModuleType:
    require(digest(path) == pin, 'protocol pin drift')
    value = types.ModuleType(path.stem + '_isolated')
    value.__file__ = str(path)
    exec(compile(path.read_bytes(), str(path), 'exec'), value.__dict__)
    return value


def execute(args: argparse.Namespace) -> None:
    own = Path(__file__).resolve()
    require(digest(own) == args.expected_controller_sha256, 'own reviewed pin required')
    c = module(IMPORTED, IMPORTED_SHA)
    c.BASE = BASE
    for sig in c.SIGNALS:
        signal.signal(sig, c.interrupted)
    watched = {}

    def watch(path: Path, pin: str | None = None) -> str:
        require(path.is_absolute() and path.is_relative_to(ROOT) and '..' not in path.parts,
                'absolute input scope')
        require(all(not p.is_symlink() for p in (path, *path.parents)), 'input symlink')
        require(path.is_file() and path.stat().st_size <= 32 << 20, 'input bound')
        stamp = digest(path)
        require(pin is None or re.fullmatch('[0-9a-f]{64}', pin) is not None and stamp == pin, 'input pin')
        require(str(path) not in watched or watched[str(path)]['sha256'] == stamp, 'input drift')
        watched[str(path)] = {'sha256': stamp, 'bytes': path.stat().st_size}
        return str(path) + '=' + stamp

    def argument(text: str) -> tuple[Path, dict]:
        name, sep, pin = text.rpartition('=')
        require(sep, 'explicit PATH=SHA256 required')
        path = Path(name)
        watch(path, pin)
        return path, c.read_json(path)

    def closure(text: str, kind: str) -> tuple[Path, dict]:
        path, value = argument(text)
        require(path.name == 'receipt.json' and path.parent.is_relative_to(BASE)
                and value['schema'] == 'mhgp7-successor-mono-controller-v1'
                and value['kind'] == kind and value['status'] in ('completed', 'failed'), 'closed capture')
        require(kind == 'heavy' or value['status'] == 'completed' and value['sources_stable'] is True,
                'build/micro admission failed')
        for name, pin in value['artifacts'].items():
            target = path.parent / name
            require(target.is_relative_to(path.parent) and '..' not in target.parts, 'artifact escape')
            watch(target, pin)
        require({str(p.relative_to(path.parent)) for p in path.parent.rglob('*') if p.is_file()}
                == set(value['artifacts']) | {'receipt.json'}, 'closed exact inventory')
        return path, value

    for path, pin in ((own, args.expected_controller_sha256), (IMPORTED, IMPORTED_SHA),
                      (COMPARE, COMPARE_SHA), (JUDGE, JUDGE_SHA), (ROOT / HEADER, args.new_producer_sha256)):
        watch(path, pin)
    matrix_path, matrix = argument(args.matrix)
    build_path, build = closure(args.build, 'build')
    micro_path, micro = closure(args.micro, 'micro')
    heavy_path, heavy = closure(args.heavy, 'heavy')
    require(micro['build_argument'] == args.build and micro['binary'] == build['binary']
            and micro['binary_sha256'] == build['binary_sha256'], 'build/micro binding')
    admission = c.read_json(heavy_path.parent / 'admission.json')
    require(admission['micro_argument'] == args.micro, 'micro/heavy binding')
    for path in (build_path, micro_path, heavy_path):
        source = c.read_json(path.parent / 'sources_before.json')
        require(source['files'][HEADER] == args.new_producer_sha256, 'new producer binding')
    for name, pin in c.read_json(micro_path.parent / 'sources_before.json')['files'].items():
        watch(ROOT / name, pin)
    require(matrix['schema'] == SCHEMA and matrix['build_argument'] == args.build
            and matrix['micro_argument'] == args.micro and matrix['heavy_argument'] == args.heavy
            and matrix['new_producer_sha256'] == args.new_producer_sha256, 'matrix admission binding')
    specs = [(8, k, s, p, cap) for k in (5, 10) for s in (8, 10, 12)
             for p, cap in (('eager', 0), ('lazy', 0), ('lazy', 1), ('lazy', 1000000))]
    specs += [(8000, 10, s, 'lazy', 1000000) for s in (8, 10, 12)]
    specs += [(n, 10, 8, 'lazy', 1000000) for n in (16000, 32000)]
    require(type(matrix['jobs']) is list and len(matrix['jobs']) == 29, 'exact comparison inventory')
    directory = c.new_directory('comparison', args.id)
    result = dict(schema=SCHEMA, kind='comparison', status='failed', started=c.now(), jobs=[],
                  controller_sha256=args.expected_controller_sha256, comparator_sha256=COMPARE_SHA,
                  imported_controller_sha256=IMPORTED_SHA, receipt_judge_sha256=JUDGE_SHA,
                  build_argument=args.build, micro_argument=args.micro, heavy_argument=args.heavy,
                  matrix_argument=args.matrix, new_producer_sha256=args.new_producer_sha256,
                  engine_invoked=False, gcp_used=False, historical_timing_pair_claim=False,
                  public_status='not_claimed', global_FULL_successful=False)
    try:
        protocol = directory / 'protocol'
        protocol.mkdir()
        for name, path in (('compare_capture.py', own), ('compare.py', COMPARE),
                           ('imported_controller.py', IMPORTED), ('primary_judge.py', JUDGE),
                           ('matrix.json', matrix_path)):
            with (protocol / name).open('xb') as stream:
                stream.write(path.read_bytes())
        c.save(directory / 'sources_before.json', dict(watched))
        for index, (job, spec) in enumerate(zip(matrix['jobs'], specs)):
            n, k, s, policy, cap = spec
            stem = f'n{n}_s{s}_k{k}_{policy}_c{cap}'
            old_dir = SINGLETON / 'micro_admission_new' / f'k{k}' if n == 8 else (
                SINGLETON / 'paired_mono' / {8: '01_new_s8', 10: '02_new_s10', 12: '05_new_s12'}[s]
                if n == 8000 else LAZY / f'heavy_scale{n // 1000}_resume')
            new_dir = micro_path.parent / f'k{k}' if n == 8 else heavy_path.parent
            require(job['id'] == stem and job['old_kind'] == ('singleton21b77' if n <= 8000 else 'lazy13c6'),
                    'job identity/source kind')
            records = {}
            for arm, parent in (('old', old_dir), ('new', new_dir)):
                for key, filename in ((arm, stem + '.receipt.json'), (arm + '_protocol', 'protocol.json'),
                                      (arm + '_sources', stem + '.sources_before.json')):
                    if job[key] is None:
                        require(arm == 'new' and n != 8 and key != 'new_protocol'
                                and not (parent / filename).exists(), 'only absent heavy may be skipped')
                        continue
                    path, value = argument(job[key])
                    require(path == parent / filename, 'job metadata route')
                    if key == arm:
                        records[arm] = value
            row = dict(id=stem, scope=job['scope'], old_kind=job['old_kind'])
            if job['scope'] == 'skip':
                require(n != 8 and type(job['reason']) is str and bool(job['reason']), 'explicit heavy skip reason')
                new = records.get('new')
                require((new is None and heavy['status'] == 'failed') or
                        (new is not None and (new['status'] != 'completed' or new.get('terminal') is None
                         or new['exit_code'] in (2, 3) and not any(r['outcome'] == 'complete_relative' for r in new['orders']))),
                        'valid terminal capture must be compared, not skipped')
                row.update(status='skipped', reason=job['reason'], new_capture_status=None if new is None else new['status'])
            else:
                old, new = records['old'], records['new']
                complete = old['exit_code'] == new['exit_code'] == 0
                require(job['scope'] == ('complete' if complete else 'successful-prefix-diagnostic'), 'honest comparison scope')
                options = ['--scope', job['scope']]
                if not complete:
                    count = min(sum(r['outcome'] == 'complete_relative' for r in v['orders']) for v in (old, new))
                    require(type(job['prefix_orders']) is int and job['prefix_orders'] == count
                            and count > 0 and (n != 32000 or count <= 8), 'bounded successful prefix')
                    options += ['--prefix-orders', str(count)]
                else:
                    require(job.get('prefix_orders') is None, 'complete comparison has no prefix')
                options += ['--expected-comparator-sha256', COMPARE_SHA, '--new-producer-sha256', args.new_producer_sha256,
                            '--old-kind', job['old_kind'], '--judge', str(JUDGE) + '=' + JUDGE_SHA]
                for key in ('old', 'new', 'old_protocol', 'new_protocol', 'old_sources', 'new_sources'):
                    options += ['--' + key.replace('_', '-'), job[key]]
                outputs = []
                for optimized in (False, True):
                    label = f'{index:02d}_{stem}_' + ('optimized' if optimized else 'normal')
                    argv = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B'] + (['-O'] if optimized else [])
                    transport = c.command(directory, label, argv + [str(COMPARE)] + options, (0, 1, 2), 60)
                    require(transport['status'] == 'completed' and transport['exit_code'] == 0, 'comparison invocation rejected:' + label)
                    output = c.read_json(directory / (label + '.stdout'))
                    require(output['comparison_status'] == 'valid' and output['scope'] == job['scope'], 'comparison verdict')
                    for name, stamp in output['inputs'].items():
                        watch(Path(name), stamp['sha256'])
                    outputs.append(output)
                require(json.dumps(outputs[0], sort_keys=True) == json.dumps(outputs[1], sort_keys=True), 'normal/-O mismatch')
                row.update(status='valid', orders_compared=outputs[0]['orders_compared'], input_digest=outputs[0]['input_digest'],
                           old_global_exit_code=outputs[0]['old_global_exit_code'], new_global_exit_code=outputs[0]['new_global_exit_code'],
                           global_complete_comparison=outputs[0]['global_complete_comparison'])
            result['jobs'].append(row)
        result.update(status='completed', reason='comparison_transport_closed_not_global_FULL_success',
                      comparisons_valid=sum(r['status'] == 'valid' for r in result['jobs']),
                      comparisons_skipped=sum(r['status'] == 'skipped' for r in result['jobs']),
                      all_planned_comparisons_valid=all(r['status'] == 'valid' for r in result['jobs']))
    except BaseException as error:
        result['reason'] = type(error).__name__ + ': ' + str(error)
    finally:
        after = {}
        for name in watched:
            try:
                after[name] = {'sha256': digest(Path(name)), 'bytes': Path(name).stat().st_size}
            except OSError as error:
                after[name] = {'unreadable': type(error).__name__ + ': ' + str(error)}
        c.save(directory / 'inputs_observed.json', watched)
        c.save(directory / 'sources_after.json', after)
        result['inputs_stable'] = watched == after
        if not result['inputs_stable']:
            result.update(status='failed', reason='input/source/digest drift')
        result.update(ended=c.now(), artifacts={str(p.relative_to(directory)): digest(p) for p in sorted(directory.rglob('*')) if p.is_file()})
        c.save(directory / 'receipt.json', result)
    print(json.dumps(result, sort_keys=True))
    require(result['status'] == 'completed', 'comparison capture failed')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    for name in ('id', 'expected-controller-sha256', 'matrix', 'build', 'micro', 'heavy', 'new-producer-sha256'):
        parser.add_argument('--' + name)
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps({'status': 'prepared_not_executed', 'requires_ROOT_GO': True, 'controller_sha256': digest(Path(__file__))}))
        return
    require(all(value is not None for value in vars(args).values()), 'explicit reviewed arguments required')
    execute(args)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(json.dumps({'status': 'failed', 'reason': str(error)}))
        raise SystemExit(1)
