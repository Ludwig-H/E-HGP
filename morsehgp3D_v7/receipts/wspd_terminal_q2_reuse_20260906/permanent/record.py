#!/usr/bin/env python3
"""Six direct commands: compile/run the permanent live gate, O2 and SAN."""
import json
from pathlib import Path
import sys
import capture_common as c

c.BASE = Path(__file__).resolve().parent
c.ROOT = c.BASE.parents[1]
c.LOG = c.BASE / 'logs'
PROJECT = c.ROOT / 'morsehgp3D_v7'
GATE = PROJECT / 'tests/wspd_terminal_reuse_gate.cpp'
FILES = sorted((PROJECT / 'src').rglob('*.hpp')) + [GATE, PROJECT / 'CMakeLists.txt',
        PROJECT / 'cmake/run_expect.cmake', c.BASE / 'capture_common.py', c.BASE / 'record.py']


def inventory():
    return {str(p.relative_to(c.ROOT)): c.sha(p) for p in FILES}


def main():
    before = inventory()
    c.save(c.BASE / 'sources_before.json', before)
    result = dict(schema='mhgp7-wspd-terminal-reuse-live-capture-v1', status='failed',
                  public_status='not_claimed', binaries={}, commands=0, cpp_closed=False)
    try:
        c.need(c.sha(PROJECT / 'src/pipeline/generate.hpp') ==
               '345129a775d430a40e151d3b1adb5cd9efeaf77a6ffb6713bd081c74d40bdd9c', 'live_generate_pin')
        for mode in ('O2', 'san'):
            flags = ['-O2'] if mode == 'O2' else ['-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            binary = c.BASE / 'bin' / mode
            dep = c.LOG / (mode + '.d')
            c.run(mode + '_compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-pthread', *flags, '-MMD', '-MF', str(dep), str(GATE), '-o', str(binary)], compile_step=True)
            result['binaries'][mode] = c.sha(binary)
            deps = dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
            observed = {str(Path(p).resolve().relative_to(c.ROOT)): c.sha(p) for p in deps}
            c.need(observed and all(k in before and v == before[k] for k, v in observed.items()), 'actual_dependencies')
            c.save(c.LOG / (mode + '.dependencies.json'), observed)
            c.run(mode + '_selftest', [str(binary), '--selftest'])
            c.run(mode + '_unknown', [str(binary), '--unknown'], 2)
            out = json.loads((c.LOG / (mode + '_selftest.stdout')).read_text())
            c.need(out['schema'] == 'mhgp7-wspd-terminal-reuse-gate-v1' and out['status'] == 'passed'
                   and type(out['calls']) is int and out['calls'] == 174 and out['refusals'] == 6, 'gate_summary')
            c.need(c.sha(binary) == result['binaries'][mode], 'stable_binary')
        c.need((c.LOG / 'O2_selftest.stdout').read_bytes() == (c.LOG / 'san_selftest.stdout').read_bytes(),
               'O2_SAN_output_difference')
        result['status'] = 'completed'
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        after = inventory()
        c.save(c.BASE / 'sources_after.json', after)
        result.update(sources_stable=before == after, commands=len(c.COMMANDS),
                      cpp_closed=all(row['closed'] for row in c.COMMANDS))
        if before != after:
            result['status'] = 'failed'
        c.save(c.BASE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
