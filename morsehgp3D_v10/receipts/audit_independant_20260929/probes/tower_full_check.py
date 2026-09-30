"""Audit borne : chaque naissance/verticale et relation de couverture contre toutes les K-parties.

Usage : python3 tower_full_check.py PROBE SOURCE_V10 [count=20]
Sortie 0 conforme, 1 ecart. Fraction exhaustive, petits nuages uniquement.
"""
import itertools
import json
import os
import random
import struct
import subprocess
import sys
import tempfile
from fractions import Fraction

sys.path.insert(0, os.path.join(sys.argv[2], "reference"))
import hgp10_ref as R  # noqa: E402


def parse(text):
    P, modes = [], {}
    mode = None
    for line in text.splitlines():
        t = line.split()
        if t[0] == "site":
            P.append(tuple(map(int, t[2:5])))
        elif t[0] == "mode":
            mode = modes.setdefault(t[1], {})
        elif t[0] == "error":
            raise RuntimeError(line)
        else:
            k = int(t[1])
            o = mode.setdefault(k, {"nodes": [], "points": [], "balls": []})
            if t[0] == "node":
                o["nodes"].append({"parent": int(t[3]), "level": Fraction(int(t[4]), int(t[5])),
                                   "lower": int(t[6]), "F": tuple(sorted(map(int, t[7:])))})
            elif t[0] == "point":
                o["points"].append((int(t[2]), int(t[3]), Fraction(int(t[4]), int(t[5]))))
            elif t[0] == "ball":
                o["balls"].append((int(t[2]), int(t[3]), Fraction(int(t[4]), int(t[5])),
                                   tuple(map(int, t[6:]))))
    return P, modes


def ancestor(nodes, v, a, closed=True):
    while nodes[v]["parent"] >= 0:
        p = nodes[v]["parent"]
        if nodes[p]["level"] > a or (not closed and nodes[p]["level"] == a):
            break
        v = p
    return v


def check(P, modes, counts):
    n = len(P)
    kmax = max(modes["core"])
    beta = {F: R.meb(P, F)[0] for q in range(1, min(n, kmax + 1) + 1)
            for F in itertools.combinations(range(n), q)}
    snapshots = {}
    for k in range(1, kmax + 1):
        levels = sorted({b for F, b in beta.items() if len(F) in (k, k + 1)})
        snapshots[k] = {}
        for a in levels:
            for closed in (False, True):
                dsu = R.DSU()
                alive = []
                ok = lambda b: b <= a if closed else b < a  # noqa: E731
                for F, b in beta.items():
                    if len(F) == k and ok(b):
                        dsu.find(F)
                        alive.append(F)
                for G, b in beta.items():
                    if len(G) == k + 1 and ok(b):
                        fs = [tuple(x for x in G if x != u) for u in G]
                        for F in fs[1:]:
                            dsu.union(fs[0], F)
                roots = {F: dsu.find(F) for F in alive}
                snapshots[k][(a, closed)] = roots
                # Topologie sur les minima : juge aussi les composantes sans points core.
                nodes = modes["core"][k]["nodes"]
                got, expected = {}, {}
                for v, nd in enumerate(nodes):
                    if not nd["F"] or not ok(nd["level"]):
                        continue
                    root = ancestor(nodes, v, a, closed)
                    got.setdefault(root, set()).add(v)
                    expected.setdefault(roots[nd["F"]], set()).add(v)
                if set(map(frozenset, got.values())) != set(map(frozenset, expected.values())):
                    raise RuntimeError("birth partition k=%d a=%s closed=%s" % (k, a, closed))
                if len(got) != len(set(roots.values())):
                    raise RuntimeError("unrepresented Gamma component k=%d a=%s" % (k, a))
                counts["cuts_births"] += 1

    def root_at(k, F, a):
        # Les niveaux traverses des verticales peuvent venir d'un autre ordre : prendre derniere coupe.
        levels = [lv for lv, closed in snapshots[k] if closed and lv <= a]
        return snapshots[k][(max(levels), True)][F]

    for mode, orders in modes.items():
        for k, o in orders.items():
            nodes = o["nodes"]
            births = [(v, nd["F"]) for v, nd in enumerate(nodes) if nd["F"]]

            def node_gamma(v, a):
                seeds = [F for b, F in births if nodes[b]["level"] <= a and ancestor(nodes, b, a) == v]
                if not seeds:
                    raise RuntimeError("node without Gamma birth")
                rs = {root_at(k, F, a) for F in seeds}
                if len(rs) != 1:
                    raise RuntimeError("node with disconnected Gamma births")
                return next(iter(rs))

            for v, nd in enumerate(nodes):
                if k < 2:
                    continue
                a = nd["level"]
                top_gamma = node_gamma(v, a)
                Fu = next(F for b, F in births if nodes[b]["level"] <= a
                          and ancestor(nodes, b, a) == v and root_at(k, F, a) == top_gamma)
                G = Fu[:-1]
                want = root_at(k - 1, G, a)
                dn = orders[k - 1]["nodes"]
                image = ancestor(dn, nd["lower"], a)
                bs = [bd["F"] for b, bd in enumerate(dn) if bd["F"] and bd["level"] <= a
                      and ancestor(dn, b, a) == image]
                if not bs or {root_at(k - 1, F, a) for F in bs} != {want}:
                    raise RuntimeError("vertical mismatch %s k=%d node=%d" % (mode, k, v))
                counts["vertical_nodes"] += 1
                if nd["F"] and not any(pv == v and e == a for _, pv, e in o["points"]):
                    counts["births_no_attached_point_at_birth_" + mode] += 1
            for b, v, a, pop in o["balls"]:
                F = tuple(sorted(pop[:k]))
                if node_gamma(v, a) != root_at(k, F, a):
                    raise RuntimeError("ball node mismatch k=%d ball=%d" % (k, b))
                # Toutes les K-parties de la boule et tous leurs points couverts appartiennent a ce Gamma root.
                for F in itertools.combinations(sorted(pop), k):
                    if root_at(k, F, a) != node_gamma(v, a):
                        raise RuntimeError("ball quotient mismatch k=%d ball=%d" % (k, b))
                    counts["covering_k_parts"] += 1
            if mode == "cover":
                for x, v, a in o["points"]:
                    want = min(b for F, b in beta.items() if len(F) == k and x in F)
                    if a != want:
                        raise RuntimeError("alpha mismatch k=%d site=%d" % (k, x))
                    F = next(F for F, b in beta.items() if len(F) == k and x in F and b == a)
                    if node_gamma(v, a) != root_at(k, F, a):
                        raise RuntimeError("cover component mismatch k=%d site=%d" % (k, x))
                    counts["alpha_exact"] += 1


def main():
    rnd = random.Random(2026092901)
    clouds = [[(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)],
              list(itertools.product((0, 2), repeat=3)),
              [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)],
              [(x, 0, 0) for x in (0, 1, 2, 5, 8, 13, 21)],
              [(10 + x, 10 + y, 0) for x, y in ((5, 0), (3, 4), (0, 5), (-3, 4), (-5, 0), (0, -5))],
              [(10 + x, 10 + y, 10 + z) for x, y, z in
               ((5, 0, 0), (-5, 0, 0), (0, 5, 0), (0, -5, 0), (0, 0, 5), (0, 0, -5))]]
    for t in range(int(sys.argv[3]) if len(sys.argv) > 3 else 20):
        n = rnd.randint(5, 8)
        pts = set()
        while len(pts) < n:
            lim = 3 if t % 4 == 0 else 1000
            p = tuple(rnd.randint(0, lim) for _ in range(3))
            if t % 4 == 1:
                p = p[:2] + (0,)
            if t % 4 == 2:
                p = tuple(262143 - x for x in p)
            pts.add(p)
        clouds.append(sorted(pts))
    counts = dict(clouds=0, cuts_births=0, vertical_nodes=0, covering_k_parts=0, alpha_exact=0,
                  births_no_attached_point_at_birth_core=0, births_no_attached_point_at_birth_cover=0)
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "in.u32le")
        for i, pts in enumerate(clouds):
            with open(src, "wb") as f:
                for p in pts:
                    f.write(struct.pack("<III", *p))
            runs = [subprocess.run([sys.argv[1], src, str(min(10, len(pts))), str(w)],
                                   capture_output=True, text=True, timeout=20) for w in (1, 4)]
            if any(r.returncode for r in runs):
                raise RuntimeError("cloud %d refused %s" % (i, [r.stdout for r in runs]))
            if runs[0].stdout != runs[1].stdout:
                raise RuntimeError("cloud %d differs at 1 and 4 threads" % i)
            P, modes = parse(runs[0].stdout)
            check(P, modes, counts)
            counts["clouds"] += 1
    print(json.dumps(dict(status="ok", **counts), sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
