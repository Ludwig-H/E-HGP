"""Exact bounded oracle for two weighted selections; no native/GCP calls."""
from fractions import Fraction as F
import json
from random import Random


def require(ok, message):
    if not ok:
        raise ValueError(message)


class Tree:
    def __init__(self, birth, parent):
        require(len(birth) == len(parent) > 0, 'node arrays')
        self.birth = list(map(F, birth))
        self.parent = list(parent)
        self.kids = [[] for _ in parent]
        roots = [v for v, p in enumerate(parent) if p is None]
        require(roots == [len(parent)-1], 'one final root')
        for v, p in enumerate(parent):
            require(self.birth[v] >= 0, 'negative birth')
            if p is not None:
                require(v < p < len(parent) and self.birth[v] <= self.birth[p], 'edge')
                self.kids[p].append(v)
        self.root = roots[0]
        self.depth = [0]*len(parent)
        self.tin, self.tout = [-1]*len(parent), [-1]*len(parent)
        self.euler = []
        pending = [(self.root, False)]
        while pending:
            v, exit_node = pending.pop()
            if exit_node:
                self.tout[v] = len(self.euler)
            else:
                self.tin[v] = len(self.euler)
                self.euler.append(v)
                pending.append((v, True))
                for u in reversed(self.kids[v]):
                    self.depth[u] = self.depth[v]+1
                    pending.append((u, False))
        require(len(self.euler) == len(parent), 'disconnected')

    def ancestor(self, v, date):
        require(self.birth[v] <= date, 'unborn start')
        while self.parent[v] is not None and self.birth[self.parent[v]] <= date:
            v = self.parent[v]
        return v

    def lca(self, a, b):
        while self.depth[a] > self.depth[b]:
            a = self.parent[a]
        while self.depth[b] > self.depth[a]:
            b = self.parent[b]
        while a != b:
            a, b = self.parent[a], self.parent[b]
        return a


def validate_atoms(tree, rows):
    require(bool(rows), 'empty atoms')
    total = F(0)
    for c, v, w in rows:
        require(type(v) is int and 0 <= v < len(tree.parent), 'bad node')
        require(type(c) in (int, F) and type(w) in (int, F), 'nonrational atom')
        require(w > 0 and tree.birth[v] <= c, 'nonpositive weight or unborn atom')
        p = tree.parent[v]
        require(p is None or c < tree.birth[p], 'dead owner')
        total += w
    return total


def select_index(keys, k, cost):
    """BFPRT pivot selection, with all comparisons on exact keys."""
    require(0 <= k < len(keys), 'order index')
    while len(keys) > 5:
        groups = [sorted(keys[i:i+5]) for i in range(0, len(keys), 5)]
        medians = [g[len(g)//2] for g in groups]
        pivot = select_index(medians, len(medians)//2, cost)
        left, equal, right = [], [], []
        for key in keys:
            cost['order_visits'] += 1
            if key < pivot:
                left.append(key)
            elif key > pivot:
                right.append(key)
            else:
                equal.append(key)
        if k < len(left):
            keys = left
        elif k < len(left)+len(equal):
            return pivot
        else:
            k -= len(left)+len(equal)
            keys = right
    return sorted(keys)[k]


def weighted_select(rows, threshold, cost):
    """First key whose inclusive prefix mass is STRICTLY > threshold."""
    rows = list(rows)
    require(0 <= threshold < sum(w for _key, w in rows), 'threshold')
    while len(rows) > 5:
        pivot = select_index([key for key, _w in rows], len(rows)//2, cost)
        left, right = [], []
        wl = we = F(0)
        for key, w in rows:
            cost['weighted_visits'] += 1
            if key < pivot:
                left.append((key, w)); wl += w
            elif key > pivot:
                right.append((key, w))
            else:
                we += w
        if threshold < wl:
            rows = left
        elif threshold < wl+we:
            return pivot
        else:
            threshold -= wl+we
            rows = right
    acc = F(0)
    for key, w in sorted(rows, key=lambda row: row[0]):
        cost['weighted_visits'] += 1
        acc += w
        if acc > threshold:
            return key
    raise ValueError('selection exhausted')


def optimized(tree, rows, mutant=None):
    if mutant == 'drop_ancestor_atoms':
        rows = [row for row in rows if not any(
            row[1] != other[1] and tree.tin[row[1]] <= tree.tin[other[1]] < tree.tout[row[1]]
            for other in rows)]
    total = validate_atoms(tree, rows)
    cost = dict(order_visits=0, weighted_visits=0, lca_calls=0)
    key = weighted_select([(tree.tin[v], w) for _c, v, w in rows], total/2, cost)
    m = tree.euler[key]
    if mutant == 'arbitrary_pivot':
        m = min((v for _c, v, _w in rows), key=lambda v: tree.tin[v])
    thresholds = []
    for c, v, w in rows:
        merge = tree.birth[tree.lca(v, m)]
        cost['lca_calls'] += 1
        thresholds.append((merge if mutant == 'ignore_activation' else max(c, merge), w))
    if mutant == 'lower_median':
        acc = F(0)
        for date, w in sorted(thresholds):
            acc += w
            if 2*acc >= total:
                t = date
                break
    else:
        t = weighted_select(thresholds, total/2, cost)
    owner = tree.ancestor(m, t)
    winning_mass = sum((w for date, w in thresholds if date <= t), F(0))
    return (t, owner), winning_mass, cost


def oracle(tree, rows):
    """Independent full cut sweep, no Euler/LCA/selection."""
    total = validate_atoms(tree, rows)
    dates = sorted(set(tree.birth) | {F(c) for c, _v, _w in rows})
    for date in dates:
        mass = {}
        for c, v, w in rows:
            if c <= date:
                # Deliberately separate closed-cut ancestor walk.
                while tree.parent[v] is not None and tree.birth[tree.parent[v]] <= date:
                    v = tree.parent[v]
                mass[v] = mass.get(v, F(0))+w
        winners = [v for v, w in mass.items() if 2*w > total]
        require(len(winners) <= 1, 'majority nonexclusive')
        if winners:
            v = winners[0]
            return (date, v), mass[v]
    raise ValueError('no majority')


def random_tree(rng, depth):
    birth, parent = [], []
    def node(d):
        cs = [node(d-1) for _ in range(rng.randrange(1, 4))] if d and rng.random() < .8 else []
        b = (max(birth[c] for c in cs) if cs else F(0))+F(rng.randrange(3))
        v = len(birth)
        birth.append(b); parent.append(None)
        for c in cs:
            parent[c] = v
        return v
    node(depth)
    return Tree(birth, parent)


def fixtures():
    tree = Tree([1, 1, 100], [2, 2, None])
    yield 'even_half', tree, [(F(1), 0, F(1)), (F(1), 1, F(1))]
    yield 'minority_first', tree, [(F(1), 0, F(1)), (F(1), 1, F(2))]
    yield 'delayed_same_branch', tree, [(F(5), 0, F(1)), (F(6), 0, F(1))]
    yield 'atomic_cohort', tree, [(F(5), 0, F(1))]*4+[(F(6), 1, F(1))]
    yield 'internal_future', tree, [(F(1), 0, F(4)), (F(101), 2, F(1))]
    yield 'root_majority_late', tree, [(F(1), 0, F(1)), (F(101), 2, F(4))]
    chain = Tree([1, 4, 10], [1, 2, None])
    yield 'internal_activation_not_birth', chain, [(F(7), 1, F(7)), (F(1), 0, F(3))]
    plateau = Tree([1, 1, 10, 10], [2, 2, 3, None])
    yield 'equal_parent_birth_promoted', plateau, [(F(1), 0, F(1)), (F(1), 1, F(1))]
    for seed in range(128):
        rng = Random(303009+seed)
        tree = random_tree(rng, 4)
        alive = [v for v, p in enumerate(tree.parent) if p is None or tree.birth[v] < tree.birth[p]]
        for profile in ('unit', 'integer', 'fraction'):
            rows = []
            for _ in range(rng.randrange(1, 41)):
                v = rng.choice(alive)
                b = tree.birth[v]
                p = tree.parent[v]
                end = tree.birth[p] if p is not None else b+5
                c = b+(end-b)*F(rng.randrange(4), 4)
                w = F(1) if profile == 'unit' else F(rng.randrange(1, 9), 1 if profile == 'integer' else rng.randrange(1, 9))
                rows.append((c, v, w))
            yield ('random_%d_%s' % (seed, profile)), tree, rows


def main():
    cases = atoms = lca_calls = permutation_checks = plateau_checks = 0
    cost_total = dict(order_visits=0, weighted_visits=0)
    named = {}
    for name, tree, rows in fixtures():
        exact, mass = oracle(tree, rows)
        for variant in (rows, list(reversed(rows)), rows[1:]+rows[:1]):
            observed, actual_mass, cost = optimized(tree, variant)
            require(observed == exact and actual_mass == mass, 'oracle mismatch '+name)
            require(2*actual_mass > sum(w for _c, _v, w in variant), 'not strict majority')
            require(cost['lca_calls'] == len(rows), 'LCA budget')
            permutation_checks += 1
            lca_calls += cost['lca_calls']
            for k in cost_total:
                cost_total[k] += cost[k]
        # A whole cohort is counted at the closed selected event.
        plateau_checks += 1
        if not name.startswith('random_'):
            named[name] = dict(date=str(exact[0]), owner=exact[1], winning_mass=str(mass))
        cases += 1; atoms += len(rows)
    mutant_cases = {'lower_median': 'even_half', 'arbitrary_pivot': 'minority_first',
                    'ignore_activation': 'delayed_same_branch', 'drop_ancestor_atoms': 'root_majority_late'}
    mutants = []
    for mutant, target in mutant_cases.items():
        _name, tree, rows = next(case for case in fixtures() if case[0] == target)
        truth, _mass = oracle(tree, rows)
        bad, _wm, _cost = optimized(tree, rows, mutant)
        require(bad != truth, 'mutant survived '+mutant)
        mutants.append(dict(mutant=mutant, case=target, correct_date=str(truth[0]), wrong_date=str(bad[0])))
    refusals = 0
    tree = Tree([1, 1, 100], [2, 2, None])
    for bad in ([], [(F(1), -1, F(1))], [(F(0), 0, F(1))], [(F(100), 0, F(1))],
                [(F(1), 0, F(-1))], [(F(1), 0, F(0))], [(1.0, 0, F(1))], [(F(1), 0, float('nan'))]):
        rejected = False
        try:
            optimized(tree, bad)
        except ValueError:
            rejected = True
        require(rejected, 'invalid atom accepted')
        refusals += 1
    growth = []
    # Selection-only operation probes: NOT n points or a LiDAR pipeline.
    for count in (8000, 16000, 32000):
        cost = dict(order_visits=0, weighted_visits=0)
        keys = [(i*7919) % count for i in range(count)]
        selected = weighted_select([(k, F(1)) for k in keys], F(count, 2), cost)
        require(selected == count//2, 'selection growth value')
        growth.append(dict(atoms=count, expected=count//2, **cost))
    report = dict(status='WEIGHTED_EULER_MAJORITY_PASS', cases=cases, atoms=atoms,
                  permutation_checks=permutation_checks, full_cohort_checks=plateau_checks,
                  lca_calls=lca_calls, **cost_total, named=named, mutants=mutants,
                  rejected_inputs=refusals, selection_only_growth=growth,
                  native_calls=0, GCP_used=False,
                  scope='abstract exact trees/atoms; reference LCA uses parent walks; not native, geometric, GPU or FULL')
    print(json.dumps(report, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
