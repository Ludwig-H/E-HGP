"""Independent condensation from strict ultrametric cuts, no sklearn/native calls.

At event d, all edges at distance d disappear atomically. Clusters smaller
than mcs die, one large child continues, multiple large children are born.
Fractions give exact lambda=1/d; input matrices already encode expZ.
"""
from fractions import Fraction as F
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True


def require(ok, message):
    if not ok:
        raise ValueError(message)


def partition(points, matrix, threshold):
    unseen, groups = set(points), []
    while unseen:
        start = min(unseen)
        unseen.remove(start)
        reached, frontier = {start}, [start]
        while frontier:
            u = frontier.pop()
            adjacent = {v for v in unseen if matrix[u][v] < threshold}
            unseen -= adjacent
            reached |= adjacent
            frontier.extend(sorted(adjacent))
        groups.append(reached)
    return groups


def oracle(row):
    matrix = [[F(str(v)) for v in line] for line in row['matrix']]
    n, mcs = len(matrix), row['mcs']
    require(mcs >= 2 and all(len(line) == n for line in matrix), 'matrix/mcs')
    require(all(matrix[i][i] == 0 and matrix[i][j] == matrix[j][i] and
                matrix[i][j] > 0 for i in range(n) for j in range(n) if i != j),
            'finite symmetric off-diagonal metric')
    clusters = [dict(parent=None, birth=F(0), stability=F(0))]
    live = {0: set(range(n))}
    exited = [None]*n

    def drop(points, cluster, lam):
        for x in points:
            require(exited[x] is None, 'point exited twice')
            exited[x] = cluster
        clusters[cluster]['stability'] += len(points)*(lam-clusters[cluster]['birth'])

    for distance in sorted({matrix[i][j] for i in range(n) for j in range(i)}, reverse=True):
        lam = 1/distance
        for c, points in sorted(tuple(live.items())):
            parts = partition(points, matrix, distance)
            if len(parts) == 1:
                continue
            del live[c]
            large = [part for part in parts if len(part) >= mcs]
            if len(large) < 2:
                keep = large[0] if large else set()
                drop(points-keep, c, lam)
                if keep:
                    live[c] = keep
            else:
                for part in parts:
                    if len(part) < mcs:
                        drop(part, c, lam)
                    else:
                        clusters[c]['stability'] += len(part)*(lam-clusters[c]['birth'])
                        child = len(clusters)
                        clusters.append(dict(parent=c, birth=lam, stability=F(0)))
                        live[child] = part
    require(not live and all(c is not None for c in exited), 'unfinished cut sweep')
    children = [[] for _ in clusters]
    for c, data in enumerate(clusters):
        if data['parent'] is not None:
            children[data['parent']].append(c)
    best, candidates = {}, set()
    for c in reversed(range(len(clusters))):
        sub = sum((best[k] for k in children[c]), F(0))
        if (c == 0 and not row['allow_single']) or (children[c] and sub > clusters[c]['stability']):
            best[c] = sub
        else:
            best[c] = clusters[c]['stability']
            candidates.add(c)
    selected = set()
    for c in sorted(candidates):
        ancestor = clusters[c]['parent']
        while ancestor is not None and ancestor not in candidates:
            ancestor = clusters[ancestor]['parent']
        if ancestor is None:
            selected.add(c)
    labels = {}
    for x, c in enumerate(exited):
        while c is not None and c not in selected:
            c = clusters[c]['parent']
        labels.setdefault(c, []).append(x)
    expected = dict(noise=labels.pop(None, []), clusters=sorted(labels.values()))
    return expected, [dict(parent=r['parent'],birth=str(r['birth']),stability=str(r['stability']))
                      for r in clusters]


def judge(root):
    reports = [json.loads((root/(mode+'.stdout')).read_text()) for mode in ('normal','optimized')]
    require(reports[0]['rows'] == reports[1]['rows'], 'paired fit rows')
    outcomes, seen = [], set()
    for row in reports[0]['rows']:
        key = row['extended'], row['factor'], row['z'], row['allow_single']
        require(key not in seen, 'duplicate configuration')
        seen.add(key)
        expected, clusters = oracle(row)
        require(expected == row['partition'], 'event oracle/sklearn disagreement')
        outcomes.append(dict(configuration=list(key),mcs=row['mcs'],n=row['n'],
                             exact_clusters=clusters,partition=expected))
    require(len(seen) == 16 and all(r['status']=='PASS' and r['actual_fit_calls']==16 for r in reports),
            'fit panel coverage')
    return dict(status='PASS',exact_configurations=16,paired_actual_fit_calls=32,
                new_fit_calls=0,native_calls=0,GCP_used=False,observations=outcomes,
                scope='strict-cut Fraction oracle of an equivalent ultrametric, not a 3D benchmark')


if __name__ == '__main__':
    require(len(sys.argv)==2,'usage: judge_sklearn_events.py capture-directory')
    print(json.dumps(judge(Path(sys.argv[1])),sort_keys=True,indent=2))
