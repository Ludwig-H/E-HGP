#!/usr/bin/env python3
"""Porte mhgp11_api_publish_reader : aller-retour publication de l'API -> lecteur officiel (audit abc30ed06).

La sonde mhgp11_api_publish_probe publie growth_ABCZ (quatre points, K = 3) par api::publish avec une provenance :
- valide : code 0, etat published_complete, et le lecteur officiel (bench/mhgp11_formats.py, check_directory) relit
  le dossier sans refus, tailles d'entree 48 et 16 octets ;
- tailles nulles, 11 octets par point, rapport 12/4 pour huit points, budget declare nul : refus
  parameter_out_of_range, code 2, etat none, ni D ni D.pending (le refus precede toute creation).
Python 3.10 nu, aucun assert.
"""
import argparse
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tests', 'support'))
sys.path.insert(0, os.path.join(ROOT, 'bench'))
import mhgp11_gate  # noqa: E402
import mhgp11_formats as formats  # noqa: E402

REFUSED = ('tailles_nulles', 'rapport_faux', 'compte_faux', 'budget_nul')
FLOOR = 26


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--probe', required=True)
    parser.add_argument('--bits', type=int, required=True)
    args = parser.parse_args()
    gate = mhgp11_gate.Gate('api_publish_reader')
    work = tempfile.mkdtemp(prefix='mhgp11_api_publish_reader_')
    try:
        directory = os.path.join(work, 'D')
        done = mhgp11_gate.expect_code(gate, [args.probe, directory, 'valide'], 0, 'publication valide')
        gate.check_eq(done.stdout.strip(), 'publish_probe valide none published_complete', 'ligne valide')
        try:
            read = formats.check_directory(directory, args.bits)
        except ValueError as error:
            gate.check(False, 'lecteur officiel : %s' % error)
        else:
            gate.check(True, 'lecteur officiel')
            inputs = read['manifest']['inputs']
            gate.check_eq([entry['bytes'] for entry in inputs], [48, 16], 'tailles d\'entree publiees')
            gate.check_eq(read['manifest']['counts']['points'], 4, 'points publies')
            gate.check_eq(read['manifest']['parameters']['budget_bytes'], None, 'budget non declare')
        for case in REFUSED:
            directory = os.path.join(work, 'R_' + case)
            done = mhgp11_gate.expect_code(gate, [args.probe, directory, case], 2, 'refus %s' % case)
            gate.check_eq(done.stdout.strip(), 'publish_probe %s parameter_out_of_range none' % case,
                          'ligne de refus %s' % case)
            gate.check(not os.path.lexists(directory), 'aucun dossier publie (%s)' % case)
            gate.check(not os.path.lexists(directory + '.pending'), 'aucun D.pending (%s)' % case)
            gate.check_eq(sorted(os.listdir(work)), sorted(['D']), 'rien d\'autre dans le dossier de travail (%s)' % case)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    code = gate.finish(FLOOR)
    if code == mhgp11_gate.OK:
        print('api_publish_reader_verdict conforme cas%d' % (1 + len(REFUSED)))
    return code


if __name__ == '__main__':
    sys.exit(main())
