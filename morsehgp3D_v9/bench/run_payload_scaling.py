#!/usr/bin/env python3
"""Local FULL host-twin ON/OFF capture; no GPU or LiDAR precision claim.

Writes fresh receipts and fresh synthetic inputs only. Every command, including
failures, is retained. --readback rechecks raw evidence, input/source/binary
hashes and paired objects; it is intentionally a LIVE, not standalone, reader.
The v30 section validators are reused without relabelling synthetic data 1 mm.
"""
import argparse
import copy
import datetime
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import signal
import struct
import subprocess
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
FAMILIES = ('uniform', 'terrain', 'clusters')
SIZES = (8000, 16000, 32000)
RECIPES = dict(uniform='uniform_splitmix64_v1', terrain='terrain_splitmix64_v1',
               clusters='eight_corner_clusters_splitmix64_v1')
DIGESTS = ('tower_digest', 'catalogue_digest', 'presentation_digest')
PAYLOAD_COUNTS = ('payload_keys', 'payload_ids', 'payload_fallback_keys')
CATALOGUE_CHANGE = frozenset(PAYLOAD_COUNTS + ('census_nodes', 'census_leaf_tests'))
SCHEMA = 'mhgp9_q3_payload_local_v1'


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    with Path(path).open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256')
    return digest.hexdigest()


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def save(path, value):
    # Receipt refresh is local to this newly created capture, never old evidence.
    temp = path.with_name(path.name + '.new')
    temp.write_text(json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n')
    temp.replace(path)


def worker():
    spec = importlib.util.spec_from_file_location('payload_scaling_worker', ROOT / 'gcp-migration/tower_worker_v9.py')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    need(module.PROBE_SCHEMA == 'mhgp9_tower_probe_v30', 'unsupported section-reader version')
    return module


def strict_json(path, w):
    return w.strict_json(Path(path).read_text())


def pins(probe, exporter):
    source = ROOT / 'morsehgp3D_v9'
    paths = [p for p in (source / 'src').rglob('*') if p.suffix in ('.hpp', '.cpp', '.cu', '.h')]
    paths += [source / name for name in ('CMakeLists.txt', 'bench/tower_probe.cpp',
              'bench/front_fixture_export.cpp', 'bench/run_payload_scaling.py', 'tests/gen/front_fixtures.hpp')]
    paths += [ROOT / 'gcp-migration/tower_worker_v9.py']
    paths += [Path(probe), Path(exporter)]
    # Build recipe files, not mutable timing/test logs. No claim that hashes
    # alone prove source-to-binary compilation; the parent build capture does.
    for target in ('mhgp9_tower_probe', 'mhgp9_front_fixture_export'):
        for name in ('flags.make', 'link.txt'):
            path = Path(probe).parent / 'CMakeFiles' / (target + '.dir') / name
            if path.is_file():
                paths.append(path)
    return {str(p.resolve()): sha(p) for p in sorted(set(paths))}


def input_identity(path, grid, family=None):
    raw = Path(path).read_bytes()
    need(len(raw) > 0 and len(raw) % 12 == 0, 'invalid u32le byte size')
    points = list(struct.iter_unpack('<III', raw))
    need(len(set(points)) == len(points), 'duplicate input site')
    maximum = 65535 if family else 262143
    need(all(max(point) <= maximum for point in points), 'input coordinate outside declared domain')
    h = 14695981039346656037
    for word in (len(points), *(x for point in points for x in point)):
        for byte in struct.pack('<Q', word):
            h = ((h ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return dict(path=str(Path(path).resolve()), n=len(points), sha256=hashlib.sha256(raw).hexdigest(),
                bytes=len(raw), fnv=f'{h:016x}', grid=grid, family=family,
                recipe=RECIPES.get(family), coordinate_max=maximum)


def levers(w, enabled):
    result = dict.fromkeys(w.LEVER_NAMES, True)
    for key in ('q34_gpu_filter', 'q34_gpu_certificates', 'q34_gpu_q3', 'device_session'):
        result[key] = False
    result['q3_interior_payload'] = enabled
    need(w._levers(result), 'invalid lever recipe')
    return result


def case_for(data, w, enabled):
    return dict(scene=data['family'] or '00', n=data['n'], k=5, s=8, workers=4,
                static_threads=4, frames=1, levers=levers(w, enabled))


def probe_command(probe, data, w, enabled):
    result = [str(probe), data['path'], '5', '4', '--s=8', '--static=4',
              '--grid=' + data['grid'], '--frames=1', '--catalogue-digest']
    result += [f'--lever={name}={int(value)}' for name, value in levers(w, enabled).items()]
    return result


def validate_probe(value, data, enabled, elapsed, w):
    """Generic input identity, strict v30 body, then shared section invariants."""
    case = case_for(data, w, enabled)
    need(type(value) is dict and set(value) == w.TOP_KEYS and value['schema'] == w.PROBE_SCHEMA,
         'probe top-level schema')
    need(value['status'] == 'complete_relative' and value['reason'] == w.complete_reason(case['levers']),
         'FULL complete_relative expected')
    need(value['input'] == dict(format='u32le', grid=data['grid'], sites=data['n'], hash=data['fnv']),
         'probe input identity')
    expected = dict(K=5, K_effective=5, s=8, workers=4, tower_static_threads=4, run_tower=True,
                    certificate_capacity=0, certificate_judge=False, lanes_capacity=0,
                    lanes_judge=False, lanes_events=0, levers=case['levers'])
    need(value['options'] == expected, 'probe option recipe')
    times = value['times_ms']
    need(type(times) is dict and set(times) == w.TIME_KEYS and all(w._number(x) for x in times.values())
         and w._number(value['chain_cpu_s']), 'probe times')
    stages = [key for key in w.STAGE_TIME_KEYS if key != 'q2'] + ['q2_wait']
    need(sum(times[key] for key in stages) <= times['chain_total'] + .01 * len(stages) and
         times['q2_wait'] <= times['q2'] + .05 and times['q2_census_index'] <= times['q2_census'] + .05 and
         times['q2_census_wait'] <= times['q2_census'] + .05, 'overlapped stage timing')
    need(w._counters(value['generator'], w.GENERATOR_KEYS) and w._counters(value['ledger'], w.LEDGER_KEYS)
         and w._tower_work(value['tower_work']) and w._catalogue(value['catalogue']), 'probe work/catalogue schema')
    need(type(value['orders']) is list and all(w._counters(row, w.ORDER_KEYS) for row in value['orders']) and
         [row['K'] for row in value['orders']] == [1, 2, 3, 4, 5], 'explicit FULL orders 1..5')
    need(all(type(value[key]) is str and re.fullmatch('[0-9a-f]{16}', value[key]) for key in DIGESTS),
         'three digest encodings')
    need(type(value['peak_rss_kb']) is int and value['peak_rss_kb'] >= -1, 'RSS type')
    frames, host = value['frames'], value['host']
    need(type(frames) is dict and set(frames) == w.FRAMES_KEYS and frames['count'] == 1 and
         frames['same_object'] is True and all(type(frames[key]) is list and len(frames[key]) == 1 and
         w._number(frames[key][0]) for key in w.FRAME_LISTS), 'frames fields')
    need(type(host) is dict and set(host) == w.HOST_KEYS and type(host['thp']) is str and
         re.fullmatch('[a-z_]{1,32}', host['thp']), 'host fields')
    need(value['device_session'] == dict(opened=False, context_ms=0, reserve_ms=0, pinned_ms=0, pinned_bytes=0),
         'host capture opened device session')
    for validate in (w.validate_euler, w.validate_occupancy, w.validate_batch,
                     w.validate_tower_detail, w.validate_tower_phases, w.validate_frames):
        validate(value, case)
    w.validate_ledger_identities(value, case['levers'])
    for frame_key, key in (('chain_total_ms', 'chain_total'), ('tower_ms', 'tower'),
                           ('q34_ms', 'q34'), ('census_ms', 'census')):
        need(abs(frames[frame_key][0] - times[key]) <= .05, 'first-frame clock differs')
    need(abs(frames['lanes_transfer_ms'][0] - value['q34_batch']['lanes_transfer_ms']) <= .05,
         'first-frame transfer clock differs')
    w.validate_external_wall(value, elapsed)
    if not data['family']:
        need(data['n'] == w.INPUTS['00']['n'] and data['sha256'] == w.INPUTS['00']['sha256'] and
             data['fnv'] == w.INPUTS['00']['fnv'], 'optional whole no-ground frame identity')
        need((value['tower_digest'], value['catalogue_digest']) == w.PINNED_DIGESTS[('00', 5)],
             'optional LiDAR published digest differs')


def compare_pair(off, on, w):
    need(w.payload_pair_equal(on, off), 'pair object/geometry/reduced census differs')
    for name in DIGESTS + ('orders', 'generator', 'ledger', 'tower_work'):
        need(off[name] == on[name], 'pair differs: ' + name)
    for name in set(off['catalogue']) - CATALOGUE_CHANGE:
        need(off['catalogue'][name] == on['catalogue'][name], 'pair catalogue differs: ' + name)
    batch_keys = w.LANES_COUNTS + ('rectangles', 'survivors', 'deferred', 'judged_edges', 'rebuilt_covers')
    for name in batch_keys:
        need(off['q34_batch'][name] == on['q34_batch'][name], 'pair batch work differs: ' + name)
    need(all(off['catalogue'][name] == 0 for name in PAYLOAD_COUNTS), 'OFF imported payload')
    cat = on['catalogue']
    need(cat['payload_keys'] > 0 and cat['payload_ids'] > 0, 'vacuous ON payload path')
    need(cat['payload_keys'] + cat['payload_fallback_keys'] == cat['by_qmin'][1] and
         cat['payload_keys'] <= cat['regular_supports'][1] and cat['payload_ids'] <= 3 * cat['payload_keys'],
         'payload coverage/cardinality')


def metrics(value):
    result = {'time.' + key: val for key, val in value['times_ms'].items()}
    result.update({'batch.' + key: value['q34_batch'][key] for key in
                   ('lanes_ms', 'lanes_task_ms', 'lanes_convert_ms', 'tail_ms')})
    for section in ('generator', 'ledger', 'catalogue', 'tower_work'):
        result.update({section + '.' + key: val for key, val in value[section].items() if type(val) is int})
    result['order.nodes'] = sum(row['nodes'] for row in value['orders'])
    result['order.parents'] = sum(row['parents'] for row in value['orders'])
    result['order.contributions'] = sum(row['contributions'] for row in value['orders'])
    return result


def summary(pairs):
    growth = []
    for family in FAMILIES:
        rows = sorted((row for row in pairs if row['family'] == family), key=lambda row: row['n'])
        for small, large in zip(rows, rows[1:]):
            for mode in ('off', 'on'):
                a, b = small[mode], large[mode]
                ratios = {key: b[key] / a[key] for key in a if a[key] > 0}
                growth.append(dict(family=family, mode=mode, n=[small['n'], large['n']], ratios=ratios,
                                   exponent={key: math.log2(val) for key, val in ratios.items() if val > 0},
                                   at_least_quadratic=[key for key, val in ratios.items() if val >= 4]))
    return dict(schema=SCHEMA, pairs=pairs, growth=growth,
                scope='FULL K1..5 CPU host-twin; synthetic u16 recipes encoded u32le; no GPU or universal complexity claim',
                timing='one paired observation per case; ordering alternated; shared local host, not a stable speedup estimate',
                clock='chain_total excludes input read, synthetic export, digest checks and segmentation; external wall is separate',
                pinned='host lease pool only; not a measurement of CUDA pinned transfers',
                growth_claim='ratios are measured diagnostics on the three synthetic regimes, not a proof of global subquadratic work')


class Capture:
    def __init__(self, out, manifest):
        self.out, self.manifest, self.child = out, manifest, None

    def interrupted(self, signum, _frame):
        self.manifest['interrupted_signal'] = signum
        raise KeyboardInterrupt('received signal ' + str(signum))

    def command(self, label, argv, role):
        entry = dict(label=label, role=role, argv=argv, cwd=str(ROOT), started=utc(), state='running',
                     stdout=label + '.stdout', stderr=label + '.stderr')
        self.manifest['commands'].append(entry)
        save(self.out / 'manifest.json', self.manifest)
        start = time.monotonic()
        try:
            with (self.out / entry['stdout']).open('xb') as stdout, (self.out / entry['stderr']).open('xb') as stderr:
                self.child = subprocess.Popen(argv, cwd=ROOT, stdout=stdout, stderr=stderr, start_new_session=True)
                entry['pid'] = self.child.pid
                save(self.out / 'manifest.json', self.manifest)
                entry['exit_code'] = self.child.wait()
                entry['state'] = 'finished'
        except BaseException:
            if self.child is not None and self.child.poll() is None:
                os.killpg(self.child.pid, signal.SIGTERM)
                try:
                    self.child.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(self.child.pid, signal.SIGKILL)
                    self.child.wait()
            entry['state'] = 'interrupted'
            if self.child is not None:
                entry['exit_code'] = self.child.returncode
            raise
        finally:
            entry['elapsed_seconds'] = time.monotonic() - start
            entry['finished'] = utc()
            for name in ('stdout', 'stderr'):
                path = self.out / entry[name]
                if path.is_file():
                    entry[name + '_sha256'] = sha(path)
            self.child = None
            save(self.out / 'manifest.json', self.manifest)
        need(entry['exit_code'] == 0, label + ' failed; raw stdout/stderr retained')
        return entry


def pair_record(data, results, labels):
    off, on = results['off'], results['on']
    return dict(family=data['family'], n=data['n'], input_sha256=data['sha256'], commands=labels,
                digests={name: on[name] for name in DIGESTS}, orders=on['orders'],
                off=metrics(off), on=metrics(on),
                on_over_off={key: metrics(on)[key] / val for key, val in metrics(off).items() if val > 0})


def mutations(off, on, w):
    killed = []
    for name in ('tower_digest', 'payload_ids', 'geometric_work'):
        bad = copy.deepcopy(on)
        if name == 'tower_digest':
            bad[name] = '0' * 16 if on[name] != '0' * 16 else '1' * 16
        elif name == 'payload_ids':
            bad['catalogue']['payload_ids'] = 3 * bad['catalogue']['payload_keys'] + 1
        else:
            key = next(iter(bad['ledger']))
            bad['ledger'][key] += 1
        try:
            compare_pair(off, bad, w)
        except ValueError:
            killed.append(name)
        else:
            raise ValueError('reader accepted mutation ' + name)
    return killed


def execute(args, w):
    out, work = Path(args.output).resolve(), Path(args.work).resolve()
    need(not out.exists() and not out.is_symlink() and not work.exists() and not work.is_symlink(),
         'receipt and input directories must both be fresh')
    need(out != work and out not in work.parents and work not in out.parents, 'receipt/input directories overlap')
    out.mkdir(parents=True)
    work.mkdir(parents=True)
    probe, exporter = Path(args.probe).resolve(), Path(args.exporter).resolve()
    manifest = dict(schema=SCHEMA, state='running', started=utc(), root=str(ROOT), probe=str(probe),
                    exporter=str(exporter), work=str(work), seed=args.seed, inputs=[], commands=[], pairs=[],
                    pins_before=pins(probe, exporter), head=subprocess.check_output(
                        ['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                    source_to_binary='build provenance belongs to parent qualification; this capture pins both independently',
                    scope='CPU host twin, FULL K5 s8 W4 static4; no GCP; no data subsampling')
    capture = Capture(out, manifest)
    old_handlers = {sig: signal.signal(sig, capture.interrupted) for sig in (signal.SIGINT, signal.SIGTERM)}
    save(out / 'manifest.json', manifest)
    try:
        recipes = [(family, n) for family in FAMILIES for n in SIZES]
        if args.lidar:
            recipes.append((None, 0))
        for index, (family, n) in enumerate(recipes):
            label = f'{family}_{n}' if family else 'lidar_00_nonground_full'
            if family:
                path = work / (label + '.u32le')
                export = capture.command(label + '_export', [str(exporter), str(n), family,
                                         str(args.seed), str(path)], 'export')
                exported = strict_json(out / export['stdout'], w)
                need(set(exported) == {'sites', 'family', 'seed', 'fixture_hash', 'duplicate_rejections',
                     'format', 'recipe_domain'} and exported['sites'] == n and exported['family'] == family and
                     exported['seed'] == args.seed and exported['format'] == 'u32le' and exported['recipe_domain'] == 'u16'
                     and re.fullmatch('[0-9a-f]{16}', exported['fixture_hash']) and
                     w._count(exported['duplicate_rejections']), 'export identity')
                data = input_identity(path, 'synthetic_u16_recipe', family)
                need(data['n'] == n, 'export site count')
                data['export'] = exported
                data['export_command'] = export['label']
            else:
                data = input_identity(args.lidar, '1mm')
                need(data['sha256'] == w.INPUTS['00']['sha256'], 'optional input is not entire pinned no-ground frame 00')
            manifest['inputs'].append(data)
            results, labels = {}, {}
            # Alternate pair order across cases; one repetition only.
            for mode in (('off', 'on') if index % 2 == 0 else ('on', 'off')):
                enabled = mode == 'on'
                print(f'{utc()} start {label} {mode}', flush=True)
                command = capture.command(label + '_' + mode, probe_command(probe, data, w, enabled), mode)
                value = strict_json(out / command['stdout'], w)
                validate_probe(value, data, enabled, command['elapsed_seconds'], w)
                results[mode], labels[mode] = value, command['label']
                print(f'{utc()} complete {label} {mode} chain_ms={value["times_ms"]["chain_total"]}', flush=True)
            compare_pair(results['off'], results['on'], w)
            if not manifest['pairs']:
                manifest['reader_mutants_killed'] = mutations(results['off'], results['on'], w)
            manifest['pairs'].append(pair_record(data, results, labels))
            save(out / 'summary.json', summary(manifest['pairs']))
            save(out / 'manifest.json', manifest)
        manifest['state'] = 'complete'
    except BaseException as error:
        manifest['state'] = 'failed'
        manifest['error'] = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        for sig, handler in old_handlers.items():
            signal.signal(sig, handler)
        manifest['finished'] = utc()
        manifest['pins_after'] = pins(probe, exporter)
        manifest['pins_unchanged'] = manifest['pins_before'] == manifest['pins_after']
        if not manifest['pins_unchanged']:
            manifest['state'] = 'failed'
            manifest['closure_error'] = 'source/binary/build recipe changed during capture'
        save(out / 'manifest.json', manifest)
    need(manifest['state'] == 'complete', manifest.get('closure_error', 'capture incomplete'))
    return readback(out, w)


def readback(out, w):
    out = Path(out).resolve()
    manifest = strict_json(out / 'manifest.json', w)
    need(manifest['schema'] == SCHEMA and manifest['state'] == 'complete', 'capture not complete')
    need(manifest['pins_before'] == manifest['pins_after'] == pins(manifest['probe'], manifest['exporter']) and
         manifest['pins_unchanged'] is True, 'LIVE source/binary/build recipe hash drift')
    commands = {row['label']: row for row in manifest['commands']}
    need(len(commands) == len(manifest['commands']), 'duplicate command label')
    for row in commands.values():
        need(row['state'] == 'finished' and row['exit_code'] == 0 and row['cwd'] == str(ROOT), 'command incomplete')
        for name in ('stdout', 'stderr'):
            need(Path(row[name]).name == row[name] and sha(out / row[name]) == row[name + '_sha256'], 'raw log hash drift')
    pairs = []
    need(len(manifest['inputs']) == len(manifest['pairs']) and len(manifest['pairs']) in (9, 10), 'missing capture pairs')
    need([(row['family'], row['n']) for row in manifest['inputs'][:9]] ==
         [(family, n) for family in FAMILIES for n in SIZES], 'synthetic matrix differs')
    for data, old in zip(manifest['inputs'], manifest['pairs']):
        identity = input_identity(data['path'], data['grid'], data['family'])
        need(all(data[key] == val for key, val in identity.items()), 'LIVE input changed')
        if data['family']:
            command = commands[data['export_command']]
            need(command['role'] == 'export' and command['argv'] == [manifest['exporter'], str(data['n']),
                 data['family'], str(manifest['seed']), data['path']] and
                 strict_json(out / command['stdout'], w) == data['export'], 'export command/evidence mismatch')
        results = {}
        for mode, label in old['commands'].items():
            command = commands[label]
            need(mode in ('on', 'off') and command['role'] == mode and
                 command['argv'] == probe_command(manifest['probe'], data, w, mode == 'on'), 'probe command binding')
            value = strict_json(out / command['stdout'], w)
            validate_probe(value, data, mode == 'on', command['elapsed_seconds'], w)
            results[mode] = value
        need(set(results) == {'on', 'off'}, 'unpaired result')
        compare_pair(results['off'], results['on'], w)
        pairs.append(pair_record(data, results, old['commands']))
        if len(pairs) == 1:
            need(mutations(results['off'], results['on'], w) == manifest['reader_mutants_killed'], 'reader mutation evidence')
    need(pairs == manifest['pairs'] and summary(pairs) == strict_json(out / 'summary.json', w), 'summary differs from raw evidence')
    need(len(commands) == 9 + 2 * len(pairs), 'unaccounted commands')
    print(json.dumps(dict(status='PASS', schema=SCHEMA, pairs=len(pairs), full_towers=2 * len(pairs),
                         reader_mutants_killed=manifest['reader_mutants_killed'],
                         gcp_used=False, gpu_qualified=False, universal_subquadratic_claim=False), sort_keys=True))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe')
    parser.add_argument('--exporter')
    parser.add_argument('--work')
    parser.add_argument('--output')
    parser.add_argument('--seed', type=int, default=3)
    parser.add_argument('--lidar', help='optional entire pinned no-ground frame 08/000000, run after nine synthetic pairs')
    parser.add_argument('--readback', help='LIVE readback of a complete capture; no probe runs')
    args = parser.parse_args()
    w = worker()
    if args.readback:
        return readback(args.readback, w)
    need(all((args.probe, args.exporter, args.work, args.output)), 'provide --probe --exporter --work --output')
    need(0 <= args.seed < 1 << 64, 'seed outside u64')
    return execute(args, w)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (Exception, KeyboardInterrupt) as error:
        print('payload_scaling: ' + type(error).__name__ + ': ' + str(error), file=sys.stderr)
        sys.exit(1)
