#!/usr/bin/env python3
"""L04 audit : budgets « mecaniques » (inegalite triangulaire terme a terme, sans lemme geometrique), tels qu'un calcul
constexpr par expression les produirait (profil v11 : num::Int<bits>), et plus petit conteneur exact par B.
Conteneurs signes : i64 (63 bits de magnitude), i128 (127), Wide<L> (64 L bits de magnitude, signe a part)."""
from math import log2

ROWS = [  # (quantite, coefficient, exposant de M)
    ('difference de coordonnees', 1, 1),
    ('dot(u, v)', 3, 2),
    ('cross : w_i', 2, 2),
    ('orient : det (somme de 3 x 2 produits)', 6, 3),
    ('q3 : N_i', 24, 5),
    ('q3 : D', 12, 4),   # 2 |w|^2 avec |w|^2 = 3 termes (2M^2)^2 = 12 M^4 ; D = 24 M^4
    ('q4 : N_i', 18, 4),
    ('q4 : D', 12, 3),
    ('side_key q3 (lhs - rhs)', 216, 6),
    ('side_key q4 (lhs - rhs)', 144, 5),
    ('orient_center q4 : somme', 180, 6),
    ('orient_center q3 : somme', 288, 7),
    ('is_midpoint q3', 96, 5),
    ('centre dans boite (T = 6), q3', 64 * 48, 5),
    ('level3 : num', 27, 6),
    ('level3 : den', 48, 4),
    ('level4 : num', 972, 8),
    ('level4 : den', 144, 6),
    ('compare : produit croise (q4, q4)', 972 * 144, 14),
]
ROWS[5] = ('q3 : D', 24, 4)


def container(bits):
    if bits < 63:
        return 'i64'
    if bits < 127:
        return 'i128'
    words = 3
    while bits >= 64 * words:
        words += 1
    return 'Wide<%d>' % words


BS = (18, 21, 24, 32)
print('| quantite | borne mecanique | ' + ' | '.join('B=%d' % b for b in BS) + ' |')
print('| --- | --- | ' + ' | '.join('---' for _ in BS) + ' |')
for name, c, e in ROWS:
    cells = []
    for b in BS:
        bits = log2(c) + e * b
        cells.append('%.1f : %s' % (bits, container(bits)))
    print('| %s | %d M^%d | %s |' % (name, c, e, ' | '.join(cells)))
