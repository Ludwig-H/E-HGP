"""Porte T2 de la tour : forets C++, attaches C n X et cartes verticales == oracle exhaustif Gamma_k
(reference/hgp10_ref.py).

Pour chaque nuage (generiques, grilles cospheriques, coplanaires), chaque ordre k <= K et chaque niveau critique a
de Gamma_k, coupes fermee (<= a) et ouverte (< a) :
  - nombre de composantes de la foret == nombre de composantes de Gamma_k(a) ;
  - partition C n X des points entres == partition de l'oracle (composante de Gamma_k(a) du sommet kNN(x)).

Temoins (audit continu du 29 sept. 2026, pool_head/AUDIT_POOL_TETE_VERTICAL § 3). L'ancien juge des verticales ne
controlait une image que si un point de donnees etait deja entre dans la composante : une image fausse avant
l'entree des points survivait. Le dump est desormais lu avec --dump-births : chaque naissance porte une K-partie W.
  - W doit avoir pour miniboule exacte le niveau de la naissance (beta(W) == niveau, Fraction) ;
  - a chaque niveau d'evenement a (Gamma_k, noeuds, entrees, noeuds de l'ordre k + 1), coupe fermee, r -> composante
    de Gamma_k(a) contenant W(feuille de r) est une BIJECTION des noeuds vivants sur les composantes, coherente
    (les enfants d'une fusion sont dans une meme composante) ; les attaches C n X sont jugees par identite ;
  - les verticales sont jugees a CHAQUE naissance et fusion u de l'ordre k + 1, meme si C n X est vide : l'image de u
    au niveau a de u est la composante de Gamma_k(a) d'une k-partie de W(feuille de u) ; lower[u] doit etre le noeud
    vivant de l'ordre k a la coupe fermee a (contrat de src/tower/tower.hpp) et avoir cette identite.
Mutants (--inject=NOM) : un mutant grave est applique au dump de sa fixture ; le juge doit accepter la fixture, puis
rejeter le mutant (code 4). Sans --inject, la porte rejoue aussi tous les mutants verticaux systematiques de ses
fixtures (toute autre image vivante, un descendant, un ancetre ne apres) : tous doivent etre tues.
Usage : python3 test_tower_oracle.py BUILD_DIR [nuages] [--inject=NOM]
Codes : 0 conforme, 1 desaccord, 2 mutant inconnu, 3 plancher (couverture, mutants non tues), 4 mutant tue
(--inject seulement ; un mutant qui survit rend 0).
Python nu (aucune dependance), aucun assert.
"""
import copy
import os
import random
import subprocess
import sys
import tempfile
from collections import defaultdict
from fractions import Fraction
from itertools import combinations

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
sys.path.insert(0, HERE)
import hgp10_ref as R  # noqa: E402
from test_catalogue_oracle import clouds  # noqa: E402


def parse(path):
    """Dump de mhgp10_tower. orders[k] : nodes = [(parent, niveau, image verticale)], points = [(xyz, noeud, entree)],
    births = {noeud: K-partie temoin (coordonnees)} (--dump-births), ids = identifiants lus."""
    orders = {}
    cur = None
    for line in open(path):
        t = line.split()
        if t[0] == 'order':
            cur = dict(nodes=[], points=[], births={}, ids=[])
            orders[int(t[1])] = cur
        elif t[0] == 'node':
            cur['ids'].append(int(t[1]))
            cur['nodes'].append((int(t[2]), Fraction(int(t[3]), int(t[4])), int(t[5]) if len(t) > 5 else -1))
            if len(t) > 6:
                cur['births'][len(cur['nodes']) - 1] = tuple(tuple(int(c) for c in s.split(',')) for s in t[6:])
        else:
            cur['points'].append(((int(t[1]), int(t[2]), int(t[3])), int(t[4]), Fraction(int(t[5]))))
    return orders


# ---------------------------------------------------------------- oracle exact (cache par nuage)

class Balls:
    """Miniboules exactes des parties de P : meme semantique que R.meb (plus petite boule candidate, support de 1 a
    4 points affinement independants de barycentriques >= 0, contenant la partie), les candidats (centre, rayon carre,
    distances carrees a tous les points) etant calcules une fois par support au lieu d'une fois par partie."""

    def __init__(self, P):
        self.P = P
        self.cand = {}
        self.memo = {}

    def support(self, S):
        if S in self.cand:
            return self.cand[S]
        pts = [self.P[i] for i in S]
        val = None
        if R.affinely_independent(pts):
            cc = R.circumcenter(pts)
            if cc is not None and all(lam >= 0 for lam in cc[1]):
                val = (R.d2(cc[0], pts[0]), [R.d2(cc[0], p) for p in self.P])
        self.cand[S] = val
        return val

    def beta(self, F):
        b = self.memo.get(F)
        if b is None:
            for q in range(1, min(4, len(F)) + 1):
                for S in combinations(F, q):
                    v = self.support(S)
                    if v is not None and (b is None or v[0] < b) and all(v[1][i] <= v[0] for i in F):
                        b = v[0]
            self.memo[F] = b
        return b


class Gamma:
    """Gamma_k : sommets (k-parties, beta) et aretes ((k+1)-parties, beta), groupes par niveau exact."""

    def __init__(self, balls, n, k):
        self.vbeta = {F: balls.beta(F) for F in combinations(range(n), k)}
        self.ebeta = {G: balls.beta(G) for G in combinations(range(n), k + 1)} if k < n else {}
        self.v_at, self.e_at = defaultdict(list), defaultdict(list)
        for F, b in self.vbeta.items():
            self.v_at[b].append(F)
        for G, b in self.ebeta.items():
            self.e_at[b].append(G)
        self.levels = sorted(set(self.v_at) | set(self.e_at))


class Ctx:
    """Cache exact d'un nuage (partage entre les K d'un meme nuage et entre les mutants d'une fixture)."""

    def __init__(self, P):
        self.P = list(P)
        self.n = len(P)
        self.idx = {p: i for i, p in enumerate(P)}
        self.balls = Balls(self.P)
        self.gammas = {}
        self.knns = {}
        self.entries = {}

    def gamma(self, k):
        if k not in self.gammas:
            self.gammas[k] = Gamma(self.balls, self.n, k)
        return self.gammas[k]

    def knn(self, x, k):
        if (x, k) not in self.knns:
            self.knns[(x, k)] = R.knn_vertex(self.P, x, k)
        return self.knns[(x, k)]

    def entry(self, x, k):
        if (x, k) not in self.entries:
            self.entries[(x, k)] = R.entry_level(self.P, x, k)
        return self.entries[(x, k)]


def gamma_sweep(P, k, ctx=None):
    """Oracle Gamma_k en un balayage : par niveau critique a, (nb composantes, partition C n X) ouverte et fermee."""
    ctx = ctx if ctx is not None else Ctx(P)
    n = len(P)
    gam = ctx.gamma(k)
    beta, cof = gam.vbeta, gam.ebeta
    levels = gam.levels
    knn = [ctx.knn(x, k) for x in range(n)]
    entry = [ctx.entry(x, k) for x in range(n)]
    parent = {}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    vs = sorted(beta.items(), key=lambda t: t[1])
    es = sorted(cof.items(), key=lambda t: t[1])
    iv = ie = 0
    out = []

    def snap(a, closed):
        roots = {find(F) for F in parent}
        groups = {}
        for x in range(n):
            if entry[x] < a or (closed and entry[x] == a):
                groups.setdefault(find(knn[x]), set()).add(x)
        return len(roots), sorted((frozenset(g) for g in groups.values()), key=lambda t: sorted(t))
    for a in levels:
        op = snap(a, False)
        while iv < len(vs) and vs[iv][1] == a:
            parent[vs[iv][0]] = vs[iv][0]
            iv += 1
        while ie < len(es) and es[ie][1] == a:
            G = es[ie][0]
            fs = [tuple(y for y in G if y != u) for u in G]
            for f in fs[1:]:
                x, y = find(fs[0]), find(f)
                if x != y:
                    parent[max(x, y)] = min(x, y)
            ie += 1
        out.append((a, op, snap(a, True)))
    return out


def top(nodes, v, a, closed):
    ok = (lambda l: l <= a) if closed else (lambda l: l < a)
    while nodes[v][0] >= 0 and ok(nodes[nodes[v][0]][1]):
        v = nodes[v][0]
    return v


def vertical_check_points(orders, k, P, levels):
    """Ancien controle (conserve) : a chaque niveau de la liste (coupe fermee), l'image d'une composante vivante
    d'ordre k contient ses points C n X (un point entre a l'ordre k l'est a l'ordre k-1). Aveugle aux composantes
    sans point entre : voir vertical_check."""
    up, down = orders[k], orders[k - 1]
    idx = {p: i for i, p in enumerate(P)}
    nodes_u, nodes_d = up['nodes'], down['nodes']
    checked = 0
    for a in levels:
        ok = lambda l: l <= a  # noqa: E731
        comp_d = {}
        for pt, v, e in down['points']:
            if ok(e):
                comp_d[idx[pt]] = top(nodes_d, v, a, True)
        for pt, v, e in up['points']:
            if not ok(e):
                continue
            ru = top(nodes_u, v, a, True)
            low = nodes_u[ru][2]
            if low < 0 or low >= len(nodes_d):
                return 'verticale absente k=%d' % k, checked
            image = top(nodes_d, low, a, True)
            if comp_d.get(idx[pt]) != image:
                return 'verticale k=%d a=%s : point hors de l\'image' % (k, a), checked
            checked += 1
    return None, checked


# ---------------------------------------------------------------- juge par temoins

def structure(o, k, ctx):
    """Arbre d'un ordre (une racine, sans cycle, parents pas plus bas, fusions d'au moins deux enfants) et temoins
    des naissances (feuilles) : K-partie de P dont la miniboule exacte est le niveau de la naissance."""
    nodes = o['nodes']
    nn = len(nodes)
    if nn == 0:
        return 'k=%d : ordre vide' % k, None
    if o['ids'] != list(range(nn)):
        return 'k=%d : identifiants de noeuds non consecutifs' % k, None
    children = [[] for _ in range(nn)]
    roots = []
    for v, (par, lv, _low) in enumerate(nodes):
        if par == -1:
            roots.append(v)
            continue
        if par < 0 or par >= nn or par == v:
            return 'k=%d noeud %d : parent %d hors bornes' % (k, v, par), None
        if nodes[par][1] < lv:
            return 'k=%d noeud %d : parent %d plus bas' % (k, v, par), None
        children[par].append(v)
    if len(roots) != 1:
        return 'k=%d : %d racines' % (k, len(roots)), None
    order = [roots[0]]
    i = 0
    while i < len(order):
        order.extend(children[order[i]])
        i += 1
    if len(order) != nn or len(set(order)) != nn:
        return 'k=%d : noeuds hors de l\'arbre' % k, None
    wit = {}
    for v in range(nn):
        if children[v]:
            if v in o['births']:
                return 'k=%d noeud %d : temoin sur une fusion' % (k, v), None
            if len(children[v]) < 2:
                return 'k=%d noeud %d : fusion unaire' % (k, v), None
            continue
        W = o['births'].get(v)
        if W is None:
            return 'k=%d noeud %d : naissance sans temoin (dump sans --dump-births ?)' % (k, v), None
        if len(W) != k or len(set(W)) != k or any(c not in ctx.idx for c in W):
            return 'k=%d noeud %d : temoin mal forme %s' % (k, v, W), None
        F = tuple(sorted(ctx.idx[c] for c in W))
        b = ctx.balls.beta(F)
        if b != nodes[v][1]:
            return 'k=%d noeud %d : temoin de miniboule %s, niveau de naissance %s' % (k, v, b, nodes[v][1]), None
        wit[v] = F
    leaf_of = [0] * nn
    for v in reversed(order):
        leaf_of[v] = v if not children[v] else leaf_of[children[v][0]]
    return None, dict(children=children, wit=wit, leaf_of=leaf_of, empty={})


def sweep(k, orders, info, ctx, cnt, vq):
    """Balayage de l'ordre k (coupes fermees a chaque niveau d'evenement) : bijection des noeuds vivants sur les
    composantes de Gamma_k par les temoins, attaches C n X par identite, verticales de l'ordre k + 1."""
    o, inf = orders[k], info[k]
    nodes = o['nodes']
    nn = len(nodes)
    children, wit, leaf_of, empty = inf['children'], inf['wit'], inf['leaf_of'], inf['empty']
    up, upinf = orders.get(k + 1), info.get(k + 1)
    gam = ctx.gamma(k)
    nodes_at, up_at, pts_at = defaultdict(list), defaultdict(list), defaultdict(list)
    for v, (_par, lv, _low) in enumerate(nodes):
        nodes_at[lv].append(v)
    if up is not None:
        for u, (_par, lv, _low) in enumerate(up['nodes']):
            up_at[lv].append(u)
    seen = set()
    for pt, v, e in o['points']:
        x = ctx.idx.get(pt)
        if x is None or x in seen:
            return 'k=%d : attache d\'un point inconnu ou double %s' % (k, pt)
        seen.add(x)
        if e != ctx.entry(x, k):
            return 'k=%d point %s : entree %s, D_k = %s' % (k, pt, e, ctx.entry(x, k))
        if not 0 <= v < nn:
            return 'k=%d point %s : noeud d\'entree %d hors bornes' % (k, pt, v)
        pts_at[e].append((x, v))
    if o['points'] and len(seen) != ctx.n:
        return 'k=%d : %d attaches pour %d points' % (k, len(seen), ctx.n)
    tp = list(range(nn))
    gp = {}

    def tfind(x):
        r = x
        while tp[r] != r:
            r = tp[r]
        while tp[x] != r:
            tp[x], x = r, tp[x]
        return r

    def gfind(F):
        r = F
        while gp[r] != r:
            r = gp[r]
        while gp[F] != r:
            gp[F], F = r, gp[F]
        return r
    alive, populated = set(), set()
    ncomp = 0
    for a in sorted(set(gam.levels) | set(nodes_at) | set(up_at) | set(pts_at)):
        for F in gam.v_at.get(a, ()):
            gp[F] = F
            ncomp += 1
        for G in gam.e_at.get(a, ()):
            r0 = gfind(G[1:])
            for j in range(1, len(G)):
                r = gfind(G[:j] + G[j + 1:])
                if r != r0:
                    gp[r] = r0
                    ncomp -= 1
        new = nodes_at.get(a, ())
        alive.update(new)
        for v in new:
            for c in children[v]:
                tp[c] = v
                alive.discard(c)
                if c in populated:
                    populated.add(v)
        for v in new:
            if children[v] and len({gfind(wit[leaf_of[c]]) for c in children[v]}) != 1:
                return 'k=%d a=%s fusion %d : enfants dans des composantes distinctes de Gamma_k' % (k, a, v)
        owner = {}
        for r in alive:
            g = gfind(wit[leaf_of[r]])
            if g in owner:
                return 'k=%d a=%s : noeuds vivants %d et %d dans une meme composante de Gamma_k' % (k, a, owner[g], r)
            owner[g] = r
        if len(alive) != ncomp:
            return 'k=%d a=%s : %d noeuds vivants contre %d composantes de Gamma_k' % (k, a, len(alive), ncomp)
        cnt['bijections'] += 1
        for x, v in pts_at.get(a, ()):
            if nodes[v][1] > a:
                return 'k=%d point %s : noeud d\'entree %d ne apres l\'entree' % (k, ctx.P[x], v)
            r = tfind(v)
            populated.add(r)
            if gfind(ctx.knn(x, k)) != gfind(wit[leaf_of[r]]):
                return 'k=%d a=%s point %s : composante d\'entree fausse' % (k, a, ctx.P[x])
            cnt['points'] += 1
        for v in new:
            empty[v] = tfind(v) not in populated
        if up is None:
            continue
        for u in up_at.get(a, ()):
            w = up['nodes'][u][2]
            if not 0 <= w < nn:
                return 'verticale k=%d noeud %d : image %d absente ou hors bornes' % (k + 1, u, w)
            if nodes[w][1] > a:
                return 'verticale k=%d noeud %d a=%s : image %d nee apres (%s)' % (k + 1, u, a, w, nodes[w][1])
            if tfind(w) != w:
                return 'verticale k=%d noeud %d a=%s : image %d non vivante a la coupe fermee' % (k + 1, u, a, w)
            Wu = upinf['wit'][upinf['leaf_of'][u]]
            if gfind(Wu[:-1]) != gfind(wit[leaf_of[w]]):
                return 'verticale k=%d noeud %d a=%s : image %d fausse' % (k + 1, u, a, w)
            cnt['verticals'] += 1
            vq.append((k + 1, u))
    return None


def new_counters():
    return dict(cuts=0, bijections=0, points=0, witnesses=0, verticals=0, verticals_empty=0, verticals_points=0)


def judge(P, orders, K, ctx=None, cnt=None):
    """Juge complet d'un dump parse (avec temoins). Rend None ou le premier ecart ; cnt recoit les controles."""
    ctx = ctx if ctx is not None else Ctx(P)
    cnt = cnt if cnt is not None else new_counters()
    kk = min(K, len(P))
    if sorted(orders) != list(range(1, kk + 1)):
        return 'ordres du dump %s, attendus 1..%d' % (sorted(orders), kk)
    info = {}
    for k in range(1, kk + 1):
        err, info[k] = structure(orders[k], k, ctx)
        if err:
            return err
        cnt['witnesses'] += len(info[k]['wit'])
    vq = []
    for k in range(1, kk + 1):
        err = sweep(k, orders, info, ctx, cnt, vq)
        if err:
            return err
    cnt['verticals_empty'] += sum(1 for k, u in vq if info[k]['empty'][u])
    # controles historiques (conserves) : comptes et partitions C n X aux coupes ouvertes et fermees des niveaux
    # critiques, puis verticales par points
    idx = ctx.idx
    for k in range(1, kk + 1):
        o = orders[k]
        nodes = o['nodes']
        for a, op, cl in gamma_sweep(P, k, ctx):
            for is_closed, (want_n, want_part) in ((True, cl), (False, op)):
                ok = (lambda l: l <= a) if is_closed else (lambda l: l < a)
                alive = sum(1 for v, (par, lv, _low) in enumerate(nodes)
                            if ok(lv) and (par < 0 or not ok(nodes[par][1])))
                if alive != want_n:
                    return 'k=%d a=%s ferme=%s composantes %d contre %d' % (k, a, is_closed, alive, want_n)
                groups = {}
                for pt, v, e in o['points']:
                    if ok(e):
                        groups.setdefault(top(nodes, v, a, is_closed), set()).add(idx[pt])
                mine = sorted((frozenset(g) for g in groups.values()), key=lambda s: sorted(s))
                if mine != want_part:
                    return 'k=%d a=%s ferme=%s partition C n X' % (k, a, is_closed)
                cnt['cuts'] += 1
    for k in range(2, kk + 1):
        levels = sorted({lv for _, lv, _ in orders[k]['nodes']} | {e for _, _, e in orders[k]['points']})
        err, c = vertical_check_points(orders, k, P, levels)
        if err:
            return err
        cnt['verticals_points'] += c
    return None


def vertical_check(orders, k, P, levels=None):
    """Carte verticale k -> k-1 (interface de l'audit, probe_vertical.py) : juge par temoins de chaque naissance
    et fusion de l'ordre k (bijections des ordres k - 1 et k comprises), puis ancien controle par points sur
    levels. Rend (ecart ou None, nombre de controles). Exige un dump --dump-births."""
    ctx = Ctx(P)
    cnt = new_counters()
    info = {}
    for j in (k - 1, k):
        err, info[j] = structure(orders[j], j, ctx)
        if err:
            return err, 0
    vq = []
    for j in (k, k - 1):
        err = sweep(j, {j: orders[j], j + 1: orders[j + 1]} if j == k - 1 else {j: orders[j]}, info, ctx, cnt, vq)
        if err:
            return err, cnt['verticals']
    if levels is not None:
        err, c = vertical_check_points(orders, k, P, levels)
        if err:
            return err, cnt['verticals'] + c
        cnt['verticals_points'] += c
    return None, cnt['verticals'] + cnt['verticals_points']


# ---------------------------------------------------------------- execution du binaire

def run_tower(exe, P, K, tmp):
    src = os.path.join(tmp, 'in.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    dump = os.path.join(tmp, 'tower.txt')
    r = subprocess.run([exe, src, '--k=%d' % K, '--threads=2', '--dump=' + dump, '--dump-births'],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return 'code %d %s' % (r.returncode, r.stdout.strip()), None
    return None, parse(dump)


def check(exe, P, K, tmp, ctx=None, cnt=None):
    """Execute la tour et juge son dump. Rend (ecart ou None, coupes jugees)."""
    cnt = cnt if cnt is not None else new_counters()
    before = cnt['cuts']
    err, orders = run_tower(exe, P, K, tmp)
    if err:
        return err, 0
    return judge(P, orders, K, ctx, cnt), cnt['cuts'] - before


# ---------------------------------------------------------------- mutants

AUDIT3 = [(0, 0, 0), (2, 0, 0), (5, 0, 0)]  # audit continu, pool_head/probe_vertical.py
E5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
# fusion d'ordre 2 au niveau 2/3 (triangle equilateral de cote racine de 2) : aucun point n'y est entre avant le
# niveau 2 ; a l'ordre 1, le site (2,2,0) ne rejoint le triangle qu'a 5/4 ; paire lointaine (50,50,50), (51,50,50)
TRIANGLE = [(1, 0, 0), (0, 1, 0), (0, 0, 1), (2, 2, 0), (50, 50, 50), (51, 50, 50)]
# trois naissances d'ordre 2 au niveau 1 ({0,2}, {2,4}, {10,12}) ; {0,2} et {2,4} fusionnent a 4, {10,12} bien plus tard
LINE5 = [(0, 0, 0), (2, 0, 0), (4, 0, 0), (10, 0, 0), (12, 0, 0)]
SQUARE = [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]
OCTA = [(10 + x, 10 + y, 10 + z) for x, y, z in ((5, 0, 0), (-5, 0, 0), (0, 5, 0), (0, -5, 0), (0, 0, 5), (0, 0, -5))]
CUBE = [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)]


def node_of_point(o, k, pt):
    return next(v for p, v, _ in o[k]['points'] if p == pt)


def born_at(o, k, level):
    return next(v for v in sorted(o[k]['births']) if o[k]['nodes'][v][1] == level)


def set_lower(o, k, v, w):
    par, lv, _low = o[k]['nodes'][v]
    o[k]['nodes'][v] = (par, lv, w)


def mut_vertical_audit(o):
    """Mutant de l'audit : image de la naissance {(0,0,0), (2,0,0)} (ordre 2, niveau 1) = singleton {(5,0,0)}."""
    set_lower(o, 2, born_at(o, 2, Fraction(1)), node_of_point(o, 1, (5, 0, 0)))


def mut_vertical_descendant(o):
    """Image = feuille {(0,0,0)}, descendant de la bonne image vivante (fusion d'ordre 1 au niveau 1)."""
    set_lower(o, 2, born_at(o, 2, Fraction(1)), node_of_point(o, 1, (0, 0, 0)))


def mut_vertical_later(o):
    """Image = racine de l'ordre 1, nee a 9/4 apres la naissance (niveau 1)."""
    set_lower(o, 2, born_at(o, 2, Fraction(1)), next(w for w, nd in enumerate(o[1]['nodes']) if nd[0] < 0))


def mut_vertical_fusion(o):
    """Fusion d'ordre 2 du triangle au niveau 2/3 : image = composante de la paire lointaine (vivante a 2/3)."""
    v = next(v for v, nd in enumerate(o[2]['nodes']) if nd[1] == Fraction(2, 3) and v not in o[2]['births'])
    set_lower(o, 2, v, top(o[1]['nodes'], node_of_point(o, 1, (50, 50, 50)), Fraction(2, 3), True))


def mut_vertical_missing(o):
    """Image absente (-1) sur la naissance d'ordre 2 au niveau 1."""
    set_lower(o, 2, born_at(o, 2, Fraction(1)), -1)


def mut_witness_swap(o):
    """Temoins des naissances {0,2} et {10,12} (niveau 1, composantes distinctes) echanges."""
    b = o[2]['births']
    u = next(v for v in b if b[v] == ((0, 0, 0), (2, 0, 0)))
    w = next(v for v in b if b[v] == ((10, 0, 0), (12, 0, 0)))
    b[u], b[w] = b[w], b[u]


def mut_witness_level(o):
    """Temoin de la naissance {(0,0,0), (2,0,0)} remplace par {(0,0,0), (5,0,0)} (miniboule 25/4, pas 1)."""
    o[2]['births'][born_at(o, 2, Fraction(1))] = ((0, 0, 0), (5, 0, 0))


INJECT = {
    'vertical_audit': (AUDIT3, 2, mut_vertical_audit),
    'vertical_descendant': (AUDIT3, 2, mut_vertical_descendant),
    'vertical_later': (AUDIT3, 2, mut_vertical_later),
    'vertical_fusion': (TRIANGLE, 3, mut_vertical_fusion),
    'vertical_missing': (AUDIT3, 2, mut_vertical_missing),
    'witness_level': (AUDIT3, 2, mut_witness_level),
    'witness_swap': (LINE5, 2, mut_witness_swap),
}
SYSTEMATIC = [(AUDIT3, 2), (TRIANGLE, 3), (E5, 4), (LINE5, 3), (SQUARE, 3), (OCTA, 4), (CUBE, 5)]


def systematic_mutants(P, orders):
    """Toutes les images verticales fausses d'une fixture : pour chaque noeud u d'ordre k >= 2 (niveau a), toute
    autre image vivante a la coupe fermee a, un descendant strict et le parent (ne apres a) de la bonne image."""
    out = []
    for k in sorted(orders):
        if k < 2:
            continue
        down = orders[k - 1]['nodes']
        for u, (_par, a, low) in enumerate(orders[k]['nodes']):
            alive = [w for w, (par, lv, _) in enumerate(down) if lv <= a and (par < 0 or down[par][1] > a)]
            wrong = [('vivante', w) for w in alive if w != low]
            kids = [w for w, nd in enumerate(down) if nd[0] == low]
            if kids:
                wrong.append(('descendant', kids[0]))
            if down[low][0] >= 0:
                wrong.append(('parent', down[low][0]))
            for kind, w in wrong:
                out.append((k, u, w, kind))
    return out


def run_systematic(exe, tmp, cnt):
    """Chaque mutant systematique doit etre rejete par le juge ; rend (tues, total, aveugles de l'ancien juge)."""
    killed = total = blind = 0
    for P, K in SYSTEMATIC:
        ctx = Ctx(P)
        err, orders = run_tower(exe, P, K, tmp)
        if err or judge(P, orders, K, ctx) is not None:
            return None, 'fixture %s K=%d non conforme : %s' % (P, K, err or judge(P, orders, K, ctx))
        for k, u, w, kind in systematic_mutants(P, orders):
            m = copy.deepcopy(orders)
            set_lower(m, k, u, w)
            total += 1
            if judge(P, m, K, ctx) is not None:
                killed += 1
            levels = sorted({lv for _, lv, _ in m[k]['nodes']} | {e for _, _, e in m[k]['points']})
            if vertical_check_points(m, k, P, levels)[0] is None:
                blind += 1
    cnt['mutants'], cnt['mutants_killed'], cnt['mutants_blind_old'] = total, killed, blind
    return (killed, total, blind), None


def inject(exe, name):
    if name not in INJECT:
        print('mutant inconnu %s (connus : %s)' % (name, ', '.join(sorted(INJECT))))
        return 2
    P, K, mutate = INJECT[name]
    with tempfile.TemporaryDirectory() as tmp:
        err, orders = run_tower(exe, P, K, tmp)
    if err:
        print('fixture %s : %s' % (name, err))
        return 1
    ctx = Ctx(P)
    base = judge(P, orders, K, ctx)
    if base is not None:
        print('fixture %s non conforme : %s' % (name, base))
        return 1
    m = copy.deepcopy(orders)
    mutate(m)
    err = judge(P, m, K, ctx)
    if err is None:
        print('mutant %s SURVIT' % name)
        return 0
    print('mutant %s tue : %s' % (name, err))
    print('mutant_killed %s' % name)
    return 4


# ---------------------------------------------------------------- campagne

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--inject=')]
    exe = os.path.join(args[0], 'mhgp10_tower')
    for a in sys.argv[1:]:
        if a.startswith('--inject='):
            return inject(exe, a[len('--inject='):])
    count = int(args[1]) if len(args) > 1 else 24
    rnd = random.Random(20260929)
    checks = fails = 0
    cnt = new_counters()
    with tempfile.TemporaryDirectory() as tmp:
        for P in clouds(count, rnd):
            P = P[:12]
            ctx = Ctx(P)
            for K in (1, 3, 5):
                err, _c = check(exe, P, K, tmp, ctx, cnt)
                checks += 1
                if err:
                    fails += 1
                    if fails <= 5:
                        print('ECART K=%d n=%d : %s\n  %s' % (K, len(P), err, P))
        err, _c = check(exe, E5, 4, tmp, Ctx(E5), cnt)
        checks += 1
        if err:
            fails += 1
            print('ECART E5 : %s' % err)
        # autocontrole du cache exact : Balls.beta == R.meb sur toutes les parties d'E5, de l'octaedre et du cube
        for Q in (E5, OCTA, CUBE):
            ctx = Ctx(Q)
            for q in range(1, len(Q) + 1):
                for F in combinations(range(len(Q)), q):
                    if ctx.balls.beta(F) != R.meb(Q, F)[0]:
                        fails += 1
                        print('ECART cache de miniboules %s %s' % (Q, F))
        res, err = run_systematic(exe, tmp, cnt)
        if err:
            fails += 1
            print('ECART mutants : %s' % err)
    print('tower_oracle_checks %d fails %d cuts %d' % (checks, fails, cnt['cuts']))
    print('witness_checks bijections %d points %d witnesses %d verticals %d verticals_empty %d verticals_points %d'
          % (cnt['bijections'], cnt['points'], cnt['witnesses'], cnt['verticals'], cnt['verticals_empty'],
             cnt['verticals_points']))
    if res is not None:
        print('mutants %d killed %d blind_to_old_judge %d' % (res[1], res[0], res[2]))
    if fails:
        return 1
    if res is None or res[0] != res[1]:
        print('PLANCHER : mutant vertical survivant')
        return 3
    # planchers contre le vert par vacuite (campagne par defaut, 24 nuages : coupes 23 444, bijections 12 574,
    # verticales 3 978 dont 2 905 sans point entre, temoins 3 271, attaches 2 252 ; 218 mutants dont 200 invisibles
    # pour l'ancien controle par points)
    floors = dict(cuts=max(500, 700 * count), bijections=400 * count, verticals=120 * count,
                  verticals_empty=80 * count, witnesses=100 * count, points=70 * count, mutants=200,
                  mutants_blind_old=150)
    low = [key for key, v in floors.items() if cnt[key] < v]
    if low:
        print('PLANCHER : %s' % ', '.join('%s %d < %d' % (key, cnt[key], floors[key]) for key in low))
        return 3
    return 0


if __name__ == '__main__':
    sys.exit(main())
