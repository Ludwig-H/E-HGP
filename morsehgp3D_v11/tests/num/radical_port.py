"""Copie a la lettre, en bibliotheque standard, des decisions exactes de bench/points_radius.py (lignes 29-117 :
sign, sqrt_diff_cmp, sqrt_cmp2, sqrt_bounds, Refusal, square_ratio, radical_classes, sign_of_radicals), source du
port C++ de src/num/radical.cpp (tranche S8). bench/points_radius.py importe numpy et ne peut pas servir une porte
fast (Python 3.10 nu sur G4) : cette copie le remplace, et source_mismatches() verifie, par l'arbre syntaxique, que
chaque fonction copiee est identique a celle du banc (une divergence fait echouer les portes, code 3).
Aucun assert. Cadre : phase=exploration_v11_hors_registre, public_status=not_claimed.
"""
import ast
from fractions import Fraction
import math

ZERO = Fraction(0)
PORTED = ('sign', 'sqrt_diff_cmp', 'sqrt_cmp2', 'sqrt_bounds', 'Refusal', 'square_ratio', 'radical_classes',
          'sign_of_radicals')


def sign(v):
    return (v > 0) - (v < 0)


def sqrt_diff_cmp(x, y, u):
    """Signe exact de sqrt(x) - sqrt(y) - u, pour x, y >= 0 rationnels et u rationnel."""
    d = sign(x - y)
    if d >= 0 and u <= 0:
        return 0 if d == 0 and u == 0 else 1
    if d <= 0 and u >= 0:
        return 0 if d == 0 and u == 0 else -1
    if d > 0:  # u > 0 : sqrt x - sqrt y - u a le signe de (x - y - u^2) - 2 u sqrt y
        w = x - y - u * u
        if w <= 0:
            return 0 if w == 0 and y == 0 else -1
        return sign(w * w - 4 * u * u * y)
    return -sqrt_diff_cmp(y, x, -u)  # d < 0 et u < 0


def sqrt_cmp2(a, b, c, d):
    """Signe exact de (sqrt a + sqrt b) - (sqrt c + sqrt d) ; les deux sommes sont positives."""
    return sqrt_diff_cmp(a * b, c * d, ((c + d) - (a + b)) / 2)


def sqrt_bounds(f, bits):
    """Encadrement [lo, hi) de sqrt(f) par isqrt a 2^-bits pres (relatif au denominateur)."""
    n, d = f.numerator, f.denominator
    s = math.isqrt((n * d) << (2 * bits))
    scale = d << bits
    return Fraction(s, scale), Fraction(s + 1, scale)


class Refusal(ArithmeticError):
    """Comparaison non nulle non separee dans le budget : refus explicite, jamais une egalite supposee."""


def square_ratio(a, b):
    """(vrai, q) si a / b est le carre du rationnel q (a, b > 0) : sqrt(a) = q sqrt(b). Sans factorisation."""
    r = a / b
    n, d = r.numerator, r.denominator
    sn, sd = math.isqrt(n), math.isqrt(d)
    if sn * sn == n and sd * sd == d:
        return True, Fraction(sn, sd)
    return False, None


def radical_classes(terms):
    """Somme de signe(f) sqrt(f) regroupee par classes de carres rationnelles : [(representant, coefficient)].
    Des racines de classes distinctes sont lineairement independantes sur Q : la somme est nulle si et seulement
    si tous les coefficients le sont."""
    classes = []
    for sgn, f in terms:
        if not f:
            continue
        for c in classes:
            ok, q = square_ratio(f, c[0])
            if ok:
                c[1] += sgn * q
                break
        else:
            classes.append([f, Fraction(sgn)])
    return [(rep, coef) for rep, coef in classes if coef]


def sign_of_radicals(terms, budget=8192):
    """Signe exact de la somme de signe(f) sqrt(f) ; refus si une somme non nulle reste non separee."""
    classes = radical_classes(terms)
    if not classes:
        return 0
    if len(classes) == 1:
        return sign(classes[0][1])
    if len(classes) == 2:
        (r1, c1), (r2, c2) = classes
        if sign(c1) == sign(c2):
            return sign(c1)
        return sign(c1) * sign(c1 * c1 * r1 - c2 * c2 * r2)  # |c1| sqrt r1 contre |c2| sqrt r2
    bits = 96
    while bits <= budget:
        lo = hi = ZERO
        for rep, coef in classes:
            a, b = sqrt_bounds(rep, bits)
            lo += coef * (a if coef > 0 else b)
            hi += coef * (b if coef > 0 else a)
        if lo > 0:
            return 1
        if hi < 0:
            return -1
        bits *= 2
    raise Refusal('somme de radicaux non nulle non separee a 2^-%d' % budget)


def _definitions(text):
    tree = ast.parse(text)
    out = {}
    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.ClassDef)) and node.name in PORTED:
            out[node.name] = ast.dump(node)
    return out


def source_mismatches(bench_path):
    """Noms des definitions portees qui different de bench/points_radius.py (ou y manquent) ; vide si identiques."""
    with open(bench_path, encoding='utf-8') as handle:
        bench = _definitions(handle.read())
    with open(__file__, encoding='utf-8') as handle:
        mine = _definitions(handle.read())
    return [name for name in PORTED if name not in bench or name not in mine or bench[name] != mine[name]]
