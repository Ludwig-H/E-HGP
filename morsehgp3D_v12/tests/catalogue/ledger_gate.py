#!/usr/bin/env python3
"""Porte d'echelle et de determinisme du catalogue (CONTRAT_CATALOGUE.md, paragraphes 6.5 et 6.7) : invariants globaux,
jamais un juge exhaustif. La sonde calcule Cat_K a chaque nombre de fils demande et rend son empreinte canonique
(SHA-256 de l'export MHGP12DP) ; la porte exige :
  - les memes nombres de boules, d'incidences et de niveaux, et les memes vingt compteurs logiques (parcours et quinze
    compteurs de feuille) que la v11 gelee sur la meme entree (tests/catalogue/v11_counts.json, comptes graves) ;
  - la meme empreinte et le meme grand livre a chaque nombre de fils (sorties identiques a l'octet).

    ledger_gate.py <sonde> <cas> (--uniform=N,GRAINE,BITS | --data=<nom du fichier sans .u32le>) --k=K --leaf=L
                   [--threads=1,4,8]

<cas> est une cle de v11_counts.json. --data lit <MHGP12_DATA_DIR>/<nom>.u32le et .ids.u32le.
Codes : 0 conforme ; 1 ecart ; 2 refus (usage, donnee ou cas absent, sonde en refus) ; 3 plancher (aucune boule).
Ligne finale si conforme : catalogue_ledger_ok cas=<cas> boules=B incidences=P niveaux=L fils=<liste>.
Python 3.10 nu, aucun assert.
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
LEDGER = ['nodes', 'leaves', 'filter_tests', 'max_depth', 'max_leaf', 'dominance_tests', 'prefixes', 'judged',
          'census_tests', 'emitted', 'q4_candidates', 'q4_levels', 'region_pair_tests', 'region_pair_rejects',
          'region_line_tests', 'region_line_rejects', 'region_line_evaluations', 'region_line_cache_hits',
          'region_line_fallbacks']


def parse(argv):
    if len(argv) < 5:
        return None
    options = {'probe': argv[1], 'case': argv[2], 'threads': [1, 4, 8], 'input': None, 'k': None, 'leaf': None}
    for item in argv[3:]:
        key, _, value = item.partition('=')
        if key == '--uniform':
            options['input'] = [item]
        elif key == '--data':
            folder = os.environ.get('MHGP12_DATA_DIR', '')
            options['input'] = [os.path.join(folder, value + '.u32le'), os.path.join(folder, value + '.ids.u32le')]
        elif key in ('--k', '--leaf') and value.isdigit():
            options[key[2:]] = value
        elif key == '--threads' and all(v.isdigit() for v in value.split(',')):
            options['threads'] = [int(v) for v in value.split(',')]
        else:
            return None
    if options['input'] is None or options['k'] is None or options['leaf'] is None:
        return None
    return options


def run(options, threads):
    cmd = [options['probe']] + options['input'] + ['--k=' + options['k'], '--leaf=' + options['leaf'],
                                                   '--threads=%d' % threads, '--digest']
    done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    lines = [json.loads(line) for line in done.stdout.decode('ascii', 'replace').splitlines() if line.startswith('{')]
    catalogue = next((d for d in lines if d.get('phase') == 'catalogue'), None)
    digest = next((d for d in lines if d.get('phase') == 'digest'), None)
    return done.returncode, catalogue, digest


def main(argv):
    options = parse(argv)
    if options is None:
        print('catalogue_ledger_refus usage')
        return 2
    with open(os.path.join(HERE, 'v11_counts.json'), encoding='ascii') as handle:
        cases = json.load(handle)['cases']
    if options['case'] not in cases:
        print('catalogue_ledger_refus cas inconnu %s' % options['case'])
        return 2
    want = cases[options['case']]
    seen, errors = [], []
    for threads in options['threads']:
        code, catalogue, digest = run(options, threads)
        if code != 0 or catalogue is None or digest is None or catalogue.get('status') != 'ok':
            print('catalogue_ledger_refus sonde code %d a %d fils' % (code, threads))
            return 2
        got = dict(catalogue['ledger'])
        got.update(balls=catalogue['balls'], incidences=catalogue['incidences'], levels=catalogue['levels'])
        for key in ['balls', 'incidences', 'levels'] + LEDGER:
            if got[key] != want[key]:
                errors.append('%s : %d, v11 %d (%d fils)' % (key, got[key], want[key], threads))
        if got['incidences'] != want['incidences_ledger']:
            errors.append('incidences du grand livre : %d, v11 %d' % (got['incidences'], want['incidences_ledger']))
        seen.append((digest['catalogue_sha256'], json.dumps(catalogue['ledger'], sort_keys=True)))
    if len(set(seen)) != 1:
        errors.append('empreinte ou grand livre differents selon le nombre de fils : %s' % [s[0][:16] for s in seen])
    for error in errors[:20]:
        print('ecart %s' % error)
    if errors:
        print('catalogue_ledger_ecart cas=%s ecarts=%d' % (options['case'], len(errors)))
        return 1
    if want['balls'] == 0:
        print('catalogue_ledger_plancher cas=%s' % options['case'])
        return 3
    print('catalogue_ledger_ok cas=%s boules=%d incidences=%d niveaux=%d fils=%s'
          % (options['case'], want['balls'], want['incidences'], want['levels'],
             ','.join(str(t) for t in options['threads'])))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
