#!/usr/bin/env python3
"""Budget de bits des predicats du generateur v11, en fonction de l'etendue locale e (bits) d'une feuille.

Regle F2 / F6 de docs/ARCHITECTURE.md : la borne porte sur la somme des valeurs absolues des termes developpes,
coefficients compris (M(a + b) = M(a) + M(b), M(a b) = M(a) M(b)). Les feuilles de l'expression sont des entiers
exacts de valeur absolue < 2^e : coordonnees locales y (relatives au coin bas de la boite), cotes h de la boite,
differences de sites u, v, s, d (bornees par la largeur de la liste, certifiee sur la liste meme).
Chaque noeud porte (coef, deg) avec M < coef * 2^(e * deg), et l'exposition E de la regle F6
(E(feuille) = 0, E(a + b) = max + 1, E(a b) = Ea + Eb + 1), calculee sur l'arbre binaire le plus defavorable.
Sortie : pour chaque valeur comparee par un predicat, bits(e) = deg * e + log2(coef), et le plus grand e tel que
la valeur tienne dans binaire64 exact (< 2^53), i64 (< 2^63), i128 (< 2^127).
"""
from fractions import Fraction
from math import log2, floor


class X:
    def __init__(self, coef, deg, expo=0, name=""):
        self.coef, self.deg, self.expo, self.name = Fraction(coef), deg, expo, name

    def __add__(self, o):
        assert self.deg == o.deg, (self.name, o.name)
        return X(self.coef + o.coef, self.deg, max(self.expo, o.expo) + 1)

    __sub__ = __add__

    def __mul__(self, o):
        if isinstance(o, int):  # coefficient entier exact (compte dans M ; puissance de deux : sans arrondi)
            return X(self.coef * o, self.deg, self.expo)
        return X(self.coef * o.coef, self.deg + o.deg, self.expo + o.expo + 1)

    __rmul__ = __mul__


def leaf():
    return X(1, 1, 0)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def vec():
    return [leaf(), leaf(), leaf()]


rows = []


def emit(pred, what, x):
    lg = log2(x.coef)
    lim = lambda bits: floor((bits - lg) / x.deg)
    rows.append((pred, what, x.deg, float(x.coef), lg, lim(53), lim(63), lim(127), x.expo))


# dominance (forme D-loc) : A = |y|^2, P_k = 2 h_k y_k ; test A_i - A_j > somme_k max(0, P_k(i) - P_k(j))
y, h = vec(), vec()
A = dot(y, y)
P = [2 * (h[k] * y[k]) for k in range(3)]
dP = [P[k] - P[k] for k in range(3)]
emit("dominance (noeud, feuille)", "A_i - A_j", A - A)
emit("dominance (noeud, feuille)", "somme des max(0, P_i - P_j)", dP[0] + dP[1] + dP[2])
# milieu dans la boite : y_i + y_j contre 0 et 2 h
emit("milieu dans la boite (q2)", "y_i + y_j ; 2 h", y[0] + y[0])
# aigu : u.v > 0, u.v < u.u, u.v < v.v
u, v, s, d, a = vec(), vec(), vec(), vec(), vec()
emit("triangle aigu (q3)", "u.v, u.u, v.v", dot(u, v))
# droite des equidistants (lemme Z, variante mesuree seulement)
c = cross(u, v)
Pz = 2 * dot(u, [y[0] + y[0] - h[0], y[1] + y[1] - h[1], y[2] + y[2] - h[2]])
l = v[0] * Pz - u[0] * Pz
r = 2 * (h[1] * c[0] + h[2] * c[1])
emit("droite des equidistants (variante)", "l = v_k P_0 - u_k P_1", l)
emit("droite des equidistants (variante)", "r = 2 somme h_j |c_kj|", r)
# centre q3 : w = u x v, t = uu v - vv u, N = t x w, D = 2 |w|^2 ; boite : 0 <= a D + N < h D
uu, vv = dot(u, u), dot(v, v)
w = cross(u, v)
t = [uu * v[k] - vv * u[k] for k in range(3)]
N3 = cross(t, w)
ww = dot(w, w)
D3 = 2 * ww
emit("centre q3", "w = u x v", w[0])
emit("centre q3", "t = uu v - vv u", t[0])
emit("centre q3", "N (numerateur)", N3[0])
emit("centre q3", "D = 2 |w|^2", D3)
emit("centre q3 dans la boite", "a D + N", a[0] * D3 + N3[0])
emit("centre q3 dans la boite", "h D", h[0] * D3)
# centre q4 : D = 2 det(u, v, s), N = uu (v x s) + vv (s x u) + ss (u x v)
ss = dot(s, s)
c1, c2, c3 = cross(v, s), cross(s, u), cross(u, v)
det = dot(u, c1)
D4 = 2 * det
N4 = [uu * c1[k] + vv * c2[k] + ss * c3[k] for k in range(3)]
emit("centre q4", "det(u, v, s)", det)
emit("centre q4", "N (numerateur)", N4[0])
emit("centre q4 dans la boite", "a D + N", a[0] * D4 + N4[0])
emit("centre q4 dans la boite", "h D", h[0] * D4)
# interieur strict du tetraedre (Gram) : mineurs g, A = g11 uu + g12 vv + g13 ss, D2 = 2 det G
uv, us, vs = dot(u, v), dot(u, s), dot(v, s)
g11, g12, g13 = vv * ss - vs * vs, vs * us - uv * ss, uv * vs - vv * us
g22, g23, g33 = uu * ss - us * us, us * uv - uu * vs, uu * vv - uv * uv
Aa = g11 * uu + g12 * vv + g13 * ss
Bb = g12 * uu + g22 * vv + g23 * ss
Cc = g13 * uu + g23 * vv + g33 * ss
detG = uu * g11 + uv * g12 + us * g13
emit("interieur strict du tetraedre (q4)", "mineur de Gram", g11)
emit("interieur strict du tetraedre (q4)", "A (barycentrique x 2 det^2)", Aa)
emit("interieur strict du tetraedre (q4)", "A + B + C", Aa + Bb + Cc)
emit("interieur strict du tetraedre (q4)", "2 det G", 2 * detG)
emit("interieur strict du tetraedre (q4)", "2 det G - A - B - C (forme filtree)", 2 * detG - Aa - Bb - Cc)
# recensement : cote d'un site z, d = z - a
dd = dot(d, d)
emit("recensement q2", "|d|^2 ; n.d", dd)
emit("recensement q3", "D |d|^2", D3 * dd)
emit("recensement q3", "2 N.d", 2 * dot(N3, d))
emit("recensement q3", "D |d|^2 - 2 N.d (forme filtree)", D3 * dd - 2 * dot(N3, d))
emit("recensement q4", "D |d|^2", D4 * dd)
emit("recensement q4", "2 N.d", 2 * dot(N4, d))
emit("recensement q4", "D |d|^2 - 2 N.d (forme filtree)", D4 * dd - 2 * dot(N4, d))
# niveaux (differences globales, e = B)
emit("niveau q2", "|u|^2 (den 4)", uu)
emit("niveau q3", "num = uu vv dd", uu * vv * dot(d, d))
emit("niveau q3", "den = 4 |w|^2", 4 * ww)
emit("niveau q4", "num = |N|^2", dot(N4, N4))
emit("niveau q4", "den = D^2", D4 * D4)

print("| Predicat | Valeur | Degre | Coefficient | bits(e) | e max binaire64 | e max i64 | e max i128 | Exposition E |")
print("|---|---|---:|---:|---|---:|---:|---:|---:|")
for pred, what, deg, coef, lg, e53, e63, e127, expo in rows:
    print(f"| {pred} | {what} | {deg} | {coef:g} | {deg} e + {lg:.2f} | {e53} | {e63} | {e127} | {expo} |")
