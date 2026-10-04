#!/usr/bin/env python3
"""Independent exact collinear Gamma_2 and EOM selection witnesses.

MEB radius of a collinear subset is half its extent. The test derives the
qualified coverage at every pair/coface event, before using the proven H
profile. No native, product oracle, fit, labels, or statistical conclusion.
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


def points(mu):
    return list(map(Q, [0, 2, 4, 2+2*mu, 4+2*mu, 6+2*mu,
                       12+2*mu, 14+2*mu, 16+2*mu]))


def gamma_model(mu):
    X = points(mu)
    pairs, triples = list(combinations(range(9), 2)), list(combinations(range(9), 3))
    beta = {p: (X[max(p)]-X[min(p)])**2/4 for p in pairs+triples}

    def cut(level):
        parent = {p: p for p in pairs if beta[p] <= level}

        def find(p):
            while parent[p] != p:
                p = parent[p]
            return p

        for t in triples:
            if beta[t] <= level:
                fs = list(combinations(t, 2))
                for f in fs[1:]:
                    parent[find(f)] = find(fs[0])
        groups = {}
        for p in parent:
            groups.setdefault(find(p), set()).update(p)
        return sorted(tuple(sorted(g)) for g in groups.values() if len(g) >= 3)

    for level in sorted(set(beta.values())):
        groups = cut(level)
        for i in range(9):
            count = sum(i in g for g in groups)
            need(count == (0 if level < 4 else 1), "every qualified lineage unique at every closed event")
        expected = [(0, 1, 2), (3, 4, 5), (6, 7, 8)] if 4 <= level < mu*mu else \
                   [(0, 1, 2, 3, 4, 5), (6, 7, 8)] if mu*mu <= level < 16 else \
                   [tuple(range(9))] if level >= 16 else []
        need(groups == expected, "complete qualified Gamma cuts match the stated H tree")
    return dict(points=list(map(str, X)), entry_radius="2", AB_merge=str(mu), root_merge="4",
                qualified_event_cuts=len(set(beta.values())),
                before_entry_active=0, before_entry_inactive=9)


def scores(mu, z, scale=Q(1)):
    a, b, c = 2*scale, mu*scale, 4*scale
    lam = lambda r: r**(-z)
    parent = 6*(lam(b)-lam(c))
    children = 6*(lam(a)-lam(b))
    D = 3*(lam(a)-lam(c))
    return parent, children, D


def selected(mu, z, scale=Q(1)):
    parent, children, _ = scores(mu, z, scale)
    # Root excluded, parent wins certified score equality, independent of IDs.
    return [[0, 1, 2, 3, 4, 5], [6, 7, 8]] if parent >= children else \
           [[0, 1, 2], [3, 4, 5], [6, 7, 8]]


def main():
    regimes = []
    mu = Q(5, 2)
    geometry = gamma_model(mu)
    expected = {1: (Q(9, 10), Q(3, 5)), 3: (Q(1161, 4000), Q(183, 500))}
    for z in (1, 3):
        parent, children, D = scores(mu, z)
        need((parent, children) == expected[z], "exact parameter-dependent EOM scores")
        need((parent > children) == (z == 1), "same tree selects parent or children depending on z")
        for scale in (Q(1, 1000), Q(1000)):
            need(scores(mu, z, scale) == tuple(v*scale**(-z) for v in (parent, children, D)),
                 "common units rescale every EOM score equally")
            need(selected(mu, z, scale) == selected(mu, z), "units alone preserve selection")
        regimes.append(dict(z=z, parent_score=str(parent), children_score=str(children),
                            D_score=str(D), blocks=selected(mu, z)))

    delta = Q(1, 1000)
    tie = Q(8, 3)
    need(scores(tie, 1)[:2] == (Q(3, 4), Q(3, 4)), "certified exact EOM tie")
    perturbations = []
    for offset in (-delta, delta):
        value = tie+offset
        model = gamma_model(value)
        parent, children, _ = scores(value, 1)
        need((parent > children) == (offset < 0), "arbitrarily small shift switches optimum")
        displacement = max(abs(a-b) for a, b in zip(points(tie), points(value)))
        need(displacement == 2*delta, "matched displacement measured on all points")
        perturbations.append(dict(offset=str(offset), matched_displacement=str(displacement),
                                  parent_score=str(parent), children_score=str(children),
                                  blocks=selected(value, 1), geometry=model))
    left, right = selected(tie-delta, 1), selected(tie+delta, 1)
    pairs = lambda blocks: {pair for block in blocks for pair in combinations(block, 2)}
    need(len(pairs(left)^pairs(right)) == 9, "nine pair associations change despite small date movement")

    margins = []
    for z in (1, 3):
        dlam = z*delta/(2-delta)**(z+1)
        gap = abs(scores(mu, z)[0]-scores(mu, z)[1])
        need(gap > 4*6*dlam, "positive score margin certifies the stable local DP decision")
        need(selected(mu-delta, z) == selected(mu, z) == selected(mu+delta, z),
             "corresponding bounded perturbations preserve certified decisions")
        margins.append(dict(z=z, gap=str(gap), bound=str(4*6*dlam)))
    need(abs(scores(tie-delta, 1)[0]-scores(tie-delta, 1)[1]) < 4*6*delta/(2-delta)**2,
         "gap certificate correctly does not certify the switching example")
    ghost_children = 6*(1-mu**(-1))
    need(ghost_children > scores(mu, 1)[0], "keeping inactive cohorts creates a false EOM winner")
    root_score = 9*Q(4)**(-1)
    descendant_score = max(scores(mu, 1)[:2])+scores(mu, 1)[2]
    need(root_score == Q(9, 4) and descendant_score == Q(33, 20),
         "root admission is a separate exact candidate policy")
    need(root_score > descendant_score, "allowing the root changes the z1 optimum to one cluster")
    print(json.dumps(dict(scope="exact finite geometry and selection examples, no native qualification",
                          checks=CHECKS, same_geometry=geometry, parametrizations=regimes,
                          tie=dict(AB_merge=str(tie), parent_score="3/4", children_score="3/4"),
                          perturbations=perturbations, changed_pairs=9,
                          margin_certificates=margins,
                          root_policy=dict(z=1, excluded_best_score=str(descendant_score),
                                           admitted_root_score=str(root_score),
                                           admitted_blocks=[list(range(9))]),
                          ghost=dict(true_child_exit_lambda="1/2", erroneous_exit_lambda="1",
                                     erroneous_children_score=str(ghost_children))),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
