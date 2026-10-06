#!/usr/bin/env python3
"""Porte mhgp11_api_supports_route_oracle (livraison L2b, docs/SORTIES.md paragraphe 11) : la sortie supports par
ses deux voies, arbre d'ordre K seul (build_order, masque 7035) et ordre K tire de FULL avec le journal des graines
(build_order_full, masque 16379 : voie de compute), donne le meme supports.mhgp11sp, le meme manifeste et les memes
registres du journal, octet pour octet, sur les nuages et aux ordres de l'oracle borne S1.

    python3 supports_route_oracle.py <mhgp11_api_supports_route_probe> --bits=<18|21|24> --work=<dossier>
            [--workers=<W>[,<W>...]] [--min-clouds=N] [--min-orders=N] [--min-balls=N] [--min-cells=N]
            [--min-extended=N] [--min-multiple=N] [--min-high=N]

Nuages et ordres : ceux de mhgp11_supports_hierarchy_fraction (suite de l'oracle reference/hgp11_ref : fixtures de la
specification et des audits, temoins D2 et E5, nuages d'Euler, fixtures historiques, familles a graine 31) et petit
temoin K10 de l'auditeur a ses ordres 1 a 12 ; PointId non denses ; nuages hors du profil exclus et comptes. Pour
chaque valeur de --workers (defaut 1,3), la sonde publie les deux voies dans des Sessions de W fils (points dans
l'ordre de l'oracle a W = 1, dans l'ordre inverse sinon). Controles, pour chaque (nuage, K, W) : une ligne de succes,
same = 1 (fichier, manifeste et journal identiques par les deux voies) ; puis, entre les W, meme fichier, meme
manifeste et meme journal (determinisme et permutation de l'entree).
Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Dernieres lignes :
    supports_route_couverture bits=<b> nuages=<c> exclus=<x> ordres=<o> boules=<w> noeuds=<v> supports=<s>
        etendues=<e> multiples=<m> cellules=<l> graines=<g> ordres_6plus=<h> fils=<W,...>
    supports_route_ok controles=<n>
Python 3.10 nu, bibliotheque standard seule, aucun assert ; aucun bytecode cree (PYTHONDONTWRITEBYTECODE).
"""
import json
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tower'))
sys.path.insert(0, os.path.join(HERE, '..', 'support'))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import attach_fraction  # noqa: E402  (requetes, PointId, temoin K10)
import mhgp11_gate  # noqa: E402
import test_supports  # noqa: E402  (suite de l'oracle S1)

FLOORS = ('clouds', 'orders', 'balls', 'cells', 'extended', 'multiple', 'high')
KEYS = ('balls', 'bytes', 'cells', 'extended', 'file', 'k', 'log', 'manifest', 'multiple', 'n', 'nodes', 'same',
        'seeds', 'status', 'supports')


class Usage(Exception):
    pass


def options(argv):
    if len(argv) < 3:
        raise Usage('arguments manquants')
    out = {'probe': argv[1], 'bits': 0, 'work': '', 'workers': [1, 3], 'floors': {}}
    for arg in argv[2:]:
        key, _, value = arg.partition('=')
        if key == '--bits' and value in ('18', '21', '24'):
            out['bits'] = int(value)
        elif key == '--work' and value:
            out['work'] = value
        elif key == '--workers' and value and all(w.isdigit() and 1 <= int(w) <= 64 for w in value.split(',')):
            out['workers'] = [int(w) for w in value.split(',')]
        elif key.startswith('--min-') and key[6:] in FLOORS and value.isdigit():
            out['floors'][key[6:]] = int(value)
        else:
            raise Usage('option inconnue ou invalide %s' % arg)
    if out['bits'] == 0 or not out['work']:
        raise Usage('--bits=18|21|24 et --work=<dossier> obligatoires')
    if len(set(out['workers'])) != len(out['workers']):
        raise Usage('--workers : valeurs en double')
    return out


def cases_of(gate, bits, clouds):
    """(nom, points, ids, K, None) des nuages du profil, au format des requetes d'attach_fraction ; exclus comptes."""
    cases, compared, excluded = [], 0, 0
    for name, points, ks in clouds:
        if max(c for p in points for c in p) >= 1 << bits or min(c for p in points for c in p) < 0:
            excluded += 1
            continue
        if not gate.check(len(points) <= attach_fraction.MAX_SITES, '%s : %d sites' % (name, len(points))):
            continue
        compared += 1
        ids = attach_fraction.point_ids(len(points))
        for k in ks:
            cases.append((name, points, ids, k, None))
    return cases, compared, excluded


def run(gate, o, workers, cases):
    """Une execution de la sonde sur toutes les requetes ; rend les lignes decodees (None si illisible)."""
    work = os.path.join(o['work'], 'w%d' % workers)
    shutil.rmtree(work, ignore_errors=True)
    done = mhgp11_gate.run([o['probe'], '--work=' + work, '--workers=%d' % workers], timeout=900,
                           stdin=attach_fraction.requests(cases, workers > 1))
    shutil.rmtree(work, ignore_errors=True)
    lines = (done.stdout or '').splitlines()
    gate.check(done.code == 0 and not (done.stderr or ''), 'sonde W%d : %s, sortie d\'erreur %r'
               % (workers, done.describe(), (done.stderr or '')[-300:]))
    if not gate.check(len(lines) == len(cases), 'sonde W%d : %d lignes pour %d requetes'
                      % (workers, len(lines), len(cases))):
        return [None] * len(cases)
    rows = []
    for (name, points, _ids, k, _doc), line in zip(cases, lines):
        where = '%s K%d W%d' % (name, k, workers)
        try:
            row = json.loads(line)
        except ValueError:
            gate.check(False, '%s : ligne illisible %r' % (where, line[:200]))
            rows.append(None)
            continue
        ok = gate.check(isinstance(row, dict) and tuple(sorted(row)) == KEYS, '%s : cles %r' % (where, line[:200]))
        ok = ok and gate.check_eq((row['status'], row['same'], row['k'], row['n']), ('ok', 1, k, len(points)),
                                  '%s : statut, accord des voies, K, n' % where)
        # Arbre couvrant (MHGP11SP 2) : un support S* par boule ; les cellules du journal ne sont plus une partie des
        # boules publiees (liaisons internes retirees).
        ok = ok and gate.check(row['supports'] == row['balls'] and row['multiple'] == 0 and row['bytes'] > 0 and
                               row['nodes'] > 0,
                               '%s : comptes incoherents %r' % (where, line[:200]))
        rows.append(row if ok else None)
    return rows


def main():
    gate = mhgp11_gate.Gate('supports_route')
    try:
        o = options(sys.argv)
    except Usage as error:
        print('usage : %s' % error)
        return mhgp11_gate.REFUSAL
    clouds = test_supports.fixtures() + test_supports.family_clouds() + [attach_fraction.WITNESS_K10]
    cases, compared, excluded = cases_of(gate, o['bits'], clouds)
    runs = [run(gate, o, w, cases) for w in o['workers']]
    totals = dict.fromkeys(('orders', 'balls', 'nodes', 'supports', 'extended', 'multiple', 'cells', 'seeds',
                            'high'), 0)
    for i, (name, _points, _ids, k, _doc) in enumerate(cases):
        rows = [r[i] for r in runs]
        if any(row is None for row in rows):
            continue
        first = rows[0]
        for w, row in zip(o['workers'][1:], rows[1:]):
            gate.check_eq((row['file'], row['manifest'], row['log'], row['cells'], row['seeds']),
                          (first['file'], first['manifest'], first['log'], first['cells'], first['seeds']),
                          '%s K%d : W%d contre W%d (fichier, manifeste, journal)' % (name, k, w, o['workers'][0]))
        totals['orders'] += 1
        totals['high'] += 1 if k >= 6 else 0
        for key in ('balls', 'nodes', 'supports', 'extended', 'multiple', 'cells', 'seeds'):
            totals[key] += first[key]
    totals['clouds'] = compared
    print('supports_route_couverture bits=%d nuages=%d exclus=%d ordres=%d boules=%d noeuds=%d supports=%d '
          'etendues=%d multiples=%d cellules=%d graines=%d ordres_6plus=%d fils=%s'
          % (o['bits'], compared, excluded, totals['orders'], totals['balls'], totals['nodes'], totals['supports'],
             totals['extended'], totals['multiple'], totals['cells'], totals['seeds'], totals['high'],
             ','.join(map(str, o['workers']))))
    below = [name for name in FLOORS if totals[name] < o['floors'].get(name, 0)]
    if gate.failures == 0 and below:
        print('PLANCHER supports_route : %s sous %s' % (', '.join(below), json.dumps(o['floors'], sort_keys=True)))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
