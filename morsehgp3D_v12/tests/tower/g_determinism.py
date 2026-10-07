#!/usr/bin/env python3
"""Determinisme et invariants globaux de l'etage G a l'echelle (CONTRAT_TOUR.md, paragraphes 8 et 9.5-9.6) : la sonde
joue la meme entree a plusieurs nombres de fils ; l'empreinte de la resolution (naissances, cellules, traces, cibles,
compteurs de l'objet et du travail : bench/tower_export.hpp) et les lignes par ordre doivent etre identiques ; par
ordre, invariants globaux (jamais un juge exhaustif) : un controle de decroissance par plus petite boule et par succes
de sonde, une longueur de chaine par representant, chaque representant termine par une sonde ou un arret.

    g_determinism.py <mhgp12_tower_probe> <cas> (--uniform=N,GRAINE,BITS | --data=NOM) --k=K [--leaf=L]
                     --threads=W1,W2,...

--data lit <MHGP12_DATA_DIR>/<NOM>.u32le et .ids.u32le. Ligne finale : g_determinism_ok cas=<cas> fils=<liste>
empreinte=<16 premiers hexadecimaux> naissances=<somme> cellules=<somme> representants=<somme> cibles_cellule=<somme>.
Codes : 0 conforme ; 1 ecart entre fils ; 2 usage ou refus de la sonde ; 3 invariant viole. Python 3.10 nu, aucun
assert.
"""
import json
import os
import subprocess
import sys


def run(probe, source, options, threads):
    cmd = [probe] + source + options + ['--threads=%d' % threads, '--digest']
    done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if done.returncode != 0:
        return done.returncode, None, None
    orders, digest = [], None
    for line in done.stdout.decode('ascii', 'replace').splitlines():
        record = json.loads(line)
        if record.get('phase') == 'ordre':
            orders.append(record)
        elif record.get('phase') == 'digest':
            digest = record['resolution_sha256']
    return 0, orders, digest


def invariants(orders):
    problems = []
    for record in orders:
        o, t, chains = record['objet'], record['travail'], record['travail']['chaines']
        k = record['k']
        balls = t['route_t1'] + t['route_cert_table'] + t['route_cert_census'] + t['route_fallback_table'] + \
            t['route_fallback_census']
        hits = t['first_probe_hits'] + t['probe_hits_after_steps']
        if k > 1 and (t['controls'] != balls + hits or sum(chains) != o['representatives'] or
                      hits + t['cell_stops'] + t['birth_stops'] != o['representatives']):
            problems.append('ordre %d : controles, chaines ou arrets incoherents' % k)
        if o['births'] == 0 or (k > 1 and o['cells'] > 0 and o['representatives'] < o['cells']):
            problems.append('ordre %d : naissances ou representants absents' % k)
    return problems


def main(argv):
    if len(argv) < 5 or not os.path.isfile(argv[1]):
        print('g_determinism_refus usage')
        return 2
    probe, case, source, options, threads = argv[1], argv[2], [], [], None
    for item in argv[3:]:
        if item.startswith('--uniform='):
            source = [item]
        elif item.startswith('--data='):
            stem = os.path.join(os.environ.get('MHGP12_DATA_DIR', ''), item[len('--data='):])
            source = [stem + '.u32le', stem + '.ids.u32le']
        elif item.startswith('--threads='):
            threads = [int(v) for v in item[len('--threads='):].split(',') if v.isdigit()]
        elif item.startswith('--k=') or item.startswith('--leaf='):
            options.append(item)
        else:
            print('g_determinism_refus option %s' % item)
            return 2
    if not source or not threads or len(threads) < 2:
        print('g_determinism_refus source ou fils absents')
        return 2
    first = None
    for w in threads:
        code, orders, digest = run(probe, source, options, w)
        if code != 0 or digest is None or not orders:
            print('g_determinism_refus sonde code %d a %d fils' % (code, w))
            return 2
        problems = invariants(orders)
        if problems:
            for problem in problems:
                print('invariant %s' % problem)
            return 3
        if first is None:
            first = (orders, digest)
        elif (orders, digest) != first:
            print('g_determinism_ecart fils=%d : sorties ou compteurs differents de %d fils' % (w, threads[0]))
            return 1
    orders, digest = first
    total = dict((key, sum(r['objet'][key] for r in orders)) for key in ('births', 'cells', 'representatives'))
    cell_targets = sum(r['travail']['cell_stops'] for r in orders)
    print('g_determinism_ok cas=%s fils=%s empreinte=%s naissances=%d cellules=%d representants=%d cibles_cellule=%d'
          % (case, ','.join(str(w) for w in threads), digest[:16], total['births'], total['cells'],
             total['representatives'], cell_targets))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
