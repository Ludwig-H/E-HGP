#!/usr/bin/env python3
"""Juge independant L02 (hors depot) : arbre de fusion EXACT de Gamma_k, etiquete par les naissances, contre le dump
enrichi de l02_dump (foret C++ de la v10, verticales, attaches core).

Objet de reference (Fraction, aucune dependance au code de la v10) :
  - beta(F) = rayon carre de la plus petite boule englobante (force brute sur les supports de 1 a 4 points) ;
  - Gamma_k(a) : sommets = k-parties F avec beta(F) <= a (coupe fermee) ou < a (ouverte) ; aretes = (k+1)-parties G,
    qui relient toutes leurs faces ;
  - arbre de fusion : a chaque niveau critique a, une composante fermee qui contient 0 composante ouverte est une
    NAISSANCE, 1 une continuation (pas de noeud), >= 2 une MULTIFUSION dont les enfants sont exactement ces composantes ;
  - verticale d'un noeud v d'ordre k cree au niveau a : composante de Gamma_{k-1}(a ferme) d'une (k-1)-partie d'un
    sommet de v ;
  - attache core de x : composante de Gamma_k(D_k(x) ferme) du sommet N_k(x).

Controles sur le dump C++ (tout noeud, tout ordre) :
  B  bijection des naissances par (niveau exact, couverture = population I u U de la boule) ;
  M  egalite des multifusions par (niveau exact, ensemble des feuilles, ensembles de feuilles des enfants) ;
  V  image verticale de CHAQUE noeud = noeud de l'ordre k-1 vivant a la coupe fermee de son niveau ;
  P  attache core de chaque site : niveau D_k(x) et noeud vivant a la coupe fermee.
Code : 0 conforme, 1 desaccord, 2 refus du binaire.
"""
import itertools
import json
import os
import random
import subprocess
import sys
import tempfile
from fractions import Fraction as Fr

comb = itertools.combinations


# ------------------------------------------------------------------ geometrie exacte (propre au juge)
def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _solve(G, r):
    n = len(G)
    M = [[Fr(v) for v in G[i]] + [Fr(r[i])] for i in range(n)]
    for c in range(n):
        piv = next((i for i in range(c, n) if M[i][c] != 0), None)
        if piv is None:
            return None
        M[c], M[piv] = M[piv], M[c]
        for i in range(n):
            if i != c and M[i][c] != 0:
                f = M[i][c] / M[c][c]
                M[i] = [x - f * y for x, y in zip(M[i], M[c])]
    return [M[i][n] / M[i][i] for i in range(n)]


def _circum(pts):
    """Centre de la sphere circonscrite dans aff(pts), coordonnees barycentriques ; None si affinement dependant."""
    p0 = pts[0]
    if len(pts) == 1:
        return (Fr(p0[0]), Fr(p0[1]), Fr(p0[2])), [Fr(1)]
    D = [_sub(p, p0) for p in pts[1:]]
    lam = _solve([[_dot(a, b) for b in D] for a in D], [Fr(_dot(a, a), 2) for a in D])
    if lam is None:
        return None
    c = tuple(Fr(p0[j]) + sum(l * d[j] for l, d in zip(lam, D)) for j in range(3))
    return c, [1 - sum(lam)] + lam


def _d2(c, p):
    return (p[0] - c[0]) ** 2 + (p[1] - c[1]) ** 2 + (p[2] - c[2]) ** 2


class Geo:
    def __init__(self, P):
        self.P = P
        self.cache = {}

    def meb(self, idx):
        """(beta, centre) de la plus petite boule englobante de la partie idx (tuple trie)."""
        r = self.cache.get(idx)
        if r is not None:
            return r
        P = self.P
        out = None
        if len(idx) == 1:
            p0 = P[idx[0]]
            out = (Fr(0), (Fr(p0[0]), Fr(p0[1]), Fr(p0[2])))
        else:
            # Lemme de Welzl : si le dernier point x est dans la MEB de idx \ {x}, c'est la meme boule ; sinon x est
            # sur le bord de la MEB de idx et appartient a TOUT support (sinon le centre serait dans l'enveloppe de
            # points de bord de idx \ {x}, et la boule serait deja la MEB de idx \ {x}).
            last = idx[-1]
            rad0, c0 = self.meb(idx[:-1])
            if _d2(c0, P[last]) <= rad0:
                out = (rad0, c0)
            else:
                rest = idx[:-1]
                for q in range(1, min(3, len(rest)) + 1):
                    for S in comb(rest, q):
                        S = S + (last,)
                        cc = _circum([P[i] for i in S])
                        if cc is None:
                            continue
                        c, lam = cc
                        if any(l < 0 for l in lam):
                            continue
                        rad = _d2(c, P[last])
                        if all(_d2(c, P[i]) <= rad for i in idx):
                            out = (rad, c)
                            break
                    if out is not None:
                        break
        self.cache[idx] = out
        return out

    def meb_brute(self, idx):
        """Force brute sur tous les supports (controle croise de meb)."""
        P = self.P
        for q in range(1, min(4, len(idx)) + 1):
            for S in comb(idx, q):
                cc = _circum([P[i] for i in S])
                if cc is None:
                    continue
                c, lam = cc
                if any(l < 0 for l in lam):
                    continue
                rad = _d2(c, P[S[0]])
                if all(_d2(c, P[i]) <= rad for i in idx):
                    return (rad, c)
        return None


# ------------------------------------------------------------------ arbre de fusion exact de Gamma_k
class Node:
    __slots__ = ('id', 'level', 'children', 'parent', 'leaves', 'cover0', 'rep', 'cover')

    def __init__(self, i, level, children, rep, cover0):
        self.id, self.level, self.children, self.rep, self.cover0 = i, level, children, rep, cover0
        self.parent = None
        self.leaves = None
        self.cover = None


def gamma_tree(geo, n, k):
    """Arbre de fusion de Gamma_k. Rend (noeuds, first_node par sommet, statistiques)."""
    beta = {F: geo.meb(F)[0] for F in comb(range(n), k)}
    cof = {G: geo.meb(G)[0] for G in comb(range(n), k + 1)} if k < n else {}
    levels = sorted(set(beta.values()) | set(cof.values()))
    vs = sorted(beta.items(), key=lambda t: t[1])
    es = sorted(cof.items(), key=lambda t: t[1])
    par = {}

    def find(x):
        r = x
        while par[r] != r:
            r = par[r]
        while par[x] != r:
            par[x], x = r, par[x]
        return r

    nodes = []
    node_of_root = {}   # racine DSU -> noeud courant de la composante
    cover_of_root = {}  # racine DSU -> couverture (points)
    first = {}
    iv = ie = 0
    stats = dict(continuations=0, cover_growth_no_merge=0, cover_growth_points=0, births=0, merges=0,
                 arity_ge3=0, levels_with_2plus_events=0, births_pop_gt_k=0, merge_new_points=0)
    for a in levels:
        new_vs = []
        while iv < len(vs) and vs[iv][1] == a:
            F = vs[iv][0]
            par[F] = F
            new_vs.append(F)
            iv += 1
        old_roots_touched = set()
        while ie < len(es) and es[ie][1] == a:
            G = es[ie][0]
            fs = [G[:i] + G[i + 1:] for i in range(k + 1)]
            r0 = find(fs[0])
            for f in fs[1:]:
                rf = find(f)
                if rf != r0:
                    par[rf] = r0
            ie += 1
        # regrouper : anciennes composantes (racines connues de node_of_root) et nouveaux sommets par racine fermee
        groups = {}
        for r_old in list(node_of_root):
            groups.setdefault(find(r_old), [[], []])[0].append(r_old)
        for F in new_vs:
            groups.setdefault(find(F), [[], []])[1].append(F)
        events = 0
        new_node_of_root, new_cover_of_root = {}, {}
        for r, (olds, news) in groups.items():
            newpts = set()
            for F in news:
                newpts.update(F)
            if len(olds) == 0:
                nd = Node(len(nodes), a, [], news[0], frozenset(newpts))
                nodes.append(nd)
                stats['births'] += 1
                if len(newpts) > k:
                    stats['births_pop_gt_k'] += 1
                events += 1
                cov = set(newpts)
            elif len(olds) == 1:
                nd = node_of_root[olds[0]]
                cov = cover_of_root[olds[0]]
                if news:
                    stats['continuations'] += 1
                    extra = newpts - cov
                    if extra:
                        stats['cover_growth_no_merge'] += 1
                        stats['cover_growth_points'] += len(extra)
                    cov = cov | newpts
            else:
                kids = [node_of_root[o] for o in olds]
                nd = Node(len(nodes), a, kids, kids[0].rep, None)
                nodes.append(nd)
                for c in kids:
                    c.parent = nd
                stats['merges'] += 1
                if len(kids) >= 3:
                    stats['arity_ge3'] += 1
                events += 1
                cov = set()
                for o in olds:
                    cov |= cover_of_root[o]
                if newpts - cov:
                    stats['merge_new_points'] += 1
                cov |= newpts
            new_node_of_root[r] = nd
            new_cover_of_root[r] = cov
            for F in news:
                first[F] = nd
        node_of_root, cover_of_root = new_node_of_root, new_cover_of_root
        if events >= 2:
            stats['levels_with_2plus_events'] += 1
    # ensembles de feuilles
    for nd in nodes:  # enfants crees avant leurs parents
        nd.leaves = frozenset([nd.id]) if not nd.children else frozenset().union(*[c.leaves for c in nd.children])
    return nodes, first, stats


def alive(nd, a):
    """Noeud vivant a la coupe fermee a, en remontant depuis nd (niveau(nd) <= a)."""
    while nd.parent is not None and nd.parent.level <= a:
        nd = nd.parent
    return nd


# ------------------------------------------------------------------ dump C++
def parse_dump(path):
    sites = {}
    orders = {}
    cur = None
    for line in open(path):
        t = line.split()
        if t[0] == 'site':
            sites[int(t[1])] = (int(t[2]), int(t[3]), int(t[4]))
        elif t[0] == 'order':
            cur = dict(nodes=[], points=[])
            orders[int(t[1])] = cur
        elif t[0] == 'node':
            kind = t[6]
            pop = [int(v) for v in t[7:]] if kind == 'B' else None
            cur['nodes'].append(dict(parent=int(t[2]), level=Fr(int(t[3]), int(t[4])), lower=int(t[5]), pop=pop))
        elif t[0] == 'point':
            cur['points'].append((int(t[1]), int(t[2]), int(t[3])))
    return sites, orders


_TREES = {}


def cached_tree(geo, P, n, k):
    key = (tuple(P), k)
    if key not in _TREES:
        _TREES[key] = gamma_tree(geo, n, k)
    return _TREES[key]


def judge_cloud(exe, P, K, tmp, kcat=None, want_stats=None, extra=(), euler_strict=True, cache=False):
    """P : liste de points entiers distincts (u18). Rend (erreur ou None, compteurs, code).
    extra : options supplementaires de la sonde ; euler_strict=False : Euler compte (cnt['euler_bad']) sans arreter ;
    cache : arbres de Gamma memorises par (nuage, ordre) (meme nuage juge plusieurs fois)."""
    n = len(P)
    src = os.path.join(tmp, 'in.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    dump = os.path.join(tmp, 'dump.txt')
    if kcat is None:
        kcat = min(12, K + 2)
    r = subprocess.run([exe, src, '--k=%d' % K, '--kcat=%d' % kcat, '--threads=2', '--dump=' + dump] + list(extra),
                       capture_output=True, text=True)
    cnt = dict(births=0, merges=0, verticals=0, points=0, euler=0, arity_ge3=0, cover_growth_no_merge=0,
               births_pop_gt_k=0, levels_with_2plus_events=0, continuations=0, merge_new_points=0, orders=0)
    cnt['euler_bad'] = 0
    cnt['_stdout'] = r.stdout.strip()
    if r.returncode != 0:
        try:
            for e in json.loads(r.stdout).get('euler', []):
                if e['in_range'] and e['chi'] != 1:
                    cnt['euler_bad'] += 1
        except ValueError:
            pass
        return 'refus code %d %s' % (r.returncode, r.stdout.strip()[:200]), cnt, 2
    led = json.loads(r.stdout)
    # Euler (calcule par la sonde sur le catalogue kcat) : chi = 1 pour K <= min(n, kcat - 2)
    for e in led.get('euler', []):
        if e['in_range']:
            cnt['euler'] += 1
            if e['chi'] != 1:
                cnt['euler_bad'] += 1
                if euler_strict:
                    return 'Euler K=%d chi=%d' % (e['K'], e['chi']), cnt, 1
    sites, orders = parse_dump(dump)
    idx = {p: i for i, p in enumerate(P)}
    s2i = {s: idx[c] for s, c in sites.items()}
    geo = Geo(P)
    trees = {}
    for k in range(1, min(K, n) + 1):
        nodes, first, st = cached_tree(geo, P, n, k) if cache else gamma_tree(geo, n, k)
        trees[k] = (nodes, first)
        for key in ('arity_ge3', 'cover_growth_no_merge', 'births_pop_gt_k', 'levels_with_2plus_events', 'continuations',
                    'merge_new_points'):
            cnt[key] += st[key]
        cnt['orders'] += 1
        o = orders.get(k)
        if o is None:
            return 'ordre %d absent du dump' % k, cnt, 1
        cn = o['nodes']
        # --- B : naissances
        gb = {}
        for nd in nodes:
            if not nd.children:
                key = (nd.level, nd.cover0)
                if key in gb:
                    return 'k=%d oracle : deux naissances de meme cle' % k, cnt, 1
                gb[key] = nd
        cpp_leafset = [None] * len(cn)
        cpp_children = [[] for _ in cn]
        seen = set()
        for v, c in enumerate(cn):
            if c['parent'] >= 0:
                if c['parent'] <= v:
                    return 'k=%d noeud %d : parent %d non posterieur' % (k, v, c['parent']), cnt, 1
                cpp_children[c['parent']].append(v)
        for v, c in enumerate(cn):
            if c['pop'] is not None:
                if cpp_children[v]:
                    return 'k=%d noeud %d : naissance avec enfants' % (k, v), cnt, 1
                key = (c['level'], frozenset(s2i[s] for s in c['pop']))
                g = gb.get(key)
                if g is None:
                    return 'k=%d naissance C++ absente de Gamma : niveau %s pop %s' % (k, c['level'], sorted(key[1])), cnt, 1
                if g.id in seen:
                    return 'k=%d naissance C++ en double' % k, cnt, 1
                seen.add(g.id)
                cpp_leafset[v] = frozenset([g.id])
                cnt['births'] += 1
        if len(seen) != len(gb):
            miss = [(str(l), sorted(c)) for (l, c), g in gb.items() if g.id not in seen][:3]
            return 'k=%d naissances de Gamma absentes du C++ : %d, ex %s' % (k, len(gb) - len(seen), miss), cnt, 1
        # --- M : multifusions (enfants avant parents dans le dump)
        for v, c in enumerate(cn):
            if c['pop'] is None:
                if len(cpp_children[v]) < 2:
                    return 'k=%d fusion %d a %d enfant(s)' % (k, v, len(cpp_children[v])), cnt, 1
                if any(cpp_leafset[u] is None for u in cpp_children[v]):
                    return 'k=%d fusion %d : enfant non resolu' % (k, v), cnt, 1
                cpp_leafset[v] = frozenset().union(*[cpp_leafset[u] for u in cpp_children[v]])
        gm = {nd.leaves: nd for nd in nodes}
        cm = {}
        for v, c in enumerate(cn):
            if cpp_leafset[v] in cm:
                return 'k=%d deux noeuds C++ de meme ensemble de feuilles' % k, cnt, 1
            cm[cpp_leafset[v]] = v
        if set(gm) != set(cm):
            return 'k=%d ensembles de feuilles differents : Gamma %d noeuds, C++ %d, communs %d' % (
                k, len(gm), len(cm), len(set(gm) & set(cm))), cnt, 1
        for ls, nd in gm.items():
            v = cm[ls]
            if cn[v]['level'] != nd.level:
                return 'k=%d noeud %s : niveau C++ %s, Gamma %s' % (k, sorted(ls), cn[v]['level'], nd.level), cnt, 1
            if nd.children:
                want = frozenset(c.leaves for c in nd.children)
                got = frozenset(cpp_leafset[u] for u in cpp_children[v])
                if want != got:
                    return 'k=%d fusion %s : enfants differents (C++ %d, Gamma %d)' % (
                        k, sorted(ls), len(got), len(want)), cnt, 1
                cnt['merges'] += 1
        o['leafset'] = cpp_leafset
        o['by_leafset'] = cm
        # --- P : attaches core
        if o['points']:
            if len(o['points']) != n:
                return 'k=%d : %d attaches pour %d sites' % (k, len(o['points']), n), cnt, 1
            for s, v, e in o['points']:
                x = s2i[s]
                d = sorted((_dot(_sub(P[x], P[y]), _sub(P[x], P[y])), y) for y in range(n))
                want_e = d[k - 1][0]
                if e != want_e:
                    return 'k=%d site %d : niveau d\'entree %d, attendu %d' % (k, x, e, want_e), cnt, 1
                N = tuple(sorted(y for _, y in d[:k]))
                g = alive(first[N], Fr(want_e))
                if cpp_leafset[v] != g.leaves:
                    return 'k=%d site %d : attache au noeud %s, attendu %s' % (
                        k, x, sorted(cpp_leafset[v]), sorted(g.leaves)), cnt, 1
                cnt['points'] += 1
    # --- V : verticales de tous les noeuds
    for k in range(2, min(K, n) + 1):
        nodes, first = trees[k]
        dn, dfirst = trees[k - 1]
        up, down = orders[k], orders[k - 1]
        for nd in nodes:
            v = up['by_leafset'][nd.leaves]
            F = nd.rep
            Fm = F[1:]
            g = alive(dfirst[Fm], nd.level)
            low = up['nodes'][v]['lower']
            if low < 0 or low >= len(down['nodes']):
                return 'verticale k=%d noeud %s : image absente' % (k, sorted(nd.leaves)), cnt, 1
            if down['leafset'][low] != g.leaves:
                return 'verticale k=%d noeud %s (niveau %s) : image C++ %s, Gamma %s' % (
                    k, sorted(nd.leaves), nd.level, sorted(down['leafset'][low]), sorted(g.leaves)), cnt, 1
            cnt['verticals'] += 1
    return None, cnt, 0


# ------------------------------------------------------------------ familles de nuages (coordonnees u18 >= 0)
def fam_generic(rnd, n):
    s = set()
    while len(s) < n:
        s.add(tuple(rnd.randint(0, 1000) for _ in range(3)))
    return sorted(s)


def fam_grid(rnd, n, side=3):
    s = set()
    while len(s) < n:
        s.add(tuple(rnd.randint(0, side) for _ in range(3)))
    return sorted(s)


def fam_plane(rnd, n):
    s = set()
    while len(s) < n:
        s.add((rnd.randint(0, 6), rnd.randint(0, 6), 0))
    return sorted(s)


def fam_clusters(rnd, n):
    """2 ou 3 amas serres (+-3) et 30 % de points epars : descentes longues."""
    nc = rnd.choice((2, 3))
    centers = [tuple(rnd.randint(20, 80) for _ in range(3)) for _ in range(nc)]
    s = set()
    while len(s) < n:
        if rnd.random() < 0.3:
            s.add(tuple(rnd.randint(0, 100) for _ in range(3)))
        else:
            c = rnd.choice(centers)
            s.add(tuple(c[i] + rnd.randint(-3, 3) for i in range(3)))
    return sorted(s)


_SPH = sorted({(x, y, z) for x in range(-3, 4) for y in range(-3, 4) for z in range(-3, 4) if x * x + y * y + z * z == 9})
_CIR = sorted({(x, y, 0) for x in range(-5, 6) for y in range(-5, 6) if x * x + y * y == 25})


def fam_sphere(rnd, n):
    """Points de reseau sur la sphere x2+y2+z2 = 9 (30 points), le centre parfois, quelques points libres."""
    m = rnd.randint(4, n)
    s = set(rnd.sample(_SPH, min(m, len(_SPH))))
    if rnd.random() < 0.5:
        s.add((0, 0, 0))
    while len(s) < n:
        s.add(tuple(rnd.randint(-4, 4) for _ in range(3)))
    return sorted((x + 5, y + 5, z + 5) for x, y, z in s)


def fam_circle(rnd, n):
    """Points de reseau sur le cercle x2+y2 = 25 (12 points) dans z = 0, le centre parfois, quelques points libres."""
    m = rnd.randint(4, n)
    s = set(rnd.sample(_CIR, min(m, len(_CIR))))
    if rnd.random() < 0.5:
        s.add((0, 0, 0))
    while len(s) < n:
        s.add((rnd.randint(-6, 6), rnd.randint(-6, 6), rnd.choice((0, 0, 0, 1, 2))))
    return sorted((x + 7, y + 7, z + 1) for x, y, z in s)


def fam_line(rnd, n):
    """Points alignes (pas regulier ou non) et quelques points hors de la droite."""
    m = rnd.randint(3, n - 1)
    step = rnd.choice(((1, 0, 0), (1, 1, 0), (1, 2, 3)))
    ts = rnd.sample(range(0, 9), min(m, 9))
    s = {(10 + t * step[0], 10 + t * step[1], 10 + t * step[2]) for t in ts}
    while len(s) < n:
        s.add(tuple(rnd.randint(8, 22) for _ in range(3)))
    return sorted(s)


def fam_cube(rnd, n):
    """Sommets de cubes et de paves, centres de faces, centre : beaucoup d'egalites de niveaux."""
    a, b, c = rnd.choice(((2, 2, 2), (2, 4, 2), (2, 4, 6), (4, 4, 2)))
    base = [(x, y, z) for x in (0, a) for y in (0, b) for z in (0, c)]
    extra = [(a // 2, b // 2, c // 2), (a // 2, b // 2, 0), (a // 2, 0, c // 2), (0, b // 2, c // 2), (a, b // 2, c // 2)]
    pool = base + extra
    s = set(rnd.sample(pool, min(n, len(pool))))
    while len(s) < n:
        s.add((rnd.randint(0, a), rnd.randint(0, b), rnd.randint(0, c)))
    return sorted(s)


FAMILIES = dict(generic=fam_generic, grid=fam_grid, plane=fam_plane, clusters=fam_clusters, sphere=fam_sphere,
                circle=fam_circle, line=fam_line, cube=fam_cube)


def main():
    exe = sys.argv[1]
    seed0 = int(sys.argv[2])
    count = int(sys.argv[3])
    fams = sys.argv[4].split(',') if len(sys.argv) > 4 else sorted(FAMILIES)
    nmin, nmax = (int(v) for v in (sys.argv[5].split('-') if len(sys.argv) > 5 else ('6', '10')))
    kcap = int(sys.argv[6]) if len(sys.argv) > 6 else 5
    tot = dict()
    fails = refus = done = 0
    with tempfile.TemporaryDirectory(dir=os.environ.get('L02_TMP')) as tmp:
        for t in range(count):
            fam = fams[t % len(fams)]
            rnd = random.Random(1000003 * seed0 + t)
            n = rnd.randint(nmin, nmax)
            P = FAMILIES[fam](rnd, n)
            rnd.shuffle(P)
            K = min(kcap, n) if rnd.random() < 0.8 else min(n, 12, kcap + 2)
            err, cnt, code = judge_cloud(exe, P, K, tmp)
            done += 1
            cnt.pop('_stdout', None)
            for key, v in cnt.items():
                tot[key] = tot.get(key, 0) + v
                tot[fam + ':' + key] = tot.get(fam + ':' + key, 0) + v
            if err:
                if code == 2:
                    refus += 1
                else:
                    fails += 1
                print('ECART famille=%s graine=%d t=%d n=%d K=%d : %s\n  P=%s' % (fam, seed0, t, n, K, err, P), flush=True)
            if done % 8 == 0:
                print('PROGRES ' + json.dumps(dict(seed=seed0, clouds=done, fails=fails, refus=refus,
                                                   **{k: v for k, v in tot.items() if ':' not in k}), sort_keys=True), flush=True)
    summary = {k: v for k, v in tot.items() if ':' not in k}
    print(json.dumps(dict(seed=seed0, clouds=done, fails=fails, refus=refus, **summary), sort_keys=True))
    perfam = {}
    for k, v in tot.items():
        if ':' in k:
            f, key = k.split(':')
            perfam.setdefault(f, {})[key] = v
    print(json.dumps(dict(per_family=perfam), sort_keys=True))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
