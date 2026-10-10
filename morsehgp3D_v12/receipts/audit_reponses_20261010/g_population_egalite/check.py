#!/usr/bin/env python3
"""Modèle exact de trois recherches et patch source isolé ; aucun moteur ni compilateur."""
from itertools import combinations
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile
HERE = Path(__file__).resolve().parent
U64 = (1 << 64) - 1


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sign(a, b):
    return (a > b) - (a < b)


def site_key(site):
    z = (site + 0x9E3779B97F4A7C15) & U64
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & U64
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & U64
    return z ^ (z >> 31)


def key_of(part, mask):
    return sum(map(site_key, part)) & U64 & mask


class Table:
    def __init__(self, parts, mask, k):
        self.mask, self.k = mask, k
        self.rows = sorted((key_of(p, mask), p, birth, birth + 1) for birth, p in enumerate(parts))
        need(all(a[:2] < b[:2] for a, b in zip(self.rows, self.rows[1:])), 'strict order')
        bits = (len(parts) - 1).bit_length() if len(parts) > 1 else 0
        self.shift = 64 - bits
        self.directory = [0] * ((1 << bits) + 1)
        for h, *_ in self.rows:
            self.directory[(h >> self.shift if self.shift < 64 else 0) + 1] += 1
        for i in range(1, len(self.directory)):
            self.directory[i] += self.directory[i - 1]

    def search(self, part, mode):
        trace, row_steps, key_steps = [], [], []
        if not self.rows or len(part) != self.k:
            return None, trace, row_steps, key_steps
        key = key_of(part, self.mask)
        bucket = key >> self.shift if self.shift < 64 else 0
        begin, end = self.directory[bucket:bucket + 2]

        def compare_row(pos):
            row = self.rows[pos][1]
            count = next((i + 1 for i, (a, b) in enumerate(zip(row, part)) if a != b), len(part))
            value = sign(row, part)
            row_steps.append((pos, value, count))
            return value

        def compare(pos):
            value = sign(self.rows[pos][0], key)
            key_steps.append(pos)
            if value == 0:
                value = compare_row(pos)
            trace.append((pos, value))
            return value

        def lower(lo, hi):
            while lo < hi:
                mid = lo + (hi - lo) // 2
                if compare(mid) < 0:
                    lo = mid + 1
                else:
                    hi = mid
            return lo

        if mode == 'old':
            pos = lower(begin, end)
            found = pos if pos < end and compare(pos) == 0 else None
        elif mode == 'early':
            lo, hi, found = begin, end, None
            while lo < hi:
                mid = lo + (hi - lo) // 2
                value = compare(mid)
                if value == 0:
                    found = mid
                    break
                if value < 0:
                    lo = mid + 1
                else:
                    hi = mid
        elif mode == 'candidate':
            lo, hi = begin, end
            while lo < hi:
                mid = lo + (hi - lo) // 2
                key_steps.append(mid)
                if self.rows[mid][0] < key:
                    lo = mid + 1
                else:
                    hi = mid
            found = None
            if lo < end:
                key_steps.append(lo)
                if self.rows[lo][0] == key:
                    if compare_row(lo) == 0:
                        found = lo
                    else:
                        pos = lower(lo + 1, end)
                        if pos < end and compare(pos) == 0:
                            found = pos
        else:
            raise ValueError(mode)
        return self.rows[found][2:] if found is not None else None, trace, row_steps, key_steps


def run(repo):
    cap = json.loads((HERE / 'capture.json').read_text())
    sources = {}
    for path, expected in cap['sources'].items():
        data = subprocess.check_output(['git', 'show', cap['source_git'] + ':' + path], cwd=repo)
        need(hashlib.sha256(data).hexdigest() == expected, 'source ' + path)
        sources[path] = data
    path = 'morsehgp3D_v12/src/tower/populations.cpp'
    source = sources[path].decode()
    need('if (pos > 0 && !s.less(s.sorted[pos - 1], e)) return fail(Reason::tower_invariant, t.k_);' in source,
         'strict layout guard')
    patch = (HERE / 'proposition.patch').read_bytes()
    need(hashlib.sha256(patch).hexdigest() == cap['patch_sha256'], 'patch pin')
    with tempfile.TemporaryDirectory(prefix='population-equality-source-') as td:
        root = Path(td)
        target = root / path
        target.parent.mkdir(parents=True)
        target.write_bytes(sources[path])
        subprocess.run(['git', 'apply', '--check', '-'], input=patch, cwd=root, check=True, capture_output=True)
        subprocess.run(['git', 'apply', '-'], input=patch, cwd=root, check=True, capture_output=True)
        changed = target.read_bytes()
        need(hashlib.sha256(changed).hexdigest() == cap['proposed_source_sha256'], 'proposed source')
        before_start, before_tail = source.split('std::optional<PopulationHit> PopulationTable::find(', 1)
        after_start, after_tail = changed.decode().split('std::optional<PopulationHit> PopulationTable::find(', 1)
        need(before_start == after_start and before_tail.split('u32 PopulationTable::candidate(', 1)[1] ==
             after_tail.split('u32 PopulationTable::candidate(', 1)[1], 'only find body changed')
        guard = 'if (entries_ == 0 || f.k != k_) return std::nullopt;'
        need(guard in before_tail.split('u32 PopulationTable::candidate(', 1)[0] and
             guard in after_tail.split('u32 PopulationTable::candidate(', 1)[0], 'same admission guards')
    totals = dict(queries=0, hits=0, misses=0, full_collision_queries=0, strict_hit_prefix=0,
                  old_comparisons=0, early_comparisons=0, saved_final_checks_on_miss=0,
                  old_row_comparisons=0, early_row_comparisons=0, candidate_row_comparisons=0,
                  old_key_comparisons=0, early_key_comparisons=0, candidate_key_comparisons=0)
    max_saved = 0
    examples = {}

    def judge(table, part):
        nonlocal max_saved
        old, early, candidate = [table.search(part, mode) for mode in ('old', 'early', 'candidate')]
        expected = next((r[2:] for r in table.rows if r[1] == part and len(part) == table.k), None)
        need(old[0] == early[0] == candidate[0] == expected, 'three exact answers')
        need(early[1] == old[1][:len(early[1])], 'prefix of old comparisons')
        need(len(early[1]) <= len(old[1]), 'non-increasing comparisons')
        if early[0] is not None:
            need(len(early[1]) < len(old[1]) and early[1][-1][1] == 0, 'strict hit prefix')
            totals['strict_hit_prefix'] += 1
        else:
            need(len(old[1]) - len(early[1]) in (0, 1), 'same miss path without final check')
            totals['saved_final_checks_on_miss'] += len(old[1]) - len(early[1])
        totals['queries'] += 1
        totals['hits' if expected is not None else 'misses'] += 1
        totals['full_collision_queries'] += table.mask == 0
        totals['old_comparisons'] += len(old[1])
        totals['early_comparisons'] += len(early[1])
        for name, value in [('old', old), ('early', early), ('candidate', candidate)]:
            totals[name + '_row_comparisons'] += len(value[2])
            totals[name + '_key_comparisons'] += len(value[3])
        max_saved = max(max_saved, len(old[1]) - len(early[1]))
        return dict(old=dict(comparisons=old[1], row_comparisons=len(old[2]), key_comparisons=len(old[3])),
                    early=dict(comparisons=early[1], row_comparisons=len(early[2]), key_comparisons=len(early[3])),
                    candidate=dict(row_comparisons=len(candidate[2]), key_comparisons=len(candidate[3])))

    universe = list(combinations(range(5), 2))
    queries = universe + [(0, 5), (2, 5), (5, 6)]
    for mask in (0, 1, 3, U64):
        for subset in range(1 << len(universe)):
            parts = [p for i, p in enumerate(universe) if subset >> i & 1]
            table = Table(parts, mask, 2)
            for part in queries:
                judge(table, part)
    for k in range(2, 13):
        prefix = tuple(range(k - 1))
        for mask in (0, 1, 3, U64):
            table = Table([prefix + (x,) for x in (100, 101, 103)], mask, k)
            for value in (99, 100, 101, 102, 103, 104):
                judge(table, prefix + (value,))
            judge(table, tuple(range(k + 1)))  # same wrong-cardinality guard
    examples['forced_zero_last_hit'] = judge(Table(universe, 0, 2), universe[-1])
    examples['forced_zero_missing_inside'] = judge(Table(universe[::2], 0, 2), universe[3])
    examples['unique_hash_hit'] = judge(Table(universe, U64, 2), universe[5])
    examples['u32_extreme'] = judge(Table([(0, (1 << 32) - 2)], U64, 2), (0, (1 << 32) - 2))
    need(totals['strict_hit_prefix'] == totals['hits'], 'all hits strict prefix')
    return dict(native_runs=0, source_git=cap['source_git'], checks=totals,
                max_comparisons_removed=max_saved, examples=examples,
                candidate_already_exists_for_G_L5=True, patch_integrated=False,
                native_equivalence_qualified=False, wall_time_gain_measured=False)


if __name__ == '__main__':
    need(len(sys.argv) == 2, 'usage: check.py DEPOT_GIT')
    result = run(Path(sys.argv[1]))
    need(json.loads(json.dumps(result)) == json.loads((HERE / 'results.json').read_text()), 'stored result')
    print(json.dumps(result, ensure_ascii=False, indent=2))
