#!/usr/bin/env python3
"""Inventory and portable result review only, without product or external execution."""
import hashlib
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
checks = 0


def need(ok, why):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(why)


def inventory(folder):
    records = {}
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        digest, relative = line.split('  ', 1)
        need(len(digest) == 64 and all(c in '0123456789abcdef' for c in digest), 'hash syntax')
        need(not Path(relative).is_absolute() and '..' not in Path(relative).parts, 'safe path')
        need(relative not in records, 'duplicate path')
        records[relative] = digest
    files = {p.relative_to(folder).as_posix() for p in folder.rglob('*')
             if p.is_file() and p != folder / 'SHA256SUMS'}
    need(set(records) == files, 'inventory ' + folder.name)
    for path, digest in records.items():
        need(hashlib.sha256((folder / path).read_bytes()).hexdigest() == digest, 'changed ' + path)
    return records


def main():
    records = inventory(ROOT)
    for child in ('ab_order', 'catalogue', 'meb', 'head', 'head_prereg_delta', 'lemma_r', 'j3_counter'):
        inventory(ROOT / child)
    expected = {'ab_order/check.py': 482,
                'catalogue/check_q3.py': 1983,
                'meb/check.py': 1344,
                'head/check_head_followup.py': 94,
                'head/source/morsehgp3D_v11/bench/points_flat_claims.py': 10,
                'head_prereg_delta/check_scope.py': 6,
                'lemma_r/check.py': 103200}
    runs = json.loads((ROOT / 'REPLAYS.json').read_text())['runs']
    need(len(runs) == 2 * len(expected), 'replay count')
    for script, count in expected.items():
        pair = [r for r in runs if r['script'] == script]
        need(len(pair) == 2 and {r['optimized'] for r in pair} == {False, True}, 'mode pair')
        need(all(r['exit_code'] == 0 and not r['stderr'] for r in pair), 'replay passed')
        need(pair[0]['stdout_sha256'] == pair[1]['stdout_sha256'], 'mode identity')
        for r in pair:
            need(records[r['stdout_file']] == r['stdout_sha256'], 'result identity')
            raw = (ROOT / r['stdout_file']).read_text()
            if script.endswith('points_flat_claims.py'):
                need(raw == 'points_flat_claims_verdict conforme checks10\n', 'selftest verdict')
            else:
                need(json.loads(raw)['checks'] == count, 'check count')
    late = json.loads((ROOT / 'late_counters/SOURCE_REVIEW.json').read_text())
    for record in late['records']:
        need(hashlib.sha256((ROOT / 'late_counters' / record['capture']).read_bytes()).hexdigest()
             == record['sha256'], 'late source identity')
    bound = 3 * late['max_leaf'] * sum(math.comb(late['max_leaf'], q) for q in range(1, 5))
    need(late['max_leaf'] == 1024 and bound == late['leaf_count_bound'] < 2**49, 'late counter bound')
    source = (ROOT / 'late_counters/leaf.cpp').read_text()
    pairs = re.findall(r'checked_add\(ledger\.(\w+), c\.(\w+)\)', source)
    need(len(pairs) == 15 and {a for a, b in pairs} == set(late['fields'])
         and all(a == b for a, b in pairs), 'fifteen checked field flushes')
    need(all(re.search(r'\b' + name + r'\s*=\s*0\b', source) for name in late['fields']),
         'local fields initialized')
    need(source.index('MHGP11_TRY(extend(leaf, 0, 0') < source.index('return flush(leaf.counts, run.ledger)'),
         'flush after successful extension')
    print(json.dumps({'status': 'PASS', 'integrity_checks': checks, 'payloads': len(records),
                      'bounded_checks_each_mode': sum(expected.values()),
                      'native_runs': 0, 'fits': 0, 'gcp_actions': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
