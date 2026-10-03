#!/usr/bin/env python3
"""C4 : recherches aleatoires independantes (vfull).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c4_random.py SEED NCLOUDS
  (a) domination P1 <= Q1 : dates (rayon), meme ancre, hauteurs u, pour m = 1 et m = k + 1 ;
  (b) bornes post-coeur : P_kappa (kappa 1, 2) e <= alpha + d_k/2, e <= d_k a K = 2, max e/d_k ;
      Q1 et P1 o Pi_{k+1} : max e/d_k ;
  (c) proposition I : pour tout niveau d'evenement F et tout site avec d_k <= F et alpha + d_k/2 <= F,
      P_kappa (kappa 1, 2) a fait entrer x avant F et son ancre est dans la composante de L_k(F) qui contient x.
"""
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import mpmath  # noqa: E402
import vfull as V  # noqa: E402


def cloud(rng):
    n = rng.randint(4, 8)
    k = rng.randint(2, min(4, n - 1))
    box = rng.choice([4, 6, 10, 50, 1000])
    pts = set()
    flat = rng.random() < 0.3
    while len(pts) < n:
        pts.add((rng.randint(0, box), rng.randint(0, box), 0 if flat else rng.randint(0, box)))
    return sorted(pts), k


def main():
    seed, count = int(sys.argv[1]), int(sys.argv[2])
    rng = random.Random(seed)
    st = {'clouds': 0, 'sites': 0, 'dom_date_viol': 0, 'dom_anchor_viol': 0, 'dom_u_viol': 0, 'dom_strict': 0,
          'dom_pairs': 0, 'bound_viol': {}, 'after_core_K2': {}, 'max_e_over_dk': {}, 'propI_checks': 0,
          'propI_viol': 0, 'examples': []}
    t0 = time.time()
    for _ in range(count):
        pts, k = cloud(rng)
        n = len(pts)
        cl = V.Cloud(pts)
        full = V.Full(cl, k)
        st['clouds'] += 1
        dk = [V.R(cl.dk2(x, k)) for x in range(n)]
        alpha = [V.R(min(a for a, _ in full.entries(x, 1))) for x in range(n)]
        for m in sorted(set([1, k + 1])):
            p1, q1 = V.rule_P(full, 1, m), V.rule_Q(full, 1, m)
            for x in range(n):
                st['sites'] += 1
                if p1.e[x] > q1.e[x] + V.TOL:
                    st['dom_date_viol'] += 1
                if p1.e[x] < q1.e[x] - V.TOL:
                    st['dom_strict'] += 1
                if p1.anchor[x] != q1.anchor[x]:
                    st['dom_anchor_viol'] += 1
                for z in range(x + 1, n):
                    st['dom_pairs'] += 1
                    if p1.u(x, z) > q1.u(x, z) + V.TOL:
                        st['dom_u_viol'] += 1
        rules = {'P1': V.rule_P(full, 1, 1), 'P2': V.rule_P(full, 2, 1), 'Q1': V.rule_Q(full, 1, 1),
                 'P1_qual': V.rule_P(full, 1, k + 1)}
        for name, r in rules.items():
            key = '%s_K%d' % (name, k)
            for x in range(n):
                ratio = r.e[x] / dk[x]
                if ratio > st['max_e_over_dk'].get(key, 0):
                    st['max_e_over_dk'][key] = float(ratio)
                if name in ('P1', 'P2'):
                    if r.e[x] > alpha[x] + dk[x] / 2 + V.TOL:
                        st['bound_viol'][key] = st['bound_viol'].get(key, 0) + 1
                        if len(st['examples']) < 5:
                            st['examples'].append({'pts': pts, 'k': k, 'x': x, 'rule': name})
                    if k == 2 and r.e[x] > dk[x] + V.TOL:
                        st['after_core_K2'][name] = st['after_core_K2'].get(name, 0) + 1
        # proposition I
        levels = sorted(set(full.level))
        for kap in (1, 2):
            r = rules['P%d' % kap]
            for F2 in levels:
                Fr = V.R(F2)
                for x in range(n):
                    if dk[x] <= Fr + V.TOL and alpha[x] + dk[x] / 2 <= Fr + V.TOL:
                        st['propI_checks'] += 1
                        F0 = cl.knn_part(x, k)
                        ok = r.e[x] <= Fr + V.TOL and full.comp_at(r.anchor[x], F2) == full.comp_at(F0, F2)
                        if not ok:
                            st['propI_viol'] += 1
                            if len(st['examples']) < 10:
                                st['examples'].append({'pts': pts, 'k': k, 'x': x, 'F2': str(F2), 'kappa': kap})
    st['seconds'] = round(time.time() - t0, 1)
    with open('recus_c4_random_%d.json' % seed, 'w') as fh:
        json.dump(st, fh, indent=1)
    print(json.dumps(st, indent=1))


if __name__ == '__main__':
    main()
