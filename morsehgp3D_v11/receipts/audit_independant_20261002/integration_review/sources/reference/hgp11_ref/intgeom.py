"""Predicats geometriques entiers exacts de l'etage B : les formules du moteur, en entiers Python (sans borne).

Port explicite de la v10 (raccord R2) : src/arith/geometry.hpp et geometry.cpp (centres, cote, orientations, formes
de niveau), src/catalogue/support.hpp (support canonique, appartenance du centre a une enveloppe convexe fermee),
src/cloud/cloud.cpp (cle de Morton). Les entiers Python sont exacts : les budgets de bits du moteur (i64, i128,
entiers larges) n'ont pas d'equivalent ici, et aucune garde de debordement n'est necessaire.

Une sphere est donnee par un point d'ancrage a (sur la sphere) et son centre rationnel c = a + N / D, D > 0 :
  1 point  : N = 0, D = 1 (rayon nul) ;
  2 points : N = b - a, D = 2 ;
  3 points : N = ((|u|^2 v - |v|^2 u) x (u x v)), D = 2 |u x v|^2, u = b - a, v = c - a ;
  4 points : N = |u|^2 (v x s) + |v|^2 (s x u) + |s|^2 (u x v), D = 2 det(u, v, s), signes ramenes a D > 0.
Cote d'un point z : signe de D |z - a|^2 - 2 N . (z - a) = D (|z - c|^2 - r^2).
Niveau r^2 = |N|^2 / D^2 pour toute sphere (l'ancre est sur la sphere).
"""
from fractions import Fraction

MORTON_BITS = 21  # cle de Morton du moteur sur 63 bits


def morton(p):
    """Cle de Morton du moteur : x au bit 0, y au bit 1, z au bit 2 de chaque niveau (cloud.cpp, morton3)."""
    key = 0
    for axis in range(3):
        v = p[axis]
        for bit in range(MORTON_BITS):
            key |= ((v >> bit) & 1) << (3 * bit + axis)
    return key


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def center1(_a):
    return (0, 0, 0), 1


def center2(a, b):
    return sub(b, a), 2


def center3(a, b, c):
    """Centre du cercle circonscrit, relatif a a ; None si les trois points sont alignes."""
    u, v = sub(b, a), sub(c, a)
    w = cross(u, v)
    if w == (0, 0, 0):
        return None
    uu, vv = dot(u, u), dot(v, v)
    t = (uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2])
    return cross(t, w), 2 * dot(w, w)


def center4(a, b, c, d):
    """Centre de la sphere circonscrite, relatif a a ; None si les quatre points sont coplanaires."""
    u, v, s = sub(b, a), sub(c, a), sub(d, a)
    vs, su, uv = cross(v, s), cross(s, u), cross(u, v)
    det = dot(u, vs)
    if det == 0:
        return None
    uu, vv, ss = dot(u, u), dot(v, v), dot(s, s)
    n = tuple(uu * vs[i] + vv * su[i] + ss * uv[i] for i in range(3))
    big = 2 * det
    if big < 0:
        return (-n[0], -n[1], -n[2]), -big
    return n, big


def through(pts):
    """Sphere circonscrite de 1 a 4 points : (N, D) relatif a pts[0], ou None s'ils sont affinement dependants."""
    if len(pts) == 1:
        return center1(pts[0])
    if len(pts) == 2:
        return None if pts[0] == pts[1] else center2(pts[0], pts[1])
    if len(pts) == 3:
        return center3(pts[0], pts[1], pts[2])
    return center4(pts[0], pts[1], pts[2], pts[3])


def side_key(anchor, center, z):
    """D (|z - c|^2 - r^2) : < 0 interieur strict, 0 coquille, > 0 exterieur."""
    n, big = center
    d = sub(z, anchor)
    return big * dot(d, d) - 2 * dot(n, d)


def sphere_key(anchor, center):
    """Identite d'une sphere : (centre en fractions, rayon carre en fraction)."""
    n, big = center
    return (tuple(Fraction(anchor[i] * big + n[i], big) for i in range(3)), Fraction(dot(n, n), big * big))


def orient(p, q, r, s):
    """Signe de det[q - p, r - p, s - p]."""
    v = dot(cross(sub(q, p), sub(r, p)), sub(s, p))
    return (v > 0) - (v < 0)


def orient_center(p, q, r, anchor, center):
    """Signe de w . (c - p), w = (q - p) x (r - p), pour le centre c = anchor + N / D."""
    n, big = center
    w = cross(sub(q, p), sub(r, p))
    v = sum(w[i] * (n[i] + big * (anchor[i] - p[i])) for i in range(3))
    return (v > 0) - (v < 0)


def acute(a, b, c):
    """Triangle strictement aigu : son centre circonscrit est strictement interieur."""
    return dot(sub(b, a), sub(c, a)) > 0 and dot(sub(a, b), sub(c, b)) > 0 and dot(sub(a, c), sub(b, c)) > 0


def non_obtuse(a, b, c):
    return dot(sub(b, a), sub(c, a)) >= 0 and dot(sub(a, b), sub(c, b)) >= 0 and dot(sub(a, c), sub(b, c)) >= 0


def is_midpoint(a, b, anchor, center):
    """Le centre est le milieu de a et b : 2 (anchor D + N) = (a + b) D."""
    n, big = center
    return all(2 * (anchor[i] * big + n[i]) == (a[i] + b[i]) * big for i in range(3))


def tetra_position(t, anchor, center):
    """Position du centre dans le tetraedre non degenere t : 1 strictement interieur, 0 sur le bord, -1 dehors."""
    on_boundary = False
    for f in range(4):
        p0, p1, p2 = t[(f + 1) % 4], t[(f + 2) % 4], t[(f + 3) % 4]
        so = orient(p0, p1, p2, t[f])
        sc = orient_center(p0, p1, p2, anchor, center)
        if sc == 0:
            on_boundary = True
        elif sc != so:
            return -1
    return 0 if on_boundary else 1


def level2(a, b):
    """Forme du moteur pour une paire : |u|^2 / 4."""
    u = sub(b, a)
    return dot(u, u), 4


def level3(a, b, c):
    """Forme du moteur pour un triangle : |u|^2 |v|^2 |u - v|^2 / (4 |u x v|^2)."""
    u, v, d = sub(b, a), sub(c, a), sub(c, b)
    w = cross(u, v)
    return dot(u, u) * dot(v, v) * dot(d, d), 4 * dot(w, w)


def level4(center):
    """Forme du moteur pour un tetraedre : |N|^2 / D^2."""
    n, big = center
    return dot(n, n), big * big


def canonical_support(shell, anchor, center):
    """q_min et support canonique d'une sphere de rayon non nul, donnee par sa coquille (positions, dans l'ordre des
    sites) : le plus petit support, puis le premier dans l'ordre lexicographique des rangs. Rend (q_min, indices dans
    shell), ou None si le centre n'est dans l'interieur relatif d'aucune partie affinement independante.
      paire     : le centre est le milieu ;
      triangle  : non aligne, strictement aigu, centre dans son plan (c'est alors son centre circonscrit) ;
      tetraedre : non degenere, centre strictement interieur."""
    m = len(shell)
    for i in range(m):
        for j in range(i + 1, m):
            if is_midpoint(shell[i], shell[j], anchor, center):
                return 2, (i, j)
    for i in range(m):
        for j in range(i + 1, m):
            for k in range(j + 1, m):
                a, b, c = shell[i], shell[j], shell[k]
                if cross(sub(b, a), sub(c, a)) == (0, 0, 0) or not acute(a, b, c):
                    continue
                if orient_center(a, b, c, anchor, center) == 0:
                    return 3, (i, j, k)
    for i in range(m):
        for j in range(i + 1, m):
            for k in range(j + 1, m):
                for l in range(k + 1, m):
                    t = (shell[i], shell[j], shell[k], shell[l])
                    if orient(t[0], t[1], t[2], t[3]) != 0 and tetra_position(t, anchor, center) == 1:
                        return 4, (i, j, k, l)
    return None


def hull_witnesses(shell, anchor, center):
    """Masques (bit i = shell[i]) des parties de la coquille d'une sphere de rayon non nul dont l'enveloppe convexe
    FERMEE contient le centre et qui suffisent a le decider (Caratheodory) : paires antipodales, triangles non obtus
    dont le plan contient le centre, tetraedres non degeneres qui le contiennent, bord compris. Une partie A de la
    coquille contient le centre dans son enveloppe fermee si et seulement si elle contient l'un de ces temoins."""
    m = len(shell)
    out = []
    for i in range(m):
        for j in range(i + 1, m):
            if is_midpoint(shell[i], shell[j], anchor, center):
                out.append(1 << i | 1 << j)
    for i in range(m):
        for j in range(i + 1, m):
            for k in range(j + 1, m):
                a, b, c = shell[i], shell[j], shell[k]
                if cross(sub(b, a), sub(c, a)) == (0, 0, 0) or not non_obtuse(a, b, c):
                    continue
                if orient_center(a, b, c, anchor, center) == 0:
                    out.append(1 << i | 1 << j | 1 << k)
    for i in range(m):
        for j in range(i + 1, m):
            for k in range(j + 1, m):
                for l in range(k + 1, m):
                    t = (shell[i], shell[j], shell[k], shell[l])
                    if orient(t[0], t[1], t[2], t[3]) != 0 and tetra_position(t, anchor, center) >= 0:
                        out.append(1 << i | 1 << j | 1 << k | 1 << l)
    return out
