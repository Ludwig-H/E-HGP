#!/usr/bin/env python3
"""Read-only one-parent package reader; extraction is explicit and create-only."""
import argparse
import hashlib
import importlib.util
import json
import posixpath
import re
import shlex
import sys
import types
from pathlib import Path, PurePosixPath

sys.dont_write_bytecode = True
FORMAT = 'fused_history_streaming_one_parent_v1'
PREFIX = 'build/v7_fused_history_streaming_20260911/'
RECORD_SHAS = {
    'ac9b7ef680804e8e619a4a2e4e4ea9f80f08b96bfda49fbc1bc29bf473b8f971',
}
FUSED_RECORD_SHAS = {
    '94073e59781bbfa22247a991ee923e6ab76e3395bf95e108612857c5f1d31bd9',
}
PARENT = {
    'directory': '../rank_guard_streaming_20260911',
    'manifest': 'MANIFEST.json',
    'sha256': 'e97e07ee140030624bf6cdafb80cadee2787450be399ef550fc0fdf23fa780cb',
    'reader_sha256': 'e4394c2a496e160b1d22e5d96f3243369a5e782bfa3e93461163a3e9ad09ca91',
}


def need(value, reason):
    if not value:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    need(isinstance(name, str) and name, 'empty path')
    path = PurePosixPath(name)
    need(not path.is_absolute() and str(path) == name and '\\' not in name and
         all(part not in ('.', '..') for part in path.parts), f'unsafe path:{name}')
    return path


def plain(path):
    need(path.is_file() and not path.is_symlink(), f'not a regular file:{path}')
    return path.read_bytes()


def pin(data):
    return {'sha256': sha(data), 'size': len(data)}


def source_roles(sources, captured):
    return {name: ('approved_versioned_recorder' if name in (PREFIX + 'record.py', PREFIX + 'record_fused.py') else
                   'captured_source' if name in captured else
                   'documentation' if name.endswith('.md') else 'packaging_tool') for name in sources}


def load_parent(directory):
    directory = Path(directory).resolve()
    need(sha(plain(directory / PARENT['manifest'])) == PARENT['sha256'], 'parent manifest pin')
    reader_path = directory / 'verify.py'
    need(sha(plain(reader_path)) == PARENT['reader_sha256'], 'parent reader pin')
    spec = importlib.util.spec_from_file_location('streaming_parent_reader', reader_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    reader = module.Reader(directory)
    reader.verify()  # Checks its own transitive atlas/rank dependencies, without copying it.
    return reader


class Reader:
    def __init__(self, package, parent=None):
        self.package = Path(package).resolve()
        self.manifest = json.loads(plain(self.package / 'MANIFEST.json'))
        need(self.manifest.get('format') == FORMAT and self.manifest.get('parent') == PARENT,
             'package format and unique parent')
        self.parent = load_parent(parent if parent is not None else self.package / PARENT['directory'])

    def bytes(self, logical):
        safe(logical)
        row = self.manifest['files'][logical]
        need(set(row) == {'provider', 'path', 'size', 'sha256'}, f'file schema:{logical}')
        safe(row['path'])
        need(type(row['size']) is int and row['size'] >= 0 and isinstance(row['sha256'], str) and
             len(row['sha256']) == 64 and all(c in '0123456789abcdef' for c in row['sha256']), 'file pin schema')
        if row['provider'] == 'parent':
            parent_row = self.parent.manifest['files'][row['path']]
            need(parent_row['size'] == row['size'] and parent_row['sha256'] == row['sha256'], 'parent logical pin')
            data = self.parent.bytes(row['path'])
        else:
            need(row['provider'] == 'object' and row['path'] == 'objects/' + row['sha256'], 'object binding')
            data = plain(self.package / row['path'])
        need(pin(data) == {'sha256': row['sha256'], 'size': row['size']}, f'bytes:{logical}')
        need(not data.startswith((b'\x7fELF', b'!<arch>\n', b'MZ')), f'binary forbidden:{logical}')
        return data

    def json(self, logical):
        return json.loads(self.bytes(logical))

    def verify(self):
        m = self.manifest
        need(set(m) == {'format', 'parent', 'package_files', 'sources', 'source_roles', 'runs', 'files', 'omitted_binaries'},
             'manifest schema')
        need(set(m['package_files']) == {'README.md', 'verify.py'}, 'package source set')
        expected = {'MANIFEST.json', *m['package_files']}
        for name, digest in m['package_files'].items():
            need(sha(plain(self.package / name)) == digest, f'package pin:{name}')
        need(isinstance(m['sources'], list) and m['sources'] == sorted(set(m['sources'])) and m['sources'], 'source set')
        need(isinstance(m['runs'], dict) and m['runs'], 'nonempty explicit run set')
        captured = {'build/' + name for directory in m['runs'] for name in self.json(directory + '/sources_before.json')}
        roles = source_roles(m['sources'], captured)
        need(m['source_roles'] == roles, 'source roles bound to capture closure, never inherited qualification')
        for filename, accepted in (('record.py', RECORD_SHAS), ('record_fused.py', FUSED_RECORD_SHAS)):
            if PREFIX + filename in m['sources']:
                need(sha(self.bytes(PREFIX + filename)) in accepted, 'supplied approved recorder revision')
        need(all(role != 'packaging_tool' or name == PREFIX + 'package_receipts.py'
                 for name, role in roles.items()), 'no unclassified uncompiled source')
        logical_expected = set(m['sources'])
        for directory, run in m['runs'].items():
            safe(directory)
            need(set(run) == {'files', 'omitted_binaries'} and run['files'] == sorted(set(run['files'])) and
                 run['omitted_binaries'] == sorted(set(run['omitted_binaries'])), 'run inventory schema')
            for logical in run['files'] + run['omitted_binaries']:
                need(logical.startswith(directory + '/'), 'run membership')
            need(not logical_expected.intersection(run['files']), 'overlapping selections')
            logical_expected.update(run['files'])
        need(logical_expected == set(m['files']), 'exact logical source/run inventory')
        omitted_expected = {name for run in m['runs'].values() for name in run['omitted_binaries']}
        need(omitted_expected == set(m['omitted_binaries']) and not omitted_expected.intersection(m['files']),
             'exact binary omission inventory')
        borrowed = 0
        for logical, row in m['files'].items():
            self.bytes(logical)
            if row['provider'] == 'object':
                expected.add(row['path'])
            else:
                borrowed += 1
        need(borrowed > 0, 'parent source non-vacuity')
        for logical, row in m['omitted_binaries'].items():
            safe(logical)
            need(set(row) == {'sha256', 'size', 'reason'} and row['reason'] in ('ELF', 'archive', 'object', 'executable') and
                 type(row['size']) is int and row['size'] > 0 and len(row['sha256']) == 64 and
                 all(c in '0123456789abcdef' for c in row['sha256']), 'binary omission attestation')
        actual = set()
        for path in self.package.rglob('*'):
            need(not path.is_symlink(), f'physical symlink:{path}')
            if path.is_file():
                actual.add(path.relative_to(self.package).as_posix())
        need(actual == expected, 'exact physical inventory; no unreferenced object')
        return {'status': 'passed', 'logical_files': len(m['files']),
                'borrowed_files': borrowed, 'local_objects': len(expected) - 3,
                'runs': len(m['runs']), 'qualification': qualification(self)}

    def extract(self, destination):
        destination = Path(destination)
        need(not destination.exists() and not destination.is_symlink(), 'fresh extraction required')
        destination.mkdir(parents=True, exist_ok=False)
        for logical in self.manifest['files']:
            target = destination / safe(logical)
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write(self.bytes(logical))


def captured_checker(reader, origin, filename):
    logical = origin + '/source_snapshot/' + PREFIX.removeprefix('build/') + filename
    source = reader.bytes(logical)
    accepted = {'record.py': RECORD_SHAS, 'record_fused.py': FUSED_RECORD_SHAS}
    need(filename in accepted and sha(source) in accepted[filename], 'approved captured checker pin:' + logical)
    checker = types.ModuleType('fused_history_captured_checks')
    checker.__file__ = str(reader.package / logical)
    exec(compile(source, checker.__file__, 'exec'), checker.__dict__)
    return checker


def source_binding(reader, snapshot, name, value):
    need(pin(reader.bytes(snapshot + name)) == value, 'captured source pin')
    current = pin(reader.bytes('build/' + name))
    recorder_versions = {PREFIX.removeprefix('build/') + 'record.py': RECORD_SHAS,
                         PREFIX.removeprefix('build/') + 'record_fused.py': FUSED_RECORD_SHAS}
    if name in recorder_versions:
        # Only this recorder has approved historical revisions. Each run is
        # checked by its own bytes, not retroactively by the latest checker.
        need(value['sha256'] in recorder_versions[name] and current['sha256'] in recorder_versions[name],
             'approved recorder revisions')
    else:
        need(current == value, 'snapshot and supplied rebuild source binding')


def parent_bench_key(parsed, arm):
    route = parsed[arm]
    return (parsed['input_digest'], parsed['n'], parsed['s'], parsed['requested_K'],
            parsed['window'], parsed['digest_convention'], arm,
            parsed['generation_threads'], parsed['history_query_threads'],
            route['geometry_workers'], route['reducer'], route['geometry_backend'])


def parent_bench_signature(parsed, arm):
    route = parsed[arm]
    fields = ('payload_digest', 'orders', 'nodes', 'parent_refs', 'contributions',
              'vertical_refs', 'occurrences', 'native_births', 'marks', 'retained_certificate_edges')
    order_fields = ('K', 'occurrences', 'windows', 'unique', 'initial_seed_hits', 'Q', 'H', 'T')
    per_order = tuple({key: order[key] for key in order_fields} for order in route['per_order'])
    signature = tuple(route[key] for key in fields) + (route['geometry_work'], route['history_work'], per_order)
    if arm == 'streaming':
        geometry = tuple(order['geometry_work'] for order in route['per_order'])
        birth = tuple({key: value for key, value in order['birth_work'].items()
                       if not key.endswith('_bytes')} for order in route['per_order'])
        team = route['parallel_geometry']
        rank = tuple(order['rank_guard_work'] for order in route['per_order'])
        signature += (geometry, birth, rank, team['geometry_batches'], team['geometry_tasks'])
    return signature


def parent_baselines(parent):
    # Parent.verify() has already checked these captures by their OWN sources.
    # Do not import their successes as qualification of the new history route.
    gates, benches = {}, {}
    for directory in parent.manifest['runs']:
        receipt = parent.json(directory + '/receipt.json')
        if receipt['status'] != 'passed':
            continue
        if receipt['kind'] == 'gate':
            for mode, value in parent.json(directory + '/results.json').items():
                if mode in ('selftest', 'high', 'uniform32'):
                    projected = {key: part for key, part in value.items()
                                 if key not in ('checks', 'history_backend', 'fused_history_memory_scope', 'fused_history_work')}
                    need(mode not in gates or gates[mode] == projected, 'consistent parent gate baselines')
                    gates[mode] = projected
        elif receipt['kind'] == 'bench-run':
            value = parent.json(directory + '/results.json').get('bench')
            if value is None:
                continue
            for arm in ('reference', 'streaming'):
                if arm not in value:
                    continue
                key, signature = parent_bench_key(value, arm), parent_bench_signature(value, arm)
                need(key not in benches or benches[key] == signature, 'consistent parent bench baselines')
                benches[key] = signature
    return gates, benches


def qualification(reader):
    def historical(path):
        marker = '/' + PREFIX
        need(isinstance(path, str) and path.count(marker) == 1, 'historical capture path')
        return PREFIX + path.split(marker, 1)[1]

    passed, failed, gate_outputs, fused_gate_outputs, bench_runs = [], [], [], [], []
    focused_capture_modes = []
    bench_groups, inter_s_groups, parallel_groups = {}, {}, {}
    parent_gates, parent_benches = parent_baselines(reader.parent)
    parent_gate_checks, parent_bench_checks = [], []
    for directory in sorted(reader.manifest['runs']):
        receipt = reader.json(directory + '/receipt.json')
        commands = reader.json(directory + '/commands.json')
        results = reader.json(directory + '/results.json')
        need(receipt['status'] in ('passed', 'failed') and receipt['commands'] == len(commands) and
             receipt['kind'] in ('gate', 'fused-gate', 'bench', 'bench-run') and receipt['public_status'] == 'not_claimed' and
             receipt['device_executed'] is False and receipt['gcp_used'] is False and receipt['performance_claim'] is False,
             'receipt closed scope')
        names = [row['name'] for row in commands]
        need(len(names) == len(set(names)), 'unique command names')
        for row in commands:
            name = row['name']
            need(reader.json(directory + '/' + name + '.command.json') == row, 'command stream binding')
            launch = reader.json(directory + '/' + name + '.launch.json')
            need(launch == {key: (None if key == 'exit_code' else row[key]) for key in
                            ('name', 'argv', 'cwd', 'expected_exit', 'exit_code')}, 'launch command binding')
            for channel in ('stdout', 'stderr'):
                need(pin(reader.bytes(directory + '/' + name + '.' + channel)) == row[channel], 'raw stream pin')
        if receipt['status'] == 'failed':
            need(receipt['error'], 'failed run retains reason')
            failed.append(directory)
            continue  # Preserve failures; never turn partial outputs into qualification.
        need(receipt['error'] is None and receipt['locale'] == 'C' and
             receipt['closure'] == dict.fromkeys(('sources', 'compiler', 'dependencies', 'binary'), True), 'passed closure')
        need(all(row['exit_code'] == row['expected_exit'] for row in commands), 'exact command exits')
        before = reader.json(directory + '/sources_before.json')
        need(before and all(reader.json(directory + '/' + name + '.json') == before for name in
                            ('sources_after', 'sources_copied_before', 'sources_copied_after')), 'full source closure')
        origin = directory
        if receipt['kind'] == 'bench-run':
            binding = receipt['source_build_receipt']
            logical = historical(binding['path'])
            origin = logical.removesuffix('/receipt.json')
            need(origin in reader.manifest['runs'] and logical == origin + '/receipt.json' and
                 pin(reader.bytes(logical)) == {key: binding[key] for key in ('sha256', 'size')}, 'bench build receipt binding')
            build = reader.json(logical)
            need(build['status'] == 'passed' and build['kind'] == 'bench', 'bench compiled parent')
        else:
            need(receipt['source_build_receipt'] is None, 'build has no source build parent')
        focused = receipt['kind'] == 'fused-gate'
        checker_name = 'record_fused.py' if focused else 'record.py'
        checker = captured_checker(reader, origin, checker_name)
        if focused:
            need(receipt['scope'] == checker.SCOPE and receipt['geometry_qualified'] is False,
                 'focused synthetic histories are not FULL or geometry qualification')
        snapshot_prefix = origin + '/source_snapshot/'
        actual_sources = {name.removeprefix(snapshot_prefix) for name in reader.manifest['files'] if name.startswith(snapshot_prefix)}
        need(actual_sources == set(before), 'snapshot exact minimal source inventory')
        for name, value in before.items():
            source_binding(reader, snapshot_prefix, name, value)
        for part in ('compiler', 'dependencies', 'binary'):
            values = reader.json(directory + '/' + part + '_before.json')
            need(values and reader.json(directory + '/' + part + '_after.json') == values, part + ' closure')
        dependencies = reader.json(directory + '/dependencies_before.json')
        used = set()
        for path, value in dependencies.items():
            need(value['kind'] in ('source_snapshot', 'boost_system_header', 'system_header'), 'dependency kind')
            if value['kind'] == 'source_snapshot':
                logical = historical(path)
                need(logical.startswith(snapshot_prefix) and pin(reader.bytes(logical)) ==
                     {key: value[key] for key in ('sha256', 'size')}, 'consumed header binding')
                used.add(logical.removeprefix(snapshot_prefix))
        need(used == set(before) - {PREFIX.removeprefix('build/') + checker_name}, 'compiled minimal local closure')
        binary = reader.json(directory + '/binary_before.json')
        omitted = reader.manifest['omitted_binaries'][origin + '/' + ('bench' if receipt['kind'] == 'bench-run' else receipt['kind'])]
        need(binary == {key: omitted[key] for key in ('sha256', 'size')} and omitted['reason'] == 'ELF', 'compiled ELF pin')
        if receipt['kind'] != 'bench-run':
            expected = ['compiler', 'local_dependencies', 'dependencies_before', 'compile']
            if receipt['kind'] in ('gate', 'fused-gate'):
                expected += [*checker.FIXTURES, *checker.CAUSES, 'unknown', 'missing']
            expected += ['dependencies_after']
            need(names == expected, 'build/gate command matrix')
            flags = next(row['argv'] for row in commands if row['name'] == 'compile')
            need(all(flag in flags for flag in ('-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread')), 'strict flags')
            compiler_pins = reader.json(directory + '/compiler_before.json')
            need(len(compiler_pins) == 1 and flags[0] in compiler_pins and
                 historical(flags[-1]) == origin + '/' + receipt['kind'] and flags[-2] == '-o' and
                 historical(flags[-3]) == snapshot_prefix + PREFIX.removeprefix('build/') +
                 (checker.SOURCE_NAME if focused else receipt['kind'] + '.cpp'),
                 'compiler/TU/output binding')
            for name in ('dependencies_before', 'compile', 'dependencies_after'):
                text = reader.bytes(directory + '/' + name + '.d').decode().replace(chr(92) + chr(10), ' ')
                target, colon, body = text.partition(':')
                need(colon and target.strip() == checker.TARGET and
                     sorted({posixpath.normpath(path) for path in shlex.split(body)}) == list(dependencies),
                     'actual compiler dependency lists')
            for row in commands:
                if row['name'] in ('compiler', 'local_dependencies', 'dependencies_before', 'compile', 'dependencies_after'):
                    need(row['exit_code'] == 0 and not reader.bytes(directory + '/' + row['name'] + '.stderr') and
                         (bool(reader.bytes(directory + '/' + row['name'] + '.stdout')) if row['name'] == 'compiler' else
                          not reader.bytes(directory + '/' + row['name'] + '.stdout')), 'no tool diagnostics')
            if receipt['sanitizer']:
                need(receipt['kind'] in ('gate', 'fused-gate') and receipt['sanitizer_environment'] == checker.SAN_ENV and
                     all(flag in flags for flag in ('-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie')),
                     'SAN and LSan flags')
            else:
                need(receipt['sanitizer_environment'] == {} and '-O2' in flags, 'O2 flags')
        if receipt['kind'] in ('gate', 'fused-gate'):
            parsed = {}
            for row in commands:
                name = row['name']
                if name in (*checker.FIXTURES, *checker.CAUSES, 'unknown', 'missing'):
                    need(row['expected_exit'] == (4 if name in checker.CAUSES else 2 if name in ('unknown', 'missing') else 0),
                         'gate expected exit')
                    need(historical(row['argv'][0]) == origin + '/' + receipt['kind'] and
                         row['argv'][1:] == ([] if name == 'missing' else ['--' + name]), 'gate CLI and ELF binding')
                    parsed[name] = checker.check_gate(name, reader.bytes(directory + '/' + name + '.stdout'),
                                                      reader.bytes(directory + '/' + name + '.stderr'))
            need(parsed == results and receipt['benchmark_executed'] is False, 'gate parsed results')
            if focused:
                fused_gate_outputs.append(parsed)
                focused_capture_modes.append({'capture': directory,
                                              'mode': 'SAN_ASan_UBSan_LSan' if receipt['sanitizer'] else 'O2',
                                              'authority': 'receipt_environment_and_compile_flags_not_directory_name'})
            else:
                gate_outputs.append(parsed)
                for mode in ('selftest', 'high', 'uniform32'):
                    projected = {key: value for key, value in parsed[mode].items()
                                 if key not in ('checks', 'history_backend', 'fused_history_memory_scope', 'fused_history_work')}
                    need(mode in parent_gates and projected == parent_gates[mode],
                         'parent gate FULL and paid geometry equality')
                    parent_gate_checks.append({'capture': directory, 'mode': mode})
        elif receipt['kind'] == 'bench-run':
            need(names == ['bench'] and receipt['sanitizer'] is False and receipt['sanitizer_environment'] == {}, 'single bench execution')
            argv = commands[0]['argv']
            need(argv[1:3] == ['-v', '-o'] and historical(argv[3]) == directory + '/rss.txt' and
                 historical(argv[4]) == origin + '/bench', 'external time and exact ELF command')
            parsed = checker.check_bench(reader.bytes(directory + '/bench.stdout'), reader.bytes(directory + '/bench.stderr'),
                                         argv[5:], commands[0]['expected_exit'])
            rss = reader.bytes(directory + '/rss.txt')
            need(reader.json(directory + '/rss_pin.json') == pin(rss), 'external RSS raw pin')
            match = re.search(rb'Maximum resident set size \(kbytes\): ([0-9]+)', rss)
            need(match is not None and results == {'bench': parsed, 'max_rss_kib': int(match[1])} and
                 receipt['benchmark_executed'] is bool(parsed), 'bench result and RSS binding')
            bench_runs.append(directory)
            if parsed is not None:
                key = tuple(parsed[name] for name in ('input_digest', 'n', 's', 'requested_K', 'digest_convention'))
                need(re.fullmatch('[0-9a-f]{64}', key[0]) and
                     key[-1] == 'mhgp7-graph-FULL-dense-raw-level-bank-v1', 'paired input/digest convention')
                group = bench_groups.setdefault(key, {'outputs': [], 'processes': set(), 'direct_comparisons': 0})
                group['processes'].add(directory)
                group['direct_comparisons'] += parsed['physical_comparison_executed']
                fields = ('payload_digest', 'nodes', 'parent_refs', 'contributions', 'vertical_refs', 'occurrences',
                          'native_births', 'marks', 'retained_certificate_edges')
                for arm in ('reference', 'streaming'):
                    if arm not in parsed:
                        continue
                    route = parsed[arm]
                    parent_key = parent_bench_key(parsed, arm)
                    if parent_key in parent_benches:
                        need(parent_bench_signature(parsed, arm) == parent_benches[parent_key],
                             'parent bench FULL and paid geometry equality')
                        parent_bench_checks.append({'capture': directory, 'arm': arm,
                            'n': parsed['n'], 's': parsed['s'], 'requested_K': parsed['requested_K'],
                            'window': parsed['window'], 'geometry_workers': route['geometry_workers']})
                    rows = route['per_order']
                    need([row['K'] for row in rows] == list(range(1, parsed['effective_K'] + 1)), 'paired order domain')
                    counts = tuple(row['occurrences'] for row in rows)
                    need(all(type(value) is int and value >= 0 for value in counts), 'paired occurrence integers')
                    signature = tuple(route[field] for field in fields) + (counts,)
                    need(not group['outputs'] or signature == group['outputs'][0], 'paired process FULL digest/payload/R')
                    group['outputs'].append(signature)
                    # Compare s only: same input/K/window/reducer and thread controls.
                    # Candidate counts, timings and sampled capacities are deliberately excluded.
                    controls = tuple(parsed[name] for name in ('generation_threads', 'geometry_threads', 'history_query_threads'))
                    s_key = (parsed['input_digest'], parsed['n'], parsed['requested_K'], parsed['window'],
                             route['reducer'], parsed['digest_convention'], controls)
                    across = inter_s_groups.setdefault(s_key, {'s': set(), 'outputs': []})
                    s_signature = signature + (route['geometry_work'],)
                    need(not across['outputs'] or s_signature == across['outputs'][0],
                         'inter-s FULL digest/payload/R/geometric work')
                    across['s'].add(parsed['s'])
                    across['outputs'].append(s_signature)
                    if arm == 'streaming':
                        # Same algorithm/input/window and upstream/history controls;
                        # only the persistent geometry worker count may vary.
                        # No per-worker distribution, scratch/capacity, RSS or time.
                        p_key = (parsed['input_digest'], parsed['n'], parsed['s'],
                                 parsed['requested_K'], parsed['window'], parsed['digest_convention'],
                                 parsed['generation_threads'], parsed['history_query_threads'],
                                 route['reducer'], route['geometry_backend'])
                        per_order = tuple({field: order[field] for field in (
                            'K', 'occurrences', 'windows', 'unique', 'initial_seed_hits',
                            'Q', 'H', 'T', 'seed_rows', 'pivots', 'native_certificate_edges',
                            'geometry_work', 'rank_guard_work')} for order in rows)
                        birth_work = tuple({field: value for field, value in order['birth_work'].items()
                                            if not field.endswith('_bytes')} for order in rows)
                        fused_work = tuple({field: value for field, value in order['fused_history_work'].items()
                                            if not field.endswith('_bytes')} for order in rows)
                        team = route['parallel_geometry']
                        p_signature = signature + (route['geometry_work'], route['history_work'],
                            per_order, birth_work, fused_work, team['geometry_batches'], team['geometry_tasks'])
                        parallel = parallel_groups.setdefault(p_key, {'workers': set(), 'outputs': []})
                        need(not parallel['outputs'] or p_signature == parallel['outputs'][0],
                             'cross-worker FULL geometry and logical work')
                        parallel['workers'].add(route['geometry_workers'])
                        parallel['outputs'].append(p_signature)
        else:
            need(results == {} and receipt['benchmark_executed'] is False, 'compile-only bench build')
        passed.append(directory)
    if len(gate_outputs) > 1:
        need(all(row == gate_outputs[0] for row in gate_outputs), 'O2/SAN gate exact output agreement')
    if len(fused_gate_outputs) > 1:
        need(all(row == fused_gate_outputs[0] for row in fused_gate_outputs), 'focused O2/SAN exact output agreement')
    return {'status': 'checked', 'passed_captures': passed, 'failed_captures_preserved': failed,
            'gate_captures': len(gate_outputs), 'focused_fused_gate_captures': len(fused_gate_outputs), 'bench_runs': bench_runs,
            'focused_capture_modes': focused_capture_modes,
            'bench_groups': [{'n': key[1], 's': key[2], 'requested_K': key[3],
                              'route_observations': len(group['outputs']), 'processes': len(group['processes']),
                              'direct_linear_comparisons': group['direct_comparisons'],
                              'cross_process_check': 'dense_digest_and_payload_counts_not_direct_linear_comparison'}
                             for key, group in sorted(bench_groups.items())],
            'inter_s_comparisons': [{'n': key[1], 'requested_K': key[2], 'window': key[3], 'reducer': key[4],
                                     's_values': sorted(group['s']), 'route_observations': len(group['outputs']),
                                     'comparison': 'FULL_dense_digest_payload_counts_R_per_K_and_geometry_work',
                                     'latency_optimum_claim': False, 'universal_geometry_claim': False}
                                    for key, group in sorted(inter_s_groups.items()) if len(group['s']) > 1],
            'parallel_geometry_comparisons': [
                {'n': key[1], 's': key[2], 'requested_K': key[3], 'window': key[4],
                 'reducer': key[8], 'geometry_backend': key[9],
                 'geometry_workers': sorted(group['workers']),
                 'route_observations': len(group['outputs']),
                 'comparison': 'FULL_dense_digest_payload_R_geometry_and_logical_reducer_work',
                 'distribution_memory_and_timing_compared': False}
                for key, group in sorted(parallel_groups.items()) if len(group['workers']) > 1],
            'parent_comparisons': {
                'gate_corpus_checks': parent_gate_checks,
                'bench_arm_checks': parent_bench_checks,
                'comparison': 'FULL_and_paid_geometry_same_input_controls_no_memory_or_timing',
                'gate_instrumentation_checks_equal_not_claimed': True,
                'parent_success_not_inherited': True},
            'performance_contract_claim': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--parent', type=Path, help='relocated unique parent, still pinned')
    parser.add_argument('--extract', type=Path)
    args = parser.parse_args()
    reader = Reader(Path(__file__).resolve().parent, args.parent)
    result = reader.verify()
    if args.extract:
        reader.extract(args.extract)
        result['extracted_to'] = str(args.extract.resolve())
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
