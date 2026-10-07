#!/usr/bin/env python3
"""Regle de resolution de la v12 contre l'etage B de la reference (morsehgp3D_v12/docs/CONTRAT_TOUR.md, 4.1 et 9).

La v12 resout chaque representant d'une jonction jusqu'a la PREMIERE cellule de fenetre (LEM-T3) et lit une cible
<< cellule b' >> comme l'element de la cellule b', deja traitee puisque son niveau est strictement inferieur ; les
cellules inertes de la fenetre sont traitees comme des jonctions (pont de LEM-T4). Ce script verifie, sur toute la
suite rapide (342 nuages, 1362 ordres) et pour trois politiques de saut (k plus proches du centre ; k plus petits
identifiants de I, regle de la v11 ; candidats voisins, levier G-L3), que le resultat canonique de chaque ordre
(arbre, verticales, entrees core et cover, coupes) est IDENTIQUE a celui de l'etage B historique, que test_ref.py juge
egal a la definition (etage A). Les compteurs de la regle sont exacts (toute derive est un plancher viole) et des
planchers gardent contre le vert par vacuite : arrets sur cellule, cellules inertes ciblees, pas inertes, sauts par
voisins et par census.

    python3 test_resolution_v12.py                 suite rapide, trois politiques
    python3 test_resolution_v12.py --inject=NOM    mutant applique a une copie de hgp12_ref : 4 s'il est tue
    python3 test_resolution_v12.py --list-mutants

Codes : 0 conforme ; 1 ecart a l'etage B ; 2 usage ; 3 compteur exact ou plancher viole, ou invariant ; 4 mutant tue
(--inject seulement ; un mutant qui survit rend 0). Python 3.10 nu, aucun assert : meme code sous python3 -O.
"""
import importlib
import importlib.util
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import hgp12_ref  # noqa: E402
from hgp12_ref import families  # noqa: E402
from hgp12_ref.model import InvariantError  # noqa: E402

OK, DISAGREEMENT, USAGE, FLOOR, MUTANT_KILLED = 0, 1, 2, 3, 4
POLICIES = ('v12_proches', 'v12_indices', 'v12_voisins')

# Compteurs EXACTS de la suite rapide, par politique (graves au premier passage, 7 octobre 2026) ; ils ne changent
# que si la suite, le catalogue de la reference ou la regle change, et se regravent alors en connaissance de cause.
_COMMON = dict(cells=8448, inert_cells=1113, inert_steps=7, inert_targets=168, jumps=35, steps=18434, targets=18392)
EXACT = {
    'v12_proches': dict(_COMMON, birth_stops=17097, cell_stops=1295, census_jumps=0, neighbour_jumps=0),
    'v12_indices': dict(_COMMON, birth_stops=17095, cell_stops=1297, census_jumps=0, neighbour_jumps=0),
    'v12_voisins': dict(_COMMON, birth_stops=17095, cell_stops=1297, census_jumps=0, neighbour_jumps=35),
}
# Planchers contre le vert par vacuite (toutes politiques) ; neighbour_jumps : politique voisins. Sur la suite rapide
# (4 a 8 points), les voisins suffisent a chaque saut : la branche de repli (census) est exercee par le fait grave
# fact_neighbour_fallback, jamais par la suite.
FLOORS = dict(cell_stops=1, inert_steps=1, jumps=1, inert_targets=1)
FLOORS_NEIGHBOURS = dict(neighbour_jumps=1)

# Mutants : correctif (texte present UNE SEULE fois) applique a une copie de hgp12_ref/constructive.py.
S12, S16, S20 = ' ' * 12, ' ' * 16, ' ' * 20
MUTANTS = {
    'inertes_omises': (
        S16 + 'cells.append((ball, [tuple(sorted(ball.inner + rep)) for rep in reps]))\n',
        S16 + "if kind == 'inert':\n" + S20 + 'continue\n' +
        S16 + 'cells.append((ball, [tuple(sorted(ball.inner + rep)) for rep in reps]))\n',
        'cellules inertes de la fenetre absentes du noyau : une cible qui s\'y arrete n\'a pas d\'element'),
    'element_sans_racine': (
        S20 + 'roots.append(root(element[index]))\n',
        S20 + 'roots.append(element[index])\n',
        'cible << cellule >> lue sans remonter a la racine courante : sommet perime d\'une composante absorbee'),
    'arret_sous_fenetre': (
        S12 + 'if ball.index is not None and ball.lo <= k <= ball.hi:  # premiere cellule de fenetre : arret\n',
        S12 + 'if ball.index is not None and k <= ball.hi:  # premiere cellule de fenetre : arret\n',
        'arret sur une boule du catalogue sous sa fenetre, qui n\'est pas une cellule de l\'ordre'),
    'plateau_coupe': (
        S12 + 'while last < len(cells) and cells[last][0].level == lam:\n' + S16 + 'last += 1\n',
        S12 + 'last += 1\n',
        'cellules d\'un meme niveau exact traitees une a une : plateau non atomique, multifusions perdues'),
}


def fail(code, message):
    print(message, file=sys.stderr)
    return code


def load_mutant(name):
    """Copie le paquet, applique le correctif, importe la copie sous un nom propre ; rend le module paquet."""
    old, new, _why = MUTANTS[name]
    tmp = tempfile.mkdtemp(prefix='hgp12_resolution_mutant_')
    try:
        target = os.path.join(tmp, 'hgp12_ref')
        shutil.copytree(os.path.join(HERE, 'hgp12_ref'), target, ignore=shutil.ignore_patterns('__pycache__'))
        path = os.path.join(target, 'constructive.py')
        with open(path, encoding='ascii') as handle:
            text = handle.read()
        if text.count(old) != 1:
            raise LookupError('mutant %s : motif present %d fois' % (name, text.count(old)))
        with open(path, 'w', encoding='ascii') as handle:
            handle.write(text.replace(old, new))
        alias = 'hgp12_resolution_mutant_' + name
        spec = importlib.util.spec_from_file_location(alias, os.path.join(target, '__init__.py'),
                                                      submodule_search_locations=[target])
        module = importlib.util.module_from_spec(spec)
        sys.modules[alias] = module
        spec.loader.exec_module(module)
        return module
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def fact_neighbour_fallback(package):
    """Fait grave du repli de G-L3 : sur l'axe des x, sites 8, 9, 10, 50, 70, 110, 111, 112 et K = 2, la partie
    {10, 110} a pour boule le segment [10, 110] (centre 60, rayon carre 2500) et deux points strictement interieurs
    (50 et 70), donc un saut ; mais ses candidats voisins (8, 9, 111, 112, et elle-meme sur la sphere) ne sont pas
    strictement interieurs : repli sur le census, saut vers {50, 70}, dont la boule (niveau 100, p = 0, m = 2) est une
    naissance de l'ordre 2. Rend la liste des ecarts."""
    xs = (8, 9, 10, 50, 70, 110, 111, 112)
    ref = package.Reference([(x, 0, 0) for x in xs], 2, resolution='v12_voisins')
    ident = dict((ref.input[i][0], ref.internal[i]) for i in range(len(xs)))
    kind, index = ref.resolve_v12(tuple(sorted((ident[10], ident[110]))), 2)
    ball = ref.balls[index]
    got = (kind, sorted(ref.input[ref.inp[x]][0] for x in ball.inner + ball.shell), str(ball.level),
           ref.v12_stats['census_jumps'], ref.v12_stats['neighbour_jumps'], ref.v12_stats['jumps'])
    want = ('naissance', [50, 70], '100', 1, 0, 1)
    return [] if got == want else ['repli des voisins : %r au lieu de %r' % (got, want)]


def fact_complete_census_in_catalogue(package):
    """Fait grave de la portee de LEM-HORS-CAT (CONTRAT_TOUR.md, paragraphe 4.2 ; correction du 7 octobre 2026) : le
    lemme ne parle que des spheres HORS de Cat_K. Carre A = (0,0,0), B = (2,0,0), C = (2,2,0), D = (0,2,0) et un point
    lointain E = (40,40,40), K = 5 : la partie F = {B, D} a pour boule le cercle du carre (rayon carre 2, p = 0), qui
    est dans Cat_5 avec S* = {A, C} (cardinal minimal, premier dans l'ordre de Morton comme dans l'ordre des
    positions) ; S* n'est pas dans F, donc la table S* -> boule ne la trouve pas par le support local de F et le moteur
    recense la sphere : census complet (p = 0 < k = 2) a l'ordre 2 = K - 3. La glose << un census complet n'arrive
    qu'aux ordres K - 1 et K >> etait fausse en general. Rend la liste des ecarts."""
    pts = [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0), (40, 40, 40)]
    ref = package.Reference(pts, 5)
    ident = dict((ref.input[i], ref.internal[i]) for i in range(len(pts)))
    part = tuple(sorted((ident[(2, 0, 0)], ident[(0, 2, 0)])))
    _anchor, _ctr, key = ref._meb(part)
    ball = ref._by_key.get(key)
    if ball is None:
        return ['census complet du catalogue : la sphere de {B, D} est absente du catalogue de reference']
    support = sorted(ref.sites[s] for s in ball.support)
    local = sorted(ref.sites[ref.site_of[x]] for x in part)
    got = (str(ball.level), ball.p, ball.qmin, support, support == local, ball.p < 2, 2 <= ref.kmax - 3)
    want = ('2', 0, 2, [(0, 0, 0), (2, 2, 0)], False, True, True)
    return [] if got == want else ['census complet du catalogue : %r au lieu de %r' % (got, want)]


def fact_lemma_needs_two_sites(package):
    """Fait grave de CST-0229 (auditeur Codex, 7 octobre 2026) : LEM-HORS-CAT exige k >= 2 (rayon positif). Le
    catalogue Cat_K du moteur ne contient que des boules positives (q_min >= 2) ; trois sites (0,0,0), (2,0,0), (4,0,0),
    K = 3 : la boule minimale du singleton {(0,0,0)} (k = 1) est le site lui-meme, de rayon nul, hors de Cat_3, avec
    p = 0 < K - 2 = 1. L'oracle range les boules de rayon nul dans son catalogue interne : on filtre ici les boules
    positives, comme T1. Rend la liste des ecarts."""
    ref = package.Reference([(0, 0, 0), (2, 0, 0), (4, 0, 0)], 3)
    first = ref.internal[0]
    _anchor, _ctr, key = ref._meb((first,))
    ball = ref._by_key.get(key)
    positive = [b for b in ref.balls if b.level > 0]
    got = (str(key[1]), ball is not None and ball.p, any((b.center, b.level) == key for b in positive), ref.kmax - 2)
    want = ('0', 0, False, 1)
    return [] if got == want else ['LEM-HORS-CAT a k = 1 : %r au lieu de %r' % (got, want)]


def fact_saturated_in_catalogue(package):
    """Fait grave du commentaire faux du contrat T2 (contre-lecture Codex, 7 octobre 2026) : un census sature ne
    prouve pas que la sphere est hors du catalogue. Carre (0,0,0), (4,0,0), (4,4,0), (0,4,0), interieurs (2,2,0) et
    (2,1,0), K = 5 ; F = la diagonale {(4,0,0), (0,4,0)}, k = 2 : boule de niveau 8, p = 2, q_min = 2, admise a Cat_5,
    mais son S* est l'autre diagonale {(0,0,0), (4,4,0)} (premiere en Morton comme en positions) : la sonde par le
    support local de F echoue et le census, de seuil k = 2, sature. Rend la liste des ecarts."""
    pts = [(0, 0, 0), (4, 0, 0), (4, 4, 0), (0, 4, 0), (2, 2, 0), (2, 1, 0)]
    ref = package.Reference(pts, 5)
    ident = dict((ref.input[i], ref.internal[i]) for i in range(len(pts)))
    part = tuple(sorted((ident[(4, 0, 0)], ident[(0, 4, 0)])))
    _anchor, _ctr, key = ref._meb(part)
    ball = ref._by_key.get(key)
    if ball is None:
        return ['census sature du catalogue : la sphere de la diagonale est absente du catalogue de reference']
    support = sorted(ref.sites[x] for x in ball.support)
    local = sorted(ref.sites[ref.site_of[x]] for x in part)
    got = (str(ball.level), ball.p, ball.qmin, ball.p + ball.qmin <= ref.kmax + 1, support, support == local, ball.p >= 2)
    want = ('8', 2, 2, True, [(0, 0, 0), (4, 4, 0)], False, True)
    return [] if got == want else ['census sature du catalogue : %r au lieu de %r' % (got, want)]


def same(a, b):
    return (a.nodes == b.nodes and a.lower == b.lower and a.core == b.core and a.cover == b.cover and
            a.cuts == b.cuts)


def run(package):
    """Rend (ecarts, compteurs par politique, nombre d'ordres) ; leve InvariantError si la regle en viole un."""
    reference = package.Reference
    gaps, totals, orders = [], dict((p, None) for p in POLICIES), 0
    for cloud in families.fast_suite():
        base = hgp12_ref.Reference(cloud.points, cloud.kmax)
        expected = [base.order(k) for k in range(1, base.orders + 1)]
        orders += base.orders
        for policy in POLICIES:
            ref = reference(cloud.points, cloud.kmax, resolution=policy)
            for k in range(1, ref.orders + 1):
                if not same(ref.order(k), expected[k - 1]):
                    gaps.append('%s, ordre %d, politique %s : resultat different de l\'etage B' % (cloud.name, k, policy))
            stats = dict(ref.v12_stats)
            if totals[policy] is None:
                totals[policy] = dict((key, 0) for key in stats)
            for key, value in stats.items():
                totals[policy][key] += value
    return gaps, totals, orders


def main(argv):
    args = argv[1:]
    if args == ['--list-mutants']:
        for name in sorted(MUTANTS):
            print('%s : %s' % (name, MUTANTS[name][2]))
        return OK
    inject = None
    if len(args) == 1 and args[0].startswith('--inject='):
        inject = args[0].split('=', 1)[1]
        if inject not in MUTANTS:
            return fail(USAGE, 'mutant inconnu : %r' % inject)
    elif args:
        return fail(USAGE, 'usage : test_resolution_v12.py [--inject=NOM | --list-mutants]')
    if inject is None:
        package = hgp12_ref
    else:
        try:
            package = load_mutant(inject)
        except LookupError as error:
            return fail(USAGE, str(error))
    # l'invariant d'un paquet mute est la classe de SA copie de model.py
    invariants = (InvariantError, importlib.import_module(package.__name__ + '.model').InvariantError)
    try:
        facts = (fact_neighbour_fallback(package) + fact_complete_census_in_catalogue(package) +
                 fact_lemma_needs_two_sites(package) + fact_saturated_in_catalogue(package))
        gaps, totals, orders = run(package)
    except invariants as error:
        if inject is not None:
            print('mutant %s tue par un invariant : %s' % (inject, error))
            return MUTANT_KILLED
        return fail(FLOOR, 'invariant viole : %s' % error)
    gaps = facts + gaps
    if inject is not None:
        if gaps:
            print('mutant %s tue : %d ecarts a l\'etage B (premier : %s)' % (inject, len(gaps), gaps[0]))
            return MUTANT_KILLED
        print('mutant %s SURVIT' % inject)
        return OK
    if gaps:
        for gap in gaps[:20]:
            print(gap, file=sys.stderr)
        return fail(DISAGREEMENT, '%d ecarts a l\'etage B' % len(gaps))
    for policy in POLICIES:
        print('%s %s' % (policy, ' '.join('%s=%d' % (key, totals[policy][key]) for key in sorted(totals[policy]))))
    problems = []
    for policy in POLICIES:
        floors = dict(FLOORS, **(FLOORS_NEIGHBOURS if policy == 'v12_voisins' else {}))
        for key, floor in floors.items():
            if totals[policy][key] < floor:
                problems.append('%s : %s = %d sous le plancher %d' % (policy, key, totals[policy][key], floor))
        if EXACT[policy] is not None and totals[policy] != EXACT[policy]:
            problems.append('%s : compteurs %r, graves %r' % (policy, totals[policy], EXACT[policy]))
    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
        return FLOOR
    common = totals[POLICIES[0]]
    print('resolution_v12_ok nuages=%d ordres=%d politiques=%d cibles=%d cellules=%d inertes=%d faits=4'
          % (len(families.fast_suite()), orders, len(POLICIES), common['targets'], common['cells'],
             common['inert_cells']))
    return OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
