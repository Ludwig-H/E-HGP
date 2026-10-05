#!/usr/bin/env python3
"""Bounded Python-only S7 check: normative binary encoding from exact S1, no native execution."""
import copy
import hashlib
import json
import pathlib
import struct
import sys
from fractions import Fraction
from itertools import combinations

sys.dont_write_bytecode = True
HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE / 'snapshot/morsehgp3D_v11'
sys.path.insert(0, str(ROOT / 'tests/cli'))
import cli_supports_oracle as C
from hgp11_ref.definition import circumsphere
from hgp11_ref.supports import barycentric, split

F, H = C.formats, C.hierarchy_fraction
NONE = (1 << 32) - 1


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def morton(p):
    return sum(((p[d] >> i) & 1) << (3*i+d) for i in range(24) for d in range(3))


def catalog_levels(points, k):
    """Independently enumerate all positive minimal supports, using Gram/Fraction (not reader cross products)."""
    shapes = {}
    queries = 0
    for q in (2, 3, 4):
        for part in combinations(points, q):
            queries += 1
            cs = circumsphere(part)
            w = None if cs is None else barycentric(part, cs[0])
            expected = cs is not None and w is not None and min(w) > 0
            got = F.sphere_of(part)
            require((got is not None) == expected, 'sphere positivity differs from Gram')
            if got is not None:
                require(tuple(Fraction(c, got[1]) for c in got[0]) == cs[0] and
                        Fraction(got[2], got[1]**2) == cs[1], 'sphere coordinates/level differ from Gram')
            if expected:
                key = cs
                shapes[key] = min(q, shapes.get(key, 5))
    levels = {Fraction(0)}
    for (c, lv), qmin in shapes.items():
        inner, shell = split(points, c, lv)
        if len(inner) + qmin <= k + 1:
            levels.add(lv)
    return {v: i for i, v in enumerate(sorted(levels))}, queries


def encode(doc, bits, rank):
    pts = [tuple(p) for p in doc['sites']]
    at = sorted(range(len(pts)), key=lambda i: morton(pts[i]))
    site = {pts[i]: j for j, i in enumerate(at)}
    nodes = doc['nodes']
    balls = []
    for b in doc['balls']:
        row = copy.deepcopy(b)
        row['supports'] = sorted([tuple(sorted(site[tuple(p)] for p in q)) for q in b['supports']],
                                 key=lambda q: (len(q), q))
        balls.append(row)
    balls.sort(key=lambda b: (nodes[b['node']]['post'], rank[Fraction(b['level'])],
                              b['supports'][0] + (NONE,)*(4-len(b['supports'][0]))))
    own = [[b for b in balls if b['node'] == v] for v in range(len(nodes))]
    supports = [q for b in balls for q in b['supports']]
    prior = [v for b in balls for v in b['prior']]
    columns = []
    for d in range(3):
        columns.append(('I', [pts[i][d] for i in at], 'xyz'[d]))
    columns.extend([('I', [doc['ids'][i] for i in at], 'point_id'),
                    ('I', [NONE if v['parent'] is None else v['parent'] for v in nodes], 'parent'),
                    ('I', [rank[Fraction(v['level'])] for v in nodes], 'rank'),
                    ('B', [v['kind'] for v in nodes], 'kind'),
                    ('I', [len(b) for b in own], 'ball_count'),
                    ('I', [rank[Fraction(b['level'])] for b in balls], 'ball_rank'),
                    ('I', [len(b['prior']) for b in balls], 'prior_count'),
                    ('H', [len(b['supports']) for b in balls], 'support_count'),
                    ('B', [C.ROLE_NAMES.index(b['role']) for b in balls], 'role'),
                    ('B', [b['p'] for b in balls], 'p'), ('B', [b['m'] for b in balls], 'm'),
                    ('B', [len(q) for q in supports], 'arity'),
                    ('I', [i for q in supports for i in q], 'sites'), ('I', prior, 'prior')])
    raw, offsets, pads = bytearray(136), {}, []
    starts = []
    for typ, values, name in columns:
        offsets[name] = len(raw)
        if name in ('x', 'parent', 'ball_rank', 'arity', 'prior'):
            starts.append(len(raw))
        raw.extend(struct.pack('<%d%s' % (len(values), typ), *values))
        while len(raw) % 8:
            pads.append(len(raw)); raw.append(0)
    words = [1, bits, doc['k'], doc['n'], len(nodes), len(nodes)-1, len(balls), len(supports),
             sum(map(len, supports)), len(prior)] + starts + [len(raw)]
    raw[:136] = b'MHGP11SP' + struct.pack('<16Q', *words)
    # Independent V2 serialization from the exact Definition tree, not decoded supports.
    geometry = hashlib.sha256(b'MHGP11GX' + struct.pack('<QQ', bits, doc['n']) +
                              b''.join(struct.pack('<III', *pts[i]) for i in at)).digest()
    signature = bytearray(b'MHGP11TK' + struct.pack('<QQQQQ', 2, bits, doc['k'], doc['n'], len(nodes)) + geometry)
    for v, node in enumerate(nodes):
        birth = ()
        if node['kind'] == 0:
            birth = (site[tuple(int(Fraction(c)) for c in node['birth_center'])],)
        elif node['kind'] == 1:
            birth = own[v][0]['supports'][0]
        signature.extend(struct.pack('<IIBB%dI' % len(birth), NONE if node['parent'] is None else node['parent'],
                                     rank[Fraction(node['level'])], node['kind'], len(birth), *birth))
        signature.extend(struct.pack('<I%dI' % len(node['children']), len(node['children']), *node['children']))
    return raw, offsets, pads, hashlib.sha256(signature).hexdigest()


def compare(raw, doc, bits):
    decoded = F.read_supports(raw, bits)
    difference = C.attach_fraction.first_difference(H.project(C.oracle_document(decoded)), H.project(doc), 'doc')
    require(difference is None, 'decoded exact S1 difference: ' + str(difference))
    return decoded


fixtures = H.test_supports.FIXTURES
clouds = [(name, fixtures[name][0], ks) for name, ks in
          [('carre', (1, 3)), ('triangle_droit', (2,)), ('passagere', (1,)),
           ('growth_abcz', (3,)), ('cube', (1,))]]
clouds.append((C.attach_fraction.WITNESS_K10[0], C.attach_fraction.WITNESS_K10[1], (10, 12)))
gate = H.mhgp11_gate.Gate('bounded_s7')
cases, compared, excluded = H.oracle_cases(gate, 21, clouds)
require(gate.failures == 0 and len(cases) == 8 and excluded == 0, 'bounded fixture preparation')
rows, binaries, queries = [], [], 0
for name, points, ids, k, doc, oracle in cases:
    ranks, checks = catalog_levels([tuple(p) for p in points], k)
    queries += checks
    raw, offsets, pads, digest = encode(doc, 21, ranks)
    decoded = compare(raw, doc, 21)
    require(decoded.tree_signature() == digest, 'independent V2 signature differs')
    rows.append(dict(name=name, k=k, nodes=decoded.N, balls=decoded.B, supports=decoded.S,
                     tree_k_sha256=digest, file_sha256=hashlib.sha256(raw).hexdigest()))
    binaries.append((raw, offsets, pads, doc, ranks))

# Coordinates at u24's upper edge exercise large cross products; no native u24 qualification is implied.
translated = [tuple(c + (1 << 24) - 3 for c in p) for p in fixtures['cube'][0]]
_unused, edge_queries = catalog_levels(translated, 1)

mutations = {}
def reject(name, changed, bits=21):
    try:
        F.read_supports(changed, bits)
    except ValueError as error:
        mutations[name] = str(error)
    else:
        raise RuntimeError('reader accepted ' + name)

raw, offsets, pads, doc, ranks = binaries[0]
x = bytearray(raw); struct.pack_into('<I', x, offsets['point_id']+4, struct.unpack_from('<I', x, offsets['point_id'])[0])
reject('duplicate_PointId', x)
require(pads, 'padding witness absent')
x = bytearray(raw); x[pads[0]] = 1; reject('nonzero_padding', x)
x = bytearray(raw); struct.pack_into('<I', x, offsets['rank'], 1); reject('site_birth_nonzero_rank', x)
ci = next(i for i, c in enumerate(cases) if c[0] == 'passagere')
x = bytearray(binaries[ci][0]); offset = binaries[ci][1]['role']; x[offset] = F.ROLE_INTERNAL
reject('merge_changed_to_internal', x)

# Completeness is deliberately an external exact-oracle gate: shell and every Q are not self-authenticating.
ci = next(i for i, c in enumerate(cases) if c[0] == 'cube')
cube = copy.deepcopy(cases[ci][4])
b = next(b for b in cube['balls'] if any(len(q) == 4 for q in b['supports']))
require(any(n == 0 for n in b['cofaces_support']), 'zero-coface tetra witness absent')
keep = [i for i, q in enumerate(b['supports']) if len(q) != 4]
for key in ('supports', 'cofaces_support', 'gabriel_cofaces_support'):
    b[key] = [b[key][i] for i in keep]
changed = encode(cube, 21, binaries[ci][4])[0]
decoded = F.read_supports(changed, 21)
diff = C.attach_fraction.first_difference(H.project(C.oracle_document(decoded)), H.project(cases[ci][4]), 'doc')
require(diff is not None, 'exact differential failed to detect omitted zero-coface supports')

result = dict(pin='966a351be1b58089fd13bc44a8dbafc56d87999a', scope='Python normative encoder + reader + exact S1; no native',
              native_executed=False, clouds=compared, orders=len(cases), gram_sphere_queries=queries,
              u24_edge_gram_sphere_queries=edge_queries, cases=rows, reader_rejected=mutations,
              Q_completeness_external_gate=dict(reader_accepts=True, differential_rejects=True, first_difference=diff))
print(json.dumps(result, sort_keys=True, separators=(',', ':')))
