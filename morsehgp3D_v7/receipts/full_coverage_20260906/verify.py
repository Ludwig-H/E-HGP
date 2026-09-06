#!/usr/bin/env python3
"""Portable evidence checks, also effective under python -O; not a rerun."""
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def need(value, reason):
    if not value:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


seen = set()
for line in (HERE / 'SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    path = (HERE / name).resolve()
    need(path.is_relative_to(HERE) and name not in seen, 'manifest path')
    need(sha(path) == digest, 'manifest hash: ' + name)
    need(path.read_bytes()[:4] != b'\x7fELF', 'ELF not allowed')
    seen.add(name)
need(seen == {str(p.relative_to(HERE)) for p in HERE.rglob('*')
              if p.is_file() and p.name != 'SHA256SUMS'}, 'manifest complete')
refs = json.loads((HERE / 'source_refs.json').read_text())
for row in refs.values():
    path = (ROOT / row['repo_path']).resolve()
    need(path.is_relative_to(ROOT) and sha(path) == row['sha256'], 'project reference')
direct = json.loads((HERE / 'direct/receipt.json').read_text())
need(direct['status'] == 'completed' and direct['all_cpp_closed'] and not direct['gcp_used'], 'direct status')
commands = json.loads((HERE / 'direct/commands.json').read_text())
need(len(commands) == 16 and all(row['closed'] and row['exit_code'] == row['expected'] for row in commands), 'direct commands')
expected = b'full_coverage_certificate checks=710 rejects=30 replay_cuts=30 gamma_cuts=34 allocation_rejects=34 authority=structural_only\n'
need((HERE / 'direct/O2_selftest.stdout').read_bytes() == expected ==
     (HERE / 'direct/SAN_selftest.stdout').read_bytes(), 'O2 SAN counts')
for name, reason in [('drop_continuation','growth.no_fake_node'),
                     ('future_contribution','growth.no_future_leak'),
                     ('final_root','replay.live_identity')]:
    need((HERE / ('direct/' + name + '.stderr')).read_text() == 'FAIL ' + reason + '\n', 'causal mutant')
cmake = json.loads((HERE / 'cmake/run_r1/receipt.json').read_text())
need(cmake['status'] == 'completed' and len(cmake['commands']) == 4 and
     all(row['closed'] and row['exit_code'] == 0 for row in cmake['commands']), 'CMake commands')
cases = ET.parse(HERE / 'cmake/run_r1/junit.xml').getroot().findall('.//testcase')
need(len(cases) == 6 and all(case.find('failure') is None and case.find('skipped') is None for case in cases), 'six CTests')
for name, digest in json.loads((HERE / 'cmake/run_r1/project_sources.json').read_text()).items():
    need(refs[name]['sha256'] == digest, 'CMake exact project source')
print(json.dumps(dict(status='verified_captures_only', files=len(seen), source_refs=len(refs),
                     direct_commands=16, ctests=6, public_status='not_claimed', gcp_used=False), sort_keys=True))
