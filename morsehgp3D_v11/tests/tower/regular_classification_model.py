"""Classification reguliere : certification affine et tableaux FULL Gamma/Fraction.

Le modele affine enumere TOUS les temoins convexes des sous-coquilles ; il ne
rejoue pas le raccourci produit. Definition construit ensuite les graphes de
k-parties et cofaces. Aucun import constructive, aucun binaire local.
"""
import copy
import itertools as it
import math
from pathlib import Path

import classification_model as convex
import forest_oracle as oracle

CASES = (
    ('singleton', ((3,5,1),)),
    ('line', ((0,0,0),(2,0,0),(4,0,0),(8,0,0))),
    ('triangle', ((0,0,0),(6,0,0),(3,6,0),(3,2,0))),
    ('tetra_empty', ((0,0,0),(4,4,0),(4,0,4),(0,4,4))),
    ('tetra', ((0,0,0),(4,4,0),(4,0,4),(0,4,4),(2,2,2))),
    ('square', ((0,0,0),(4,0,0),(0,4,0),(4,4,0),(2,2,0))),
    ('coshell', ((5,5,0),(2,1,5),(10,5,5),(2,9,5),(5,9,8))),
)
NONE = (1 << 32)-1


def derive(points):
    records = oracle.data.fixtures.records(points)
    sites, _, balls, _ = oracle.data.geometry(records,len(points))
    levels = sorted({0} | {b.level for b in balls})
    truth = oracle.reference(sites)
    orders = []
    for k in range(1,len(points)+1):
        original = truth.order(k)
        parents = [NONE]*len(original.nodes)
        for i,n in enumerate(original.nodes):
            for child in n.children:
                parents[child] = i
        edges, nodes = [], []
        for i,n in enumerate(original.nodes):
            key = NONE if n.children else sites.index(n.center) if k == 1 else next(
                b for b,ball in enumerate(balls) if (ball.level,ball.center) == (n.level,n.center))
            nodes.append((levels.index(n.level),parents[i],len(edges),len(n.children),key))
            edges.extend(n.children)
        orders.append(dict(nodes=nodes,edges=edges,lower=list(original.lower or ())))
    return sites,balls,orders


def header():
    lines = ['// Attendus engendres par regular_classification_model.py : Gram + Gamma, pas code produit.',
             '#pragma once', '#include "forest_support.hpp"',
             'namespace regular_classification_test {', 'using namespace forest_test;',
             'struct ExpectedOrder { std::vector<std::array<u32,5>> nodes; std::vector<u32> edges, lower; };',
             'struct Fixture { const char* name; std::vector<Xyz> xyz; std::vector<ExpectedOrder> orders; };',
             'inline std::vector<Fixture> fixtures() { return {']
    def seq(values):
        return '{'+','.join('kNone' if v == NONE else str(v) for v in values)+'}'
    for name,points in CASES:
        _,_,orders = derive(points)
        lines.append('  {"'+name+'",{'+','.join(seq(p) for p in points)+'},{')
        for o in orders:
            lines.append('    {{'+','.join(seq(n) for n in o['nodes'])+'},'+seq(o['edges'])+','+seq(o['lower'])+'},')
        lines.append('  }},')
    lines.extend(['}; }', '}  // namespace regular_classification_test', ''])
    return '\n'.join(lines)


def validate(value, expected):
    return oracle.equal(value,expected)


def run():
    checks = corruptions = regular = extended = orders = 0
    coverage = set()
    for bits in (18,21,24):
        for name,points in CASES:
            _,_,baseline = derive(points)
            for factor in (1,1 << (bits-4)):
                scaled = tuple(tuple(v*factor for v in p) for p in points)
                sites,balls,truth = derive(scaled)
                checks += validate(truth,baseline)
                checks += validate(derive(scaled[::-1])[2],baseline)
                orders += len(truth)
                for ball in balls:
                    shell = convex.certify(tuple(sites[i] for i in ball.shell),ball.center,ball.qmin)
                    for k in range(max(1,ball.p+ball.qmin-1),min(len(points),ball.p+len(ball.shell))+1):
                        t = k-ball.p
                        strict = convex.exhaustive(shell,t)
                        expected = dict(kind=2 if strict else 1,combinations=math.comb(len(ball.shell),t))
                        if len(ball.shell) != ball.qmin:
                            extended += 1
                            continue
                        regular += 1
                        coverage.add((ball.qmin,ball.p > 0))
                        actual = dict(kind=1 if k == ball.p+ball.qmin else 2,
                                      combinations=1 if k == ball.p+ball.qmin else ball.qmin)
                        checks += validate(actual,expected)
                        oracle.require(len(strict) in (0,ball.qmin),'toutes faces strictes'); checks += 1
                        for field in expected:
                            bad = dict(actual); bad[field] += 1
                            try:
                                validate(bad,expected)
                            except ValueError:
                                corruptions += 1
                            else:
                                raise ValueError('mutation reguliere acceptee')
    oracle.require(coverage == {(q,p) for q in (2,3,4) for p in (False,True)},'q2..4 avec et sans interieur')
    text = header()
    oracle.require(Path(__file__).with_name('regular_classification_truth.hpp').read_text() == text,'tableaux natifs graves')
    # Toutes les donnees des tableaux sont controlees, y compris les parents, cles, aretes et verticales.
    for _,points in CASES:
        _,_,truth = derive(points)
        for o in truth:
            for field in ('nodes','edges','lower'):
                if not o[field]:
                    continue
                bad = copy.deepcopy(o)
                if field == 'nodes':
                    n = list(bad[field][0]); n[0] += 1; bad[field][0] = tuple(n)
                else:
                    bad[field][0] += 1
                try:
                    validate(bad,o)
                except ValueError:
                    corruptions += 1
                else:
                    raise ValueError('mutation tableau acceptee')
    oracle.require(regular > 0 and extended > 0,'branches non vacantes')
    print(f'regular_classification_model_verdict conforme orders{orders} regular{regular} extended{extended} '
          f'checks{checks} corruptions{corruptions} native0')


if __name__ == '__main__':
    run()
