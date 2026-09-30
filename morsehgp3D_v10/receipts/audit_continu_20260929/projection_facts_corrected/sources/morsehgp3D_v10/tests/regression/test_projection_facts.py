"""Regression (29 septembre 2026) : faits mathematiques de la projection des points depuis la tour, graves sur les
binaires natifs. Registre racine docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md, section V10 ; amendement de
docs/conception/CLUSTER_v2.md par. 3.2 ; audits audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md
(par. 2, 2.1, 2.2, 4.2), pool_head/CONTRE_AUDIT_PROJECTION_MULTIK_20260929.md et
audit_independant_20260929/TOUR_ET_POINTS.md (par. 5).

F1  Recouvrement des couvertures. K = 2, sites alignes 0, 2, 4, rayon 1 : L_2(1) a deux composantes, les points 1
    et 3 ; leurs couvertures discretes {0,2} et {2,4} se recouvrent en 2. L'entree cover donne 2 a un seul
    proprietaire (ordre canonique du catalogue) : c'est une laminarisation particuliere des amas discrets, pas
    leur semantique complete (false_in_general pour l'enonce contraire).
F2  Discontinuite de cover. K = 2, {0, L-1, 2L} puis {0, L+1, 2L}, L = 1000 et 100000 : fusion gauche-milieu
    (L-1)/2 puis L en cover, soit un saut (L+1)/2 pour un deplacement de 2 ; L-1 puis L+1 en core, dans la borne
    2 epsilon (false_in_general pour << cover herite de la stabilite de core >>).
F3  Croisement multi-K. {0,20,22,50,52} : (K=1, r=10) contient {0,20,22}, (K=2, r=15) contient {20,22,50,52} ;
    temoin de l'audit independant {0,1,4,7} : (K=1, r=1/2) contient {0,1}, (K=4, r=6) contient {1,4}. La coupe
    comparable aux deux (K grand, r petit) les raffine toutes deux (false_in_general pour << FULL multi-K est un
    arbre unique sur les points >>).
F4  Borne de stabilite de core (proved_here) : |u_K^X(i,j) - u_K^Y(i,j)| <= 2 epsilon en rayon. Cas d'egalite
    grave : deux sites a distance L, puis chacun ecarte de e (0, 1 puis -e, 1+e a l'echelle entiere) : fusion et
    entrees core L puis L + 2e.

Chaque valeur est lue sur les sorties exactes des binaires : mhgp10_tower --dump (niveaux rationnels num/den),
mhgp10_catalogue --dump (supports et populations des boules), mhgp10_cluster --tree (hierarchie de points que lit
la tete ; ses niveaux double sont exacts sur ces fixtures, ce que la porte verifie en les comparant aux niveaux
rationnels de la tour). Les valeurs attendues ne sont pas recopiees : un oracle rationnel independant les recalcule
depuis les coordonnees (sites alignes : les composantes de L_K(r) sont celles de leur trace sur l'axe, reunion des
intervalles des fenetres de K sites consecutifs). Des mutants des sorties lues doivent etre tues, et un mutant
equivalent (l'autre proprietaire du point conteste de F1) accepte : la porte n'est verte ni par vacuite, ni par
un departage recopie. Python nu (ni numpy ni scipy), sans assert : la porte tient sous python3 -O et tourne sur la
VM G4 (label fast).

  python3 test_projection_facts.py <dossier de build>
  -> 0 conforme ; 1 fait contredit, sortie illisible, binaire en echec ou mutant survivant ; 2 refus avant calcul
     (binaire absent) ; 3 plancher de controles non atteint
"""
import copy
import math
import os
import struct
import subprocess
import sys
import tempfile
from fractions import Fraction

BINARIES = ('mhgp10_catalogue', 'mhgp10_tower', 'mhgp10_cluster')
FLOOR_CHECKS = 133  # controles evalues sur les vraies sorties, compte exact (plancher contre le vert par vacuite)

F1_SITES = [0, 2, 4]
F2_CASES = [(1000, 1), (100000, 1)]  # (L, d) : {0, L - d, 2L} puis {0, L + d, 2L}
F3_CASES = [
    dict(name='continu', xs=[0, 20, 22, 50, 52], lo=(1, Fraction(10)), hi=(2, Fraction(15)),
         a=[0, 20, 22], b=[20, 22, 50, 52]),
    dict(name='independant', xs=[0, 1, 4, 7], lo=(1, Fraction(1, 2)), hi=(4, Fraction(6)),
         a=[0, 1], b=[1, 4]),
]
F4_CASES = [(1, 1), (1000, 1), (1000, 7)]  # (L, e) : {e, e + L} puis {0, L + 2e}


class Fail(Exception):
    """Sortie native absente, illisible ou incoherente."""


def canon(blocks):
    return sorted(sorted(b) for b in blocks)


def refines(fine, coarse):
    return all(any(set(b) <= set(c) for c in coarse) for b in fine)


def root(a):
    """Rayon exact d'un niveau (rayon carre) rationnel ; Fail si ce n'est pas le carre d'un rationnel."""
    if a < 0:
        raise Fail('niveau negatif %s' % a)
    n, d = a.numerator, a.denominator
    rn, rd = math.isqrt(n), math.isqrt(d)
    if rn * rn != n or rd * rd != d:
        raise Fail('niveau %s : pas le carre d un rationnel' % a)
    return Fraction(rn, rd)


# ------------------------------------------------------------------ oracle independant (sites alignes, en rayons)

class Line:
    """Oracle exact pour des sites distincts de l'axe x, en rayons (pas en rayons carres).

    La trace de L_K(r) sur l'axe est la reunion des intervalles J_i(r) = [s_{i+K-1} - r, s_i + r] des fenetres
    W_i de K sites consecutifs ; J_i nait au rayon b_i = (s_{i+K-1} - s_i) / 2, J_i et J_{i+1} se touchent des
    m_i = (s_{i+K} - s_i) / 2. Les sites etant alignes, la projection orthogonale sur l'axe ne s'eloigne d'aucun
    site : chaque composante de L_K(r) dans R^3 rencontre l'axe selon un intervalle (sa trace), composantes et traces
    se correspondent, et la distance d'un site a une composante est celle a sa trace. La boule minimale de sites
    alignes est la boule diametrale de leurs extremes."""

    def __init__(self, xs, k):
        self.s = sorted(xs)
        self.k = k
        self.w = len(self.s) - k + 1
        if self.w < 1 or len(set(self.s)) != len(self.s):
            raise Fail('oracle : sites invalides')
        self.birth = [Fraction(self.s[i + k - 1] - self.s[i], 2) for i in range(self.w)]
        self.link = [Fraction(self.s[i + k] - self.s[i], 2) for i in range(self.w - 1)]

    def d_k(self, x):
        """Rayon d'entree core : distance au K-ieme site le plus proche, x compris."""
        return Fraction(sorted(abs(x - t) for t in self.s)[self.k - 1])

    def core_window(self, x):
        d = self.d_k(x)
        for i in range(self.w):
            if abs(x - self.s[i]) <= d and abs(x - self.s[i + self.k - 1]) <= d:
                return i
        raise Fail('oracle : aucune fenetre pour %d' % x)

    def alpha(self, x):
        """Entree cover : rayon de la plus petite boule fermee contenant x et K - 1 autres sites, et les fenetres
        qui la realisent (plusieurs : ex aequo, que le moteur departage par l'ordre canonique du catalogue)."""
        j = self.s.index(x)
        cands = range(max(0, j - self.k + 1), min(j, self.w - 1) + 1)
        a = min(self.birth[i] for i in cands)
        return a, [i for i in cands if self.birth[i] == a]

    def run(self, i, r):
        lo = hi = i
        while lo > 0 and self.link[lo - 1] <= r:
            lo -= 1
        while hi < self.w - 1 and self.link[hi] <= r:
            hi += 1
        return lo, hi

    def components(self, r):
        return sorted(set(self.run(i, r) for i in range(self.w) if self.birth[i] <= r))

    def trace(self, comp, r):
        lo, hi = comp
        return (min(self.s[i + self.k - 1] for i in range(lo, hi + 1)) - r,
                max(self.s[i] for i in range(lo, hi + 1)) + r)

    def discrete_cover(self, comp, r):
        """Amas discret D_r(C) = {x : d(x, C) <= r}."""
        lo, hi = self.trace(comp, r)
        return [x for x in self.s if lo - r <= x <= hi + r]

    def join(self, i, j):
        return max(self.link[min(i, j):max(i, j)], default=Fraction(0))

    def u_core(self, x, y):
        return max(self.d_k(x), self.d_k(y), self.join(self.core_window(x), self.core_window(y)))

    def u_cover(self, x, y, wx, wy):
        return max(self.alpha(x)[0], self.alpha(y)[0], self.join(wx, wy))

    def core_partition(self, r):
        groups = {}
        for x in self.s:
            if self.d_k(x) <= r:
                groups.setdefault(self.run(self.core_window(x), r), []).append(x)
        return canon(groups.values())

    def cover_partitions(self, r):
        """Toutes les partitions cover admissibles au rayon r : un choix de fenetre par point ex aequo."""
        pts = [x for x in self.s if self.alpha(x)[0] <= r]
        out = [[]]
        for x in pts:
            out = [chosen + [(x, wi)] for chosen in out for wi in self.alpha(x)[1]]
        parts = []
        for chosen in out:
            groups = {}
            for x, wi in chosen:
                groups.setdefault(self.run(wi, r), []).append(x)
            p = canon(groups.values())
            if p not in parts:
                parts.append(p)
        return sorted(parts)


# ------------------------------------------------------------------ lecture des sorties natives

def read_tower(path, xs):
    """mhgp10_tower --dump : par ordre, noeuds (parent, niveau exact) et points x -> (noeud, entree exacte, rang
    cover ou None). Niveaux en rayon carre."""
    orders, cur = {}, None
    try:
        with open(path) as f:
            for line in f:
                t = line.split()
                if not t:
                    continue
                if t[0] == 'order':
                    cur = {'nodes': [], 'points': {}, 'n_nodes': int(t[2])}
                    orders[int(t[1])] = cur
                elif t[0] == 'node':
                    if int(t[1]) != len(cur['nodes']):
                        raise Fail('%s : noeuds hors ordre' % path)
                    cur['nodes'].append((int(t[2]), Fraction(int(t[3]), int(t[4]))))
                elif t[0] == 'point':
                    if t[2] != '0' or t[3] != '0':
                        raise Fail('%s : point hors de l axe' % path)
                    if t[5].startswith('r'):  # entree cover : noeud, rang de noeud, niveau exact num/den
                        cur['points'][int(t[1])] = (int(t[4]), Fraction(int(t[6]), int(t[7])), int(t[5][1:]))
                    else:  # entree core : noeud, D_K(x) entier
                        cur['points'][int(t[1])] = (int(t[4]), Fraction(int(t[5])), None)
                else:
                    raise Fail('%s : ligne inconnue %r' % (path, line))
    except (IndexError, ValueError, TypeError, KeyError, OSError) as e:
        raise Fail('%s illisible : %s' % (path, e))
    if not orders:
        raise Fail('%s : aucun ordre' % path)
    for k, o in orders.items():
        if len(o['nodes']) != o['n_nodes'] or sorted(o['points']) != sorted(xs):
            raise Fail('%s : ordre %d incomplet' % (path, k))
    return orders


def read_catalogue(path):
    """mhgp10_catalogue --dump : rang q p u flags | S* | I | U (coordonnees)."""
    balls = []
    try:
        with open(path) as f:
            for line in f:
                if not line.strip():
                    continue
                head, sup, inner, shell = line.rstrip('\n').split('|')
                rank, q, p, u, _flags = (int(v) for v in head.split())

                def pts(s):
                    return [tuple(int(c) for c in tok.split(',')) for tok in s.split()]
                balls.append(dict(rank=rank, q=q, p=p, u=u, support=pts(sup), I=pts(inner), U=pts(shell)))
    except (ValueError, TypeError, OSError) as e:
        raise Fail('%s illisible : %s' % (path, e))
    if not balls:
        raise Fail('%s : catalogue vide' % path)
    return balls


def read_tree(path, xs):
    """mhgp10_cluster --tree : niveaux (double), noeuds (rang, parent), points (indice d'entree, noeud, rang)."""
    try:
        with open(path) as f:
            lines = [ln.split() for ln in f.read().split('\n') if ln.strip()]
        i = 0

        def section(name):
            nonlocal i
            if lines[i][0] != name:
                raise Fail('%s : section %s attendue' % (path, name))
            i += 1
            return int(lines[i - 1][1])
        nl = section('levels')
        levels = [Fraction(float(t[0])) for t in lines[i:i + nl]]
        i += nl
        nn = section('nodes')
        nodes = [(int(t[0]), int(t[1])) for t in lines[i:i + nn]]
        i += nn
        npts = section('points')
        points = {}
        for t in lines[i:i + npts]:
            points[xs[int(t[0])]] = (int(t[1]), int(t[2]))
        if len(levels) != nl or len(nodes) != nn or len(points) != len(xs) or npts != len(xs):
            raise Fail('%s : arbre incomplet' % path)
    except (IndexError, ValueError, TypeError, OverflowError, OSError) as e:
        raise Fail('%s illisible : %s' % (path, e))
    return dict(levels=levels, nodes=nodes, points=points)


def hier_from_dump(order):
    return dict(parent=[p for p, _ in order['nodes']], level=[lv for _, lv in order['nodes']],
                points={x: (v, e) for x, (v, e, _r) in order['points'].items()})


def hier_from_tree(tree):
    try:
        lv = tree['levels']
        return dict(parent=[p for _, p in tree['nodes']], level=[lv[r] for r, _ in tree['nodes']],
                    points={x: (v, lv[r]) for x, (v, r) in tree['points'].items()})
    except IndexError:
        raise Fail('arbre : rang hors des niveaux')


def ancestors(h, v):
    chain = []
    while v >= 0:
        if v >= len(h['parent']) or v in chain:
            raise Fail('hierarchie : parent invalide')
        chain.append(v)
        v = h['parent'][v]
    return chain


def merge(h, x, y):
    """Hauteur de fusion (rayon carre) de deux points : entrees et plus petit ancetre commun."""
    (vx, ex), (vy, ey) = h['points'][x], h['points'][y]
    ax = ancestors(h, vx)
    for v in ancestors(h, vy):
        if v in ax:
            return max(ex, ey, h['level'][v])
    raise Fail('hierarchie : pas d ancetre commun a %d et %d' % (x, y))


def top(h, v, a):
    chain = ancestors(h, v)
    t = chain[0]
    for u in chain[1:]:
        if h['level'][u] > a:
            break
        t = u
    return t


def cut(h, a):
    """Partition des points entres a la coupe fermee a (rayon carre)."""
    groups = {}
    for x, (v, e) in h['points'].items():
        if e <= a:
            groups.setdefault(top(h, v, a), []).append(x)
    return canon(groups.values())


def alive(h, a):
    return sum(1 for v, lv in enumerate(h['level'])
               if lv <= a and (h['parent'][v] < 0 or h['level'][h['parent'][v]] > a))


class Runner:
    def __init__(self, build, tmp):
        self.build, self.tmp, self.count, self.calls = build, tmp, 0, 0

    def call(self, argv):
        r = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True,
                           timeout=300)
        self.calls += 1
        if r.returncode != 0:
            raise Fail('code %d : %s %s' % (r.returncode, ' '.join(argv), (r.stdout + r.stderr).strip()[-400:]))

    def cloud(self, xs):
        self.count += 1
        path = os.path.join(self.tmp, 'c%02d.u32le' % self.count)
        with open(path, 'wb') as f:
            f.write(b''.join(struct.pack('<3I', x, 0, 0) for x in xs))
        return path

    def tower(self, xs, k, entry):
        src = self.cloud(xs)
        dump = src + '.dump'
        self.call([os.path.join(self.build, 'mhgp10_tower'), src, '--k=%d' % k, '--threads=1', '--entry=' + entry,
                   '--dump=' + dump])
        return read_tower(dump, xs)

    def catalogue(self, xs, k):
        src = self.cloud(xs)
        dump = src + '.cat'
        self.call([os.path.join(self.build, 'mhgp10_catalogue'), src, '--k=%d' % k, '--threads=1', '--dump=' + dump])
        return read_catalogue(dump)

    def trees(self, xs, ks, entry):
        src = self.cloud(xs)
        tree = src + '.tree'
        order = ['--k=%d' % ks[0]] if len(ks) == 1 else ['--k-list=' + ','.join(str(k) for k in ks)]
        self.call([os.path.join(self.build, 'mhgp10_cluster'), src, src + '.lab'] + order +
                  ['--mcs=2', '--threads=1', '--entry=' + entry, '--tree=' + tree])
        if len(ks) == 1:
            return {ks[0]: read_tree(tree, xs)}
        return {k: read_tree(tree + '.k%d' % k, xs) for k in ks}


def collect(rn):
    """Toutes les executions natives ; les juges ne lisent que ce dictionnaire (les mutants en derivent)."""
    d = dict(f1=dict(xs=F1_SITES, cat=rn.catalogue(F1_SITES, 2),
                     tower={e: rn.tower(F1_SITES, 2, e)[2] for e in ('core', 'cover')},
                     tree={e: rn.trees(F1_SITES, [2], e)[2] for e in ('core', 'cover')}),
             f2=[], f3=[], f4=[])
    for L, dl in F2_CASES:
        clouds = {}
        for sign in (-1, 1):
            xs = [0, L + sign * dl, 2 * L]
            clouds[sign] = dict(xs=xs, tower={e: rn.tower(xs, 2, e)[2] for e in ('core', 'cover')},
                                tree={e: rn.trees(xs, [2], e)[2] for e in ('core', 'cover')})
        d['f2'].append(dict(L=L, d=dl, clouds=clouds))
    for c in F3_CASES:
        ks = [c['lo'][0], c['hi'][0]]
        d['f3'].append(dict(c, tower=rn.tower(c['xs'], max(ks), 'core'), trees=rn.trees(c['xs'], ks, 'core')))
    for L, e in F4_CASES:
        side = {}
        for name, xs in (('X', [e, e + L]), ('Y', [0, L + 2 * e])):
            side[name] = dict(xs=xs, tower=rn.tower(xs, 2, 'core')[2], tree=rn.trees(xs, [2], 'core')[2])
        d['f4'].append(dict(L=L, e=e, X=side['X'], Y=side['Y']))
    return d


# ------------------------------------------------------------------ juges des quatre faits

class Judge:
    def __init__(self, log):
        self.checks, self.failures, self.log = 0, [], log

    def ok(self, cond, what):
        self.checks += 1
        if not cond:
            self.failures.append(what)
        return cond

    def say(self, text):
        if self.log:
            print(text)


def judge_f1(d, j):
    xs, k, r = d['xs'], 2, Fraction(1)
    a = r * r
    line = Line(xs, k)
    comps = line.components(r)
    covers = [line.discrete_cover(c, r) for c in comps]
    j.ok(len(comps) == 2 and [line.trace(c, r) for c in comps] == [(1, 1), (3, 3)],
         'F1 oracle : L_2(1) a deux composantes, les points 1 et 3')
    j.ok(covers == [[0, 2], [2, 4]], 'F1 oracle : couvertures {0,2} et {2,4}')
    j.ok(all(line.alpha(x)[0] == r for x in xs) and len(line.alpha(2)[1]) == 2,
         'F1 oracle : alpha_2 = 1 partout, deux boules ex aequo pour le point 2')
    # catalogue natif : les boules du plus bas niveau sont les deux amas discrets au rayon 1
    cat = d['cat']
    r0 = min(b['rank'] for b in cat)
    low = [b for b in cat if b['rank'] == r0]
    pops = canon([p[0] for p in b['I'] + b['U']] for b in low)
    j.ok(pops == covers, 'F1 catalogue : populations du plus bas niveau %s = couvertures %s' % (pops, covers))
    centers = []
    for b in low:
        sup = sorted(p[0] for p in b['support'])
        good = b['q'] == 2 and len(sup) == 2 and b['p'] + b['u'] >= k
        j.ok(good, 'F1 catalogue : boule diametrale de poids >= K')
        if good:
            j.ok(Fraction((sup[1] - sup[0]) ** 2, 4) == a, 'F1 catalogue : niveau du support = 1')
            centers.append(Fraction(sup[0] + sup[1], 2))
    j.ok(sorted((c, c) for c in centers) == [line.trace(c, r) for c in comps],
         'F1 catalogue : centres des deux boules = composantes de l oracle')
    # tour native, entree cover : deux composantes vivantes au rayon 1, un seul proprietaire pour le point 2
    cov = d['tower']['cover']
    hc = hier_from_dump(cov)
    j.ok(alive(hc, a) == len(comps), 'F1 tour : %d composantes vivantes a a = 1' % alive(hc, a))
    for x in xs:
        v, e, rk = cov['points'][x]
        j.ok(rk is not None and rk == r0 + 1 and e == line.alpha(x)[0] ** 2,
             'F1 tour : entree cover de %d au rang %d, niveau alpha^2' % (x, r0 + 1))
    owner = {x: top(hc, hc['points'][x][0], a) for x in xs}
    j.ok(owner[0] != owner[4] and owner[2] in (owner[0], owner[4]),
         'F1 tour : 0 et 4 dans deux composantes, 2 dans l une d elles')
    part = cut(hc, a)
    admissible = line.cover_partitions(r)
    j.ok(len(admissible) == 2 and part in admissible,
         'F1 tour : partition cover %s parmi les admissibles %s' % (part, admissible))
    j.ok(set(covers[0]) & set(covers[1]) == {2} and sum(1 for c in covers if c in part) == 1,
         'F1 : les amas discrets se recouvrent en 2 et la premiere couverture n en garde qu un comme bloc')
    # entree core : personne n'est entre au rayon 1 (D_2 = 2 pour tous), puis un seul bloc au rayon 2
    hk = hier_from_dump(d['tower']['core'])
    j.ok(cut(hk, a) == line.core_partition(r) == [], 'F1 tour : aucun point core entre au rayon 1')
    j.ok(cut(hk, Fraction(4)) == line.core_partition(Fraction(2)), 'F1 tour : partition core au rayon 2 = oracle')
    for e in ('core', 'cover'):
        ht, hd = hier_from_tree(d['tree'][e]), hier_from_dump(d['tower'][e])
        j.ok(all(cut(ht, lv) == cut(hd, lv) for lv in (a, Fraction(4))), 'F1 %s : arbre de la tete = tour' % e)
    j.say('F1 K=2 sites %s, rayon 1 : amas discrets %s (catalogue : %s), cover %s, core %s'
          % (xs, covers, pops, part, cut(hk, a)))


def judge_f2(cases, j):
    for c in cases:
        L, dl = c['L'], c['d']
        eps = 2 * dl  # deplacement du point du milieu, les deux autres fixes
        u = {}
        for sign, cl in sorted(c['clouds'].items()):
            xs = cl['xs']
            line = Line(xs, 2)
            for e in ('core', 'cover'):
                hd, ht = hier_from_dump(cl['tower'][e]), hier_from_tree(cl['tree'][e])
                for p, q in ((0, 1), (1, 2), (0, 2)):
                    x, y = xs[p], xs[q]
                    got = merge(hd, x, y)
                    j.ok(merge(ht, x, y) == got, 'F2 %s %s : fusion de la tete = tour' % (xs, e))
                    if e == 'core':
                        want = line.u_core(x, y)
                    else:
                        ax, ay = line.alpha(x), line.alpha(y)
                        j.ok(len(ax[1]) == 1 and len(ay[1]) == 1, 'F2 oracle : aucun ex aequo de couverture')
                        want = line.u_cover(x, y, ax[1][0], ay[1][0])
                    j.ok(root(got) == want, 'F2 %s %s (%d,%d) : fusion %s, oracle %s' % (xs, e, x, y, root(got), want))
                    u[(sign, e, p, q)] = root(got)
        jump = u[(1, 'cover', 0, 1)] - u[(-1, 'cover', 0, 1)]
        core = u[(1, 'core', 0, 1)] - u[(-1, 'core', 0, 1)]
        j.ok(u[(-1, 'cover', 0, 1)] == Fraction(L - dl, 2) and u[(1, 'cover', 0, 1)] == L,
             'F2 L=%d : fusion cover gauche-milieu (L-d)/2 puis L' % L)
        j.ok(jump == Fraction(L + dl, 2) and jump > 2 * eps,
             'F2 L=%d : saut cover (L+d)/2 = %s hors de la borne 2 epsilon = %d' % (L, jump, 2 * eps))
        j.ok(core == 2 * dl and u[(-1, 'core', 0, 1)] == L - dl, 'F2 L=%d : core L-d puis L+d' % L)
        j.say('F2 K=2 {0, %d-+%d, %d} : fusion gauche-milieu cover %s -> %s (saut %s), core %s -> %s, epsilon %d'
              % (L, dl, 2 * L, u[(-1, 'cover', 0, 1)], u[(1, 'cover', 0, 1)], jump, u[(-1, 'core', 0, 1)],
                 u[(1, 'core', 0, 1)], eps))


def judge_f3(cases, j):
    for c in cases:
        (k1, r1), (k2, r2) = c['lo'], c['hi']
        parts = {}
        for k, r in ((k1, r1), (k2, r2), (k2, r1)):
            hd, ht = hier_from_dump(c['tower'][k]), hier_from_tree(c['trees'][k])
            part = cut(hd, r * r)
            j.ok(part == Line(c['xs'], k).core_partition(r),
                 'F3 %s : Pi_%d(r=%s) = %s, oracle' % (c['name'], k, r, part))
            j.ok(cut(ht, r * r) == part, 'F3 %s : arbre de la tete = tour (K=%d)' % (c['name'], k))
            parts[(k, r)] = part
        A, B = sorted(c['a']), sorted(c['b'])
        present = A in parts[(k1, r1)] and B in parts[(k2, r2)]
        crossing = bool(set(A) & set(B)) and bool(set(A) - set(B)) and bool(set(B) - set(A))
        j.ok(present, 'F3 %s : blocs %s et %s presents' % (c['name'], A, B))
        j.ok(crossing, 'F3 %s : les deux blocs se croisent' % c['name'])
        j.ok(k1 < k2 and r1 < r2, 'F3 %s : parametres incomparables' % c['name'])
        j.ok(refines(parts[(k2, r1)], parts[(k1, r1)]) and refines(parts[(k2, r1)], parts[(k2, r2)]),
             'F3 %s : la coupe comparable (K=%d, r=%s) raffine les deux' % (c['name'], k2, r1))
        j.say('F3 %s %s : Pi_%d(r=%s) = %s ; Pi_%d(r=%s) = %s ; blocs %s et %s %s'
              % (c['name'], c['xs'], k1, r1, parts[(k1, r1)], k2, r2, parts[(k2, r2)], A, B,
                 'presents et croises' if present and crossing else 'ABSENTS ou non croises'))


def judge_f4(cases, j):
    for c in cases:
        L, e = c['L'], c['e']
        vals = {}
        for side in ('X', 'Y'):
            cl = c[side]
            xs = cl['xs']
            line = Line(xs, 2)
            hd, ht = hier_from_dump(cl['tower']), hier_from_tree(cl['tree'])
            u = merge(hd, xs[0], xs[1])
            j.ok(merge(ht, xs[0], xs[1]) == u, 'F4 %s : fusion de la tete = tour' % xs)
            j.ok(root(u) == line.u_core(xs[0], xs[1]), 'F4 %s : fusion core = oracle' % xs)
            entry = [root(hd['points'][x][1]) for x in xs]
            j.ok(entry == [line.d_k(x) for x in xs], 'F4 %s : entrees core = oracle' % xs)
            vals[side] = (root(u), entry)
        eps = max(abs(x - y) for x, y in zip(c['X']['xs'], c['Y']['xs']))
        j.ok(eps == e and vals['X'][0] == L, 'F4 L=%d e=%d : deplacement e, fusion L' % (L, e))
        j.ok(vals['Y'][0] - vals['X'][0] == 2 * eps, 'F4 L=%d e=%d : fusion core L puis L + 2e (egalite)' % (L, e))
        j.ok(all(y - x == 2 * eps for x, y in zip(vals['X'][1], vals['Y'][1])),
             'F4 L=%d e=%d : entrees core L puis L + 2e (egalite)' % (L, e))
        j.say('F4 K=2 %s -> %s (epsilon %d) : fusion core %s -> %s, ecart %s pour 2 epsilon = %d'
              % (c['X']['xs'], c['Y']['xs'], eps, vals['X'][0], vals['Y'][0], vals['Y'][0] - vals['X'][0], 2 * eps))


FACTS = (('F1', 'f1', judge_f1), ('F2', 'f2', judge_f2), ('F3', 'f3', judge_f3), ('F4', 'f4', judge_f4))


def run_fact(judge, data, log):
    """Une exception d'un juge (sortie incoherente, mutant qui casse la structure) est un echec du fait, nomme."""
    j = Judge(log)
    try:
        judge(data, j)
    except Exception as e:  # toute exception d'un juge est un echec, jamais un succes
        j.failures.append('exception %s : %s' % (type(e).__name__, e))
    return j


# ------------------------------------------------------------------ mutants des sorties lues

def mut_cover_as_core(d):
    d['f1']['tower']['cover'] = d['f1']['tower']['core']
    d['f1']['tree']['cover'] = d['f1']['tree']['core']
    for c in d['f2']:
        for cl in c['clouds'].values():
            cl['tower']['cover'] = cl['tower']['core']
            cl['tree']['cover'] = cl['tree']['core']


def mut_forget_entry(d):
    """Hauteurs de fusion sans dates d'entree : chaque point date de la naissance de son noeud."""
    for c in d['f4']:
        for side in ('X', 'Y'):
            o, t = c[side]['tower'], c[side]['tree']
            for x, (v, _e, rk) in list(o['points'].items()):
                o['points'][x] = (v, o['nodes'][v][1], rk)
            for x, (v, _r) in list(t['points'].items()):
                t['points'][x] = (v, t['nodes'][v][0])


def mut_levels_as_radii(d):
    """Niveaux lus comme des rayons (tous les niveaux eleves au carre)."""
    for c in d['f4']:
        for side in ('X', 'Y'):
            o, t = c[side]['tower'], c[side]['tree']
            o['nodes'] = [(p, lv * lv) for p, lv in o['nodes']]
            o['points'] = {x: (v, e * e, rk) for x, (v, e, rk) in o['points'].items()}
            t['levels'] = [lv * lv for lv in t['levels']]


def mut_orders_confused(d):
    """Coupe d'ordre haut lue dans l'ordre bas."""
    for c in d['f3']:
        k1, k2 = c['lo'][0], c['hi'][0]
        c['tower'][k2] = c['tower'][k1]
        c['trees'][k2] = c['trees'][k1]


def mut_deferred_owner(d):
    """Le point conteste 2 n'est donne a personne au rayon 1 (ancrage differe a la racine)."""
    o, t = d['f1']['tower']['cover'], d['f1']['tree']['cover']
    root_o = next(v for v, (p, _lv) in enumerate(o['nodes']) if p < 0)
    o['points'][2] = (root_o, o['nodes'][root_o][1], o['points'][2][2])
    root_t = next(v for v, (_r, p) in enumerate(t['nodes']) if p < 0)
    t['points'][2] = (root_t, t['nodes'][root_t][0])


def mut_other_owner(d):
    """Equivalent : le point conteste 2 donne a l'autre proprietaire (celui de 4). Doit etre accepte."""
    o, t = d['f1']['tower']['cover'], d['f1']['tree']['cover']
    o['points'][2] = (o['points'][4][0],) + tuple(o['points'][2][1:])
    t['points'][2] = (t['points'][4][0], t['points'][2][1])


KILLERS = (('cover_lu_comme_core', mut_cover_as_core, ('F1', 'F2')),
           ('dates_d_entree_oubliees', mut_forget_entry, ('F4',)),
           ('niveaux_lus_comme_rayons', mut_levels_as_radii, ('F4',)),
           ('ordres_confondus', mut_orders_confused, ('F3',)),
           ('point_conteste_differe', mut_deferred_owner, ('F1',)))
EQUIVALENTS = (('autre_proprietaire', mut_other_owner, ('F1',)),)


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    build = sys.argv[1]
    missing = [b for b in BINARIES if not os.access(os.path.join(build, b), os.X_OK)]
    if missing:
        print('REFUS binaires absents : %s' % ' '.join(missing))
        return 2
    with tempfile.TemporaryDirectory() as tmp:
        rn = Runner(build, tmp)
        try:
            data = collect(rn)
        except Fail as e:
            print('ECHEC execution native : %s' % e)
            return 1
        except subprocess.TimeoutExpired as e:
            print('ECHEC delai depasse : %s' % e)
            return 1
        except Exception as e:  # sortie native inattendue : echec nomme, jamais un succes
            print('ECHEC execution native (%s) : %s' % (type(e).__name__, e))
            return 1
    failures, checks, facts_ok = [], 0, 0
    for name, key, judge in FACTS:
        j = run_fact(judge, data[key], True)
        checks += j.checks
        facts_ok += not j.failures
        failures += ['%s : %s' % (name, f) for f in j.failures]
    by_name = dict((name, (key, judge)) for name, key, judge in FACTS)
    killed = 0
    for mname, mutate, targets in KILLERS:
        d = copy.deepcopy(data)
        mutate(d)
        dead = [t for t in targets if run_fact(by_name[t][1], d[by_name[t][0]], False).failures]
        ok = len(dead) == len(targets)
        killed += ok
        print('mutant %s : %s' % (mname, 'tue par %s' % ','.join(dead) if ok else 'SURVIVANT (%s)' % ','.join(targets)))
        if not ok:
            failures.append('mutant %s survivant' % mname)
    accepted = 0
    for mname, mutate, targets in EQUIVALENTS:
        d = copy.deepcopy(data)
        mutate(d)
        bad = [f for t in targets for f in run_fact(by_name[t][1], d[by_name[t][0]], False).failures]
        accepted += not bad
        print('equivalent %s : %s' % (mname, 'accepte' if not bad else 'REJETE : %s' % bad[0]))
        if bad:
            failures.append('equivalent %s rejete' % mname)
    print('executions natives %d, controles %d (plancher %d)' % (rn.calls, checks, FLOOR_CHECKS))
    for f in failures:
        print('ECHEC', f)
    if failures:
        return 1
    if checks < FLOOR_CHECKS:
        print('PLANCHER %d controles < %d' % (checks, FLOOR_CHECKS))
        return 3
    print('projection_facts_ok faits=%d/%d mutants_tues=%d/%d equivalents_acceptes=%d/%d'
          % (facts_ok, len(FACTS), killed, len(KILLERS), accepted, len(EQUIVALENTS)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
