#!/usr/bin/env python3
"""Modèle borné exact : première sonde virtuelle, collisions, rang strict et file ; aucun moteur."""
from dataclasses import dataclass
from fractions import Fraction as Q
from itertools import combinations, product
import hashlib
import json
from pathlib import Path
import subprocess
import sys
HERE = Path(__file__).resolve().parent
MASK64 = (1 << 64) - 1


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sign(a, b):
    return (a > b) - (a < b)


def site_key(x):
    z = (x + 0x9E3779B97F4A7C15) & MASK64
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK64
    return z ^ (z >> 31)


@dataclass(frozen=True)
class View:
    inner: tuple
    shell: tuple
    mask: int

    @property
    def k(self):
        return len(self.inner) + self.mask.bit_count()

    def __iter__(self):
        i, rest = 0, self.mask
        while i < len(self.inner) or rest:
            j = (rest & -rest).bit_length() - 1 if rest else 0
            inside = not rest or (i < len(self.inner) and self.inner[i] < self.shell[j])
            if inside:
                yield self.inner[i]
                i += 1
            else:
                yield self.shell[j]
                rest &= rest - 1

    def part(self):
        # Independent oracle: selection and built-in sorting, not the virtual merge.
        return tuple(sorted(self.inner + tuple(x for j, x in enumerate(self.shell) if self.mask >> j & 1)))

    def key(self, mask):
        # Already performed once per cell/representative by current passes.cpp.
        base = sum(map(site_key, self.inner)) & MASK64
        shell_keys = tuple(map(site_key, self.shell))
        return (base + sum(h for j, h in enumerate(shell_keys) if self.mask >> j & 1)) & MASK64 & mask


def certify(view, k):
    need(type(view.mask) is int and 0 <= view.mask <= MASK64, 'u64 mask')
    need(len(view.shell) <= 64 and 2 <= k <= 12, 'shell/order domain')
    need(all(type(x) is int and 0 <= x < (1 << 32) for x in view.inner + view.shell), 'SiteIdx domain')
    need(all(a < b for values in (view.inner, view.shell) for a, b in zip(values, values[1:])), 'sorted unique I/U')
    need(set(view.inner).isdisjoint(view.shell), 'disjoint I/U')
    need(view.mask >> len(view.shell) == 0, 'mask outside shell')
    need(view.k == k, 'mask cardinality')


def cmp_row_virtual(row, view):
    cursor = iter(view)  # Fresh cursor for EACH comparison, never a shared consumed cursor.
    for x in row:
        y = next(cursor)
        if x != y:
            return sign(x, y)
    return 0


class Table:
    def __init__(self, parts, mask, k):
        self.k, self.mask = k, mask
        self.rows = sorted((sum(map(site_key, p)) & MASK64 & mask, p, birth, birth % 4 + 1)
                           for birth, p in enumerate(parts))
        bits = (len(parts) - 1).bit_length() if len(parts) > 1 else 0
        self.shift = 64 - bits
        self.directory = [0] * ((1 << bits) + 1)
        for key, *_ in self.rows:
            self.directory[(key >> self.shift if self.shift < 64 else 0) + 1] += 1
        for i in range(1, len(self.directory)):
            self.directory[i] += self.directory[i - 1]

    def find(self, query, virtual):
        part = None if virtual else query.part()
        if not self.rows or query.k != self.k:
            return None, []
        key, trace = query.key(self.mask), []
        bucket = key >> self.shift if self.shift < 64 else 0
        lo, hi = self.directory[bucket:bucket + 2]
        stop = hi

        def compare(pos):
            at, row, *_ = self.rows[pos]
            value = sign(at, key) if at != key else (cmp_row_virtual(row, query) if virtual else sign(row, part))
            trace.append((pos, value))
            return value

        while lo < hi:
            mid = lo + (hi - lo) // 2
            if compare(mid) < 0:
                lo = mid + 1
            else:
                hi = mid
        hit = self.rows[lo][2:] if lo < stop and compare(lo) == 0 else None
        return hit, trace


def prefix(hit, part, junction, levels=20, table_present=True):
    # Common resolver prefix; continuation after a miss is deliberately not modelled.
    counts = dict(probes=0, controls=0, first_probe_hits=0, chain_histogram_0=0, max_chain=0)
    if not table_present or junction >= levels:
        return ('tower_invariant', None, counts, 0)
    counts['probes'] += 1
    if hit is not None:
        birth, rank = hit
        counts['controls'] += 1
        if rank >= junction:
            return ('tower_invariant', None, counts, 0)
        counts['first_probe_hits'] += 1
        counts['chain_histogram_0'] += 1
        return ('birth', birth, counts, 1)
    return ('locate_same_resolver', part, counts, 1)


def pipeline(queries, table, virtual):
    slots, events, results, formed = [None] * 16, [], [], 0
    incoming = outgoing = 0

    def finish(item):
        nonlocal formed
        rep, view, early_part = item
        hit, searches = table.find(view, virtual)
        part = early_part
        if virtual and hit is None:
            part = view.part()
            formed += 1
        # The real call on miss retains FirstProbe{done=true, hit=nullopt}.
        results.append((rep, prefix(hit, part, 12), searches))
        events.append(('finish', rep))

    for rep, view in enumerate(queries):
        early = None if virtual else view.part()
        formed += not virtual
        slots[incoming % 16] = (rep, view, early)
        events.append(('directory', rep, view.key(table.mask)))
        if incoming >= 8:
            previous = slots[(incoming - 8) % 16]
            events.append(('bucket', previous[0], previous[1].key(table.mask)))
        incoming += 1
        if incoming - outgoing == 16:
            finish(slots[outgoing % 16])
            outgoing += 1
    while outgoing < incoming:
        finish(slots[outgoing % 16])
        outgoing += 1
    return results, events, formed


def run(repo):
    cap = json.loads((HERE / 'capture.json').read_text())
    for path, h in cap['sources'].items():
        raw = subprocess.check_output(['git', 'show', cap['source_git'] + ':' + path], cwd=repo)
        need(hashlib.sha256(raw).hexdigest() == h, 'source pin ' + path)
    masks = (0, 1, 3, MASK64)
    tables = {(k, m): Table(list(combinations(range(6), k))[::2], m, k) for k in range(2, 6) for m in masks}
    counts = dict(views=0, lookups=0, hits=0, misses=0, comparisons=0, forced_zero=0)
    retained = []
    for labels in product(range(4), repeat=6):
        inner = tuple(i for i, label in enumerate(labels) if label == 1)
        shell = tuple(i for i, label in enumerate(labels) if label >= 2)
        selected = sum(1 << j for j, site in enumerate(shell) if labels[site] == 3)
        view = View(inner, shell, selected)
        if not 2 <= view.k <= 5:
            continue
        counts['views'] += 1
        certify(view, view.k)
        need(tuple(view) == view.part(), 'merge sequence')
        for mask in masks:
            table = tables[view.k, mask]
            reference, steps = table.find(view, False)
            candidate, candidate_steps = table.find(view, True)
            expected = next((row[2:] for row in table.rows if row[1] == view.part()), None)
            need(candidate == reference == expected and steps == candidate_steps, 'collision lookup/comparisons')
            need(view.key(mask) == (sum(map(site_key, view.part())) & MASK64 & mask), 'existing additive hash')
            counts['lookups'] += 1
            counts['comparisons'] += len(steps)
            counts['hits' if candidate else 'misses'] += 1
            counts['forced_zero'] += mask == 0
        if view.k == 2:
            retained.append(view)
    directed = [View(tuple(range(0, 10, 2)), tuple(range(1, 129, 2)), sum(1 << j for j in (0, 2, 7, 13, 31, 47, 63))),
                View((), tuple(range(64)), 1 | (1 << 63)), View(tuple(range(12)), (), 0)]
    for view in directed:
        certify(view, view.k)
        need(2 <= view.k <= 12 and tuple(view) == view.part(), '12 sites / bit63')
        for mask in masks:
            table = Table([view.part()], mask, view.k)
            need(table.find(view, False) == table.find(view, True), 'directed exact lookup')
    # Empty table, wrong order, missing exact key under forced collision.
    need(Table([], 0, 2).find(retained[0], True) == (None, []), 'empty table')
    need(Table([(0, 1, 2)], 0, 3).find(View((), (0, 1), 3), True) == (None, []), 'wrong cardinality')
    need(Table([(0, 1)], 0, 2).find(View((), (0, 2), 3), True)[0] is None, 'hash-only false hit')
    invalid = [
        (View((), (1, 2), 5), 2),  # correct popcount, but bit 2 outside U
        (View((0,), (1, 2), 3), 2),
        (View((2, 1), (), 0), 2), (View((), (2, 1), 3), 2),
        (View((1,), (1, 2), 1), 2),
        (View((1, 1), (), 0), 2), (View((), (1, 1), 3), 2),
        (View((), (1, 2), -1), 2), (View((), tuple(range(65)), 3), 2),
        (View(tuple(range(13)), (), 0), 13),
    ]
    for view, k in invalid:
        try:
            certify(view, k)
        except ValueError:
            continue
        raise ValueError('invalid view accepted')
    rank_cases = 0
    for hit in (None, (7, 3), (8, 4), (9, 5)):
        for junction in (4, 20):
            for present in (False, True):
                materialized = retained[0].part()
                lazy = materialized if hit is None else None
                need(prefix(hit, materialized, junction, table_present=present) ==
                     prefix(hit, lazy, junction, table_present=present), 'rank and counter equivalence')
                rank_cases += 1
    need(prefix((7, 4), None, 4)[0] == prefix((7, 5), None, 4)[0] == 'tower_invariant', 'strict rank')
    queue_cases, skipped = 0, 0
    for length in (0, 1, 7, 8, 15, 16, 17, 33, 65):
        queries = (retained * 2)[:length]
        for mask in masks:
            old = pipeline(queries, tables[2, mask], False)
            new = pipeline(queries, tables[2, mask], True)
            need(old[:2] == new[:2], 'same 16-slot prefetch queue and prefix results')
            misses = sum(row[1][0] == 'locate_same_resolver' for row in new[0])
            need(new[2] == misses and old[2] == length, 'materialization only on miss')
            skipped += old[2] - new[2]
            queue_cases += 1
    points = ((Q(0), Q(0)), (Q(4), Q(0)), (Q(2), Q(3)))
    center = (Q(2), Q(5, 6))
    distance2 = lambda a, b: sum((x - y) ** 2 for x, y in zip(a, b))
    radius = Q(169, 36)
    need(all(distance2(point, center) == radius for point in points), 'triple circle')
    weights = (Q(13, 36), Q(13, 36), Q(5, 18))
    need(sum(weights) == 1 and all(w > 0 for w in weights), 'strict support')
    need(tuple(sum(w * p[axis] for w, p in zip(weights, points)) for axis in range(2)) == center, 'center in hull')
    pair_levels = []
    for pair in combinations(range(3), 2):
        mid = tuple(sum(points[i][axis] for i in pair) / 2 for axis in range(2))
        level = distance2(points[pair[0]], mid)
        other = next(i for i in range(3) if i not in pair)
        need(distance2(points[other], mid) > level and level < radius, 'Gabriel pair strictly before junction')
        pair_levels.append(str(level))
    need(pair_levels == ['4', '13/4', '13/4'], 'triangle exact levels')
    return dict(native_runs=0, source_git=cap['source_git'], exhaustive=counts, directed_views=len(directed), invalid_preconditions_refused=len(invalid),
                rank_counter_cases=rank_cases, queue_cases=queue_cases, model_materializations_skipped=skipped,
                triangle=dict(center=['2', '5/6', '0'], level='169/36', pair_levels=pair_levels,
                              distinct_births=3, inner=[], shell_size=3, order=2),
                guarantees=['same sorted stream', 'same lower-bound comparisons including hash collisions',
                            'same first-probe counters and strict rank', 'same 16-slot/8-stage event order',
                            'Part formed only on miss in model'], gain_measured=False)


if __name__ == '__main__':
    need(len(sys.argv) == 2, 'usage: check.py DEPOT_GIT')
    result = run(Path(sys.argv[1]))
    need(result == json.loads((HERE / 'results.json').read_text()), 'stored result')
    print(json.dumps(result, ensure_ascii=False, indent=2))
