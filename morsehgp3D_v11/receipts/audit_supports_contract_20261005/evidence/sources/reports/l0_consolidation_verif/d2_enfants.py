"""Enfants de la fusion de niveau 1681/25 du temoin D2 a K = 2 : vrai Gamma_2 (all_vertices=None : tous les
sommets, toutes les liaisons), puis liaisons de W_2 seules, sommets gardes ou retires. Bibliotheque standard."""
import itertools, os, sys
from fractions import Fraction
REF = "/workspaces/E-HGP/build/v11-impl-l0/morsehgp3D_v11/reference"
sys.path.insert(0, REF)
import ref_mutants
S = ref_mutants.load_private(os.path.join(REF, 'hgp11_ref'), 'x_stage')['supports']
names = 'ABCZW'
def run(pts, k, target, all_vertices):
    sup = S.Supports(pts)
    D = sup.definition
    _b, _g, every = sup._window(k)
    def kept(part):
        level, center, _c = D.meb(part)
        ball = every.get((center, level))
        return ball is not None and ball.p + ball.q <= k + 1
    at = {}
    for part in itertools.combinations(range(sup.n), k):
        if all_vertices is None or all_vertices or kept(part):
            at.setdefault(D.beta(part), ([], []))[0].append(part)
    for part in itertools.combinations(range(sup.n), k + 1):
        if kept(part) or all_vertices is None:
            at.setdefault(D.beta(part), ([], []))[1].append(part)
    parent = {}
    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    for level in sorted(at):
        vertices, edges = at[level]
        old_members = {}
        for x in parent:
            old_members.setdefault(find(x), set()).add(x)
        for x in vertices:
            parent[x] = x
        for g in edges:
            faces = [g[:s] + g[s + 1:] for s in range(k + 1)]
            for face in faces:
                parent.setdefault(face, face)
            for face in faces[1:]:
                parent[find(face)] = find(faces[0])
        if level == target:
            groups = {}
            for root, mem in old_members.items():
                groups.setdefault(find(root), []).append(sorted(''.join(names[i] for i in p) for p in mem))
            for kids in groups.values():
                if len(kids) >= 2:
                    print(str(level), sorted(kids))
d2 = [(2, 10, 0), (18, 10, 0), (10, 20, 0), (9, 3, 0), (11, 3, 0)]
print('vrai'); run(d2, 2, Fraction(1681, 25), None)
print('sommets gardes'); run(d2, 2, Fraction(1681, 25), True)
print('sommets retires'); run(d2, 2, Fraction(1681, 25), False)
