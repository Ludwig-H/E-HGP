#!/usr/bin/env python3
"""Finish the same pinned qualification outside ptrace; preserve r2 failure."""
import importlib.util
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
OLD = BASE.parent / 'v7_local_plateau_20260906_qualification_r2'
spec = importlib.util.spec_from_file_location('capture', OLD / 'record.py')
c = importlib.util.module_from_spec(spec)
spec.loader.exec_module(c)
c.BASE = BASE
c.LOG = BASE / 'logs'


def main():
    result = dict(schema='mhgp7-private-local-plateau-qualification-completion-v1', status='failed',
                  public_status='not_claimed', previous_receipt_sha256=c.sha(OLD / 'receipt.json'),
                  reason='r2 LeakSanitizer cannot run under sandbox ptrace; no source delta')
    try:
        binary = OLD / 'bin/san'
        c.require(c.sha(binary) == 'de695de57fc1a80c46cc66ea0c095a82ee74ee8d0402a41b4eb0f9b22775da60', 'pinned_san')
        sources = json.loads((OLD / 'logs/san.sources_before.json').read_text())
        c.require(all(c.sha(p) == h for p, h in sources.items()), 'same_SAN_sources')
        c.save(BASE / 'san.sources_before.json', sources)
        c.command('san_selftest', [str(binary), '--selftest'])
        c.command('san_unknown', [str(binary), '--unknown'], 2)
        c.require((OLD / 'logs/O2_selftest.stdout').read_bytes() == (c.LOG / 'san_selftest.stdout').read_bytes(), 'O2_SAN_literal_equality')
        source = OLD / 'mutant/local_gate.cpp'
        common = ['g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
                  '-I' + str(c.ROOT / 'morsehgp3D_v7'), '-I' + str(c.BOOST)]
        c.command('mutant_dependencies', [*common, '-MM', str(source)])
        before = c.dependencies(c.LOG / 'mutant_dependencies.stdout')
        c.save(BASE / 'mutant.sources_before.json', before)
        mutant = BASE / 'bin/mutant'
        dep = c.LOG / 'mutant.d'
        c.command('mutant_compile', [*common, '-MMD', '-MF', str(dep), str(source), '-o', str(mutant)])
        c.require(c.dependencies(dep) == before, 'mutant_dependencies_stable')
        result['mutant_binary_sha256'] = c.sha(mutant)
        c.command('mutant_selftest', [str(mutant), '--selftest'], 1)
        c.require((c.LOG / 'mutant_selftest.stderr').read_text() ==
                  'local plateau rejected: quotient.strict_component_count\n', 'causal_star_mutant_message')
        c.require(all(c.sha(p) == h for p, h in sources.items()), 'same_SAN_sources_after')
        result['status'] = 'completed'
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        result.update(commands=len(c.ROWS), cpp_closed=all(row['closed'] for row in c.ROWS))
        c.save(BASE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
