#!/usr/bin/env python3
"""Cas restant du mutant drop_extra_ball : la fusion du bloc retire est-elle
portee par un autre bloc du meme niveau (lot atomique) ? Aucun assert."""
import json
import sys
from itertools import combinations
import tower_corpus as tc


def main():
    d = json.load(open(sys.argv[1]))
    out = []
    for case in [c for c in d['cases'] if not c['explained']]:
        points = [tuple(p) for p in case['points']]; ids = case['ids']; kmax = case['kmax']
        model = tc.build_model(points)
        rows, _ = tc.catalogue(model, kmax, ids)
        radius = tc.Q(case['radius'])
        level = model['level']; n = model['n']
        report = dict(n=n, kmax=kmax, radius=case['radius'], dropped_key=case['dropped_key'], ranks=[])
        for rank in case['ranks']:
            if rank['single_covering_S']:
                continue
            k = rank['k']
            verts = [m for m in range(1, model['full'] + 1) if tc.popcount(m) == k and level[m] < radius]
            parent = {v: v for v in verts}

            def find(v):
                while parent[v] != v:
                    v = parent[v]
                return v
            for m in range(1, model['full'] + 1):
                if tc.popcount(m) == k + 1 and level[m] < radius:
                    faces = [m ^ (1 << i) for i in range(n) if m >> i & 1]
                    for g in faces[1:]:
                        a, b = find(faces[0]), find(g)
                        if a != b:
                            parent[b] = a
            roots_open = sorted({find(v) for v in verts})
            same_level = [r for r in rows if r['radius'] == radius and r['p'] + r['arity'] - 1 <= k <= min(kmax, r['p'] + r['u'])]
            dropped = tuple(case['dropped_key'])
            merges = {}
            for r in same_level:
                ball = model['balls'][r['key']]
                S = [i for i in range(n) if ball['closed'] >> i & 1]
                supports = ball['supports']
                roots = set()
                for f in combinations(S, k):
                    m = sum(1 << i for i in f)
                    if not any(T & ~m == 0 for T in supports) and level[m] < radius:
                        roots.add(find(m))
                merges[r['key']] = sorted(roots)
            dsu = {r: r for r in roots_open}

            def f2(v):
                while dsu[v] != v:
                    v = dsu[v]
                return v
            for key, ps in merges.items():
                if key == dropped:
                    continue
                for q in ps[1:]:
                    a, b = f2(ps[0]), f2(q)
                    if a != b:
                        dsu[b] = a
            ps = merges[dropped]
            redundant = len(ps) >= 2 and len({f2(p) for p in ps}) == 1
            report['ranks'].append(dict(k=k, open_roots=len(roots_open), same_level_blocks=[
                dict(key=list(key), parents=ps) for key, ps in merges.items()],
                dropped_merge_carried_by_other_blocks=redundant))
        report['explained_by_redundant_merge'] = all(r['dropped_merge_carried_by_other_blocks'] for r in report['ranks'])
        out.append(report)
    print(json.dumps(dict(cases=out, all_explained=all(r['explained_by_redundant_merge'] for r in out)),
                     sort_keys=True, separators=(',', ':')))
    return 0 if all(r['explained_by_redundant_merge'] for r in out) else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except tc.Refusal as r:
        print('REFUSAL', r.reason, file=sys.stderr)
        sys.exit(r.code)
