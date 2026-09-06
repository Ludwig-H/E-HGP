#!/usr/bin/env python3
"""Small O2/SAN qualification; no histogram performance measurement."""
import json
from pathlib import Path
import sys
import capture_common as c

c.BASE = Path(__file__).resolve().parent
c.ROOT = c.BASE.parents[1]
c.LOG = c.BASE / 'logs'
PROJECT = c.ROOT / 'morsehgp3D_v7'
PRIVATE = c.ROOT / 'build/v7_wspd_histogram_blocks_20260906'
GATE = PRIVATE / 'histogram_gate.cpp'
FILES = sorted((PROJECT / 'src').rglob('*.hpp')) + [GATE, PRIVATE / 'histogram_blocks.hpp',
        c.BASE / 'capture_common.py', c.BASE / 'record.py']


def inventory():
    return {str(p.relative_to(c.ROOT)): c.sha(p) for p in FILES}


def main():
    before = inventory()
    c.save(c.BASE / 'sources_before.json', before)
    result = dict(schema='mhgp7-private-histogram-block-qualification-v1', status='failed',
                  public_status='not_claimed', binaries={}, commands=0, cpp_closed=False)
    try:
        for mode in ('O2', 'san'):
            flags = ['-O2'] if mode == 'O2' else ['-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            binary = c.BASE / 'bin' / mode
            dep = c.LOG / (mode + '.d')
            c.run(mode + '_compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-pthread', *flags, '-I' + str(PROJECT), '-MMD', '-MF', str(dep), str(GATE), '-o', str(binary)],
                compile_step=True)
            result['binaries'][mode] = c.sha(binary)
            deps = dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
            observed = {str(Path(p).resolve().relative_to(c.ROOT)): c.sha(p) for p in deps}
            c.need(observed and all(k in before and v == before[k] for k, v in observed.items()), 'actual_dependencies')
            c.save(c.LOG / (mode + '.dependencies.json'), observed)
            c.run(mode + '_selftest', [str(binary), '--selftest'])
            c.run(mode + '_unknown', [str(binary), '--unknown'], 2)
            out = json.loads((c.LOG / (mode + '_selftest.stdout')).read_text())
            c.need(out['schema'] == 'mhgp7-private-histogram-blocks-v1' and out['status'] == 'passed'
                   and type(out['comparisons']) is int and out['comparisons'] == 126, 'gate_summary')
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
