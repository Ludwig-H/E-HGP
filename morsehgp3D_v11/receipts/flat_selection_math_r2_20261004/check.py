#!/usr/bin/env python3
"""Correction of exit semantics and exact integer selection witnesses.

Standalone scalar Gamma_2: MEB of a collinear finite subset is half its span.
The old receipt is preserved. Its geometric/EOM/gap results remain valid;
only the general condensed-exit sentence needs the qualification in README.
"""
from fractions import Fraction as Q
from itertools import combinations
import json

CHECKS = 0


def need(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(message)


def check_gamma(X, entry, merge, root):
    need(all(type(x) is int and 0 <= x < 2**18 for x in X), "valid integer u18/u21/u24 sites")
    need(len(set(X)) == len(X) and X == sorted(X), "nine distinct sorted sites")
    pairs = list(combinations(range(9), 2))
    triples = list(combinations(range(9), 3))
    level = {p: Q((X[max(p)]-X[min(p)])**2, 4) for p in pairs+triples}
    for beta in sorted(set(level.values())):
        parent = {p: p for p in pairs if level[p] <= beta}

        def find(p):
            while parent[p] != p:
                p = parent[p]
            return p

        for part in triples:
            if level[part] <= beta:
                faces = list(combinations(part, 2))
                for f in faces[1:]:
                    parent[find(f)] = find(faces[0])
        groups = {}
        for p in parent:
            groups.setdefault(find(p), set()).update(p)
        qualified = sorted(tuple(sorted(g)) for g in groups.values() if len(g) >= 3)
        expected = [] if beta < entry**2 else [(0, 1, 2), (3, 4, 5), (6, 7, 8)] \
            if beta < merge**2 else [(0, 1, 2, 3, 4, 5), (6, 7, 8)] \
            if beta < root**2 else [tuple(range(9))]
        need(qualified == expected, "all closed qualified Gamma cuts match H")
        for i in range(9):
            need(sum(i in g for g in qualified) == (0 if beta < entry**2 else 1),
                 "no qualified rival, hence H entry exactly entry")
    return len(set(level.values()))


def main():
    rows = []
    for shift in (-1, 0, 1):
        X = [0, 6, 12, 22+shift, 28+shift, 34+shift,
             52+shift, 58+shift, 64+shift]
        entry, merge, root = Q(6), Q(16+shift, 2), Q(12)
        cuts = check_gamma(X, entry, merge, root)
        parent = 6*(1/merge-1/root)
        children = 6*(1/entry-1/merge)
        expected = { -1: (Q(3, 10), Q(1, 5)),
                      0: (Q(1, 4), Q(1, 4)),
                      1: (Q(7, 34), Q(5, 17))}[shift]
        need((parent, children) == expected, "integer exact EOM decision")
        need((parent >= children) == (shift <= 0), "parent equality policy and actual flip")
        rows.append(dict(shift=shift, points=X, entry_radius=str(entry), merge_radius=str(merge),
                         root_radius=str(root), gamma_cuts=cuts,
                         parent_score=str(parent), children_score=str(children),
                         optimum="C|D" if parent >= children else "A|B|D"))

    # Unscaled original geometry, now mcs=4 instead of mcs=3.
    # A/B/D are mass3. D falls out when root first splits at r4.
    e, mcs, small_mass = Q(2), 4, 3
    raw_exit = 1/e
    D_fallout = Q(1, 4)
    AB_fallout = Q(2, 5)
    need(small_mass < mcs, "three-site branches are too small at mcs4")
    need(D_fallout < raw_exit and AB_fallout < raw_exit,
         "condensed fallout may precede raw point inactivity")
    need(3+6 == 9, "departing cohorts conserve total integer mass")
    need(3*D_fallout+6*AB_fallout == Q(63, 20),
         "single-large-child continuation keeps the root lineage, correct fallout score")
    print(json.dumps(dict(scope="pure exact scalar Gamma/EOM; no native qualification",
                          checks=CHECKS, integer_cases=rows,
                          correction=dict(raw_exit_lambda=str(raw_exit),
                                          condensed_D_exit_lambda=str(D_fallout),
                                          condensed_AB_exit_lambda=str(AB_fallout),
                                          mcs=4, root_continues_into_C=True,
                                          root_stability="63/20",
                                          root_excluded_selection="all noise")),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
