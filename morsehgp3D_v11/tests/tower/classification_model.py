"""Modele T2 de classification au premier temoin strict, sans MEB ni moteur.

Certification hors compte : elimination rationnelle des quatre equations
barycentriques, puis Caratheodory (supports affinement independants <=4).
Le prefixe du classificateur est compare a l'inventaire EXHAUSTIF des traces.
Ce modele ne remplace ni le rejeu FULL ni son juge de coupes/verticales.

Les indices de temoin designent les positions dans la coquille ordonnee.
Le cas t=m=13 est mathematique seulement : la fenetre native K<=12 le refuse.
Le modele Python materialise ses petits temoins ; aucun cout natif n'en decoule.
"""
import copy
from dataclasses import dataclass
from fractions import Fraction as F
from itertools import combinations
import json
import math
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def affine_weights(points, center):
    """Systeme affine 4 x q, sans Gram, sphere ou comparaison de rayons."""
    q = len(points)
    rows = [[F(p[j]) for p in points]+[F(center[j])] for j in range(3)]
    rows.append([F(1)]*(q+1))
    pivots = []
    for column in range(q):
        pivot = next((r for r in range(len(pivots), 4) if rows[r][column]), None)
        if pivot is None:
            return None  # Les temoins dependants sont couverts par un sous-ensemble.
        target = len(pivots)
        rows[target], rows[pivot] = rows[pivot], rows[target]
        value = rows[target][column]
        rows[target] = [x/value for x in rows[target]]
        for r in range(4):
            if r != target:
                factor = rows[r][column]
                rows[r] = [x-factor*y for x, y in zip(rows[r], rows[target])]
        pivots.append(target)
    if any(not any(row[:q]) and row[q] for row in rows):
        return None
    return tuple(rows[r][q] for r in pivots)


@dataclass(frozen=True)
class Shell:
    points: tuple
    center: tuple
    witnesses: tuple
    qmin: int


def certify(points, center, qmin=None):
    """Petite coquille critique exacte ; qmin est global, pas celui du premier prefixe."""
    points, center = tuple(tuple(p) for p in points), tuple(F(x) for x in center)
    require(len(center) == 3 and len(points) >= 2, 'dimensions')
    require(all(len(p) == 3 and all(type(x) is int for x in p) for p in points), 'coordonnees')
    require(len(set(points)) == len(points), 'sites distincts')
    radii = {sum((F(x)-c)**2 for x, c in zip(p, center)) for p in points}
    require(len(radii) == 1 and next(iter(radii)) > 0, 'coquille positive')
    witnesses = []
    for size in range(1, min(4, len(points))+1):
        for subset in combinations(range(len(points)), size):
            weights = affine_weights(tuple(points[i] for i in subset), center)
            if weights is not None and all(w >= 0 for w in weights):
                witnesses.append(frozenset(subset))
    require(bool(witnesses), 'centre hors enveloppe convexe')
    actual = min(map(len, witnesses))
    require(qmin is None or type(qmin) is int and qmin == actual, 'qmin global')
    return Shell(points, center, tuple(witnesses), actual)


def native_window(shell, inner, order, kmax):
    """Seulement la fenetre scalaire existante ; pas une factory FullDomain."""
    require(all(type(x) is int for x in (inner, order, kmax)), 'parametre entier')
    require(0 <= inner < 2**32 and 1 <= order <= kmax <= 12, 'fenetre native')
    require(inner+shell.qmin-1 <= order <= inner+len(shell.points), 'fenetre cellule')
    t = order-inner
    require(math.comb(len(shell.points), t) <= (2**64-1)//2, 'capacite deux passes')
    return t


def classify(shell, t):
    """Premier temoin par separation convexe fermee, jamais par MEB(A)."""
    m = len(shell.points)
    require(type(t) is int and shell.qmin-1 <= t <= m, 'fenetre mathematique')
    out = dict(kind='birth', combinations=math.comb(m, t), examined=0,
               expected_meb_calls=0, first=None, witness=None, path='all_shell')
    if t == m:
        return out
    if t < shell.qmin:
        out.update(kind='strict_traces', first=0, witness=list(range(t)), path='below_qmin')
        return out
    out['path'] = 'searched'
    for ordinal, subset in enumerate(combinations(range(m), t)):
        out['examined'] += 1
        out['expected_meb_calls'] += 1  # Contrat propose, aucun appel MEB dans ce modele.
        chosen = frozenset(subset)
        if not any(w <= chosen for w in shell.witnesses):
            out.update(kind='strict_traces', first=ordinal, witness=list(subset))
            return out
    return out


def exhaustive(shell, t):
    """Reference sans raccourcis ni arret anticipe ; toutes les traces sont conservees."""
    return tuple((i, tuple(part)) for i, part in enumerate(combinations(range(len(shell.points)), t))
                 if not any(w <= frozenset(part) for w in shell.witnesses))


def expected(shell, t):
    strict = exhaustive(shell, t)
    m, count = len(shell.points), math.comb(len(shell.points), t)
    analytic = t == m or t < shell.qmin
    tested = 0 if analytic else strict[0][0]+1 if strict else count
    return dict(kind='strict_traces' if strict else 'birth', combinations=count,
                examined=tested, expected_meb_calls=tested,
                first=strict[0][0] if strict else None,
                witness=list(strict[0][1]) if strict else None,
                path='all_shell' if t == m else 'below_qmin' if t < shell.qmin else 'searched')


def equal(actual, wanted):
    require(type(actual) is type(wanted), 'type exact')
    if isinstance(wanted, dict):
        require(actual.keys() == wanted.keys(), 'champs exacts')
        return 1+sum(equal(actual[k], v) for k, v in wanted.items())
    if isinstance(wanted, list):
        require(len(actual) == len(wanted), 'taille exacte')
        return 1+sum(equal(a, b) for a, b in zip(actual, wanted))
    require(actual == wanted, 'valeur exacte')
    return 1


def judge(actual, shell, t):
    return equal(actual, expected(shell, t))


def fixtures(bits):
    maximum = (1 << bits)-1
    square = ((0,0,0), (4,0,0), (0,4,0), (4,4,0))
    wide = ((0,5,5), (1,2,5), (1,8,5), (2,1,5), (2,9,5), (5,0,5), (5,10,5),
            (8,1,5), (8,9,5), (9,2,5), (9,8,5), (10,5,5), (5,5,0))
    rows = [('pair', ((0,0,0), (3,0,0)), (F(3,2),0,0), 2),
            ('triangle', ((0,0,0), (4,0,0), (2,3,0)), (2,F(5,6),0), 3),
            ('tetra', ((0,0,0), (2,2,0), (2,0,2), (0,2,2)), (1,1,1), 4),
            ('square_first', square, (2,2,0), 2),
            ('square_late', tuple(square[i] for i in (0,3,1,2)), (2,2,0), 2),
            ('octa', ((2,2,0), (2,0,2), (0,2,2), (4,2,2), (2,4,2), (2,2,4)), (2,2,2), 2),
            # Coquille entiere qmin3 ; prefixe (0,1,2,4) certifie q4, support global (1,2,3).
            ('global_qmin', ((5,5,0), (2,1,5), (10,5,5), (2,9,5), (5,9,8)), (5,5,5), 3),
            ('extended_q4', ((10,5,5), (9,8,5), (5,2,1), (1,5,8), (9,2,5)), (5,5,5), 4),
            ('shell13', wide, (5,5,5), 2),
            ('high_tetra', ((0,0,0), (maximum,maximum,0), (maximum,0,maximum),
                            (0,maximum,maximum)), (F(maximum,2),)*3, 4)]
    return tuple((name, certify(points, center, qmin)) for name, points, center, qmin in rows)


class NoSearch:
    def __iter__(self):
        raise RuntimeError('branche analytique ayant lance une recherche')


def main():
    checks, cases, corruptions, facts, refusals = 0, 0, 0, 0, 0
    profiles = []
    for bits in (18, 21, 24):
        rows = dict(fixtures(bits))
        profile_cases = 0
        for name, shell in rows.items():
            for t in range(shell.qmin-1, len(shell.points)+1):
                result = classify(shell, t)
                checks += judge(result, shell, t)
                cases += 1; profile_cases += 1
                if t <= 12:
                    require(native_window(shell, 0, t, 12) == t, 'fenetre native admissible')
                    facts += 1
            # Une permutation change l'ordinal, jamais l'existence d'une trace stricte.
            reversed_shell = certify(shell.points[::-1], shell.center, shell.qmin)
            for t in (shell.qmin-1, min(shell.qmin, len(shell.points)), len(shell.points)):
                reverse = classify(reversed_shell, t)
                checks += judge(reverse, reversed_shell, t)
                require(reverse['kind'] == classify(shell, t)['kind'], 'invariance par permutation')
                facts += 1; cases += 1; profile_cases += 1
        for name, t, first, examined, kind in (
            ('square_first', 2, 0, 1, 'strict_traces'),
            ('square_late', 2, 1, 2, 'strict_traces'),
            ('square_first', 3, None, 4, 'birth'),
            ('global_qmin', 2, 0, 0, 'strict_traces'),
            ('shell13', 13, None, 0, 'birth')):
            value = classify(rows[name], t)
            require((value['first'], value['examined'], value['kind']) == (first, examined, kind), name)
            facts += 1
        global_shell = rows['global_qmin']
        prefix = certify(tuple(global_shell.points[i] for i in (0,1,2,4)), global_shell.center)
        require(prefix.qmin == 4 and global_shell.qmin == 3, 'qmin ne vient pas du prefixe')
        require(frozenset((1,2,3)) in global_shell.witnesses and 0 not in (1,2,3), 'support global sans premier U')
        strict = exhaustive(rows['octa'], 2)
        require(len(strict) == 12 and classify(rows['octa'], 2)['examined'] == 1, 'octa 12 traces, premier temoin')
        require(2*math.comb(6,2)+1 == 31 and 4*math.comb(6,2) == 60, 'rejeu exhaustif conserve')
        require(native_window(rows['octa'],1,3,5) == 2, 't=k-p avec interieur non vide')
        for name, t in (('shell13',13), ('global_qmin',2)):
            shell = rows[name]
            no_search = Shell(shell.points, shell.center, NoSearch(), shell.qmin)
            checks += equal(classify(no_search,t), classify(shell,t))
            facts += 1
        require(affine_weights(((0,0,0),(4,4,0)),(2,2,0)) == (F(1,2),F(1,2)), 'contact ferme')
        require(affine_weights(((0,0,0),(4,0,0)),(2,2,0)) is None, 'separation stricte')
        facts += 7
        # Reponses corrompues : dates de temoin, omissions, compte univers/prefixe, types et branches.
        for name, t in (('square_first',2), ('square_late',2), ('square_first',3), ('shell13',13)):
            good = classify(rows[name], t)
            for key, bad_value in (('kind','incorrect'), ('first',42), ('witness',[99]),
                                   ('examined',good['examined']+1), ('combinations',0),
                                   ('expected_meb_calls',False), ('path','incorrect')):
                bad = copy.deepcopy(good); bad[key] = bad_value
                try:
                    judge(bad, rows[name], t)
                except ValueError:
                    corruptions += 1
                else:
                    raise ValueError('corruption non detectee')
        for action in (
            lambda: native_window(rows['shell13'],0,13,13),
            lambda: native_window(rows['tetra'],0,2,12),
            lambda: native_window(rows['pair'],0,True,12),
            lambda: classify(rows['tetra'],2),
            lambda: certify(global_shell.points,global_shell.center,4),
            lambda: certify(((0,0,0),(1,0,0)),(0,0,0))):
            try:
                action()
            except ValueError:
                refusals += 1
            else:
                raise ValueError('refus manquant')
        profiles.append(dict(bits=bits, cases=profile_cases))
    require(corruptions == 84 and refusals == 18 and cases >= 200 and facts >= 200, 'planchers')
    print(json.dumps(dict(verdict='conforme', native=0, cases=cases, checks=checks,
                         corruptions=corruptions, refusals=refusals, facts=facts, profiles=profiles), sort_keys=True))


if __name__ == '__main__':
    try:
        require(len(sys.argv) == 1, 'usage classification_model.py')
        main()
    except (ValueError, TypeError, KeyError, IndexError) as error:
        print('REFUS '+str(error), file=sys.stderr)
        raise SystemExit(1)
