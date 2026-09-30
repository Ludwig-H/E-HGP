"""Independent exact event oracle for a FIXED API tree; not an MEB-realizability test.

The minimum-size branch dies as soon as a direct point-exit cohort leaves
fewer than mcs units. Completion singletons are not extra active clusters.
mcs=1 is only a positive API control (standard sklearn requires mcs>=2).
All levels here are positive perfect squares: z=1 and z=2 lambda are Fraction.
No native process, random benchmark, float ground truth, or asserts.
"""
from fractions import Fraction as F
import json
from math import isfinite, isqrt
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
LEVEL = (1, 4, 9, 16, 25)
BIRTH = (1, 16, 25)
CHILDREN = ((), (), (0, 1))
NODE = (0, 0, 0, 1, 1)
FIXTURES = {
    'staggered': ((1, 4, 9, 16, 16), (1, 1, 1, 1, 1)),
    'cohort': ((1, 4, 4, 16, 16), (1, 1, 1, 1, 1)),
    'same_date': ((4, 4, 4, 16, 16), (1, 1, 1, 1, 1)),
    'weighted_core': ((1, 4, 9, 16, 16), (2, 1, 1, 1, 1)),
}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def load(text):
    def unique(pairs):
        row = {}
        for key, value in pairs:
            require(key not in row, 'duplicate JSON key')
            row[key] = value
        return row
    def bad_constant(value):
        raise ValueError('nonfinite JSON')
    def finite(value):
        number = float(value)
        require(isfinite(number), 'nonfinite JSON float')
        return number
    return json.loads(text, object_pairs_hook=unique, parse_constant=bad_constant, parse_float=finite)


def lam(beta, z):
    if z == 2:
        return F(1, beta)
    require(z == 1 and isqrt(beta)**2 == beta, 'nonrational lambda outside fixture')
    return F(1, isqrt(beta))


def exact_oracle(case, mcs, z):
    """Sweep the decreasing squared-radius cuts and their direct exit cohorts.

    Alive sets are explicit labelled points, NOT a cached subtree-size
    computation. All events at one beta are atomic. This bounded fixture has
    unequal geometric births; point-exit equalities are exercised in cohorts.
    """
    entry, weight = FIXTURES[case]
    descendants = []
    for v in range(3):
        own = {x for x, node in enumerate(NODE) if node == v}
        for c in CHILDREN[v]:
            own |= descendants[c]
        descendants.append(own)
    mass = lambda points: sum(weight[x] for x in points)
    clusters = [dict(parent=None, birth=F(0), stability=F(0), mass=mass(range(5)))]
    alive = {2: (0, set(range(5)))}
    point_cluster, point_lambda = [None]*5, [None]*5
    collapses = []
    def drop(points, c, value):
        for x in sorted(points):
            require(point_cluster[x] is None, 'point paid twice')
            point_cluster[x], point_lambda[x] = c, value
            clusters[c]['stability'] += weight[x]*(value-clusters[c]['birth'])
    for beta in sorted(set(entry) | set(BIRTH), reverse=True):
        value = lam(beta, z)
        # First all direct exits at this level, independently of their node.
        for v in sorted(tuple(alive)):
            c, points = alive[v]
            cohort = {x for x in points if entry[x] == beta}
            drop(cohort, c, value)
            points -= cohort
            if not points:
                del alive[v]
            elif mass(points) < mcs:
                collapses.append(dict(beta=str(beta), node=v, remaining=mass(points),
                                      cohort_mass=mass(cohort), points=sorted(points)))
                drop(points, c, value)
                del alive[v]
        # Then the spatial component split at that beta, if it survived.
        for v in (2, 1, 0):
            if BIRTH[v] != beta or v not in alive:
                continue
            c, points = alive.pop(v)
            parts = [(child, points & descendants[child]) for child in CHILDREN[v]]
            large = [(child, part) for child, part in parts if mass(part) >= mcs]
            if len(large) >= 2:
                for child, part in parts:
                    if mass(part) < mcs:
                        drop(part, c, value)
                        continue
                    clusters[c]['stability'] += mass(part)*(value-clusters[c]['birth'])
                    new = len(clusters)
                    clusters.append(dict(parent=c, birth=value, stability=F(0), mass=mass(part)))
                    alive[child] = (new, set(part))
            elif len(large) == 1:
                child, keep = large[0]
                drop(points-keep, c, value)
                alive[child] = (c, set(keep))
            else:
                drop(points, c, value)
    require(not alive and all(c is not None for c in point_cluster), 'unfinished exact sweep')
    children = [[] for _ in clusters]
    for c, row in enumerate(clusters):
        if row['parent'] is not None:
            children[row['parent']].append(c)
    best, picked = {}, set()
    for c in reversed(range(len(clusters))):
        sub = sum((best[k] for k in children[c]), F(0))
        if children[c] and sub > clusters[c]['stability']:
            best[c] = sub
        else:
            best[c] = clusters[c]['stability']
            picked.add(c)
    selected = []
    for c in sorted(picked):
        parent = clusters[c]['parent']
        while parent is not None and parent not in picked:
            parent = clusters[parent]['parent']
        if parent is None:
            selected.append(c)
    label_id = {c: i for i, c in enumerate(selected)}
    labels = []
    for c in point_cluster:
        while c is not None and c not in label_id:
            c = clusters[c]['parent']
        labels.append(-1 if c is None else label_id[c])
    return dict(clusters=clusters, point_cluster=point_cluster, point_lambda=point_lambda,
                selected=selected, labels=labels, threshold_collapses=collapses)


def near(actual, expected):
    return type(actual) in (int, float) and isfinite(actual) and abs(actual-float(expected)) <= 1e-12


def judge(path):
    rows = [load(line) for line in path.read_text().splitlines()]
    require(len(rows) == 16, 'native row floor')
    identities = set()
    comparisons = []
    mismatch_rows = flips = positives = 0
    for row in rows:
        key = row['case'], row['mcs'], row['z']
        require(key not in identities and key[0] in FIXTURES and
                type(key[1]) is int and key[1] in (1, 2) and type(key[2]) is int and key[2] in (1, 2),
                'case identity')
        identities.add(key)
        require(row['api_valid'] is True and row['allow_single'] is True, 'API/single scope')
        expected = exact_oracle(*key)
        require(len(row['clusters']) == len(expected['clusters']) == 3, 'cluster topology')
        require(len(row['point_lambda']) == len(row['labels']) == len(row['point_cluster']) == 5,
                'point output count')
        difference = False
        for actual, truth in zip(row['clusters'], expected['clusters']):
            require(actual['parent'] == truth['parent'] and actual['mass'] == truth['mass'] and
                    near(actual['birth'], truth['birth']), 'unexpected topology/birth/mass difference')
            difference |= not near(actual['stability'], truth['stability'])
        difference |= any(not near(actual, truth) for actual, truth in
                          zip(row['point_lambda'], expected['point_lambda']))
        require(row['point_cluster'] == expected['point_cluster'], 'unexpected point-cluster difference')
        should_differ = key[0] in ('staggered', 'cohort') and key[1] == 2
        require(difference == should_differ, 'unexpected equality/disagreement')
        flip = row['selected'] != expected['selected']
        require(flip == (should_differ and key[2] == 1), 'unexpected EOM outcome')
        require(row['labels'] == expected['labels'] or flip, 'unexpected labels without EOM flip')
        mismatch_rows += difference
        flips += flip
        positives += not difference
        comparisons.append(dict(case=key[0], mcs=key[1], z=key[2], differs=difference,
            eom_differs=flip, native_selected=row['selected'], exact_selected=expected['selected'],
            native_labels=row['labels'], exact_labels=expected['labels'],
            native_stabilities=[r['stability'] for r in row['clusters']],
            exact_stabilities=list(map(str, (r['stability'] for r in expected['clusters']))),
            native_point_lambda=row['point_lambda'], exact_point_lambda=list(map(str,expected['point_lambda'])),
            threshold_collapses=expected['threshold_collapses']))
    required = {(case, mcs, z) for case in FIXTURES for mcs in (1, 2) for z in (1, 2)}
    require(identities == required and mismatch_rows == 4 and flips == 2 and positives == 12, 'nonvacuity coverage')
    return dict(status='EXPECTED_API_DIFFERENCE_CONFIRMED', rows=16, exact_positive_controls=12,
                differing_rows=4, EOM_flips=2, comparisons=comparisons,
                scope='fixed API tree; no MEB/strong-catalogue realizability claim', native_executions=0)


if __name__ == '__main__':
    require(len(sys.argv) == 2, 'usage: check.py native.stdout')
    print(json.dumps(judge(Path(sys.argv[1])), sort_keys=True, indent=2))

