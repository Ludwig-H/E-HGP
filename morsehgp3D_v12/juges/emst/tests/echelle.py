#!/usr/bin/env python3
"""Banc d'echelle du juge JUG-EMST (pas une porte) : nuages uniformes a graine fixe, temps et compteurs publies.

    python3 echelle.py --juge <mhgp12_jug_emst> --travail <dossier> [--tailles 1000000 4000000] [--bits 21]
                       [--graine 12] [--prises 1]

Les coordonnees sont tirees uniformement dans [0, 2^bits) par SplitMix64 (meme generateur que l'oracle borne) ; les
doublons eventuels sont retires (decision D8). Le temps local ne predit pas G4 : il donne l'ordre de grandeur et la
pente entre deux tailles, rien de definitif. Sortie : une ligne JSON par prise. Python 3.10 nu, aucun assert.
"""
import argparse
import json
import os
import sys
from array import array

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import commun as C  # noqa: E402

MASQUE = (1 << 64) - 1


def splitmix(etat):
    etat = (etat + 0x9E3779B97F4A7C15) & MASQUE
    z = etat
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASQUE
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASQUE
    return etat, z ^ (z >> 31)


def nuage_uniforme(chemin, n, bits, graine):
    etat, vus, mots = graine * 1000003, set(), array('I')
    while len(vus) < n:
        etat, r = splitmix(etat)
        etat, s = splitmix(etat)
        p = (r & ((1 << bits) - 1), (r >> 32) & ((1 << bits) - 1), s & ((1 << bits) - 1))
        if p not in vus:
            vus.add(p)
            mots.extend(p)
    if sys.byteorder != 'little':
        mots.byteswap()
    with open(chemin, 'wb') as sortie:
        mots.tofile(sortie)


def main(argv):
    parser = argparse.ArgumentParser(description='Banc d\'echelle de JUG-EMST')
    parser.add_argument('--juge', required=True)
    parser.add_argument('--travail', required=True)
    parser.add_argument('--tailles', type=int, nargs='+', default=[1000000, 4000000])
    parser.add_argument('--bits', type=int, choices=(21, 24, 32), default=21)
    parser.add_argument('--graine', type=int, default=12)
    parser.add_argument('--prises', type=int, default=1)
    try:
        args = parser.parse_args(argv[1:])
    except SystemExit:
        return C.USAGE
    os.makedirs(args.travail, exist_ok=True)
    for n in args.tailles:
        chemin = os.path.join(args.travail, 'uniforme_u%d_n%d.u32le' % (args.bits, n))
        if not os.path.isfile(chemin):
            nuage_uniforme(chemin, n, args.bits, args.graine)
        for prise in range(args.prises):
            code, sortie, erreur = C.lancer(os.path.abspath(args.juge), [chemin, '--bits', str(args.bits)], 7200)
            if code != C.OK or sortie is None:
                print('echec n=%d code=%s %s' % (n, code, erreur.strip()))
                return C.DESACCORD
            print(json.dumps(dict(n=n, bits=args.bits, prise=prise, arithmetique=sortie['arithmetique'],
                                  temps_ms=sortie['temps_ms'], boruvka=sortie['boruvka'], fusions=sortie['fusions'],
                                  multifusions=sortie['multifusions'], sha256_arbre=sortie['sha256_arbre']),
                             sort_keys=True))
            sys.stdout.flush()
    return C.OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
