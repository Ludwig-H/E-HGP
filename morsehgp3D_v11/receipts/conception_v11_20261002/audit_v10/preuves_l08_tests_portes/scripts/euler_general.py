"""Invariant de Morse-Euler par ordre sur une entree QUELCONQUE (coquilles etendues comprises), pour le catalogue v10.

  E_K = sites [K = 1] + somme des contributions e_K(B) = 1   pour K = 1 .. kcat - 2.
  Coquille reguliere (u = q) : e_K = (-1)^(q - m) C(q - 1, m - 1), m = K - p (comptes by_q_p de la ligne JSON).
  Coquille etendue : e_K = 1 - chi(Lambda_m), par enumeration exacte des cellules de l'arrangement des grands cercles
  (fonction chi_cells de l'auditeur C de la v9, importee telle quelle : morsehgp3D_v9/audits/c_euler_20260923/
  euler_degenerate.py, sha256 ee0bf49c...c68b). Centre exact par Fraction (reference/hgp10_ref.py).
Seules les lignes du dump dont le drapeau « coquille etendue » est leve sont lues (filtrees par awk).
Usage : python3 euler_general.py BUILD SRC_V10 CHEMIN_V9_EULER IN.u32le kcat fils
"""
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
from fractions import Fraction
from math import comb, lcm

build, src, v9, inp, kcat, threads = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), int(sys.argv[6])
sys.path.insert(0, os.path.join(src, 'reference'))
import hgp10_ref as R  # noqa: E402

spec = importlib.util.spec_from_file_location('euler_degenerate', v9)
E = importlib.util.module_from_spec(spec)
spec.loader.exec_module(E)  # le module ne fait rien a l'import (main garde)


def e_reg(q, m):
    return (-1) ** (q - m) * comb(q - 1, m - 1) if 1 <= m <= q else 0


with tempfile.TemporaryDirectory(dir='/tmp/v11-audit/l08_tests_portes') as tmp:
    dump = os.path.join(tmp, 'cat.txt')
    r = subprocess.run([os.path.join(build, 'mhgp10_catalogue'), inp, '--k=%d' % kcat, '--threads=%d' % threads,
                        '--dump=' + dump], capture_output=True, text=True)
    js = json.loads([l for l in r.stdout.splitlines() if l.startswith('{')][-1])
    if r.returncode != 0:
        print(json.dumps({'refus': js}))
        sys.exit(2)
    ext = subprocess.run(['awk', '$5 % 2 == 1', dump], capture_output=True, text=True).stdout.splitlines()
    lines = int(subprocess.run(['wc', '-l', dump], capture_output=True, text=True).stdout.split()[0])
conv = lambda s: [tuple(int(v) for v in t.split(',')) for t in s.split()]  # noqa: E731
E_K = {K: (js['sites'] if K == 1 else 0) for K in range(1, kcat - 1)}
for q in (2, 3, 4):
    for p, c in enumerate(js['by_q_p']['q%d' % q]):
        for K in E_K:
            E_K[K] += c * e_reg(q, K - p)
shells = {}
for line in ext:
    head, sup, inner, shell = line.split('|')
    rank, q, p, u, flags = (int(t) for t in head.split())
    S, U = conv(sup), conv(shell)
    if u != len(U):
        print(json.dumps({'erreur': 'coquille ponderee, hors formule'}))
        sys.exit(3)
    c, lam = R.circumcenter(S)
    den = 1
    for x in c:
        den = lcm(den, Fraction(x).denominator)
    dirs = [tuple(int((Fraction(x[i]) - c[i]) * den) for i in range(3)) for x in U]
    shells[u] = shells.get(u, 0) + 1
    for K in E_K:
        m = K - p
        E_K[K] -= e_reg(q, m)                    # retire la contribution « reguliere » comptee par by_q_p
        if 1 <= m <= u:
            E_K[K] += 1 - E.chi_cells(dirs, m)   # contribution exacte de la coquille etendue
ok = all(v == 1 for v in E_K.values())
print(json.dumps({'entree': os.path.basename(inp), 'sites': js['sites'], 'kcat': kcat, 'balls': js['balls'],
                  'lignes_du_dump': lines, 'coquilles_etendues': len(ext), 'etendues_par_taille': shells,
                  'E_K': E_K, 'verdict': 'EULER_OK' if ok else 'EULER_ECART'}, sort_keys=True))
sys.exit(0 if ok else 1)
