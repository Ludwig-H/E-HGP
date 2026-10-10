#!/usr/bin/env python3
"""Static patch composition and exact synthetic geometry; no native execution."""
import hashlib
import itertools
import json
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent


def need(test, message):
    if not test:
        raise RuntimeError(message)


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def apply(text, load):
    """Strict application in memory, validating every unchanged/deleted line."""
    lines = text.splitlines(True)
    result = {}
    i = 0
    while i < len(lines):
        need(lines[i].startswith('--- a/'), 'old file header')
        name = lines[i][6:].strip()
        i += 1
        need(lines[i] == '+++ b/' + name + '\n', 'new file header')
        i += 1
        old = load(name).splitlines(True)
        dest = []
        at = 0
        while i < len(lines) and not lines[i].startswith('--- '):
            m = re.fullmatch(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@[^\n]*\n', lines[i])
            need(m is not None, 'hunk header')
            start = int(m[1]) - 1
            need(start >= at, 'overlapping hunks')
            dest.extend(old[at:start])
            at = start
            old_n = new_n = 0
            i += 1
            while i < len(lines) and lines[i][:1] in (' ', '-', '+') and not lines[i].startswith('--- a/'):
                line = lines[i]
                if line[0] in ' -':
                    need(at < len(old) and old[at] == line[1:], 'context: ' + name)
                    at += 1
                    old_n += 1
                if line[0] in ' +':
                    dest.append(line[1:])
                    new_n += 1
                i += 1
            need(old_n == int(m[2] or 1) and new_n == int(m[4] or 1), 'hunk lengths')
        dest.extend(old[at:])
        need(name not in result, 'duplicate file')
        result[name] = ''.join(dest)
    return result


def fixture():
    # Twice each midpoint, hence all comparisons are integers, radius^2 scaled by four = 49.
    points = list(itertools.product(range(0, 21, 7), repeat=3))
    centres = set()
    for a, b in itertools.combinations(points, 2):
        distance2 = sum((x - y) ** 2 for x, y in zip(a, b))
        need(distance2 >= 49, 'minimum spacing')
        if distance2 != 49:
            continue
        centre2 = tuple(x + y for x, y in zip(a, b))
        on = sum(sum((2 * x - c) ** 2 for x, c in zip(p, centre2)) == 49 for p in points)
        inside = sum(sum((2 * x - c) ** 2 for x, c in zip(p, centre2)) < 49 for p in points)
        need(on == 2 and inside == 0 and centre2 not in centres, 'Gabriel pair')
        centres.add(centre2)
    need(len(points) == 27 and len(centres) == 54, 'nontrivial K2 cohort')
    return {'sites': len(points), 'distinct_empty_pair_balls': len(centres), 'four_times_radius_squared': 49}


def main():
    need(len(sys.argv) == 2, 'usage: python check.py /path/to/repository')
    repo = Path(sys.argv[1])
    meta = json.loads((HERE / 'capture.json').read_text())
    def load(name):
        return subprocess.check_output(['git', '-C', str(repo), 'show', meta['source_commit'] + ':' + name], text=True)
    dependency = HERE / meta['dependency']
    dependency_text = dependency.read_text()
    need(sha(dependency_text) == meta['dependency_sha256'], 'CST-0244 patch digest')
    prior = apply(dependency_text, load)
    patch = (HERE / 'proposition.patch').read_text()
    need(sha(patch) == meta['patch_sha256'], 'test patch digest')
    changed = apply(patch, lambda name: prior[name] if name in prior else load(name))
    expected = {'morsehgp3D_v12/' + p for p in meta['sources']}
    need(set(changed) == expected and not (set(changed) & set(prior)), 'isolated two-file test extension')
    for name, hashes in meta['sources'].items():
        full = 'morsehgp3D_v12/' + name
        need(sha(load(full)) == hashes['before_sha256'], 'preimage hash')
        need(sha(changed[full]) == hashes['after_sha256'], 'postimage hash')
    for name, wanted in meta['context_sha256'].items():
        need(sha(load('morsehgp3D_v12/' + name)) == wanted, 'context hash: ' + name)
    unit = changed['morsehgp3D_v12/tests/tower/pipeline_unit.cpp']
    fault = changed['morsehgp3D_v12/tests/tower/pipeline_fault.cpp']
    # Integration anchors, not a claim of native behavior or syntax qualification.
    need('for (const bool engaged : {false, true})' in unit, 'admission ON/OFF')
    need('limit - 1, *pool, engaged, true' in unit and 'CHECK(binding >= 8)' in unit, 'strict limits per mode')
    need('CHECK_EQ(rejected.reason, Reason::tower_invariant)' in unit, 'late switch')
    for marker in ('Measured measure(', 'Outcome under_limit(', 'void late_switch('):
        part = unit.split(marker, 1)[1].split('tower::detail::open_session', 1)[0]
        need('run->chain_sites = engaged ? 0 : MemoryBudget::kUnlimited;' in part, 'threshold before open')
    part = fault.split('Result<Tower> forced_tower(', 1)[1].split('tower::detail::open_session', 1)[0]
    need('run->chain_sites = chain_sites;' in part, 'fault threshold before open')
    need('std::optional<u64> chain_sites = std::nullopt' in fault, 'public default route retained')
    need('for (const u64 chain_sites : {MemoryBudget::kUnlimited, u64{0}})' in fault, 'fault ON/OFF')
    need('make_chain(27, 2, 9, budget, *three, true)' in fault, 'K2 grid')
    need('CHECK(diag.forest.work[1].max_cohort > 1)' in fault and 'chain_sites == 0 ? 3u : 0u' in fault,
         'cohort and chain mask refer to same order')
    need('chain_number_jobs > 0 && diag.chain_history_jobs > 0' in fault, 'nonvacuous path counters')
    need('sweep(c, budget, *single, total, 1, chain_sites), total' in fault, 'exhaustive single-thread refusals')
    need('const unsigned long long parallel = parallel_calls.nothrow;' in fault, 'own three-thread baseline')
    valid = fault.split('bool valid_digest_hex(', 1)[1].split('// Adaptateur de porte', 1)[0]
    need('value.size() != 64' in valid and "c >= '0' && c <= '9'" in valid and "c >= 'a' && c <= 'f'" in valid,
         'digest sentinel rejected by shape validation')
    attempt = fault.split('Outcome attempt(', 1)[1].split('MHGP12_TEST(allocation,', 1)[0]
    off = attempt.index('nothrow_left.store(-1);')
    count = attempt.index('*calls = Calls{')
    fingerprint = attempt.index('*digest = digest_of(')
    validate = attempt.index('!valid_digest_hex(*digest)')
    need(off < count < fingerprint < validate, 'digest outside injection and counted allocations')
    old_fault = load('morsehgp3D_v12/tests/tower/pipeline_fault.cpp')
    section = lambda s: s.split('MHGP12_TEST(allocation, 12)', 1)[1].split('// Balayage des allocations', 1)[0]
    need(section(old_fault) == section(fault), 'public throwing-allocation gate unchanged')
    print(json.dumps({'source_commit': meta['source_commit'], 'composed_files': len(prior) + len(changed),
                      'test_files': len(changed), 'fixture': fixture(), 'native_execution': False,
                      'status': 'static proposal only; CST-0245 not closed'}, sort_keys=True))


if __name__ == '__main__':
    main()
