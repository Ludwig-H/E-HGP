#!/usr/bin/env python3
"""Determinisme et invariants globaux de l'etage G a l'echelle (CONTRAT_TOUR.md, paragraphes 8 et 9.5-9.6) : la sonde
joue la meme entree a plusieurs nombres de fils ; l'empreinte de la resolution (naissances, cellules, traces, cibles,
compteurs de l'objet et du travail : bench/tower_export.hpp) et les lignes par ordre doivent etre identiques ; par
ordre, invariants globaux (jamais un juge exhaustif) : un controle de decroissance par plus petite boule et par succes
de sonde, une longueur de chaine par representant, chaque representant termine par une sonde ou un arret.
Admission : une passe tour_g conforme a K/fils, tous les ordres 1..min(K,sites), naissances k1 = sites, compteurs
entiers non negatifs, histogrammes de 16 cases et SHA-256 complet ; au moins deux nombres de fils distincts.
Le generateur deduplique : --uniform=N borne les sites par N sans imposer leur egalite. Le digest complet est compare
entre fils ; seul son affichage reste abrege pour conserver la ligne des wrappers existants.

    g_determinism.py <mhgp12_tower_probe> <cas> (--uniform=N,GRAINE,BITS | --data=NOM) --k=K [--leaf=L]
                     --threads=W1,W2,...

--data lit <MHGP12_DATA_DIR>/<NOM>.u32le et .ids.u32le. Ligne finale : g_determinism_ok cas=<cas> fils=<liste>
empreinte=<16 premiers hexadecimaux> naissances=<somme> cellules=<somme> representants=<somme> cibles_cellule=<somme>.
Codes : 0 conforme ; 1 ecart entre fils ; 2 usage ou refus de la sonde ; 3 invariant viole. Python 3.10 nu, aucun
assert.
"""
import json
import os
import re
import subprocess
import sys


OBJECT_FIELDS = ('births', 'cells', 'inert_cells', 'extended_cells', 'representatives')
WORK_FIELDS = ('probes', 'first_probe_hits', 'probe_hits_after_steps', 'route_t1', 'route_cert_table',
               'route_cert_census', 'route_fallback_table', 'route_fallback_census', 'fallback_no_proposal',
               'fallback_not_in_part', 'fallback_certificate', 'census_saturated', 'census_complete',
               'census_sites', 'census_sites_max', 'census_nodes', 'jumps_catalogue', 'jumps_census',
               'inert_steps', 'cell_stops', 'birth_stops', 'controls', 'max_chain')


def natural(value):
    return type(value) is int and value >= 0


def decimal(text):
    return re.fullmatch(r'[0-9]+', text) is not None


def run(probe, source, options, threads, kmax, site_limit):
    cmd = [probe] + source + options + ['--threads=%d' % threads, '--digest']
    try:
        done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        if done.returncode != 0:
            return done.returncode, None, None
        rows = [json.loads(line) for line in done.stdout.decode('utf-8').splitlines()]
    except (OSError, ValueError, UnicodeError):
        return 2, None, None
    if len(rows) < 4 or not all(isinstance(r, dict) for r in rows):
        return 2, None, None
    # Une passe, tous les ordres, une seule empreinte, puis la ligne de sortie conforme que tower_probe.cpp ecrit
    # toujours en dernier (contre-lecture de la proposition de l'auditeur : sa derniere ligne n'est pas l'empreinte).
    stage, orders, last, end = rows[0], rows[1:-2], rows[-2], rows[-1]
    if end != {'phase': 'exit', 'status': 'ok', 'reason': 'none', 'order': 0}:
        return 2, None, None
    numeric = ('pass', 'order', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns')
    if stage.get('phase') != 'tour_g' or stage.get('status') != 'ok' or stage.get('reason') != 'none' or \
            not all(natural(stage.get(field)) for field in numeric):
        return 2, None, None
    sites = stage['sites']
    if stage['pass'] != 0 or stage['order'] != 0 or not 1 <= stage['coord_bits'] <= 32 or \
            stage['kmax'] != kmax or stage['threads'] != threads or not 1 <= sites <= 0xFFFFFFFF or \
            (site_limit is not None and sites > site_limit):
        return 2, None, None
    if [r.get('k') for r in orders] != list(range(1, min(kmax, sites) + 1)):
        return 2, None, None
    for record in orders:
        obj, work = record.get('objet'), record.get('travail')
        if record.get('phase') != 'ordre' or not natural(record.get('k')) or not isinstance(obj, dict) or \
                not isinstance(work, dict) or set(obj) != set(OBJECT_FIELDS) or \
                set(work) != set(WORK_FIELDS) | {'chaines'}:
            return 2, None, None
        chains = work['chaines']
        if not all(natural(obj[key]) for key in OBJECT_FIELDS) or \
                not all(natural(work[key]) for key in WORK_FIELDS) or not isinstance(chains, list) or \
                len(chains) != 16 or not all(natural(value) for value in chains):
            return 2, None, None
    if orders[0]['objet']['births'] != sites:
        return 2, None, None
    digest = last.get('resolution_sha256')
    if set(last) != {'phase', 'resolution_sha256'} or last.get('phase') != 'digest' or \
            not isinstance(digest, str) or re.fullmatch(r'[0-9a-f]{64}', digest) is None:
        return 2, None, None
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
    seen, kmax, site_limit = set(), None, None
    for item in argv[3:]:
        key, separator, value = item.partition('=')
        if not separator or key in seen:
            print('g_determinism_refus option %s' % item)
            return 2
        seen.add(key)
        if key == '--uniform':
            parts = value.split(',')
            if source or len(parts) != 3 or not all(decimal(v) for v in parts):
                return 2
            count, seed, bits = map(int, parts)
            if not 1 <= count <= 1 << 26 or seed > 0xFFFFFFFFFFFFFFFF or not 1 <= bits <= 32:
                return 2
            source, site_limit = [item], count  # sites distincts <= N : synthetic() supprime les doublons.
        elif key == '--data':
            if source or not value:
                return 2
            stem = os.path.join(os.environ.get('MHGP12_DATA_DIR', ''), value)
            source = [stem + '.u32le', stem + '.ids.u32le']
        elif key == '--threads':
            parts = value.split(',')
            if not all(decimal(v) for v in parts):
                return 2
            threads = [int(v) for v in parts]
            if len(threads) < 2 or len(set(threads)) != len(threads) or any(v < 1 for v in threads):
                return 2
        elif key in ('--k', '--leaf'):
            if not decimal(value) or not 1 <= int(value) <= (12 if key == '--k' else 1 << 20):
                return 2
            options.append(item)
            if key == '--k':
                kmax = int(value)
        else:
            print('g_determinism_refus option %s' % item)
            return 2
    if not source or threads is None or kmax is None:
        print('g_determinism_refus source, ordre ou fils absents')
        return 2
    first = None
    for w in threads:
        code, orders, digest = run(probe, source, options, w, kmax, site_limit)
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
