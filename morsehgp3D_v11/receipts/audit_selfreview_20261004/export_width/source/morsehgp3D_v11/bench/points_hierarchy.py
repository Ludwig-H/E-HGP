#!/usr/bin/env python3
"""Hierarchie laminaire de points depuis FULL : pendaison fidele a marge (regle H_m) et regles temoins.

Lit l'export MHGP11PH de bench/points_export.cpp et rend, pour un ordre k, une pendaison : chaque site entre une
seule fois, a une date exacte e_i, sur un noeud vivant o_i de la foret FULL_k, puis suit ses ancetres. Les blocs
de points sont donc laminaires et ne se reunissent qu'aux fusions de FULL (docs/HIERARCHIE_POINTS.md).

Regles (toutes sans choix lexicographique) :
  core   : e = D_k(x), noeud de la composante contenant x (P1).
  cover  : e = alpha_k(x)^2 ; plusieurs couvreurs au premier instant -> leur LCA, date max(alpha, naissance) (P4).
  first  : premiere couverture QUALIFIEE (couverture d'au moins m sites), ex aequo -> LCA ; sans marge.
  margin : H_m. t = premiere couverture qualifiee, p1 un point le plus bas de R_i^(m) ;
           e = t + max_q [ m(p1, q) - h(q) ] sur les points q de R_i^(m) (rivaux : couvreurs qualifies avant leur
           reunion avec la lignee de p1) ; proprietaire = ancetre de p1 vivant a e. Stable : |de| <= 3 eps,
           |du| <= 5 eps sous entrelacement eps (preuve dans la doc).

Niveaux : rayons carres exacts (paires d'entiers num/den, aucun flottant dans une decision ; les flottants ne
servent qu'a pre-trier, avec repli exact a l'egalite approchee).

Format MHGP11PH (mots u64 LE apres 8 octets 'MHGP11PH') :
  version, coord_bits, kmax, n, L, nb_ordres, ordres...
  sites : n x (x, y, z, id) ; niveaux : L x (num W mots, den W mots), W = 3 en version 1 (profils u18, u21) et
  W = 4 en version 2 (profil u24 : numerateur de 8B+12 = 204 bits ; audit P2 du 4 octobre 2026)
  par ordre : k, N, naissances, racine ; N x (parent, rang) ; n x (noeud core, D_k entier) ;
              T ; offsets n+1 ; T mots (rang << 32 | noeud), groupes par site, tries.
"""
from fractions import Fraction
import numpy as np

NONE = 0xFFFFFFFF
MAGIC = b'MHGP11PH'


class ExportError(ValueError):
    pass


def need(condition, reason):
    if not condition:
        raise ExportError(reason)


class Levels(object):
    """Niveaux exacts du catalogue : paires (num, den) d'entiers a la demande, flottants pour pre-trier."""

    def __init__(self, limbs):
        need(limbs.ndim == 2 and limbs.shape[1] in (6, 8), 'mots_de_niveau')
        self.limbs = limbs
        self.words = limbs.shape[1] // 2
        w = limbs.astype(np.float64)
        num = sum(w[:, j] * 2.0 ** (64 * j) for j in range(self.words))
        den = sum(w[:, self.words + j] * 2.0 ** (64 * j) for j in range(self.words))
        need(bool(np.all(den > 0)), 'denominateur_nul')
        self.approx = num / den
        self.cache = {}

    @classmethod
    def from_fractions(cls, values):
        """Niveaux donnes par des Fraction (oracle de reference) : memes mots que l'export natif (trois, ou quatre
        si une valeur depasse 192 bits)."""
        words = 3 if all(v.numerator < 1 << 192 and v.denominator < 1 << 192 for v in values) else 4
        limbs = np.zeros((len(values), 2 * words), dtype=np.uint64)
        mask = (1 << 64) - 1
        for r, value in enumerate(values):
            need(value.numerator >= 0 and value.numerator < 1 << (64 * words) and
                 value.denominator < 1 << (64 * words), 'niveau')
            for j in range(words):
                limbs[r, j] = (value.numerator >> (64 * j)) & mask
                limbs[r, words + j] = (value.denominator >> (64 * j)) & mask
        return cls(limbs)

    def __len__(self):
        return len(self.approx)

    def exact(self, rank):
        rank = int(rank)
        got = self.cache.get(rank)
        if got is None:
            w = [int(x) for x in self.limbs[rank]]
            got = (sum(w[j] << (64 * j) for j in range(self.words)),
                   sum(w[self.words + j] << (64 * j) for j in range(self.words)))
            self.cache[rank] = got
        return got


def read_export(path):
    with open(path, 'rb') as stream:
        need(stream.read(8) == MAGIC, 'magie')
    raw = np.fromfile(path, dtype='<u8', offset=8)
    pos = [0]

    def take(count):
        need(pos[0] + count <= len(raw), 'export_tronque')
        out = raw[pos[0]:pos[0] + count]
        pos[0] += count
        return out

    version, bits, kmax, n, nlevels, norders = (int(x) for x in take(6))
    need(version in (1, 2), 'version')
    words = 3 if version == 1 else 4
    orders = [int(x) for x in take(norders)]
    sites = take(4 * n).reshape(n, 4).astype(np.int64)
    levels = Levels(take(2 * words * nlevels).reshape(nlevels, 2 * words))
    result = dict(bits=bits, kmax=kmax, n=n, xyz=sites[:, :3], ids=sites[:, 3], levels=levels, orders={})
    for expected in orders:
        k, count, births, root = (int(x) for x in take(4))
        need(k == expected, 'ordre')
        nodes = take(2 * count).reshape(count, 2).astype(np.int64)
        parent = nodes[:, 0].copy()
        parent[parent == NONE] = -1
        core = take(2 * n).reshape(n, 2)
        total = int(take(1)[0])
        offsets = take(n + 1).astype(np.int64)
        packed = take(total)
        need(offsets[0] == 0 and offsets[-1] == total and bool(np.all(np.diff(offsets) > 0)), 'offsets')
        result['orders'][k] = Order(k, parent, nodes[:, 1].copy(), births, root, core[:, 0].astype(np.int64),
                                    core[:, 1].astype(np.int64), offsets, (packed >> 32).astype(np.int64),
                                    (packed & 0xFFFFFFFF).astype(np.int64), levels)
    need(pos[0] == len(raw), 'octets_en_trop')
    return result


class Order(object):
    """Foret FULL_k, entrees core et incidences fortes par site."""

    def __init__(self, k, parent, rank, births, root, core_node, core_d, offsets, inc_rank, inc_node, levels):
        self.k, self.parent, self.rank, self.births, self.root = k, parent, rank, births, root
        self.core_node, self.core_d, self.offsets = core_node, core_d, offsets
        self.inc_rank, self.inc_node, self.levels = inc_rank, inc_node, levels
        self.n = len(offsets) - 1
        self.size = len(parent)
        self.inc_site = np.repeat(np.arange(self.n, dtype=np.int64), np.diff(offsets))
        need(int(np.sum(parent < 0)) == 1 and parent[root] < 0, 'racine')
        has = parent >= 0
        need(bool(np.all(rank[parent[has]] > rank[has])), 'parent_pas_plus_haut')
        self._lift = None

    # --- structure ---
    def lifting(self):
        """Profondeurs et table des sauts binaires par saut de pointeurs (racine fixe)."""
        if self._lift is None:
            up = np.where(self.parent >= 0, self.parent, np.arange(self.size))
            depth = (self.parent >= 0).astype(np.int64)
            table = [up]
            while True:
                nxt = up[up]
                if np.array_equal(nxt, up):
                    break
                depth = depth + depth[up]
                up = nxt
                table.append(up)
            self._lift = (depth, table)
        return self._lift

    def merge_list(self):
        """Fusions triees par rang et leurs enfants, partagees par toutes les regles d'un ordre."""
        if getattr(self, '_merges', None) is None:
            has = np.flatnonzero(self.parent >= 0)
            perm = np.argsort(self.parent[has], kind='stable')
            child, par = has[perm], self.parent[has][perm]
            cut = np.flatnonzero(np.r_[True, par[1:] != par[:-1]])
            heads = par[cut].tolist()
            groups = np.split(child, cut[1:])
            kids = {h: g.tolist() for h, g in zip(heads, groups)}
            merges = sorted(heads, key=lambda v: int(self.rank[v]))
            self._merges = (kids, merges)
        return self._merges

    def lca(self, a, b):
        """LCA vectorise par sauts binaires."""
        depth, table = self.lifting()
        a, b = a.copy(), b.copy()
        swap = depth[a] < depth[b]
        a[swap], b[swap] = b[swap], a[swap]
        diff = depth[a] - depth[b]
        for j, up in enumerate(table):
            sel = (diff >> j) & 1 == 1
            a[sel] = up[a[sel]]
        for up in reversed(table):
            sel = up[a] != up[b]
            a[sel] = up[a[sel]]
            b[sel] = up[b[sel]]
        out = np.where(a == b, a, self.parent[a])
        return out

    def ancestor_at(self, start, target):
        """Ancetre le plus haut de start dont le niveau est <= target (paire exacte), coupe fermee."""
        depth, table = self.lifting()
        lev = self.levels
        x = int(start)
        for up in reversed(table):
            y = int(up[x])
            if y != x and le_level(lev, int(self.rank[y]), target):
                x = y
        while self.parent[x] >= 0 and le_level(lev, int(self.rank[self.parent[x]]), target):
            x = int(self.parent[x])
        return x


def cmp(a, b):
    left, right = a[0] * b[1], b[0] * a[1]
    return (left > right) - (left < right)


def le_level(levels, rank, value):
    approx = levels.approx[rank]
    target = value[0] / value[1]
    if approx < target * (1 - 1e-12):
        return True
    if approx > target * (1 + 1e-12) + 1e-300:
        return False
    return cmp(levels.exact(rank), value) <= 0


def qualify(order, m):
    """Rang de qualification de chaque noeud pendant sa vie (couverture >= m sites distincts), -1 sinon.

    m <= k : tout noeud couvre au moins k sites des sa naissance. m = k + 1 : toute fusion est qualifiee a sa
    naissance (deux composantes distinctes ne couvrent jamais les memes k sites : elles partageraient le sommet),
    une naissance l'est au (k + 1)-ieme site distinct de ses propres incidences. Au-dela : recurrence generale."""
    if m <= order.k:
        return order.rank.copy()
    if m == order.k + 1:
        return qualify_next(order, m)
    return qualify_general(order, m)


def qualify_next(order, m):
    n, size = order.n, order.size
    key = order.inc_node * n + order.inc_site
    perm = np.lexsort((order.inc_rank, key))
    ks, rs = key[perm], order.inc_rank[perm]
    first = np.r_[True, ks[1:] != ks[:-1]]
    pair_node, pair_rank = ks[first] // n, rs[first]
    perm = np.lexsort((pair_rank, pair_node))
    pair_node, pair_rank = pair_node[perm], pair_rank[perm]
    counts = np.bincount(pair_node, minlength=size)
    starts = np.searchsorted(pair_node, np.arange(size))
    qual = np.full(size, -1, dtype=np.int64)
    has = counts >= m
    qual[has] = pair_rank[starts[has] + m - 1]
    merge = np.zeros(size, dtype=bool)
    merge[order.parent[order.parent >= 0]] = True
    qual[merge] = order.rank[merge]
    return qual


def qualify_general(order, m):
    size = order.size
    children = [[] for _ in range(size)]
    for v, p in enumerate(order.parent.tolist()):
        if p >= 0:
            children[p].append(v)
    by_node = np.lexsort((order.inc_rank, order.inc_node))
    nodes_sorted = order.inc_node[by_node]
    starts = np.searchsorted(nodes_sorted, np.arange(size + 1))
    inc_rank = order.inc_rank[by_node].tolist()
    inc_site = order.inc_site[by_node].tolist()
    qual = [-1] * size
    done = [False] * size
    pending = {}
    rank = order.rank.tolist()
    for v in np.argsort(order.rank, kind='stable').tolist():
        kids = children[v]
        if any(done[c] for c in kids):
            for c in kids:
                pending.pop(c, None)
            qual[v], done[v] = rank[v], True
            continue
        cover = set()
        for c in kids:
            cover |= pending.pop(c, set())
        if len(cover) >= m:
            qual[v], done[v] = rank[v], True
            continue
        for j in range(int(starts[v]), int(starts[v + 1])):
            cover.add(inc_site[j])
            if len(cover) >= m:
                qual[v], done[v] = max(rank[v], inc_rank[j]), True
                break
        if not done[v]:
            pending[v] = cover
    return np.array(qual, dtype=np.int64)


def qualified_starts(order, qual):
    """Point de depart qualifie de chaque incidence : (noeud, rang). Premier ancetre qualifie par saut de pointeurs."""
    size = order.size
    ok = qual >= 0
    ptr = np.where(ok | (order.parent < 0), np.arange(size), order.parent)
    while True:
        nxt = np.where(ok[ptr], ptr, ptr[ptr])
        if np.array_equal(nxt, ptr):
            break
        ptr = nxt
    v, r = order.inc_node, order.inc_rank
    own = ok[v]
    up = ptr[np.where(order.parent[v] >= 0, order.parent[v], v)]
    node = np.where(own, v, up)
    need(bool(np.all(ok[node])), 'jamais_qualifie')
    rank = np.where(own, np.maximum(r, qual[v]), qual[node])
    return node, rank


def first_points(order, node, rank):
    """Par site : rang minimal t et un noeud vivant a t (premiere incidence de rang t)."""
    t = np.minimum.reduceat(rank, order.offsets[:-1])
    hit = np.flatnonzero(rank == t[order.inc_site])
    sites = order.inc_site[hit]
    first = hit[np.r_[True, sites[1:] != sites[:-1]]]
    need(len(first) == order.n, 'premier_point')
    return t, node[first]


class Hanging(object):
    """Pendaison : par site, date exacte (num, den), proprietaire, rang plancher et date strictement entre rangs."""

    def __init__(self, order, num, den, owner, floor, strict, rule, extra=None):
        self.order, self.num, self.den, self.owner = order, num, den, owner
        self.floor, self.strict, self.rule = floor, strict, rule
        self.extra = extra or {}

    def entry(self, i):
        return (self.num[i], self.den[i])

    # Interface commune avec les dates en rayon (bench/points_radius.py) : valeur, ordre exact, coupe fermee.
    def value(self, i):
        return Fraction(self.num[i], self.den[i])

    def cmp_entries(self, i, j):
        a, b = self.value(i), self.value(j)
        return (a > b) - (a < b)

    def le(self, i, level):
        """Vrai si le site i est entre a la coupe fermee level (Fraction, niveau carre)."""
        return self.value(i) <= level


def floor_rank(levels, value):
    """Plus grand rang r avec niveau(r) <= value, et vrai si l'inegalite est stricte (decision exacte)."""
    target = value[0] / value[1]
    r = int(np.searchsorted(levels.approx, target, side='right')) - 1
    r = max(0, min(r, len(levels) - 1))
    while r + 1 < len(levels) and cmp(levels.exact(r + 1), value) <= 0:
        r += 1
    while r > 0 and cmp(levels.exact(r), value) > 0:
        r -= 1
    return r, cmp(levels.exact(r), value) < 0


def lca_reduce(order, sites, nodes, count):
    """LCA par site de tous les noeuds listes (sites tries) ; rend un tableau de taille count."""
    out = np.full(count, -1, dtype=np.int64)
    first = np.r_[True, sites[1:] != sites[:-1]]
    out[sites[first]] = nodes[first]
    occurrence = np.arange(len(sites)) - np.maximum.accumulate(np.where(first, np.arange(len(sites)), 0))
    for k in range(1, int(occurrence.max()) + 1 if len(sites) else 1):
        sel = occurrence == k
        s = sites[sel]
        out[s] = order.lca(out[s], nodes[sel])
    return out


def finish(order, num, den, owner, rule, extra=None):
    """Rangs planchers exacts et coherence : le proprietaire est vivant a la date d'entree."""
    levels = order.levels
    floor = np.zeros(order.n, dtype=np.int64)
    strict = np.zeros(order.n, dtype=bool)
    for i in range(order.n):
        r, s = floor_rank(levels, (num[i], den[i]))
        floor[i], strict[i] = r, s
    o = owner
    alive = (order.rank[o] <= floor) & ((order.parent[o] < 0) | (order.rank[np.maximum(order.parent[o], 0)] > floor))
    need(bool(np.all(alive)), 'proprietaire_non_vivant:' + rule)
    return Hanging(order, num, den, owner, floor, strict, rule, extra)


def hang_core(order):
    num = [int(d) for d in order.core_d.tolist()]
    return finish(order, num, [1] * order.n, order.core_node.copy(), 'core')


def hang_first(order, m, rule):
    """Premiere couverture (qualifiee si m > k) ; ex aequo au premier instant -> LCA, date max(t, naissance)."""
    qual = qualify(order, m)
    node, rank = qualified_starts(order, qual)
    t = np.minimum.reduceat(rank, order.offsets[:-1])
    tie = np.flatnonzero(rank == t[order.inc_site])
    owner = lca_reduce(order, order.inc_site[tie], node[tie], order.n)
    when = np.maximum(t, order.rank[owner])
    levels = order.levels
    pairs = [levels.exact(r) for r in when.tolist()]
    extra = dict(ties=int(np.sum(order.rank[owner] > t)))
    return finish(order, [p[0] for p in pairs], [p[1] for p in pairs], owner, rule, extra)


def hang_margin(order, m, rule='margin'):
    """Regle H_m : e = t + max_q [m(p1, q) - h(q)] sur les departs qualifies q des incidences du site."""
    qual = qualify(order, m)
    node, rank = qualified_starts(order, qual)
    t, p1 = first_points(order, node, rank)
    a = p1[order.inc_site]
    w = order.lca(a, node)
    rival = w != node  # le depart q n'est pas sur la remontee de p1
    levels = order.levels
    meet = order.rank[w]
    approx = np.where(rival, levels.approx[meet] - levels.approx[rank], 0.0)
    best = np.maximum.reduceat(approx, order.offsets[:-1])
    slack = 1e-9 * (levels.approx[meet] + levels.approx[rank]) + 1e-300
    candidate = np.flatnonzero(rival & (approx + slack >= best[order.inc_site] - 1e-9 * np.abs(best[order.inc_site])))
    exact = {}
    for j in candidate.tolist():
        s = int(order.inc_site[j])
        hn, hd = levels.exact(int(meet[j]))
        qn, qd = levels.exact(int(rank[j]))
        term = (hn * qd - qn * hd, hd * qd)
        if s not in exact or cmp(term, exact[s]) > 0:
            exact[s] = term
    num, den, owner = [], [], p1.copy()
    delayed = 0
    for i in range(order.n):
        tn, td = levels.exact(int(t[i]))
        d = exact.get(i)
        if d is None or d[0] <= 0:
            num.append(tn)
            den.append(td)
            continue
        delayed += 1
        value = (tn * d[1] + d[0] * td, td * d[1])
        num.append(value[0])
        den.append(value[1])
        owner[i] = order.ancestor_at(int(p1[i]), value)
    return finish(order, num, den, owner, rule, dict(delayed=delayed))


def same_lineage(order, a, b):
    return order.lca(np.array([a]), np.array([b]))[0] in (a, b)


def ultrametric(hanging):
    """Hauteurs de reunion exactes u(i, j) (Fraction), u(i, i) = date d'entree ; petits nuages seulement."""
    order, n = hanging.order, hanging.order.n
    need(n <= 64, 'ultrametrique_petits_nuages')
    e = [Fraction(hanging.num[i], hanging.den[i]) for i in range(n)]
    u = [[None] * n for _ in range(n)]
    for i in range(n):
        u[i][i] = e[i]
        for j in range(i + 1, n):
            a, b = int(hanging.owner[i]), int(hanging.owner[j])
            w = int(order.lca(np.array([a]), np.array([b]))[0])
            value = max(e[i], e[j])
            if w not in (a, b):
                ln, ld = order.levels.exact(int(order.rank[w]))
                value = max(value, Fraction(ln, ld))
            u[i][j] = u[j][i] = value
    return u


THING = (10, 11, 13, 15, 16, 18, 20, 30, 31, 32, 252, 253, 254, 255, 256, 257, 258, 259)
VOID = (0, 1, 52, 99)


def lidar_objects(raw, minimum):
    """Instances 'thing' (sem dans THING, inst > 0) d'au moins minimum points, comme Zoltan/demos/tools."""
    raw = np.asarray(raw, dtype=np.int64)
    sem, inst = raw & 0xFFFF, raw >> 16
    label = np.where(np.isin(sem, THING) & (inst > 0), raw, -1)
    keys, counts = np.unique(label[label >= 0], return_counts=True)
    keys = keys[counts >= minimum]
    index = {int(key): j for j, key in enumerate(keys.tolist())}
    obj = np.array([index.get(int(x), -1) for x in label.tolist()], dtype=np.int64)
    return obj, np.isin(sem, VOID), [int(key) for key in keys.tolist()]


class Evaluator(object):
    """Meilleur IoU de chaque objet parmi les blocs d'une hierarchie, plateaux fermes seulement.

    IoU au sens panoptique : les sites void sont retires du bloc (ils restent dans l'arbre). Un bloc n'est lu
    qu'apres toutes les operations de son niveau exact ; seuls les objets dont le compte a pu croitre sont relus."""

    def __init__(self, ids, obj, void, objects):
        self.dsu = list(range(ids))
        self.size = [0] * ids
        self.counts = [None] * ids
        self.obj, self.void = obj.tolist(), void.tolist()
        self.total = [0] * objects
        for o, v in zip(self.obj, self.void):
            if o >= 0 and not v:
                self.total[o] += 1
        self.best = [0.0] * objects
        self.best_size = [0] * objects
        self.best_ref = [None] * objects  # (noeud de l'arbre, niveau du plateau) du meilleur bloc
        self.node_of = list(range(ids))  # noeud courant de chaque racine
        self.level = None  # niveau du plateau en cours, pose par l'appelant avant close()
        self.dirty = set()
        self.touched = set()
        self.blocks = 0  # blocs distincts publies (un par racine modifiee et par plateau)

    def find(self, x):
        dsu = self.dsu
        root = x
        while dsu[root] != root:
            root = dsu[root]
        while dsu[x] != root:
            dsu[x], x = root, dsu[x]
        return root

    def enter(self, site, target):
        r = self.find(target)
        if not self.void[site]:
            self.size[r] += 1
        o = self.obj[site]
        if o >= 0 and not self.void[site]:
            c = self.counts[r]
            if c is None:
                c = self.counts[r] = {}
            c[o] = c.get(o, 0) + 1
            self.dirty.add((r, o))
        self.touched.add(r)

    def merge(self, parts, target):
        roots = []
        for p in parts:
            r = self.find(p)
            if r not in roots:
                roots.append(r)
        t = self.find(target)
        if t not in roots:
            roots.append(t)
        nonempty = [r for r in roots if self.size[r] or self.counts[r]]
        base = max(roots, key=lambda r: len(self.counts[r] or ()))
        for r in roots:
            if r == base:
                continue
            self.dsu[r] = base
            self.size[base] += self.size[r]
            c = self.counts[r]
            if c:
                b = self.counts[base]
                if b is None:
                    b = self.counts[base] = {}
                for o, value in c.items():
                    b[o] = b.get(o, 0) + value
            self.counts[r] = None
        self.node_of[base] = target
        if len(nonempty) >= 2:
            self.touched.add(base)
            for o in (self.counts[base] or {}):
                self.dirty.add((base, o))

    def close(self):
        self.blocks += len(set(self.find(r) for r in self.touched))
        self.touched.clear()
        for r, o in self.dirty:
            r = self.find(r)
            c = self.counts[r].get(o, 0) if self.counts[r] else 0
            if c == 0:
                continue
            iou = c / (self.size[r] + self.total[o] - c)
            if iou > self.best[o]:
                self.best[o], self.best_size[o] = iou, self.size[r]
                self.best_ref[o] = (self.node_of[r], self.level)
        self.dirty.clear()


def evaluate_hanging(hanging, obj, void, objects):
    """Plateaux : rang r (fusions de rang r et entrees de date exacte niveau(r)), puis entrees strictement entre
    r et r + 1 groupees par date exacte."""
    order = hanging.order
    ev = Evaluator(order.size, obj, void, objects)
    kids, merges = order.merge_list()
    entries = sorted(range(order.n), key=lambda i: (int(hanging.floor[i]), bool(hanging.strict[i])))
    entries = sort_strict_groups(hanging, entries)
    mi = ei = 0
    rank = order.rank.tolist()
    while mi < len(merges) or ei < len(entries):
        r = min(rank[merges[mi]] if mi < len(merges) else 1 << 62,
                int(hanging.floor[entries[ei]]) if ei < len(entries) else 1 << 62)
        while mi < len(merges) and rank[merges[mi]] == r:
            v = merges[mi]
            ev.merge(kids[v], v)
            mi += 1
        while ei < len(entries) and int(hanging.floor[entries[ei]]) == r and not hanging.strict[entries[ei]]:
            i = entries[ei]
            ev.enter(i, int(hanging.owner[i]))
            ei += 1
        ev.level = Fraction(*order.levels.exact(r))
        ev.close()
        while ei < len(entries) and int(hanging.floor[entries[ei]]) == r:
            i = entries[ei]
            while ei < len(entries) and int(hanging.floor[entries[ei]]) == r and \
                    hanging.cmp_entries(entries[ei], i) == 0:
                j = entries[ei]
                ev.enter(j, int(hanging.owner[j]))
                ei += 1
            ev.level = hanging.value(i)
            ev.close()
    return ev


def sort_strict_groups(hanging, entries):
    """Trie, a rang plancher egal, les dates strictement entre deux niveaux par l'ordre exact de la pendaison."""
    from functools import cmp_to_key
    out, j = [], 0
    while j < len(entries):
        i = entries[j]
        key = (int(hanging.floor[i]), bool(hanging.strict[i]))
        k = j
        while k < len(entries) and (int(hanging.floor[entries[k]]), bool(hanging.strict[entries[k]])) == key:
            k += 1
        group = entries[j:k]
        if key[1] and len(group) > 1:
            group = sorted(group, key=cmp_to_key(hanging.cmp_entries))
        out.extend(group)
        j = k
    return out


def hdbscan_tree(xyz, k):
    """Arbre du lien simple de la distance d'atteignabilite mutuelle de sklearn (min_samples = k, soi compris)."""
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_samples=k, min_cluster_size=2, metric='euclidean', algorithm='kd_tree', n_jobs=1, copy=True)
    model.fit(np.asarray(xyz, dtype=np.float64))
    return np.asarray(model._single_linkage_tree_)


def evaluate_linkage(tree, n, obj, void, objects):
    """Meme evaluateur sur un arbre du lien simple (n feuilles presentes au niveau zero, fusions ex aequo d'un bloc)."""
    ev = Evaluator(2 * n - 1, obj, void, objects)
    for i in range(n):
        ev.enter(i, i)
    ev.level = 0.0
    ev.close()
    left = tree['left_node'] if tree.dtype.names else tree[:, 0]
    right = tree['right_node'] if tree.dtype.names else tree[:, 1]
    value = tree['value'] if tree.dtype.names else tree[:, 2]
    left, right, value = left.astype(np.int64).tolist(), right.astype(np.int64).tolist(), value.tolist()
    j = 0
    while j < len(value):
        level = value[j]
        while j < len(value) and value[j] == level:
            ev.merge([left[j], right[j]], n + j)
            j += 1
        ev.level = level
        ev.close()
    ev.tree = (left, right)
    return ev


def hanging_members(hanging, node, level):
    """Sites (ordre natif) du bloc du noeud a la coupe fermee level : entres a level et pendus sous le noeud."""
    order = hanging.order
    entered = np.array([hanging.le(i, level) for i in range(order.n)], dtype=bool)
    owners = hanging.owner[entered]
    inside = order.lca(owners, np.full(len(owners), node, dtype=np.int64)) == node
    return np.flatnonzero(entered)[inside]


def linkage_members(ev, node, n):
    """Feuilles du noeud d'un arbre du lien simple."""
    left, right = ev.tree
    stack, out = [node], []
    while stack:
        x = stack.pop()
        if x < n:
            out.append(x)
        else:
            stack.extend((left[x - n], right[x - n]))
    return np.array(sorted(out), dtype=np.int64)
