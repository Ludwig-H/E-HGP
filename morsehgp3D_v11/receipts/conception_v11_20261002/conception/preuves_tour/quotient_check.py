# Controle de conception (leger) : quotient local d'une coquille etendue par ENSEMBLES SEPARABLES MAXIMAUX
# contre la definition brute (t-parties separables, A ~ A' ssi A u A' separable).
# Centre a l'origine, points entiers sur une sphere (ou un cercle) : u_x = x.
import itertools, random, sys
from fractions import Fraction

def det3(a, b, c):
    return (a[0]*(b[1]*c[2]-b[2]*c[1]) - a[1]*(b[0]*c[2]-b[2]*c[0]) + a[2]*(b[0]*c[1]-b[1]*c[0]))
def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def dot(a, b):
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

# --- brute : 0 dans conv(A) (enveloppe fermee) <=> A contient un support (paire antipodale, triangle plan
#     contenant 0, tetraedre contenant 0), par Caratheodory. Test exact par dependances positives.
def origin_in_hull_small(pts):
    n = len(pts)
    if n == 1:
        return pts[0] == (0, 0, 0)
    if n == 2:
        a, b = pts
        return cross(a, b) == (0, 0, 0) and dot(a, b) < 0
    if n == 3:
        a, b, c = pts
        if det3(a, b, c) != 0:
            return False
        # coplanaires avec 0 : 0 dans le triangle ferme <=> combinaison positive (au sens large) nulle
        nrm = cross(a, b)
        if nrm == (0, 0, 0):
            nrm = cross(a, c)
        if nrm == (0, 0, 0):
            nrm = cross(b, c)
        if nrm == (0, 0, 0):
            # tous colineaires : 0 dans conv ssi deux de sens opposes
            return any(dot(p, q) < 0 for p, q in itertools.combinations(pts, 2))
        s = [dot(cross(a, b), nrm), dot(cross(b, c), nrm), dot(cross(c, a), nrm)]
        return all(x >= 0 for x in s) or all(x <= 0 for x in s)
    if n == 4:
        a, b, c, d = pts
        vol = det3(b[0]-a[0] and (b[0]-a[0], b[1]-a[1], b[2]-a[2]) or (b[0]-a[0], b[1]-a[1], b[2]-a[2]),
                   (c[0]-a[0], c[1]-a[1], c[2]-a[2]), (d[0]-a[0], d[1]-a[1], d[2]-a[2]))
        if vol == 0:
            return any(origin_in_hull_small(list(t)) for t in itertools.combinations(pts, 3))
        s = [det3(b, c, d), -det3(a, c, d), det3(a, b, d), -det3(a, b, c)]
        return all(x >= 0 for x in s) or all(x <= 0 for x in s)
    raise ValueError

def separable_brute(pts):
    for r in (2, 3, 4):
        for sub in itertools.combinations(pts, r):
            if origin_in_hull_small(list(sub)):
                return False
    return True

def brute_pieces(U, t):
    m = len(U)
    verts = [A for A in itertools.combinations(range(m), t) if separable_brute([U[i] for i in A])]
    if not verts:
        return None  # naissance
    idx = {A: i for i, A in enumerate(verts)}
    par = list(range(len(verts)))
    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    # aretes : (t+1)-parties separables relient leurs faces (meme cloture que "A u A' separable")
    if t + 1 <= m:
        for B in itertools.combinations(range(m), t + 1):
            if separable_brute([U[i] for i in B]):
                faces = [idx[tuple(x for x in B if x != y)] for y in B]
                for f in faces[1:]:
                    a, b = find(faces[0]), find(f)
                    if a != b:
                        par[a] = b
    comps = {}
    for A in verts:
        comps.setdefault(find(idx[A]), set()).add(A)
    return sorted(sorted(c) for c in comps.values())

# --- conception : ensembles separables maximaux par sommets de l'arrangement de grands cercles
def windows_2d(Z, U, nrm):
    """Z : indices de vecteurs dans le plan de normale nrm. Rend les fenetres semi-ouvertes [theta_l, theta_l + pi)."""
    out = set()
    for l in Z:
        w = 1 << l
        for k in Z:
            if k == l:
                continue
            c = dot(cross(U[l], U[k]), nrm)
            if c > 0:
                w |= 1 << k
            elif c == 0 and dot(U[l], U[k]) > 0:  # meme direction : impossible pour des points distincts d'une sphere
                w |= 1 << k
        out.add(w)
    return out

def maximal_separable(U):
    m = len(U)
    S = set()
    done = set()
    for i in range(m):
        for j in range(i + 1, m):
            if (i, j) in done:
                continue
            n = cross(U[i], U[j])
            if n == (0, 0, 0):
                continue
            for sg in (1, -1):
                v0 = (sg*n[0], sg*n[1], sg*n[2])
                P = 0
                Z = []
                for x in range(m):
                    d = dot(v0, U[x])
                    if d > 0:
                        P |= 1 << x
                    elif d == 0:
                        Z.append(x)
                for a in Z:
                    for b in Z:
                        if a < b:
                            done.add((a, b))
                for w in windows_2d(Z, U, v0):
                    S.add(P | w)
    if not S:
        # tous colineaires : paire antipodale (m = 2) ; ensembles separables maximaux = singletons
        for x in range(m):
            S.add(1 << x)
    return S

def design_pieces(U, t, S):
    alive = [M for M in S if bin(M).count('1') >= t]
    if not alive:
        return None
    par = list(range(len(alive)))
    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    for a in range(len(alive)):
        for b in range(a + 1, len(alive)):
            if bin(alive[a] & alive[b]).count('1') >= t:
                ra, rb = find(a), find(b)
                if ra != rb:
                    par[ra] = rb
    groups = {}
    for a, M in enumerate(alive):
        groups.setdefault(find(a), []).append(M)
    m = len(U)
    comps = []
    for Ms in groups.values():
        c = set()
        for M in Ms:
            els = [i for i in range(m) if M >> i & 1]
            for A in itertools.combinations(els, t):
                c.add(A)
        comps.append(sorted(c))
    return sorted(comps)

def shells():
    out = []
    # sphere r2 = 5 : permutations de (+-2, +-1, 0) : 24 points ; sous-ensembles
    s5 = sorted(set(p for a in (2, -2) for b in (1, -1) for p in itertools.permutations((a, b, 0))))
    s9 = sorted(set([p for a in (3, -3) for p in itertools.permutations((a, 0, 0))] +
                    [p for a in (2, -2) for b in (2, -2) for c in (1, -1) for p in itertools.permutations((a, b, c))]))
    cube = [(a, b, c) for a in (1, -1) for b in (1, -1) for c in (1, -1)]
    octa = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]
    c25 = [(x, y, 0) for x in range(-5, 6) for y in range(-5, 6) if x*x + y*y == 25]
    c65 = [(x, y, 0) for x in range(-9, 10) for y in range(-9, 10) if x*x + y*y == 65]
    out.append(('cube', cube))
    out.append(('octa', octa))
    rng = random.Random(7)
    for name, pool, sizes, reps in (('s5', s5, (4, 5, 6, 7, 8, 9), 40), ('s9', s9, (5, 6, 7, 8, 9), 40),
                                    ('c25', c25, (3, 4, 5, 6, 7, 8, 9), 30), ('c65', c65, (4, 6, 8, 9), 20),
                                    ('cube', cube, (4, 5, 6, 7), 20), ('octa', octa, (3, 4, 5), 10)):
        for sz in sizes:
            for _ in range(reps):
                if sz <= len(pool):
                    out.append((name + str(sz), rng.sample(pool, sz)))
    # mixtes : cercle + points hors plan
    for _ in range(60):
        k = rng.randint(3, 6)
        out.append(('mix', rng.sample(c25, k) and (rng.sample([p for p in s5 if p[2] == 0] , min(k, 8)) + rng.sample([p for p in s5 if p[2] != 0], rng.randint(1, 3)))))
    return out

def main():
    checks = fails = births = joins = inert = 0
    maxm = 0
    for name, U in shells():
        U = list(dict.fromkeys(U))
        m = len(U)
        maxm = max(maxm, m)
        S = maximal_separable(U)
        for t in range(1, m + 1):
            b = brute_pieces(U, t)
            d = design_pieces(U, t, S)
            checks += 1
            if b != d:
                fails += 1
                if fails <= 5:
                    print('ECART', name, U, 't', t, 'brut', None if b is None else len(b), 'conception', None if d is None else len(d))
            if b is None:
                births += 1
            elif len(b) >= 2:
                joins += 1
            else:
                inert += 1
    print('quotient_checks', checks, 'fails', fails, 'births', births, 'joins', joins, 'inert', inert, 'max_m', maxm)
    return 1 if fails else 0

if __name__ == '__main__':
    sys.exit(main())
