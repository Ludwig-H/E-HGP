#!/usr/bin/env python3
"""Auditeur C, piste D2 -- oracle exact borne de la generation par deletion locale.

Hors produit, hors registre, public_status=not_claimed. Aucune dependance au code v9.

Verifie sur de petits nuages entiers (n <= 10), en arithmetique exacte :
  E1  fermeture : tout interieur I != vide d'une boule critique possede v dans I avec
      I \\ {v} spheriquement separable (LP exact en rationnels) ;
  E2  BFS D2 par rhomboides pleins : en partant de J = vide, les enfants de J sont
      J u W pour W sous-ensemble non vide des sommets finis d'un tetraedre (fini ou infini)
      de Del(P \\ J) dont la sphere contient TOUT J ; en position generique l'ensemble
      atteint doit egaler l'ensemble des parties separables de taille <= Pmax (LP) ;
  E3  emission : une face F (2..4 sommets) d'un tetraedre de la region de conflit de J
      (sphere contenant au moins un point de J) est emise si sa boule minimale exacte a
      son centre dans relint conv F et un interieur exact egal a J ; l'ensemble emis,
      filtre par p + q_min <= Kmax + 1, doit egaler le catalogue exhaustif ;
  E4  lecture litterale de l'enonce : la boule d'un support S d'interieur I apparait-elle
      comme NOUVEAU simplexe S en retirant v de Del(P \\ (I \\ {v})) ? (compte des cas ou S
      etait deja un simplexe de Delaunay avant la deletion) ; forme corrigee : S est face
      d'un tetraedre nouveau (sphere contenant v) ;
  E5  lemme d'etoile : pour toute face F de Del(X) et sa boule minimale, vide(X) <=> aucun
      sommet de l'etoile de F strictement interieur.
Degenerescences : nuages sur petites grilles, perturbation entiere explicite (x * S + d,
|d| petit) utilisee SEULEMENT pour la triangulation et le BFS ; toutes les boules emises
sont recalculees exactement sur les coordonnees d'origine (cle, interieur, coquille, q_min).
"""
import itertools
import json
import random
import sys
from fractions import Fraction as F
import os

PREFILTER = os.environ.get("D2_PREFILTER") == "1"
ONLY_DEGENERATE = os.environ.get("D2_ONLY_DEGENERATE") == "1"


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def det3(u, v, w):
    return (u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0])
            + u[2] * (v[0] * w[1] - v[1] * w[0]))


def orient(a, b, c, d):
    x = det3(sub(b, a), sub(c, a), sub(d, a))
    return (x > 0) - (x < 0)


def det4(m):
    # expansion le long de la premiere ligne
    tot = 0
    for j in range(4):
        minor = [[m[r][c] for c in range(4) if c != j] for r in range(1, 4)]
        tot += (-1) ** j * m[0][j] * det3(*minor)
    return tot


def insphere_raw(a, b, c, d, e):
    rows = []
    for p in (a, b, c, d):
        q = sub(p, e)
        rows.append([q[0], q[1], q[2], dot(q, q)])
    x = det4(rows)
    return (x > 0) - (x < 0)


# calibration du signe : e strictement interieur <=> insphere_raw * orient == SIGN_IN
_T = ((0, 0, 0), (4, 0, 0), (0, 4, 0), (0, 0, 4))
SIGN_IN = insphere_raw(*_T, (1, 1, 1)) * orient(*_T)
assert SIGN_IN != 0


def inside_tet(T, e, X):
    """+1 si e strictement dans la sphere (ou au-dela de la face de bord pour un tet infini),
    0 si sur, -1 sinon. T = (a, b, c, d) indices, d = None pour le sommet infini."""
    if len(T) == 5:
        a, b, c, _, side = T
        return orient(X[a], X[b], X[c], e) * side
    a, b, c, d = T
    o = orient(X[a], X[b], X[c], X[d])
    return insphere_raw(X[a], X[b], X[c], X[d], e) * o * SIGN_IN


def fin(T):
    return T[:3] if len(T) == 5 else T


def delaunay(ids, X):
    """Tetraedres de Delaunay (sphere ouverte vide) de ids, plus tetraedres infinis (faces de
    bord). Pour un ensemble generique, c'est la triangulation de Delaunay exacte."""
    tets = []
    for T in itertools.combinations(ids, 4):
        if orient(*(X[i] for i in T)) == 0:
            continue
        ok = True
        for e in ids:
            if e in T:
                continue
            if inside_tet(T, X[e], X) > 0:
                ok = False
                break
        if ok:
            tets.append(T)
    for tri in itertools.combinations(ids, 3):
        a, b, c = tri
        sides = set()
        for e in ids:
            if e in tri:
                continue
            sides.add(orient(X[a], X[b], X[c], X[e]))
        sides.discard(0)
        if len(sides) == 1:
            s = sides.pop()
            tets.append((a, b, c, None, -s))  # "interieur" = demi-espace oppose au nuage
    return tets


def generic_zero(ids, X, tets):
    """Detecte une degenerescence dans la triangulation (cospherique ou coplanaire)."""
    for T in tets:
        if len(T) == 5:
            for e in ids:
                if e not in fin(T) and orient(X[T[0]], X[T[1]], X[T[2]], X[e]) == 0:
                    return True
            continue
        for e in ids:
            if e not in T and inside_tet(T, X[e], X) == 0:
                return True
    return False


# ---------------------------------------------------------------- boules minimales exactes
def solve(mat, rhs):
    n = len(mat)
    a = [list(map(F, row)) + [F(r)] for row, r in zip(mat, rhs)]
    for col in range(n):
        piv = next((r for r in range(col, n) if a[r][col] != 0), None)
        if piv is None:
            return None
        a[col], a[piv] = a[piv], a[col]
        for r in range(n):
            if r != col and a[r][col] != 0:
                f = a[r][col] / a[col][col]
                a[r] = [x - f * y for x, y in zip(a[r], a[col])]
    return [a[i][n] / a[i][i] for i in range(n)]


def meb_center(S):
    p0 = S[0]
    vs = [sub(s, p0) for s in S[1:]]
    k = len(vs)
    gram = [[dot(vs[i], vs[j]) for j in range(k)] for i in range(k)]
    rhs = [F(dot(v, v), 2) for v in vs]
    lam = solve(gram, rhs)
    if lam is None:
        return None, None
    c = tuple(F(p0[t]) + sum(lam[j] * vs[j][t] for j in range(k)) for t in range(3))
    return c, [1 - sum(lam)] + lam


def d2(c, z):
    return sum((F(z[t]) - c[t]) ** 2 for t in range(3))


def census(c, r2, P):
    I, U = [], []
    for i, z in enumerate(P):
        v = d2(c, z)
        if v < r2:
            I.append(i)
        elif v == r2:
            U.append(i)
    return tuple(I), tuple(U)


def q_min(c, U, P):
    for q in (2, 3, 4):
        for S in itertools.combinations(U, q):
            cc, bary = meb_center([P[i] for i in S])
            if cc is not None and cc == c and all(b > 0 for b in bary):
                return q
    return None


def exhaustive_catalogue(P, kmax):
    out = {}
    n = len(P)
    for q in (2, 3, 4):
        for S in itertools.combinations(range(n), q):
            c, bary = meb_center([P[i] for i in S])
            if c is None or any(b <= 0 for b in bary):
                continue
            r2 = d2(c, P[S[0]])
            key = (c, r2)
            if key in out:
                continue
            I, U = census(c, r2, P)
            out[key] = (I, U, q)  # premier q rencontre = q_min (boucle q croissante)
    return {k: v for k, v in out.items() if len(v[0]) + v[2] <= kmax + 1}


# ---------------------------------------------------------------- separabilite spherique (LP exact)
def simplex_min(A, b, cost):
    """min cost.y  s.c. A y = b, y >= 0 (b >= 0). Bland. Retourne la valeur optimale (Fraction)
    ou None si infaisable. A : liste de lignes."""
    m, nv = len(A), len(A[0])
    # tableau avec artificiels
    T = [[F(x) for x in A[i]] + [F(1) if j == i else F(0) for j in range(m)] + [F(b[i])] for i in range(m)]
    basis = [nv + i for i in range(m)]
    ncol = nv + m

    def run(obj):
        while True:
            # couts reduits
            red = []
            for j in range(ncol):
                if j in basis:
                    red.append(F(0))
                    continue
                v = obj[j] - sum(obj[basis[i]] * T[i][j] for i in range(m))
                red.append(v)
            ent = next((j for j in range(ncol) if red[j] < 0 and allowed[j]), None)
            if ent is None:
                return
            best = None
            for i in range(m):
                if T[i][ent] > 0:
                    ratio = T[i][-1] / T[i][ent]
                    if best is None or ratio < best[0] or (ratio == best[0] and basis[i] < basis[best[1]]):
                        best = (ratio, i)
            if best is None:
                raise RuntimeError("unbounded")
            r = best[1]
            pv = T[r][ent]
            T[r] = [x / pv for x in T[r]]
            for i in range(m):
                if i != r and T[i][ent] != 0:
                    f = T[i][ent]
                    T[i] = [x - f * y for x, y in zip(T[i], T[r])]
            basis[r] = ent

    allowed = [True] * ncol
    obj1 = [F(0)] * nv + [F(1)] * m
    run(obj1)
    if sum(T[i][-1] for i in range(m) if basis[i] >= nv) != 0:
        return None
    # sortir les artificiels restants de la base si possible
    for i in range(m):
        if basis[i] >= nv:
            j = next((j for j in range(nv) if T[i][j] != 0), None)
            if j is not None:
                pv = T[i][j]
                T[i] = [x / pv for x in T[i]]
                for k in range(m):
                    if k != i and T[k][j] != 0:
                        f = T[k][j]
                        T[k] = [x - f * y for x, y in zip(T[k], T[i])]
                basis[i] = j
    allowed = [j < nv for j in range(ncol)]
    obj2 = [F(x) for x in cost] + [F(0)] * m
    run(obj2)
    return sum(obj2[basis[i]] * T[i][-1] for i in range(m))


def separable(J, P):
    """J strictement dans une sphere, le reste strictement dehors (demi-espaces en limite)."""
    cols, h = [], []
    for i, x in enumerate(P):
        if i in J:
            cols.append((-2 * x[0], -2 * x[1], -2 * x[2], 1, 1))
            h.append(-dot(x, x))
        else:
            cols.append((2 * x[0], 2 * x[1], 2 * x[2], -1, 1))
            h.append(dot(x, x))
    cols.append((0, 0, 0, 0, 1))
    h.append(1)
    A = [[cols[j][r] for j in range(len(cols))] for r in range(5)]
    b = [0, 0, 0, 0, 1]
    val = simplex_min(A, b, h)
    return val is not None and val > 0


# ---------------------------------------------------------------- D2
def d2_bfs(P, X, kmax, stats):
    """P : coordonnees exactes ; X : coordonnees (eventuellement perturbees) de triangulation."""
    n = len(P)
    pmax = kmax - 1
    levels = [set() for _ in range(pmax + 1)]
    levels[0].add(frozenset())
    emitted = {}
    for p in range(pmax + 1):
        for J in sorted(levels[p], key=lambda s: sorted(s)):
            ids = [i for i in range(n) if i not in J]
            tets = delaunay(ids, X)
            if generic_zero(ids, X, tets):
                stats["nongeneric_triangulations"] += 1
            if J:
                cr = [T for T in tets if any(inside_tet(T, X[j], X) > 0 for j in J)]
                rt = [T for T in tets if all(inside_tet(T, X[j], X) > 0 for j in J)]
            else:
                cr = tets
                rt = tets
            stats["processed_J"] += 1
            stats["conflict_tets"] += len(cr)
            stats["rhomboid_tets"] += len(rt)
            for T in rt:
                fv = list(fin(T))
                for r in range(1, len(fv) + 1):
                    for W in itertools.combinations(fv, r):
                        if p + r <= pmax:
                            levels[p + r].add(J | frozenset(W))
            faces = set()
            for T in cr:
                fv = tuple(sorted(fin(T)))
                for q in (2, 3, 4):
                    for Fc in itertools.combinations(fv, q):
                        faces.add(Fc)
            stats["faces_tested"] += len(faces)
            for Fc in faces:
                if PREFILTER and p + len(Fc) > kmax + 1:
                    continue  # piege : pre-filtre par l'arite de la face au lieu de q_min exact
                c, bary = meb_center([P[i] for i in Fc])
                if c is None or any(bb <= 0 for bb in bary):
                    continue
                r2 = d2(c, P[Fc[0]])
                I, U = census(c, r2, P)
                if frozenset(I) != J:
                    continue
                key = (c, r2)
                emitted.setdefault(key, set()).add((J, Fc))
    reached = set().union(*levels)
    return emitted, reached


def check_cloud(P, kmax, rng, stats, perturb):
    n = len(P)
    if perturb:
        S = 10 ** 9
        while True:
            X = [tuple(S * z[t] + rng.randint(-100, 100) for t in range(3)) for z in P]
            ok = True
            # rejet si la perturbation laisse une degenerescence globale evidente
            for T in itertools.combinations(range(n), 4):
                if orient(*(X[i] for i in T)) == 0:
                    ok = False
                    break
            if ok:
                break
    else:
        X = [tuple(z) for z in P]
    cat = exhaustive_catalogue(P, kmax)
    emitted, reached = d2_bfs(P, X, kmax, stats)
    em = {}
    for key, srcs in emitted.items():
        c, r2 = key
        I, U = census(c, r2, P)
        qm = q_min(c, U, P)
        if len(I) + qm <= kmax + 1:
            em[key] = srcs
    res = {"n": n, "catalogue": len(cat), "emitted": len(em)}
    missing = [k for k in cat if k not in em]
    extra = [k for k in em if k not in cat]
    res["missing"] = len(missing)
    res["extra"] = len(extra)
    res["multi_emitted"] = sum(1 for s in em.values() if len(s) > 1)
    res["degenerate_balls"] = sum(1 for (I, U, q) in cat.values() if len(U) != q)
    # E2 : ensembles atteints vs separables exacts
    pmax = kmax - 1
    sep = set()
    for r in range(0, pmax + 1):
        for J in itertools.combinations(range(n), r):
            if r == 0 or separable(set(J), P):
                sep.add(frozenset(J))
    res["separable_sets"] = len(sep)
    res["reached_sets"] = len(reached)
    res["separable_not_reached"] = len(sep - reached)
    res["reached_not_separable"] = len(reached - sep)
    # E1 : fermeture
    clos_fail = 0
    for (I, U, q) in cat.values():
        if not I:
            continue
        if not any(separable(set(I) - {v}, P) for v in I):
            clos_fail += 1
    res["closure_failures"] = clos_fail
    return res, cat, X


def literal_hole_check(P, cat, stats):
    """E4 (nuages generiques, coordonnees exactes)."""
    n = len(P)
    for (c, r2), (I, U, q) in cat.items():
        if not I or len(U) != q:
            continue
        S = tuple(sorted(U))
        after = [i for i in range(n) if i not in I]
        tets_after = delaunay(after, P)
        for v in I:
            Jp = set(I) - {v}
            if not separable(Jp, P):
                continue
            stats["hole_cases"] += 1
            new_ok = False
            for T in tets_after:
                fs = set(fin(T))
                if set(S) <= fs and inside_tet(T, P[v], P) > 0:
                    new_ok = True
                    break
            if not new_ok:
                stats["hole_refined_failures"] += 1
            before = [i for i in range(n) if i not in Jp]
            tets_before = delaunay(before, P)
            pre = any(set(S) <= set(fin(T)) for T in tets_before)
            if pre:
                stats["support_preexisting_before_deletion"] += 1
                stats.setdefault("preexisting_by_q", {}).setdefault(str(q), 0)
                stats["preexisting_by_q"][str(q)] += 1


def star_lemma_check(P, stats):
    ids = list(range(len(P)))
    tets = delaunay(ids, P)
    faces = {}
    for T in tets:
        fv = tuple(sorted(fin(T)))
        for q in (2, 3):
            for Fc in itertools.combinations(fv, q):
                faces.setdefault(Fc, set()).update(fv)
    for Fc, star in faces.items():
        c, bary = meb_center([P[i] for i in Fc])
        if c is None:
            continue
        r2 = d2(c, P[Fc[0]])
        full = any(d2(c, P[i]) < r2 for i in ids if i not in Fc)
        loc = any(d2(c, P[i]) < r2 for i in star if i not in Fc)
        stats["star_lemma_faces"] += 1
        if full != loc:
            stats["star_lemma_failures"] += 1


def load_fixture(path):
    d = json.load(open(path))
    return [tuple(int(v) for v in p) for p in d["points"]]


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    trials = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    fixtures = sys.argv[3:]
    rng = random.Random(seed)
    stats = {k: 0 for k in ("processed_J", "conflict_tets", "rhomboid_tets", "faces_tested",
                            "nongeneric_triangulations", "hole_cases", "hole_refined_failures",
                            "support_preexisting_before_deletion", "star_lemma_faces",
                            "star_lemma_failures")}
    summary = {"clouds": 0, "generic": 0, "degenerate": 0, "missing": 0, "extra": 0,
               "multi_emitted": 0, "closure_failures": 0, "separable_not_reached": 0,
               "reached_not_separable_generic": 0, "reached_not_separable_degenerate": 0,
               "catalogue_balls": 0, "degenerate_balls": 0, "separable_sets": 0}
    clouds = []
    for path in fixtures:
        clouds.append(("fixture:" + path.split("/")[-1], load_fixture(path), False))
    for t in range(trials):
        n = rng.randint(8, 10)
        span = rng.choice([2, 3, 4] if ONLY_DEGENERATE else [3, 4, 5, 1000, 100000])
        pts = set()
        while len(pts) < n:
            pts.add(tuple(rng.randint(0, span) for _ in range(3)))
        pts = sorted(pts)
        clouds.append(("rand%d_span%d" % (t, span), pts, span <= 5))
    for name, pts, deg in clouds:
        # un nuage "generique" doit l'etre vraiment ; sinon on le traite comme degenere
        tets = delaunay(list(range(len(pts))), pts)
        if not deg and generic_zero(list(range(len(pts))), pts, tets):
            deg = True
        kmax = min(5, len(pts) - 4)
        res, cat, X = check_cloud(pts, kmax, rng, stats, perturb=deg)
        summary["clouds"] += 1
        summary["degenerate" if deg else "generic"] += 1
        for k in ("missing", "extra", "multi_emitted", "closure_failures", "separable_not_reached"):
            summary[k] += res[k]
        summary["reached_not_separable_degenerate" if deg else "reached_not_separable_generic"] += res["reached_not_separable"]
        summary["catalogue_balls"] += res["catalogue"]
        summary["degenerate_balls"] += res["degenerate_balls"]
        summary["separable_sets"] += res["separable_sets"]
        if not deg:
            literal_hole_check(pts, cat, stats)
            star_lemma_check(pts, stats)
        if res["missing"] or res["extra"] or res["closure_failures"] or res["separable_not_reached"]:
            print("FAIL", name, res, flush=True)
        else:
            print("ok", name, "deg" if deg else "gen", res, flush=True)
    print(json.dumps({"summary": summary, "stats": stats}, sort_keys=True))
    bad = summary["missing"] + summary["extra"] + summary["closure_failures"] + summary["separable_not_reached"] \
        + summary["reached_not_separable_generic"] + stats["hole_refined_failures"] + stats["star_lemma_failures"]
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
