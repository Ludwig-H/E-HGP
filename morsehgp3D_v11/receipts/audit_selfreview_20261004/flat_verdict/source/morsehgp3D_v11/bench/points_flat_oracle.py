#!/usr/bin/env python3
"""Oracle independant de la sortie plate v11 (porte E1) : tete certifiee sur H^r_{k+1} (cote T) et sur l'arbre du
lien simple de l'atteignabilite mutuelle de HDBSCAN (cote A), petits nuages seulement.

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, public_status=not_claimed. Specification du
juge : build/v11-points-select/juge/SPEC.md, par. 1.1, 2 et 4.2-4.3 (porte points_flat_gate.py, oracle des petits
nuages et fixtures F1-F11).

Independance. Ce fichier ne lit ni n'importe bench/points_flat.py, bench/points_radius.py, ni les prototypes du
workflow (equite/nary_head.py, modele/scripts/modele_lib.py). Il importe seulement deux oracles de la hierarchie,
deja independants du banc : reference/hgp11_ref (Definition, etage A, verite de la tour par Gamma_k) et
bench/points_reference.py (reference_radius_rules : dates et proprietaires de H^r_{k+1} par force brute sur les
coupes, decisions exactes). Condensation, scores, selection et arithmetique sont ecrits ici.

Arithmetique propre (classe Rad). Un nombre est une somme finie c_1 sqrt(s_1) + ... + c_m sqrt(s_m), s_j entiers
>= 1 sans facteur carre deux a deux distincts, c_j rationnels (Fraction) non nuls. Les racines d'entiers sans
facteur carre distincts sont lineairement independantes sur Q (Besicovitch, 1940) : la forme est unique, et un
nombre est nul si et seulement si sa forme n'a aucun terme (egalite CERTIFIEE). Parties sans facteur carre par
division d'essai jusqu'a la racine cubique du cofacteur (le cofacteur restant vaut alors 1, p, p^2 ou p q, et seul
p^2 est un carre) ; produit de deux sans-facteur-carre par pgcd : s t = g^2 (s/g)(t/g). Inverse par le produit des
conjugues de signe (norme formelle, rationnelle, non nulle par independance lineaire), verifie par x * (1/x) == 1.
Signe d'un nombre non nul par encadrements isqrt a precision doublee ; au-dela de SIGN_MAX_BITS, refus explicite
(OracleRefusal), jamais de decision silencieuse.

Semantique (consigne de l'oracle, SPEC par. 1.1).
  Cote T : res = Definition(points).order(k) ; (e_i, o_i) = reference_radius_rules(res, n, m=k+1)['margin_r'] ;
    date exacte e_i = sqrt(t) + sqrt(meet) - sqrt(h). Coupe fermee de rayon r : sites engages {i : e_i <= r},
    i et j dans le meme bloc ssi u(i, j) <= r, u(i, j) = max(e_i, e_j, sqrt(niveau(lca(o_i, o_j)))) si le lca
    n'est ni o_i ni o_j, max(e_i, e_j) sinon. Niveaux d'evenement : valeurs distinctes de {e_i} et des
    sqrt(niveau(v)) de tous les noeuds, ordonnees exactement.
  Cote A : core2(x) = k-ieme plus petite distance carree de x aux sites, x compris (min_samples = k, soi compris,
    comme sklearn) ; mr2(a, b) = max(core2(a), core2(b), d2(a, b)) en entiers ; tous les sites presents des 0 ;
    bloc au niveau l = composante du graphe {mr2 <= l^2}. Niveaux d'evenement : hauteurs distinctes des fusions
    du lien simple (Kruskal par plateaux), sqrt(N), N entier.
  Tete commune : balayage des niveaux d'evenement croissants, plateaux atomiques (tout ce qui arrive au meme
    niveau exact est traite ensemble). Un bloc est GROS s'il a au moins mcs sites engages. Pour un gros bloc B au
    niveau l et les clusters vivants au niveau precedent dont le bloc est inclus dans B : 0 -> un cluster nait
    (bas = l, tous les sites de B le rejoignent en l) ; 1 -> il continue, les sites nouveaux de B le rejoignent
    en l ; >= 2 -> ils meurent (haut = l) et un parent N-aire nait en l, tous les sites de B le rejoignent en l.
    Le cluster vivant au dernier niveau est la racine, exclue de la selection.
    Score S(C) = somme sur les sites x de C de phi(rejoint_x) - phi(haut_C), phi(r) = r^(-z), unite native.
    EOM (juge par enumeration, pas le programme dynamique) : les antichaines optimales se decomposent sur les
    sous-arbres des enfants de la racine ; O = reunion des antichaines optimales (valeur exacte maximale) ; la
    selection est l'ensemble des elements maximaux (les plus hauts) de O. C'est le programme dynamique avec
    parent sur egalite certifiee (preuve courte dans _select_eom) ; un programme dynamique ecrit a part le
    recoupe, tout desaccord leve OracleInvariantError. Feuilles : clusters non racine sans enfant.
    Labels : plus petit PointId du cluster retenu ; -1 sinon ; tout -1 si n < mcs ou sans scission.

API : tower_flat(points, k, mcs, z=1, selection='eom') et hdbscan_flat(...) rendent un dict : labels (n entiers),
clusters (members tries, bas et haut en chaine 'forme exacte ~ valeur', score exact en chaine, children, root),
root, selected (indices de clusters), certified_equalities (comparaisons EOM S(C) = somme V(D) exactes),
eom_comparisons, levels (cote T : valeurs distinctes de {e_i} et des rayons de noeuds ; cote A : hauteurs
distinctes des fusions), level_coincidences (cote T : dates exactement egales a un rayon de noeud ou a une autre
date, comptees par la forme canonique). La structure d'un cote est memorisee par (sites, k) : plusieurs (mcs, z, selection) sur
le meme nuage ne recalculent pas Definition. head(struct, ...) rend en plus les objets exacts (Rad).

Usage : python3 morsehgp3D_v11/bench/points_flat_oracle.py --self-test   (code 0 conforme, 3 sinon ; JSON)
        python3 morsehgp3D_v11/bench/points_flat_oracle.py --timing      (temps par taille de nuage ; JSON)
Bibliotheque standard seulement (plus numpy, tire par points_reference) ; tient sous python3 -O : aucune decision
ne repose sur assert.

Domaine : oracle de correction des petits nuages (n <= 12 a 14, k <= 4) ; Definition est exhaustive en C(n, k).
Jamais un backend, jamais une mesure d'echelle.

Limites connues. Tete T1/A1/T3/A3/TL/AL seulement : ni V1 (existence C), ni V2 (premiere couverture), ni la
completion 1-NN, ni kappa = 2, ni la sortie binarisee de sklearn (R0). Bornes explicites, chacune levant
OracleRefusal : TRIAL_LIMIT (cofacteur de radicande > ~8e18 apres petits facteurs), SIGN_MAX_BITS, CONJ_MAX_CLASSES,
ANTICHAIN_MAX. points_reference importe points_hierarchy (banc) au chargement, mais reference_radius_rules ne
l'utilise pas. Les niveaux affiches ('~ valeur') sont des lectures flottantes ; aucune decision n'en depend.
"""
from fractions import Fraction
from functools import cmp_to_key
from itertools import permutations, product
from math import gcd, isqrt
import json
import os
import random
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'reference'))
sys.path.insert(0, HERE)

from hgp11_ref import Definition  # noqa: E402
import points_reference as pref  # noqa: E402

TRIAL_LIMIT = 2 * 10 ** 6       # plus grand diviseur d'essai admis (radicandes jusqu'a environ 8e18) ; refus au-dela
SIGN_MAX_BITS = 1 << 18         # precision maximale d'un encadrement de signe ; refus au-dela
CONJ_MAX_CLASSES = 6            # classes maximales d'un nombre a inverser (2^5 conjugues) ; refus au-dela
ANTICHAIN_MAX = 200000          # antichaines maximales enumerees par sous-arbre ; refus au-dela


class OracleRefusal(Exception):
    """Refus explicite : une borne de calcul de l'oracle est atteinte (factorisation, signe, classes, antichaines)."""


class OracleInvariantError(Exception):
    """Invariant interne viole : defaut de l'oracle ou d'un oracle importe (code de porte 3)."""


# ---------------------------------------------------------------------------------------------- arithmetique

_SQUAREFREE = {}


def squarefree_split(value):
    """value = q^2 * s avec s sans facteur carre ; rend (q, s). Division d'essai tant que d^3 <= cofacteur : a la
    sortie, tout facteur premier du cofacteur m est >= d > m^(1/3), donc m vaut 1, p, p^2 ou p r (p != r), et m
    est un carre si et seulement si m = p^2 (ou 1)."""
    if not isinstance(value, int) or value <= 0:
        raise ValueError('entier strictement positif attendu : %r' % (value,))
    hit = _SQUAREFREE.get(value)
    if hit is not None:
        return hit
    q, s, m, d = 1, 1, value, 2
    while d * d * d <= m:
        if d > TRIAL_LIMIT:
            raise OracleRefusal('factorisation : diviseur d essai > %d pour %d' % (TRIAL_LIMIT, value))
        if m % d == 0:
            e = 0
            while m % d == 0:
                m //= d
                e += 1
            q *= d ** (e // 2)
            if e % 2:
                s *= d
        d = 3 if d == 2 else d + 2
    if m > 1:
        r = isqrt(m)
        if r * r == m:
            q *= r
        else:
            s *= m
    if q * q * s != value:
        raise OracleInvariantError('partie sans facteur carre fausse pour %d' % value)
    _SQUAREFREE[value] = (q, s)
    return q, s


class Rad(object):
    """Somme c_1 sqrt(s_1) + ... ; s_j sans facteur carre distincts, c_j Fraction non nuls (forme canonique)."""

    __slots__ = ('t',)

    def __init__(self, terms=None):
        self.t = {}
        if terms:
            for s, c in terms.items():
                c = Fraction(c)
                if c:
                    self.t[int(s)] = c

    @staticmethod
    def _raw(terms):
        x = Rad()
        x.t = terms
        return x

    @staticmethod
    def rational(q):
        return Rad({1: Fraction(q)})

    @staticmethod
    def sqrt_of(value):
        """Racine exacte d'un rationnel >= 0 : sqrt(p / q) = a sqrt(sp) / (b sqrt(sq)) = a sqrt(sp sq) / (b sq),
        avec p = a^2 sp, q = b^2 sq ; sp et sq sont premiers entre eux (p et q le sont), sp sq est sans carre."""
        value = Fraction(value)
        if value < 0:
            raise ValueError('racine d un negatif : %s' % value)
        if value == 0:
            return Rad()
        a, sp = squarefree_split(value.numerator)
        b, sq = squarefree_split(value.denominator)
        if gcd(sp, sq) != 1:
            raise OracleInvariantError('parties sans carre non premieres entre elles')
        return Rad._raw({sp * sq: Fraction(a, b * sq)})

    def key(self):
        return tuple(sorted(self.t.items()))

    def is_zero(self):
        return not self.t

    def __eq__(self, other):
        return isinstance(other, Rad) and self.t == other.t

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.key())

    def __add__(self, other):
        out = dict(self.t)
        for s, c in other.t.items():
            v = out.get(s, 0) + c
            if v:
                out[s] = v
            else:
                out.pop(s, None)
        return Rad._raw(out)

    def __neg__(self):
        return Rad._raw(dict((s, -c) for s, c in self.t.items()))

    def __sub__(self, other):
        return self + (-other)

    def scale(self, q):
        q = Fraction(q)
        if q == 0:
            return Rad()
        return Rad._raw(dict((s, c * q) for s, c in self.t.items()))

    def __mul__(self, other):
        out = {}
        for s1, c1 in self.t.items():
            for s2, c2 in other.t.items():
                g = gcd(s1, s2)
                s = (s1 // g) * (s2 // g)
                v = out.get(s, 0) + c1 * c2 * g
                if v:
                    out[s] = v
                else:
                    out.pop(s, None)
        return Rad._raw(out)

    def power(self, z):
        if not isinstance(z, int) or z < 0:
            raise ValueError('exposant entier >= 0 attendu')
        out = Rad.rational(1)
        for _ in range(z):
            out = out * self
        return out

    def inverse(self):
        """1/x par le produit des conjugues de signe. Avec y_j = c_j sqrt(s_j), le produit P des 2^(m-1) facteurs
        y_1 + sum eps_j y_j (eps dans {+1, -1}^(m-1)) est invariant par y_j -> -y_j pour tout j (permutation des
        facteurs ; pour j = 1, chaque facteur devient l'oppose d'un autre, en nombre pair) : c'est un polynome en
        les y_j^2, donc un rationnel. Chaque facteur est une combinaison a coefficients non nuls de racines
        lineairement independantes : P != 0. Alors 1/x = (produit des autres facteurs) / P."""
        items = sorted(self.t.items())
        m = len(items)
        if m == 0:
            raise ZeroDivisionError('inverse de zero')
        if m == 1:
            s, c = items[0]
            inv = Rad._raw({s: 1 / (c * s)})
        else:
            if m > CONJ_MAX_CLASSES:
                raise OracleRefusal('inverse : %d classes > %d' % (m, CONJ_MAX_CLASSES))
            num = Rad.rational(1)
            for signs in product((1, -1), repeat=m - 1):
                if all(e == 1 for e in signs):
                    continue
                factor = {items[0][0]: items[0][1]}
                for (s, c), e in zip(items[1:], signs):
                    factor[s] = c * e
                num = num * Rad._raw(factor)
            norm = self * num
            if set(norm.t) != set([1]):
                raise OracleInvariantError('norme formelle non rationnelle ou nulle : %s' % norm)
            inv = num.scale(1 / norm.t[1])
        if self * inv != Rad.rational(1):
            raise OracleInvariantError('x * (1/x) != 1 pour x = %s' % self)
        return inv

    def bounds(self, prec):
        """Encadrement entier : x dans [lo / D, hi / D], D = den * 2^prec (isqrt par exces et par defaut)."""
        den = 1
        for c in self.t.values():
            den = den * c.denominator // gcd(den, c.denominator)
        lo = hi = 0
        one = 1 << prec
        for s, c in self.t.items():
            a = c.numerator * (den // c.denominator)
            if s == 1:
                low = high = one
            else:  # s > 1 sans facteur carre n'est jamais un carre : low < sqrt(s) 2^prec < low + 1
                low = isqrt(s << (2 * prec))
                high = low + 1
            if a > 0:
                lo += a * low
                hi += a * high
            else:
                lo += a * high
                hi += a * low
        return lo, hi, den << prec

    def sign(self):
        """Signe exact : 0 ssi la forme canonique est vide ; sinon encadrements a precision doublee."""
        if not self.t:
            return 0
        values = list(self.t.values())
        if all(c > 0 for c in values):
            return 1
        if all(c < 0 for c in values):
            return -1
        prec = 64
        while prec <= SIGN_MAX_BITS:
            lo, hi, _den = self.bounds(prec)
            if lo > 0:
                return 1
            if hi < 0:
                return -1
            prec *= 2
        raise OracleRefusal('signe non decide a %d bits pour %s' % (SIGN_MAX_BITS, self))

    def approx(self):
        lo, hi, den = self.bounds(96)
        return float(Fraction(lo + hi, 2 * den))

    def __str__(self):
        if not self.t:
            return '0'
        parts = []
        for s, c in sorted(self.t.items()):
            mag = abs(c)
            if s == 1:
                body = str(mag)
            elif mag == 1:
                body = 'sqrt(%d)' % s
            else:
                body = '%s*sqrt(%d)' % (mag, s)
            parts.append(('-' if c < 0 else '+', body))
        out = ('-' if parts[0][0] == '-' else '') + parts[0][1]
        for sg, body in parts[1:]:
            out += ' %s %s' % (sg, body)
        return out

    __repr__ = __str__


def rad_cmp(x, y):
    return (x - y).sign()


def fmt_level(x):
    return '%s ~ %.6f' % (x, x.approx())


# ---------------------------------------------------------------------------------------------- structures

def _groups(items, linked):
    """Composantes de la relation linked sur items (union-find) : liste de frozensets triee par plus petit site."""
    items = list(items)
    parent = dict((i, i) for i in items)

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for a in range(len(items)):
        for b in range(a + 1, len(items)):
            i, j = items[a], items[b]
            if linked(i, j):
                ri, rj = find(i), find(j)
                if ri != rj:
                    parent[max(ri, rj)] = min(ri, rj)
    out = {}
    for i in items:
        out.setdefault(find(i), set()).add(i)
    return sorted((frozenset(b) for b in out.values()), key=min)


def _check_points(points):
    pts = [tuple(int(c) for c in p) for p in points]
    for p in pts:
        if len(p) != 3:
            raise ValueError('triplet attendu : %r' % (p,))
    if len(set(pts)) != len(pts):
        raise ValueError('sites distincts attendus')
    if not pts:
        raise ValueError('nuage vide')
    return pts


def _check_decimal(date, e_dec):
    """Garde : la date exacte reconstruite tombe dans l'arrondi Decimal (120 chiffres) de points_reference."""
    lo, hi, den = date.bounds(400)
    ref = Fraction(e_dec)
    slack = Fraction(1, 10 ** 100)
    if not (Fraction(lo, den) - slack <= ref <= Fraction(hi, den) + slack):
        raise OracleInvariantError('date exacte %s hors de la date Decimal %s' % (date, e_dec))


class Structure(object):
    """Arbre de blocs d'un cote : niveaux d'evenement exacts (Rad, croissants) et blocs a chaque niveau."""

    def __init__(self, side, n, levels, blocks, extra=None):
        self.side, self.n, self.levels, self.blocks, self.extra = side, n, levels, blocks, extra or {}


def tower_structure(points, k):
    """Cote T : H^r_{k+1} depuis Definition et reference_radius_rules ; blocs par l'ultrametrique exacte u."""
    pts = _check_points(points)
    n = len(pts)
    if not isinstance(k, int) or k < 1 or k + 1 > n:
        raise ValueError('ordre k hors de [1, n - 1] : %r' % (k,))
    res = Definition(pts).order(k)
    entries, tree = pref.reference_radius_rules(res, n, m=k + 1)
    rows = entries['margin_r']
    dates, owners, triples = [], [], []
    for e_dec, owner, (t, meet, h) in rows:
        date = Rad.sqrt_of(t) + Rad.sqrt_of(meet) - Rad.sqrt_of(h)
        if date.sign() <= 0:
            raise OracleRefusal('date nulle ou negative : %s' % date)
        _check_decimal(date, e_dec)
        dates.append(date)
        owners.append(owner)
        triples.append((t, meet, h))
    node_rad = [Rad.sqrt_of(node.level) for node in res.nodes]
    uniq = {}
    for x in dates + node_rad:
        uniq.setdefault(x.key(), x)
    levels = sorted(uniq.values(), key=cmp_to_key(rad_cmp))
    for a, b in zip(levels, levels[1:]):
        if rad_cmp(a, b) >= 0:
            raise OracleInvariantError('niveaux non strictement croissants')
    rank = dict((x.key(), r) for r, x in enumerate(levels))
    e_rank = [rank[d.key()] for d in dates]
    v_rank = [rank[x.key()] for x in node_rad]
    u = [[0] * n for _ in range(n)]
    for i in range(n):
        u[i][i] = e_rank[i]
        for j in range(i + 1, n):
            w = tree.lca(owners[i], owners[j])
            r = max(e_rank[i], e_rank[j])
            if w != owners[i] and w != owners[j]:
                r = max(r, v_rank[w])
            u[i][j] = u[j][i] = r
    for i in range(n):  # u est une ultrametrique (les blocs sont une partition a chaque niveau)
        for j in range(n):
            for l in range(n):
                if u[i][l] > max(u[i][j], u[j][l]):
                    raise OracleInvariantError('u non ultrametrique en (%d, %d, %d)' % (i, j, l))

    def blocks(r):
        engaged = [i for i in range(n) if e_rank[i] <= r]
        return _groups(engaged, lambda i, j: u[i][j] <= r)
    node_keys = set(x.key() for x in node_rad)
    coincidences = len(dates) + len(node_keys) - len(levels)  # dates egales a un rayon de noeud ou a une date
    extra = {'dates': dates, 'owners': owners, 'triples': triples, 'e_rank': e_rank, 'u_rank': u,
             'coincidences': coincidences}
    return Structure('T', n, levels, blocks, extra)


def hdbscan_structure(points, k):
    """Cote A : lien simple exact de l'atteignabilite mutuelle (carres entiers, soi compris), plateaux groupes."""
    pts = _check_points(points)
    n = len(pts)
    if not isinstance(k, int) or k < 1 or k > n:
        raise ValueError('min_samples k hors de [1, n] : %r' % (k,))
    d2 = [[sum((a - b) ** 2 for a, b in zip(p, q)) for q in pts] for p in pts]
    core2 = [sorted(row)[k - 1] for row in d2]
    mr2 = [[max(core2[a], core2[b], d2[a][b]) for b in range(n)] for a in range(n)]
    parent = list(range(n))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    heights = set()
    for value, a, b in sorted((mr2[a][b], a, b) for a in range(n) for b in range(a + 1, n)):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[max(ra, rb)] = min(ra, rb)
            heights.add(value)
    squares = sorted(heights)
    levels = [Rad.sqrt_of(v) for v in squares]

    def blocks(r):
        bound = squares[r]
        return _groups(range(n), lambda a, b: mr2[a][b] <= bound)
    return Structure('A', n, levels, blocks, {'core2': core2, 'squares': squares})


# ---------------------------------------------------------------------------------------------- tete commune

def condense(struct, mcs):
    """Condensation critere A en balayage croissant, plateaux atomiques, multifusions N-aires jamais binarisees.
    Chaque cluster : bas, haut (rangs de niveau ; haut None pour la racine), joins {site: rang ou il rejoint},
    children. Rend (clusters, racine) ; racine None si aucun gros bloc."""
    clusters, live = [], {}
    for r in range(len(struct.levels)):
        new_live = {}
        for block in struct.blocks(r):
            if len(block) < mcs:
                continue
            inside = sorted(cid for blk, cid in live.items() if blk <= block)
            if not inside:
                cid = len(clusters)
                clusters.append({'bas': r, 'haut': None, 'joins': dict((x, r) for x in block), 'children': []})
            elif len(inside) == 1:
                cid = inside[0]
                joins = clusters[cid]['joins']
                if not set(joins) <= block:
                    raise OracleInvariantError('bloc vivant non inclus')
                for x in block:
                    if x not in joins:
                        joins[x] = r
            else:
                cid = len(clusters)
                for c in inside:
                    clusters[c]['haut'] = r
                clusters.append({'bas': r, 'haut': None, 'joins': dict((x, r) for x in block), 'children': inside})
            new_live[block] = cid
        for blk in live:
            if not any(blk <= b for b in new_live):
                raise OracleInvariantError('cluster vivant perdu au niveau %d' % r)
        live = new_live
    if not clusters:
        return clusters, None
    if len(live) != 1:
        raise OracleInvariantError('plusieurs clusters vivants au dernier niveau')
    (block, root), = live.items()
    if len(block) != struct.n:
        raise OracleInvariantError('le dernier niveau ne contient pas tous les sites')
    for cid, c in enumerate(clusters):
        if (c['haut'] is None) != (cid == root):
            raise OracleInvariantError('haut absent hors racine')
    return clusters, root


def scores(struct, clusters, root, z):
    """S(C) = somme_x phi(rejoint_x) - phi(haut_C), phi(l) = l^(-z) exact (Rad)."""
    cache = {}

    def phi(r):
        if r not in cache:
            level = struct.levels[r]
            if level.sign() <= 0:
                raise OracleInvariantError('phi d un niveau nul')
            cache[r] = level.inverse().power(z)
        return cache[r]
    out = {}
    for cid, c in enumerate(clusters):
        if cid == root:
            continue
        counts = {}
        for r in c['joins'].values():
            counts[r] = counts.get(r, 0) + 1
        total = phi(c['haut']).scale(-len(c['joins']))
        for r, m in sorted(counts.items()):
            total = total + phi(r).scale(m)
        if total.sign() <= 0:
            raise OracleInvariantError('score non strictement positif')
        out[cid] = total
    return out


def _parents(clusters):
    parent = [None] * len(clusters)
    for cid, c in enumerate(clusters):
        for d in c['children']:
            parent[d] = cid
    return parent


def _antichains(cid, clusters, memo):
    """Antichaines du sous-arbre de cid (vide comprise), en tuples d'identifiants."""
    if cid not in memo:
        combos = [()]
        for d in clusters[cid]['children']:
            sub = _antichains(d, clusters, memo)
            combos = [a + b for a in combos for b in sub]
            if len(combos) > ANTICHAIN_MAX:
                raise OracleRefusal('plus de %d antichaines' % ANTICHAIN_MAX)
        memo[cid] = combos + [(cid,)]
    return memo[cid]


def _value(antichain, score):
    total = Rad()
    for c in antichain:
        total = total + score[c]
    return total


def _best(values):
    best = values[0]
    for v in values[1:]:
        if rad_cmp(v, best) > 0:
            best = v
    return best


def _select_eom(clusters, root, score):
    """Juge EOM par enumeration. Les antichaines de la foret non racine sont les produits des antichaines des
    sous-arbres des enfants de la racine, de valeurs additives : une antichaine est optimale ssi chacune de ses
    restrictions l'est. O = reunion des antichaines optimales ; selection = elements maximaux de O.
    Equivalence avec le programme dynamique a parent sur egalite (V(C) = max(S(C), somme V(D)), C retenu ssi
    S(C) >= somme V(D) et aucun ancetre non racine retenu) : si le PD retient C, la selection du PD est optimale
    et contient C, et aucun ancetre P n'est dans O car S(P) < V(P) ; reciproquement, si C est maximal dans O et
    un ancetre P avait S(P) >= somme V(D), la restriction a P d'une antichaine optimale contenant C vaut V(P)
    = S(P), et la remplacer par {P} garde l'optimalite : P serait dans O, contradiction.
    Rend (selection triee, comparaisons, egalites certifiees)."""
    parent = _parents(clusters)
    memo = {}
    in_o = set()
    for top in clusters[root]['children']:
        ants = _antichains(top, clusters, memo)
        values = [_value(a, score) for a in ants]
        best = _best(values)
        for a, v in zip(ants, values):
            if (v - best).is_zero():
                in_o.update(a)

    def has_ancestor_in(c, pool):
        p = parent[c]
        while p is not None and p != root:
            if p in pool:
                return True
            p = parent[p]
        return False
    selected = sorted(c for c in in_o if not has_ancestor_in(c, in_o))
    # comparaisons EOM : S(C) contre somme des V(D) des enfants, V par enumeration du sous-arbre
    vbest = {}
    for cid in range(len(clusters)):
        if cid != root:
            vbest[cid] = _best([_value(a, score) for a in _antichains(cid, clusters, memo) if a])
    comparisons = equalities = 0
    decide = {}
    for cid, c in enumerate(clusters):
        if cid == root or not c['children']:
            continue
        kids = Rad()
        for d in c['children']:
            kids = kids + vbest[d]
        diff = score[cid] - kids
        comparisons += 1
        if diff.is_zero():
            equalities += 1
        decide[cid] = diff.sign() >= 0
    # recoupement par un programme dynamique ecrit a part (parent sur egalite certifiee)
    dp = []

    def descend(c):
        if not clusters[c]['children'] or decide[c]:
            dp.append(c)
        else:
            for d in clusters[c]['children']:
                descend(d)
    for top in clusters[root]['children']:
        descend(top)
    if sorted(dp) != selected:
        raise OracleInvariantError('enumeration %s != programme dynamique %s' % (selected, sorted(dp)))
    return selected, comparisons, equalities


def head(struct, mcs, z=1, selection='eom'):
    """Tete commune : condensation, scores, selection, labels. Rend un dict detaille (objets Rad compris)."""
    if not isinstance(mcs, int) or mcs < 2:
        raise ValueError('mcs >= 2 attendu')
    if not isinstance(z, int) or z < 1:
        raise ValueError('z entier >= 1 attendu')
    if selection not in ('eom', 'leaf'):
        raise ValueError('selection eom ou leaf')
    n = struct.n
    clusters, root = condense(struct, mcs) if n >= mcs else ([], None)
    score = scores(struct, clusters, root, z) if clusters else {}
    selected, comparisons, equalities = [], 0, 0
    if clusters and clusters[root]['children']:
        if selection == 'eom':
            selected, comparisons, equalities = _select_eom(clusters, root, score)
        else:
            selected = sorted(c for c in range(len(clusters)) if c != root and not clusters[c]['children'])
    labels = [-1] * n
    for c in selected:
        members = sorted(clusters[c]['joins'])
        for x in members:
            if labels[x] != -1:
                raise OracleInvariantError('clusters retenus non disjoints')
            labels[x] = members[0]
    for c in selected:
        if len(clusters[c]['joins']) < mcs:
            raise OracleInvariantError('cluster retenu sous mcs')
    return {'struct': struct, 'clusters': clusters, 'root': root, 'score': score, 'selected': selected,
            'comparisons': comparisons, 'equalities': equalities, 'labels': labels}


def publish(detail, k, mcs, z, selection):
    struct = detail['struct']
    rows = []
    for cid, c in enumerate(detail['clusters']):
        rows.append({'members': sorted(c['joins']), 'bas': fmt_level(struct.levels[c['bas']]),
                     'haut': None if c['haut'] is None else fmt_level(struct.levels[c['haut']]),
                     'score': None if cid == detail['root'] else str(detail['score'][cid]),
                     'children': list(c['children']), 'root': cid == detail['root']})
    return {'side': struct.side, 'n': struct.n, 'k': k, 'mcs': mcs, 'z': z, 'selection': selection,
            'labels': list(detail['labels']), 'clusters': rows, 'root': detail['root'],
            'selected': list(detail['selected']), 'certified_equalities': detail['equalities'],
            'eom_comparisons': detail['comparisons'], 'levels': len(struct.levels),
            'level_coincidences': struct.extra.get('coincidences', 0)}


_STRUCTURES = {}
_STRUCTURES_MAX = 64


def _structure(side, points, k):
    """Structure d'un cote, memorisee par (cote, sites, k) : elle ne depend ni de mcs, ni de z, ni de la selection,
    et la tete ne la modifie pas (Definition domine le cout : reutilisee entre les (mcs, z, selection))."""
    key = (side, tuple(tuple(int(c) for c in p) for p in points), k)
    hit = _STRUCTURES.get(key)
    if hit is None:
        hit = tower_structure(points, k) if side == 'T' else hdbscan_structure(points, k)
        if len(_STRUCTURES) >= _STRUCTURES_MAX:
            _STRUCTURES.clear()
        _STRUCTURES[key] = hit
    return hit


def tower_flat(points, k, mcs, z=1, selection='eom'):
    """Sortie plate de la tete certifiee sur H^r_{k+1} (cote T, rayons)."""
    return publish(head(_structure('T', points, k), mcs, z, selection), k, mcs, z, selection)


def hdbscan_flat(points, k, mcs, z=1, selection='eom'):
    """Sortie plate de la meme tete sur le lien simple de l'atteignabilite mutuelle (cote A, distances)."""
    return publish(head(_structure('A', points, k), mcs, z, selection), k, mcs, z, selection)


# ---------------------------------------------------------------------------------------------- auto-test

def partition(labels):
    groups = {}
    for i, lab in enumerate(labels):
        if lab != -1:
            groups.setdefault(lab, []).append(i)
    return {'clusters': sorted(sorted(g) for g in groups.values()),
            'bruit': [i for i, lab in enumerate(labels) if lab == -1]}


def _part(clusters, n):
    used = set(x for c in clusters for x in c)
    return {'clusters': sorted(sorted(c) for c in clusters), 'bruit': [i for i in range(n) if i not in used]}


def _find(detail, members):
    for cid, c in enumerate(detail['clusters']):
        if sorted(c['joins']) == sorted(members):
            return cid
    return None


F1 = [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]
F3 = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
F4 = [(x, 0, 0) for x in (0, 2, 4, 7, 9, 11, 17, 19, 21)]
F7 = [(1, 6, 0), (2, 3, 3), (2, 7, 5), (3, 5, 7), (4, 2, 5), (5, 6, 8), (7, 0, 7), (8, 3, 2), (8, 5, 6),
      (8, 5, 8)]
F8 = [(x, 0, 0) for x in (0, 3, 7, 16, 22, 27, 99, 107, 114)]
F9 = [(0, 0, 3), (0, 3, 0), (0, 5, 4), (0, 6, 5), (1, 0, 3), (2, 4, 2)]
F11 = [(0, 0, 0), (10, 0, 0), (20, 2, 0), (5, 9, 0), (15, 9, 1), (10, 18, 0), (100, 0, 0), (103, 0, 0)]
# F9, cote A seulement (min_samples = 3, mcs = 2) : sortie gravee depuis cet oracle le 4 octobre 2026 (SPEC
# par. 4.3 : << A1 grave depuis l'oracle >>). core2 = (18, 18, 9, 17, 18, 9) ; plateaux de mr2 : 9 {2, 5}, 17
# {2, 3, 5}, 18 : {0, 1, 4} se forme ET rejoint {2, 3, 5} au meme niveau exact (aretes 0-1, 0-4, 1-5, 4-5 a 18).
# {0, 1, 4} n'est donc jamais un bloc atomique : un seul cluster (la racine, ne en 3 = sqrt(9)), aucune scission,
# tout bruit. sklearn rend [0, 0, 1, 1, 0, 1] parce qu'il binarise le plateau 18 (contre-exemple a M3).
F9_GRAVE = {'labels': [-1] * 6, 'egalites': 0,
            'clusters': [{'members': [0, 1, 2, 3, 4, 5], 'bas': '3 ~ 3.000000', 'haut': None, 'children': []}]}


def f5_points(t, embed=False):
    xs = [0, 6, 12] + [x + t for x in (22, 28, 34, 52, 58, 64)]
    return [(x, x, 0) if embed else (x, 0, 0) for x in xs]


def _eom_equality(detail, parent_members, kid_members):
    """(S(parent), somme S(enfants)) exacts pour des clusters designes par leurs membres ; None si absents."""
    p = _find(detail, parent_members)
    kids = [_find(detail, m) for m in kid_members]
    if p is None or any(c is None for c in kids) or p == detail['root']:
        return None
    total = Rad()
    for c in kids:
        total = total + detail['score'][c]
    return detail['score'][p], total


def self_test(record=None):
    """Fixtures F1, F3-F9, F11 et invariants D1 (k = 1) et D2 (theoreme 9) sur petits nuages ; rend les ecarts.
    record : liste facultative ou chaque verification est consignee (fixture, attendu, obtenu)."""
    gaps = []
    log = record if record is not None else []

    def check(name, got, want, note=''):
        row = {'fixture': name, 'attendu': want, 'obtenu': got, 'conforme': got == want}
        if note:
            row['note'] = note
        log.append(row)
        if got != want:
            gaps.append(row)

    tally = {'egalites_certifiees': 0, 'sorties': 0}

    def run(side, pts, k, mcs, z=1, selection='eom'):
        struct = tower_structure(pts, k) if side == 'T' else hdbscan_structure(pts, k)
        det = head(struct, mcs, z, selection)
        tally['egalites_certifiees'] += det['equalities']
        tally['sorties'] += 1
        return det
    noise6 = {'clusters': [], 'bruit': list(range(6))}
    # F1 : deux triangles equilateraux exacts
    for mcs in (2, 3):
        check('F1 T k=2 mcs=%d eom z=1' % mcs, partition(run('T', F1, 2, mcs)['labels']),
              {'clusters': [[0, 1, 2], [3, 4, 5]], 'bruit': []})
        check('F1 A k=2 mcs=%d eom z=1' % mcs, partition(run('A', F1, 2, mcs)['labels']), noise6)
    check('F1 T k=2 mcs=4 eom z=1', partition(run('T', F1, 2, 4)['labels']), noise6)
    check('F1 A k=2 mcs=4 eom z=1', partition(run('A', F1, 2, 4)['labels']), noise6)
    # F3 : cinq points, et sous les 120 permutations (labels ramenes aux indices d'origine)
    want3 = {'clusters': [[1, 2], [3, 4]], 'bruit': [0]}
    for side in ('T', 'A'):
        check('F3 %s k=2 mcs=2 eom z=1' % side, partition(run(side, F3, 2, 2)['labels']), want3)
        bad = 0
        for perm in permutations(range(5)):
            lab = run(side, [F3[i] for i in perm], 2, 2)['labels']
            got = _part([[perm[x] for x in c] for c in partition(lab)['clusters']], 5)
            bad += got != want3
        check('F3 %s 120 permutations : partitions differentes' % side, bad, 0)
    # F4 : neuf sites sur un axe, cote T
    a, b, d = [0, 1, 2], [3, 4, 5], [6, 7, 8]
    check('F4 T k=2 mcs=3 eom z=1', partition(run('T', F4, 2, 3)['labels']), _part([a + b, d], 9))
    check('F4 T k=2 mcs=3 eom z=3', partition(run('T', F4, 2, 3, z=3)['labels']), _part([a, b, d], 9))
    check('F4 T k=2 mcs=3 leaf', partition(run('T', F4, 2, 3, selection='leaf')['labels']), _part([a, b, d], 9))
    for z, sel in ((1, 'eom'), (3, 'eom'), (1, 'leaf')):
        check('F4 T k=2 mcs=4 %s z=%d' % (sel, z), partition(run('T', F4, 2, 4, z, sel)['labels']), _part([], 9))
    # F5 : translations t = -1, 0, +1 de B et D ; F6 : t = 0 plonge par x -> (x, x, 0)
    for t, want in ((-1, [a + b, d]), (0, [a + b, d]), (1, [a, b, d])):
        det = run('T', f5_points(t), 2, 3)
        check('F5 T t=%+d k=2 mcs=3 eom z=1' % t, partition(det['labels']), _part(want, 9))
        if t == 0:
            check('F5 T t=0 egalites certifiees', det['equalities'], 1)
            eq = _eom_equality(det, a + b, [a, b])
            check('F5 T t=0 S(AuB), S(A)+S(B)', None if eq is None else [str(eq[0]), str(eq[1])], ['1/4', '1/4'])
    det = run('T', f5_points(0, embed=True), 2, 3)
    check('F6 T k=2 mcs=3 eom z=1', partition(det['labels']), _part([a + b, d], 9))
    check('F6 T egalites certifiees', det['equalities'], 1)
    eq = _eom_equality(det, a + b, [a, b])
    check('F6 T S(AuB), S(A)+S(B)', None if eq is None else [str(eq[0]), str(eq[1])],
          ['1/8*sqrt(2)', '1/8*sqrt(2)'])
    # F7 : dix points, regle de mort par masse engagee
    check('F7 T k=2 mcs=2 eom z=1', partition(run('T', F7, 2, 2)['labels']),
          {'clusters': [[1, 2, 3, 4], [5, 8, 9]], 'bruit': [0, 6, 7]})
    # F8 : 1D, k = 1, egalite exacte S(AuB) = S(A) + S(B)
    for side, value in (('A', '7/12'), ('T', '7/6')):
        det = run(side, F8, 1, 3)
        check('F8 %s k=1 mcs=3 eom z=1' % side, partition(det['labels']), _part([a + b, d], 9))
        check('F8 %s egalites certifiees' % side, det['equalities'], 1)
        eq = _eom_equality(det, a + b, [a, b])
        check('F8 %s S(AuB), S(A)+S(B)' % side, None if eq is None else [str(eq[0]), str(eq[1])], [value, value])
    # F9 : cote A seulement, min_samples = 3, mcs = 2 : gravee depuis l'oracle
    det = run('A', F9, 3, 2)
    pub = publish(det, 3, 2, 1, 'eom')
    got9 = {'labels': det['labels'], 'egalites': det['equalities'],
            'clusters': [dict((key, c[key]) for key in ('members', 'bas', 'haut', 'children'))
                         for c in pub['clusters']]}
    check('F9 A k=3 mcs=2 eom z=1 (grave depuis l oracle)', got9, F9_GRAVE)
    # F11 : paire a boule mixte, cote T, k = 2, mcs = 2
    det = run('T', F11, 2, 2)
    cid = _find(det, [6, 7])
    if cid is None:
        check('F11 T bloc {6, 7} condense', None, 'present')
    else:
        c = det['clusters'][cid]
        lv = det['struct'].levels
        check('F11 T {6,7} bas = sqrt(6893/4)', lv[c['bas']] == Rad.sqrt_of(Fraction(6893, 4)), True,
              'bas = %s' % fmt_level(lv[c['bas']]))
        top = None if c['haut'] is None else lv[c['haut']].approx()
        check('F11 T {6,7} haut ~ 42,741 (|haut - 42.7405| <= 5e-4)',
              top is not None and abs(top - 42.7405) <= 5e-4, True,
              'haut = %s' % (None if c['haut'] is None else fmt_level(lv[c['haut']])))
        check('F11 T {6,7} haut = sqrt(7307/4) (grave depuis l oracle)',
              c['haut'] is not None and lv[c['haut']] == Rad.sqrt_of(Fraction(7307, 4)), True,
              'valeur exacte 42.740496... : << 42,741 >> de la SPEC est un double arrondi de 42,7405 (rapport modele)')
        check('F11 T {6,7} feuille (sans enfant)', c['children'], [])
    det_leaf = run('T', F11, 2, 2, selection='leaf')
    check('F11 T k=2 mcs=2 leaf', partition(det_leaf['labels']),
          {'clusters': [[0, 3], [1, 4], [6, 7]], 'bruit': [2, 5]}, 'attendu du rapport modele (critere A)')
    check('F11 T k=2 mcs=2 eom z=1', partition(det['labels']),
          {'clusters': [[0, 1, 2, 3, 4, 5], [6, 7]], 'bruit': []}, 'attendu du rapport modele (critere A)')
    # D1 (k = 1 : T et A identiques) et D2 (theoreme 9 : z = 3 raffine z = 1, feuilles raffinent z = 3)
    rng = random.Random(20261004)
    d1_bad = d2_bad = d1_runs = d2_runs = 0
    for trial in range(8):
        n = rng.randint(6, 9)
        cloud = set()
        while len(cloud) < n:
            cloud.add(tuple(rng.randint(0, 5) for _ in range(3)))
        cloud = sorted(cloud)
        ts, as_ = tower_structure(cloud, 1), hdbscan_structure(cloud, 1)
        for mcs in (2, 3, 4):
            for z, sel in ((1, 'eom'), (3, 'eom'), (1, 'leaf')):
                d1_runs += 1
                if head(ts, mcs, z, sel)['labels'] != head(as_, mcs, z, sel)['labels']:
                    d1_bad += 1
        for k in (2, 3):
            for struct in (tower_structure(cloud, k), hdbscan_structure(cloud, k)):
                for mcs in (2, 3):
                    s1, s3, sl = (head(struct, mcs, 1, 'eom'), head(struct, mcs, 3, 'eom'),
                                  head(struct, mcs, 1, 'leaf'))
                    d2_runs += 1
                    if not (_refines(s3, s1) and _refines(sl, s3)):
                        d2_bad += 1
    check('D1 k=1 labels T == A (%d sorties)' % d1_runs, d1_bad, 0)
    check('D2 theoreme 9, raffinement (%d arbres)' % d2_runs, d2_bad, 0)
    check('arithmetique jugee par Decimal et boucles (ecarts)', arith_self_test(), [])
    log.append({'fixture': 'compteurs des fixtures (hors D1, D2)', 'obtenu': dict(tally), 'info': True})
    return gaps


def arith_self_test(trials=300, seed=20261004):
    """Juge de l'arithmetique par une arithmetique autre (Decimal a 120 chiffres, carres parfaits par boucle).
    Rend la liste des ecarts."""
    import decimal
    gaps = []
    for value in range(1, 3001):  # partie sans facteur carre contre la plus grande racine carree divisant value
        q = max(d for d in range(1, isqrt(value) + 1) if value % (d * d) == 0)
        if squarefree_split(value) != (q, value // (q * q)):
            gaps.append({'arith': 'squarefree', 'valeur': value})
    big = (1000003 ** 2) * 999983 * 7  # cofacteur p^2 au-dela de la racine cubique, puis p r
    if squarefree_split(big) != (1000003, 999983 * 7):
        gaps.append({'arith': 'squarefree', 'valeur': big})
    if squarefree_split(1000003 * 999983) != (1, 1000003 * 999983):
        gaps.append({'arith': 'squarefree', 'valeur': 1000003 * 999983})
    two, half = Rad.sqrt_of(2), Rad.sqrt_of(Fraction(1, 2))
    identities = [
        (Rad.sqrt_of(8) - two.scale(2), Rad()),
        (half, two.scale(Fraction(1, 2))),
        ((two + Rad.sqrt_of(3)).power(2), Rad.rational(5) + Rad.sqrt_of(6).scale(2)),
        (Rad.sqrt_of(Fraction(6893, 4)), Rad({6893: Fraction(1, 2)})),
    ]
    for got, want in identities:
        if got != want:
            gaps.append({'arith': 'identite', 'obtenu': str(got), 'attendu': str(want)})
    rng = random.Random(seed)
    ctx = decimal.Context(prec=120)

    def dec_sqrt(f):
        return (decimal.Decimal(f.numerator, ctx) / decimal.Decimal(f.denominator, ctx)).sqrt(ctx)
    for trial in range(trials):
        fr = [Fraction(rng.randint(1, 5000), rng.randint(1, 300)) for _ in range(6)]
        x = Rad.sqrt_of(fr[0]) + Rad.sqrt_of(fr[1]) - Rad.sqrt_of(fr[2])
        y = Rad.sqrt_of(fr[3]) + Rad.sqrt_of(fr[4]) - Rad.sqrt_of(fr[5])
        with decimal.localcontext(ctx):
            dx = dec_sqrt(fr[0]) + dec_sqrt(fr[1]) - dec_sqrt(fr[2])
            dy = dec_sqrt(fr[3]) + dec_sqrt(fr[4]) - dec_sqrt(fr[5])
            diff = dx - dy
            want = 0 if abs(diff) < decimal.Decimal('1e-100') else (1 if diff > 0 else -1)
        if want != 0 and rad_cmp(x, y) != want:
            gaps.append({'arith': 'signe', 'x': str(x), 'y': str(y)})
        if not x.is_zero():
            inv = x.inverse()  # x * (1/x) == 1 est verifie dans inverse()
            with decimal.localcontext(ctx):
                err = abs(decimal.Decimal(repr(inv.approx())) * dx - 1)
            if err > decimal.Decimal('1e-12'):
                gaps.append({'arith': 'inverse', 'x': str(x)})
            cube = inv.power(3)
            if cube * x.power(3) != Rad.rational(1):
                gaps.append({'arith': 'cube', 'x': str(x)})
    near = Rad.sqrt_of(10 ** 12 + 1) - Rad.rational(10 ** 6) - Rad.rational(Fraction(1, 2 * 10 ** 6))
    if near.sign() != -1:  # sqrt(N^2 + 1) < N + 1/(2N) : quasi-egalite a 1e-19 pres
        gaps.append({'arith': 'quasi-egalite', 'x': str(near)})
    return gaps


def _refines(fine, coarse):
    """Tout cluster retenu de fine est descendant (au sens large) d'un cluster retenu de coarse (meme arbre)."""
    parent = _parents(fine['clusters'])
    chosen = set(coarse['selected'])
    for c in fine['selected']:
        p = c
        while p is not None and p not in chosen:
            p = parent[p]
        if p is None:
            return False
    return True


def timing(sizes=(6, 8, 10, 12), orders=(1, 2, 3, 4), clouds=3, side=60, seed=7):
    """Temps moyens par nuage (secondes) : Definition seule, reference_radius_rules seule, structure T complete
    (Definition + regles + niveaux exacts + u), 9 tetes T (mcs 2, 3, 4 x eom z = 1, eom z = 3, feuilles),
    structure A et 9 tetes A. Coordonnees uniformes dans [0, side]^3."""
    rng = random.Random(seed)
    rows = []
    for n in sizes:
        for k in orders:
            if k + 1 > n:
                continue
            keys = ('definition', 'regles', 'structure_T', 'tetes_T', 'structure_A', 'tetes_A')
            acc = dict((key, 0.0) for key in keys)
            nlev = 0
            for _ in range(clouds):
                cloud = set()
                while len(cloud) < n:
                    cloud.add(tuple(rng.randint(0, side) for _ in range(3)))
                cloud = sorted(cloud)
                marks = [time.time()]
                res = Definition(cloud).order(k)
                marks.append(time.time())
                pref.reference_radius_rules(res, n, m=k + 1)
                marks.append(time.time())
                ts = tower_structure(cloud, k)
                marks.append(time.time())
                for mcs in (2, 3, 4):
                    for z, sel in ((1, 'eom'), (3, 'eom'), (1, 'leaf')):
                        head(ts, mcs, z, sel)
                marks.append(time.time())
                as_ = hdbscan_structure(cloud, k)
                marks.append(time.time())
                for mcs in (2, 3, 4):
                    for z, sel in ((1, 'eom'), (3, 'eom'), (1, 'leaf')):
                        head(as_, mcs, z, sel)
                marks.append(time.time())
                for i, key in enumerate(keys):
                    acc[key] += marks[i + 1] - marks[i]
                nlev += len(ts.levels)
            row = {'n': n, 'k': k, 'nuages': clouds, 'coordonnees': '0..%d' % side}
            for key in keys:
                row[key + '_s'] = round(acc[key] / clouds, 4)
            row['niveaux_T_moyen'] = round(nlev / float(clouds), 1)
            rows.append(row)
    return rows


def main(argv):
    if '--self-test' in argv:
        t0 = time.time()
        record = []
        try:
            gaps = self_test(record)
            error = None
        except Exception as exc:  # refus, invariant, ou defaut d'un oracle importe : jamais un crash, code 3
            gaps, error = [{'exception': '%s: %s' % (type(exc).__name__, exc)}], str(exc)
        out = {'oracle': 'points_flat_oracle', 'conforme': not gaps, 'ecarts': gaps, 'verifications': record,
               'duree_s': round(time.time() - t0, 2)}
        if error is not None:
            out['erreur'] = error
        print(json.dumps(out, indent=1, ensure_ascii=True))
        return 0 if not gaps else 3
    if '--timing' in argv:
        print(json.dumps(timing(), indent=1))
        return 0
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
