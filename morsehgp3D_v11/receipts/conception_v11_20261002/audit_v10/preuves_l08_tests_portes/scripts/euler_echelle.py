"""Juge global INDEPENDANT du catalogue v10 aux tailles d'interet : invariant de Morse-Euler par ordre
(note v9 NOTE_C_INVARIANT_EULER_20260923 ; statut proved_here en v9, jamais porte en v10).

  E_K = n [K = 1] + somme sur les boules critiques de e_K(B) = 1,   pour K = 1 .. kcat - 2,
  e_K(B) = (-1)^(q - m) C(q - 1, m - 1) si 1 <= m = K - p <= q, 0 sinon   (coquille reguliere : u = q).

Lu sur la ligne JSON de mhgp10_catalogue (comptes by_q_p) : aucun dump, aucun tableau par paire, cout O(K).
La formule fermee ne vaut que si toutes les coquilles sont regulieres (extended == 0, weighted == 0) : sinon le
script le dit et ne conclut pas.
Usage : python3 euler_echelle.py BUILD n kcat graine fils [fichier.u32le]
"""
import json
import os
import random
import subprocess
import sys
import tempfile
from math import comb

build, n, kcat, seed, threads = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
given = sys.argv[6] if len(sys.argv) > 6 else None


def e(q, m):
    return (-1) ** (q - m) * comb(q - 1, m - 1) if 1 <= m <= q else 0


with tempfile.TemporaryDirectory() as tmp:
    if given:
        src = given
    else:
        rnd = random.Random(seed)
        pts = set()
        while len(pts) < n:
            pts.add((rnd.randrange(1 << 18), rnd.randrange(1 << 18), rnd.randrange(1 << 18)))
        pts = sorted(pts)
        rnd.shuffle(pts)
        src = os.path.join(tmp, 'in.u32le')
        with open(src, 'wb') as f:
            for p in pts:
                for v in p:
                    f.write(int(v).to_bytes(4, 'little'))
    r = subprocess.run([os.path.join(build, 'mhgp10_catalogue'), src, '--k=%d' % kcat, '--threads=%d' % threads],
                       capture_output=True, text=True)
    js = json.loads([l for l in r.stdout.splitlines() if l.startswith('{')][-1])
    if r.returncode != 0:
        print(json.dumps({'refus': js}))
        sys.exit(2)
    sites = js['sites']
    out = {'entree': given or 'uniforme_u18 graine %d' % seed, 'n': js['n'], 'sites': sites, 'kcat': kcat,
           'balls': js['balls'], 'extended': js['extended'], 'weighted': js['weighted'], 'max_shell': js['max_shell'],
           'catalogue_s': js['catalogue_s']}
    regular = js['extended'] == 0 and js['weighted'] == 0
    E = {}
    for K in range(1, kcat - 1):
        total = sites if K == 1 else 0
        for q in (2, 3, 4):
            counts = js['by_q_p']['q%d' % q]
            for p, c in enumerate(counts):
                total += c * e(q, K - p)
        E[K] = total
    out['E_K'] = E
    out['formule_applicable'] = regular
    out['verdict'] = ('EULER_OK' if all(v == 1 for v in E.values()) else 'EULER_ECART') if regular else 'NON_CONCLUANT_coquilles_etendues'
    print(json.dumps(out, sort_keys=True))
    sys.exit(0 if out['verdict'] != 'EULER_ECART' else 1)
