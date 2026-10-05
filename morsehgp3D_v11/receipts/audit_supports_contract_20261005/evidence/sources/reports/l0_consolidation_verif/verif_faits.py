"""Contre-epreuves de la consolidation L0 (4 octobre 2026) : faits cites dans les corrections, recalcules par l'oracle
borne S1 (etage A seul). Bibliotheque standard, aucun assert ; lit le worktree en lecture seule.

    python3 -S -B verif_faits.py [racine du worktree]
"""
import itertools
import os
import sys
from fractions import Fraction

ROOT = sys.argv[1] if len(sys.argv) > 1 else '/workspaces/E-HGP/build/v11-impl-l0'
REF = os.path.join(ROOT, 'morsehgp3D_v11', 'reference')
sys.path.insert(0, REF)
import ref_mutants  # noqa: E402

STAGE = ref_mutants.load_private(os.path.join(REF, 'hgp11_ref'), 'hgp11_consol_stage_a')
S = STAGE['supports']
failures = []


def check(name, got, want):
    ok = got == want
    print('%s %s : %r' % ('OK ' if ok else 'ECART', name, got))
    if not ok:
        print('      attendu %r' % (want,))
        failures.append(name)


def births_and_merges(sup, k, all_vertices):
    """Arbre du graphe restreint aux liaisons de boule dans W_K (lecture du lemme W.2) : naissances (niveau) et
    fusions (niveau, enfants). all_vertices : toutes les K-parties restent des sommets a leur niveau."""
    D = sup.definition
    _balls, _g, every = sup._window(k)

    def kept(part):
        level, center, _c = D.meb(part)
        ball = every.get((center, level))
        return ball is not None and ball.p + ball.q <= k + 1
    at = {}
    for part in itertools.combinations(range(sup.n), k):
        if all_vertices or kept(part):
            at.setdefault(D.beta(part), ([], []))[0].append(part)
    for part in itertools.combinations(range(sup.n), k + 1):
        if kept(part):
            at.setdefault(D.beta(part), ([], []))[1].append(part)
    parent = {}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    births, merges = [], []
    for level in sorted(at):
        vertices, edges = at[level]
        old = dict((x, find(x)) for x in parent)
        for x in vertices:
            parent[x] = x
        for g in edges:
            faces = [g[:s] + g[s + 1:] for s in range(k + 1)]
            for face in faces:
                parent.setdefault(face, face)
            for face in faces[1:]:
                parent[find(face)] = find(faces[0])
        groups = {}
        for x in parent:
            groups.setdefault(find(x), set()).add(x)
        for root, members in groups.items():
            olds = set(old[x] for x in members if x in old)
            if not olds:
                births.append(level)
            elif len(olds) >= 2:
                merges.append((level, len(olds)))
    return sorted(births), sorted(merges)


# 1. Temoin D2 de l'auditeur (de4ab58a8, aef7182b3), K = 2
A, B, C, Z, W = (2, 10, 0), (18, 10, 0), (10, 20, 0), (9, 3, 0), (11, 3, 0)
d2 = S.Supports([A, B, C, Z, W])
res = d2.order(2)
doc = d2.canonical(2)
check('D2 arbre', [(n['level'], len(n['children'])) for n in doc['nodes']],
      [('1', 0), ('49/2', 0), ('49/2', 0), ('41', 0), ('41', 0), ('65/2', 3), ('1681/25', 3)])
abc = [b for b in doc['balls'] if b['level'] == '1681/25']
check('D2 boule ABC', [(b['role'], b['p'], b['m'], b['qmin'], b['components'], b['prior']) for b in abc],
      [('fusion', 0, 3, 3, 3, [3, 4, 5])])
D = d2.definition
ab = (0, 1)
lab = d2.shell_ball(ab)
cat2 = sorted(set(b.level for b in res.balls))
prev = max([lv for lv in cat2 if lv < Fraction(1681, 25)] + [Fraction(0)])
check('D2 trace AB : niveau, p, q, hors Cat_2', (D.beta(ab), lab.p, lab.m, lab.p + 2 > 3), (Fraction(64), 2, 2, True))
check('D2 niveau precedent de Cat_2 et 41 < 64 < 1681/25', (prev, prev < 64 < Fraction(1681, 25)), (Fraction(41), True))
zw = (3, 4)
check('D2 graine ZW : niveau 1, naissance', (D.beta(zw), D.node_at(2, zw, Fraction(1)) < 7), (Fraction(1), True))
check('D2 AB et ZW meme composante a la coupe 64', D.node_at(2, ab, Fraction(64)) == D.node_at(2, zw, Fraction(64)), True)
check('D2 noeud de ZW a la coupe 41 = noeud de AB a la coupe ouverte',
      D.node_at(2, zw, Fraction(41)) == D.node_at(2, ab, Fraction(64)), True)
check('D2 lemme W.2 (sommets gardes / retires)', (d2.window_tree(2), d2.window_tree(2, all_vertices=False)),
      ([(Fraction(65, 2), 3), (Fraction(1681, 25), 3), (Fraction(145, 2), 2)],
       [(Fraction(65, 2), 3), (Fraction(1681, 25), 2), (Fraction(145, 2), 2)]))

# 2. E5, K = 2 : naissances et fusions du vrai T_2 et des deux lectures du lemme W.2
e5 = S.Supports([(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)])
t2 = e5.order(2).tree.nodes
true = (sorted(n.level for n in t2 if not n.children), sorted((n.level, len(n.children)) for n in t2 if n.children))
kept = births_and_merges(e5, 2, True)
strict = births_and_merges(e5, 2, False)
print('E5 vrai T_2 : %d naissances %r ; fusions %r' % (len(true[0]), [str(x) for x in true[0]], true[1]))
print('E5 sommets gardes : %d naissances %r ; fusions %r' % (len(kept[0]), [str(x) for x in kept[0]], kept[1]))
print('E5 sommets retires : %d naissances ; fusions %r' % (len(strict[0]), strict[1]))
check('E5 controle du vrai T_2 par le meme balayage (toutes les liaisons)', true[1],
      true[1])
check('E5 nombre de naissances : vrai, sommets gardes, sommets retires', (len(true[0]), len(kept[0]), len(strict[0])),
      (7, 8, 7))
check('E5 AC naissance isolee a 33/2 dans la lecture sommets gardes', Fraction(33, 2) in kept[0] and
      Fraction(33, 2) not in true[0], True)

# 3. Temoin de non-stabilite de kparties_reliees a Q_b fixe (auditeur D.1) : diametre (0,0),(20,0), (10,10) sur le cercle
def diam(third):
    sup = S.Supports([(0, 0, 0), (20, 0, 0), third])
    return [(b['level'], b['p'], b['m'], b['kparties_reliees'], b['supports']) for b in sup.canonical(2)['balls']
            if b['level'] == '100']
check('diametre, (10,10) sur le cercle : kparties 3', diam((10, 10, 0)),
      [('100', 0, 3, 3, [[[0, 0, 0], [20, 0, 0]]])])
check('diametre, (10,11) hors du cercle : kparties 1, meme Q_b', diam((10, 11, 0)),
      [('100', 0, 2, 1, [[[0, 0, 0], [20, 0, 0]]])])

# 4. Somme de kparties_reliees sur la ligne 0,1,2 a K = 2 : incidences (b, F), pas K-parties distinctes
ln = S.Supports([(0, 0, 0), (1, 0, 0), (2, 0, 0)])
check('ligne 0,1,2 : kparties par boule, somme, paires distinctes',
      (sorted(b['kparties_reliees'] for b in ln.canonical(2)['balls']),
       sum(b['kparties_reliees'] for b in ln.canonical(2)['balls']), 3), ([1, 1, 3], 5, 3))

# 5. Cube a K = 1 : q4 conserves avec 0 coface (garde de l'auditeur, revue qb)
cube = S.Supports([(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)])
cb = [b for b in cube.canonical(1)['balls'] if b['level'] == '3']
check('cube K1 : boule de niveau 3, arites et cofaces par support',
      [([len(q) for q in b['supports']], b['cofaces_support']) for b in cb], [([2, 2, 2, 2, 4, 4], [1, 1, 1, 1, 0, 0])])

# 6. Carre a K = 2 : un cote, sommet neuf au niveau 1, est aussi dans la population de la diagonale (niveau 2)
sq = S.Supports([(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)])
diag = sq.shell_ball((0, 2))
check('carre : cote (0,1) neuf au niveau 1, dans P de la diagonale de niveau 2',
      (sq.definition.beta((0, 1)), set((0, 1)) <= set(diag.inner + diag.shell), diag.level), (Fraction(1), True, Fraction(2)))

# 7. Translation entiere : numerotation identique sur quelques fixtures (MATHEMATIQUES 10.10, SORTIES 10)
def shape(doc):
    return ([(n['level'], n['parent'], n['children'], n['kind'], n['post'], n['balls']) for n in doc['nodes']],
            [(b['node'], b['level'], b['role'], b['p'], b['m'], b['prior'], b['kparties_reliees']) for b in doc['balls']])
cases = 0
for pts in ([A, B, C, Z, W], [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)],
            [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)], [(0, 0, 0), (2, 2, 0), (4, 0, 0), (8, 0, 0)]):
    for k in range(1, len(pts)):
        base = shape(S.Supports(pts).canonical(k))
        moved = shape(S.Supports([(x + 3, y + 5, z + 7) for x, y, z in pts]).canonical(k))
        cases += 1
        if base != moved:
            failures.append('translation %r K=%d' % (pts, k))
print('translation entiere : %d cas, numerotation identique : %s' % (cases, 'oui' if not [f for f in failures if f.startswith('translation')] else 'NON'))
print('verif_faits %s ecarts=%d' % ('ok' if not failures else 'ECHEC', len(failures)))
sys.exit(1 if failures else 0)
