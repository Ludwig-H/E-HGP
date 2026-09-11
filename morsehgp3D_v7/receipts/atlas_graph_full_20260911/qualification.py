"""Bounded science/closure checks; no compiler, network or process invocation."""
import hashlib
import json
import posixpath
import re
import shlex

ATLAS = 'build/v7_atlas_graph_20260911/'
MSF = 'build/v7_composable_msf_20260911/'
FULL = 'build/v7_graph_full_20260911/'
SAN = {'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
CAPTURE_ROOT = '/workspaces/E-HGP/'
EXTRACT_CAUSES = {'target': 'gate.direct_terminal', 'slot': 'gate.direct_terminal',
    'pivot': 'gate.pivot_ordinal_zero', 'edge_date': 'gate.edge_identity_date',
    'mark_date': 'gate.mark_identity_admission', 'omit_birth': 'gate.native_births'}
FULL_CAUSES = {'mutant-growth-date': 'graph_gate.single_birth_contribution',
    'mutant-vertical': 'graph_gate.vertical_identity', 'mutant-parent': 'graph_gate.duplicate_native_parent',
    'mutant-node-date': 'graph_gate.parent_time', 'reject-descendant': 'graph_full.descendant_leaf',
    'mutant-mask': 'graph_gate.dated_contributions', 'mutant-native-anchor': 'graph_gate.native_history_bijection'}
FULL_POSITIVE = {'selftest': (96, 366, 18240, 5224, 92440,
        'c370a290b8844bc733a49594ce4e97262a2542b1613a6d22da72b5eb3eb895a5'),
    'high': (12, 120, 33312, 24560, 15502392,
        '4597fce598c71e476108aceea8a0608a6904178ccafe2e68ac9416af5d6f5a0d'),
    'uniform32': (6, 60, 186288, 0, 0,
        '8830f3c03ec6824edd3ed86df78597291008e8e4e25034cb88cc7c866fa0acfd')}
CASE_NAMES = {'selftest': {'E5', 'actual_equal_radius_descent', 'growth_ABCZ', 'growth_ABCZ_doubled_lot',
    'growth_redundant', 'inert_ball', 'inert_ball_doubled_lot', 'line4', 'pair', 'portal_equal',
    'shell7_window', 'singleton', 'square', 'support3', 'support4', 'u16_tetra'},
    'high': {'spatial12', 'shell14'}, 'uniform32': {'uniform32'}}


def need(value, why):
    if not value:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pin(data):
    return {'sha256': sha(data), 'size': len(data)}


def strict_flags(argv, san):
    need(all(flag in argv for flag in ('-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror')), 'strict flags')
    need(('-fsanitize=address,undefined' in argv) is san, 'sanitizer flags')


def omitted_binary(reader, logical, digest, size=None):
    row = reader.manifest['omitted_elf'][logical]
    need(row['sha256'] == digest and (size is None or row['size'] == size), 'executed ELF omission pin')


def execution_paths(commands, binary, modes, name_key='name'):
    for row in commands:
        name = row[name_key]
        if name in modes:
            option = modes[name]
            need(row['argv'] == [binary, *([] if option is None else [option])], 'exact executable/mode argv:' + name)


def dep_paths(data, target):
    head, separator, body = data.decode().replace('\\\n', ' ').partition(':')
    need(separator and head.strip() == target, 'make dependency target')
    return {posixpath.normpath(name) for name in shlex.split(body)}


def progress(data, count, causal=False, mode=None):
    active, starts, ends = None, 0, 0
    identities = []
    for line in data.decode().splitlines():
        start = re.fullmatch(r'start=([A-Za-z0-9_]+/v[01]/s(?:8|10|12))', line)
        end = re.fullmatch(r'complete=([A-Za-z0-9_]+/v[01]/s(?:8|10|12)) nodes=([1-9][0-9]*)', line)
        if start:
            need(active is None, 'overlapping progress')
            active = start[1]; starts += 1
            identities.append(active)
        elif end:
            need(active == end[1], 'mismatched progress')
            active = None; ends += 1
        else:
            raise ValueError('unexpected stderr/compiler/sanitizer diagnostic:' + line)
    need(starts == count and (ends == 0 and active is not None if causal else ends == count and active is None),
         'progress closure')
    if mode in CASE_NAMES:
        expected = {f'{name}/v{variant}/s{s}' for name in CASE_NAMES[mode] for variant in (0, 1) for s in (8, 10, 12)}
        need(len(identities) == len(set(identities)) and set(identities) == expected, 'exact executed fixture matrix')


def parity(reader, first, second, names):
    for name in names:
        for channel in ('stdout', 'stderr'):
            suffix = name + '.' + channel
            need(reader.bytes(first + suffix) == reader.bytes(second + suffix), 'O2/SAN identity:' + suffix)


def extraction(reader):
    source_maps = []
    names = ['compiler', 'compile', 'selftest', *EXTRACT_CAUSES, 'unknown', 'missing']
    for folder, san in [('o2_r1', False), ('san_root_r1', True)]:
        p = ATLAS + folder + '/'
        receipt = reader.json(p + 'receipt.json')
        need(receipt['status'] == 'passed' and receipt['source_stable'] is True and receipt['commands'] == 11 and
             receipt['san'] is san and receipt['gcp_used'] is False and receipt['device_executed'] is False, 'extraction receipt')
        sources = reader.json(p + 'sources_before.json')
        need(sources == reader.json(p + 'sources_after.json'), 'extraction source closure')
        source_maps.append(sources)
        for name, digest in sources.items():
            need(sha(reader.bytes(ATLAS + name)) == digest, 'extraction source pin:' + name)
        commands = reader.json(p + 'commands.json')
        need([r['name'] for r in commands] == names, 'extraction exact commands')
        strict_flags(commands[1]['argv'], san)
        binary = reader.json(p + 'binary.json')
        binary_path = CAPTURE_ROOT + p + 'gate'
        need(binary['path'] == binary_path, 'extraction binary path')
        omitted_binary(reader, p + 'gate', binary['sha256'])
        execution_paths(commands, binary_path, {name: '--' + name.replace('_', '-') for name in names[2:-1]} | {'missing': None})
        compiler = reader.json(p + 'compiler_pin.json')['path']
        opt = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if san else ['-O2', '-DNDEBUG']
        need(commands[1]['argv'] == [compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
            *opt, '-MMD', '-MF', CAPTURE_ROOT + p + 'gate.d', CAPTURE_ROOT + ATLAS + 'gate.cpp', '-o', binary_path],
            'extraction exact compile TU')
        for dep in dep_paths(reader.bytes(p + 'gate.d'), binary_path):
            need(dep.startswith(CAPTURE_ROOT + ATLAS), 'extraction dependency tree')
            name = dep[len(CAPTURE_ROOT + ATLAS):]
            need(name in sources and sha(reader.bytes(ATLAS + name)) == sources[name], 'extraction consumed source')
        for row in commands:
            expected = 4 if row['name'] in EXTRACT_CAUSES else 2 if row['name'] in ('unknown', 'missing') else 0
            need(row['returncode'] == row['expected'] == expected, 'extraction exact exit')
            if san:
                need(row['environment'] == SAN, 'extraction SAN environment')
        need(not reader.bytes(p + 'compile.stderr'), 'extraction compiler diagnostics')
        data = reader.bytes(p + 'selftest.stdout')
        need(sha(data) == '8410fd1e52d638022f1e310c931239336bd5ba1e9afb957f55d4f02e58f9925b', 'extraction exact result')
        result = json.loads(data)
        need(result['checks'] == 401731 and result['fixtures'] == 12 and result['representatives'] == 52307 and
             result['marks'] == result['cells'] == 30422 and result['loops_removed'] == 171 and
             result['retained_edges'] == 35862 and result['device_executed'] is False, 'extraction nonvacuity')
        need(result['meb_calls'] == result['anchor_hits'] + result['intruder_queries'] == 12522 and
             sum(row['H'] for row in result['per_k']) == sum(row['T'] for row in result['per_k']) == 715,
             'extraction work identities')
        for name, cause in EXTRACT_CAUSES.items():
            value = reader.json(p + name + '.stdout')
            need(value['status'] == 'causal_rejection' and value['cause'] == cause and value['checks'] > 0 and
                 value['fault'] == '--' + name.replace('_', '-'), 'extraction exact cause')
    common = set(source_maps[0]) & set(source_maps[1])
    need(all(source_maps[0][name] == source_maps[1][name] for name in common), 'shared source unchanged')
    added = set(source_maps[1]) - set(source_maps[0])
    expected = {'source/morsehgp3D_v7/oracle/local_plateau_oracle.hpp',
        'source/morsehgp3D_v7/tests/census_tower_oracle.hpp', 'source/morsehgp3D_v7/tests/full_ball_tower_gate.cpp'}
    need(source_maps[0] == source_maps[1] and not added and expected <= common,
         'identical extraction pin maps, three T2 sources already present before O2')
    parity(reader, ATLAS + 'o2_r1/', ATLAS + 'san_root_r1/', names[2:])
    return sorted(added)


def msf(reader):
    source_maps = []
    for folder, san in [('o2_r1', False), ('san_root_r1', True)]:
        p = MSF + folder + '/'
        r = reader.json(p + 'receipt.json')
        need(r['status'] == 'passed' and r['error'] is None and r['source_stable'] is True and
             r['sanitizer'] is san and r['sanitizer_environment'] == (SAN if san else {}) and
             r['compiler_and_system_headers_pinned'] is False and r['GCP_used'] is False and
             r['geometry_qualified'] is False, 'MSF receipt scope')
        before = reader.json(p + 'source_before.json')
        source_maps.append(before)
        need(before == reader.json(p + 'source_after.json'), 'MSF source closure')
        for name, digest in before.items():
            need(sha(reader.bytes(p + name)) == digest, 'MSF copied source pin')
        commands = r['commands']
        need([row['name'] for row in commands] == ['compile', 'selftest', 'unknown', 'missing'], 'MSF command set')
        strict_flags(commands[0]['argv'], san)
        binary_path = CAPTURE_ROOT + p + 'gate'
        omitted_binary(reader, p + 'gate', r['binary_sha256'])
        execution_paths(commands, binary_path, {'selftest': '--selftest', 'unknown': '--unknown', 'missing': None})
        opt = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if san else ['-O2']
        need(commands[0]['argv'] == ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', *opt,
            '-MMD', '-MF', CAPTURE_ROOT + p + 'gate.d', CAPTURE_ROOT + p + 'gate.cpp', '-o', binary_path], 'MSF exact compile TU')
        need(sha(reader.bytes(p + 'gate.d')) == r['dependency_sha256'], 'MSF dependency file pin')
        for dep in dep_paths(reader.bytes(p + 'gate.d'), binary_path):
            need(dep.startswith(CAPTURE_ROOT + p), 'MSF consumed copied source tree')
            name = dep[len(CAPTURE_ROOT + p):]
            need(name in before and sha(reader.bytes(p + name)) == before[name], 'MSF actual dependency')
        for row in commands:
            expected = 2 if row['name'] in ('unknown', 'missing') else 0
            need(row['exit_code'] == row['expected_exit'] == expected, 'MSF exact exit')
            for channel in ('stdout', 'stderr'):
                need(sha(reader.bytes(p + row['name'] + '.' + channel)) == row[channel + '_sha256'], 'MSF stream pin')
            need(not reader.bytes(p + row['name'] + '.stderr'), 'MSF diagnostic')
        data = reader.bytes(p + 'selftest.stdout')
        need(sha(data) == '9a8c1fb0d62e61ce5b3fb67bc75a7c1a3268ba7f7e63bb5d1662c43edefb127c', 'MSF result pin')
        result = json.loads(data)
        need(result['variants'] == 56 and result['cut_pairs'] == 522 and result['rejects'] == 65 and
             result['mutant_refutations'] == 4 and result['projection_checked'] == 1 and
             result['GPU_executed'] is False and result['RSS_measured'] is False, 'MSF nonvacuity')
    parity(reader, MSF + 'o2_r1/', MSF + 'san_root_r1/', ['selftest', 'unknown', 'missing'])
    need(source_maps[0] == source_maps[1] and sha(reader.bytes(MSF + 'composable_msf.hpp')) ==
         source_maps[0]['composable_msf.hpp'], 'MSF O2/SAN/current source identity')


def full(reader):
    names = ['compiler', 'dependencies_before', 'compile', *FULL_POSITIVE, *FULL_CAUSES,
             'unknown', 'missing', 'dependencies_after']
    totals = None
    source_maps = []
    for folder, san in [('tower_o2_r1', False), ('tower_san_root_r1', True)]:
        p = FULL + folder + '/'
        receipt = reader.json(p + 'receipt.json')
        need(receipt['status'] == 'passed' and receipt['error'] is None and receipt['commands'] == 16 and
             receipt['closure'] == {'sources': True, 'compiler': True, 'dependencies': True, 'binary': True} and
             receipt['sanitizer'] is san and receipt['sanitizer_environment'] == (SAN if san else {}) and
             receipt['public_status'] == 'not_claimed' and receipt['performance_claim'] is False and
             receipt['device_executed'] is False and receipt['gcp_used'] is False, 'FULL closed scope')
        sources = reader.json(p + 'sources_before.json')
        source_maps.append(sources)
        need(sources == reader.json(p + 'sources_after.json') == reader.json(p + 'sources_copied_before.json') ==
             reader.json(p + 'sources_copied_after.json'), 'FULL copied sources closure')
        for name, expected in sources.items():
            need(pin(reader.bytes(p + 'source_snapshot/' + name)) == expected, 'FULL real snapshot source:' + name)
            need(pin(reader.bytes('build/' + name)) == expected, 'FULL current source linked to snapshot:' + name)
        for kind in ('compiler', 'dependencies', 'binary'):
            before = reader.json(p + kind + '_before.json')
            need(before and before == reader.json(p + kind + '_after.json'), 'FULL closure:' + kind)
        deps = reader.json(p + 'dependencies_before.json')
        for filename in ('dependencies_before.d', 'compile.d', 'dependencies_after.d'):
            need(dep_paths(reader.bytes(p + filename), 'mhgp7_graph_full_tower') == set(deps), 'FULL actual dependency selection')
        need(any(name.endswith('/boost/multiprecision/cpp_int.hpp') and row['kind'] == 'boost_system_header'
                 for name, row in deps.items()), 'actual Boost dependency attested')
        for name, row in deps.items():
            need(row['kind'] in ('source_snapshot', 'boost_system_header', 'system_header'), 'FULL dependency kind')
            if row['kind'] == 'source_snapshot':
                prefix, separator, tail = name.partition('/source_snapshot/')
                need(separator and prefix, 'FULL snapshot dependency path')
                need(pin(reader.bytes(p + 'source_snapshot/' + tail)) == {'sha256': row['sha256'], 'size': row['size']},
                     'FULL consumed dependency pin')
        commands = reader.json(p + 'commands.json')
        need([row['name'] for row in commands] == names, 'FULL exact command set')
        strict_flags(commands[2]['argv'], san)
        binary = reader.json(p + 'binary_before.json')
        binary_path = CAPTURE_ROOT + p + 'tower_gate'
        omitted_binary(reader, p + 'tower_gate', binary['sha256'], binary['size'])
        execution_paths(commands, binary_path, {name: '--' + name for name in [*FULL_POSITIVE, *FULL_CAUSES, 'unknown']} | {'missing': None})
        compiler_pins = reader.json(p + 'compiler_before.json')
        need(len(compiler_pins) == 1, 'FULL compiler identity')
        compiler = next(iter(compiler_pins))
        need(commands[0]['argv'] == [compiler, '--version'], 'FULL compiler version identity')
        snapshot = CAPTURE_ROOT + p + 'source_snapshot/'
        flags = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
            '-isystem', CAPTURE_ROOT + 'build/v7_boost_gate/extracted/usr/include', '-I', snapshot + 'v7_atlas_graph_20260911']
        flags += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if san else ['-O2']
        tu = snapshot + 'v7_graph_full_20260911/tower_gate.cpp'
        need(commands[2]['argv'] == [*flags, '-MD', '-MF', CAPTURE_ROOT + p + 'compile.d', '-MT',
            'mhgp7_graph_full_tower', tu, '-o', binary_path], 'FULL compile exact snapshot TU')
        for row, name in [(commands[1], 'dependencies_before'), (commands[-1], 'dependencies_after')]:
            need(row['argv'] == [*flags, '-M', '-MF', CAPTURE_ROOT + p + name + '.d', '-MT',
                'mhgp7_graph_full_tower', tu], 'FULL dependency command exact TU')
        for row in commands:
            expected = 4 if row['name'] in FULL_CAUSES else 2 if row['name'] in ('unknown', 'missing') else 0
            need(row['exit_code'] == row['expected_exit'] == expected, 'FULL exact exit:' + row['name'])
            for channel in ('stdout', 'stderr'):
                need(pin(reader.bytes(p + row['name'] + '.' + channel)) == row[channel], 'FULL captured stream pin')
            if row['name'] in ('compiler', 'dependencies_before', 'compile', 'dependencies_after'):
                need(not reader.bytes(p + row['name'] + '.stderr'), 'FULL compiler/sanitizer diagnostics')
        results = reader.json(p + 'results.json')
        need(set(results) == {*FULL_POSITIVE, *FULL_CAUSES, 'unknown', 'missing'}, 'FULL exact result set')
        for name, expected in FULL_POSITIVE.items():
            data = reader.bytes(p + name + '.stdout')
            need(sha(data) == expected[5], 'FULL exact qualified result:' + name)
            value = json.loads(data)
            need(value == results[name] and value['status'] == 'passed' and value['public_status'] == 'not_claimed' and
                 value['gcp_used'] is False and value['graph_construction_parallel'] is False and value['workers'] == [1, 4],
                 'FULL result scope')
            need((value['runs'], value['graph_orders'], value['paired_nodes'], value['oracle_cuts'], value['oracle_verticals']) ==
                 expected[:5] and value['census_runs'] == value['runs'] and value['paired_verticals'] == value['paired_nodes'],
                 'FULL positive nonvacuity')
            need(value['composed_orders'] == value['hub_orders'] == 3 * value['graph_orders'] and
                 value['hub_source_edges'] >= value['hub_tree_edges'] >= value['hub_projected_edges'] >= 3 * value['tree_edges'] > 0,
                 'FULL native and hub routes, three widths')
            progress(reader.bytes(p + name + '.stderr'), expected[0], mode=name)
        for name, cause in FULL_CAUSES.items():
            value = reader.json(p + name + '.stdout')
            need(value == results[name] == {'status': 'causal_rejection', 'cause': cause}, 'FULL exact mutation cause')
            progress(reader.bytes(p + name + '.stderr'), 1, True)
        for name in ('unknown', 'missing'):
            need(results[name] is None and not reader.bytes(p + name + '.stdout') and not reader.bytes(p + name + '.stderr'),
                 'FULL silent CLI rejection')
        totals = {key: sum(results[name][key] for name in FULL_POSITIVE)
                  for key in ('runs', 'paired_nodes', 'paired_contributions', 'paired_verticals', 'oracle_cuts', 'oracle_verticals')}
        need(totals['runs'] == 114 and results['uniform32']['hub_different_certificates'] == 135 and
             results['uniform32']['hub_orders'] == 180 and results['uniform32']['oracle_cuts'] == 0, 'FULL boundaries')
    parity(reader, FULL + 'tower_o2_r1/', FULL + 'tower_san_root_r1/', [*FULL_POSITIVE, *FULL_CAUSES, 'unknown', 'missing'])
    need(source_maps[0] == source_maps[1], 'FULL O2/SAN exact source identity')
    expected_pins = {FULL + 'tower_gate.cpp': '05340a05757524c61f79cb3eeb3e5eaae43c4bf81cc0ad60b6b36e6f4365debb',
        FULL + 'graph_full.hpp': 'bad5051fca4d78345841b4171dfb8a438c0f5a752452f784f4c9c196966a3f2d',
        ATLAS + 'atlas_graph.hpp': 'bb2b84d990a99289c2cc08198cedf44ec7c3013901a32af7ee7ecbcfaec6a7c0'}
    for name, digest in expected_pins.items():
        need(sha(reader.bytes(name)) == digest, 'ROOT reviewed primary source pin')
    need(source_maps[0]['v7_composable_msf_20260911/composable_msf.hpp']['sha256'] ==
         reader.json(MSF + 'o2_r1/source_before.json')['composable_msf.hpp'], 'qualified MSF linked to FULL snapshot')
    return totals


def verify(reader):
    added = extraction(reader)
    msf(reader)
    totals = full(reader)
    smoke = reader.json(FULL + 'smoke_o2_r1.json')
    need(smoke['status'] == 'passed' and smoke['capture_kind'] == 'exec_tool_results_combined_output_not_separate_stdout_stderr' and
         [row['exit_code'] for row in smoke['commands']] == [0, 0, 2, 2], 'historical smoke scope')
    need(json.loads(smoke['commands'][1]['combined_output'])['checks'] == 30 and smoke['geometry_qualified'] is False,
         'historical smoke nonvacuity')
    omitted_binary(reader, FULL + 'smoke_o2_r1', smoke['binary_sha256'])
    return {'qualification_commands_excluding_smoke': 62, 'historical_smoke_commands': 4, 'all_commands': 66,
            'FULL': totals, 'extraction_pin_additions_between_O2_SAN': added,
            'mutations_per_sanitizer_mode': {'extraction': 6, 'MSF': 4, 'FULL': 7},
            'scope': 'bounded private CPU composition, no industrial or GPU claim'}
