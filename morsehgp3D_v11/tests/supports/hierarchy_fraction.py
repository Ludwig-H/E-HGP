#!/usr/bin/env python3
"""Porte mhgp11_supports_hierarchy_fraction (tranche S6b) : differentiel COMPLET de la hierarchie des supports
native (build_support_hierarchy, sonde mhgp11_supports_hierarchy_probe en mode requetes) contre l'oracle borne S1
(reference/hgp11_ref/supports.py : Fraction, etage A seul) : noeuds, rattachements, roles, branches, Q_b et comptes.

    python3 hierarchy_fraction.py <mhgp11_supports_hierarchy_probe> --bits=<18|21|24> [--workers=<W>[,<W>...]]
            [--min-clouds=N] [--min-orders=N] [--min-balls=N] [--min-nodes=N] [--min-births=N] [--min-merges=N]
            [--min-internals=N] [--min-supports=N] [--min-extended=N] [--min-multiple=N] [--min-tetra=N]
            [--min-high=N]

Nuages et ordres : ceux de mhgp11_tower_attach_fraction (tests/tower/attach_fraction.py, dont les fonctions sont
reprises) : suite de l'oracle (fixtures de la specification et des audits, temoins D2 et E5, nuages d'Euler, fixtures
historiques, familles a graine 31) et petit temoin K10 de l'auditeur a ses ordres 1 a 12 ; PointId non denses. Pour
chaque valeur de --workers, la sonde construit l'arbre d'ordre K et la hierarchie (W = 1 : voie serielle sans Pool,
points dans l'ordre de l'oracle ; W > 1 : voie par lots et hierarchie sur un Pool de W fils, points dans l'ordre
inverse).

Controles, pour chaque (nuage, K, W) :
  - la sonde rend une ligne JSON, sans refus, avec le juge natif conforme (check vide : I5, I6, I11, rattachement) ;
  - ORDRE NATIF : boules par (postordre du noeud, rang, BallIdx) strictement croissants, listes propres contigues ;
    supports par (arite, SiteIdx lexicographiques), sites croissants, S* d'arite qmin en tete ; les SiteIdx sont les
    rangs de Morton des coordonnees (recalcules ici) ;
  - le niveau publie de chaque boule (hexadecimal) egale le rayon carre de la boule minimale de son S* (etage A),
    dont on tire le centre exact ;
  - la sonde, ramenee aux conventions de l'oracle (niveaux en fractions reduites, boules d'un meme noeud et d'un meme
    niveau par centre, sites des supports par coordonnees, supports par (arite, coordonnees) avec leurs comptes, centre
    de naissance de chaque noeud), egale la sortie canonique de l'oracle sur TOUS ses champs : k, n, sites, ids ; noeuds
    (level, parent, children, kind, post, balls, birth_center) ; boules (node, level, center, role, p, m, qmin,
    components, prior, supports, kparties_reliees, compressed_parts, strict_traces, cofaces, cofaces_support,
    gabriel_cofaces, gabriel_cofaces_support) ; tailles de sous-arbre egales a celles des enfants de l'oracle.
Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Dernieres lignes :
    hierarchy_fraction_couverture bits=<b> nuages=<c> exclus=<x> ordres=<o> boules=<w> noeuds=<v> naissances=<n>
        fusions=<f> internes=<i> supports=<s> etendues=<e> multiples=<m> tetraedres=<t> ordres_6plus=<h> fils=<W,...>
    hierarchy_fraction_ok controles=<n>
Python 3.10 nu, bibliotheque standard seule, aucun assert ; aucun bytecode cree (PYTHONDONTWRITEBYTECODE).
"""
import json
import os
import sys
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tower'))
sys.path.insert(0, os.path.join(HERE, '..', 'support'))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import attach_fraction  # noqa: E402  (nuages, requetes, comparaison de documents)
import mhgp11_gate  # noqa: E402
import sample_judge  # noqa: E402  (rangs de Morton : Cloud)
import test_supports  # noqa: E402

DOC_KEYS = ('balls', 'check', 'format', 'ids', 'k', 'n', 'nodes', 'sites', 'version')
NODE_KEYS = ('balls', 'birth_key', 'birth_site', 'children', 'kind', 'level', 'parent', 'post', 'size')
BALL_KEYS = ('cofaces', 'cofaces_support', 'components', 'compressed_parts', 'gabriel_cofaces',
             'gabriel_cofaces_support', 'key', 'kparties_reliees', 'level', 'm', 'node', 'p', 'prior', 'qmin', 'rank',
             'role', 'sites', 'strict_traces', 'supports')
COMPARED_NODE = ('level', 'parent', 'children', 'kind', 'post', 'balls', 'birth_center')
COMPARED_BALL = ('node', 'level', 'center', 'role', 'p', 'm', 'qmin', 'components', 'prior', 'supports',
                 'kparties_reliees', 'compressed_parts', 'strict_traces', 'cofaces', 'cofaces_support',
                 'gabriel_cofaces', 'gabriel_cofaces_support')
FLOORS = ('clouds', 'orders', 'balls', 'nodes', 'births', 'merges', 'internals', 'supports', 'extended', 'multiple',
          'tetra', 'high')


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


def oracle_cases(gate, bits, clouds):
    """(nom, points, ids, K, document normalise par JSON, oracle) des nuages du profil ; exclus comptes."""
    oracle_module = test_supports.STAGE['supports']
    cases, excluded, compared = [], 0, 0
    for name, points, ks in clouds:
        if max(c for p in points for c in p) >= 1 << bits or min(c for p in points for c in p) < 0:
            excluded += 1
            continue
        if not gate.check(len(points) <= attach_fraction.MAX_SITES, '%s : %d sites' % (name, len(points))):
            continue
        compared += 1
        ids = attach_fraction.point_ids(len(points))
        try:
            oracle = oracle_module.Supports(points)
            docs = [json.loads(json.dumps(oracle.canonical(k, ids), sort_keys=True)) for k in ks]
        except Exception as exc:  # noqa: BLE001 -- un lemme viole par l'oracle est un ecart, pas une trace Python
            gate.check(False, '%s : oracle : %s : %s' % (name, type(exc).__name__, exc))
            continue
        for k, doc in zip(ks, docs):
            cases.append((name, points, ids, k, doc, oracle))
    return cases, compared, excluded


def fraction(pair):
    return Fraction(int(pair[0], 16), int(pair[1], 16))


def native_order(gate, where, got, cloud):
    """Ordre natif : boules (post, rang, BallIdx), listes propres contigues, supports (arite, SiteIdx), Morton."""
    nodes, balls = got['nodes'], got['balls']
    keys = [(nodes[b['node']]['post'], b['rank'], b['key']) for b in balls]
    ok = gate.check(all(keys[i] < keys[i + 1] for i in range(len(keys) - 1)),
                    '%s : boules hors de l\'ordre (postordre, rang, BallIdx)' % where)
    for v, node in enumerate(nodes):
        own = node['balls']
        ok = gate.check(own == list(range(own[0], own[0] + len(own))) if own else True,
                        '%s : liste propre du noeud %d non contigue' % (where, v)) and ok
        ok = gate.check(all(balls[i]['node'] == v for i in own),
                        '%s : noeud %d, boules d\'un autre' % (where, v)) and ok
    for at, b in enumerate(balls):
        sites = b['sites']
        sorted_ok = (all(s == sorted(s) and len(set(s)) == len(s) for s in sites) and
                     all((len(sites[i]), sites[i]) < (len(sites[i + 1]), sites[i + 1]) for i in range(len(sites) - 1)))
        ok = gate.check(sites and len(sites[0]) == b['qmin'] and sorted_ok,
                        '%s boule %d : supports hors de l\'ordre (arite, SiteIdx)' % (where, at)) and ok
        morton = all(cloud.rank.get(tuple(p)) == s for q, r in zip(b['supports'], sites) for p, s in zip(q, r))
        ok = gate.check(morton and [len(q) for q in b['supports']] == [len(r) for r in sites],
                        '%s boule %d : SiteIdx et rangs de Morton' % (where, at)) and ok
    return ok


def normalize(gate, where, got, oracle, index_of):
    """Document de la sonde aux conventions de l'oracle (None sur ecart de niveau)."""
    centers = []
    for at, b in enumerate(got['balls']):
        star = tuple(sorted(index_of[tuple(p)] for p in b['supports'][0]))
        level, center, _closed = oracle.definition.meb(star)
        if not gate.check(level == fraction(b['level']), '%s boule %d : niveau publie %s, boule de S* %s'
                          % (where, at, fraction(b['level']), level)):
            return None
        centers.append(center)
    nodes = got['nodes']
    order = sorted(range(len(got['balls'])), key=lambda i: (nodes[got['balls'][i]['node']]['post'],
                                                            fraction(got['balls'][i]['level']), centers[i]))
    new = dict((old, j) for j, old in enumerate(order))
    by_key = dict((b['key'], i) for i, b in enumerate(got['balls']))
    balls = []
    for old in order:
        b = got['balls'][old]
        ranked = sorted(range(len(b['supports'])), key=lambda j: (len(b['supports'][j]), sorted(b['supports'][j])))
        out = dict((key, b[key]) for key in COMPARED_BALL if key in b)
        out['level'] = str(fraction(b['level']))
        out['center'] = [str(c) for c in centers[old]]
        out['supports'] = [sorted(b['supports'][j]) for j in ranked]
        out['cofaces_support'] = [b['cofaces_support'][j] for j in ranked]
        out['gabriel_cofaces_support'] = [b['gabriel_cofaces_support'][j] for j in ranked]
        balls.append(out)
    out_nodes = []
    for node in nodes:
        birth = None
        if node['kind'] == 0:
            birth = [str(c) for c in node['birth_site']]
        elif node['kind'] == 1:
            at = by_key.get(node['birth_key'])
            birth = None if at is None else [str(c) for c in centers[at]]
        out_nodes.append(dict(level=str(fraction(node['level'])), parent=node['parent'], children=node['children'],
                              kind=node['kind'], post=node['post'], balls=sorted(new[i] for i in node['balls']),
                              birth_center=birth))
    return dict(k=got['k'], n=got['n'], sites=got['sites'], ids=got['ids'], nodes=out_nodes, balls=balls)


def project(doc):
    out = dict((key, doc.get(key)) for key in ('k', 'n', 'sites', 'ids'))
    out['nodes'] = [dict((key, node.get(key)) for key in COMPARED_NODE) for node in doc['nodes']]
    out['balls'] = [dict((key, ball.get(key)) for key in COMPARED_BALL) for ball in doc['balls']]
    return out


def sizes_ok(got, want):
    size = [None] * len(want['nodes'])

    def walk(v):
        if size[v] is None:
            size[v] = 1 + sum(walk(c) for c in want['nodes'][v]['children'])
        return size[v]
    return all(node['size'] == walk(v) for v, node in enumerate(got['nodes']))


def compare_one(gate, where, line, case):
    _name, points, ids, _k, want, oracle = case
    try:
        got = json.loads(line)
    except ValueError:
        gate.check(False, '%s : ligne illisible %r' % (where, line[:200]))
        return
    if not gate.check(isinstance(got, dict) and 'status' not in got, '%s : refus %r' % (where, line[:200])):
        return
    shape = (sorted(got) == sorted(DOC_KEYS) and got.get('format') == 'hgp11_hierarchy_probe' and
             got.get('version') == 1 and all(sorted(v) == sorted(NODE_KEYS) for v in got['nodes']) and
             all(sorted(b) == sorted(BALL_KEYS) for b in got['balls']))
    if not gate.check(shape, '%s : cles ou format du document' % where):
        return
    gate.check(got['check'] == '', '%s : juge natif : %s' % (where, got['check']))
    if not native_order(gate, where, got, sample_judge.Cloud([tuple(p) for p in points], ids)):
        return
    index_of = dict((tuple(p), i) for i, p in enumerate(points))
    mine = normalize(gate, where, got, oracle, index_of)
    if mine is None:
        return
    found = attach_fraction.first_difference(project(mine), project(want), 'doc')
    gate.check(found is None, '%s : %s' % (where, found))
    gate.check(sizes_ok(got, want), '%s : tailles de sous-arbre' % where)


def compare_run(gate, probe, workers, cases):
    shaped = [case[:5] for case in cases]
    done = mhgp11_gate.run([probe, '--workers=%d' % workers], timeout=900,
                           stdin=attach_fraction.requests(shaped, workers > 1))
    lines = (done.stdout or '').splitlines()
    ok = gate.check(done.code == 0 and not (done.stderr or ''), 'sonde W%d : %s, sortie d\'erreur %r'
                    % (workers, done.describe(), (done.stderr or '')[-300:]))
    if not gate.check(len(lines) == len(cases), 'sonde W%d : %d lignes pour %d requetes'
                      % (workers, len(lines), len(cases))) or not ok:
        return
    for case, line in zip(cases, lines):
        compare_one(gate, '%s K%d W%d' % (case[0], case[3], workers), line, case)


def count(cases, compared, excluded):
    totals = dict((name, 0) for name in FLOORS)
    totals['clouds'] = compared
    for _name, _points, _ids, k, doc, _oracle in cases:
        totals['orders'] += 1
        totals['high'] += k >= 6
        totals['nodes'] += len(doc['nodes'])
        for ball in doc['balls']:
            totals['balls'] += 1
            totals['births'] += ball['role'] == 'naissance'
            totals['merges'] += ball['role'] == 'fusion'
            totals['internals'] += ball['role'] == 'interne'
            totals['supports'] += len(ball['supports'])
            totals['extended'] += ball['m'] > ball['qmin']
            totals['multiple'] += len(ball['supports']) > 1
            totals['tetra'] += sum(len(q) == 4 for q in ball['supports'])
    totals['excluded'] = excluded
    return totals


def main():
    gate = mhgp11_gate.Gate('hierarchy_fraction')
    try:
        o = options(sys.argv)
    except Usage as error:
        print('usage : %s' % error)
        return mhgp11_gate.REFUSAL
    clouds = test_supports.fixtures() + test_supports.family_clouds() + [attach_fraction.WITNESS_K10]
    cases, compared, excluded = oracle_cases(gate, o['bits'], clouds)
    for workers in o['workers']:
        compare_run(gate, o['probe'], workers, cases)
    t = count(cases, compared, excluded)
    print('hierarchy_fraction_couverture bits=%d nuages=%d exclus=%d ordres=%d boules=%d noeuds=%d naissances=%d '
          'fusions=%d internes=%d supports=%d etendues=%d multiples=%d tetraedres=%d ordres_6plus=%d fils=%s'
          % (o['bits'], t['clouds'], t['excluded'], t['orders'], t['balls'], t['nodes'], t['births'], t['merges'],
             t['internals'], t['supports'], t['extended'], t['multiple'], t['tetra'], t['high'],
             ','.join(map(str, o['workers']))))
    below = [name for name in FLOORS if t[name] < o['floors'].get(name, 0)]
    if gate.failures == 0 and below:
        print('PLANCHER hierarchy_fraction : %s sous %s' % (', '.join(below), json.dumps(o['floors'], sort_keys=True)))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
