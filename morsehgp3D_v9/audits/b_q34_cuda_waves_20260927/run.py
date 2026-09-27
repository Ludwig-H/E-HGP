#!/usr/bin/env python3
"""Fresh portable wire qualification; CUDA TU pinned but NOT compiled here.

Explicit reuse of frozen process-group orchestration, not inherited evidence.
"""
import importlib.util
from pathlib import Path
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
BASE_FILE = HERE.parent / 'b_q34_direct_bands_20260927/run.py'
spec = importlib.util.spec_from_file_location('cuda_wave_local_orchestration', BASE_FILE)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, sha, read = base.need, base.sha, base.read
need(sha(BASE_FILE) == '3165f540438340a23792b0c26217b20bc0eec07635c6f661a7acdffa8ff249e5', 'frozen collector')
base.HERE = HERE
base.TARGET = 'mhgp9_q34_cuda_waves'
base.SCHEMA = 'mhgp9_q34_cuda_waves_portable_capture_v1'
base.PROBE = 'mhgp9_q34_cuda_waves_v1'


def sources():
    paths = [p for p in (base.ROOT / 'morsehgp3D_v9/src').rglob('*') if p.suffix in ('.hpp', '.cpp', '.h', '.cu')]
    paths += [p for p in HERE.iterdir() if p.suffix in ('.hpp', '.cpp', '.cu', '.py') or p.name == 'CMakeLists.txt']
    paths += [HERE.parent / d / f for d, f in (
        ('b_q34_arena_waves_20260927', 'waves.hpp'), ('b_q34_collective_arena_20260927', 'arena.hpp'),
        ('b_q34_direct_bands_20260927', 'direct.hpp'), ('b_q34_factor_plan_20260926', 'plan.hpp'),
        ('b_q34_factor_plan_20260926', 'inputs.py'))]
    paths += [BASE_FILE, base.HELPER, *base.LIBS.values(), base.ROOT / 'morsehgp3D_v9/tests/gen/front_fixtures.hpp']
    return {str(p): sha(p) for p in sorted(set(paths))}


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
                '-DMHGP9_GEN_LIBRARY=' + str(base.LIBS[kind]), '-DMHGP9_CUDA_WAVES_ENABLE_CUDA=OFF'], 0),
            ('build_' + kind, ['cmake', '--build', state['builds'][kind], '--target', base.TARGET, '-j', '2'], 0),
            ('wire_cpp17_' + kind, [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsyntax-only',
                '-I' + str(base.ROOT / 'morsehgp3D_v9/src'), '-include', str(HERE / 'wire.hpp'), '-x', 'c++', '/dev/null'], 0),
            ('gate_' + kind, [binary, '--gate'], 0)]
        for label, args in (
            ('zero', ['--q=0']), ('negative', ['--q=-1']), ('suffix', ['--q=7tail']),
            ('overflow', ['--q=18446744073709551616']), ('wide_k', ['--k=4294967301']),
            ('wide_s', ['--s=4294967304']), ('missing', ['--frame']), ('stub', ['--cuda'])):
            rows.append(('refuse_' + kind + '_' + label, [binary, '--gate', *args], 2))
    for mutant in ('MASK6', 'ORDINAL'):
        output = str(Path(state['builds']['release']) / ('mutant_' + mutant.lower()))
        rows += [('compile_mutant_' + mutant.lower(), ['c++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
            '-DMHGP9_AUDIT_CUDA_WAVES_MUTANT_' + mutant,
            '-I' + str(base.ROOT / 'morsehgp3D_v9/src'), '-I' + str(base.ROOT / 'morsehgp3D_v9/src/gen'),
            '-I' + str(base.ROOT / 'morsehgp3D_v9/tests/gen'), str(HERE / 'probe.cpp'), str(HERE / 'runner.cpp'),
            str(HERE / 'runner_stub.cpp'), str(base.LIBS['release']), '-pthread', '-o', output], 0),
            ('mutant_' + mutant.lower(), [output, '--gate'], 2)]
    return rows


OLD_BUILD_PINS = base.build_pins
def build_pins(state):
    pins = OLD_BUILD_PINS(state)
    for mutant in ('mask6', 'ordinal'):
        p = Path(state['builds']['release']) / ('mutant_' + mutant)
        pins[str(p)] = sha(p)
    return pins


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == base.SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False, 'closed local capture')
    need(state['cases'] == [] and state['partitions'] == [], 'gate only')
    need(state['pins_before'] == state['pins_after'] == sources(), 'LIVE source closure including uncompiled CUDA TU')
    need(state['build_pins'] == build_pins(state), 'LIVE binaries')
    commands = recipe(state)
    need(len(commands) == len(state['commands']), 'complete command inventory')
    gates = []
    mutants = []
    for row, (name, argv, code) in zip(state['commands'], commands):
        need(row['name'] == name and row['argv'] == argv and row['exit_code'] == code and row['group_closed'] and
             row['ended_epoch'] >= row['started_epoch'], 'command identity/closure')
        need(read(directory / (name + '.command.json')) == row, 'stored command')
        need(all(row.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'intent')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == row[stream + '_sha256'], 'stream hash')
        stderr = (directory / (name + '.stderr')).read_text()
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == base.PROBE and value['status'] == 'passed' and value['mode'] == 'gate' and
                 value['cuda_executed'] is False and not stderr, 'portable gate only')
            for key in ('cases', 'runs', 'queries', 'survivors', 'planned', 'fallbacks', 'empty', 'zero_output',
                        'reordered', 'mixed_masks', 'pool_rejected', 'pool_lane_reduced', 'fallback_survivors'):
                need(type(value[key]) is int and value[key] > 0, 'nonvacuity: ' + key)
            need(value['runs'] == 4*value['cases'], 'wave-size matrix')
            gates.append(value)
        if name.startswith('refuse_'):
            expected = 'CUDA not compiled' if name.endswith('_stub') else ('cuda_probe.usage' if name.endswith('_missing') else
                ('cuda_probe.configuration' if name.endswith('_zero') else
                 ('cuda_probe.number_range' if name.endswith(('_overflow', '_wide_k', '_wide_s')) else 'cuda_probe.number')))
            need(stderr.strip() == expected, 'precise refusal')
        if name.startswith('mutant_'):
            allowed = {'mask6': ('cuda_waves.physical_counts', 'cuda_gate.survivors_order_mask'),
                       'ordinal': ('cuda_waves.duplicate_ordinal', 'cuda_gate.survivors_order_mask', 'cuda_gate.u64_decode')}
            need(stderr.strip() in allowed[name[7:]], 'causal mutant killed')
            mutants.append(dict(name=name, cause=stderr.strip()))
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitizer equality')
    return dict(status='PASS', commands=len(commands), gate=gates[0], mutants=mutants,
                CUDA_compiled=False, GCP_used=False, scope='portable_wire_and_predicates_only_no_GPU_no_FULL')


base.sources, base.recipe, base.readback, base.build_pins = sources, recipe, readback, build_pins
base.cases = lambda: ([], {}, [])
if __name__ == '__main__':
    base.main()
