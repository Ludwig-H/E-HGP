"""Derivations exactes autonomes ; aucun import, binaire ou test produit."""

from fractions import Fraction as F
from itertools import permutations
import hashlib
import json
from pathlib import Path
import sys


HERE = Path(__file__).resolve().parent
guards = 0


def require(condition, label):
    global guards
    guards += 1
    if not condition:
        raise RuntimeError(label)


# coefficient * M^degree, magnitude strictement inferieure ; capacite native.
BOUNDS = {
    'dot': (3, 2, 2, 63), 'cross': (2, 2, 1, 63),
    'det': (6, 3, 3, 127), 'N3': (24, 5, 5, 127),
    'D3': (24, 4, 5, 127), 'N4': (18, 4, 5, 127),
    'D4': (12, 3, 4, 127), 'midpoint_member': (96, 5, 7, 127),
    'side': (216, 6, 8, None), 'center_orientation': (288, 7, 9, None),
    'q3_level_num': (27, 6, 5, None), 'q3_level_den': (48, 4, 6, None),
    'q4_level_num': (972, 8, 12, None), 'q4_level_den': (144, 6, 8, None),
}
profiles = {}
for bits in (18, 21, 24):
    profiles[bits] = {}
    for name, (coefficient, degree, extra, native) in BOUNDS.items():
        require(coefficient <= 2**extra, 'coefficient ' + name)
        width = degree * bits + extra
        if native is not None:
            require(width <= native, 'capacite native ' + name)
        profiles[bits][name] = width
    require(3*bits+3 <= 127, 't q3 avant cross')
    require(4*bits+4 <= 127, 'uu*vv q3 avant large')
    require(4*bits+6 <= 127, '4*g q3 avant large')
    require(5*bits+6 <= 127, 'N+D*offset avant large')
future32 = {name: degree*32+extra for name, (_c, degree, extra, native) in BOUNDS.items()
            if native is not None and degree*32+extra > native}
require(set(future32) == {'dot', 'cross', 'N3', 'D3', 'N4', 'midpoint_member'},
        '32 bits necessite aussi les intermediaires natifs')


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return [x-y for x, y in zip(a, b)]


def solve(rows, rhs):
    matrix = [[F(v) for v in row]+[F(value)] for row, value in zip(rows, rhs)]
    size = len(rhs)
    for col in range(size):
        pivot = next((j for j in range(col, size) if matrix[j][col]), None)
        if pivot is None:
            return None
        matrix[col], matrix[pivot] = matrix[pivot], matrix[col]
        factor = matrix[col][col]
        matrix[col] = [v/factor for v in matrix[col]]
        for j in range(size):
            if j != col:
                factor = matrix[j][col]
                matrix[j] = [v-factor*w for v, w in zip(matrix[j], matrix[col])]
    return [row[-1] for row in matrix]


def center4(points):
    a = points[0]
    return solve([[2*v for v in sub(p, a)] for p in points[1:]],
                 [dot(p, p)-dot(a, a) for p in points[1:]])


def barycentric(center, points):
    return solve([[p[j] for p in points] for j in range(3)]+[[1]*4], list(center)+[1])


fixtures = [
    ('regular', [(0,0,0),(2,2,0),(2,0,2),(0,2,2)], [F(1)]*3, F(3), True),
    ('face_weight_zero', [(0,0,0),(4,0,0),(2,3,0),(2,0,2)], [F(2),F(5,6),F(0)], F(169,36), False),
    ('obtuse_prefix', [(5,2,1),(10,5,5),(9,8,5),(1,5,8)], [F(5)]*3, F(25), True),
    ('ill_conditioned_nonconvex', [(0,0,0),(225077,1,0),(225068,1,0),(152369,7,1)], None, None, False),
]
prepared = []
expected = []
for name, points, known_center, known_level, strict in fixtures:
    center = center4(points)
    require(center is not None, name+' centre existe')
    if known_center is not None:
        require(center == known_center, name+' centre connu')
    level = dot(sub(points[0], center), sub(points[0], center))
    if known_level is not None:
        require(level == known_level, name+' niveau connu')
    require(all(dot(sub(p,center),sub(p,center)) == level for p in points), name+' coquille complete')
    weights = barycentric(center, points)
    require(weights is not None and all(w>0 for w in weights) == strict, name+' interiorite stricte')
    for order in permutations(points):
        require(center4(order) == center, name+' permutation du support')
    if name == 'obtuse_prefix':
        require(not all(dot(sub(points[j],points[i]),sub(points[k],points[i]))>0
                        for i,j,k in ((0,1,2),(1,0,2),(2,0,1))), 'prefixe q3 obtus garde q4 strict')
    if name == 'ill_conditioned_nonconvex':
        require(any(v<0 or v>262143 for v in center), 'centre non convexe hors boite permis')
    # Protocole de num/probe, queries de coquille : 4 lignes par support.
    for query in points:
        prepared.append(' '.join(map(str,[4]+[v for p in points+[query] for v in p])))
    expected.append({'name':name,'points':points,'center':[str(v) for v in center],
                     'level':str(level),'barycentric':[str(w) for w in weights],
                     'strictly_inside':strict,'support_sides':[0]*4})

require(len(prepared)==16, 'plancher fixture G4')
if '--write-fixtures' in sys.argv:
    (HERE/'G4_INPUT.txt').write_text('\n'.join(prepared)+'\n')
    (HERE/'G4_EXPECTED.json').write_text(json.dumps(expected,sort_keys=True,indent=2)+'\n')
if '--manifest' in sys.argv:
    for line in (HERE/'SHA256SUMS').read_text().splitlines():
        digest,name=line.split('  ',1)
        require(hashlib.sha256((HERE/name).read_bytes()).hexdigest()==digest,'empreinte '+name)
print(json.dumps({'guards':guards,'profiles':profiles,'future32_native_changes':future32,
                  'fixtures':expected,'executed_product':False,'GCP_used':False},sort_keys=True,indent=2))
