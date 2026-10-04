#!/usr/bin/env python3
"""Sortie plate certifiee de la hierarchie de points v11 : arbre de points N-aire, condensation (critere A),
selection EOM N-aire a scores certifies ou feuilles, labels par PointId.

Specification : juge E1 du workflow v11-points-select (4 octobre 2026), § 1.1, 2.3, 4.1-4.2. Port explicite, rien
n'est importe du code de recherche prive : semantique N-aire et plateaux atomiques (lemme D, equite/nary_head.py),
reciproque exacte d'une date (recu morsehgp3D_v11/receipts/eom_exact_audit_20261004, formule contre-relue par
l'auditeur mathematique).

Chaine, identique pour les deux arbres :
  arbre de points  blocs de sites engages ; a un niveau exact, toutes les fusions et toutes les entrees de meme date
                   forment un plateau atomique (coupe fermee) ; aucun bloc vide ou unaire, au plus 2n - 1 blocs.
                   Tour : pendaison H^r_{k+1} (bench/points_radius.py), dates et proprietaires exacts.
                   HDBSCAN : arbre du lien simple de sklearn, niveaux sqrt(N) exacts (N entier relu, fl(sqrt N) verifie),
                   tous les sites presents des le niveau 0.
  condensation     critere A : un bloc est gros s'il compte au moins mcs sites engages ; s'il contient un seul cluster
                   vivant, celui-ci continue ; deux ou plus, ils meurent et un parent nait (N-aire, jamais binarise) ;
                   aucun, un cluster nait. Les sites des petits blocs absorbes et les entrees rejoignent le cluster au
                   niveau du plateau : c'est leur sortie condensee.
  score            S(C) = somme sur les sites p de C de phi(rejoint_p) - phi(haut_C), phi(r) = r^-z dans l'unite native
                   (rayon pour la tour, distance pour HDBSCAN ; un facteur global ne change aucune decision).
  selection        'eom' : C retenu ssi S(C) >= somme des S^ de ses enfants ; egalite CERTIFIEE -> parent.
                   'leaf' : clusters sans enfant. Racine exclue ; sans scission, tout est bruit.
  arithmetique     filtre flottant a borne d'erreur rigoureuse ; sinon repli exact sur des sommes de racines de
                   rationnels : egalite certifiee quand toutes les classes de carres s'annulent (racines de classes
                   distinctes independantes sur Q), signe par encadrements entiers ; au-dela du budget, Refusal
                   (refus compte par l'appelant), jamais un choix force.

Le z de la selection est un cadran de granularite (theoreme 9 du juge : z plus grand raffine) ; le choix de la
selection publiee se fait sur des criteres ecrits d'avance, pas ici.
"""
from fractions import Fraction
import math

import numpy as np

import points_hierarchy as ph
from points_radius import Refusal, square_ratio

ZERO = Fraction(0)
ONE = Fraction(1)
U = 2.0 ** -53
# Petits nombres premiers de la signature de classe de carres (accelere le regroupement ; jamais une decision).
SIGNATURE_PRIMES = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97,
                    101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173)


# ------------------------------------------------------------------------------------------- arithmetique exacte

def _valuation(x, p):
    v = 0
    while x % p == 0:
        x //= p
        v += 1
    return v, x


def class_signature(r):
    """Invariant de la classe de carres d'un rationnel r > 0 : parite des valuations et caractere quadratique de la
    partie inversible, pour une liste fixe de petits premiers. Meme classe => meme signature (la reciproque n'est
    pas supposee : elle est tranchee par square_ratio)."""
    a, b = r.numerator, r.denominator
    out = []
    for p in SIGNATURE_PRIMES:
        va, a1 = _valuation(a, p)
        vb, b1 = _valuation(b, p)
        if p == 2:
            out.append(((va - vb) & 1) * 8 + (a1 * b1) % 8)
        else:
            out.append(((va - vb) & 1) * 4 + (1 if pow((a1 * b1) % p, (p - 1) // 2, p) == 1 else 2))
    return tuple(out)


_SIGNATURES = {}


def _signature(r):
    s = _SIGNATURES.get(r)
    if s is None:
        if len(_SIGNATURES) > 1 << 20:
            _SIGNATURES.clear()
        s = _SIGNATURES[r] = class_signature(r)
    return s


def group_classes(terms):
    """Regroupe une somme de c sqrt(r) par classes de carres : [(coefficient, representant)] sans coefficient nul.
    Exact : deux radicandes ne partagent une classe que si square_ratio le certifie."""
    buckets = {}
    for c, r in terms:
        if not c:
            continue
        key = _signature(r)  # un carre parfait a la signature de 1 : il rejoint les termes rationnels
        bucket = buckets.setdefault(key, [])
        for entry in bucket:
            if entry[1] == r:
                entry[0] += c
                break
            ok, q = square_ratio(r, entry[1])
            if ok:  # sqrt(r) = q sqrt(rep)
                entry[0] += c * q
                break
        else:
            bucket.append([c, r])
    return [(c, r) for bucket in buckets.values() for c, r in bucket if c]


def _interval_sign(terms, bits):
    """Signe de somme c sqrt(r) par encadrement entier en virgule fixe (2^-bits relatif a la plus grande valeur) ;
    0 si l'encadrement contient 0 (non decide)."""
    if not terms:
        return 0
    top = max(abs(float(c)) * math.sqrt(float(r)) for c, r in terms)
    shift = bits - (math.frexp(top)[1] if top > 0 else 0)
    lo = hi = 0
    for c, r in terms:
        a, b = c.numerator, c.denominator
        n, d = r.numerator, r.denominator
        target = n * d
        if shift >= 0:
            target <<= 2 * shift
            s = math.isqrt(target)
            exact = s * s == target
            num_lo, num_hi, den = a * s, a * (s if exact else s + 1), b * d
        else:
            s = math.isqrt(target)
            exact = s * s == target
            num_lo, num_hi, den = a * s, a * (s if exact else s + 1), (b * d) << (-shift)
        if a < 0:
            num_lo, num_hi = num_hi, num_lo
        lo += num_lo // den
        hi += -((-num_hi) // den)
    if lo > 0:
        return 1
    if hi < 0:
        return -1
    return 0


class RadSum(object):
    """Somme finie de c_j sqrt(r_j), c_j rationnels, r_j rationnels > 0 (terme rationnel : r = 1)."""
    __slots__ = ('terms',)

    def __init__(self, terms=()):
        self.terms = [(c, r) for c, r in terms if c]

    def __add__(self, other):
        return RadSum(self.terms + other.terms)

    def __sub__(self, other):
        return RadSum(self.terms + [(-c, r) for c, r in other.terms])

    def scale(self, k):
        return RadSum([(c * k, r) for c, r in self.terms]) if k else RadSum()

    def grouped(self):
        return RadSum(group_classes(self.terms))

    def sign(self, budget_bits=8192):
        """Signe exact ; 0 seulement si l'egalite est certifiee ; Refusal si non separe dans le budget."""
        if not self.terms:
            return 0
        s = _interval_sign(self.terms, 64)
        if s:
            return s
        groups = group_classes(self.terms)
        if not groups:
            return 0
        bits = 128
        while bits <= budget_bits:
            s = _interval_sign(groups, bits)
            if s:
                return s
            bits *= 2
        raise Refusal('somme de radicaux non nulle non separee a 2^-%d' % budget_bits)

    def approx(self):
        return math.fsum(float(c) * math.sqrt(float(r)) for c, r in self.terms)


def _mask_value(mask, t, m, q):
    v = ONE
    if mask & 1:
        v *= t
    if mask & 2:
        v *= m
    if mask & 4:
        v *= q
    return v


def _inverse_date(t, m, q):
    """1/e pour e = sqrt t + sqrt m - sqrt q > 0, dans la base {sqrt t, sqrt m, sqrt q, sqrt(tmq)} codee par masques
    (bit 1 : t, 2 : m, 4 : q) : {masque: coefficient}. Formule du recu eom_exact_audit_20261004 :
    1/e = [(t-m-q) sqrt t + (m-t-q) sqrt m + d sqrt q - 2 sqrt(tmq)] / Delta, d = t+m-q, Delta = d^2 - 4tm ;
    si Delta = 0, e > 0 impose q = (sqrt t - sqrt m)^2 et e = 2 sqrt(min(t, m))."""
    d = t + m - q
    delta = d * d - 4 * t * m
    if delta:
        return {1: (t - m - q) / delta, 2: (m - t - q) / delta, 4: d / delta, 7: Fraction(-2) / delta}
    # Delta = 0 : q = (sqrt t + sqrt m)^2 donne e = 0 (refus) ; q = (sqrt t - sqrt m)^2 donne e = 2 sqrt(min(t, m)).
    low = min(t, m)
    if not low or RadSum([(ONE, t), (ONE, m), (Fraction(-1), q)]).sign() <= 0:
        raise Refusal('date non positive')
    return {1 if low == t else 2: 1 / (2 * low)}


def _mask_mul(x, y, t, m, q):
    out = {}
    for a, ca in x.items():
        for b, cb in y.items():
            c = ca * cb * _mask_value(a & b, t, m, q)
            key = a ^ b
            out[key] = out.get(key, ZERO) + c
    return {k: v for k, v in out.items() if v}


# ------------------------------------------------------------------------------------------------------ niveaux

class Level(object):
    """Niveau exact d'un plateau : sqrt(t) + sqrt(m) - sqrt(q), t, m, q rationnels >= 0 ('sq' si m = q).

    'sq' : rayon (tour) ou distance (HDBSCAN) dont le carre t est un niveau rationnel exact ; 'date' : date d'entree
    de H^r_{k+1} avec rival (t, m, q niveaux carres de FULL)."""
    __slots__ = ('t', 'm', 'q', '_phi', '_exact')

    def __init__(self, t, m=ZERO, q=ZERO):
        t, m, q = Fraction(t), Fraction(m), Fraction(q)
        if m == q:
            m = q = ZERO
        self.t, self.m, self.q = t, m, q
        self._phi = {}
        self._exact = {}

    @property
    def kind(self):
        return 'sq' if not self.m else 'date'

    @classmethod
    def from_rvalue(cls, v):
        return cls(v.t, v.m, v.q)

    def approx(self):
        return math.sqrt(self.t) + math.sqrt(self.m) - math.sqrt(self.q)

    def __str__(self):
        if not self.m:
            return 'sqrt(%s)' % self.t
        return 'sqrt(%s)+sqrt(%s)-sqrt(%s)' % (self.t, self.m, self.q)

    def phi(self, z):
        """(valeur flottante de r^-z, borne rigoureuse de son erreur RELATIVE) ; borne infinie : repli exact force."""
        got = self._phi.get(z)
        if got is not None:
            return got
        if not self.m:
            if not self.t:
                raise Refusal('niveau nul')
            r = math.sqrt(self.t.numerator / self.t.denominator)
            p = r
            for _ in range(z - 1):
                p *= r
            value = 1.0 / p
            err = (2 * z + 3) * U
        else:
            a = math.sqrt(self.t.numerator / self.t.denominator)
            b = math.sqrt(self.m.numerator / self.m.denominator)
            c = math.sqrt(self.q.numerator / self.q.denominator)
            e = (a + b) - c
            slack = 4.0 * U * (a + b + c)
            if e - slack <= 0:
                value, err = (1.0 / e ** z if e > 0 else 0.0), math.inf
            else:
                rho = slack / (e - slack)
                if z * rho > 0.1:
                    value, err = 1.0 / e ** z, math.inf
                else:
                    p = e
                    for _ in range(z - 1):
                        p *= e
                    value = 1.0 / p
                    err = 1.25 * z * rho + (2 * z + 3) * U
        self._phi[z] = (value, err)
        return value, err

    def phi_exact(self, z):
        """r^-z exact (RadSum), z entier >= 1."""
        got = self._exact.get(z)
        if got is not None:
            return got
        t, m, q = self.t, self.m, self.q
        if not m:
            if not t:
                raise Refusal('niveau nul')
            if z % 2:
                out = RadSum([(t ** (-(z + 1) // 2), t)])
            else:
                out = RadSum([(t ** (-z // 2), ONE)])
        else:
            inv = _inverse_date(t, m, q)
            acc = dict(inv)
            for _ in range(z - 1):
                acc = _mask_mul(acc, inv, t, m, q)
            out = RadSum([(c, _mask_value(mask, t, m, q)) for mask, c in acc.items() if c])
        self._exact[z] = out
        return out


class FloatLevel(object):
    """Niveau relu d'un export flottant : valeur et borne de son erreur relative ; aucun repli exact (une
    comparaison EOM non separee par le filtre devient un refus compte)."""
    __slots__ = ('value', 'relerr', '_phi')

    def __init__(self, value, relerr):
        self.value, self.relerr = float(value), float(relerr)
        self._phi = {}

    kind = 'float'

    def approx(self):
        return self.value

    def __str__(self):
        return '%.17g' % self.value

    def phi(self, z):
        got = self._phi.get(z)
        if got is None:
            if self.value <= 0:
                raise Refusal('niveau nul')
            rho = self.relerr
            err = 1.25 * z * rho + (2 * z + 3) * U if z * rho <= 0.1 else math.inf
            got = self._phi[z] = (1.0 / self.value ** z, err)
        return got

    def phi_exact(self, z):
        raise Refusal('niveau flottant : pas de repli exact')


def level_relerr(level):
    """Borne de l'erreur relative de Level.approx() (3 racines correctement arrondies, 2 additions)."""
    if not level.m:
        return 2.0 * U
    a, b, c = math.sqrt(level.t), math.sqrt(level.m), math.sqrt(level.q)
    e = (a + b) - c
    slack = 4.0 * U * (a + b + c)
    return slack / (e - slack) if e > slack else math.inf


# -------------------------------------------------------------------------------------------------- arbre de points

class PointTree(object):
    """Arbre de points N-aire a plateaux atomiques.

    levels[p]        niveau exact du plateau p (strictement croissants) ;
    block_plateau[b] plateau de creation du bloc b (par fusion d'au moins deux blocs non vides, ou par entree) ;
    block_parent[b]  bloc cree par la fusion qui absorbe b (-1 : racine) ; les enfants d'un bloc de fusion sont les
                     blocs dont il est le parent, un bloc d'entree n'en a aucun ;
    site_block[s], site_plateau[s]  bloc et plateau d'entree du site s (indice natif) ;
    ids[s]           PointId (indice d'entree) du site natif s."""

    def __init__(self, n, ids=None):
        self.n = n
        self.ids = np.arange(n, dtype=np.int64) if ids is None else np.asarray(ids, dtype=np.int64)
        self.levels = []
        self.block_plateau = []
        self.block_parent = []
        self.block_merged = []  # vrai si le bloc est cree par fusion
        self.site_block = np.full(n, -1, dtype=np.int64)
        self.site_plateau = np.full(n, -1, dtype=np.int64)

    def add_plateau(self, level):
        self.levels.append(level)
        return len(self.levels) - 1

    def add_block(self, plateau, children=()):
        b = len(self.block_plateau)
        self.block_plateau.append(plateau)
        self.block_parent.append(-1)
        self.block_merged.append(bool(children))
        for c in children:
            self.block_parent[c] = b
        return b

    def enter(self, site, block, plateau):
        self.site_block[site] = block
        self.site_plateau[site] = plateau

    def finish(self):
        """Retire les plateaux sans evenement et verifie la forme (chaque site entre une fois, enfants avant parents)."""
        used = np.zeros(len(self.levels), dtype=bool)
        used[np.asarray(self.block_plateau, dtype=np.int64)] = True
        used[self.site_plateau[self.site_plateau >= 0]] = True
        if not np.all(self.site_plateau >= 0):
            raise ValueError('site jamais entre')
        remap = np.cumsum(used) - 1
        self.levels = [lv for lv, u in zip(self.levels, used) if u]
        self.block_plateau = [int(remap[p]) for p in self.block_plateau]
        self.site_plateau = remap[self.site_plateau]
        for b, parent in enumerate(self.block_parent):
            if parent >= 0 and not self.block_plateau[parent] > self.block_plateau[b]:
                raise ValueError('parent pas plus haut')
        return self

    def blocks(self):
        return len(self.block_plateau)

    # --- serialisation compacte (.npz, sans pickle) : niveaux exacts (table de rationnels) ou flottants bornes ---
    def save(self, path, meta=None, exact=True):
        import json
        common = dict(
            block_plateau=np.array(self.block_plateau, dtype=np.int32),
            block_parent=np.array(self.block_parent, dtype=np.int32),
            block_merged=np.array(self.block_merged, dtype=np.uint8),
            site_block=self.site_block.astype(np.int32), site_plateau=self.site_plateau.astype(np.int32),
            ids=self.ids.astype(np.uint32),
            meta=np.frombuffer(json.dumps(meta or {}, sort_keys=True).encode(), dtype=np.uint8))
        if not exact:
            approx = np.array([lv.approx() for lv in self.levels], dtype=np.float64)
            relerr = np.array([level_relerr(lv) if isinstance(lv, Level) else lv.relerr for lv in self.levels],
                              dtype=np.float64)
            np.savez_compressed(path, level_approx=approx, level_relerr=relerr, **common)
            return
        table, index = [], {}

        def ref(f):
            j = index.get(f)
            if j is None:
                j = index[f] = len(table)
                table.append(f)
            return j
        refs = np.array([(ref(lv.t), ref(lv.m), ref(lv.q)) for lv in self.levels], dtype=np.int32).reshape(-1, 3)
        blob, offsets = bytearray(), [0]
        for f in table:
            for x in (f.numerator, f.denominator):
                blob += x.to_bytes(max(1, (x.bit_length() + 7) // 8), 'little')
                offsets.append(len(blob))
        np.savez_compressed(path, refs=refs, blob=np.frombuffer(bytes(blob), dtype=np.uint8),
                            offsets=np.array(offsets, dtype=np.uint64), **common)

    @classmethod
    def load(cls, path):
        import json
        with np.load(path) as z:
            pt = cls(len(z['ids']), z['ids'])
            if 'refs' in z.files:
                blob, offsets = z['blob'].tobytes(), z['offsets'].astype(np.int64).tolist()
                ints = [int.from_bytes(blob[offsets[i]:offsets[i + 1]], 'little') for i in range(len(offsets) - 1)]
                table = [Fraction(ints[2 * j], ints[2 * j + 1]) for j in range(len(ints) // 2)]
                pt.levels = [Level(table[a], table[b], table[c]) for a, b, c in z['refs'].tolist()]
            else:
                pt.levels = [FloatLevel(v, e) for v, e in zip(z['level_approx'].tolist(), z['level_relerr'].tolist())]
            pt.block_plateau = z['block_plateau'].astype(np.int64).tolist()
            pt.block_parent = z['block_parent'].astype(np.int64).tolist()
            pt.block_merged = [bool(x) for x in z['block_merged'].tolist()]
            pt.site_block = z['site_block'].astype(np.int64)
            pt.site_plateau = z['site_plateau'].astype(np.int64)
            pt.meta = json.loads(z['meta'].tobytes().decode() or '{}')
        return pt


def tower_point_tree(hanging, split_entries=False, binarize=False):
    """Arbre de points de la pendaison (ph.Hanging en niveaux carres ou points_radius.RadiusHanging en rayon), meme
    balayage que ph.evaluate_hanging : au rang r, fusions FULL de rang r puis entrees de date exactement niveau(r) (un
    plateau), puis entrees strictement entre r et r + 1 groupees par date exacte (un plateau par date).
    Mutants de la porte : split_entries (entrees de meme date en plateaux separes), binarize (fusion N-aire en chaine
    de fusions binaires dans l'ordre des indices)."""
    order = hanging.order
    n = order.n
    # Foret FULL sans aucune fusion (un seul noeud, petits nuages a grand k) : merge_list ne la couvre pas.
    kids, merges = order.merge_list() if np.any(order.parent >= 0) else ({}, [])
    entries = sorted(range(n), key=lambda i: (int(hanging.floor[i]), bool(hanging.strict[i])))
    entries = ph.sort_strict_groups(hanging, entries)
    pt = PointTree(n)
    dsu = np.arange(order.size, dtype=np.int64)
    block_of = {}

    def find(x):
        root = x
        while dsu[root] != root:
            root = dsu[root]
        while dsu[x] != root:
            dsu[x], x = root, dsu[x]
        return root

    def enter(i, p):
        root = find(int(hanging.owner[i]))
        b = block_of.get(root)
        if b is None:
            b = block_of[root] = pt.add_block(p)
        pt.enter(i, b, p)

    rank = order.rank.tolist()
    floor, strict = hanging.floor, hanging.strict
    if hasattr(hanging, 'values'):
        def entry_level(i):
            return Level.from_rvalue(hanging.values[i])
    else:
        def entry_level(i):
            return Level(hanging.value(i))  # date en niveau carre : rayon sqrt(date)
    mi = ei = 0
    while mi < len(merges) or ei < len(entries):
        r = min(rank[merges[mi]] if mi < len(merges) else 1 << 62,
                int(floor[entries[ei]]) if ei < len(entries) else 1 << 62)
        level = Level(Fraction(*order.levels.exact(r)))
        p = pt.add_plateau(level)
        while mi < len(merges) and rank[merges[mi]] == r:
            v = merges[mi]
            parts = []
            for c in kids[v]:
                root = find(c)
                b = block_of.pop(root, None)
                if b is not None and b not in parts:
                    parts.append(b)
                dsu[root] = v
            if len(parts) >= 2 and binarize:
                b = pt.add_block(p, parts[:2])
                for x in parts[2:]:
                    p = pt.add_plateau(level)
                    b = pt.add_block(p, [b, x])
                block_of[v] = b
            elif len(parts) >= 2:
                block_of[v] = pt.add_block(p, parts)
            elif parts:
                block_of[v] = parts[0]
            mi += 1
        while ei < len(entries) and int(floor[entries[ei]]) == r and not strict[entries[ei]]:
            enter(entries[ei], p)
            ei += 1
        while ei < len(entries) and int(floor[entries[ei]]) == r:
            i = entries[ei]
            p = pt.add_plateau(entry_level(i))
            while ei < len(entries) and int(floor[entries[ei]]) == r and hanging.cmp_entries(entries[ei], i) == 0:
                enter(entries[ei], p)
                ei += 1
                if split_entries and ei < len(entries) and int(floor[entries[ei]]) == r and \
                        hanging.cmp_entries(entries[ei], i) == 0:
                    p = pt.add_plateau(entry_level(i))
    return pt.finish()


def exact_square_levels(values):
    """Relit les niveaux d'un arbre de sklearn : N = arrondi(v^2) entier et fl(sqrt N) == v, sinon ValueError."""
    v = np.asarray(values, dtype=np.float64)
    big = np.rint(v * v)
    if not np.all(big < 2.0 ** 51):
        raise ValueError('niveau hors de 2^51')
    out = big.astype(np.int64)
    if not np.array_equal(np.sqrt(out.astype(np.float64)), v):
        raise ValueError('niveau non relu exactement')
    return out


def linkage_point_tree(tree, n, binarize=False):
    """Arbre de points de l'arbre du lien simple de sklearn (_single_linkage_tree_ : left, right, value), plateaux
    atomiques par N exact ; les n sites sont des blocs d'entree au plateau 0 (niveau 0, jamais lu car mcs >= 2).
    binarize : mutant M1 (une fusion binaire par ligne, dans l'ordre des lignes)."""
    if tree.dtype.names:
        left, right, value = tree['left_node'], tree['right_node'], tree['value']
    else:
        left, right, value = tree[:, 0], tree[:, 1], tree[:, 2]
    left, right = left.astype(np.int64).tolist(), right.astype(np.int64).tolist()
    big = exact_square_levels(value).tolist()
    if any(big[j] > big[j + 1] for j in range(len(big) - 1)):
        raise ValueError('lignes non triees')
    pt = PointTree(n)
    p0 = pt.add_plateau(Level(ZERO))
    node_block = {}
    for i in range(n):
        pt.enter(i, pt.add_block(p0), p0)
        node_block[i] = i
    dsu = list(range(2 * n))

    def find(x):
        root = x
        while dsu[root] != root:
            root = dsu[root]
        while dsu[x] != root:
            dsu[x], x = root, dsu[x]
        return root
    j = 0
    while j < len(big):
        N = big[j]
        k = j
        while k < len(big) and big[k] == N:
            k += 1
        if binarize:
            for row in range(j, k):
                p = pt.add_plateau(Level(Fraction(N)))
                a, b = find(left[row]), find(right[row])
                dsu[a] = dsu[b] = n + row
                node_block[n + row] = pt.add_block(p, [node_block[a], node_block[b]])
        else:
            p = pt.add_plateau(Level(Fraction(N)))
            parts = {}
            for row in range(j, k):
                for x in (left[row], right[row]):
                    if x < n + j:  # noeud anterieur au plateau : une partie
                        parts[x] = None
            roots_before = {x: find(x) for x in parts}
            for row in range(j, k):
                a, b = find(left[row]), find(right[row])
                dsu[a] = dsu[b] = n + row
            groups = {}
            for x, root in roots_before.items():
                groups.setdefault(find(x), []).append(node_block[root])
            for top, blocks in groups.items():
                blocks = sorted(set(blocks))
                node_block[top] = pt.add_block(p, blocks)
        j = k
    return pt.finish()


# ------------------------------------------------------------------------------------------------- condensation

class Condensed(object):
    """Arbre condense : clusters dans l'ordre de naissance (enfants avant parents)."""

    def __init__(self):
        self.parent, self.children, self.bottom, self.top, self.joins, self.size = [], [], [], [], [], []

    def new(self, plateau, children=()):
        c = len(self.parent)
        self.parent.append(-1)
        self.children.append(list(children))
        self.bottom.append(plateau)
        self.top.append(-1)
        self.joins.append([])
        self.size.append(0)
        for d in children:
            self.parent[d] = c
            self.top[d] = plateau
        return c

    def join(self, c, plateau, count):
        if count:
            self.joins[c].append((plateau, count))
            self.size[c] += count

    def __len__(self):
        return len(self.parent)

    def roots(self):
        return [c for c in range(len(self)) if self.parent[c] < 0]


def condense(pt, mcs, inject=None):
    """Critere A sur l'arbre de points. Rend (Condensed, first) : first[s] = premier cluster du site natif s (-1 si
    aucun). inject : mutants de la porte ('seuil_moins_un', 'sorties_brutes')."""
    if mcs < 2:
        raise ValueError('mcs >= 2')
    threshold = mcs - 1 if inject == 'seuil_moins_un' else mcs
    nb = pt.blocks()
    mass = [0] * nb
    clus = [-1] * nb
    pend = [None] * nb
    children = [[] for _ in range(nb)]
    for b, parent in enumerate(pt.block_parent):
        if parent >= 0:
            children[parent].append(b)
    P = len(pt.levels)
    created = [[] for _ in range(P)]
    for b, p in enumerate(pt.block_plateau):
        created[p].append(b)
    entering = [dict() for _ in range(P)]
    order = np.argsort(pt.site_plateau, kind='stable')
    for s in order.tolist():
        p = int(pt.site_plateau[s])
        entering[p].setdefault(int(pt.site_block[s]), []).append(s)
    cond = Condensed()
    first = np.full(pt.n, -1, dtype=np.int64)
    raw = inject == 'sorties_brutes'
    final = None
    if inject == 'masse_finale':  # mutant : masse = sites qui finiront sous le bloc (couverture), pas les engages
        final = [0] * nb
        for b in pt.site_block.tolist():
            final[b] += 1
        for b in range(nb):  # un parent est cree apres ses enfants
            if pt.block_parent[b] >= 0:
                final[pt.block_parent[b]] += final[b]
    site_plateau = pt.site_plateau
    for p in range(P):
        touched = list(created[p])
        for b in entering[p]:
            if pt.block_plateau[b] != p:
                touched.append(b)
        for B in touched:
            if pt.block_plateau[B] == p:
                parts = children[B]
            else:
                parts = [B]
            fresh = entering[p].get(B, [])
            m_new = sum(mass[x] for x in parts) + len(fresh)
            big = [clus[x] for x in parts if clus[x] >= 0]
            newcomers = []
            for x in parts:
                if clus[x] < 0 and pend[x]:
                    newcomers.extend(pend[x])
            newcomers.extend(fresh)
            for x in parts:
                pend[x] = None
            if (final[B] if final is not None else m_new) < threshold:
                if big:
                    raise ValueError('masse decroissante')
                mass[B], clus[B], pend[B] = m_new, -1, newcomers
                continue
            if not big:
                c = cond.new(p)
                members = newcomers
            elif len(big) == 1:
                c = big[0]
                members = newcomers
            else:
                c = cond.new(p, big)
                members = None
            if raw:  # mutant : sortie de chaque site = sa propre date d'entree, aucune condensation
                if members is None:
                    for d in big:
                        for q, count in cond.joins[d]:
                            cond.join(c, q, count)
                for s in newcomers:
                    cond.join(c, int(site_plateau[s]), 1)
            elif members is None:
                cond.join(c, p, m_new)
            else:
                cond.join(c, p, len(members))
            for s in newcomers:
                first[s] = c
            mass[B], clus[B], pend[B] = m_new, c, None
    tops = cond.roots()
    if len(tops) >= 2:  # foret : racine virtuelle au niveau infini, ses enfants (phi(haut) = 0) sont admissibles
        cond.new(-1, tops)
        for d in tops:
            cond.top[d] = -1
    return cond, first


# --------------------------------------------------------------------------------------------------- selection

class Selection(object):
    def __init__(self, selected, stats):
        self.selected = selected
        self.stats = stats


def _score_float(cond, levels, c, z, top_phi):
    """S(C) flottant et borne rigoureuse de son erreur absolue."""
    vals, err = [], 0.0
    for p, count in cond.joins[c]:
        v, e = levels[p].phi(z)
        term = count * v
        vals.append(term)
        err += abs(term) * (e + U + e * U) / (1.0 - e) if e < 0.5 else math.inf
    vt, et = top_phi
    size = cond.size[c]
    if size and vt:
        term = size * vt
        vals.append(-term)
        err += abs(term) * (et + U + et * U) / (1.0 - et) if et < 0.5 else math.inf
    s = math.fsum(vals)
    return s, (err + U * abs(s)) * 1.0001 + 1e-300


def _score_exact(cond, levels, c, z):
    terms = []
    for p, count in cond.joins[c]:
        terms.extend(levels[p].phi_exact(z).scale(Fraction(count)).terms)
    t = cond.top[c]
    if t >= 0:
        terms.extend(levels[t].phi_exact(z).scale(Fraction(-cond.size[c])).terms)
    return RadSum(terms)


def select(pt, cond, z=1, method='eom', inject=None, budget_bits=8192):
    """Selection sur l'arbre condense. method 'eom' ou 'leaf'. inject : 'egalite_enfants', 'racine_admise',
    'flottant_seul', 'niveau_carre'. Refusal si une comparaison EOM n'est ni separee ni certifiee egale."""
    nc = len(cond)
    stats = dict(clusters=nc, decisions=0, exact=0, equalities=0, margins=[])
    roots = set(c for c in range(nc) if cond.parent[c] < 0)
    admissible = [c not in roots or inject == 'racine_admise' for c in range(nc)]
    zz = 2 * z if inject == 'niveau_carre' else z
    levels = pt.levels
    if method == 'leaf':
        chosen = [admissible[c] and not cond.children[c] for c in range(nc)]
    elif method == 'eom':
        chosen = [False] * nc
        hat = [None] * nc  # (flottant, erreur)
        hat_exact = {}
        score = [None] * nc

        def s_float(c):
            if score[c] is None:
                t = cond.top[c]
                top_phi = levels[t].phi(zz) if t >= 0 else (0.0, 0.0)
                score[c] = _score_float(cond, levels, c, zz, top_phi)
            return score[c]

        def exact_hat(c):
            """S^ exact d'un cluster deja decide (pile explicite : arbres condenses profonds)."""
            stack = [c]
            while stack:
                x = stack[-1]
                if x in hat_exact:
                    stack.pop()
                    continue
                if chosen[x] or not cond.children[x]:
                    hat_exact[x] = _score_exact(cond, levels, x, zz)
                    stack.pop()
                    continue
                todo = [d for d in cond.children[x] if d not in hat_exact]
                if todo:
                    stack.extend(todo)
                    continue
                hat_exact[x] = RadSum([term for d in cond.children[x] for term in hat_exact[d].terms])
                stack.pop()
            return hat_exact[c]

        for c in range(nc):
            if c in roots and inject != 'racine_admise':
                continue
            s, es = s_float(c)
            if not cond.children[c]:
                chosen[c] = True
                hat[c] = (s, es)
                continue
            sub = [hat[d] for d in cond.children[c]]
            total = math.fsum(v for v, _ in sub)
            etot = sum(e for _, e in sub) + U * abs(total)
            diff = s - total
            bound = (es + etot + U * abs(diff)) * 1.0001 + 1e-300
            stats['decisions'] += 1
            scale = max(abs(s), abs(total), 1e-300)
            stats['margins'].append(abs(diff) / scale)
            if inject == 'flottant_seul':
                sign = (diff > 0) - (diff < 0)
            elif abs(diff) > bound:
                sign = 1 if diff > 0 else -1
            else:
                stats['exact'] += 1
                own = _score_exact(cond, levels, c, zz)
                sub_exact = RadSum([term for d in cond.children[c] for term in exact_hat(d).terms])
                sign = (own - sub_exact).sign(budget_bits)
                if sign == 0:
                    stats['equalities'] += 1
            keep_parent = sign > 0 or (sign == 0 and inject != 'egalite_enfants')
            if keep_parent:
                chosen[c] = True
                hat[c] = (s, es)
            else:
                hat[c] = (total, etot)
    else:
        raise ValueError('methode')
    # Passe descendante : un cluster retenu masque ses descendants.
    selected = [False] * nc
    blocked = [False] * nc
    for c in reversed(range(nc)):
        if cond.parent[c] >= 0 and (blocked[cond.parent[c]] or selected[cond.parent[c]]):
            blocked[c] = True
            continue
        if chosen[c] and admissible[c]:
            selected[c] = True
    margins = stats.pop('margins')
    stats['min_margin'] = min(margins) if margins else None
    stats['margins_below_1e-6'] = sum(1 for m in margins if m < 1e-6)
    return Selection(selected, stats)


def labels(pt, cond, first, sel):
    """Labels par PointId (indice d'entree) : identifiant canonique = plus petit PointId du cluster ; -1 = bruit."""
    nc = len(cond)
    owner = [-1] * nc
    for c in reversed(range(nc)):  # parents apres enfants dans l'ordre de naissance : on descend
        p = cond.parent[c]
        if p >= 0 and owner[p] >= 0:
            owner[c] = owner[p]
        elif sel.selected[c]:
            owner[c] = c
    native = np.full(pt.n, -1, dtype=np.int64)
    has = first >= 0
    native[has] = np.asarray(owner, dtype=np.int64)[first[has]] if nc else -1
    out = np.full(pt.n, -1, dtype=np.int64)
    out[pt.ids] = native
    # identifiants canoniques
    canon = {}
    for pid, c in enumerate(out.tolist()):
        if c >= 0 and c not in canon:
            canon[c] = pid
    return np.array([canon[c] if c >= 0 else -1 for c in out.tolist()], dtype=np.int64)


def flat(pt, mcs, z=1, method='eom', inject=None, budget_bits=8192):
    """Chaine complete : (labels par PointId, statistiques). Refusal propage."""
    cond, first = condense(pt, mcs, inject)
    sel = select(pt, cond, z, method, inject, budget_bits)
    lab = labels(pt, cond, first, sel)
    stats = dict(sel.stats)
    stats.update(selected=int(sum(sel.selected)), noise=int(np.sum(lab < 0)))
    return lab, stats
