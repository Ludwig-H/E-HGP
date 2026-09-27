#!/usr/bin/env python3
"""Bounded CPU seam gates, compiled libraries read-only, never calls GCP."""
import importlib.util
import json
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BASE = HERE.parent / 'b_q34_factor_plan_20260926'
sys.dont_write_bytecode = True
sys.path.insert(0, str(BASE))
spec = importlib.util.spec_from_file_location('seam_base', BASE / 'run.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, sha, read, save = base.need, base.sha, base.read, base.save
OUT = ROOT / 'morsehgp3D_v9/receipts/q34_batch_seam_20260927'
BUILD = Path('/workspaces/E-HGP/build/v9-audit-q34-seam-20260927-r1')
TARGET = 'mhgp9_q34_batch_seam'


def pins():
    result = base.source_pins()
    for path in [*HERE.glob('*.cpp'), *HERE.glob('*.py'), HERE / 'CMakeLists.txt',
                 HERE.parent / 'b_q34_bands_20260927/bands.hpp']:
        result[str(path)] = sha(path)
    return result


def recipe():
    rows = []
    for kind, compiler in [('release', 'g++'), ('sanitize', 'clang++')]:
        build = BUILD / kind
        rows += [(kind+'_configure', ['cmake', '-S', str(HERE), '-B', str(build),
                  '-DCMAKE_BUILD_TYPE='+('Release' if kind == 'release' else 'Debug'),
                  '-DCMAKE_CXX_COMPILER='+compiler, '-DMHGP9_SOURCE_ROOT='+str(ROOT/'morsehgp3D_v9'),
                  '-DMHGP9_GEN_LIBRARY='+str(base.LIBS[kind]),
                  '-DMHGP9_AUDIT_SANITIZE='+('OFF' if kind == 'release' else 'ON')], 0),
                 (kind+'_build', ['cmake', '--build', str(build), '--parallel', '2'], 0),
                 (kind+'_gate', [str(build/TARGET)], 0)]
        rows += [(kind+'_'+name, [str(build/TARGET), name], 1) for name in ('no-sort', 'lose-implicit')]
    return rows


def capture():
    need(not BUILD.exists() and not OUT.exists(), 'fresh paths only')
    OUT.mkdir(parents=True)
    commands = base.command_class()(OUT, dict(os.environ,
        ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
    state = dict(status='running', pins_before=pins(), commands=[], GCP_used=False)
    save(OUT/'capture.json', state)
    try:
        for name, argv, code in recipe():
            rc, _, _ = commands.run(name, argv)
            state['commands'] = commands.rows
            save(OUT/'capture.json', state)
            need(rc == code, 'unexpected exit '+name+': '+str(rc))
        state['pins_after'] = pins()
        need(state['pins_before'] == state['pins_after'], 'source drift')
        state['build_pins'] = {str(p): sha(p) for kind in ('release','sanitize') for p in
            (BUILD/kind/TARGET, BUILD/kind/'CMakeCache.txt',
             BUILD/kind/'CMakeFiles'/f'{TARGET}.dir/flags.make',
             BUILD/kind/'CMakeFiles'/f'{TARGET}.dir/link.txt')}
        state['status'] = 'completed'
        save(OUT/'capture.json', state)
        check()
    except BaseException as error:
        state.update(status='failed', error=type(error).__name__+': '+str(error))
        save(OUT/'capture.json', state)
        raise


def check():
    state = read(OUT/'capture.json')
    need(state['status'] == 'completed' and state['GCP_used'] is False, 'closed local capture')
    need(state['pins_before'] == state['pins_after'] == pins(), 'LIVE source inventory')
    for path, pin in state['build_pins'].items():
        need(sha(Path(path)) == pin, 'build drift')
    need(len(state['commands']) == len(recipe()), 'complete recipe')
    for row, (name, argv, code) in zip(state['commands'], recipe()):
        need(row['name'] == name and row['argv'] == argv and row['exit_code'] == code, 'command binding')
        need(read(OUT/(name+'.command.json')) == row, 'stored command binding')
        need(all(row.get(k) == v for k,v in read(OUT/(name+'.intent.json')).items()), 'intent binding')
        for stream in ('stdout', 'stderr'):
            need(sha(OUT/(name+'.'+stream)) == row[stream+'_sha256'], 'output binding')
    a, b = read(OUT/'release_gate.stdout'), read(OUT/'sanitize_gate.stdout')
    need(a == b and a['status'] == 'pass' and a['schema'] == 'mhgp9_q34_batch_seam_v1', 'gate equality')
    need(a['cases'] == 576 and min(a[k] for k in ('implicit_pairs','implicit3','implicit4','survivors','reorder_cases')) > 0,
         'nonvacuity')
    for kind in ('release','sanitize'):
        for name, message in [('no-sort','seam_original_order'),('lose-implicit','seam_lane_accounting')]:
            need((OUT/(kind+'_'+name+'.stderr')).read_text() == 'factor_plan.'+message+'\n', 'causal mutant')
    print(json.dumps(dict(status='passed', commands=10, gates=a, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    need(sys.argv[1:] in (['run'],['check']), 'usage: run.py run|check')
    capture() if sys.argv[1] == 'run' else check()
