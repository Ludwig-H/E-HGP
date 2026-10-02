"""Controle documentaire AST et preuve Fraction autonome, sans importer la reference."""

import ast
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys


HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'source' / 'reference'
guards = 0


def require(condition, label):
    global guards
    guards += 1
    if not condition:
        raise RuntimeError(label)


def imports(path):
    tree = ast.parse(path.read_text(), filename=str(path))
    return [(node.level, node.module, [name.name for name in node.names])
            for node in ast.walk(tree) if isinstance(node, ast.ImportFrom)], tree


all_imports = {}
for name in ('definition', 'constructive', 'judge', 'model', 'intgeom'):
    rows, tree = imports(SOURCE / 'hgp11_ref' / (name + '.py'))
    all_imports[name] = rows
    if name == 'model':
        funcs = [n.name for n in tree.body if isinstance(n, ast.FunctionDef)]
        require(funcs == ['mask_of', 'members'], 'modele sans algorithme structurel')
        record = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == 'OrderResult')
        require([n.name for n in record.body if isinstance(n, ast.FunctionDef)] == ['__init__'],
                'OrderResult enregistrement sans parent/ancetre/coupe')
    if name in ('definition', 'constructive', 'judge'):
        allowed = {
            'definition': {'Cut', 'Entry', 'InvariantError', 'Node', 'OrderResult', 'mask_of'},
            'constructive': {'Cut', 'Entry', 'InvariantError', 'Node', 'OrderResult'},
            'judge': {'InvariantError'},
        }[name]
        shared = [set(row[2]) for row in rows if row[:2] == (1, 'model')]
        require(len(shared) == 1 and shared[0] == allowed, 'imports communs ' + name)
    if name == 'definition':
        require(all(row[1] != 'constructive' and 'intgeom' not in row[2] for row in rows),
                'A sans B ou geometrie B')
    if name == 'constructive':
        require(all(row[1] != 'definition' for row in rows), 'B sans A')
rows, tree = imports(SOURCE / 'interval_oracle.py')
all_imports['interval_oracle'] = rows
require(rows == [(0, 'fractions', ['Fraction'])], 'attendu intervalles sans paquet HGP')
require(not any(isinstance(n, ast.Import) for n in ast.walk(tree)),
        'attendu intervalles sans import indirect')

# Temoin analytique : une boule critique inerte peut manquer sans changer FULL.
# K1 : Gamma est le graphe complet des sites, chaque arete ayant beta=d^2/4.
points = ((0, 0), (4, 0), (2, 3))
edges = [(Fraction(sum((x-y)**2 for x, y in zip(points[i], points[j])), 4), i, j)
         for i in range(3) for j in range(i+1, 3)]
require(sorted(level for level, _i, _j in edges) == [Fraction(13, 4)] * 2 + [Fraction(4)],
        'niveaux exacts du triangle')
require(sum((x-y)**2 for x, y in zip(points[2], (2, 0))) > 4,
        'coquille AB complete; C exterieur')


def partition(edge_set, level, closed):
    roots = list(range(3))
    def find(i):
        while roots[i] != i:
            i = roots[i]
        return i
    for beta, i, j in edge_set:
        if beta <= level if closed else beta < level:
            roots[find(j)] = find(i)
    return sorted(sorted(i for i in range(3) if find(i) == r) for r in set(find(i) for i in range(3)))


pruned = [edge for edge in edges if edge[1:] != (0, 1)]
for level in (Fraction(0), Fraction(13, 8), Fraction(13, 4), Fraction(29, 8), Fraction(4), Fraction(5)):
    for closed in (False, True):
        require(partition(edges, level, closed) == partition(pruned, level, closed),
                'boule critique inerte invisible a FULL')

# Lire uniquement les coordonnees litterales : aucun generateur ni oracle importe.
family_ast = ast.parse((SOURCE / 'hgp11_ref' / 'families.py').read_text())
fixtures = next(node.value for node in family_ast.body if isinstance(node, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'FIXTURES' for t in node.targets))
triangles = {}
for entry in fixtures.elts:
    name = ast.literal_eval(entry.elts[0])
    if name not in ('two_triangles', 'two_triangles_1998', 'two_triangles_1700'):
        continue
    pts = ast.literal_eval(entry.elts[1])
    d2 = lambda i, j: sum((x-y)**2 for x, y in zip(pts[i], pts[j]))
    require([d2(0, 1), d2(0, 2), d2(1, 2)] == [4000000, 3999824, 3999824],
            'triangle ABC approche, non equilateral')
    require([d2(3, 4), d2(3, 5), d2(4, 5)] == [3999824, 3999824, 4000000],
            'triangle DEF approche, non equilateral')
    bridge = {'two_triangles': 2000, 'two_triangles_1998': 1998, 'two_triangles_1700': 1700}[name]
    require(d2(2, 3) == bridge**2, 'pont exact declare')
    triangles[name] = {'ABC_squared_sides': [d2(0, 1), d2(0, 2), d2(1, 2)],
                       'bridge_squared': d2(2, 3)}
triangle_beta = Fraction(3999824**2, 4 * 1732**2)
require(triangle_beta == Fraction(249978000484, 187489), 'date circonscrite approchee')
require(triangle_beta > 1000000 > 999956, 'fenetre sept paires avant fusion triangle')

if '--manifest' in sys.argv:
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        require(hashlib.sha256((HERE / name).read_bytes()).hexdigest() == digest, 'empreinte ' + name)

print(json.dumps({'guards': guards, 'imports': all_imports,
                  'inert_ball': {'k': 1, 'points': points, 'removed_ball': 'AB',
                                 'beta_AB': '4', 'connecting_beta_AC_BC': '13/4',
                                 'same_FULL_components': True},
                  'approximate_thesis_fixtures': triangles,
                  'triangle_squared_circumradius': str(triangle_beta),
                  'executed_reference_or_engine': False}, sort_keys=True, indent=2))
