#!/usr/bin/env python3
"""L04 audit : budget de bits des predicats de arith/geometry.{hpp,cpp} pour des coordonnees dans [0, M], M = 2^B - 1.

Lemmes utilises (sites a, b, c, d, z dans le cube [0, M]^3, u = b - a, v = c - a, s = d - a, w = u x v) :
  (L1) |composante d'une difference| <= M ; |u|^2 <= 3 M^2 ;
  (L2) |w_i| <= M^2 (double de l'aire d'un triangle projete dans un carre de cote M) ; |w|^2 <= 3 M^4 (atteint) ;
  (L3) |det(u, v, s)| <= 2 M^3 (six fois le volume d'un tetraedre inscrit dans un cube : au plus M^3 / 3, atteint).
Chaque ligne : (quantite, coefficient c, exposant e, conteneur, capacite en bits de MAGNITUDE) pour la borne c * M^e.
"""
from math import log2

ROWS = [
    ('dot(u, u) (i64)', 3, 2, 'i64', 63),
    ('cross : w_i (i64)', 1, 2, 'i64', 63),
    ('orient : det (i128)', 2, 3, 'i128', 127),
    ('center3 : t_i = uu v_i - vv u_i', 6, 3, 'i128', 127),
    ('center3 : N_i', 12, 5, 'i128', 127),
    ('center3 : D = 2 |w|^2', 6, 4, 'i128', 127),
    ('center4 : N_i', 9, 4, 'i128', 127),
    ('center4 : D = 2 |det|', 4, 3, 'i128', 127),
    ('side q3 : D |z-a|^2', 18, 6, 'i128', 127),
    ('side q3 : 2 N.(z-a) et partiels', 72, 6, 'i128', 127),
    ('side_key q3 : difference', 90, 6, 'i128', 127),
    ('side q4 : D |z-a|^2', 12, 5, 'i128', 127),
    ('side q4 : 2 N.(z-a) et partiels', 54, 5, 'i128', 127),
    ('side_key q4 : difference', 66, 5, 'i128', 127),
    ('orient_center q4 : cc_i = N_i + D (a_i - p_i)', 13, 4, 'i128', 127),
    ('orient_center q4 : somme des w_i cc_i', 39, 6, 'i128', 127),
    ('orient_center_wide q3 : cc_i', 18, 5, 'i128', 127),
    ('orient_center_wide q3 : somme (Wide<4>)', 54, 7, 'Wide<4>', 256),
    ('is_midpoint q3 : 2 (a_i D + N_i)', 36, 5, 'i128', 127),
    ('level3 : uu * vv (u128)', 9, 4, 'u128', 128),
    ('level3 : num = uu vv dd (I192)', 27, 6, 'I192', 192),
    ('level3 : den = 4 |w|^2 (u128)', 12, 4, 'u128', 128),
    ('level4 : num = |N|^2 (I192)', 243, 8, 'I192', 192),
    ('level4 : den = D^2 (u128)', 16, 6, 'u128', 128),
    ('compare : num_a * den_b (Wide<5>), q4 contre q4', 243 * 16, 14, 'Wide<5>', 320),
]
BITS = (18, 19, 20, 21, 24, 32)
print('| quantite | borne | conteneur | ' + ' | '.join('B=%d' % b for b in BITS) + ' | plus grand B sur |')
print('| --- | --- | --- | ' + ' | '.join('---:' for _ in BITS) + ' | ---: |')
for name, c, e, cont, cap in ROWS:
    cells = []
    for b in BITS:
        need = log2(c) + e * b  # majorant : M < 2^b
        cells.append('%.1f%s' % (need, '' if need < cap else ' **X**'))
    bmax = max(b for b in range(1, 64) if log2(c) + e * b < cap)
    print('| %s | %d M^%d | %s (%d) | %s | %d |' % (name, c, e, cont, cap, ' | '.join(cells), bmax))
