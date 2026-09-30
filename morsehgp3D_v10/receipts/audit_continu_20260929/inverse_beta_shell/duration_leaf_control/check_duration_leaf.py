"""Tiny design control, not a complete production projection.

Only actual birth leaves, no ancestor atoms in the denominator. Their
point-specific weights are covered lifetime in phi=1/r or phi=1/beta.
K2 Gamma is enumerated explicitly on four points only.
"""
from fractions import Fraction as Q
from itertools import combinations
from math import isqrt
from pathlib import Path
import hashlib
import importlib.util
import json
import sys

if len(sys.argv) != 2:
    raise SystemExit('usage: check_duration_leaf.py FROZEN_FRACTION_REFERENCE')
ref_path = Path(sys.argv[1]).resolve()
inputs = (Path(__file__).resolve(), ref_path)
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
spec = importlib.util.spec_from_file_location('frozen_ref', ref_path)
ref = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ref)


def check(ok, message):
    if not ok:
        raise RuntimeError(message)


def tree(P):
    pair_beta = {F: ref.meb(P,F)[0] for F in combinations(range(len(P)), 2)}
    tri_beta = {G: ref.meb(P,G)[0] for G in combinations(range(len(P)), 3)}
    levels = sorted(set(pair_beta.values()) | set(tri_beta.values()))
    dsu, alive, branch_for_pair, nodes = ref.DSU(), set(), {}, []
    for beta in levels:
        prior = branch_for_pair.copy()
        for F, born in pair_beta.items():
            if born == beta:
                dsu.find(F)
                alive.add(F)
        for G, joined in tri_beta.items():
            if joined == beta:
                Fs = list(combinations(G,2))
                for F in Fs[1:]:
                    dsu.union(Fs[0],F)
        comps = {}
        for F in alive:
            comps.setdefault(dsu.find(F), []).append(F)
        branch_for_pair = {}
        for root in sorted(comps):
            Fs = comps[root]
            previous = sorted({prior[F] for F in Fs if F in prior})
            if len(previous) == 1:
                v = previous[0]
            else:
                v = len(nodes)
                nodes.append({'birth': beta, 'death': None, 'parent': None,
                              'leaf': not previous, 'first': {}})
                for c in previous:
                    check(nodes[c]['death'] is None, 'branch dies twice')
                    nodes[c]['death'], nodes[c]['parent'] = beta, v
            for F in Fs:
                branch_for_pair[F] = v
                for x in F:
                    nodes[v]['first'].setdefault(x,beta)
    check(sum(n['parent'] is None for n in nodes) == 1, 'not a rooted tree')
    return levels, nodes


def phi(beta, power):
    if beta is None:
        return Q(0),Q(0)
    check(beta > 0, 'zero handling not covered by this design control')
    inverse = 1/beta
    if power == 2:
        return inverse,inverse
    # Enclose sqrt(inverse) by adjacent exact dyadic rationals.
    B = 200
    scaled = inverse.numerator * (1 << (2*B))
    denominator = inverse.denominator
    floor = isqrt(scaled // denominator)
    low = Q(floor, 1 << B)
    high = low if floor*floor*denominator == scaled else Q(floor+1,1 << B)
    check(low*low <= inverse <= high*high, 'bad sqrt interval')
    return low,high


def analyze(P, power):
    levels, nodes = tree(P)
    atoms = [[] for _ in P]
    for v,node in enumerate(nodes):
        if not node['leaf']:
            continue
        death_phi = phi(node['death'],power)
        for x,t in sorted(node['first'].items()):
            birth_phi = phi(t,power)
            lo,hi = birth_phi[0]-death_phi[1],birth_phi[1]-death_phi[0]
            check(lo > 0, 'zero or uncertain covered lifespan')
            atoms[x].append((v,t,lo,hi))
    check(all(atoms), 'fixture point not covered by a birth leaf')

    def ancestor(v,beta):
        while nodes[v]['parent'] is not None and nodes[nodes[v]['parent']]['birth'] <= beta:
            v = nodes[v]['parent']
        return v

    cuts = sorted(set(levels) | {(a+b)/2 for a,b in zip(levels,levels[1:])})
    previous, pair_first, trace, pair_checks = None, {}, [], 0
    for beta in cuts:
        owners = []
        for x,rows in enumerate(atoms):
            Wlo,Whi = sum((a[2] for a in rows),Q(0)),sum((a[3] for a in rows),Q(0))
            masses = {}
            for v,t,lo,hi in rows:
                if t <= beta:
                    c = ancestor(v,beta)
                    old = masses.get(c,(Q(0),Q(0)))
                    masses[c] = old[0]+lo,old[1]+hi
            found = []
            for c,(lo,hi) in masses.items():
                if 2*lo > Whi:
                    found.append(c)
                else:
                    check(2*hi <= Wlo, 'majority comparison uncertified')
            check(len(found) <= 1,'two strict majorities')
            owners.append(('component',found[0]) if found else ('singleton',x))
        for i,j in combinations(range(len(P)),2):
            if previous is not None:
                check(previous[i] != previous[j] or owners[i] == owners[j], 'point block split')
                pair_checks += 1
            if owners[i] == owners[j]:
                pair_first.setdefault(str((i,j)),str(beta))
        groups = {}
        for x,o in enumerate(owners):
            groups.setdefault(o,[]).append(x)
        trace.append({'beta': str(beta), 'blocks': sorted(groups.values())})
        previous = owners
    return {'point_leaf_incidence_count': sum(map(len,atoms)),
            'leaf_count': sum(n['leaf'] for n in nodes), 'node_count': len(nodes),
            'pair_first_beta': pair_first, 'pair_checks': pair_checks,
            'leaf_geometry': [{'birth':str(n['birth']), 'death':str(n['death']),
                               'first_covers':{str(x):str(t) for x,t in n['first'].items()}}
                              for n in nodes if n['leaf']], 'trace':trace}


def main():
    fixtures = {
        'tetra_original': [(0,0,0),(8192,0,0),(0,10240,0),(0,0,204800)],
        'tetra_perturbed': [(1,1,1),(8192,0,0),(0,10240,0),(0,0,204800)],
        'two_pairs': [(0,0,0),(1,0,0),(100,0,0),(101,0,0)],
        'two_pairs_tetra': [(0,0,0),(1,0,0),(100,1,1),(101,1,2)],
    }
    rows = {}
    for name,P in fixtures.items():
        row = {}
        for power in (1,2):
            row['inverse_radius' if power == 1 else 'inverse_beta'] = analyze(P,power)
            first = row['inverse_radius' if power == 1 else 'inverse_beta']['pair_first_beta']
            check(first[str((0,1))] == str(ref.d2(P[0],P[1])/4), 'CA or left pair not recovered early')
            if name.startswith('two_pairs'):
                check(first[str((2,3))] == str(ref.d2(P[2],P[3])/4), 'right pair not recovered early')
        rows[name] = row
    after = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    check(before == after,'source changed during control')
    print(json.dumps({'status':'DURATION_LEAF_TINY_CONTROL_PASS',
                      'scope':'four_site_K2_control_only_not_general_complete_projection',
                      'GCP_used':False,'engine_modified':False,'native_calls':0,
                      'argv':sys.argv,'optimized':sys.flags.optimize,
                      'hashes_before':before,'hashes_after':after,'rows':rows},indent=2))


if __name__ == '__main__':
    main()
