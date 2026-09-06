#!/usr/bin/env python3
"""Read-only CMake capture judge: no compiler, subprocess or mutable audit."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    require(len(sys.argv) == 2, 'usage: verify_cmake.py <capture>')
    root = Path(sys.argv[1]).resolve()
    report = json.loads((root / 'run.json').read_text())
    require(report['schema'] == 'mhgp7-full-meb-product-qualification-v1', 'schema')
    require(report['status'] == 'passed' and report['sources_stable'] is True, 'terminal')
    before = json.loads((root / 'sources_before.json').read_text())
    after = json.loads((root / 'sources_after.json').read_text())
    require(before == after and len(before) > 100, 'source stability and nonvacuum')
    require(before['morsehgp3D_v7/src/forest/silent_incidence.hpp'] ==
            'f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76', 'F immutable')
    require(before['morsehgp3D_v7/src/forest/meb_proposal.hpp'] ==
            'f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3', 'proposal pin')
    expected_names = ['compiler', 'cmake', 'git_head', 'git_status'] + [
        f'{mode}_{step}' for mode in ('release', 'san') for step in ('configure', 'build', 'ctest')]
    require([c['name'] for c in report['commands']] == expected_names, 'command inventory')
    for cmd in report['commands']:
        require(cmd['status'] == 'completed' and type(cmd['exit_code']) is int and cmd['exit_code'] == 0 and
                cmd['process_group_closed'] is True, 'command closed: ' + cmd['name'])
        for stream in ('stdout', 'stderr'):
            require(sha(root / (cmd['name'] + '.' + stream)) == cmd[stream + '_sha256'], 'command bytes')
        require((root / (cmd['name'] + '.stderr')).stat().st_size == 0, 'no compiler/SAN stderr')
    summaries = {}
    for mode in ('release', 'san'):
        raw = (root / (mode + '_ctest.stdout')).read_text()
        require('100% tests passed, 0 tests failed out of 30' in raw, '30 CTests: ' + mode)
        require(len(re.findall(r'\d+/30 Test\s+#\d+: .*Passed', raw)) == 30, 'individual CTests')
        rows = [json.loads(line.split(': ', 1)[1]) for line in raw.splitlines()
                if re.match(r'^\d+: \{"schema":', line)]
        full = [r for r in rows if r['schema'] == 'mhgp7-full-gabriel-meb-gate-v1']
        local = [r for r in rows if r['schema'] == 'mhgp7-meb-proposal-local-v1']
        require(len(full) == len(local) == 2, 'two new gates, two modes')
        for row in full + local:
            require(row['status'] == 'passed' and row['public_status'] == 'not_claimed', 'gate terminal')
        for row in full:
            for field, expected in {'clouds': 21, 'orders': 93, 'matrix_calls': 1488,
                                    'pairs': 1116, 'gamma_runs': 1488, 'cuts': 33792,
                                    'persistent_cases': 42, 'failures': 0}.items():
                require(type(row[field]) is int and row[field] == expected, 'FULL count ' + field)
            if row['test_mode'] == '--rejects':
                require(row['metadata_rejects'] == 28 and row['retained_proposal_refusals'] == 120 and
                        row['budgets'] == [{'exact': 32, 'one_short': 32}] * 5, 'five budget boundaries')
        require([(r['test_mode'], r['calls'], r['exception_boundaries']) for r in local] ==
                [('selftest', 109, 0), ('rejects', 45, 3)], 'local comparisons and explicit faults')
        eager = re.findall(r'\d+: full_gabriel_allocation .*allocations=(\d+) fault_runs=(\d+)', raw)
        lazy = re.findall(r'\d+: full_gabriel_lazy_allocation .*allocations=(\d+) fault_runs=(\d+)', raw)
        require(eager == [('49', '49')] * 3 and lazy == [('209', '209')] * 3, 'three allocation P sweeps')
        summaries[mode] = {'full': full, 'local': local, 'eager': eager, 'lazy': lazy}
    require(summaries['release'] == summaries['san'], 'same semantic summaries across builds')
    print(json.dumps({'status': 'passed', 'public_status': 'not_claimed', 'ctests_per_build': 30,
                      'source_pins': len(before), 'commands': len(report['commands']),
                      'full_outputs_per_gate_mode': 1488, 'Gamma_cuts_per_gate_mode': 33792,
                      'eager_faults_per_P': 49, 'lazy_faults_per_P': 209,
                      'P_configurations': 3, 'performance_claim': False}, sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(f'rejected: {error}', file=sys.stderr)
        raise SystemExit(1)
