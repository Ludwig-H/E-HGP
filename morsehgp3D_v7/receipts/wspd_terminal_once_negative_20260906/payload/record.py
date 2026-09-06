#!/usr/bin/env python3
"""Private small front differential; prepare and execute are explicit modes."""
import importlib.util
import json
from pathlib import Path
import shutil
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
PREVIOUS = ROOT / 'build/v7_wspd_q2_reuse_20260906_qualification_r1'
GEN = 'src/pipeline/generate.hpp'
REFERENCE = '345129a775d430a40e151d3b1adb5cd9efeaf77a6ffb6713bd081c74d40bdd9c'
COMMON = '003e3e6c972d880e2848fd2fb2371e3e650e5397e273970665308903a8cf9ea3'
GATE = '45c4736806e103b930c8ac7da1982d069d7ae115a9b310a62f4c847158fc6b2e'


def sha(path):
    import hashlib
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(ok, why):
    if not ok:
        raise ValueError(why)


def prepare():
    need(sha(PREVIOUS / 'record.py') == COMMON, 'common_pin')
    need(sha(PREVIOUS / 'front_gate.cpp') == GATE, 'gate_pin')
    need(sha(ROOT / 'morsehgp3D_v7' / GEN) == REFERENCE, 'live_reference_pin')
    for arm in ('reference', 'candidate'):
        shutil.copytree(ROOT / 'morsehgp3D_v7/src', BASE / arm / 'src')
    for source, name in ((PREVIOUS / 'record.py', 'capture_common.py'),
                         (PREVIOUS / 'front_gate.cpp', 'front_gate.cpp')):
        with (BASE / name).open('xb') as out:
            out.write(source.read_bytes())
    for name in ('logs', 'bin'):
        (BASE / name).mkdir()
    print('prepared; candidate still identical: apply reviewed private patch before --execute')


def execute():
    need(sha(BASE / 'capture_common.py') == COMMON, 'common_pin')
    need(sha(BASE / 'front_gate.cpp') == GATE, 'gate_pin')
    spec = importlib.util.spec_from_file_location('terminal_once_capture_common', BASE / 'capture_common.py')
    c = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(c)
    c.BASE, c.ROOT, c.LOG = BASE, ROOT, BASE / 'logs'
    before = c.sources()
    c.save(BASE / 'sources_before.json', before)
    old = {k.removeprefix('reference/'): v for k, v in before.items() if k.startswith('reference/')}
    new = {k.removeprefix('candidate/'): v for k, v in before.items() if k.startswith('candidate/')}
    need(old.keys() == new.keys() and [k for k in old if old[k] != new[k]] == [GEN], 'only_generate_delta')
    need(old[GEN] == REFERENCE, 'reference_pin')
    result = dict(schema='mhgp7-private-wspd-terminal-once-capture-v1', status='failed',
                  public_status='not_claimed', timing_claim=False, cpu=c.CPU,
                  reference_generate_sha256=old[GEN], candidate_generate_sha256=new[GEN],
                  gate_sha256=GATE, capture_common_sha256=COMMON, binaries={})
    try:
        c.run('compiler_version', ['g++', '--version'])
        for arm in ('reference', 'candidate'):
            label = 'O2_' + arm
            binary, dep = BASE / 'bin' / label, c.LOG / (label + '.d')
            c.run(label + '_compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic',
                  '-Werror', '-pthread', '-O2', '-I' + str(BASE / arm), '-MMD', '-MF', str(dep),
                  str(BASE / 'front_gate.cpp'), '-o', str(binary)], compile_step=True)
            result['binaries'][label] = sha(binary)
            dependencies = dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
            need(str(BASE / arm / GEN) in dependencies, 'actual_generate_dependency')
            need(all(Path(p).is_relative_to(BASE / arm) or Path(p) == BASE / 'front_gate.cpp'
                     for p in dependencies), 'dependency_snapshot_escape')
            c.save(c.LOG / (label + '.dependencies.json'), {p: sha(Path(p)) for p in dependencies})
            c.run(label + '_selftest', [str(binary), '--selftest'])
            c.run(label + '_unknown', [str(binary), '--unknown'], 2)
            need(sha(binary) == result['binaries'][label], 'binary_changed')
        pair = [str(c.LOG / ('O2_' + arm + '_selftest.stdout')) for arm in ('reference', 'candidate')]
        for opt in ('normal', 'optimized'):
            c.run('compare_' + opt, ['python3', '-B', *(['-O'] if opt == 'optimized' else []),
                  str(BASE / 'compare.py'), *pair])
        need((c.LOG / 'compare_normal.stdout').read_bytes() ==
             (c.LOG / 'compare_optimized.stdout').read_bytes(), 'normal_optimized_difference')
        result['status'] = 'completed'
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        after = c.sources()
        c.save(BASE / 'sources_after.json', after)
        result.update(sources_stable=before == after, commands=len(c.COMMANDS),
                      all_process_groups_closed=all(row['closed'] for row in c.COMMANDS))
        if before != after:
            result['status'] = 'failed'
        c.save(BASE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True))
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    if sys.argv[1:] == ['--prepare']:
        prepare()
    elif sys.argv[1:] == ['--execute']:
        sys.exit(execute())
    else:
        print('inert: --prepare copies sources; --execute qualifies only the 174 small fronts')
        sys.exit(2)
