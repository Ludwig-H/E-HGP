"""Mutants J2c (hors patch) : copie du code J2c final vers /tmp/j2work/j2cmut/mN, une faute par copie.

M1 : borne haute de la boite ajustee sans + 1 (centres sur la face haute de l'enveloppe perdus).
M2 : D-loc avec le cote h de l'axe 0 pour les trois axes (boites non cubiques : dominance fausse).
M3 : cle du reservoir sur le centre cubique (demi-cote de l'axe 0 pour les trois axes).
usage : python3 mutants_j2c.py SRC_V10
"""
import os
import shutil
import sys

SRC = sys.argv[1]
OUT = '/tmp/j2work/j2cmut'
G = 'src/catalogue/generator.cpp'
MUTANTS = {
    'm1': [('    S.hi[a] = std::min(S.hi[a] + 1, Q.hi[a]);', '    S.hi[a] = std::min(S.hi[a], Q.hi[a]);')],
    'm2': [('    yx[ny] = 2 * hx * a;\n    yy[ny] = 2 * hy * b;\n    yz[ny] = 2 * hz * c;',
            '    yx[ny] = 2 * hx * a;\n    yy[ny] = 2 * hx * b;\n    yz[ny] = 2 * hx * c;'),
           ('    const i64 px = 2 * hx * a, py = 2 * hy * b, pz = 2 * hz * c,',
            '    const i64 px = 2 * hx * a, py = 2 * hx * b, pz = 2 * hx * c,')],
    'm3': [('    const i64 da = 2 * (X.x - Q.lo[0]) - hx, db = 2 * (X.y - Q.lo[1]) - hy, dc = 2 * (X.z - Q.lo[2]) - hz;',
            '    const i64 da = 2 * (X.x - Q.lo[0]) - hx, db = 2 * (X.y - Q.lo[1]) - hx, dc = 2 * (X.z - Q.lo[2]) - hx;')],
}
os.makedirs(OUT, exist_ok=True)
for name, edits in MUTANTS.items():
    dst = os.path.join(OUT, name, 'morsehgp3D_v10')
    if os.path.exists(os.path.dirname(dst)):
        shutil.rmtree(os.path.dirname(dst))
    shutil.copytree(SRC, dst)
    path = os.path.join(dst, G)
    text = open(path).read()
    for old, new in edits:
        if text.count(old) != 1:
            print('motif introuvable ou multiple pour', name, ':', old[:60])
            sys.exit(1)
        text = text.replace(old, new)
    open(path, 'w').write(text)
    print(name, 'pret')
