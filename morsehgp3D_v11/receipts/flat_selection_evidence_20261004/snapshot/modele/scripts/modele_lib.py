#!/usr/bin/env python3
"""Bibliotheque du rapport « modele » : de H^r_{k+1} (oracle exact, petits nuages) a une sortie plate.

Tout est exact : un rayon est une somme rationnelle de racines carrees de rationnels (classe Rad) ; l'egalite de
deux rayons est certifiee par classes de carres rationnelles (des racines de classes distinctes sont lineairement
independantes sur Q), une somme non nulle est separee par encadrements isqrt ; au-dela du budget : refus (Refusal),
jamais une egalite supposee. Les stabilites d'excès de masse (EOM) sont des intervalles rationnels raffines jusqu'a
separation ; une comparaison non separee au budget est signalee (drapeau `non_tranche`), jamais tranchee en silence.

Objets :
  PointHierarchy(points, k)      pendaison H^r_{k+1} de l'oracle (contexte/oracle_hierarchy.py), u(i, j) exact,
                                 dates de comptage des criteres d'existence (A entres, B coeur du noeud,
                                 C resolus, E maturite interpolee), couverture du noeud (D).
  Treegram(n, radii, urank, ...) hierarchie filtree generique (rangs entiers apres tri exact des rayons) ; sert aussi
                                 a l'ultrametrique de HDBSCAN (scikit-learn), en flottants exacts.
  condensed_tree(...)            arbre condense N-aire, plateaux atomiques, criteres de masse par dates de comptage.
  eom_select(...)                programme dynamique d'antichaine (HDBSCAN : parent gagnant aux ex aequo,
                                 racine non selectionnable), lambda = r^(-z), z entier, z = 'log' ou 'leaves'.
"""
from fractions import Fraction
from functools import cmp_to_key
import decimal
import math
import os
import sys

CTX = '/workspaces/E-HGP/build/v11-points-select/contexte'
if CTX not in sys.path:
    sys.path.insert(0, CTX)
import oracle_hierarchy as oh  # noqa: E402  (pose aussi les chemins du banc et de la reference v11)
import points_radius as prad  # noqa: E402
import points_reference as pr  # noqa: E402

Refusal = prad.Refusal
ZERO = Fraction(0)


# ----------------------------------------------------------------------------------------------- rayons exacts

def _classes(terms):
    classes = []
    for c, a in terms:
        if c == 0 or a == 0:
            continue
        for cl in classes:
            ok, q = prad.square_ratio(a, cl[0])
            if ok:
                cl[1] += c * q
                break
        else:
            classes.append([a, Fraction(c)])
    return [(r, c) for r, c in classes if c != 0]


class Rad(object):
    """Somme finie sum_j c_j sqrt(a_j), c_j rationnels, a_j rationnels >= 0."""
    __slots__ = ('terms', '_f')

    def __init__(self, terms):
        self.terms = tuple((Fraction(c), Fraction(a)) for c, a in terms if c != 0 and a != 0)
        self._f = None

    @staticmethod
    def sqrt(a):
        return Rad([(1, a)])

    @staticmethod
    def rat(q):
        """Le rationnel q, ecrit q * sqrt(1)."""
        return Rad([(q, 1)])

    @staticmethod
    def from_rvalue(v):
        return Rad([(1, v.t), (1, v.m), (-1, v.q)])

    def __add__(self, other):
        return Rad(self.terms + other.terms)

    def __sub__(self, other):
        return Rad(self.terms + tuple((-c, a) for c, a in other.terms))

    def scale(self, q):
        q = Fraction(q)
        return Rad([(c * q, a) for c, a in self.terms])

    def approx(self):
        if self._f is None:
            self._f = float(sum(float(c) * math.sqrt(float(a)) for c, a in self.terms))
        return self._f

    def sign(self, budget=8192):
        classes = _classes(self.terms)
        if not classes:
            return 0
        if len(classes) == 1:
            return 1 if classes[0][1] > 0 else -1
        if len(classes) == 2:
            (r1, c1), (r2, c2) = classes
            s1 = 1 if c1 > 0 else -1
            s2 = 1 if c2 > 0 else -1
            if s1 == s2:
                return s1
            d = c1 * c1 * r1 - c2 * c2 * r2
            return s1 * ((d > 0) - (d < 0))
        bits = 64
        while bits <= budget:
            lo = hi = ZERO
            for rep, coef in classes:
                a, b = prad.sqrt_bounds(rep, bits)
                lo += coef * (a if coef > 0 else b)
                hi += coef * (b if coef > 0 else a)
            if lo > 0:
                return 1
            if hi < 0:
                return -1
            bits *= 2
        raise Refusal('somme de radicaux non separee a 2^-%d' % budget)

    def interval(self, bits):
        lo = hi = ZERO
        for c, a in self.terms:
            x, y = prad.sqrt_bounds(a, bits)
            lo += c * (x if c > 0 else y)
            hi += c * (y if c > 0 else x)
        return lo, hi

    def __str__(self):
        out = []
        for c, a in self.terms:
            if a == 1:
                out.append('%s' % c)
            elif c == 1:
                out.append('sqrt(%s)' % a)
            elif c == -1:
                out.append('-sqrt(%s)' % a)
            else:
                out.append('%s*sqrt(%s)' % (c, a))
        return '+'.join(out).replace('+-', '-') if out else '0'


def rcmp(a, b):
    """Ordre exact de deux rayons (Rad ou Fraction)."""
    if isinstance(a, Fraction) and isinstance(b, Fraction):
        return (a > b) - (a < b)
    if isinstance(a, Fraction):
        a = Rad.rat(a)
    if isinstance(b, Fraction):
        b = Rad.rat(b)
    return (a - b).sign()


def rmax(*values):
    best = values[0]
    for v in values[1:]:
        if rcmp(v, best) > 0:
            best = v
    return best


def sort_unique(values):
    """Valeurs triees, doublons exacts fusionnes : (liste unique, fonction rang)."""
    vals = sorted(values, key=cmp_to_key(rcmp))
    uniq = []
    for v in vals:
        if not uniq or rcmp(uniq[-1], v) != 0:
            uniq.append(v)

    def rank(v):
        lo, hi = 0, len(uniq) - 1
        while lo <= hi:
            mid = (lo + hi) // 2
            c = rcmp(v, uniq[mid])
            if c == 0:
                return mid
            if c < 0:
                hi = mid - 1
            else:
                lo = mid + 1
        raise KeyError('rayon absent de la liste')
    return uniq, rank


def rinterval(v, bits):
    if isinstance(v, Fraction):
        return v, v
    return v.interval(bits)


def rfloat(v):
    return float(v) if isinstance(v, Fraction) else v.approx()


# ----------------------------------------------------------------------------------------------- H^r_{k+1}

def popcount(x):
    return bin(x).count('1')


class PointHierarchy(object):
    """H^r_{k+1} (ou H^r_m) d'un petit nuage entier, depuis l'oracle exact de la definition.

    Champs : n, k, m, e[i] (Rad, date d'entree), owner[i], parent[v], birth[v] (Rad, rayon de naissance),
    birth_level[v] (Fraction, niveau carre), d[i] (Rad, rayon de coeur d_k), t[i] (Rad, premiere couverture
    qualifiee), D[i] (Rad, marge), rho[i] (Rad, rayon de resolution : max(e_i, sup des rencontres des rivaux
    qualifies)), sB[i] (Rad, premier rayon >= e_i ou x_i est point de coeur du noeud de son bloc)."""

    def __init__(self, points, k, m=None, names=None):
        self.points = [tuple(int(c) for c in p) for p in points]
        self.names = list(names) if names else [str(i) for i in range(len(self.points))]
        self.h = h = oh.hierarchy(self.points, k, m)
        self.n, self.k, self.m = h.n, h.k, h.m
        self.res = h.res
        self.e = [Rad.from_rvalue(v) for v in h.entry_exact]
        self.owner = list(h.owner)
        self.parent = list(h.parent)
        self.birth_level = list(h.birth_level)
        self.birth = [Rad.sqrt(b) for b in self.birth_level]
        self.d = [Rad.sqrt(Fraction(int(x))) for x in h.order.core_d.tolist()]
        self.tree = pr.Tree(self.res.nodes)
        self._depth = None
        self._rivals()
        self._node_core()

    # --- arbre FULL_k
    def chain(self, v):
        out = [v]
        while self.parent[out[-1]] >= 0:
            out.append(self.parent[out[-1]])
        return out

    def lca(self, a, b):
        seen = set(self.chain(a))
        for w in self.chain(b):
            if w in seen:
                return w
        raise AssertionError('foret')

    def alive_at(self, v, r):
        """Ancetre de v vivant au rayon r (coupe fermee) ; r >= naissance de v."""
        x = v
        while self.parent[x] >= 0 and rcmp(self.birth[self.parent[x]], r) <= 0:
            x = self.parent[x]
        return x

    def u(self, i, j):
        if i == j:
            return self.e[i]
        a, b = self.owner[i], self.owner[j]
        w = self.lca(a, b)
        val = rmax(self.e[i], self.e[j])
        if w not in (a, b):
            val = rmax(val, self.birth[w])
        return val

    # --- rivaux qualifies, marge et resolution (route de la definition, coupes fermees)
    def _rivals(self):
        n, m = self.n, self.m
        qualified = [[] for _ in range(n)]
        for cut in self.res.cuts:
            for v, coverage, _core in cut.closed:
                if popcount(coverage) >= m:
                    for i in range(n):
                        if coverage >> i & 1:
                            qualified[i].append((cut.level, v))
        self.t, self.D, self.rho, self.rival_bars = [], [], [], []
        for i in range(n):
            pts = qualified[i]
            t = min(level for level, _ in pts)
            v1 = next(v for level, v in pts if level == t)
            bars = {}
            for level, v in pts:
                meet = self.tree.meet(v1, t, v, level)
                if meet > level:  # rival : rencontre strictement apres sa propre hauteur
                    key = (v, meet)
                    if key not in bars or level < bars[key]:
                        bars[key] = level
            # barres rivales (naissance c, rencontre M) par branche : plus basse hauteur par (noeud, rencontre)
            bl = sorted(set((c, M) for (v, M), c in bars.items()))
            self.rival_bars.append(bl)
            tr = Rad.sqrt(t)
            Dmax = Rad.rat(ZERO)
            for c, M in bl:
                term = Rad.sqrt(M) - Rad.sqrt(c)
                if rcmp(term, Dmax) > 0:
                    Dmax = term
            self.t.append(tr)
            self.D.append(Dmax)
            # controle : e = t + D (definition de la regle)
            if rcmp(tr + Dmax, self.e[i]) != 0:
                raise AssertionError('date de la regle != t + D pour le site %d' % i)
            rho = self.e[i]
            for c, M in bl:
                rho = rmax(rho, Rad.sqrt(M))
            self.rho.append(rho)

    # --- coeur du noeud (C n X) : premier rayon >= e_i ou x_i est dans la composante de son bloc
    def _node_core(self):
        n = self.n
        cuts = self.res.cuts
        self.sB = []
        for i in range(n):
            ei = self.e[i]
            found = None
            # derniere coupe d'evenement de niveau <= e_i^2, puis les suivantes
            start = 0
            for c, cut in enumerate(cuts):
                if rcmp(Rad.sqrt(cut.level), ei) <= 0:
                    start = c
            for c in range(start, len(cuts)):
                cut = cuts[c]
                r = ei if c == start else Rad.sqrt(cut.level)
                node = self.alive_at(self.owner[i], r)
                for v, _coverage, core in cut.closed:
                    if core >> i & 1 and v == node:
                        found = r
                        break
                if found is not None:
                    break
            if found is None:
                raise AssertionError('site %d jamais point de coeur de sa lignee' % i)
            self.sB.append(found)

    def covered_at(self, node, r):
        """Sites couverts (amas discret) du noeud vivant au rayon r."""
        best = None
        for cut in self.res.cuts:
            if rcmp(Rad.sqrt(cut.level), r) <= 0:
                best = cut
        for v, coverage, _core in best.closed:
            if v == node:
                return [i for i in range(self.n) if coverage >> i & 1]
        raise AssertionError('noeud non vivant')

    def maturity(self, theta):
        """Dates de maturite interpolee max(e, (1 - theta) e + theta d_k) (forme interpolee de la v10 ; un site ne
        compte jamais avant d'etre membre), theta rationnel."""
        th = Fraction(theta)
        return [rmax(self.e[i], self.e[i].scale(1 - th) + self.d[i].scale(th)) for i in range(self.n)]

    def treegram(self, count_dates=None, label='A'):
        n = self.n
        U = [[self.u(i, j) for j in range(n)] for i in range(n)]
        cd = count_dates if count_dates is not None else list(self.e)
        return Treegram(n, U, cd, label=label, names=self.names)


# ----------------------------------------------------------------------------------------------- treegramme

class Treegram(object):
    """Hierarchie filtree : U[i][j] rayon de reunion (U[i][i] = entree), dates de comptage s_i >= U[i][i].

    Les rayons sont des Rad ou des Fraction ; tout est converti en rangs entiers par tri exact."""

    def __init__(self, n, U, count_dates, label='', names=None):
        self.n, self.label = n, label
        self.names = list(names) if names else [str(i) for i in range(n)]
        values = [U[i][j] for i in range(n) for j in range(i, n)] + list(count_dates)
        self.radii, rank = sort_unique(values)
        self.urank = [[rank(U[i][j]) for j in range(n)] for i in range(n)]
        self.srank = [rank(s) for s in count_dates]
        for i in range(n):
            if self.srank[i] < self.urank[i][i]:
                raise AssertionError('date de comptage avant l entree')

    def blocks_at(self, r):
        """Blocs (sites entres, reunis a u <= r) au rang r, coupe fermee."""
        n = self.n
        act = [i for i in range(n) if self.urank[i][i] <= r]
        out, seen = [], set()
        for i in act:
            if i in seen:
                continue
            b = frozenset(j for j in act if self.urank[i][j] <= r)
            seen |= b
            out.append(b)
        return sorted(out, key=lambda b: sorted(b))


def condensed_tree(tg, mcs):
    """Arbre condense N-aire, plateaux atomiques : un rang = toutes les entrees et reunions de ce rayon exact.

    Masse d'un bloc au rang r = nombre de ses sites de date de comptage <= r. Gros bloc : masse >= mcs.
    Un gros bloc sans amas vivant inclus naît (feuille) ; avec un seul, il le continue ; avec >= 2, ils meurent et un
    amas parent naît (multifusion N-aire, jamais binarisee). Rend (amas, defauts de monotonie)."""
    n = tg.n
    R = len(tg.radii)
    clusters = []
    alive = {}  # id d'amas -> bloc courant (frozenset)
    violations = []
    # evenements par rang : entrees et reunions (une ultrametrique : il suffit d'unir les paires de ce rang)
    enter = [[] for _ in range(R)]
    pairs = [[] for _ in range(R)]
    for i in range(n):
        enter[tg.urank[i][i]].append(i)
        for j in range(i + 1, n):
            pairs[tg.urank[i][j]].append((i, j))
    dsu = list(range(n))

    def find(x):
        while dsu[x] != x:
            dsu[x] = dsu[dsu[x]]
            x = dsu[x]
        return x
    active = set()
    for r in range(R):
        active.update(enter[r])
        for i, j in pairs[r]:
            a, b = find(i), find(j)
            if a != b:
                dsu[a] = b
        groups = {}
        for i in active:
            groups.setdefault(find(i), []).append(i)
        blocks = [frozenset(g) for g in groups.values()]
        new_alive = {}
        for b in blocks:
            mass = sum(1 for i in b if tg.srank[i] <= r)
            inside = [c for c, bb in alive.items() if bb <= b]
            if mass < mcs:
                if inside:
                    violations.append(dict(rank=r, block=sorted(b), clusters=inside))
                continue
            if len(inside) == 1:
                c = inside[0]
                new_alive[c] = b
                cl = clusters[c]
            else:
                c = len(clusters)
                cl = dict(id=c, birth=r, death=None, children=inside, parent=None, join={}, members=None)
                clusters.append(cl)
                for ch in inside:
                    clusters[ch]['death'] = r
                    clusters[ch]['parent'] = c
                new_alive[c] = b
            for i in b:
                if tg.srank[i] <= r and i not in cl['join']:
                    cl['join'][i] = r
            cl['block'] = b
        for c, bb in alive.items():
            if c not in new_alive and clusters[c]['death'] is None:
                clusters[c]['death'] = r  # disparition (critere non monotone) : signalee par violations
        alive = new_alive
    for cl in clusters:
        cl['members'] = sorted(cl['block'])
    return clusters, violations


# ----------------------------------------------------------------------------------------------- excès de masse

def _phi_interval(v, z, bits):
    """Intervalle de v^(-z) (z entier >= 1) ; v rayon > 0."""
    lo, hi = rinterval(v, bits)
    if lo <= 0:
        raise Refusal('rayon non separe de zero')
    return hi ** (-z), lo ** (-z)


def _log_interval(v, bits):
    lo, hi = rinterval(v, bits)
    if lo <= 0:
        raise Refusal('rayon nul en echelle log')
    with decimal.localcontext(decimal.Context(prec=60)):
        a = (decimal.Decimal(lo.numerator) / decimal.Decimal(lo.denominator)).ln()
        b = (decimal.Decimal(hi.numerator) / decimal.Decimal(hi.denominator)).ln()
        eps = decimal.Decimal(10) ** -50
        return Fraction(a - eps), Fraction(b + eps)


def stability_interval(tg, cl, z, bits):
    """Intervalle de S(C) = sum_p (phi(j_p) - phi(r_d)), phi = r^(-z) ou -log r ; racine : phi(inf) = 0."""
    lo = hi = ZERO
    death = cl['death']
    for i, j in cl['join'].items():
        if z == 'log':
            if death is None:
                return None  # racine : stabilite infinie en echelle log (jamais selectionnee)
            a, b = _log_interval(tg.radii[death], bits)
            c, d = _log_interval(tg.radii[j], bits)
            lo += a - d
            hi += b - c
        else:
            c, d = _phi_interval(tg.radii[j], z, bits)
            if death is None:
                a = b = ZERO
            else:
                a, b = _phi_interval(tg.radii[death], z, bits)
            lo += c - b
            hi += d - a
    return lo, hi


def eom_select(tg, clusters, z, allow_root=False, budget=4096):
    """Antichaine de stabilite maximale (DP de HDBSCAN, N-aire). z entier, 'log' ou 'leaves'.

    Ex aequo certifie impossible a trancher au budget : parent garde (convention de scikit-learn) et drapeau.
    Rend (ids selectionnes, nombre de comparaisons non tranchees)."""
    if not clusters:
        return [], 0
    root = [c['id'] for c in clusters if c['parent'] is None]
    order = sorted(clusters, key=lambda c: c['birth'])
    if z == 'leaves':
        sel = [c['id'] for c in clusters if not c['children'] and (allow_root or c['parent'] is not None)]
        return sel, 0
    def run(bits, force):
        forced = [0]
        memo = {}

        def St(c):
            if c['id'] in memo:
                return memo[c['id']]
            s = stability_interval(tg, c, z, bits)
            if not c['children']:
                out = (s, [c['id']])
            else:
                lo = hi = ZERO
                sel = []
                for ch in c['children']:
                    got = St(clusters[ch])
                    if got is None:
                        memo[c['id']] = None
                        return None
                    (a, b), chsel = got
                    lo += a
                    hi += b
                    sel += chsel
                if s is None:
                    out = ((lo, hi), sel)
                elif s[0] >= hi:  # parent si S(C) >= somme des optima des enfants (convention scikit-learn)
                    out = (s, [c['id']])
                elif s[1] < lo:
                    out = ((lo, hi), sel)
                elif force:
                    forced[0] += 1
                    out = ((max(s[0], lo), max(s[1], hi)), [c['id']])
                else:
                    out = None
            memo[c['id']] = out
            return out
        selected = []
        for rid in root:
            rc = clusters[rid]
            tops = [rc] if allow_root else [clusters[ch] for ch in rc['children']]
            for c in tops:
                got = St(c)
                if got is None:
                    return None, 0
                selected += got[1]
        return sorted(selected), forced[0]

    bits = 64
    while True:
        try:
            sel, forced = run(bits, bits * 2 > budget)
        except Refusal:
            sel, forced = None, 0
        if sel is not None:
            return sel, forced
        bits *= 2
        if bits > budget:
            sel, forced = run(budget, True)
            return sel, forced


def labels_from_selection(tg, clusters, selected):
    lab = [-1] * tg.n
    for idx, cid in enumerate(selected):
        for i in clusters[cid]['members']:
            if lab[i] != -1:
                raise AssertionError('selection non laminaire')
            lab[i] = idx
    return lab


def partition_of(labels, names):
    groups = {}
    for i, l in enumerate(labels):
        if l >= 0:
            groups.setdefault(l, []).append(names[i])
    return sorted(sorted(g) for g in groups.values())


# ----------------------------------------------------------------------------------------------- HDBSCAN

def hdbscan_ultrametric(xyz, k):
    """Ultrametrique de l'arbre du lien simple de l'atteignabilite mutuelle de scikit-learn (min_samples = k),
    diagonale = distance de coeur ; valeurs flottantes converties exactement en Fraction."""
    import numpy as np
    from sklearn.cluster import HDBSCAN
    X = np.asarray(xyz, dtype=np.float64)
    model = HDBSCAN(min_samples=k, min_cluster_size=2, metric='euclidean', algorithm='kd_tree', n_jobs=1, copy=True)
    model.fit(X)
    tree = np.asarray(model._single_linkage_tree_)
    n = len(X)
    left = tree['left_node'].astype(int).tolist()
    right = tree['right_node'].astype(int).tolist()
    value = tree['value'].tolist()
    members = {i: [i] for i in range(n)}
    U = [[None] * n for _ in range(n)]
    for j, (a, b, v) in enumerate(zip(left, right, value)):
        fa, fb = members.pop(a), members.pop(b)
        fv = Fraction(v)
        for x in fa:
            for y in fb:
                U[x][y] = U[y][x] = fv
        members[n + j] = fa + fb
    # distances de coeur : k-ieme plus proche, soi compris (convention de scikit-learn)
    from sklearn.neighbors import NearestNeighbors
    nn = NearestNeighbors(n_neighbors=k).fit(X)
    dist, _ = nn.kneighbors(X)
    for i in range(n):
        U[i][i] = Fraction(float(dist[i, k - 1]))
        for j in range(n):
            if j != i and U[i][j] < U[i][i]:
                raise AssertionError('arete sous la distance de coeur')
    return U


def hdbscan_labels(xyz, k, mcs, allow_single=False):
    import numpy as np
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_samples=k, min_cluster_size=mcs, metric='euclidean', algorithm='kd_tree', n_jobs=1,
                    copy=True, allow_single_cluster=allow_single, cluster_selection_method='eom')
    model.fit(np.asarray(xyz, dtype=np.float64))
    return model.labels_.tolist()


def same_partition(la, lb):
    """Egalite de deux etiquetages a renumerotation pres (bruit = -1 fixe)."""
    if len(la) != len(lb):
        return False
    m1, m2 = {}, {}
    for a, b in zip(la, lb):
        if (a == -1) != (b == -1):
            return False
        if a == -1:
            continue
        if m1.setdefault(a, b) != b or m2.setdefault(b, a) != a:
            return False
    return True
