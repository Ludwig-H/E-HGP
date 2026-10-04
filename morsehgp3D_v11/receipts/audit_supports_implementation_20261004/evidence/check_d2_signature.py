#!/usr/bin/env python3
"""Private D2 proposal model and source-only S1 checks. No native code/oracle run."""
import ast
import copy
import hashlib
import itertools
import json
import math
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
NONE = (1 << 32) - 1
checks = 0


def check(condition, text):
    global checks
    checks += 1
    if not condition:
        raise ValueError(text)


def u32(x):
    if type(x) is not int or not 0 <= x <= NONE:
        raise ValueError('u32')
    return struct.pack('<I', x)


def u64(x):
    if type(x) is not int or not 0 <= x < 1 << 64:
        raise ValueError('u64')
    return struct.pack('<Q', x)


def morton(point):
    return sum(((point[a] >> b) & 1) << (3*b + a) for a in range(3) for b in range(24))


def canonical_sites(rows):
    return sorted(rows, key=lambda r: morton(r[:3]))


def children_from_parent(nodes):
    kids = [[] for _ in nodes]
    roots = []
    for i, n in enumerate(nodes):
        p = n['parent']
        if p == NONE:
            roots.append(i)
        elif 0 <= p < len(nodes) and p != i:
            kids[p].append(i)
        else:
            raise ValueError('parent')
    if len(roots) != 1:
        raise ValueError('root')
    return kids, roots[0]


def postorder(nodes):
    kids, root = children_from_parent(nodes)
    out, seen = [], set()
    def walk(v):
        if v in seen:
            raise ValueError('cycle')
        seen.add(v)
        for c in kids[v]:
            walk(c)
        out.append(v)
    walk(root)
    if len(out) != len(nodes):
        raise ValueError('disconnected')
    return out


def geometry_digest(bits, sites):
    raw = b'MHGP11GX' + u64(bits) + u64(len(sites))
    for x, y, z, _point_id in sites:
        raw += u32(x) + u32(y) + u32(z)
    return hashlib.sha256(raw).digest()


def signature(bits, k, sites, nodes, birth_supports):
    """Proposed V2 byte stream, not current SORTIES.md legacy SHA."""
    kids, _root = children_from_parent(nodes)
    raw = b'MHGP11TK' + b''.join(u64(x) for x in (2, bits, k, len(sites), len(nodes)))
    raw += geometry_digest(bits, sites)
    for i, node in enumerate(nodes):
        support = birth_supports[i]
        raw += u32(node['parent']) + u32(node['rank'])
        raw += bytes((node['kind'], len(support)))
        raw += b''.join(u32(x) for x in support)
        raw += u32(len(kids[i])) + b''.join(u32(c) for c in kids[i])
    return hashlib.sha256(raw).hexdigest()


def from_sp(doc):
    """Only SP-published fields. Existing source proof supplies canonical assumptions."""
    nodes, sites = doc['nodes'], doc['sites']
    lex_sites = sorted(range(len(sites)), key=lambda s: tuple(sites[s][:3]))
    own, begin = {}, 0
    for v in postorder(nodes):
        end = begin + nodes[v]['ball_count']
        own[v] = doc['balls'][begin:end]
        if len(own[v]) != end - begin:
            raise ValueError('ball_count')
        begin = end
    if begin != len(doc['balls']):
        raise ValueError('extra ball')
    births = []
    for v, node in enumerate(nodes):
        if node['kind'] == 0:
            if doc['k'] != 1 or v >= len(lex_sites):
                raise ValueError('K1 leaf')
            births.append((lex_sites[v],))
        elif node['kind'] == 1:
            if not own[v] or own[v][0]['rank'] != node['rank']:
                raise ValueError('missing birth ball')
            births.append(tuple(own[v][0]['supports'][0]))
        elif node['kind'] == 2:
            births.append(())
        else:
            raise ValueError('kind')
    return signature(doc['bits'], doc['k'], sites, nodes, births)


def from_full_engine(doc, catalogue, birth_keys):
    """FullDomain supplies catalogue S*, absent from legacy FUL1 bytes."""
    births = []
    for node, key in zip(doc['nodes'], birth_keys):
        if node['kind'] == 0:
            births.append((key,))
        elif node['kind'] == 1:
            births.append(tuple(catalogue[key]['support']))
        else:
            births.append(())
    return signature(doc['bits'], doc['k'], doc['sites'], doc['nodes'], births)


def node(parent, rank, kind, count):
    return dict(parent=parent, rank=rank, kind=kind, ball_count=count)


def main():
    singleton = dict(bits=21, k=1, sites=[(3, 2, 1, 19)],
                     nodes=[node(NONE, 0, 0, 0)], balls=[])
    pair = dict(bits=21, k=1, sites=canonical_sites([(0, 2, 0, 91), (2, 0, 0, 5)]),
                nodes=[node(2, 0, 0, 0), node(2, 0, 0, 0), node(NONE, 1, 2, 1)],
                balls=[dict(rank=1, supports=[(0, 1)])])
    line = dict(bits=21, k=2, sites=[(0, 0, 0, 21), (1, 0, 0, 8), (2, 0, 0, 42)],
                nodes=[node(2, 1, 1, 1), node(2, 1, 1, 1), node(NONE, 2, 2, 1)],
                balls=[dict(rank=1, supports=[(0, 1)]), dict(rank=1, supports=[(1, 2)]),
                       dict(rank=2, supports=[(0, 2)])])
    fixtures = [(singleton, [], [0]),
                (pair, [dict(support=(0, 1))], [1, 0, NONE]),
                (line, [dict(support=(0, 1)), dict(support=(1, 2)), dict(support=(0, 2))],
                 [0, 1, NONE])]
    results = []
    for doc, catalogue, keys in fixtures:
        sp_hash = from_sp(doc)
        check(sp_hash == from_full_engine(doc, catalogue, keys), 'SP/FullDomain payload mismatch')
        other = copy.deepcopy(doc)
        other['sites'] = [tuple(r[:3]) + (1000 + j,) for j, r in enumerate(doc['sites'])]
        check(from_sp(other) == sp_hash, 'PointId must be excluded from semantic signature')
        check(json.dumps(other, sort_keys=True) != json.dumps(doc, sort_keys=True),
              'artifact necessarily differs on this ID change')
        for perm in itertools.permutations(doc['sites']):
            check(canonical_sites(perm) == doc['sites'], 'row permutation canonicalization')
        wider = copy.deepcopy(doc)
        wider['bits'] = 24
        check(from_sp(wider) != sp_hash, 'explicit profile namespace')
        changed = copy.deepcopy(doc)
        changed['sites'][0] = (changed['sites'][0][0] + 4,) + changed['sites'][0][1:]
        check(from_sp(changed) != sp_hash, 'geometry namespace')
        results.append(dict(k=doc['k'], sites=len(doc['sites']), nodes=len(doc['nodes']),
                            balls=len(doc['balls']), tree_signature_v2=sp_hash))
    check(pair['sites'][0][:3] == (2, 0, 0), 'Morton order')
    check(sorted(range(2), key=lambda s: pair['sites'][s][:3]) == [1, 0], 'K1 lex/Morton mismatch exercised')
    line_populations = [(0, 1), (1, 2), (0, 1, 2)]
    incidences = [(b, part) for b, population in enumerate(line_populations)
                  for part in itertools.combinations(population, 2)]
    distinct = {part for _, part in incidences}
    repeated = {part: sum(q == part for _, q in incidences) for part in sorted(distinct)}
    check([math.comb(len(p), 2) for p in line_populations] == [1, 1, 3], 'per-ball K-parts')
    check(len(incidences) == 5 and len(distinct) == 3, 'incidence/global distinction')
    check(repeated == {(0, 1): 2, (0, 2): 1, (1, 2): 2}, 'repeated K-parts')
    check(sum(math.comb(24, q) for q in range(2, 5)) == 12926, 'u16 support_count')
    max_kparts = max(math.comb(p + m, k) for k in range(1, 13)
                     for p in range(k) for m in range(1, 25))
    max_cofaces_bound = max(math.comb(p + m, k + 1) for k in range(1, 13)
                           for p in range(k) for m in range(1, 25))
    check(max_kparts < 1 << 32 and max_cofaces_bound < 1 << 32, 'per-ball u32 upper bounds')
    check((NONE - 1) * max(max_kparts, max_cofaces_bound) < 1 << 64, 'aggregate u64 bound')
    paths = ['l0__morsehgp3D_v11__reference__test_supports.py',
             'l0__morsehgp3D_v11__reference__hgp11_ref__supports.py',
             'l0__morsehgp3D_v11__reference__ref_mutants.py']
    trees = {p: ast.parse((HERE/'sources'/p).read_text()) for p in paths}
    check(not any(isinstance(n, ast.Assert) for t in trees.values() for n in ast.walk(t)),
          'S1 judge/oracle must not rely on assert')
    pilot = trees[paths[0]]
    assignments = {target.id: n.value for n in pilot.body if isinstance(n, ast.Assign)
                   for target in n.targets if isinstance(target, ast.Name)}
    def literal_dict_call(name):
        call = assignments[name]
        return {kw.arg: ast.literal_eval(kw.value) for kw in call.keywords}
    expected = literal_dict_call('SUITE_EXACT')
    floors = literal_dict_call('FLOORS')
    check(expected['clouds'] == 205 and expected['orders'] == 935, 'declared suite identity')
    check(all(expected[key] >= val for key, val in floors.items()), 'declared floors reachable')
    mutations = trees[paths[2]]
    mutant_ast = next(n.value for n in mutations.body if isinstance(n, ast.Assign)
                      and any(isinstance(t, ast.Name) and t.id == 'SUPPORT_MUTANTS' for t in n.targets))
    mutant_names = [ast.literal_eval(x) for x in mutant_ast.keys]
    check(len(mutant_names) == 7, 'seven declared causal oracle mutants')
    return dict(schema='mhgp11.audit.supports_d2_proposal.v1', checks=checks,
                scope='private stdlib proposal model + AST declarations only; no S1/native/G4 qualification',
                signatures=results,
                line3_k2=dict(per_ball=[1, 1, 3], incidence_total=len(incidences),
                              unique_parts=len(distinct), repeated_parts={str(k): v for k,v in repeated.items()}),
                limits=dict(support_count_max=12926, kparts_max=max_kparts,
                            cofaces_per_ball_upper_bound=max_cofaces_bound),
                s1_declared_not_executed=dict(expected=expected, floors=floors, mutants=mutant_names))


if __name__ == '__main__':
    print(json.dumps(main(), ensure_ascii=False, sort_keys=True, indent=2))
