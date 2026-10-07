#!/usr/bin/env python3
"""Oracle exact borné du complexe alpha d'ordre k, A_k(r) (Morse HGP 3D v11, 6 octobre 2026).

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference (oracle Python), profile=quantized_u21_input_only,
public_status=not_claimed. Oracle de CORRECTION sur petits nuages entiers (n <= N_MAX = 60, ordres k <= K_MAX = 8) ;
jamais une mesure d'échelle, jamais dans le calcul de la hiérarchie (invariant d'architecture : la mosaïque d'ordre
supérieur n'est construite qu'ici, en aval, bornée).

Objet (note de l'auditeur du 6 octobre 2026, commit d2be6bdc7, § 1, et seconde note 28d70f8ab) :
  - P fini de sites entiers distincts, poids unitaires ; d_k(y) = distance au k-ième plus proche site ;
    Omega_k(r) = {d_k <= r} (boules fermées, coupe fermée, niveau stocké a = r^2) ;
  - V_Q = domaine de Voronoï d'ordre k de la k-partie Q ; on ne garde comme SOMMETS que les Q de domaine PLEIN
    (dimension de aff(P)) ; la mosaïque est la subdivision régulière duale (barycentres c_Q = somme(Q) / k) ;
  - chaque cellule sigma a une clé (I', U') : I' = intersection, U' = réunion moins intersection de sa FAMILLE
    (toutes les k-parties dont le barycentre est sur sigma, sommets ou non) ; la face duale est
    F_sigma = {y : |y-u| égaux (u dans U'), I' à distance <= , les autres à distance >= } ;
    famille = {I' u J : J inclus dans U', |J| = k - |I'|} (vérifié) ; dim sigma = dim aff(U') (vérifié) ;
  - niveau a_sigma = min_{y dans F_sigma} d_k(y)^2, CERTIFIÉ : témoin z admissible (z dans F_sigma, exact),
    valeur d_k(z)^2 recalculée, multiplicateurs exacts (conditions KKT du problème convexe : z = somme w_s s,
    somme w_s = 1 sur les sites de la sphère S(z, a), w >= 0 sur les sites « dedans » de la clé, w <= 0 sur les
    sites « dehors », libres sur U') ;
  - A_k(r) = {sigma : a_sigma <= r^2} ; composantes par le 1-squelette ; plateaux (égalités de niveaux) traités
    d'un bloc ; événements H0 (naissances, fusions) ;
  - attribution : toute étiquette Q (pleine ou non) d'une cellule active appartient à la composante de cette
    cellule (unicité vérifiée) ; jamais par la position d'un barycentre ;
  - couverture P inter (C + B_r) = réunion des étiquettes pleines actives de la composante (formule de l'auditeur,
    § 1.3) ; contre-épreuve indépendante par Gamma_K (thèse, Déf. 20-22, Th. 2) : sommets = k-parties de boule
    minimale <= r, liaisons = (k+1)-parties de boule minimale <= r ;
  - offset exact par pièces convexes : C_v(r) + B_r = réunion des C_Q(r) + B_r (Q attribués à v) ; appartenance
    exacte d'un point x à une pièce : min_{y dans V_Q} max_{s dans Q u {x}} |y-s|^2 <= r^2 (même certificat KKT).

Construction (exacte, sans position générale, sans simulation de simplicité) :
  1. rang affine r de P ; « sommets de Voronoï » d'ordre k dans aff(P) = centres des sphères passant par r + 1
     sites affinement indépendants, avec |In| < k < |In| + |On| (In : strictement dedans, On : dessus) ; énumération
     EXHAUSTIVE des (r+1)-parties (filtre entier numpy int64 sous borne prouvée de non-débordement, |x - o| <= 1000 ;
     sinon entiers Python) ; dédoublonnage par centre exact ;
  2. cellule maximale (dimension r) = conv des barycentres des I u J ; treillis exact de ses faces (enveloppe 3D :
     plans d'appui exacts proposés par force brute ou par qhull puis VÉRIFIÉS : plan d'appui exact, fermeture (chaque
     arête dans exactement deux facettes), Euler) ; faces de toutes dimensions identifiées par leur clé ;
  3. contrôle STRICT global : appariement des facettes (2 cellules maximales, ou 1 et alors sur le bord de
     l'enveloppe globale, vérifié exactement), caractéristique d'Euler de la mosaïque = 1, cohérence clé / famille /
     dimension de chaque face, juge d'échantillon exact (k plus proches voisins de points rationnels tirés au hasard :
     ensemble unique => sommet de la mosaïque ; sinon clé (In, On) => face de la mosaïque), recouvrement exact
     (un point intérieur tiré au hasard est dans exactement une cellule maximale).
Toute violation lève ErreurOracle (aucune sortie partielle n'est rendue comme complète).

Unités : celles des coordonnées entières données. Indices de sites : rang dans la liste donnée.
Usage : voir la fin du fichier (``python oracle_ak.py --demo``) et README.md du dossier.
"""
import itertools
import math
import random
from fractions import Fraction as Fr

import os as _os

try:  # accélérateur facultatif (filtre entier exact sous borne prouvée) ; ORACLE_AK_PUR=1 le désactive
    import numpy as _np
except Exception:  # noqa: BLE001
    _np = None
_PUR = _os.environ.get('ORACLE_AK_PUR') == '1'
if _PUR:
    _np = None

N_MAX = 60
K_MAX = 8
COORD_NUMPY = 1000      # |x - origine| <= 1000 : 4896 B^5 < 2^63 (voir _spheres_numpy)
LIMITE_FAMILLE = 20000  # nombre maximal d'étiquettes d'une cellule (borne déclarée)


class ErreurOracle(RuntimeError):
    pass


def besoin(ok, pourquoi):
    if not ok:
        raise ErreurOracle(pourquoi)


# ------------------------------------------------------------------------------------------------ arithmétique

def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def n2(a):
    return dot(a, a)


def somme(pts):
    s = (0, 0, 0)
    for p in pts:
        s = add(s, p)
    return s


def rang_affine(pts):
    """Dimension affine exacte (0 à 3) d'une liste de points (entiers ou Fractions)."""
    pts = list(pts)
    if not pts:
        return -1
    a = pts[0]
    vec = [sub(q, a) for q in pts[1:]]
    u = next((v for v in vec if any(v)), None)
    if u is None:
        return 0
    w = next((cross(u, v) for v in vec if any(cross(u, v))), None)
    if w is None:
        return 1
    if any(dot(w, v) for v in vec):
        return 3
    return 2


def base_affine(pts):
    """Indices d'une base affine maximale (glouton dans l'ordre donné)."""
    base = [0]
    for i in range(1, len(pts)):
        cand = [pts[j] for j in base] + [pts[i]]
        if rang_affine(cand) == len(cand) - 1:
            base.append(i)
            if len(base) == 4:
                break
    return base


def resoudre(G, b):
    """Gauss exact (Fractions) ; None si singulier."""
    m = len(G)
    A = [[Fr(x) for x in G[i]] + [Fr(b[i])] for i in range(m)]
    for c in range(m):
        piv = next((i for i in range(c, m) if A[i][c] != 0), None)
        if piv is None:
            return None
        A[c], A[piv] = A[piv], A[c]
        for i in range(m):
            if i != c and A[i][c] != 0:
                f = A[i][c] / A[c][c]
                A[i] = [x - f * y for x, y in zip(A[i], A[c])]
    return [A[i][m] / A[i][i] for i in range(m)]


def circoncentre(pts):
    """Centre (Fractions) dans aff(pts) et rayon carré de la sphère passant par des points affinement
    indépendants ; None si dépendants."""
    a = pts[0]
    V = [sub(p, a) for p in pts[1:]]
    if not V:
        return tuple(Fr(x) for x in a), Fr(0)
    G = [[dot(V[i], V[j]) for j in range(len(V))] for i in range(len(V))]
    lam = resoudre(G, [Fr(n2(v), 2) for v in V])
    if lam is None:
        return None
    c = tuple(Fr(a[t]) + sum(lam[i] * V[i][t] for i in range(len(V))) for t in range(3))
    return c, sum((c[t] - a[t]) ** 2 for t in range(3))


def d2(c, p):
    return (c[0] - p[0]) ** 2 + (c[1] - p[1]) ** 2 + (c[2] - p[2]) ** 2


def coeffs_barycentriques(z, T):
    """Coordonnées affines exactes de z dans aff(T) (T affinement indépendants) ; None si z hors de aff(T)."""
    a = T[0]
    V = [sub(t, a) for t in T[1:]]
    zr = tuple(Fr(z[i]) - a[i] for i in range(3))
    if not V:
        return [Fr(1)] if all(x == 0 for x in zr) else None
    G = [[dot(V[i], V[j]) for j in range(len(V))] for i in range(len(V))]
    lam = resoudre(G, [sum(zr[t] * V[i][t] for t in range(3)) for i in range(len(V))])
    if lam is None:
        return None
    rec = tuple(sum(lam[i] * V[i][t] for i in range(len(V))) for t in range(3))
    if rec != zr:
        return None
    return [1 - sum(lam)] + lam


def meb(pts):
    """Boule minimale exacte de quelques points : (centre Fractions, rayon carré)."""
    best = None
    for t in range(1, min(4, len(pts)) + 1):
        for sous in itertools.combinations(range(len(pts)), t):
            S = [pts[i] for i in sous]
            cc = circoncentre(S)
            if cc is None:
                continue
            c, r2 = cc
            if best is not None and r2 >= best[1]:
                continue
            if all(d2(c, p) <= r2 for p in pts):
                best = (c, r2)
    besoin(best is not None, 'boule minimale introuvable')
    return best


# ------------------------------------------------------------------------------------------------ sphères d'ordre <= K

def _norm_centre(C, D):
    g = math.gcd(math.gcd(abs(C[0]), abs(C[1])), math.gcd(abs(C[2]), abs(D)))
    return (C[0] // g, C[1] // g, C[2] // g, D // g)


def _centre_entier(T, r):
    """Centre (C, D) entier (D > 0) de la sphère circonscrite à T (r + 1 points), dans aff(T) ; None si dégénéré."""
    a = T[0]
    if r == 1:
        b = T[1]
        return (add(a, b), 2) if a != b else None
    if r == 2:
        u, v = sub(T[1], a), sub(T[2], a)
        w = cross(u, v)
        if w == (0, 0, 0):
            return None
        uu, vv = n2(u), n2(v)
        nn = cross((uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2]), w)
        D = 2 * n2(w)
        return (tuple(D * a[j] + nn[j] for j in range(3)), D)
    u, v, t = sub(T[1], a), sub(T[2], a), sub(T[3], a)
    det = dot(u, cross(v, t))
    if det == 0:
        return None
    vt, tu, uv = cross(v, t), cross(t, u), cross(u, v)
    uu, vv, tt = n2(u), n2(v), n2(t)
    nn = tuple(uu * vt[j] + vv * tu[j] + tt * uv[j] for j in range(3))
    D = 2 * det
    C = tuple(D * a[j] + nn[j] for j in range(3))
    if D < 0:
        D, C = -D, tuple(-x for x in C)
    return C, D


def _classer(P, C, D):
    """(In, On) exacts à la sphère de centre C / D passant par... : classement par D^2 |y - p|^2 (entiers)."""
    h = [(D * p[0] - C[0]) ** 2 + (D * p[1] - C[1]) ** 2 + (D * p[2] - C[2]) ** 2 for p in P]
    return h


def _spheres_python(P, r, kmax, sous_ensembles):
    out = {}
    for T in sous_ensembles:
        ce = _centre_entier([P[i] for i in T], r)
        if ce is None:
            continue
        C, D = ce
        a = P[T[0]]
        ref = (D * a[0] - C[0]) ** 2 + (D * a[1] - C[1]) ** 2 + (D * a[2] - C[2]) ** 2
        dedans = 0
        for p in P:
            if (D * p[0] - C[0]) ** 2 + (D * p[1] - C[1]) ** 2 + (D * p[2] - C[2]) ** 2 < ref:
                dedans += 1
                if dedans >= kmax:
                    break
        if dedans < kmax:
            # clé = (centre, rayon) : deux sphères CONCENTRIQUES de rayons différents sont distinctes
            out.setdefault(_norm_centre(C, D) + (Fr(ref, D * D),), T)
    return out


def _spheres_numpy(P, kmax, bloc=20000):
    """Filtre exact en int64 des 4-parties (rang 3). Borne : coordonnées centrées |x| <= B = 1000 ; |det| <= 48 B^3,
    D <= 96 B^3, |nn_j| <= 288 B^4, |C_j| <= 384 B^4, |D(|p|^2 - |a|^2)| <= 288 B^5, |2 C.(p - a)| <= 4608 B^5 :
    toute quantité <= 4896 B^5 < 4.9e18 < 2^63. Rend {centre normalisé : 4-partie} (sphères avec |In| < kmax)."""
    np = _np
    X = np.array(P, dtype=np.int64)
    o = np.round(X.mean(axis=0)).astype(np.int64)
    X = X - o
    besoin(int(np.abs(X).max()) <= COORD_NUMPY, 'filtre numpy hors borne')
    Pc = [tuple(int(c) for c in x) for x in X.tolist()]
    n = len(P)
    nrm = np.einsum('ij,ij->i', X, X)
    out = {}
    it = itertools.combinations(range(n), 4)
    while True:
        chunk = list(itertools.islice(it, bloc))
        if not chunk:
            break
        idx = np.array(chunk, dtype=np.int64)
        a, b, c, d = X[idx[:, 0]], X[idx[:, 1]], X[idx[:, 2]], X[idx[:, 3]]
        u, v, t = b - a, c - a, d - a
        vt, tu, uv = np.cross(v, t), np.cross(t, u), np.cross(u, v)
        det = np.einsum('ij,ij->i', u, vt)
        uu, vv, tt = (np.einsum('ij,ij->i', w, w) for w in (u, v, t))
        nn = uu[:, None] * vt + vv[:, None] * tu + tt[:, None] * uv
        D = 2 * det
        C = D[:, None] * a + nn
        neg = D < 0
        D = np.where(neg, -D, D)
        C = np.where(neg[:, None], -C, C)
        ok = D != 0
        S = D[:, None] * (nrm[None, :] - nrm[idx[:, 0]][:, None]) - 2 * (C @ X.T - np.einsum('ij,ij->i', C, a)[:, None])
        dedans = (S < 0).sum(axis=1)
        garde = np.flatnonzero(ok & (dedans < kmax))
        for i in garde.tolist():
            T = chunk[i]
            ce = _centre_entier([Pc[j] for j in T], 3)
            C0, D0 = ce
            # re-décision exacte en entiers Python
            h = _classer(Pc, C0, D0)
            ref = h[T[0]]
            besoin(sum(1 for x in h if x < ref) < kmax, 'filtre numpy incohérent')
            Cg = tuple(C0[j] + D0 * int(o[j]) for j in range(3))
            out.setdefault(_norm_centre(Cg, D0) + (Fr(ref, D0 * D0),), T)
    return out


# ------------------------------------------------------------------------------------------------ treillis exact

def _plan_2d(pts):
    """Normale entière d'un ensemble coplanaire de rang 2 et coordonnée à omettre pour la projection."""
    a = pts[0]
    vec = [sub(q, a) for q in pts[1:]]
    u = next(v for v in vec if any(v))
    w = next(cross(u, v) for v in vec if any(cross(u, v)))
    j = max(range(3), key=lambda i: abs(w[i]))
    return w, j


def _enveloppe_2d(pts, idx):
    """Sommets (indices) de l'enveloppe convexe exacte, dans l'ordre, sans points alignés (rang 2 supposé)."""
    w, j = _plan_2d([pts[i] for i in idx])
    keep = [t for t in range(3) if t != j]
    proj = {i: (pts[i][keep[0]], pts[i][keep[1]]) for i in idx}
    order = sorted(set(idx), key=lambda i: proj[i])

    def cr(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    low, up = [], []
    for i in order:
        while len(low) >= 2 and cr(proj[low[-2]], proj[low[-1]], proj[i]) <= 0:
            low.pop()
        low.append(i)
    for i in reversed(order):
        while len(up) >= 2 and cr(proj[up[-2]], proj[up[-1]], proj[i]) <= 0:
            up.pop()
        up.append(i)
    return low[:-1] + up[:-1]


def _sur_segment(p, a, b):
    ab, ap = sub(b, a), sub(p, a)
    return cross(ab, ap) == (0, 0, 0) and 0 <= dot(ap, ab) <= dot(ab, ab)


def _faces_polygone(pts, idx):
    """Faces d'un polygone (rang 2) : [(dim, frozenset d'indices)], polygone compris."""
    som = _enveloppe_2d(pts, idx)
    out = [(2, frozenset(idx))]
    for i in range(len(som)):
        a, b = som[i], som[(i + 1) % len(som)]
        out.append((1, frozenset(x for x in idx if _sur_segment(pts[x], pts[a], pts[b]))))
        out.append((0, frozenset([a])))
    return out


def _facettes_brutes(pts):
    fac = set()
    N = len(pts)
    for i, j, l in itertools.combinations(range(N), 3):
        nrm = cross(sub(pts[j], pts[i]), sub(pts[l], pts[i]))
        if nrm == (0, 0, 0):
            continue
        s = [dot(nrm, sub(p, pts[i])) for p in pts]
        if all(x >= 0 for x in s) or all(x <= 0 for x in s):
            fac.add(frozenset(t for t in range(N) if s[t] == 0))
    return fac


def _facettes_qhull(pts):
    """Propositions qhull (flottant) puis vérification exacte de chaque plan d'appui ; None si qhull indisponible ou
    si une proposition échoue (repli force brute)."""
    if _PUR:
        return None
    try:
        from scipy.spatial import ConvexHull
    except Exception:  # noqa: BLE001
        return None
    X = [[float(c) for c in p] for p in pts]
    try:
        h = ConvexHull(X)
    except Exception:  # noqa: BLE001
        return None
    fac = set()
    N = len(pts)
    for s in h.simplices.tolist():
        i, j, l = s
        nrm = cross(sub(pts[j], pts[i]), sub(pts[l], pts[i]))
        if nrm == (0, 0, 0):
            return None
        sg = [dot(nrm, sub(p, pts[i])) for p in pts]
        if not (all(x >= 0 for x in sg) or all(x <= 0 for x in sg)):
            return None
        fac.add(frozenset(t for t in range(N) if sg[t] == 0))
    return fac


def treillis(pts):
    """Treillis exact des faces de conv(pts) (points entiers distincts) : liste de (dim, frozenset d'indices des
    points situés sur la face), polytope compris. Complétude vérifiée (fermeture et Euler en dimension 3)."""
    N = len(pts)
    r = rang_affine(pts)
    tout = frozenset(range(N))
    if r == 0:
        return [(0, tout)]
    if r == 1:
        a = pts[0]
        e = next(sub(p, a) for p in pts if p != a)
        pr = [dot(sub(p, a), e) for p in pts]
        return [(1, tout), (0, frozenset([pr.index(min(pr))])), (0, frozenset([pr.index(max(pr))]))]
    if r == 2:
        return _faces_polygone(pts, list(range(N)))
    fac = None
    if N > 12:
        fac = _facettes_qhull(pts)
    faces = None
    for essai in (fac, 'brut'):
        if essai is None:
            continue
        if essai == 'brut':
            essai = _facettes_brutes(pts)
        faces = {(3, tout)}
        aretes = {}
        for F in essai:
            besoin(rang_affine([pts[i] for i in F]) == 2, 'facette de rang != 2')
            for d, s in _faces_polygone(pts, sorted(F)):
                faces.add((d, s))
                if d == 1:
                    aretes[s] = aretes.get(s, 0) + 1
        ferme = all(c == 2 for c in aretes.values())
        nv = sum(1 for d, _ in faces if d == 0)
        ne = sum(1 for d, _ in faces if d == 1)
        nf = sum(1 for d, _ in faces if d == 2)
        if ferme and nv - ne + nf == 2:
            break
        faces = None
    besoin(faces is not None, 'treillis 3D non fermé')
    return sorted(faces, key=lambda f: (f[0], sorted(f[1])))


# ------------------------------------------------------------------------------------------------ nuage et ordres

class Nuage(object):
    """Nuage entier borné ; sphères d'ordre <= kmax (exhaustives) ; mosaïques par ordre (paresseuses)."""

    def __init__(self, points, kmax, accelerer=True):
        P = [tuple(int(c) for c in p) for p in points]
        besoin(1 <= len(P) <= N_MAX, 'n hors de [1, %d]' % N_MAX)
        besoin(len(set(P)) == len(P), 'sites confondus (le modèle FULL exige des sites distincts)')
        besoin(1 <= kmax <= min(K_MAX, len(P)), 'kmax hors de [1, min(%d, n)]' % K_MAX)
        self.P, self.n, self.kmax = P, len(P), kmax
        self.rang = rang_affine(P)
        self.spheres = self._spheres(accelerer)
        self._ordres = {}

    def _spheres(self, accelerer):
        P, r, kmax = self.P, self.rang, self.kmax
        if r <= 0:
            return []
        if r == 3 and accelerer and _np is not None:
            Xn = _np.array(P, dtype=_np.int64)
            spread = int(_np.abs(Xn - _np.round(Xn.mean(axis=0)).astype(_np.int64)).max())
            if spread <= COORD_NUMPY:          # même centrage que _spheres_numpy
                cand = _spheres_numpy(P, kmax)
            else:
                cand = _spheres_python(P, 3, kmax, itertools.combinations(range(self.n), 4))
        else:
            cand = _spheres_python(P, r, kmax, itertools.combinations(range(self.n), r + 1))
        out = []
        for key in sorted(cand):
            C, D = key[:3], key[3]
            h = _classer(P, C, D)
            ref = h[cand[key][0]]
            besoin(Fr(ref, D * D) == key[4], 'rayon de la sphère incohérent')
            In = tuple(i for i in range(self.n) if h[i] < ref)
            On = tuple(i for i in range(self.n) if h[i] == ref)
            besoin(rang_affine([P[i] for i in On]) == r, 'sphère de rang insuffisant')
            out.append(dict(C=C, D=D, num=ref, In=In, On=On,
                            centre=tuple(Fr(c, D) for c in C), r2=Fr(ref, D * D)))
        return out

    def ordre(self, k, **options):
        if k not in self._ordres:
            self._ordres[k] = Mosaique(self, k, **options)
        return self._ordres[k]

    # -- fonctions exactes du modèle
    def distances2(self, y):
        return sorted(d2(y, p) for p in self.P)

    def dk2(self, y, k):
        return self.distances2(y)[k - 1]

    def structure(self, y, k):
        """(In, On) exacts en y à l'ordre k : In = sites strictement plus proches que d_k, On = à distance d_k."""
        d = [d2(y, p) for p in self.P]
        dk = sorted(d)[k - 1]
        return (tuple(i for i in range(self.n) if d[i] < dk), tuple(i for i in range(self.n) if d[i] == dk), dk)


class Mosaique(object):
    """Mosaïque exacte d'ordre k : étiquettes, cellules maximales, faces (clé, dimension, famille, sommets),
    incidences (bord, cofaces), niveaux certifiés, composantes et couverture aux coupes.

    Champs : faces (liste de dict : id, dim, I, U, fam (frozenset d'étiquettes), som (étiquettes sommets),
    a (Fraction), z (témoin), w (multiplicateurs : {site : Fraction})), cle -> id, etiquettes (tuple trié -> id),
    pleine[id] (étiquette sommet), max_cells (liste d'ids de faces maximales), bord[id], cofaces[id],
    niveaux (liste triée des niveaux distincts), comptes."""

    def __init__(self, nuage, k, certifier=True, juge=200, graine=0):
        besoin(1 <= k <= nuage.kmax, 'ordre hors de [1, kmax]')
        self.N, self.k = nuage, k
        self.P, self.n = nuage.P, nuage.n
        self.etiquettes, self.etiq_list, self.pleine = {}, [], []
        self.faces, self.cle = [], {}
        self.max_cells = []
        self.cell_faces = {}
        self.comptes = {}
        self._construire()
        self._controler(juge, graine)
        self._niveaux(certifier)

    # -- étiquettes et faces
    def _etiq(self, Q):
        Q = tuple(sorted(Q))
        if Q not in self.etiquettes:
            self.etiquettes[Q] = len(self.etiq_list)
            self.etiq_list.append(Q)
            self.pleine.append(False)
        return self.etiquettes[Q]

    def _face(self, dim, fam):
        Qs = [set(self.etiq_list[e]) for e in fam]
        I = set.intersection(*Qs)
        U = set.union(*Qs) - I
        I, U = tuple(sorted(I)), tuple(sorted(U))
        if dim == 0:
            besoin(len(fam) == 1, 'ordre %d : sommet portant %d étiquettes' % (self.k, len(fam)))
            besoin(not U, 'ordre %d : sommet à coquille' % self.k)
        else:
            j = self.k - len(I)
            besoin(0 < j < len(U), 'ordre %d : clé (%s, %s) hors de 0 < j < |U\'|' % (self.k, I, U))
            attendu = frozenset(self.etiquettes.get(tuple(sorted(I + J)), -1) for J in itertools.combinations(U, j))
            besoin(attendu == fam, 'ordre %d : famille de la face (%s, %s) incomplète ou excédentaire' % (self.k, I, U))
            besoin(rang_affine([self.P[u] for u in U]) == dim, 'ordre %d : dimension duale (%s, %s)' % (self.k, I, U))
        key = (I, U)
        if key in self.cle:
            f = self.faces[self.cle[key]]
            besoin(f['dim'] == dim and f['fam'] == fam, 'ordre %d : face (%s, %s) incohérente entre cellules'
                   % (self.k, I, U))
            return f['id']
        fid = len(self.faces)
        self.faces.append(dict(id=fid, dim=dim, I=I, U=U, fam=fam, som=None, a=None, z=None, w=None))
        self.cle[key] = fid
        return fid

    def _construire(self):
        k, P = self.k, self.P
        cells = [s for s in self.N.spheres if len(s['In']) < k < len(s['In']) + len(s['On'])]
        self.dim_max = self.N.rang
        if not cells:
            # aucun sommet de Voronoï d'ordre k : une seule étiquette (k = n) ou erreur
            besoin(k == self.n, 'ordre %d : aucune cellule maximale alors que k < n' % k)
            e = self._etiq(range(self.n))
            self.pleine[e] = True
            fid = self._face(0, frozenset([e]))
            self.faces[fid]['som'] = frozenset([e])
            self.max_cells = [fid]
            self.cell_faces[fid] = [fid]
            self.dim_max = 0
            self.bord, self.cofaces = {fid: set()}, {fid: set()}
            return
        bord, cof = {}, {}
        for s in cells:
            I, U = s['In'], s['On']
            j = k - len(I)
            nfam = math.comb(len(U), j)
            besoin(nfam <= LIMITE_FAMILLE, 'ordre %d : cellule à %d étiquettes (> %d)' % (k, nfam, LIMITE_FAMILLE))
            labs = [self._etiq(I + J) for J in itertools.combinations(U, j)]
            pos = {}
            for e in labs:
                pos.setdefault(somme(P[q] for q in self.etiq_list[e]), []).append(e)
            plist = sorted(pos)
            lat = treillis(plist)
            besoin(max(d for d, _ in lat) == self.dim_max, 'ordre %d : cellule maximale de dimension %d != %d'
                   % (k, max(d for d, _ in lat), self.dim_max))
            ids = []
            for d, sidx in lat:
                fam = frozenset(e for i in sidx for e in pos[plist[i]])
                fid = self._face(d, fam)
                if d == 0:
                    self.pleine[next(iter(fam))] = True
                ids.append((fid, fam, d))
            top = [fid for fid, _, d in ids if d == self.dim_max]
            besoin(len(top) == 1, 'ordre %d : plusieurs cellules maximales pour une sphère' % k)
            top = top[0]
            besoin(top not in self.cell_faces, 'ordre %d : cellule maximale rencontrée deux fois' % k)
            self.max_cells.append(top)
            self.cell_faces[top] = [fid for fid, _, _ in ids]
            self.faces[top]['sphere'] = s
            for fid, fam, d in ids:
                for gid, gfam, dg in ids:
                    if fam < gfam:
                        cof.setdefault(fid, set()).add(gid)
                        if dg == d + 1:
                            bord.setdefault(gid, set()).add(fid)
        for f in self.faces:
            bord.setdefault(f['id'], set())
            cof.setdefault(f['id'], set())
        self.bord, self.cofaces = bord, cof
        for f in self.faces:
            f['som'] = frozenset(e for e in f['fam'] if self.pleine[e])
            if f['dim'] == 0:
                besoin(len(f['som']) == 1, 'sommet sans étiquette pleine')

    # -- contrôle strict global
    def _controler(self, juge, graine):
        k, P = self.k, self.P
        F = self.faces
        cpt = self.comptes
        cpt['f_vecteur'] = [sum(1 for f in F if f['dim'] == d) for d in range(self.dim_max + 1)]
        euler = sum((-1) ** f['dim'] for f in F)
        cpt['euler'] = euler
        besoin(euler == 1, 'ordre %d : caractéristique d\'Euler %d != 1' % (k, euler))
        cpt['etiquettes'] = len(self.etiq_list)
        cpt['etiquettes_pleines'] = sum(self.pleine)
        cpt['cellules_maximales'] = len(self.max_cells)
        # bord : appariement des facettes des cellules maximales
        r = self.dim_max
        if r >= 1:
            compte = {}
            for top in self.max_cells:
                for fid in self.bord[top]:
                    compte[fid] = compte.get(fid, 0) + 1
            besoin(all(c in (1, 2) for c in compte.values()), 'ordre %d : facette dans plus de 2 cellules' % k)
            vs = [self.somme_etiq(e) for e in range(len(self.etiq_list)) if self.pleine[e]]
            nb = 0
            for fid, c in compte.items():
                if c == 2:
                    continue
                nb += 1
                pts = [self.somme_etiq(e) for e in F[fid]['som']]
                besoin(self._sur_bord_global(pts, vs, r), 'ordre %d : facette libre hors du bord (trou)' % k)
            cpt['facettes_de_bord'] = nb
            # enveloppe inférieure des relevés (puissance des barycentres pondérés, BCY Th. 4.8) : à chaque facette
            # intérieure partagée par A et B, les sommets de B hors de la facette sont STRICTEMENT au-dessus de
            # l'hyperplan d'appui de A (puissance en z_A) et réciproquement ; convexité locale => régularité globale
            par_facette = {}
            for top in self.max_cells:
                for fid in self.bord[top]:
                    par_facette.setdefault(fid, []).append(top)
            nloc = 0
            for fid, tops in par_facette.items():
                if len(tops) != 2:
                    continue
                for A, B in (tops, tops[::-1]):
                    zA = F[A]['sphere']['centre']
                    base = self.puissance(next(iter(F[A]['som'])), zA)
                    for e in F[B]['som'] - F[fid]['som']:
                        besoin(self.puissance(e, zA) > base, 'ordre %d : relevé non localement convexe' % k)
                        nloc += 1
                    besoin(all(self.puissance(e, zA) == base for e in F[A]['fam']), 'ordre %d : relevés non coplanaires' % k)
            cpt['convexite_locale_controles'] = nloc
            cpt['facettes_interieures'] = sum(1 for c in compte.values() if c == 2)
        # juge d'échantillon exact
        rnd = random.Random(graine + 7919 * k)
        lo = [min(p[t] for p in P) for t in range(3)]
        hi = [max(p[t] for p in P) for t in range(3)]
        manques, sommets, faces_vues = 0, 0, 0
        for _ in range(juge):
            y = tuple(Fr(rnd.randint(4 * lo[t] - 4 - (hi[t] - lo[t]), 4 * hi[t] + 4 + (hi[t] - lo[t])), 4)
                      for t in range(3))
            if rnd.random() < 0.3:   # points d'intérêt : milieux de sites (contacts, bissectrices)
                a, b = rnd.sample(range(self.n), 2) if self.n >= 2 else (0, 0)
                y = tuple(Fr(P[a][t] + P[b][t], 2) for t in range(3))
            if self.N.rang < 3:      # projeter dans aff(P) : la structure est invariante le long de la normale
                y = self._projeter(y)
            In, On, _ = self.N.structure(y, k)
            if len(In) + len(On) == k:
                sommets += 1
                e = self.etiquettes.get(tuple(sorted(In + On)))
                if e is None or not self.pleine[e]:
                    manques += 1
            else:
                faces_vues += 1
                if (In, On) not in self.cle:
                    manques += 1
        cpt['juge'] = dict(tirages=juge, sommets=sommets, faces=faces_vues, manques=manques)
        besoin(manques == 0, 'ordre %d : juge d\'échantillon : %d manques' % (k, manques))
        # recouvrement exact : points intérieurs dans exactement une cellule maximale
        if r == 3:
            cpt['recouvrement'] = self._recouvrement(rnd, 60)

    def puissance(self, e, z):
        """k fois la puissance du barycentre pondéré de l'étiquette e en z : somme des distances carrées à Q."""
        return sum(d2(z, self.P[q]) for q in self.etiq_list[e])

    def vertical(self, a, M_bas):
        """Application pi0(A_k(r)) -> pi0(A_{k-1}(r)) (inclusion Omega_k(r) dans Omega_{k-1}(r)) : chaque sommet actif Q
        (témoin z, d_{k-1}(z) <= d_k(z) <= r) est envoyé sur la face d'ordre k-1 qui contient z dans son intérieur
        relatif (clé (In, On) à l'ordre k-1), puis sur sa composante. Vérifie que l'application est bien définie
        (tous les sommets d'une composante ont la même image). Rend la liste des images."""
        besoin(M_bas.k == self.k - 1 and M_bas.N is self.N, 'vertical : ordres non consécutifs du même nuage')
        comp, etiq, _ = self.composantes(a)
        comp_b, etiq_b, face_b = M_bas.composantes(a)
        images = []
        for g in comp:
            cibles = set()
            for e in g:
                f = self.faces[self.cle[(self.etiq_list[e], ())]]
                In, On, dk = self.N.structure(f['z'], M_bas.k)
                besoin(dk <= a, 'vertical : témoin hors de Omega_{k-1}(r)')
                if len(In) + len(On) == M_bas.k:
                    fid = M_bas.cle[(tuple(sorted(In + On)), ())]
                else:
                    fid = M_bas.cle[(In, On)]
                besoin(fid in face_b, 'vertical : face d\'ordre k-1 inactive')
                cibles.add(face_b[fid])
            besoin(len(cibles) == 1, 'vertical : composante envoyée sur %d composantes' % len(cibles))
            images.append(cibles.pop())
        return images

    def somme_etiq(self, e):
        return somme(self.P[q] for q in self.etiq_list[e])

    def _projeter(self, y):
        P = self.P
        b = base_affine(P)
        T = [P[i] for i in b]
        a = T[0]
        V = [sub(t, a) for t in T[1:]]
        if not V:
            return tuple(Fr(c) for c in a)
        G = [[dot(V[i], V[j]) for j in range(len(V))] for i in range(len(V))]
        yr = tuple(Fr(y[t]) - a[t] for t in range(3))
        lam = resoudre(G, [sum(yr[t] * V[i][t] for t in range(3)) for i in range(len(V))])
        return tuple(a[t] + sum(lam[i] * V[i][t] for i in range(len(V))) for t in range(3))

    def _sur_bord_global(self, pts, vs, r):
        if r == 1:
            a = vs[0]
            e = next(sub(p, a) for p in vs if p != a)
            pr = [dot(sub(p, a), e) for p in vs]
            x = dot(sub(pts[0], a), e)
            return x == min(pr) or x == max(pr)
        if r == 2:
            w, _ = _plan_2d(vs)
            a, b = pts[0], next(p for p in pts if p != pts[0])
            nrm = cross(w, sub(b, a))
            s = [dot(nrm, sub(p, a)) for p in vs]
            return all(x >= 0 for x in s) or all(x <= 0 for x in s)
        a = pts[0]
        b = next(p for p in pts if p != a)
        c = next(p for p in pts if cross(sub(b, a), sub(p, a)) != (0, 0, 0))
        nrm = cross(sub(b, a), sub(c, a))
        s = [dot(nrm, sub(p, a)) for p in vs]
        return all(x >= 0 for x in s) or all(x <= 0 for x in s)

    def _demi_espaces(self, top):
        """Facettes de la cellule maximale top (dimension 3) : liste de (normale, point, signe intérieur)."""
        if not hasattr(self, '_hs'):
            self._hs = {}
        if top in self._hs:
            return self._hs[top]
        som = [self.somme_etiq(e) for e in self.faces[top]['som']]
        cen = tuple(Fr(sum(p[t] for p in som), len(som)) for t in range(3))
        out = []
        for fid in self.bord[top]:
            pts = [self.somme_etiq(e) for e in self.faces[fid]['som']]
            a = pts[0]
            b = next(p for p in pts if p != a)
            c = next(p for p in pts if cross(sub(b, a), sub(p, a)) != (0, 0, 0))
            nrm = cross(sub(b, a), sub(c, a))
            sg = dot(nrm, tuple(cen[t] - a[t] for t in range(3)))
            out.append((nrm, a, 1 if sg > 0 else -1))
        self._hs[top] = out
        return out

    def _recouvrement(self, rnd, tirages):
        """Points intérieurs (combinaisons convexes rationnelles de sommets de cellules) : chacun doit être dans
        exactement une cellule maximale fermée lorsqu'il est intérieur strict à l'une d'elles."""
        tops = self.max_cells
        boites = {}
        for top in tops:
            som = [self.somme_etiq(e) for e in self.faces[top]['som']]
            boites[top] = ([min(p[t] for p in som) for t in range(3)], [max(p[t] for p in som) for t in range(3)])
        bilan = dict(tirages=0, interieurs=0, defauts=0)
        allv = [self.somme_etiq(e) for e in range(len(self.etiq_list)) if self.pleine[e]]
        for _ in range(tirages):
            pts = rnd.sample(allv, min(4, len(allv)))
            wts = [rnd.randint(1, 9) for _ in pts]
            y = tuple(Fr(sum(w * p[t] for w, p in zip(wts, pts)), sum(wts)) for t in range(3))
            dedans_strict, dedans = 0, 0
            for top in tops:
                lo, hi = boites[top]
                if any(y[t] < lo[t] or y[t] > hi[t] for t in range(3)):
                    continue
                vals = [sg * dot(nrm, tuple(y[t] - a[t] for t in range(3))) for nrm, a, sg in self._demi_espaces(top)]
                if all(v >= 0 for v in vals):
                    dedans += 1
                    if all(v > 0 for v in vals):
                        dedans_strict += 1
            bilan['tirages'] += 1
            if dedans_strict:
                bilan['interieurs'] += 1
                if dedans != 1:
                    bilan['defauts'] += 1
            elif dedans == 0:
                bilan['defauts'] += 1
        besoin(bilan['defauts'] == 0, 'ordre %d : recouvrement défaillant %r' % (self.k, bilan))
        return bilan

    # -- niveaux
    def _hz(self, z):
        """Distances carrées exactes de z (Fractions) à tous les sites, en entiers : (D, h) avec
        h_i = D^2 |z - p_i|^2."""
        D = 1
        for c in z:
            D = D * c.denominator // math.gcd(D, c.denominator)
        C = tuple(c.numerator * (D // c.denominator) for c in z)
        return D, [(D * p[0] - C[0]) ** 2 + (D * p[1] - C[1]) ** 2 + (D * p[2] - C[2]) ** 2 for p in self.P]

    def _propre(self, f):
        """Niveau propre : centre z et rayon carré du minimum non contraint sur aff(F_f), et admissibilité."""
        P = self.P
        if f['dim'] == 0:
            Q = self.etiq_list[next(iter(f['som']))]
            z, r2 = meb([P[q] for q in Q])
            D, h = self._hz(z)
            hq = max(h[q] for q in Q)
            dedans = set(Q)
            ok = all(h[i] >= hq for i in range(self.n) if i not in dedans)
            return z, r2, ok
        U = f['U']
        b = base_affine([P[u] for u in U])
        z, r2 = circoncentre([P[U[i]] for i in b])
        D, h = self._hz(z)
        hu = h[U[0]]
        besoin(all(h[u] == hu for u in U), 'coquille non cosphérique')
        dedans = set(f['I'])
        uu = set(U)
        ok = all((h[i] <= hu) if i in dedans else (h[i] >= hu) for i in range(self.n) if i not in uu)
        return z, r2, ok

    def _niveaux(self, certifier):
        F = self.faces
        prop = {}
        for f in F:
            z, r2, ok = self._propre(f)
            prop[f['id']] = (r2, z) if ok else None
        # propagation descendante : a(f) = min(propre admissible, a des cofaces immédiates)
        ordre = sorted(F, key=lambda f: -f['dim'])
        best = {}
        cofi = {f['id']: set() for f in F}
        for g in F:
            for f in self.bord[g['id']]:
                cofi[f].add(g['id'])
        for f in ordre:
            cands = [prop[f['id']]] if prop[f['id']] is not None else []
            cands += [best[g] for g in cofi[f['id']]]
            besoin(cands, 'face sans niveau admissible')
            best[f['id']] = min(cands, key=lambda c: c[0])
            f['a'], f['z'] = best[f['id']]
        self.comptes['faces_a_niveau_propre'] = sum(1 for f in F if prop[f['id']] is not None and
                                                    prop[f['id']][0] == f['a'])
        # monotonie (bord)
        for g in F:
            for fid in self.bord[g['id']]:
                besoin(F[fid]['a'] <= g['a'], 'niveau non monotone')
        if certifier:
            n_ok = 0
            for f in F:
                f['w'] = self.certifier(f)
                n_ok += 1
            self.comptes['certificats_kkt'] = n_ok
        self.niveaux = sorted(set(f['a'] for f in F))
        self.comptes['niveaux_distincts'] = len(self.niveaux)
        self.comptes['plateaux_multiples'] = sum(1 for a in self.niveaux
                                                 if sum(1 for f in F if f['a'] == a and f['dim'] == 0) > 1)

    def contraintes(self, f):
        """Description exacte de F_f : (dedans, libres, dehors) ; dedans = I' (ou Q), libres = U'."""
        if f['dim'] == 0:
            Q = self.etiq_list[next(iter(f['som']))]
            return set(Q), set(), set(range(self.n)) - set(Q)
        return set(f['I']), set(f['U']), set(range(self.n)) - set(f['I']) - set(f['U'])

    def face_duale(self, f):
        """Description linéaire exacte (entiers) de F_sigma : (egalites, inegalites), chaque contrainte (n, c) signifie
        n . y = c ou n . y <= c. Sommet Q : V_Q par ses facettes (arêtes de la mosaïque en c_Q) ; sinon E(U') et les
        demi-espaces des sites dedans (I') et dehors (tous, redondances comprises)."""
        P = self.P
        if f['dim'] == 0:
            e = next(iter(f['som']))
            ineg = []
            for q, p in self.voisins_domaine(e):      # |y - q|^2 <= |y - p|^2  <=>  2 y.(p - q) <= |p|^2 - |q|^2
                ineg.append((tuple(2 * (P[p][t] - P[q][t]) for t in range(3)), n2(P[p]) - n2(P[q])))
            return [], ineg
        U = f['U']
        u0 = P[U[0]]
        eg = [(tuple(2 * (u0[t] - P[u][t]) for t in range(3)), n2(u0) - n2(P[u])) for u in U[1:]]
        ineg = []
        for i in f['I']:                                # |y - i|^2 <= |y - u0|^2
            ineg.append((tuple(2 * (u0[t] - P[i][t]) for t in range(3)), n2(u0) - n2(P[i])))
        for o in range(self.n):                         # |y - u0|^2 <= |y - o|^2
            if o not in f['I'] and o not in U:
                ineg.append((tuple(2 * (P[o][t] - u0[t]) for t in range(3)), n2(P[o]) - n2(u0)))
        return eg, ineg

    def certifier(self, f, z=None, a=None):
        """Certificat exact de a_f = min_{F_f} d_k^2 : témoin z dans F_f, d_k(z)^2 = a, multiplicateurs KKT.
        Rend {site : poids} ; lève ErreurOracle si le certificat échoue."""
        P = self.P
        z = f['z'] if z is None else z
        a = f['a'] if a is None else a
        dedans, libres, dehors = self.contraintes(f)
        D, dist = self._hz(z)
        aD = a * D * D
        besoin(aD.denominator == 1, 'KKT : niveau incompatible avec le témoin')
        a_ = aD.numerator
        # z dans F_f
        if f['dim'] == 0:
            besoin(max(dist[q] for q in dedans) == a_, 'KKT : valeur au témoin (sommet)')
            besoin(all(dist[o] >= a_ for o in dehors), 'KKT : témoin hors du domaine (sommet)')
        else:
            besoin(all(dist[u] == a_ for u in libres), 'KKT : témoin hors de E(U\')')
            besoin(all(dist[i] <= a_ for i in dedans), 'KKT : témoin, I\' hors de la boule')
            besoin(all(dist[o] >= a_ for o in dehors), 'KKT : témoin, site extérieur dans la boule')
        besoin(sorted(dist)[self.k - 1] == a_, 'KKT : d_k(z)^2 != a')
        a = a_
        actifs = [i for i in range(self.n) if dist[i] == a]
        # multiplicateurs : z = somme w_s s, somme w = 1, signes
        for t in range(1, min(4, len(actifs)) + 1):
            for T in itertools.combinations(actifs, t):
                pts = [P[i] for i in T]
                if rang_affine(pts) != t - 1:
                    continue
                lam = coeffs_barycentriques(z, pts)
                if lam is None:
                    continue
                ok = True
                for i, l in zip(T, lam):
                    if i in dedans and l < 0:
                        ok = False
                    if i in dehors and l > 0:
                        ok = False
                if ok:
                    return {i: l for i, l in zip(T, lam) if l != 0}
        raise ErreurOracle('KKT : aucun multiplicateur admissible (face %s, %s)' % (f['I'], f['U']))

    def plateaux(self):
        """Niveau -> liste des faces de ce niveau (égalités exactes ; une coupe fermée les prend d'un bloc)."""
        out = {}
        for f in self.faces:
            out.setdefault(f['a'], []).append(f['id'])
        return dict(sorted(out.items()))

    def niveaux_dtm(self):
        """ATTRIBUT (jamais pour activer ni rattacher, note d2be6bdc7 § 3) : b_sigma = min sur F_sigma de f_k^2,
        f_k^2 = moyenne des distances carrées aux k plus proches = puissance du barycentre pondéré. Même schéma que
        a_sigma : minimum non contraint sur aff(F_tau) (projection de c_Q sur E(U'_tau), ou c_Q pour un sommet),
        admissible s'il est dans F_tau fermée, minimum sur les cofaces. Vérifie b <= a <= k b (inégalité de la note).
        Rend {face : b}."""
        P, k = self.P, self.k
        prop = {}
        for f in self.faces:
            e = next(iter(f['som']))
            Q = self.etiq_list[e]
            c = tuple(Fr(sum(P[q][t] for q in Q), k) for t in range(3))
            if f['dim'] == 0:
                y = c
                D, h = self._hz(y)
                ok = max(h[q] for q in Q) <= min((h[i] for i in range(self.n) if i not in Q), default=None) \
                    if len(Q) < self.n else True
            else:
                U = f['U']
                b = base_affine([P[u] for u in U])
                B = [U[i] for i in b]
                u0 = P[B[0]]
                Nn = [tuple(2 * (u0[t] - P[u][t]) for t in range(3)) for u in B[1:]]
                rhs = [n2(u0) - n2(P[u]) for u in B[1:]]
                G = [[dot(a_, b_) for b_ in Nn] for a_ in Nn]
                mu = resoudre(G, [sum(Nn[i][t] * c[t] for t in range(3)) - rhs[i] for i in range(len(Nn))])
                y = tuple(c[t] - sum(mu[i] * Nn[i][t] for i in range(len(Nn))) for t in range(3))
                D, h = self._hz(y)
                hu = h[U[0]]
                besoin(all(h[u] == hu for u in U), 'projection DTM hors de E(U\')')
                dedans, uu = set(f['I']), set(U)
                ok = all((h[i] <= hu) if i in dedans else (h[i] >= hu) for i in range(self.n) if i not in uu)
            prop[f['id']] = (sum(d2(y, P[q]) for q in Q) / k) if ok else None
        cofi = {f['id']: set() for f in self.faces}
        for g in self.faces:
            for fid in self.bord[g['id']]:
                cofi[fid].add(g['id'])
        b = {}
        for f in sorted(self.faces, key=lambda f: -f['dim']):
            cands = ([prop[f['id']]] if prop[f['id']] is not None else []) + [b[g] for g in cofi[f['id']]]
            besoin(cands, 'face sans niveau DTM admissible')
            b[f['id']] = min(cands)
            besoin(b[f['id']] <= f['a'] <= k * b[f['id']], 'inégalité b <= a <= k b violée (face %s, %s)'
                   % (f['I'], f['U']))
        return b

    # -- coupes
    def actifs(self, a):
        return [f['id'] for f in self.faces if f['a'] <= a]

    def composantes(self, a, strict=False):
        """Composantes de A_k(r) à la coupe fermée a = r^2 (ouverte si strict : a_sigma < a). Rend
        (comp, etiq_comp, face_comp) : comp = liste de frozensets d'étiquettes pleines (sommets) ; etiq_comp :
        étiquette (pleine ou non, de toute face active) -> indice de composante ; face_comp : face active -> indice."""
        F = self.faces
        act = [f for f in F if (f['a'] < a if strict else f['a'] <= a)]
        parent = {}

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        for f in act:
            if f['dim'] == 0:
                e = next(iter(f['som']))
                parent[e] = e
        for f in act:
            if f['dim'] == 1:
                s = sorted(f['som'])
                besoin(len(s) == 2, 'arête à %d sommets' % len(s))
                ra, rb = find(s[0]), find(s[1])
                if ra != rb:
                    parent[max(ra, rb)] = min(ra, rb)
        groupes = {}
        for e in parent:
            groupes.setdefault(find(e), set()).add(e)
        comp = [frozenset(g) for _, g in sorted(groupes.items())]
        idx = {e: i for i, g in enumerate(comp) for e in g}
        face_comp, etiq_comp = {}, {}
        for f in act:
            cs = {idx[e] for e in f['som']}
            besoin(len(cs) == 1, 'face active à cheval sur deux composantes')
            c = cs.pop()
            face_comp[f['id']] = c
            for e in f['fam']:
                besoin(etiq_comp.get(e, c) == c, 'étiquette attribuée à deux composantes (C_Q non connexe ?)')
                etiq_comp[e] = c
        return comp, etiq_comp, face_comp

    def couverture(self, a, strict=False):
        """Couverture de chaque composante : réunion des sites de ses étiquettes pleines (formule § 1.3)."""
        comp, _, _ = self.composantes(a, strict)
        return [frozenset(q for e in g for q in self.etiq_list[e]) for g in comp]

    def evenements(self):
        """Événements H0 par plateau (coupe fermée) : naissances, fusions, continuations. Rend la liste des nœuds :
        dict(id, niveau, genre ('naissance' | 'fusion'), enfants, parent, etiquettes (sommets à la naissance))."""
        F = self.faces
        par_niveau = {}
        for f in F:
            if f['dim'] <= 1:
                par_niveau.setdefault(f['a'], []).append(f)
        parent = {}

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        noeud_de = {}       # racine union-find -> nœud vivant
        noeuds = []
        for a in sorted(par_niveau):
            avant = {find(e): noeud_de[find(e)] for e in parent}
            nouveaux = []
            for f in par_niveau[a]:
                if f['dim'] == 0:
                    e = next(iter(f['som']))
                    parent[e] = e
                    nouveaux.append(e)
            for f in par_niveau[a]:
                if f['dim'] == 1:
                    s = sorted(f['som'])
                    ra, rb = find(s[0]), find(s[1])
                    if ra != rb:
                        parent[max(ra, rb)] = min(ra, rb)
            groupes = {}
            for e in parent:
                groupes.setdefault(find(e), set()).add(e)
            ancien = {}
            for r0, nd in avant.items():
                ancien.setdefault(find(r0), set()).add(nd)
            noeud_de = {}
            for r, membres in groupes.items():
                olds = ancien.get(r, set())
                if len(olds) >= 2:
                    nid = len(noeuds)
                    noeuds.append(dict(id=nid, niveau=a, genre='fusion', enfants=sorted(olds), parent=None,
                                       etiquettes=None))
                    for c in olds:
                        noeuds[c]['parent'] = nid
                elif not olds:
                    nid = len(noeuds)
                    noeuds.append(dict(id=nid, niveau=a, genre='naissance', enfants=[], parent=None,
                                       etiquettes=sorted(membres)))
                else:
                    nid = olds.pop()
                noeud_de[r] = nid
        return noeuds

    def strates_maximales(self, a):
        """Cellules actives maximales (aucune coface active) à la coupe a, par dimension : ce que le rendu doit
        dessiner en plus des faces exposées des volumes (strates isolées de dimension 0, 1, 2)."""
        act = {f['id'] for f in self.faces if f['a'] <= a}
        out = {}
        for fid in act:
            if not any(g in act for g in self.cofaces[fid]):
                d = self.faces[fid]['dim']
                out[d] = out.get(d, 0) + 1
        return out

    def faces_exposees(self, a):
        """Faces de dimension 2 actives bordant exactement une cellule de dimension 3 active (rendu du solide)."""
        act = {f['id'] for f in self.faces if f['a'] <= a}
        out = []
        for fid in act:
            if self.faces[fid]['dim'] != 2:
                continue
            n3 = sum(1 for g in self.cofaces[fid] if g in act and self.faces[g]['dim'] == 3)
            if n3 == 1:
                out.append(fid)
        return sorted(out)

    def euler_coupe(self, a):
        return sum((-1) ** f['dim'] for f in self.faces if f['a'] <= a)

    def betti_mod2(self, a):
        """Nombres de Betti mod 2 de A_k(r) (complexe cellulaire régulier : incidence mod 2 = facettes)."""
        act = [f for f in self.faces if f['a'] <= a]
        par_dim = {}
        for f in act:
            par_dim.setdefault(f['dim'], []).append(f['id'])
        idx = {d: {fid: i for i, fid in enumerate(l)} for d, l in par_dim.items()}

        def rang(d):
            if d not in par_dim or d - 1 not in par_dim:
                return 0
            lignes = []
            for fid in par_dim[d]:
                v = 0
                for g in self.bord[fid]:
                    v |= 1 << idx[d - 1][g]
                lignes.append(v)
            rg, piv = 0, {}
            for v in lignes:
                while v:
                    h = v.bit_length() - 1
                    if h in piv:
                        v ^= piv[h]
                    else:
                        piv[h] = v
                        rg += 1
                        break
            return rg
        rk = {d: rang(d) for d in range(0, 5)}
        return [len(par_dim.get(d, [])) - rk.get(d, 0) - rk.get(d + 1, 0) for d in range(self.dim_max + 1)]

    # -- offset exact par pièces convexes
    def voisins_domaine(self, e):
        """Contraintes de V_Q (Q pleine) : paires (q, p) des arêtes de la mosaïque incidentes au sommet."""
        Q = set(self.etiq_list[e])
        out = set()
        for f in self.faces:
            if f['dim'] == 1 and e in f['som']:
                a, b = f['U']
                if a in Q:
                    out.add((a, b))
                else:
                    out.add((b, a))
        return sorted(out)

    def min_max_contraint(self, A, paires, sites=None):
        """min_{y : |y-q| <= |y-p| pour (q, p) dans paires} max_{s dans A} |y - s|^2, exact, par énumération des
        ensembles actifs de taille croissante ; le premier point KKT trouvé est le minimum global (problème convexe :
        y = somme alpha_s s - somme lambda (p - q), alpha >= 0, somme alpha = 1, lambda >= 0, réalisabilité, max
        atteint sur les actifs). Rend (valeur, y). sites : liste de points (par défaut self.P)."""
        P = self.P if sites is None else sites
        A = sorted(set(A))
        for taille in range(1, 5):
            for na in range(1, min(taille, len(A)) + 1):
                nc = taille - na
                for SA in itertools.combinations(A, na):
                    for SC in itertools.combinations(paires, nc):
                        sol = _kkt_actif(P, SA, SC)
                        if sol is None:
                            continue
                        y, val = sol
                        if any(d2(y, P[s]) > val for s in A):
                            continue
                        if any(d2(y, P[q]) > d2(y, P[p]) for q, p in paires):
                            continue
                        return val, y
        raise ErreurOracle('min-max contraint introuvable')

    def dans_offset(self, x, e, a):
        """x (point rationnel) dans C_Q(r) + B_r, Q = étiquette pleine e, r^2 = a : exact. Équivaut à
        min_{y dans V_Q} max_{s dans Q u {x}} |y - s|^2 <= a (V_Q décrit par les arêtes de la mosaïque en c_Q)."""
        Q = list(self.etiq_list[e])
        sites = list(self.P) + [tuple(x)]
        val, _ = self.min_max_contraint(Q + [len(self.P)], self.voisins_domaine(e), sites)
        return val <= a

    def offset(self, a, c):
        """Pièces convexes de l'offset exact de la composante c à la coupe a : liste des étiquettes pleines Q
        attribuées (C_Q(r) non vide) ; C_v(r) + B_r = réunion des C_Q(r) + B_r."""
        comp, _, _ = self.composantes(a)
        return sorted(comp[c])


def _kkt_actif(P, SA, SC):
    """Système linéaire exact d'un ensemble actif (SA : sites actifs du max, SC : paires actives) ; None si singulier
    ou si un multiplicateur est négatif. Rend (y, valeur)."""
    na, nc = len(SA), len(SC)
    nv = 3 + na + nc
    M, b = [], []
    for t in range(3):
        row = [0] * nv
        row[t] = 1
        for i, s in enumerate(SA):
            row[3 + i] = -P[s][t]
        for j, (q, p) in enumerate(SC):
            row[3 + na + j] = P[p][t] - P[q][t]
        M.append(row)
        b.append(0)
    row = [0] * nv
    for i in range(na):
        row[3 + i] = 1
    M.append(row)
    b.append(1)
    s0 = P[SA[0]]
    for s in SA[1:]:
        row = [0] * nv
        for t in range(3):
            row[t] = 2 * (s0[t] - P[s][t])
        M.append(row)
        b.append(n2(s0) - n2(P[s]))
    for q, p in SC:
        row = [0] * nv
        for t in range(3):
            row[t] = 2 * (P[p][t] - P[q][t])
        M.append(row)
        b.append(n2(P[p]) - n2(P[q]))
    if len(M) != nv:
        return None
    sol = resoudre(M, b)
    if sol is None or any(x < 0 for x in sol[3:]):
        return None
    y = tuple(sol[:3])
    return y, d2(y, P[SA[0]])


# ------------------------------------------------------------------------------------------------ juge des réductions

def diametre2(M, fid):
    """Diamètre carré exact de la cellule (sommets = barycentres des étiquettes pleines)."""
    som = [tuple(Fr(c, M.k) for c in M.somme_etiq(e)) for e in M.faces[fid]['som']]
    return max((d2(a, b) for a in som for b in som), default=Fr(0))


def diametres2_composantes(M, a, faces=None):
    """D_v(r)^2 = max des diamètres carrés des cellules actives de chaque composante (faces : sous-ensemble, par
    défaut A_k(r) entier). Borne de Hausdorff d_H(L_v, A_v) <= D_v quand L garde tous les sommets (note 28d70f8ab)."""
    comp, _, face_comp = M.composantes(a)
    out = [Fr(0)] * len(comp)
    for fid, c in face_comp.items():
        if faces is None or fid in faces:
            out[c] = max(out[c], diametre2(M, fid))
    return out


def verifier_effondrements(M, paires, proteger_sommets=True):
    """Juge indépendant d'une liste ordonnée d'effondrements élémentaires (sigma, tau) sur la mosaïque ENTIÈRE
    (toute la plage de filtration) : sigma facette de tau, mêmes niveaux exacts, sigma libre (tau est sa seule coface
    restante), sommets protégés. Rend L (ensemble des faces restantes, fermé par faces). Lève ErreurOracle au premier
    défaut. Ce certificat donne, pour tout r, A_k(r) qui s'effondre sur L_r = L inter A_k(r) (Forman, effondrements
    élémentaires d'un complexe cellulaire régulier)."""
    K = {f['id'] for f in M.faces}
    for i, (s, t) in enumerate(paires):
        besoin(s in K and t in K, 'paire %d : cellule absente ou déjà retirée' % i)
        besoin(s in M.bord[t], 'paire %d : sigma n\'est pas une facette de tau' % i)
        besoin(M.faces[s]['a'] == M.faces[t]['a'], 'paire %d : niveaux différents' % i)
        besoin(not (proteger_sommets and M.faces[s]['dim'] == 0), 'paire %d : sommet protégé retiré' % i)
        besoin({g for g in M.cofaces[s] if g in K} == {t}, 'paire %d : sigma n\'est pas libre' % i)
        K.discard(s)
        K.discard(t)
    for fid in K:
        besoin(all(g in K for g in M.bord[fid]), 'L non fermé par faces')
    return K


def juger_sous_complexe(M, a, L):
    """Juge d'un représentant proposé L (ensemble d'identifiants de faces, d'une coupe ou de toute la plage) à la coupe
    a, par L_r = L inter A_k(r) : fermé par faces ; pour chaque composante de A_k(r) : sommets tous gardés ?, même nombre de composantes de L dans la
    composante (1), mêmes Betti mod 2 (global), D_v^2 (borne de Hausdorff L <-> A si les sommets sont gardés).
    Rend un dict de constats (ne lève pas : c'est un juge)."""
    A = {f['id'] for f in M.faces if f['a'] <= a}
    L0 = set(L)
    L = L0 & A                       # un L de toute la plage est jugé par sa coupe L_r = L inter A_k(r)
    out = dict(inclus=L0 <= A, ferme=all(g in L for fid in L for g in M.bord[fid]))
    comp, _, face_comp = M.composantes(a)
    sommets_A = {fid for fid in A if M.faces[fid]['dim'] == 0}
    out['sommets_gardes'] = sommets_A <= L
    # composantes de L restreintes à chaque composante de A
    parent = {fid: fid for fid in L if M.faces[fid]['dim'] == 0}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    for fid in L:
        if M.faces[fid]['dim'] == 1:
            s = [M.cle[(M.etiq_list[e], ())] for e in M.faces[fid]['som']]
            if all(x in parent for x in s):
                ra, rb = find(s[0]), find(s[1])
                if ra != rb:
                    parent[ra] = rb
    par_comp = {}
    for fid in parent:
        par_comp.setdefault(face_comp.get(fid), set()).add(find(fid))
    out['composantes_par_noeud'] = [len(par_comp.get(c, ())) for c in range(len(comp))]
    out['pi0_egal'] = all(x == 1 for x in out['composantes_par_noeud'])

    def betti(faces):
        par_dim = {}
        for fid in faces:
            par_dim.setdefault(M.faces[fid]['dim'], []).append(fid)
        idx = {d: {fid: i for i, fid in enumerate(l)} for d, l in par_dim.items()}

        def rang(d):
            if d not in par_dim or d - 1 not in par_dim:
                return 0
            piv, rg = {}, 0
            for fid in par_dim[d]:
                v = 0
                for g in M.bord[fid]:
                    if g in idx[d - 1]:
                        v |= 1 << idx[d - 1][g]
                while v:
                    h = v.bit_length() - 1
                    if h in piv:
                        v ^= piv[h]
                    else:
                        piv[h] = v
                        rg += 1
                        break
            return rg
        rk = {d: rang(d) for d in range(5)}
        return [len(par_dim.get(d, [])) - rk[d] - rk.get(d + 1, 0) for d in range(4)]
    out['betti_A'], out['betti_L'] = betti(A), betti(L)
    out['betti_egal'] = out['betti_A'] == out['betti_L']
    out['D2_par_noeud'] = [str(x) for x in diametres2_composantes(M, a)]
    return out


def reduction_gloutonne(M, proteger_sommets=True):
    """Générateur de TEST (premier prototype de la note 28d70f8ab, § 3) : paires libres de même niveau, de plus grande
    dimension d'abord, puis de plus petit diamètre, départagées par les clés triées. Reproductible, pas canonique,
    aucune minimalité revendiquée. Rend la liste des paires (à passer au juge verifier_effondrements)."""
    K = {f['id'] for f in M.faces}
    paires = []
    diam = {}
    while True:
        cand = []
        for t in K:
            ft = M.faces[t]
            if ft['dim'] == 0:
                continue
            for s in M.bord[t]:
                fs = M.faces[s]
                if fs['a'] != ft['a'] or (proteger_sommets and fs['dim'] == 0):
                    continue
                if {g for g in M.cofaces[s] if g in K} != {t}:
                    continue
                if t not in diam:
                    diam[t] = diametre2(M, t)
                cand.append((-ft['dim'], diam[t], fs['I'], fs['U'], ft['I'], ft['U'], s, t))
        if not cand:
            break
        cand.sort()
        s, t = cand[0][-2], cand[0][-1]
        paires.append((s, t))
        K.discard(s)
        K.discard(t)
    return paires


# ------------------------------------------------------------------------------------------------ robustesse (pi0)

def racine_le(x, a, d2):
    """sqrt(x) <= sqrt(a) + sqrt(d2), décidé exactement (x, a, d2 rationnels >= 0)."""
    lhs = Fr(x) - a - d2
    if lhs <= 0:
        return True
    return lhs * lhs <= 4 * Fr(a) * d2


def coupe_sous(M, a, d2=0):
    """Plus grand niveau critique de M au plus (sqrt(a) + sqrt(d2))^2 (exact) ; None s'il n'y en a pas."""
    best = None
    for x in M.niveaux:
        if racine_le(x, a, d2):
            best = x
    return best


def application_temoins(M_src, a_src, M_dst, d2=0):
    """Application pi0(A^{src}_k(r)) -> pi0(A^{dst}_{k'}(r + delta)), delta^2 = d2, induite par une inclusion des
    multi-couvertures (déplacement apparié <= delta : d_k^{P'} <= d_k^P + delta ; ajouts de m sites : Omega_k^P dans
    Omega_k^{P u O} dans Omega_{k-m}^P ; suppressions ; ordres voisins). Chaque sommet actif de src est représenté par
    son témoin z (dans C_Q(r)) ; z est envoyé sur la face de dst qui le contient dans son intérieur relatif, dont on
    vérifie l'activité à la coupe cible ; l'application doit être bien définie (une seule image par composante).
    Rend (images, coupe cible). Lève ErreurOracle si l'inclusion supposée est violée ou si l'application est mal
    définie."""
    comp, _, _ = M_src.composantes(a_src)
    cible = coupe_sous(M_dst, a_src, d2)
    if not comp:
        return [], cible
    besoin(cible is not None, 'application : aucune coupe cible (inclusion violée)')
    _, _, face_dst = M_dst.composantes(cible)
    images = []
    for g in comp:
        im = set()
        for e in g:
            z = M_src.faces[M_src.cle[(M_src.etiq_list[e], ())]]['z']
            In, On, dk = M_dst.N.structure(z, M_dst.k)
            besoin(racine_le(dk, a_src, d2), 'application : témoin hors de la multi-couverture cible')
            if len(In) + len(On) == M_dst.k:
                fid = M_dst.cle[(tuple(sorted(In + On)), ())]
            else:
                fid = M_dst.cle[(In, On)]
            besoin(fid in face_dst, 'application : face cible inactive')
            im.add(face_dst[fid])
        besoin(len(im) == 1, 'application : composante envoyée sur %d composantes' % len(im))
        images.append(im.pop())
    return images, cible


# ------------------------------------------------------------------------------------------------ Gamma_K (thèse)

def gamma_k(P, K, a, beta=None):
    """Composantes de Gamma_K(r) (thèse, Déf. 21) à la coupe fermée a = r^2 : sommets = K-parties de boule minimale
    <= a, liaisons = (K+1)-parties de boule minimale <= a (toutes leurs K-faces reliées). Rend (composantes :
    liste de frozensets de K-parties, polyèdres : réunions de sites). Exhaustif : borné (C(n, K+1) boules)."""
    n = len(P)
    if beta is None:
        beta = {}

    def b(F):
        if F not in beta:
            beta[F] = meb([P[i] for i in F])[1]
        return beta[F]
    som = [F for F in itertools.combinations(range(n), K) if b(F) <= a]
    parent = {F: F for F in som}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    if K < n:
        for G in itertools.combinations(range(n), K + 1):
            if b(G) <= a:
                fs = [tuple(x for x in G if x != s) for s in G]
                for f in fs[1:]:
                    ra, rb = find(fs[0]), find(f)
                    if ra != rb:
                        parent[ra] = rb
    groupes = {}
    for F in som:
        groupes.setdefault(find(F), set()).add(F)
    comps = [frozenset(g) for g in groupes.values()]
    return comps, [frozenset(i for F in g for i in F) for g in comps]


# ------------------------------------------------------------------------------------------------ démonstration

def _demo():
    pts = [(0, 0, 0), (1, 0, 0), (10, 0, 0)]
    N = Nuage(pts, 3)
    M = N.ordre(3)
    print('{0,1,10} k=3 :', [(f['dim'], f['I'], f['U'], str(f['a'])) for f in M.faces])
    tet = [(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)]
    M = Nuage(tet, 2).ordre(2)
    print('tétraèdre k=2 : f-vecteur', M.comptes['f_vecteur'])


def _cli(argv):
    """python oracle_ak.py --points '[[0,0,0],[1,0,0],[10,0,0]]' --k 3 [--coupe 25] [--faces]
    (ou --fichier points.json) : résumé JSON exact (niveaux en fractions), composantes et couverture à la coupe."""
    import argparse
    import json
    ap = argparse.ArgumentParser()
    ap.add_argument('--points')
    ap.add_argument('--fichier')
    ap.add_argument('--k', type=int, required=True)
    ap.add_argument('--coupe', help='niveau a = r^2 (fraction p/q acceptée)')
    ap.add_argument('--faces', action='store_true')
    a = ap.parse_args(argv)
    pts = json.loads(open(a.fichier).read() if a.fichier else a.points)
    M = Nuage([tuple(p) for p in pts], a.k).ordre(a.k)
    out = dict(k=a.k, n=len(pts), comptes=M.comptes, niveaux=[str(x) for x in M.niveaux],
               evenements=[dict(n, niveau=str(n['niveau'])) for n in M.evenements()])
    if a.faces:
        out['faces'] = [dict(id=f['id'], dim=f['dim'], I=f['I'], U=f['U'], a=str(f['a']),
                             z=[str(c) for c in f['z']], w={str(i): str(v) for i, v in (f['w'] or {}).items()})
                        for f in M.faces]
    if a.coupe:
        cut = Fr(a.coupe)
        comp, etiq, _ = M.composantes(cut)
        out['coupe'] = dict(a=str(cut), composantes=[[M.etiq_list[e] for e in sorted(g)] for g in comp],
                            couverture=[sorted(c) for c in M.couverture(cut)], betti_mod2=M.betti_mod2(cut),
                            strates_maximales=M.strates_maximales(cut))
    print(json.dumps(out, default=str))


if __name__ == '__main__':
    import sys
    if '--demo' in sys.argv:
        _demo()
    elif len(sys.argv) > 1:
        _cli(sys.argv[1:])
