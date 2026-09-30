"""Juge d'echelle K=1 : tour FULL d'ordre 1 (export natif) contre EMST exact (Prim entier O(n^2), memoire O(n)).
Compare (a) le multiensemble des niveaux de fusion (beta, multiplicite enfants-1) a {d^2/4} des aretes EMST,
(b) les partitions a des coupes tirees. Aucune assertion Python : codes de retour explicites."""
import json, subprocess, sys, time
from fractions import Fraction
import numpy as np

def prim_sq(p):
    n = len(p); p = p.astype(np.int64)
    best = np.full(n, np.iinfo(np.int64).max, dtype=np.int64); parent = np.full(n, -1, dtype=np.int64)
    used = np.zeros(n, dtype=bool); cur = 0; used[0] = True; edges = []
    for _ in range(n - 1):
        d = ((p - p[cur]) ** 2).sum(axis=1)
        upd = (~used) & (d < best); best[upd] = d[upd]; parent[upd] = cur
        cand = np.where(used, np.iinfo(np.int64).max, best); nxt = int(np.argmin(cand))
        edges.append((int(best[nxt]), int(parent[nxt]), nxt)); used[nxt] = True; cur = nxt
    return edges

def main(path, binary, workers):
    pts = np.frombuffer(open(path, 'rb').read(), dtype='<u4').reshape(-1, 3)
    t0 = time.time()
    out = subprocess.run([binary, '--input', path, '--k', '1', '--workers', str(workers)], capture_output=True, text=True)
    if out.returncode != 0: print('export refused', out.stderr[:300]); return 2
    t_export = time.time() - t0
    rep = json.loads(out.stdout)['native']
    nodes = rep['nodes']; pops = rep['populations']
    tower = []
    for nd in nodes:
        if nd['children']:
            beta = Fraction(int(nd['level']['num']), int(nd['level']['den']))
            tower += [beta * 4] * (len(nd['children']) - 1)
    t1 = time.time(); mst = prim_sq(pts); t_prim = time.time() - t1
    a = sorted(tower); b = sorted(Fraction(e[0]) for e in mst)
    ok_multiset = (a == b)
    # partitions at sampled cuts: product forest vs Kruskal on MST edges
    n = len(pts); leaf_point = {}
    for nd in nodes:
        if not nd['children']:
            pop = pops[nd['id']] if nd['id'] < len(pops) else None
    # leaves: node ids < n, population shell holds the point id
    leaf_point = {i: pops[i]['shell'][0] for i in range(n)}
    parent_of = {}
    for nd in nodes:
        for c in nd['children']: parent_of[c] = nd['id']
    level = {nd['id']: Fraction(int(nd['level']['num']), int(nd['level']['den'])) for nd in nodes}
    rng = np.random.default_rng(7); cuts = sorted(set(rng.choice(b, size=min(12, len(b)), replace=False)))
    bad_cuts = 0
    for t in cuts:
        # product: root of each leaf among nodes with 4*level <= t (closed cut)
        def root(x):
            while x in parent_of and level[parent_of[x]] * 4 <= t: x = parent_of[x]
            return x
        prod = {}
        for i in range(n): prod.setdefault(root(i), set()).add(leaf_point[i])
        P = sorted(sorted(s) for s in prod.values())
        uf = list(range(n))
        def f(x):
            while uf[x] != x: uf[x] = uf[uf[x]]; x = uf[x]
            return x
        for d, u, v in mst:
            if d <= t: uf[f(u)] = f(v)
        ref = {}
        for i in range(n): ref.setdefault(f(i), set()).add(i)
        R = sorted(sorted(s) for s in ref.values())
        bad_cuts += (P != R)
    print(f'k1_emst_judge n={n} merges={len(a)} mst_edges={len(b)} multiset_equal={ok_multiset} '
          f'cuts={len(cuts)} bad_cuts={bad_cuts} export_s={t_export:.2f} prim_s={t_prim:.2f}')
    return 0 if ok_multiset and bad_cuts == 0 else 1

if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2], int(sys.argv[3])))
