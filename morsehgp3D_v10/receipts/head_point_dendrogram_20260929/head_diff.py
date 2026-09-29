"""Differentiel de la tete : etiquettes et arbre exporte (--tree : niveaux en %.17g, rangs, parents, attaches) du binaire
d'origine contre le binaire modifie, octet pour octet, sur 18 cas (trames LiDAR entieres, scene de collision de
niveaux de la porte de regression, scenes shells et filaments ; K = 1, 2, 3, 5, 8, 10 ; entrees core et cover).

  python3 head_diff.py AVANT/mhgp10_cluster APRES/mhgp10_cluster REPERTOIRE_DE_TRAVAIL
Code 0 si les 18 cas sont identiques."""
import hashlib
import os
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'bench', 'synthetic'))
import scenes  # noqa: E402

LIDAR = '/workspaces/E-HGP/build/v10-scale-inputs'
SCENES = {
    'collision': {'family': 'anisotropic', 'n': 8000, 'groups': 8, 'level': 'extreme', 'noise_fraction': 0.0,
                  'seed': 171554247519005774},
    'shells': {'family': 'shells', 'n': 8000, 'groups': 8, 'level': 'hard', 'noise_fraction': 0.1, 'seed': 12345},
    'filaments': {'family': 'filaments', 'n': 16000, 'groups': 8, 'level': 'medium', 'noise_fraction': 0.1,
                  'seed': 777},
}
CASES = [('lidar02_full', 5, 'cover'), ('lidar02_full', 10, 'cover'), ('lidar00_full', 5, 'core'),
         ('lidar01_full', 10, 'core'), ('lidar02_full', 1, 'core'), ('lidar02_full', 1, 'cover'),
         ('lidar00_full', 2, 'core'), ('lidar00_full', 2, 'cover'), ('collision', 8, 'core'), ('collision', 8, 'cover'),
         ('collision', 1, 'core'), ('collision', 3, 'core'), ('shells', 10, 'cover'), ('shells', 3, 'core'),
         ('shells', 2, 'cover'), ('filaments', 5, 'cover'), ('filaments', 10, 'core'), ('filaments', 1, 'cover')]


def main():
    before, after, work = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(work, exist_ok=True)
    inputs = {f: os.path.join(LIDAR, f + '.u32le') for f in ('lidar00_full', 'lidar01_full', 'lidar02_full')}
    for name, spec in SCENES.items():
        P, L, _ = scenes.generate(spec)
        G, _, _, _ = scenes.quantize18(P, L)
        inputs[name] = os.path.join(work, name + '.u32le')
        np.ascontiguousarray(G, dtype='<u4').tofile(inputs[name])
    bad = 0
    for name, k, entry in CASES:
        h = []
        for tag, exe in (('avant', before), ('apres', after)):
            out, tree = os.path.join(work, 'o_' + tag), os.path.join(work, 't_' + tag)
            r = subprocess.run([exe, inputs[name], out, '--k=%d' % k, '--mcs=89', '--z=3', '--entry=' + entry,
                                '--threads=4', '--tree=' + tree], capture_output=True, text=True)
            h.append(None if r.returncode else
                     hashlib.sha256(open(out, 'rb').read() + open(tree, 'rb').read()).hexdigest())
        same = h[0] is not None and h[0] == h[1]
        bad += not same
        print('%-13s K=%-2d %-5s %s %s' % (name, k, entry, h[0][:16] if h[0] else '-',
                                         'IDENTIQUES' if same else 'DIFFERENTS'), flush=True)
    print('ECARTS %d sur %d' % (bad, len(CASES)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
