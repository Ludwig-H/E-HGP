#!/usr/bin/env python3
"""Porte mhgp11_tower_attach_fraction (tranche S3 ; apports des auditeurs du 5 octobre 2026) : differentiel de la
sonde du rattachement (mhgp11_tower_attach_probe, mode requetes) contre l'oracle borne S1
(reference/hgp11_ref/supports.py : Fraction, etage A seul), projete sur les champs de S3.

    python3 attach_fraction.py <mhgp11_tower_attach_probe> --bits=<18|21|24> [--workers=<W>[,<W>...]]
            [--min-clouds=N] [--min-orders=N] [--min-balls=N] [--min-nodes=N] [--min-births=N] [--min-merges=N]
            [--min-internals=N] [--min-passing=N] [--min-weak=N] [--min-wide=N] [--min-high=N]

Nuages :
  - ceux de la suite de l'oracle (reference/test_supports.py : fixtures de la specification et des audits, dont les
    temoins D2 et E5, nuages d'Euler, fixtures historiques a positions distinctes, puis 16 nuages de 5 a 10 points par
    famille, graine 31), a leurs ordres (ceux de la specification, et 1 a min(5, n - 1)) ;
  - le petit temoin a K eleve de l'auditeur (receipts/audit_native_integration_20261005/qb/normal.json : les quatre
    coins d'un carre de cote 20 et huit sites interieurs, 12 sites), a tous ses ordres 1 a 12 : la boule de centre
    (10, 10, 10) et de niveau 200 (p = 8, m = 4, qmin = 2) y est interne a K9, fusion de quatre branches a K10
    (quatre traces strictes) et naissance a K11 et K12. La suite de l'oracle s'arrete a K5 : ce temoin est le seul
    cas borne de l'arbre et du rattachement a K >= 6.
PointId arbitraires, non denses : 2654435761 (i + 1) mod 2^32 pour le i-ieme point de l'oracle.

Pour chaque (nuage, K), la sonde prepare Cat_K (kmax = K : le domaine etroit de la facade, seul ou le temoin D2 est un
contre-cas) et construit l'arbre d'ordre K seul (build_order), une fois par valeur de --workers : la voie serielle
(W = 1) recoit les points dans l'ordre de l'oracle, la voie par lots (W > 1, Pool de W fils) dans l'ordre inverse
(permutation effective des que n >= 2 ; la sortie canonique n'en depend pas). L'oracle rend la sortie canonique de
l'ordre K (canonical(k, ids)) : arbre par la definition (composantes de Gamma_K sur toutes les K-parties), W_K,
att(b) a la coupe fermee sur toutes les K-parties de P_b, ant(b) a la coupe ouverte des K-parties strictes, roles par
les niveaux, traces strictes par enumeration, lemmes A a H controles.

Controles, pour chaque (nuage, K, W) :
  - la sonde rend une ligne JSON par requete, code 0, rien sur la sortie d'erreur ; format hgp11_attach_probe,
    version 1, exactement les cles attendues (document, noeuds, boules) ;
  - le document, sans format, version et s_star, egale la projection de l'oracle sur les champs de S3 : k, n, sites
    (ordre lexicographique), ids ; noeuds (level, parent, children, kind, post, balls, birth_center) ; boules dans
    l'ordre (postordre du noeud, niveau, centre exact) avec node (att(b)), level, center, role, p, m, qmin,
    components (|ant(b)|), prior (ant(b), role fusion seulement) et strict_traces ;
  - S* de la sonde (s_star) est un support positif minimal de l'oracle, d'arite qmin.
Un nuage hors du domaine du profil (une coordonnee >= 2^bits) est exclu et compte (u18 : les deux cercles n = 1023
de la specification) ; tous les autres sont compares.
Codes : 0 conforme ; 1 ecart (dont un refus ou un arret de la sonde, et un lemme viole par l'oracle) ; 2 refus
d'usage ; 3 plancher. Dernieres lignes :
    attach_fraction_couverture bits=<b> nuages=<c> exclus=<x> ordres=<o> boules=<w> noeuds=<v> naissances=<n>
        fusions=<f> internes=<i> passageres=<s> faibles=<a> fusions_3plus=<t> ordres_6plus=<h> fils=<W,...>
    attach_fraction_ok controles=<n>
Les compteurs viennent de l'oracle, une fois par (nuage, K) ; chaque valeur de --workers les compare tous.
Python 3.10 nu, bibliotheque standard seule, aucun assert. L'oracle est charge sans le paquet hgp11_ref
(ref_mutants.load_private, comme reference/test_supports.py) ; la porte ne cree aucun bytecode
(PYTHONDONTWRITEBYTECODE). Prototype : impl_s3/attach_vs_oracle.py du rapport S3 (hors depot).
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'support'))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import mhgp11_gate  # noqa: E402
import test_supports  # noqa: E402  (suite de l'oracle S1 : fixtures, familles, chargement prive)

DOC_KEYS = ('balls', 'format', 'ids', 'k', 'n', 'nodes', 'sites', 'version')
NODE_KEYS = ('balls', 'birth_center', 'children', 'kind', 'level', 'parent', 'post')
BALL_KEYS = ('center', 'components', 'level', 'm', 'node', 'p', 'prior', 'qmin', 'role', 'strict_traces')
PROBE_BALL_KEYS = BALL_KEYS + ('s_star',)
FLOORS = ('clouds', 'orders', 'balls', 'nodes', 'births', 'merges', 'internals', 'passing', 'weak', 'wide', 'high')
BUDGET = 1 << 32   # octets par requete (MemoryBudget de la sonde)
MAX_SITES = 14     # mode requetes de la sonde
ABSENT = '<absent>'
# Petit temoin a K eleve de l'auditeur (receipts/audit_native_integration_20261005/qb/normal.json) : quatre coins,
# huit sites interieurs ; ordres 1 a 12 (K <= n).
WITNESS_K10 = ('temoin_k10_audit',
               [(0, 0, 10), (0, 20, 10), (20, 0, 10), (20, 20, 10), (9, 9, 10), (9, 10, 10), (9, 11, 10),
                (10, 9, 10), (10, 10, 10), (10, 11, 10), (11, 9, 10), (11, 10, 10)],
               tuple(range(1, 13)))


class Usage(Exception):
    pass


def options(argv):
    if len(argv) < 3:
        raise Usage('arguments manquants')
    out = {'probe': argv[1], 'bits': 0, 'workers': [1], 'floors': {}}
    for arg in argv[2:]:
        key, _, value = arg.partition('=')
        if key == '--bits' and value in ('18', '21', '24'):
            out['bits'] = int(value)
        elif key == '--workers' and value and all(w.isdigit() and 1 <= int(w) <= 64 for w in value.split(',')):
            out['workers'] = [int(w) for w in value.split(',')]
        elif key.startswith('--min-') and key[6:] in FLOORS and value.isdigit():
            out['floors'][key[6:]] = int(value)
        else:
            raise Usage('option inconnue ou invalide %s' % arg)
    if out['bits'] == 0:
        raise Usage('--bits=18|21|24 obligatoire')
    if len(set(out['workers'])) != len(out['workers']):
        raise Usage('--workers : valeurs en double')
    return out


def point_ids(count):
    return [(2654435761 * (i + 1)) & 0xFFFFFFFF for i in range(count)]


def project(doc):
    """Champs de S3 d'un document (sonde ou oracle) ; une cle absente vaut ABSENT, jamais une exception."""
    out = dict((key, doc.get(key, ABSENT)) for key in ('k', 'n', 'sites', 'ids'))
    nodes, balls = doc.get('nodes'), doc.get('balls')
    out['nodes'] = ([dict((key, node.get(key, ABSENT)) for key in NODE_KEYS) for node in nodes]
                    if isinstance(nodes, list) and all(isinstance(node, dict) for node in nodes) else ABSENT)
    out['balls'] = ([dict((key, ball.get(key, ABSENT)) for key in BALL_KEYS) for ball in balls]
                    if isinstance(balls, list) and all(isinstance(ball, dict) for ball in balls) else ABSENT)
    return out


def first_difference(got, want, path):
    """Premier chemin ou deux valeurs JSON different, ou None."""
    if type(got) is not type(want):
        return '%s : %r contre %r' % (path, got, want)
    if isinstance(want, dict):
        for key in sorted(set(got) | set(want)):
            if key not in got or key not in want:
                return '%s.%s : cle presente d\'un seul cote' % (path, key)
            found = first_difference(got[key], want[key], '%s.%s' % (path, key))
            if found:
                return found
        return None
    if isinstance(want, list):
        if len(got) != len(want):
            return '%s : longueur %d contre %d' % (path, len(got), len(want))
        for i, (a, b) in enumerate(zip(got, want)):
            found = first_difference(a, b, '%s[%d]' % (path, i))
            if found:
                return found
        return None
    return None if got == want else '%s : %r contre %r' % (path, got, want)


def oracle_cases(gate, bits, clouds):
    """(nom, points, ids, K, document de l'oracle normalise par JSON) des nuages du profil ; exclus comptes."""
    oracle_module = test_supports.STAGE['supports']
    cases, excluded, compared = [], 0, 0
    for name, points, ks in clouds:
        if max(c for p in points for c in p) >= 1 << bits or min(c for p in points for c in p) < 0:
            excluded += 1
            continue
        if not gate.check(len(points) <= MAX_SITES, '%s : %d sites, au plus %d en mode requetes'
                          % (name, len(points), MAX_SITES)):
            continue
        compared += 1
        ids = point_ids(len(points))
        try:
            oracle = oracle_module.Supports(points)
            docs = [json.loads(json.dumps(oracle.canonical(k, ids), sort_keys=True)) for k in ks]
        except Exception as exc:  # noqa: BLE001 -- un lemme viole par l'oracle est un ecart, pas une trace Python
            gate.check(False, '%s : oracle : %s : %s' % (name, type(exc).__name__, exc))
            continue
        for k, doc in zip(ks, docs):
            cases.append((name, points, ids, k, doc))
    return cases, compared, excluded


def requests(cases, reverse):
    text = []
    for _name, points, ids, k, _doc in cases:
        order = list(range(len(points)))
        if reverse:
            order.reverse()
        text.append('%d %d %d\n' % (k, BUDGET, len(points)))
        text.extend('%d %d %d %d\n' % (points[i][0], points[i][1], points[i][2], ids[i]) for i in order)
    return ''.join(text)


def compare_run(gate, probe, workers, cases):
    """Une execution de la sonde sur toutes les requetes ; chaque ligne comparee a l'oracle."""
    done = mhgp11_gate.run([probe, '--workers=%d' % workers], timeout=600, stdin=requests(cases, workers > 1))
    lines = (done.stdout or '').splitlines()
    ok = gate.check(done.code == 0 and not (done.stderr or ''), 'sonde W%d : %s, sortie d\'erreur %r'
                    % (workers, done.describe(), (done.stderr or '')[-300:]))
    if not gate.check(len(lines) == len(cases), 'sonde W%d : %d lignes pour %d requetes'
                      % (workers, len(lines), len(cases))) or not ok:
        return
    shown = 0
    for (name, _points, _ids, k, want), line in zip(cases, lines):
        where = '%s K%d W%d' % (name, k, workers)
        try:
            got = json.loads(line)
        except ValueError:
            gate.check(False, '%s : ligne illisible %r' % (where, line[:200]))
            continue
        if not gate.check(isinstance(got, dict) and 'status' not in got, '%s : refus %r' % (where, line[:200])):
            continue
        gate.check(got.get('format') == 'hgp11_attach_probe' and got.get('version') == 1,
                   '%s : format %r version %r' % (where, got.get('format'), got.get('version')))
        nodes, balls = got.get('nodes'), got.get('balls')
        shape = (sorted(got) == sorted(DOC_KEYS) and isinstance(nodes, list) and isinstance(balls, list) and
                 all(isinstance(v, dict) and sorted(v) == sorted(NODE_KEYS) for v in nodes) and
                 all(isinstance(b, dict) and sorted(b) == sorted(PROBE_BALL_KEYS) for b in balls))
        gate.check(shape, '%s : cles du document, des noeuds ou des boules' % where)
        found = first_difference(project(got), project(want), 'doc')
        if not gate.check(found is None, '%s : %s' % (where, found)):
            shown += 1
            if shown <= 3:
                print('  sonde  : %s' % json.dumps(project(got), sort_keys=True)[:600])
                print('  oracle : %s' % json.dumps(project(want), sort_keys=True)[:600])
            continue
        for at, (mine, theirs) in enumerate(zip(balls, want['balls'])):
            star = mine.get('s_star')
            supports = [sorted(map(tuple, s)) for s in theirs['supports']]
            gate.check(isinstance(star, list) and len(star) == theirs['qmin'] and
                       sorted(map(tuple, star)) in supports,
                       '%s boule %d (%s) : S* %r hors des supports d\'arite qmin' % (where, at, theirs['level'], star))


def count(cases, compared, excluded):
    totals = dict((name, 0) for name in FLOORS)
    totals['clouds'] = compared
    for _name, _points, _ids, k, doc in cases:
        totals['orders'] += 1
        totals['high'] += k >= 6
        totals['nodes'] += len(doc['nodes'])
        totals['wide'] += sum(len(node['children']) >= 3 for node in doc['nodes'])
        for ball in doc['balls']:
            totals['balls'] += 1
            totals['births'] += ball['role'] == 'naissance'
            totals['merges'] += ball['role'] == 'fusion'
            totals['internals'] += ball['role'] == 'interne'
            totals['passing'] += ball['role'] == 'fusion' and ball['components'] == 1
            totals['weak'] += ball['p'] + ball['qmin'] == k + 1
    totals['excluded'] = excluded
    return totals


def main():
    gate = mhgp11_gate.Gate('attach_fraction')
    try:
        o = options(sys.argv)
    except Usage as error:
        print('usage : %s' % error)
        return mhgp11_gate.REFUSAL
    clouds = test_supports.fixtures() + test_supports.family_clouds() + [WITNESS_K10]
    cases, compared, excluded = oracle_cases(gate, o['bits'], clouds)
    for workers in o['workers']:
        compare_run(gate, o['probe'], workers, cases)
    t = count(cases, compared, excluded)
    print('attach_fraction_couverture bits=%d nuages=%d exclus=%d ordres=%d boules=%d noeuds=%d naissances=%d '
          'fusions=%d internes=%d passageres=%d faibles=%d fusions_3plus=%d ordres_6plus=%d fils=%s'
          % (o['bits'], t['clouds'], t['excluded'], t['orders'], t['balls'], t['nodes'], t['births'], t['merges'],
             t['internals'], t['passing'], t['weak'], t['wide'], t['high'], ','.join(map(str, o['workers']))))
    below = [name for name in FLOORS if t[name] < o['floors'].get(name, 0)]
    if gate.failures == 0 and below:
        print('PLANCHER attach_fraction : %s sous %s' % (', '.join(below), json.dumps(o['floors'], sort_keys=True)))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
