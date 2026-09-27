#!/usr/bin/env python3
"""Fresh portable resident qualification; CUDA source pinned, not compiled here.

Explicit orchestration reuse from direct_bands/run.py; not inherited evidence.
"""
import importlib.util
import hashlib
import math
from pathlib import Path
import struct
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
BASE_FILE = HERE.parent / 'b_q34_direct_bands_20260927/run.py'
if hashlib.sha256(BASE_FILE.read_bytes()).hexdigest() != '3165f540438340a23792b0c26217b20bc0eec07635c6f661a7acdffa8ff249e5':
    raise RuntimeError('frozen collector changed')
spec = importlib.util.spec_from_file_location('resident_local_orchestration', BASE_FILE)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, sha, read = base.need, base.sha, base.read
base.HERE = HERE
base.TARGET = 'mhgp9_q34_filtered_resident'
base.SCHEMA = 'mhgp9_q34_filtered_resident_portable_capture_v1'
base.PROBE = 'mhgp9_q34_filtered_resident_v1'
MUTANTS = ('CLOSE', 'RAW_BASE', 'OWNER')
FAULTS = ('RECTANGLE', 'PAIR')
TIMES = set('input index front open input_copy_validate index_copy cuda_init index_upload '
            'rectangle_upload_allocate rectangle_kernel rectangle_download_allocate rectangle_release '
            'compaction input_release arena consume pair_upload_allocate waves_including_count_download '
            'survivor_download_allocate order_convert pair_release adapter_cleanup adapter reference '
            'comparison destruction total'.split())


def fixture_bytes():
    # Tiny artificial u32 coordinate file; never called LiDAR or SemanticKITTI.
    return b''.join(struct.pack('<III', 10*i, (i*i*13) % 101, (i*37) % 83) for i in range(12))


def sources():
    paths = [p for p in (base.ROOT / 'morsehgp3D_v9/src').rglob('*') if p.suffix in ('.hpp', '.cpp', '.h', '.cu')]
    paths += [p for p in HERE.iterdir() if p.suffix in ('.hpp', '.cpp', '.cu', '.py') or p.name == 'CMakeLists.txt']
    paths += [HERE.parent / d / f for d, f in (
        ('b_q34_cuda_waves_20260927', 'wire.hpp'), ('b_q34_cuda_waves_20260927', 'cuda_api.hpp'),
        ('b_q34_cuda_waves_20260927', 'snapshot.hpp'), ('b_q34_arena_waves_20260927', 'waves.hpp'),
        ('b_q34_collective_arena_20260927', 'arena.hpp'), ('b_q34_direct_bands_20260927', 'direct.hpp'),
        ('b_q34_factor_plan_20260926', 'plan.hpp'), ('b_q34_factor_plan_20260926', 'inputs.py'))]
    paths += [BASE_FILE, base.HELPER, *base.LIBS.values(), base.ROOT / 'morsehgp3D_v9/tests/gen/front_fixtures.hpp']
    return {str(p): sha(p) for p in sorted(set(paths))}


def fixture_path(state):
    return Path(state['builds']['release']) / 'tiny_artificial.u32'


def compile_variant(state, kind, name, macro):
    compiler = 'c++' if kind == 'release' else 'clang++'
    output = str(Path(state['builds'][kind]) / name)
    argv = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    argv += ['-O2'] if kind == 'release' else ['-O1', '-g1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    argv += ['-D' + macro, '-I' + str(base.ROOT / 'morsehgp3D_v9/src'),
             '-I' + str(base.ROOT / 'morsehgp3D_v9/src/gen'), '-I' + str(base.ROOT / 'morsehgp3D_v9/tests/gen'),
             str(HERE / 'probe.cpp'), str(HERE / 'device_cpu.cpp'), str(HERE / 'device_stub.cpp'),
             str(base.LIBS[kind]), '-pthread', '-o', output]
    return [('compile_' + kind + '_' + name, argv, 0), (kind + '_' + name, [output, '--gate'], 2)]


def recipe(state):
    rows = [('system', ['uname', '-a'], 0)]
    for kind in ('release', 'sanitize'):
        compiler = 'c++' if kind == 'release' else 'clang++'
        binary = state['binaries'][kind]
        flags = '-Wall -Wextra -Wpedantic -Werror'
        if kind == 'sanitize':
            flags += ' -O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer'
        rows += [('compiler_' + kind, [compiler, '--version'], 0),
            ('configure_' + kind, ['cmake', '-S', str(HERE), '-B', state['builds'][kind],
                '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_COMPILER=' + compiler, '-DCMAKE_CXX_FLAGS=' + flags,
                '-DMHGP9_SOURCE_ROOT=' + str(base.ROOT / 'morsehgp3D_v9'),
                '-DMHGP9_GEN_LIBRARY=' + str(base.LIBS[kind]), '-DMHGP9_RESIDENT_ENABLE_CUDA=OFF'], 0),
            ('build_' + kind, ['cmake', '--build', state['builds'][kind], '--target', base.TARGET, '-j', '2'], 0),
            ('api_cpp17_' + kind, [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsyntax-only',
                '-I' + str(base.ROOT / 'morsehgp3D_v9/src'), '-include', str(HERE / 'device.hpp'), '-x', 'c++', '/dev/null'], 0),
            ('gate_' + kind, [binary, '--gate'], 0)]
        if kind == 'release':
            rows.append(('fixture', [sys.executable, str(HERE / 'run.py'), '--fixture', str(fixture_path(state))], 0))
        rows.append(('tiny_' + kind, [binary, '--frame', str(fixture_path(state)), '--qr', '7', '--q', '7', '--workers', '4'], 0))
        for label, args in (
            ('zero_q', ['--q', '0']), ('zero_qr', ['--qr', '0']), ('zero_workers', ['--workers', '0']),
            ('negative', ['--q', '-1']), ('suffix', ['--q', '7tail']), ('overflow', ['--q', '18446744073709551616']),
            ('wide_k', ['--k', '4294967301']), ('wide_s', ['--s', '4294967304']),
            ('wide_workers', ['--workers', '4294967296']), ('missing', ['--frame']), ('stub', ['--cuda'])):
            rows.append(('refuse_' + kind + '_' + label, [binary, '--gate', *args], 2))
        for fault in FAULTS:
            rows += compile_variant(state, kind, 'fault_' + fault.lower(), 'MHGP9_RESIDENT_FAULT_' + fault)
    for mutant in MUTANTS:
        rows += compile_variant(state, 'release', 'mutant_' + mutant.lower(), 'MHGP9_RESIDENT_MUTANT_' + mutant)
    return rows


OLD_BUILD_PINS = base.build_pins
def build_pins(state):
    pins = OLD_BUILD_PINS(state)
    for kind in ('release', 'sanitize'):
        for name in ['fault_' + f.lower() for f in FAULTS] + ([] if kind != 'release' else ['mutant_' + m.lower() for m in MUTANTS]):
            p = Path(state['builds'][kind]) / name
            pins[str(p)] = sha(p)
    pins[str(fixture_path(state))] = sha(fixture_path(state))
    return pins


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == base.SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False, 'closed local capture')
    need(state['cases'] == [] and state['partitions'] == [], 'qualification only')
    need(state['pins_before'] == state['pins_after'] == sources(), 'LIVE source closure including uncompiled CUDA TU')
    need(state['build_pins'] == build_pins(state), 'LIVE binaries')
    need(fixture_path(state).read_bytes() == fixture_bytes(), 'artificial fixture identity')
    commands = recipe(state)
    need(len(commands) == len(state['commands']), 'complete command inventory')
    gates, tiny, mutations, faults = [], [], [], []
    for row, (name, argv, code) in zip(state['commands'], commands):
        need(row['name'] == name and row['argv'] == argv and row['exit_code'] == code and row['group_closed'] and
             row['ended_epoch'] >= row['started_epoch'], 'command identity/closure')
        need(read(directory / (name + '.command.json')) == row, 'stored command')
        need(all(row.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'intent')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == row[stream + '_sha256'], 'stream hash')
        stderr = (directory / (name + '.stderr')).read_text()
        stdout = (directory / (name + '.stdout')).read_text()
        if name.startswith(('gate_', 'tiny_')):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == base.PROBE and value['status'] == 'passed' and value['cuda_executed'] is False and not stderr,
                 'portable verdict only')
            if name.startswith('gate_'):
                need(value['mode'] == 'gate' and value['cases'] == 85 and value['portable_runs'] == 3*value['cases'] and value['cuda_runs'] == 0,
                     'portable gate matrix')
                for key in ('queries', 'survivors', 'planned', 'fallbacks', 'empty', 'zero_output', 'reordered', 'pool_rejected',
                            'pool_lane_reduced', 'fallback_survivors', 'closed', 'source_gaps', 'arena_gaps', 'raw_holes',
                            'rectangle_waves', 'pair_waves', 'rejections'):
                    need(type(value[key]) is int and value[key] > 0, 'nonvacuity: ' + key)
                gates.append(value)
            else:
                need(value['mode'] == 'frame' and value['n'] == 12 and value['workers_arena'] == 4 and value['workers_front'] == 1 and
                     value['reference_workers'] == 4 and value['Q_requested'] == value['Qr_requested'] == 7 and
                     value['device'] == 'portable_exact_predicates' and value['k'] == 5 and value['s'] == 8, 'tiny frame identity')
                need(value['physical_queries'] == value['E'] and 0 < value['S'] <= value['E'] <= value['P'] <= value['raw_pairs'], 'tiny nonvacuity')
                times = value['times_ms']
                need(set(times) == TIMES and all(type(v) in (int, float) and math.isfinite(v) and v >= 0 for v in times.values()), 'time schema')
                need(times['adapter']+.01 >= sum(times[k] for k in ('open', 'arena', 'consume', 'adapter_cleanup')), 'complete adapter lifetime')
                need(times['total']+.01 >= sum(times[k] for k in ('input', 'index', 'front', 'adapter', 'reference', 'comparison', 'destruction')),
                     'reference paid separately')
                tiny.append({k: v for k, v in value.items() if k != 'times_ms'})
        if name.startswith('refuse_'):
            expected = ('resident.CUDA_not_compiled' if name.endswith('_stub') else
                ('resident_probe.usage' if name.endswith('_missing') else
                 ('resident_probe.configuration' if '_zero_' in name else
                  ('resident_probe.number_range' if name.endswith(('_overflow', '_wide_k', '_wide_s', '_wide_workers')) else 'resident_probe.number'))))
            need(stderr.strip() == expected and not stdout, 'precise refusal/no success')
        if name.startswith('release_mutant_'):
            causes = dict(close='resident_gate.rectangle_masks', raw_base='resident_gate.compact_identity', owner='resident_gate.refusal_missing')
            need(stderr.strip() == causes[name.removeprefix('release_mutant_')] and not stdout, 'causal mutant killed')
            mutations.append(dict(name=name, cause=stderr.strip()))
        if name.startswith(('release_fault_', 'sanitize_fault_')):
            expected = 'resident.' + name.rsplit('_', 1)[1] + '_stack_failure'
            need(stderr.strip() == expected and not stdout, 'synthetic portable failure/no publication')
            faults.append(dict(name=name, cause=stderr.strip()))
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitizer gate equality')
    need(len(tiny) == 2 and tiny[0] == tiny[1], 'Release/sanitizer artificial frame equality')
    need(len(mutations) == 3 and len(faults) == 4, 'all variants')
    return dict(status='PASS', commands=len(commands), gate=gates[0], tiny=tiny[0], mutants=mutations, injected_portable_failures=faults,
                CUDA_compiled=False, GCP_used=False, scope='portable_resident_seam_only_no_GPU_no_FULL')


base.sources, base.recipe, base.readback, base.build_pins = sources, recipe, readback, build_pins
base.cases = lambda: ([], {}, [])
if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--fixture':
        target = Path(sys.argv[2]);need(not target.exists(), 'fresh fixture');target.write_bytes(fixture_bytes())
    else:
        base.main()
