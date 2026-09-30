"""Porte T2 du generateur : catalogue C++ == oracle brut exact (reference/hgp10_ref.py), petits nuages.

Pour chaque nuage (generiques et grilles degenerees : cospheriques, coplanaires, alignes ; nuages a multiplicites ;
fixtures gravees aux extremes u18) et chaque K :
  - meme multiensemble de boules (q_min, p, I, U) (une boule critique est determinee par (I, U)) ;
  - S* est un support valide de cardinal q_min inclus dans U, et c'est le support CANONIQUE : le plus petit, dans
    l'ordre lexicographique des rangs de Morton, des supports valides de cardinal q_min (SPEC_V10 § 3), imprime
    dans l'ordre de Morton ;
  - niveau exact publie (dump --dump-levels : cat.level[rang]) == rayon carre de la reference (Fraction) ;
  - rang == rang dense, a partir de 0, des niveaux exacts distincts publies ; stdout : balls et levels ;
  - poids p (interieur) et u (coquille) avec multiplicites, drapeaux bit0 coquille etendue (|U| > q_min) et bit1
    coquille ponderee ; admission SPEC_V10 § 3 : p + q_min <= K + 1, ou p <= K - 1 si la coquille est ponderee.
Audit continu du 29 sept. 2026 (catalogue/AUDIT_CATALOGUE_J2_J2C § 3 P2) : S* minimal, niveaux, rang dense et entrees
ponderees n'etaient pas juges. Mutants (--inject=NOM) : un mutant grave est applique au dump de sa fixture ; le juge
doit accepter la fixture puis rejeter le mutant (code 4). Sans --inject, la porte rejoue aussi les mutants
systematiques de ses fixtures (support non canonique, niveau, rangs, drapeaux, poids, boule retiree) : tous tues.
Usage : python3 test_catalogue_oracle.py BUILD_DIR [nuages] [--inject=NOM]
Codes : 0 conforme, 1 desaccord, 2 mutant inconnu, 3 plancher (couverture, mutants non tues), 4 mutant tue
(--inject seulement ; un mutant qui survit rend 0).
Python nu (aucune dependance), aucun assert.
"""
import copy
import json
import os
import random
import subprocess
import sys
import tempfile
from fractions import Fraction
from itertools import combinations, product

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import hgp10_ref as R  # noqa: E402


def parse_dump(path):
    """Dump de mhgp10_catalogue : rang q p u flags [num den] | S* | I | U (le niveau n'y est qu'avec --dump-levels)."""
    balls = []
    for line in open(path):
        head, sup, inner, shell = line.rstrip('\n').split('|')
        h = [int(t) for t in head.split()]
        rank, q, p, u, flags = h[:5]
        level = None  # absent (dump par defaut) ; False : champ illisible
        if len(h) != 5:
            level = Fraction(h[5], h[6]) if len(h) == 7 and h[6] > 0 else False
        conv = lambda s: [tuple(int(v) for v in t.split(',')) for t in s.split()]  # noqa: E731
        balls.append(dict(rank=rank, q=q, p=p, u=u, flags=flags, level=level, sup=conv(sup), I=conv(inner),
                          U=conv(shell)))
    return balls


def is_support(S, center, radius2):
    if not R.affinely_independent(S):
        return False
    cc = R.circumcenter(S)
    if cc is None:
        return False
    c, lam = cc
    return c == center and all(l > 0 for l in lam) and R.d2(c, S[0]) == radius2


def morton(p):
    """Cle de Morton du produit (src/cloud/cloud.cpp, morton3) : x au bit 0, y au bit 1, z au bit 2 de chaque niveau."""
    return sum(((p[a] >> b) & 1) << (3 * b + a) for a in range(3) for b in range(21))


def canonical_support(U, q, center, radius2):
    """S* de reference : parmi les parties de U de cardinal q qui sont des supports valides, la plus petite dans
    l'ordre lexicographique des cles de Morton (ordre des sites du produit), triee par Morton. Rend (S*, nombre)."""
    cands = [tuple(sorted(S, key=morton)) for S in combinations(sorted(U), q) if is_support(list(S), center, radius2)]
    if not cands:
        return None, 0
    return min(cands, key=lambda S: [morton(p) for p in S]), len(cands)


class Ref:
    """Spheres critiques d'un nuage de SITES (positions distinctes) : R.critical_balls, calcule une fois."""

    def __init__(self, sites):
        self.sites = list(sites)
        self.crit = R.critical_balls(self.sites)


def expected(ref, weights, K):
    """Catalogue attendu, q_min >= 2 (rayon nul : table des sites, pas le catalogue). Poids 1 partout : exactement
    R.catalogue (p + q_min <= min(K + 1, n)). Multiplicites : p = poids de I, u = poids de U ; coquille ponderee
    (un site de U de poids > 1) admise si p <= K - 1 (SPEC_V10 § 3, generator.cpp judge)."""
    total = sum(weights)
    out = []
    for b in ref.crit:
        if b.qmin < 2:
            continue
        p = sum(weights[i] for i in b.I)
        u = sum(weights[i] for i in b.U)
        wsh = any(weights[i] > 1 for i in b.U)
        if (p <= K - 1) if wsh else (p + b.qmin <= min(K + 1, total)):
            out.append((b, p, u, wsh))
    return out


def new_counters():
    return dict(balls=0, levels=0, canonical=0, canonical_choice=0, weighted=0, extended=0, dense=0)


def judge(ref, weights, K, got, js, cnt=None):
    """Juge un dump parse (avec niveaux) et la ligne JSON du binaire. Rend None ou le premier ecart."""
    cnt = cnt if cnt is not None else new_counters()
    P = ref.sites
    want = expected(ref, weights, K)
    key = lambda q, p, I, U: (q, p, frozenset(I), frozenset(U))  # noqa: E731
    W = {}
    for b, p, u, wsh in want:
        W[key(b.qmin, p, [P[i] for i in b.I], [P[i] for i in b.U])] = (b, u, wsh)
    G = {}
    for b in got:
        k = key(b['q'], b['p'], b['I'], b['U'])
        if k in G:
            return 'boule en double %s' % (k,)
        G[k] = b
    if set(W) != set(G):
        miss = [k for k in W if k not in G][:3]
        extra = [k for k in G if k not in W][:3]
        return 'manquantes %d en trop %d ex %s %s' % (len(set(W) - set(G)), len(set(G) - set(W)), miss, extra)
    dense = {lv: i for i, lv in enumerate(sorted({b.level for b, _u, _w in W.values()}))}
    if js.get('balls') != len(got) or js.get('levels') != len(dense):
        return 'stdout : balls %s levels %s, attendus %d et %d' % (js.get('balls'), js.get('levels'), len(got),
                                                                   len(dense))
    prev = None
    for b in sorted(got, key=lambda t: t['rank']):
        ref_b, u, wsh = W[key(b['q'], b['p'], b['I'], b['U'])]
        if len(b['sup']) != b['q'] or not set(b['sup']) <= set(b['U']):
            return 'support hors coquille'
        if not is_support(b['sup'], ref_b.center, ref_b.level):
            return 'support invalide'
        star, ncand = canonical_support(b['U'], b['q'], ref_b.center, ref_b.level)
        if tuple(b['sup']) != star:
            return 'support non canonique %s, attendu %s (ordre de Morton)' % (b['sup'], star)
        if b['u'] != u:
            return 'poids de coquille %d, attendu %d' % (b['u'], u)
        if b['flags'] != (1 if len(b['U']) > b['q'] else 0) | (2 if wsh else 0):
            return 'drapeaux %d (|U| = %d, q_min = %d, coquille ponderee %s)' % (b['flags'], len(b['U']), b['q'], wsh)
        if b['level'] is None:
            return 'niveau absent du dump (--dump-levels)'
        if b['level'] is False or b['level'] != ref_b.level:
            return 'niveau %s, attendu %s' % (b['level'], ref_b.level)
        if b['rank'] != dense[ref_b.level]:
            return 'rang %d, rang dense attendu %d' % (b['rank'], dense[ref_b.level])
        if prev is not None:
            if (prev[0] == b['rank']) != (prev[1] == ref_b.level) or prev[1] > ref_b.level:
                return 'rangs incoherents'
        prev = (b['rank'], ref_b.level)
        cnt['balls'] += 1
        cnt['levels'] += 1
        cnt['dense'] += 1
        cnt['canonical'] += 1
        cnt['canonical_choice'] += ncand >= 2
        cnt['weighted'] += wsh
        cnt['extended'] += len(b['U']) > b['q']
    return None


def run_catalogue(exe, pts, K, tmp, threads=2, leaf=0):
    """pts : liste des points d'entree (doublons = multiplicites). Rend (ecart, dump parse, JSON)."""
    src = os.path.join(tmp, 'in.u32le')
    with open(src, 'wb') as f:
        for p in pts:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    dump = os.path.join(tmp, 'dump.txt')
    argv = [exe, src, '--k=%d' % K, '--threads=%d' % threads, '--dump=' + dump, '--dump-levels']
    if leaf:
        argv.append('--leaf=%d' % leaf)
    r = subprocess.run(argv, capture_output=True, text=True)
    if r.returncode != 0:
        return 'refus %s' % r.stdout.strip(), None, None
    try:
        js = json.loads(r.stdout.strip().splitlines()[-1])
    except (ValueError, IndexError):
        return 'stdout illisible', None, None
    return None, parse_dump(dump), js


def sites_of(pts):
    """Positions distinctes (ordre de premiere apparition) et multiplicites."""
    sites, weights, at = [], [], {}
    for p in pts:
        if p in at:
            weights[at[p]] += 1
        else:
            at[p] = len(sites)
            sites.append(p)
            weights.append(1)
    return sites, weights


def check(exe, P, K, tmp, ref=None, cnt=None, threads=2, leaf=0):
    """P : points d'entree (doublons permis). Rend None ou l'ecart."""
    sites, weights = sites_of(P)
    ref = ref if ref is not None else Ref(sites)
    if ref.sites != sites:
        return 'reference d\'un autre nuage'
    err, got, js = run_catalogue(exe, P, K, tmp, threads, leaf)
    if err:
        return err
    return judge(ref, weights, K, got, js, cnt)


def translation_check(exe, rnd, tmp):
    """Regression du 28 sept. : un nuage compact loin de l'origine (racine bien plus grande que les donnees)
    etait refuse (wide_leaf). Le catalogue doit etre le meme, a translation pres."""
    pts = set()
    while len(pts) < 3000:
        pts.add(tuple(rnd.randint(0, 20000) for _ in range(3)))
    P = sorted(pts)
    dumps = []
    for off in (0, 200000):
        src = os.path.join(tmp, 'tr.u32le')
        with open(src, 'wb') as f:
            for p in P:
                for v in p:
                    f.write(int(v + off).to_bytes(4, 'little'))
        dump = os.path.join(tmp, 'tr%d.txt' % off)
        r = subprocess.run([exe, src, '--k=5', '--threads=2', '--dump=' + dump], capture_output=True, text=True)
        if r.returncode != 0:
            return 'translation off=%d refus %s' % (off, r.stdout.strip())
        def shift(line, d):
            head, rest = line.split('|', 1)
            parts = []
            for seg in rest.split('|'):
                pts_ = sorted(tuple(int(c) - d for c in t.split(',')) for t in seg.split())
                parts.append(' '.join(','.join(str(c) for c in t) for t in pts_))
            return head + '|' + '|'.join(parts)
        # l'indice de site est le rang de Morton, qui depend de la translation : forme canonique triee
        dumps.append(sorted(shift(l.rstrip('\n'), off) for l in open(dump)))
    return None if dumps[0] == dumps[1] and len(dumps[0]) > 1000 else 'translation : catalogues differents'


def clouds(count, rnd):
    for t in range(count):
        n = rnd.randint(5, 22)
        kind = t % 4
        pts = set()
        while len(pts) < n:
            if kind == 0:
                pts.add(tuple(rnd.randint(0, 1000) for _ in range(3)))
            elif kind == 1:
                pts.add(tuple(rnd.randint(0, 3) for _ in range(3)))
            elif kind == 2:
                pts.add((rnd.randint(0, 6), rnd.randint(0, 6), 0))  # coplanaire
            else:
                pts.add(tuple(rnd.choice((0, 2, 4)) + rnd.randint(0, 1) for _ in range(3)))
        P = sorted(pts)
        rnd.shuffle(P)
        yield P


def weighted(P, rnd):
    """Multiplicites 1, 1, 2, 3 ou 5 par position ; doublons melanges dans l'entree."""
    pts = [p for p in P for _ in range(rnd.choice((1, 1, 2, 3, 5)))]
    rnd.shuffle(pts)
    return pts


E = 262143  # plus grande coordonnee u18
SQUARE = [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]
OCTA_W = [(10 + d if a == 0 else 10, 10 + d if a == 1 else 10, 10 + d if a == 2 else 10) for a in range(3)
          for d in (-2, 2)] + [(10, 10, 10)]
# fixtures gravees (audit continu, catalogue/check_r1_interrupted.py) : (nom, positions, multiplicites)
FIXTURES = [
    ('pair_boundary', [(0, 0, 0), (E, E, E)], [1, 1]),
    ('pair_weighted', [(12, 12, 12), (14, 12, 12)], [3, 1]),
    ('square_top', [(0, 0, E), (2, 0, E), (0, 2, E), (2, 2, E)], [1] * 4),
    ('octa_weighted', OCTA_W, [3, 1, 2, 1, 4, 1, 2]),
    ('cube_max', list(product((0, E), repeat=3)), [1] * 8),
    ('right_triangle', [(0, 0, 0), (3, 0, 0), (0, 4, 0)], [1] * 3),
    ('square', SQUARE + [(7, 7, 1)], [1] * 5),
    ('square_weighted', SQUARE + [(1, 1, 5)], [1, 2, 1, 1, 3]),
]


def expand(sites, weights):
    return [p for p, w in zip(sites, weights) for _ in range(w)]


# ---------------------------------------------------------------- mutants

def ball_where(got, pred):
    return next(i for i, b in enumerate(got) if pred(b))


def mut_support_not_canonical(got):
    """Carre (4 sites cocycliques) : S* = l'autre diagonale, support valide de meme cardinal, non canonique."""
    i = ball_where(got, lambda b: len(b['U']) == 4)
    got[i]['sup'] = [(2, 0, 0), (0, 2, 0)]


def mut_level_value(got):
    """Niveau publie de la boule du carre (rayon carre 2) double, rang inchange."""
    got[ball_where(got, lambda b: len(b['U']) == 4)]['level'] *= 2


def mut_rank_offset(got):
    """Tous les rangs decales de 1 : ordre conserve, mais pas dense a partir de 0."""
    for b in got:
        b['rank'] += 1


def mut_rank_gap(got):
    """Trou dans les rangs : les rangs >= 1 augmentent de 1."""
    for b in got:
        b['rank'] += b['rank'] >= 1


def mut_flag_extended(got):
    got[ball_where(got, lambda b: len(b['U']) == 4)]['flags'] ^= 1


def mut_flag_weighted(got):
    got[ball_where(got, lambda b: b['flags'] & 2)]['flags'] ^= 2


def mut_weight_shell(got):
    got[ball_where(got, lambda b: b['flags'] & 2)]['u'] += 1


def mut_weighted_admission(got):
    """Octaedre pondere, K = 3 : retire une boule admise par la seule regle ponderee (p <= K - 1, p + q_min > K + 1)."""
    del got[ball_where(got, lambda b: b['flags'] & 2 and b['p'] + b['q'] > 3 + 1)]


INJECT = {
    'support_not_canonical': ('square', 3, mut_support_not_canonical),
    'level_value': ('square', 3, mut_level_value),
    'rank_offset': ('square', 3, mut_rank_offset),
    'rank_gap': ('square', 3, mut_rank_gap),
    'flag_extended': ('square', 3, mut_flag_extended),
    'flag_weighted': ('square_weighted', 3, mut_flag_weighted),
    'weight_shell': ('square_weighted', 3, mut_weight_shell),
    'weighted_admission': ('octa_weighted', 3, mut_weighted_admission),
}
SYSTEMATIC = [('square', 3), ('square_weighted', 3), ('octa_weighted', 3), ('octa_weighted', 5), ('cube_max', 3),
              ('right_triangle', 2)]


def systematic_mutants(ref, got):
    """Pour chaque boule : chaque autre support valide de cardinal q_min, un autre niveau (le suivant ou le double),
    chaque drapeau inverse, u + 1, la boule retiree ; puis rangs decales et trous a chaque rang >= 1."""
    out = []
    levels = sorted({b['level'] for b in got})
    for i, b in enumerate(got):
        for S in combinations(sorted(b['U']), b['q']):
            S = tuple(sorted(S, key=morton))
            if S != tuple(b['sup']) and is_support(list(S), R.circumcenter(list(b['sup']))[0], b['level']):
                out.append(('support', i, S))
        nxt = [lv for lv in levels if lv > b['level']]
        out.append(('niveau', i, nxt[0] if nxt else 2 * b['level']))
        out.append(('drapeau0', i, None))
        out.append(('drapeau1', i, None))
        out.append(('poids_u', i, None))
        out.append(('retrait', i, None))
    out.append(('rangs+1', None, None))
    for r in range(1, len(levels)):
        out.append(('trou', r, None))
    return out


def apply_mutant(got, m):
    kind, i, arg = m
    g = copy.deepcopy(got)
    if kind == 'support':
        g[i]['sup'] = list(arg)
    elif kind == 'niveau':
        g[i]['level'] = arg
    elif kind == 'drapeau0':
        g[i]['flags'] ^= 1
    elif kind == 'drapeau1':
        g[i]['flags'] ^= 2
    elif kind == 'poids_u':
        g[i]['u'] += 1
    elif kind == 'retrait':
        del g[i]
    elif kind == 'rangs+1':
        for b in g:
            b['rank'] += 1
    else:
        for b in g:
            b['rank'] += b['rank'] >= i
    return g


def fixture(name):
    return next((s, w) for n, s, w in FIXTURES if n == name)


def run_systematic(exe, tmp):
    killed = total = 0
    kinds = {}
    for name, K in SYSTEMATIC:
        sites, weights = fixture(name)
        ref = Ref(sites)
        err, got, js = run_catalogue(exe, expand(sites, weights), K, tmp)
        if err or judge(ref, weights, K, got, js) is not None:
            return None, 'fixture %s K=%d non conforme : %s' % (name, K, err or judge(ref, weights, K, got, js))
        for m in systematic_mutants(ref, got):
            g = apply_mutant(got, m)
            jj = dict(js, balls=len(g))
            total += 1
            dead = judge(ref, weights, K, g, jj) is not None
            killed += dead
            d = kinds.setdefault(m[0], [0, 0])
            d[0] += 1
            d[1] += dead
    return (killed, total, kinds), None


def inject(exe, name):
    if name not in INJECT:
        print('mutant inconnu %s (connus : %s)' % (name, ', '.join(sorted(INJECT))))
        return 2
    fx, K, mutate = INJECT[name]
    sites, weights = fixture(fx)
    ref = Ref(sites)
    with tempfile.TemporaryDirectory() as tmp:
        err, got, js = run_catalogue(exe, expand(sites, weights), K, tmp)
    if err:
        print('fixture %s : %s' % (fx, err))
        return 1
    base = judge(ref, weights, K, got, js)
    if base is not None:
        print('fixture %s non conforme : %s' % (fx, base))
        return 1
    g = copy.deepcopy(got)
    mutate(g)
    err = judge(ref, weights, K, g, dict(js, balls=len(g)))
    if err is None:
        print('mutant %s SURVIT' % name)
        return 0
    print('mutant %s tue : %s' % (name, err))
    print('mutant_killed %s' % name)
    return 4


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--inject=')]
    exe = os.path.join(args[0], 'mhgp10_catalogue')
    for a in sys.argv[1:]:
        if a.startswith('--inject='):
            return inject(exe, a[len('--inject='):])
    count = int(args[1]) if len(args) > 1 else 40
    rnd = random.Random(20260928)
    checks = fails = 0
    cnt = new_counters()
    wchecks = 0

    def report(tag, err):
        nonlocal fails
        if err:
            fails += 1
            if fails <= 5:
                print('ECART %s : %s' % (tag, err))
    with tempfile.TemporaryDirectory() as tmp:
        for P in clouds(count, rnd):
            ref = Ref(P)
            for K in (1, 2, 3, 5):
                checks += 1
                report('K=%d n=%d %s' % (K, len(P), P), check(exe, P, K, tmp, ref, cnt))
        checks += 1
        report('translation', translation_check(exe, rnd, tmp))
        # multiplicites (graine propre : la suite des nuages ci-dessus est inchangee)
        wrnd = random.Random(2026092901)
        for P in clouds(max(4, count // 3), wrnd):
            pts = weighted(P[:12], wrnd)
            sites, weights = sites_of(pts)
            ref = Ref(sites)
            for K in (1, 2, 3, 5):
                checks += 1
                wchecks += 1
                report('ponderee K=%d n=%d %s' % (K, len(pts), pts), check(exe, pts, K, tmp, ref, cnt))
        # fixtures gravees : K jusqu'a 12, feuilles et fils varies (sortie independante de ces parametres)
        for name, sites, weights in FIXTURES:
            ref = Ref(sites)
            for K in (1, 2, 3, 5, 10, 12):
                for leaf, threads in ((0, 1), (max(8, K + 1), 4), (256, 1)):
                    checks += 1
                    report('%s K=%d feuille=%d fils=%d' % (name, K, leaf, threads),
                           check(exe, expand(sites, weights), K, tmp, ref, cnt, threads, leaf))
        res, err = run_systematic(exe, tmp)
        report('mutants', err)
    print('catalogue_oracle_checks %d fails %d balls %d' % (checks, fails, cnt['balls']))
    print('record_checks levels %d dense_ranks %d canonical %d canonical_with_choice %d weighted %d extended %d '
          'weighted_calls %d' % (cnt['levels'], cnt['dense'], cnt['canonical'], cnt['canonical_choice'],
                                 cnt['weighted'], cnt['extended'], wchecks))
    if res is not None:
        kinds = ' '.join('%s=%d/%d' % (k, v[1], v[0]) for k, v in sorted(res[2].items()))
        print('mutants %d killed %d %s' % (res[1], res[0], kinds))
    if fails:
        return 1
    if res is None or res[0] != res[1]:
        print('PLANCHER : mutant du catalogue survivant')
        return 3
    # planchers contre le vert par vacuite (campagne par defaut, 40 nuages : 18 140 boules jugees, dont 619 a
    # support canonique choisi parmi plusieurs, 2 937 a coquille ponderee, 4 212 a coquille etendue ; 52 appels
    # ponderes ; 571 mutants systematiques)
    cnt['weighted_calls'], cnt['mutants'] = wchecks, res[1]
    floors = dict(balls=1000, levels=1000, canonical_choice=100, weighted=500, extended=500, weighted_calls=16,
                  mutants=500)
    low = [k for k, v in floors.items() if cnt[k] < v]
    if checks < 4 * count or low:
        print('PLANCHER : %s' % ', '.join(['checks %d' % checks] +
                                          ['%s %d < %d' % (k, cnt[k], floors[k]) for k in low]))
        return 3
    return 0


if __name__ == '__main__':
    sys.exit(main())
