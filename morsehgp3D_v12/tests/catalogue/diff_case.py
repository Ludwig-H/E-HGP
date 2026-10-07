#!/usr/bin/env python3
"""Porte du differentiel contre la v11 sur un cas (CONTRAT_CATALOGUE.md, paragraphe 6.1) : la sonde calcule Cat_K
et l'exporte (MHGP12DP) dans un dossier temporaire neuf, puis le lecteur de transition (reference/
transition_catalogue.py, regle du paragraphe 6.1, conventions v11 pour la reference et v12 pour le candidat) le juge
contre le vidage de la v11 gelee du meme cas (outil mhgp12_vidage de microbancs/mes_m3_m4_tour, v11 construite depuis
l'archive epinglee par microbancs/outils/source_v11.py).

    diff_case.py <sonde> <lecteur> <cat.bin de la v11> --data=<nom> --frame=<trame> --k=K --leaf=L [--threads=W]

L'entree est <MHGP12_DATA_DIR>/<nom>.u32le et .ids.u32le ; <trame> est le nom de trame du vidage de la v11 (le
lecteur exige la meme trame et le meme K dans les deux en-tetes). Codes et lignes : ceux du lecteur (0 conforme,
1 desaccord, 2 refus, 3 invariant) ; 2 si la sonde refuse. Python 3.10 nu, aucun assert.
"""
import os
import subprocess
import sys
import tempfile

OPTIONS = ('--data', '--frame', '--k', '--leaf', '--threads')


def main(argv):
    if len(argv) < 7 or not all(os.path.isfile(path) for path in argv[1:4]):
        print('diff_case_refus usage ou fichier absent')
        return 2
    options = {'data': None, 'frame': None, 'k': None, 'leaf': None, 'threads': '4'}
    for item in argv[4:]:
        key, _, value = item.partition('=')
        if key in OPTIONS and value:
            options[key[2:]] = value
        else:
            print('diff_case_refus option %s' % item)
            return 2
    if None in options.values():
        print('diff_case_refus option manquante')
        return 2
    stem = os.path.join(os.environ.get('MHGP12_DATA_DIR', ''), options['data'])
    with tempfile.TemporaryDirectory() as folder:
        out = os.path.join(folder, 'v12')
        cmd = [argv[1], stem + '.u32le', stem + '.ids.u32le', '--k=' + options['k'], '--leaf=' + options['leaf'],
               '--threads=' + options['threads'], '--out=' + out, '--frame=' + options['frame']]
        done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        if done.returncode != 0:
            print('diff_case_refus sonde code %d' % done.returncode)
            return 2
        reader = [sys.executable, '-S', argv[2], argv[3], os.path.join(out, 'cat.bin')]
        judged = subprocess.run(reader, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        sys.stdout.write(judged.stdout.decode('ascii', 'replace'))
        sys.stderr.write(judged.stderr.decode('ascii', 'replace'))
        return judged.returncode


if __name__ == '__main__':
    sys.exit(main(sys.argv))
