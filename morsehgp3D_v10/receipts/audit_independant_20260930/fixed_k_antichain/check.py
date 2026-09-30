"""Bounded exact structural audit, archived native incidences, no engine execution."""
from fractions import Fraction as F
import hashlib
from itertools import combinations
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT/'source_snapshot'))
import frontier_core as fc
import arms
import cover_band as original
import antichain as reduced


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


class Tree:
    def __init__(self, levels, parents):
        self.levels = tuple(F(x) for x in levels)
        self.parent = tuple(parents)
        self.children = [[] for _ in parents]
        for v, p in enumerate(parents):
            if p != -1:
                need(self.levels[p] >= self.levels[v], 'nonmonotone tree')
                self.children[p].append(v)

    def __len__(self):
        return len(self.parent)

    def level(self, v):
        return self.levels[v]

    def ancestor(self, v, beta, closed=True):
        need(closed, 'only closed cuts used')
        if self.level(v) > beta:
            return None
        while self.parent[v] != -1 and self.level(self.parent[v]) <= beta:
            v = self.parent[v]
        return v

    def is_ancestor(self, a, v):
        while v != -1:
            if a == v:
                return True
            v = self.parent[v]
        return False

    def lca(self, a, b):
        ancestors = set()
        while a != -1:
            ancestors.add(a)
            a = self.parent[a]
        while b != -1:
            if b in ancestors:
                return b
            b = self.parent[b]
        return None


class Context:
    def __init__(self, forest, alphas, witnesses):
        self.forest, self.alphas, self.data = forest, tuple(alphas), tuple(witnesses)
        self.n = len(alphas)

    def cover_level(self, s):
        return self.alphas[s]

    def witnesses(self):
        return self.data


def selected(ctx, s, eta):
    return [v for _, beta, v in ctx.witnesses()[s]
            if beta <= (1+eta)**2*ctx.cover_level(s)]


def check_minima(ctx, value, eta):
    f = ctx.forest
    tin, tout = reduced.euler(f)
    for s in range(ctx.n):
        nodes = set(selected(ctx, s, eta))
        oracle = tuple(sorted((v for v in nodes if not any(
            w != v and f.is_ancestor(v, w) for w in nodes)),key=lambda v:tin[v]))
        need(value.minima[s] == oracle, 'Euler antichain differs from pairwise oracle')
        first = {v for _, beta, v in ctx.witnesses()[s] if beta == ctx.cover_level(s)}
        need(first <= set(value.minima[s]), 'first-cover node removed')
        need(value.dates[s] >= ctx.cover_level(s), 'entry precedes cover')
        need(all(f.is_ancestor(value.nodes[s], v) for v in first), 'entry lacks first-cover ancestry')
        need(f.ancestor(value.nodes[s], value.dates[s], True) == value.nodes[s], 'dead entry owner')


def abstract_checks():
    count, contexts = 0, 0
    shapes = [Tree([1,2,4],[2,2,-1]),
              Tree([1,1,4,6,9],[2,2,4,4,-1]),
              Tree([1,1,4,4,9],[2,2,3,4,-1])]
    etas = (F(0),F(1,4),F(1,2),F(1),F(2),F(4))
    for f in shapes:
        dates = sorted(set(f.levels))
        dates = sorted(set(dates + [(a+b)/2 for a,b in zip(dates,dates[1:])] + [dates[-1]+1]))
        pool = [(i,beta,v) for i,(beta,v) in enumerate(
            (beta,v) for beta in dates for v in range(len(f)) if f.ancestor(v,beta) == v)]
        for size in (1,2,3):
            for entries in combinations(pool,size):
                contexts += 1
                ctx = Context(f,[min(beta for _,beta,_ in entries)],[entries])
                previous = None
                for eta in etas:
                    a = reduced.cover_band_antichain(ctx,eta)
                    b = original.cover_band_lca(ctx,eta)
                    check_minima(ctx,a,eta)
                    need(a.dates[0] <= b.dates[0], 'reduced entry later than original')
                    need(f.is_ancestor(b.nodes[0],a.nodes[0]), 'original not ancestor of reduction')
                    if eta == 0:
                        need(a.dates == b.dates and a.nodes == b.nodes, 'eta0 changed')
                    if previous is not None:
                        need(previous.dates[0] <= a.dates[0], 'eta delayed-entry monotonicity failed')
                        need(set(previous.minima[0]) <= set(a.minima[0]), 'eta removed a minimal lineage')
                    reverse = Context(f,ctx.alphas,[tuple(reversed(entries))])
                    c = reduced.cover_band_antichain(reverse,eta)
                    need((a.dates,a.nodes,a.minima) == (c.dates,c.nodes,c.minima), 'permutation changed result')
                    duplicate = Context(f,ctx.alphas,[entries+entries])
                    d = reduced.cover_band_antichain(duplicate,eta)
                    need((a.dates,a.nodes,a.minima) == (d.dates,d.nodes,d.minima), 'duplicates changed result')
                    previous = a
                    count += 1
    # Equal-level child is not alive on the closed plateau; its parent is.
    plateau = shapes[2]
    bad = Context(plateau,[F(4)],[[(0,F(4),2)]])
    for method,error in ((original.cover_band_lca,original.BandError),
                         (reduced.cover_band_antichain,reduced.ReductionError)):
        try:
            method(bad,F(0))
        except error:
            pass
        else:
            raise RuntimeError('dead plateau witness accepted')
    good = Context(plateau,[F(1)],[[(0,F(1),0),(1,F(1),1)]])
    g = reduced.cover_band_antichain(good,F(0))
    need(g.dates == (F(4),) and g.nodes == (3,), 'plateau normalization failed')
    # A unary subdivision alters original dated-node LCA, while reduction contracts it.
    base = Tree([1,2,4],[2,2,-1])
    split = Tree([1,2,4,3],[3,2,-1,2])
    oldctx = Context(base,[F(1)],[[(0,F(1),0),(1,F(7,2),0)]])
    newctx = Context(split,[F(1)],[[(0,F(1),0),(1,F(7,2),3)]])
    oa,ob = original.cover_band_lca(oldctx,F(1)),original.cover_band_lca(newctx,F(1))
    ra,rb = reduced.cover_band_antichain(oldctx,F(1)),reduced.cover_band_antichain(newctx,F(1))
    need(oa.dates == (F(1),) and ob.dates == (F(3),), 'unary control absent')
    need(ra.dates == rb.dates == (F(1),) and ra.nodes == rb.nodes == (0,), 'unary invariance failed')
    # Disconnected roots have no finite LCA: do not fabricate a finite owner/date.
    disjoint = Tree([1,1],[-1,-1])
    ctx = Context(disjoint,[F(1)],[[(0,F(1),0),(1,F(1),1)]])
    try:
        reduced.cover_band_antichain(ctx,F(0))
    except reduced.ReductionError as error:
        need('different roots' in str(error), 'wrong disconnected refusal')
    else:
        raise RuntimeError('disconnected roots silently joined')
    for eta in (True,0.125,F(-1)):
        try:
            reduced.cover_band_antichain(good,eta)
        except reduced.ReductionError:
            pass
        else:
            raise RuntimeError('invalid eta accepted')
    return {'contexts':contexts,'eta_cases':count,'unary_original_dates':['1','3'],
            'unary_reduced_dates':['1','1'],'closed_plateau_owner':3,'distinct_roots':'explicit refusal'}


def native_checks():
    rows, checks = [], 0
    etas = (F(0),F(1,64),F(1,32),F(1,8),F(1,4),F(1),F(4))
    for path in sorted((ROOT/'fixtures').glob('*.json')):
        export = fc.load_export(path)
        ctx = arms.ArmContext(export)
        witnesses = ctx.witnesses()
        expected = [[] for _ in range(ctx.n)]
        for ball in export.balls:
            if ball.population >= ctx.k and ball.p+ball.q <= ctx.k:
                for s in ball.I+ball.U:
                    expected[s].append((ball.index,ball.level,ctx.order.ball_node[ball.index]))
        need(witnesses == expected, 'incomplete strong interior/shell incidences')
        all_versions, stats = [], []
        for eta in etas:
            a = reduced.cover_band_antichain(ctx,eta)
            b = original.cover_band_lca(ctx,eta)
            check_minima(ctx,a,eta)
            need(all(x <= y for x,y in zip(a.dates,b.dates)), 'later reduced native entry')
            if eta == 0:
                need(a.dates == b.dates and a.nodes == b.nodes, 'native eta0 changed')
            reverse = Context(ctx.forest,[ctx.cover_level(s) for s in range(ctx.n)],
                              [tuple(reversed(w)) for w in witnesses])
            c = reduced.cover_band_antichain(reverse,eta)
            need((a.dates,a.nodes,a.minima) == (c.dates,c.nodes,c.minima), 'native permutation changed')
            aa,bb = fc.Attachments(ctx.forest,a.dates,a.nodes),fc.Attachments(ctx.forest,b.dates,b.nodes)
            need(tuple(aa.nodes) == a.nodes and tuple(aa.dates) == a.dates, 'non-neutral reduced attachment')
            all_versions.append((a,b,aa,bb))
            stats.append({'eta':str(eta),'original_dates':[str(x) for x in b.dates],
                          'reduced_dates':[str(x) for x in a.dates],
                          'reduced_nodes':list(a.nodes),'minima':[list(x) for x in a.minima],
                          'advanced_sites':sum(x<y for x,y in zip(a.dates,b.dates)),
                          'removed_distinct_nodes':a.removed_distinct_nodes})
        cuts = sorted({F(0),max(ctx.forest.levels)+1}|set(ctx.forest.levels)|
                      {t for a,b,_aa,_bb in all_versions for t in a.dates+b.dates})
        for i,(a,b,aa,bb) in enumerate(all_versions):
            previous = None
            for beta in cuts:
                current = aa.blocks(beta)
                need(fc.nested(bb.blocks(beta),current), 'original does not refine reduction')
                if previous is not None:
                    need(fc.nested(previous,current), 'reduction is not laminar in radius')
                previous = current
                if i:
                    prior = all_versions[i-1][2]
                    need(fc.nested(current,prior.blocks(beta)), 'larger eta does not refine smaller')
                for s in range(ctx.n):
                    if beta >= b.dates[s]:
                        need(aa.label(s,beta) == bb.label(s,beta), 'lineage differs after original entry')
                checks += 1
        x = ctx.point_id.index(0)
        if path.stem == 'internal_k3':
            a = all_versions[2][0]
            need(a.dates[x] == 25 and ctx.forest.birth[a.nodes[x]] == -1, 'lost internal K3 owner')
        if path.stem == 'internal_k5':
            a = all_versions[2][0]
            need(a.dates[x] == 105625 and ctx.forest.birth[a.nodes[x]] == -1, 'lost internal K5 owner')
        if path.stem.startswith('near_tie'):
            a,b = all_versions[2][:2]
            need(a.dates[x] == b.dates[x] and len(a.minima[x]) == 2, 'genuine near-tie conflict removed')
        if path.stem == 'triangle':
            a,b = all_versions[5][:2]
            need(b.dates == (F(13,4),)*3 and a.dates == (F(13,4),F(1),F(9,4)), 'triangle mechanism failed')
            need(len(a.minima[0]) == 2 and len(a.minima[1]) == len(a.minima[2]) == 1,
                 'triangle independent/ancestral lineages confused')
        if path.stem == 'triangle_default':
            a,b = all_versions[3][:2]  # eta = 1/8
            target = ctx.point_id.index(1)  # (8,0,0), independent of Morton ordering
            need(ctx.cover_level(target) == 16 and b.dates[target] == F(73,4)
                 and a.dates[target] == 16 and len(a.minima[target]) == 1,
                 'default-eta ancestral delay was not removed')
            need(F(81,4) == F(81,64)*ctx.cover_level(target), 'wrong squared-radius band units')
        if path.stem.startswith('triangle'):
            # Independent exact Gamma oracle: every pair is a vertex; the only
            # Johnson union is the right triangle. Its MEB is the hypotenuse
            # diameter: containment attains the pair-distance lower bound.
            points = {pid:export.sites[s] for s,pid in enumerate(ctx.point_id)}
            pair_levels = {f:F(sum((points[f[0]][j]-points[f[1]][j])**2
                                  for j in range(3)),4) for f in combinations(range(3),2)}
            join = max(pair_levels.values())
            need(pair_levels[(1,2)] == pair_levels[(0,1)]+pair_levels[(0,2)],
                 'triangle is not right at point0')
            for beta in sorted({F(0)}|set(pair_levels.values())|{join+1}):
                active = {f for f,t in pair_levels.items() if t <= beta}
                blocks = [active] if beta >= join else [{f} for f in sorted(active)]
                mapping = {f:i for i,block in enumerate(blocks) for f in block}
                signatures = []
                for v in ctx.forest.components(beta):
                    signature = set()
                    for leaf in ctx.forest.births_under(v):
                        ball = export.balls[ctx.forest.birth[leaf]]
                        face = tuple(sorted(ctx.point_id[s] for s in ball.I+ball.U))
                        signature.add(mapping[face])
                    need(len(signature) == 1, 'triangle native/Gamma component mismatch')
                    signatures.extend(signature)
                need(sorted(signatures) == list(range(len(blocks))), 'triangle native/Gamma bijection failed')
                if pair_levels[(0,1)] <= beta < join:
                    need(sum(any(1 in face for face in block) for block in blocks) == 1,
                         'triangle target has an independent covering competitor before fusion')
                checks += 1
        rows.append({'case':path.stem,'K':ctx.k,'point_ids':ctx.point_id,
                     'strong_incidences':sum(map(len,witnesses)),'qmin_recomputed':ctx.qmin_checked,
                     'variants':stats})
    return rows,checks


def main():
    paths = sorted([Path(__file__).resolve(),ROOT/'antichain.py']+
                   list(ROOT.glob('capture_*.py'))+list(ROOT.glob('*_capture.json'))+
                   list((ROOT/'source_snapshot').glob('*.py'))+list((ROOT/'fixtures').glob('*')))
    before = {str(p.relative_to(ROOT)):sha(p) for p in paths}
    abstract = abstract_checks()
    rows, checks = native_checks()
    after = {str(p.relative_to(ROOT)):sha(p) for p in paths}
    need(before == after, 'consumer inputs changed')
    report = {'status':'PASS','abstract':abstract,'native_exports':rows,'native_cut_checks':checks,
              'sources_before':before,'sources_after':after,'optimize_flag':sys.flags.optimize,
              'GCP_used':False,'limits':'bounded consumer audit; archived exact native inputs; no statistics/head/performance claim'}
    out = ROOT/('optimized_receipt.json' if sys.flags.optimize else 'normal_receipt.json')
    need(not out.exists(), 'refuse to overwrite receipt')
    out.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'status':'PASS','abstract_contexts':abstract['contexts'],
                      'abstract_eta_cases':abstract['eta_cases'],'native_exports':len(rows),
                      'native_cut_checks':checks,'triangle_original':['13/4']*3,
                      'triangle_reduced':['13/4','1','9/4'],
                      'default_eta_target_original':'73/4','default_eta_target_reduced':'16'},sort_keys=True))


if __name__ == '__main__':
    main()
