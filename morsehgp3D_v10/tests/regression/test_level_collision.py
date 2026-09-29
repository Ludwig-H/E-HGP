"""Regression (29 septembre 2026) : deux niveaux exacts distincts dont les approximations double coincident (ou
s'inversent d'un ulp) cassaient la validation du dendrogramme de points (invariant_violated / rank_order) a K = 8
sur une scene dev anisotrope extreme de 8 000 points (1,4 million de niveaux exacts). Correctif : niveaux publies
strictement croissants en double, niveaux exacts indiscernables fusionnes dans le meme rang.

  python3 test_level_collision.py <dossier de build>   -> code 0 si conforme, 1 sinon
"""
import json
import os
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'bench', 'synthetic'))
import scenes  # noqa: E402

SPEC = {'family': 'anisotropic', 'n': 8000, 'groups': 8, 'level': 'extreme', 'noise_fraction': 0.0,
        'seed': 171554247519005774}


def main():
    build = sys.argv[1]
    P, L, _ = scenes.generate(SPEC)
    G, _, _, _ = scenes.quantize18(P, L)
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, 'in.u32le')
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        for extra in ([], ['--entry=cover']):
            if extra and not os.path.exists(os.path.join(build, 'mhgp10_cluster')):
                continue
            tree = os.path.join(tmp, 'tree')
            r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, os.path.join(tmp, 'out'), '--k=8', '--mcs=89',
                                '--threads=2', '--tree=' + tree] + extra, capture_output=True, text=True)
            if r.returncode == 2 and 'option inconnue' in r.stderr:
                continue  # binaire sans mode couverture
            status = None
            for line in r.stdout.splitlines():
                if line.startswith('{'):
                    status = json.loads(line).get('status')
            if r.returncode != 0 or status != 'ok':
                failures.append('%s : code %d, statut %s' % (extra or 'core', r.returncode, status))
                continue
            with open(tree) as f:
                lines = f.read().split('\n')
            nl = int(lines[0].split()[1])
            levels = [float(x) for x in lines[1:1 + nl]]
            if any(b <= a for a, b in zip(levels, levels[1:])):
                failures.append('%s : niveaux non strictement croissants' % (extra or 'core'))
            print('%s : %d niveaux publies, statut ok' % (extra or 'core', nl))
    for f in failures:
        print('ECHEC', f)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
