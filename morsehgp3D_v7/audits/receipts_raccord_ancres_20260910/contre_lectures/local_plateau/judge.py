"""Independent rational judge of local_plateau rank() on explicit integer shells.
No assert; exceptions on disagreement. Fractions only."""
from __future__ import annotations
from fractions import Fraction as Q
from itertools import combinations
import json, random, subprocess, sys
from collections import Counter

def need(ok, why):
    if not ok:
        raise ValueError(why)

# ---------- spheres: (a, b, c) BallKey; center = -b/(2a); r2 = |center|^2 - c/a
SPHERES = {
    'int_center_5_r25': (1, (-10, -10, -10), 50),          # center (5,5,5), r2=25
    'half_center_11_2_r121_4': (1, (-11, -10, -10), 50),   # center (11/2,5,5), r2=121/4
    'big_center_30000_r5000': (1, (-60000, -60000, -60000), 2675000000),  # r2=25e6
    'third_center_31_3': (3, (-62, -60, -60), 896),         # center (31/3,10,10), r2=73/9, no integer antipodes
}

def center_of(key):
    a, b, c = key
    return tuple(Q(-bi, 2 * a) for bi in b)

def r2_of(key):
    a, b, c = key
    cen = center_of(key)
    return sum(x * x for x in cen) - Q(c, a)

def power(key, p):
    a, b, c = key
    return a * sum(x * x for x in p) + sum(bi * xi for bi, xi in zip(b, p)) + c

def sphere_points(key, box):
    (x0, x1), (y0, y1), (z0, z1) = box
    return [(x, y, z) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1) for z in range(z0, z1 + 1)
            if power(key, (x, y, z)) == 0]

def interior_points(key, box):
    (x0, x1), (y0, y1), (z0, z1) = box
    return [(x, y, z) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1) for z in range(z0, z1 + 1)
            if power(key, (x, y, z)) < 0]

# ---------- exact convex containment via barycentric Gram solve (own implementation)
def solve(mat, n):
    # mat: n rows, n+1 cols of Fractions; returns solution or None if singular
    m = [row[:] for row in mat]
    for col in range(n):
        piv = next((r for r in range(col, n) if m[r][col] != 0), None)
        if piv is None:
            return None
        m[col], m[piv] = m[piv], m[col]
        d = m[col][col]
        m[col] = [v / d for v in m[col]]
        for r in range(n):
            if r != col and m[r][col] != 0:
                f = m[r][col]
                m[r] = [a - f * b for a, b in zip(m[r], m[col])]
    return [m[i][n] for i in range(n)]

def in_conv_affine_independent(pts, c):
    """c in conv(pts) when pts affinely independent; None if dependent (skip)."""
    p0 = pts[0]
    E = [tuple(Q(pi[k] - p0[k]) for k in range(3)) for pi in pts[1:]]
    n = len(E)
    if n == 0:
        return tuple(Q(v) for v in p0) == c
    off = tuple(c[k] - p0[k] for k in range(3))
    G = [[sum(E[i][k] * E[j][k] for k in range(3)) for j in range(n)] + [sum(E[i][k] * off[k] for k in range(3))] for i in range(n)]
    w = solve(G, n)
    if w is None:
        return None
    if any(wi < 0 for wi in w) or 1 - sum(w) < 0:
        return False
    rec = tuple(Q(p0[k]) + sum(w[i] * E[i][k] for i in range(n)) for k in range(3))
    return rec == c

def contains_table(shell, c):
    u = len(shell)
    tab = [False] * (1 << u)
    for mask in range(1, 1 << u):
        bits = [i for i in range(u) if mask >> i & 1]
        found = False
        for size in range(1, min(4, len(bits)) + 1):
            for sub in combinations(bits, size):
                r = in_conv_affine_independent([shell[i] for i in sub], c)
                if r:
                    found = True
                    break
            if found:
                break
        tab[mask] = found
    return tab

def popcount(m):
    return bin(m).count('1')

def expected_ranks(p, u, tab, q_min, h, general):
    full = (1 << u) - 1
    out = {}
    for k in range(1, p + u + 2):
        r = dict(present=False, adh=0, aih=0, inert=0, nslc=0, closed=0, cshell=0, cint=0,
                 rv=0, sc=0, ua=0, slots=0, comps=[])
        if k > p + u:
            out[k] = r
            continue
        r['present'] = True
        r['closed'] = full
        r['inert'] = int(k <= p or (k - p) <= q_min - 2)
        r['nslc'] = int(k > p and k - p > h)
        if k <= p:
            r['aih'] = 1
            r['comps'] = [(k, 0, full, [])]
            out[k] = r
            continue
        t = k - p
        if q_min == 2 and u >= 3 and t == 1 and not general:
            r['adh'] = 1
            r['rv'] = u
            r['comps'] = [(p, 1, full, [1 << b for b in range(u)])]
            out[k] = r
            continue
        verts = [m for m in range(1 << u) if not tab[m] and popcount(m) == t]
        cof = [m for m in range(1 << u) if not tab[m] and popcount(m) == t + 1]
        r['rv'] = len(verts); r['sc'] = len(cof); r['ua'] = len(cof) * t; r['slots'] = 2 * (1 << u)
        adj = {v: set() for v in verts}
        for a, b in combinations(verts, 2):
            un = a | b
            if popcount(un) == t + 1 and not tab[un]:
                adj[a].add(b); adj[b].add(a)
        seen, comps = set(), []
        for v in verts:
            if v in seen:
                continue
            stack, comp = [v], []
            seen.add(v)
            while stack:
                x = stack.pop(); comp.append(x)
                for y in adj[x]:
                    if y not in seen:
                        seen.add(y); stack.append(y)
            comp.sort()
            cover = 0
            for x in comp: cover |= x
            comps.append((p, comp[0], cover, comp))
        comps.sort(key=lambda c: c[1])
        r['comps'] = comps
        cover_all = 0
        for c in comps: cover_all |= c[2]
        r['cshell'] = full & ~cover_all
        r['cint'] = int(not comps and p != 0)
        out[k] = r
    return out

def parse_probe(text):
    tables, ranks, rejected = {}, {}, {}
    for line in text.splitlines():
        tok = line.split()
        if not tok: continue
        if tok[0] == 'table':
            i, tag, q, h = int(tok[1]), tok[2], int(tok[3]), int(tok[4])
            sup_idx = tok.index('supports'); con_idx = tok.index('contains')
            sups = [] if sup_idx + 1 == con_idx else [int(v) for v in tok[sup_idx + 1].split(',')]
            tables[(i, tag)] = dict(q=q, h=h, supports=sups, contains=[ch == '1' for ch in tok[con_idx + 1]])
        elif tok[0] == 'rank':
            i, tag, k = int(tok[1]), tok[2], int(tok[3])
            present, adh, aih, inert, nslc, closed, cshell, cint, rv, sc, ua, slots, ncomp = [int(v) for v in tok[4:17]]
            comps = []
            for spec in tok[17:]:
                ip, rep, cover, mem = spec.split(':')
                members = [] if mem == '-' else [int(v) for v in mem.split('.')]
                comps.append((int(ip), int(rep), int(cover), members))
            need(len(comps) == ncomp, 'probe_component_count')
            ranks[(i, tag, k)] = dict(present=bool(present), adh=adh, aih=aih, inert=inert, nslc=nslc, closed=closed,
                                      cshell=cshell, cint=cint, rv=rv, sc=sc, ua=ua, slots=slots, comps=comps)
        elif tok[0] == 'rejected':
            i = int(tok[1]); rejected[i] = (tok[2], tok[3])
    return tables, ranks, rejected

def main():
    rng = random.Random(20260910)
    shells = []  # (name, key, interior pts, shell pts, ids)
    for name, key in SPHERES.items():
        cen = center_of(key)
        if name.startswith('big'):
            box = ((25000, 35000), (25000, 35000), (25000, 35000))
            sp = [(30000 + 1000 * dx, 30000 + 1000 * dy, 30000 + 1000 * dz) for dx in range(-5, 6) for dy in range(-5, 6) for dz in range(-5, 6)
                  if dx * dx + dy * dy + dz * dz == 25]
            need(all(power(key, q) == 0 for q in sp), 'big_sphere_points')
            inter = [(30000 + 700 * dx, 30000 + 700 * dy, 30000 + 700 * dz) for dx in range(-3, 4) for dy in range(-3, 4) for dz in range(-3, 4)
                     if power(key, (30000 + 700 * dx, 30000 + 700 * dy, 30000 + 700 * dz)) < 0]
        elif name.startswith('third'):
            box = ((5, 16), (5, 16), (5, 16))
            sp = sphere_points(key, box); inter = interior_points(key, box)
        else:
            box = ((0, 11), (0, 11), (0, 11))
            sp = sphere_points(key, box); inter = interior_points(key, box)
        need(len(sp) >= 8, f'{name}: sphere points {len(sp)}')
        antipode = {}
        for q in sp:
            ap = tuple(2 * cen[k] - q[k] for k in range(3))
            if all(v.denominator == 1 for v in ap) and tuple(int(v) for v in ap) in set(sp):
                antipode[q] = tuple(int(v) for v in ap)
        umax = min(8, len(sp))
        for u in range(2, umax + 1):
            for mode in ('with_antipodes', 'no_antipodes'):
                trials = 0
                produced = 0
                while produced < (10 if u >= 3 else 3) and trials < 400:
                    trials += 1
                    if mode == 'no_antipodes':
                        chosen = []
                        pool = sp[:]; rng.shuffle(pool)
                        for q in pool:
                            if len(chosen) == u: break
                            if antipode.get(q) in chosen: continue
                            chosen.append(q)
                        if len(chosen) < u: break
                    else:
                        if not antipode: break
                        q0 = rng.choice(list(antipode))
                        chosen = [q0, antipode[q0]]
                        pool = [q for q in sp if q not in chosen]; rng.shuffle(pool)
                        chosen += pool[:u - 2]
                        if len(chosen) < u: break
                    p = rng.choice([0, 0, 1, 2, 3]) if inter else 0
                    ints = rng.sample(inter, min(p, len(inter)))
                    ids = rng.sample(range(1, 2**32 - 1), len(ints) + u)
                    shells.append((name, mode, key, ints, chosen, ids))
                    produced += 1
    # a few u=2 antipodal and u=2 non-antipodal (must be rejected: shell does not define miniball)
    key = SPHERES['int_center_5_r25']
    shells.append(('int_center_5_r25', 'u2_reject', key, [], [(10, 5, 5), (5, 10, 5)], [7, 3]))
    shells.append(('int_center_5_r25', 'hemisphere_reject', key, [], [(10, 5, 5), (5, 10, 5), (8, 9, 5), (9, 8, 5)], [7, 3, 9, 1]))
    # build probe input
    lines = [str(len(shells))]
    for name, mode, key, ints, sh, ids in shells:
        a, b, c = key
        lines.append(f'{a} {b[0]} {b[1]} {b[2]} {c} {len(ints)} {len(sh)}')
        for pt, i in zip(ints + sh, ids):
            lines.append(f'{i} {pt[0]} {pt[1]} {pt[2]}')
    inp = '\n'.join(lines) + '\n'
    open(sys.argv[2], 'w').write(inp)
    proc = subprocess.run([sys.argv[1]], input=inp, capture_output=True, text=True)
    need(proc.returncode == 0, f'probe exit {proc.returncode}: {proc.stderr[:500]}')
    tables, ranks, rejected = parse_probe(proc.stdout)
    stats = Counter(); rank_cmp = 0; t1_cmp = 0; semantic_eq = 0; tab_cmp = 0
    profile = Counter()
    for idx, (name, mode, key, ints, sh, ids) in enumerate(shells):
        cen = center_of(key); r2 = r2_of(key)
        need(r2 > 0, 'positive radius')
        need(all(power(key, q) == 0 for q in sh) and all(power(key, q) < 0 for q in ints), 'census')
        p, u = len(ints), len(sh)
        # sort by id as the header does
        int_sorted = [pt for pt, i in sorted(zip(ints, ids[:p]), key=lambda z: z[1])]
        sh_sorted = [pt for pt, i in sorted(zip(sh, ids[p:]), key=lambda z: z[1])]
        tab = contains_table(sh_sorted, cen)
        if not tab[-1]:
            need(idx in rejected and rejected[idx] == ('nominal=local.shell_defines_miniball', 'general=local.shell_defines_miniball'),
                 f'shell {idx} should be rejected: {rejected.get(idx)}')
            stats['rejected_hemisphere'] += 1
            continue
        need(idx not in rejected, f'shell {idx} unexpectedly rejected {rejected.get(idx)}')
        supports = sorted((m for m in range(1, 1 << u) if tab[m] and all(not tab[m ^ (1 << b)] for b in range(u) if m >> b & 1)), key=lambda m: (popcount(m), m))
        q_min = min(popcount(m) for m in supports)
        h = max(popcount(m) for m in range(1 << u) if not tab[m])
        n_antipodal = sum(1 for a_, b_ in combinations(sh_sorted, 2) if all(a_[k] + b_[k] == 2 * cen[k] for k in range(3)))
        need((q_min == 2) == (n_antipodal > 0), 'q_min2_iff_antipodal_pair')
        for tag, general in (('nominal', False), ('general', True)):
            T = tables[(idx, tag)]
            need(T['q'] == q_min and T['h'] == h and T['supports'] == supports and T['contains'] == tab, f'table mismatch {idx} {tag}')
            tab_cmp += 1
            exp = expected_ranks(p, u, tab, q_min, h, general)
            for k in range(1, p + u + 2):
                got = ranks[(idx, tag, k)]
                for field in ('present', 'adh', 'aih', 'inert', 'nslc', 'closed', 'cshell', 'cint', 'rv', 'sc', 'ua', 'slots', 'comps'):
                    need(got[field] == exp[k][field], f'shell {idx} {name} {mode} {tag} K={k} field {field}: got {got[field]} expected {exp[k][field]}')
                rank_cmp += 1
                if k == p + 1:
                    t1_cmp += 1
        # nominal vs general semantic equality at every rank
        for k in range(1, p + u + 2):
            gn, gg = ranks[(idx, 'nominal', k)], ranks[(idx, 'general', k)]
            for field in ('present', 'aih', 'inert', 'nslc', 'closed', 'cshell', 'cint', 'comps'):
                need(gn[field] == gg[field], f'nominal/general differ shell {idx} K={k} {field}')
            if k == p + 1:
                semantic_eq += 1
                if q_min == 2 and u >= 3:
                    need(gn['adh'] == 1 and gg['adh'] == 0 and gn['rv'] == u and gn['sc'] == 0 and gn['ua'] == 0 and gn['slots'] == 0, 'shortcut counters')
                    need(gg['rv'] == u and gg['sc'] == u * (u - 1) // 2 - n_antipodal and gg['ua'] == gg['sc'] and gg['slots'] == 2 * (1 << u), 'general counters t=1')
                    stats['t1_shortcut_vs_general_identical'] += 1
                else:
                    need(gn['adh'] == 0 and gg['adh'] == 0, 'no shortcut')
                    stats['t1_general_only'] += 1
        stats['shells'] += 1
        profile[(name, mode, u, p, q_min, n_antipodal)] += 1
    summary = dict(shells_accepted=stats['shells'], shells_rejected_hemisphere=stats['rejected_hemisphere'],
                   table_comparisons=tab_cmp, rank_comparisons_vs_python=rank_cmp,
                   t1_comparisons_vs_python=t1_cmp, t1_nominal_vs_general_identical=semantic_eq,
                   t1_shortcut_cases=stats['t1_shortcut_vs_general_identical'], t1_general_only_cases=stats['t1_general_only'],
                   by_u=dict(Counter(u for (_, _, u, _, _, _), n in profile.items() for _ in range(n))),
                   by_qmin=dict(Counter(q for (_, _, _, _, q, _), n in profile.items() for _ in range(n))),
                   by_p=dict(Counter(p for (_, _, _, p, _, _), n in profile.items() for _ in range(n))),
                   by_antipodal_pairs=dict(Counter(a for (_, _, _, _, _, a), n in profile.items() for _ in range(n))),
                   by_sphere=dict(Counter(s for (s, _, _, _, _, _), n in profile.items() for _ in range(n))),
                   status='passed')
    print(json.dumps(summary, sort_keys=True))

if __name__ == '__main__':
    main()
