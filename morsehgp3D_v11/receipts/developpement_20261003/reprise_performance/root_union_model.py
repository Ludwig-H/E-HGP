"""Audit autonome : pliage de racines deja trouvees, sans moteur ni geometrique."""
import itertools
import json


def require(condition, message):
    if not condition:
        raise ValueError(message)


class Fold:
    def __init__(self, n, reduced=False, stale_mutant=False):
        self.parent = list(range(n))
        self.chains = {i: [i] for i in range(n)}
        self.touched = set()
        self.closed = []
        self.reduced = reduced
        self.stale_mutant = stale_mutant
        self.finds = self.touches = self.unions = 0

    def find(self, x):
        self.finds += 1
        root = x
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[x] != x:
            nxt = self.parent[x]
            self.parent[x] = root
            x = nxt
        return root

    def touch(self, r):
        self.touches += 1
        if r not in self.touched:
            self.touched.add(r)
            self.chains[r] = [r]

    def merge_roots(self, a, b):
        # Precondition de la variante correcte : deux racines touchees.
        if not self.stale_mutant:
            require(self.parent[a] == a and self.parent[b] == b,
                    'helper exige deux racines courantes')
            require(a in self.touched and b in self.touched, 'helper exige touch')
        if a == b:
            return a
        a, b = sorted((a, b))
        self.parent[b] = a
        self.chains[a] = self.chains[a] + self.chains[b]
        self.unions += 1
        return a

    def cell(self, seeds):
        first = None
        for seed in seeds:
            r = self.find(seed)
            self.touch(r)
            if first is None:
                first = r
            elif self.reduced:
                merged = self.merge_roots(first, r)
                if not self.stale_mutant:
                    first = merged
            else:
                a, b = self.find(first), self.find(r)
                self.touch(a)
                self.touch(b)
                self.merge_roots(a, b)

    def plateau(self, cells):
        for seeds in cells:
            self.cell(seeds)
        groups = []
        for r in sorted(self.touched):
            if self.parent[r] == r:
                group = tuple(sorted(self.chains[r]))
                groups.append(group)
        self.closed.append(tuple(groups))
        self.touched.clear()

    def partition(self):
        out = {}
        for x in range(len(self.parent)):
            r = x
            while self.parent[r] != r:
                r = self.parent[r]
            out.setdefault(r, []).append(x)
        return tuple(tuple(xs) for _, xs in sorted(out.items()))


def oracle(n, plateaus):
    groups = [{i} for i in range(n)]
    for cells in plateaus:
        for seeds in cells:
            affected = [g for g in groups if g.intersection(seeds)]
            joined = set().union(*affected)
            groups = [g for g in groups if g not in affected] + [joined]
    return tuple(sorted((tuple(sorted(g)) for g in groups), key=lambda g: g[0]))


def check(n, plateaus):
    old, new = Fold(n), Fold(n, reduced=True)
    traces = cells = 0
    for p in plateaus:
        old.plateau(p)
        new.plateau(p)
        traces += sum(map(len, p))
        cells += len(p)
    require(old.partition() == new.partition() == oracle(n, plateaus), 'partition')
    require(old.closed == new.closed, 'anciennes composantes fermees du plateau')
    require(old.unions == new.unions, 'nombre unions')
    require(old.finds == old.touches == 3 * traces - 2 * cells, 'ancien cout')
    require(new.finds == new.touches == traces, 'nouveau cout')
    return old, new


def main():
    cases = 0
    for size in range(1, 5):
        for seq in itertools.product(range(5), repeat=size):
            check(5, [[seq]])
            cases += 1
    # Deux cellules au meme plateau puis plateau suivant : alias de racines,
    # repetitions, et anciennes composantes deja fusionnees.
    for order in itertools.permutations(range(5)):
        check(5, [[order[:3], order[2:]], [(order[4], order[0], order[1])]])
        cases += 1
    old, new = check(6, [[(5, 3, 4)]])
    mutant = Fold(6, reduced=True, stale_mutant=True)
    mutant.plateau([(5, 3, 4)])
    require(mutant.partition() != new.partition(), 'mutant first perime doit echouer')
    print(json.dumps({
        'cases': cases,
        'fixture': {'seeds': [5, 3, 4], 'original': old.partition(),
                    'reduced': new.partition(), 'stale_first_mutant': mutant.partition(),
                    'find_touch_original': old.finds, 'find_touch_reduced': new.finds},
        'regular_ng00': {'traces': 3621204, 'cells': 1306469,
                        'find_touch_original': 3 * 3621204 - 2 * 1306469,
                        'find_touch_reduced': 3621204,
                        'redundant_each': 2 * (3621204 - 1306469)},
        'native_executions': 0
    }, sort_keys=True))


if __name__ == '__main__':
    main()
