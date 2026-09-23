"""Audit C, lentille 3 : bornes 18 bits des predicats q3/q4 (constantes)
et echantillonnage adverse exact. Aucun moteur n'est execute."""
import random
from fractions import Fraction as Fr

M = 262143
def bits(x): return abs(x).bit_length()
checks = []
def chk(name, value, limit_bits):
    ok = bits(value) <= limit_bits and abs(value) < (1 << limit_bits)
    checks.append((name, bits(value), limit_bits, ok))

# A. Majorants analytiques (formules du code, M=262143)
chk("ExactBall q3 power 360M^6 (commentaire)", 360 * M**6, 117)
chk("ExactBall q3 power 216M^6 (recalcul)", 216 * M**6, 117)
chk("make_q4 remaining 396M^6", 396 * M**6, 117)
chk("make_q4 power 180M^5", 180 * M**5, 98)
chk("q3_center |det|<=512M^6", 512 * M**6, 117)
chk("q3_center num<=480M^6", 480 * M**6, 117)
chk("q3_center p,q<=32M^4", 32 * M**4, 77)
chk("dead-lane form: 2^20*15M^2 + 2*8M^2*2^21", (1 << 20) * 15 * M**2 + 2 * 8 * M**2 * (1 << 21), 62)
chk("dead-lane outside: 3*3*(2*M*2^21)^2", 9 * (2 * M * (1 << 21))**2, 84)
chk("node_bounds: 3*(2^20*(2M)^2+2*2M*2*M*2^21)+2^20*3M^2", 3 * ((1 << 20) * (2*M)**2 + 2*(2*M)*(2*M*(1 << 21))) + (1 << 20) * 3 * M**2, 62)
chk("witness 16*Xi_high<=192M^4", 192 * M**4, 80)
chk("witness 3*(4H)^2<=27M^4", 27 * M**4, 77)
chk("compare_roots 72M^5", 72 * M**5, 97)
chk("family power 144M^6", 144 * M**6, 117)
chk("tower q3 level num 27M^6", 27 * M**6, 113)
chk("tower q3 level den 36M^4", 36 * M**4, 79)
chk("tower q3 level cross 27M^6*36M^4", 27 * M**6 * 36 * M**4, 192)
chk("tower q4 |N'|^2 <= 3(72M^4)^2", 3 * (72 * M**4)**2, 160)
chk("tower q4 det^2 <= (48M^3)^2", (48 * M**3)**2, 120)
chk("tower q4 cross <= 2^160*2^120", 3 * (72 * M**4)**2 * (48 * M**3)**2, 280)
chk("tower q4 B <= 240M^4 (entete dit <2^74)", 240 * M**4, 81)
chk("tower q4 C <= 576M^5 (entete dit <2^90)", 576 * M**5, 100)
chk("tower q4 center-side det3 <= 720M^6", 720 * M**6, 118)

# B. Echantillonnage adverse exact des formules du code
def sub(p, q): return tuple(p[i] - q[i] for i in range(3))
def dot(p, q): return sum(p[i] * q[i] for i in range(3))
def cross(p, q): return (p[1]*q[2]-p[2]*q[1], p[2]*q[0]-p[0]*q[2], p[0]*q[1]-p[1]*q[0])
EXT = [0, 1, M // 3, M // 2, M - 1, M]
rng = random.Random(20260923)
def rpoint():
    return tuple(rng.choice(EXT) if rng.random() < 0.6 else rng.randrange(M + 1) for _ in range(3))

maxima = {}
def rec(k, v): maxima[k] = max(maxima.get(k, 0), bits(v))
def primitive(v):
    from math import gcd
    g = 0
    for c in v: g = gcd(g, abs(c))
    return tuple(c // g for c in v)
def exact_ball_q3(a, b, x):
    d, u = sub(b, a), sub(x, a)
    dd, uu, du = dot(d, d), dot(u, u), dot(d, u)
    if du <= 0 or dd - du <= 0 or uu - du <= 0: return None
    g = dd * uu - du * du
    if g <= 0: return None
    ad, au = uu * (dd - du), dd * (uu - du)
    lin = tuple(ad * d[i] + au * u[i] for i in range(3))
    rec("q3 G", g); [rec("q3 W", w) for w in lin]
    A = g; B = tuple(-2 * g * a[i] - lin[i] for i in range(3)); C = g * dot(a, a) + dot(lin, a)
    rec("q3 A", A); [rec("q3 B", t) for t in B]; rec("q3 C", C)
    return (A, B, C, g, lin)
def power(ball, z):
    A, B, C = ball[0], ball[1], ball[2]
    return A * dot(z, z) + dot(B, z) + C
def basis(a, b):
    v = sub(b, a); main = 0
    for i in range(3):
        if abs(v[i]) > abs(v[main]): main = i
    ii, jj = (main + 1) % 3, (main + 2) % 3
    h = abs(v[main]); s = 1 if v[main] > 0 else -1
    A = [0, 0, 0]; B = [0, 0, 0]
    A[ii] = h; A[main] = -s * v[ii]; B[jj] = h; B[main] = -s * v[jj]
    return v, tuple(A), tuple(B)
def form(a, b, z, A, B):
    v = sub(b, a); w = tuple(2 * z[i] - a[i] - b[i] for i in range(3))
    return (dot(w, w) - dot(v, v), -2 * dot(w, A), -2 * dot(w, B))

n_q3 = n_q4 = 0
for trial in range(60000):
    a, b, x = rpoint(), rpoint(), rpoint()
    if len({a, b, x}) < 3: continue
    ball = exact_ball_q3(a, b, x)
    if ball is None: continue
    n_q3 += 1
    for _ in range(3):
        z = rpoint(); rec("q3 power(z)", power(ball, z))
    # centre q3 en coordonnees de cellule (formule Q4LocalGeometry::q3_center)
    v, A, B = basis(a, b)
    c0, fx, fy = form(a, b, x, A, B)
    aa, ab, bb = dot(A, A), dot(A, B), dot(B, B)
    p = aa * fy - ab * fx; q = ab * fy - bb * fx; det = fx * q - fy * p
    rec("q3c |c0|", c0); rec("q3c fx", fx); rec("q3c p", p); rec("q3c q", q); rec("q3c det", det)
    rec("q3c numx", c0 * q); rec("q3c numy", c0 * p)
    # recoupement : m + (u1 A + u2 B)/2 == -B/(2A) de la boule exacte
    u1, u2 = Fr(-c0 * q, det), Fr(c0 * p, det)
    centre_cell = tuple(Fr(a[i] + b[i], 2) + (u1 * A[i] + u2 * B[i]) / 2 for i in range(3))
    centre_ball = tuple(Fr(-ball[1][i], 2 * ball[0]) for i in range(3))
    assert centre_cell == centre_ball, "q3 centre mismatch"
    # lemme : |c-m|^2 <= D/12 si ab proprietaire (plus longue)
    D = dot(v, v); E = dot(sub(x, a), sub(x, a)); F2 = dot(sub(x, b), sub(x, b))
    if E <= D and F2 <= D:
        m = tuple(Fr(a[i] + b[i], 2) for i in range(3))
        cm2 = sum((centre_ball[i] - m[i])**2 for i in range(3))
        assert cm2 <= Fr(D, 12), "q3 disk lemma"
        assert 3 * (u1*u1*aa + 2*u1*u2*ab + u2*u2*bb) <= D, "q3 disk in cell coords"
        assert abs(u1) <= Fr(3, 2) and abs(u2) <= Fr(3, 2)
    # formes et bornes du prouveur a l'echelle 2^20 (coin |alpha|<=2^21)
    for z in (rpoint(), rpoint()):
        f = form(a, b, z, A, B)
        val = (1 << 20) * f[0] + abs(f[1]) * (1 << 21) + abs(f[2]) * (1 << 21)
        rec("dead-lane form max", val)

def make_q4(pts):
    a = pts[0]; d = [sub(p, a) for p in pts[1:]]
    cof = [cross(d[1], d[2]), cross(d[2], d[0]), cross(d[0], d[1])]
    det = dot(d[0], cof[0])
    if det == 0: return None
    num = [0, 0, 0]
    for r in range(3):
        sl = dot(d[r], d[r])
        for ax in range(3): num[ax] += sl * cof[r][ax]
    den = 2 * det * det; rem = den
    rec("q4 det", det); [rec("q4 num", t) for t in num]
    for c in cof:
        w = dot(num, c); rec("q4 weight", w)
        if w <= 0: return None
        rem -= w
    rec("q4 remaining", rem)
    if rem <= 0: return None
    return det, num
for trial in range(60000):
    pts = [rpoint() for _ in range(4)]
    if len(set(pts)) < 4: continue
    r = make_q4(pts)
    if r is None: continue
    n_q4 += 1
    det, num = r
    a = pts[0]
    centre = tuple(Fr(a[i]) + Fr(num[i], 2 * det) for i in range(3))
    # arete proprietaire (la plus longue), au moins une face aigue, disque D/8
    edges = sorted(((dot(sub(pts[i], pts[j]), sub(pts[i], pts[j])), i, j) for i in range(4) for j in range(i+1, 4)), reverse=True)
    D, i, j = edges[0]
    oth = [k for k in range(4) if k not in (i, j)]
    m = tuple(Fr(pts[i][t] + pts[j][t], 2) for t in range(3))
    cm2 = sum((centre[t] - m[t])**2 for t in range(3))
    assert cm2 <= Fr(D, 8), "q4 disk lemma"
    R2 = sum((centre[t] - pts[i][t])**2 for t in range(3))
    # boule dans le cover : R + |c-m| < |ab|  <=>  4 D cm2 < (D + cm2 - R2)^2 et D+cm2-R2>0
    s = D + cm2 - R2
    assert s > 0 and 4 * D * cm2 < s * s, "q4 ball inside cover"
    acute = [dot(sub(pts[k], pts[i]), sub(pts[k], pts[j])) > 0 for k in oth]
    assert any(acute), "q4 acute face lemma"

print("q3 samples", n_q3, "q4 samples", n_q4)
for name, b, lim, ok in checks:
    print(("PASS" if ok else "FAIL"), f"{name}: {b} bits <= {lim}")
for k in sorted(maxima): print("observed", k, maxima[k], "bits")
bad = [c for c in checks if not c[3]]
print("RESULT", "PASS" if not bad else "FAIL")
