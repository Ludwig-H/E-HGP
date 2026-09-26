#!/usr/bin/env python3
"""Toy checks for the object-safety lens of L3, L5, L10, L12 (scratch, not a receipt).

L3: first failure (position, facet, reason) of the BallId-indexed guards of
    order_block_lean (full_ball_tower.hpp:1267-1268) versus a position-indexed
    fast path (pos < lot_begin) plus a slow path re-deriving the reason from
    level_run, on programs that are sorted subsequences of a ball universe,
    with targets that may be out of range, outside the program, in the same
    lot, in a later lot, or valid.
L5: group order of the lot DSU (1306-1319) versus a counting sort on the
    min representative (CSR), random lots.
L10: failure-path `representatives` of the per-facet adds versus a batched
    sum, on a mid-order failure.
L12: fused live count versus the scan (1402-1404), using the spec lane's
    transliteration; also the spec's literal formula n0 + births + merges -
    sum|parents| (births already include the K1 domain nodes, 1369 -> 1170).
"""
import importlib.util
import random
import sys

SPEC = "/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/phaseA/spec/phase_a_model.py"
spec = importlib.util.spec_from_file_location("phase_a_model", SPEC)
model = importlib.util.module_from_spec(spec)
spec.loader.exec_module(model)

ABSENT = None


def l3_instance(rng):
    nballs = rng.randint(2, 30)
    level_run = sorted(rng.randint(0, 8) for _ in range(nballs))
    rng.shuffle(level_run)  # level_run per BallId (any assignment)
    by_level = sorted(range(nballs), key=lambda b: (level_run[b], b))
    program = [b for b in by_level if rng.random() < 0.6]
    if not program:
        program = [by_level[0]]
    facets = []
    for j, b in enumerate(program):
        fs = []
        for _ in range(rng.choice([0, 1, 2, 3])):
            kind = rng.random()
            if kind < 0.05:
                fs.append(nballs + rng.randint(0, 3))  # out of range
            else:
                fs.append(rng.randrange(nballs))      # anything in range
        facets.append(fs)
    return nballs, level_run, program, facets


def l3_today(nballs, level_run, program, facets):
    """First failure of the BallId-indexed guards; anchors written after each lot."""
    anchors = [ABSENT] * nballs
    begin = 0
    while begin < len(program):
        run = level_run[program[begin]]
        end = begin + 1
        while end < len(program) and level_run[program[end]] == run:
            end += 1
        for j in range(begin, end):
            for f, t in enumerate(facets[j]):
                if not (t < nballs and level_run[t] < level_run[program[j]]):
                    return (j, f, "static_target_not_strict")
                if anchors[t] is ABSENT:
                    return (j, f, "static_closed_anchor_missing")
        for j in range(begin, end):
            anchors[program[j]] = 1  # any node id: only presence matters here
        begin = end
    return None


def l3_positions(nballs, level_run, program, facets):
    """Phase 0 translates t -> pos (sentinel if not in program, never refuses);
    phase A: pos < lot_begin is the fast path, otherwise re-derive the reason."""
    pos_of = [ABSENT] * nballs  # the O(balls) fill
    for p, b in enumerate(program):
        pos_of[b] = p
    translated = [[(t, pos_of[t] if t < nballs and pos_of[t] is not ABSENT and program[pos_of[t]] == t else ABSENT)
                   for t in fs] for fs in facets]
    begin = 0
    while begin < len(program):
        run = level_run[program[begin]]
        end = begin + 1
        while end < len(program) and level_run[program[end]] == run:
            end += 1
        for j in range(begin, end):
            for f, (t, p) in enumerate(translated[j]):
                if p is not ABSENT and p < begin:
                    continue
                # slow path, from the BallId kept beside the position
                if not (t < nballs and level_run[t] < level_run[program[j]]):
                    return (j, f, "static_target_not_strict")
                return (j, f, "static_closed_anchor_missing")
        begin = end
    return None


def l5_groups_dsu(roots):
    n = len(roots)
    dsu = list(range(n))

    def find(a):
        while dsu[a] != a:
            dsu[a] = dsu[dsu[a]]
            a = dsu[a]
        return a
    owners = sorted((p, b) for b in range(n) for p in roots[b])
    for i in range(1, len(owners)):
        if owners[i - 1][0] == owners[i][0]:
            a, b = find(owners[i - 1][1]), find(owners[i][1])
            dsu[max(a, b)] = min(a, b)
    groups = [[] for _ in range(n)]
    for b in range(n):
        groups[find(b)].append(b)
    return [g for g in groups if g]


def l5_groups_csr(roots):
    # any connected components, then label = min block index, counting sort stable in b
    n = len(roots)
    label = list(range(n))
    changed = True
    while changed:  # naive label propagation (a different CC algorithm on purpose)
        changed = False
        for a in range(n):
            for b in range(n):
                if set(roots[a]) & set(roots[b]) and label[a] != label[b]:
                    m = min(label[a], label[b])
                    label[a] = label[b] = m
                    changed = True
    count = [0] * (n + 1)
    for b in range(n):
        count[label[b] + 1] += 1
    for i in range(n):
        count[i + 1] += count[i]
    slots = [None] * n
    fill = count[:]
    for b in range(n):
        slots[fill[label[b]]] = b
        fill[label[b]] += 1
    return [slots[count[r]:count[r + 1]] for r in range(n) if count[r + 1] > count[r]]


def main():
    rng = random.Random(20260926)
    # L3
    stats = {"equal": 0, "failing": 0, "not_strict": 0, "anchor_missing": 0, "ok_runs": 0}
    for _ in range(20000):
        inst = l3_instance(rng)
        a, b = l3_today(*inst), l3_positions(*inst)
        if a != b:
            print("L3 MISMATCH", inst, a, b)
            return 1
        stats["equal"] += 1
        if a is None:
            stats["ok_runs"] += 1
        else:
            stats["failing"] += 1
            stats["not_strict" if a[2].endswith("not_strict") else "anchor_missing"] += 1
    print("L3 ok", stats)
    # L5
    for _ in range(20000):
        n = rng.randint(2, 9)
        roots = [sorted(set(rng.randrange(8) for _ in range(rng.randint(0, 3)))) for _ in range(n)]
        if l5_groups_dsu(roots) != l5_groups_csr(roots):
            print("L5 MISMATCH", roots)
            return 1
    print("L5 ok 20000")
    # L12 on valid instances (spec transliteration)
    literal_off = 0
    for i in range(4000):
        k1 = i % 2 == 0
        inst = model.instance(rng, k1)
        ref = model.reference(inst)
        st = ref["stats"]
        fl = ref["flat"]
        sum_parents = sum(fl["parent_begin"][a + 1] - fl["parent_begin"][a]
                          for a in range(len(fl["parent_begin"]) - 1)
                          if fl["parent_begin"][a + 1] - fl["parent_begin"][a] >= 2)
        fused = st["births"] + st["merges"] - sum_parents
        if fused != ref["live"] or len(ref["nxt"]) != st["births"] + st["merges"]:
            print("L12 MISMATCH", i, fused, ref["live"])
            return 1
        literal = inst["n0"] + st["births"] + st["merges"] - sum_parents
        if literal != ref["live"]:
            literal_off += 1
    print("L12 ok 4000 (fused = births + merges - sum|parents| of merges);",
          "literal n0 + births + merges - sum|parents| differs on", literal_off, "instances (all K1)")
    # L10: a mid-order failure at facet f of block j
    c = [2, 1, 0, 3, 2]
    j, f = 3, 1
    per_facet = sum(c[:j]) + f + 1
    print("L10 failure at block", j, "facet", f, ": per-facet representatives =", per_facet,
          "; batched sum =", sum(c), "; batched-at-end (not reached) = 0")
    return 0


if __name__ == "__main__":
    sys.exit(main())
