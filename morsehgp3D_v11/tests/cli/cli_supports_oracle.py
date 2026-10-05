#!/usr/bin/env python3
"""Porte mhgp11_cli_supports_oracle (tranche S7) : le fichier supports.mhgp11sp publie par l'executable, relu par le
lecteur officiel en bibliotheque standard (bench/mhgp11_formats.py), egale le vidage canonique de l'oracle borne S1
(reference/hgp11_ref/supports.py, canonical(k, ids) : Fraction, etage A seul), sur tous ses champs.

    python3 cli_supports_oracle.py --cli <mhgp11> --bits <18|21|24> [--min-clouds=N] [--min-orders=N]
            [--min-balls=N] [--min-extended=N] [--min-multiple=N] [--min-signatures=N]

Nuages : ceux de mhgp11_supports_hierarchy_fraction (tests/supports/hierarchy_fraction.py : suite de l'oracle S1,
fixtures de la specification et des audits, temoins D2 et E5, nuages d'Euler, fixtures historiques, familles a graine
31, et le petit temoin K10 de l'auditeur a ses ordres 1 a 12), PointId non denses. Un nuage hors du profil est exclu
et compte.

Pour chaque (nuage, K) : un appel --sortie=supports, sur un fil (points dans l'ordre de l'oracle) ou sur quatre
(points dans l'ordre inverse), en alternance. Controles :
  - code 0, une ligne d'etat de succes (output supports, comptes nodes, balls, supports, prior egaux au manifeste) ;
  - le dossier est conforme au lecteur (check_directory : manifeste canonique, inventaire, taille, sha256, decodage
    strict et tous les controles de read_supports, agregats du manifeste recomptes, tree_k_sha256 egal a la
    signature version 2 recalculee depuis le fichier) ;
  - le fichier decode, ramene aux conventions de l'oracle (sites et ids en ordre lexicographique, niveaux et centres
    exacts tires de S*, boules d'un noeud par (niveau, centre), supports par (arite, coordonnees), comptes derives par
    le lecteur), egale canonical(k, ids) sur k, n, sites, ids, noeuds (level, parent, children, kind, post, balls,
    birth_center) et boules (node, level, center, role, p, m, qmin, components, prior, supports, kparties_reliees,
    compressed_parts, strict_traces, cofaces, cofaces_support, gabriel_cofaces, gabriel_cofaces_support).
Pour chaque (nuage, K) avec K <= 4 : un appel --sortie=full sur la meme entree ; son tree_k_sha256 egale celui de la
sortie supports, lui-meme juge contre la serialisation version 2 du lecteur (signature commune des sorties, docs/
SORTIES.md, paragraphe 8).
Plafond de coquille (appel entier) : 25 des 30 sites de x^2 + y^2 + z^2 = 9 (temoin sphere9 de S6b), K1 et K2, un et
quatre fils : code 2, support_shell_capacity a l'etape compute, ni D ni D.pending ; --sortie=full sur la meme entree
est conforme (aucun plafond pour FULL). Sphere5 (24 sites) admise : la boule centrale publie ses 828 supports.

Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Dernieres lignes :
    cli_supports_oracle_couverture bits=<b> nuages=<c> exclus=<x> ordres=<o> boules=<w> etendues=<e> multiples=<m>
        tetraedres=<t> ordres_6plus=<h> signatures=<s> refus=<r>
    cli_supports_oracle_ok controles=<n>
Python 3.10 nu, bibliotheque standard seule, aucun assert ; aucun bytecode cree (PYTHONDONTWRITEBYTECODE).
"""
import json
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'supports'))
sys.path.insert(0, os.path.join(HERE, '..', 'tower'))
import cli_support as cs  # noqa: E402
import attach_fraction  # noqa: E402  (first_difference, temoin K10)
import hierarchy_fraction  # noqa: E402  (nuages et cas de l'oracle, projection des documents)
import mhgp11_gate  # noqa: E402
import test_supports  # noqa: E402

formats = cs.formats
ROLE_NAMES = ('naissance', 'fusion', 'interne')
FLOORS = ('clouds', 'orders', 'balls', 'extended', 'multiple', 'signatures')


class Usage(Exception):
    pass


def options(argv):
    out = {'cli': None, 'bits': 0, 'floors': {}}
    args = list(argv[1:])
    while args:
        arg = args.pop(0)
        key, eq, value = arg.partition('=')
        if key in ('--cli', '--bits') and not eq:
            if not args:
                raise Usage('valeur manquante pour %s' % key)
            value = args.pop(0)
        if key == '--cli' and value:
            out['cli'] = value
        elif key == '--bits' and value in ('18', '21', '24'):
            out['bits'] = int(value)
        elif key.startswith('--min-') and key[6:] in FLOORS and value.isdigit():
            out['floors'][key[6:]] = int(value)
        else:
            raise Usage('option inconnue ou invalide %s' % arg)
    if out['cli'] is None or out['bits'] == 0:
        raise Usage('--cli et --bits=18|21|24 obligatoires')
    return out


def oracle_document(sp):
    """Fichier decode (formats.SupportsFile) aux conventions de canonical(k, ids) de l'oracle, normalise par JSON."""
    pts = sp.points
    lex = sorted(range(sp.n), key=lambda s: pts[s])
    centers = [sp.center(b) for b in range(sp.B)]
    levels = [sp.level(b) for b in range(sp.B)]
    order = sorted(range(sp.B), key=lambda b: (sp.post[sp.node_of[b]], levels[b], centers[b]))
    new = dict((old, j) for j, old in enumerate(order))
    balls = []
    for b in order:
        supports = sp.supports_of(b)
        counts = sp.counts(b)
        coords = [sorted(list(pts[s]) for s in q) for q in supports]
        ranked = sorted(range(len(supports)), key=lambda j: (len(coords[j]), coords[j]))
        first = sp.prior_at[b]
        balls.append(dict(
            node=sp.node_of[b], level=str(levels[b]), center=[str(c) for c in centers[b]],
            role=ROLE_NAMES[sp.role[b]], p=sp.p[b], m=sp.m[b], qmin=sp.qmin[b], components=counts['components'],
            prior=list(sp.prior[first:first + sp.prior_count[b]]), supports=[coords[j] for j in ranked],
            kparties_reliees=counts['kparties_reliees'], compressed_parts=counts['compressed_parts'],
            strict_traces=counts['strict_traces'], cofaces=counts['cofaces'],
            cofaces_support=[counts['cofaces_support'][j] for j in ranked], gabriel_cofaces=counts['gabriel_cofaces'],
            gabriel_cofaces_support=[counts['gabriel_cofaces_support'][j] for j in ranked]))
    nodes = []
    for v in range(sp.N):
        own = range(sp.first_ball[v], sp.first_ball[v] + sp.ball_count[v])
        kind = sp.kind[v]
        if kind == formats.KIND_SITE:
            level, birth = '0', [str(c) for c in pts[sp.leaf_site[v]]]
        else:
            level = str(levels[sp.first_ball[v]])
            birth = [str(c) for c in centers[sp.first_ball[v]]] if kind == formats.KIND_BIRTH else None
        nodes.append(dict(level=level, parent=None if v == sp.root else sp.parent[v], children=list(sp.children[v]),
                          kind=kind, post=sp.post[v], balls=sorted(new[b] for b in own), birth_center=birth))
    doc = dict(k=sp.k, n=sp.n, sites=[list(pts[s]) for s in lex], ids=[sp.point_id[s] for s in lex], nodes=nodes,
               balls=balls)
    return json.loads(json.dumps(doc))


class Runner:
    def __init__(self, gate, args, root):
        self.gate, self.args, self.root = gate, args, root
        self.calls = 0

    def publish(self, where, points, ids, k, workers, output, reverse=False):
        """Ecrit l'entree, appelle le CLI, relit le dossier ; rend (rapport du lecteur, ligne) ou (None, ligne)."""
        folder = tempfile.mkdtemp(prefix='c', dir=self.root)
        order = list(reversed(range(len(points)))) if reverse else None
        xyz, names = cs.write_inputs(folder, points, ids, order=order)
        directory = os.path.join(folder, 'D')
        argv = cs.cli_argv(self.args['cli'], xyz, names, directory, k, workers=workers, output=output)
        result, rows = cs.run_cli(argv, timeout=300)
        self.calls += 1
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        if not self.gate.check(result.code == 0 and line.get('status') == 'ok' and line.get('output') == output,
                               '%s %s : %s, ligne %r, erreur %r' % (where, output, result.describe(), rows,
                                                                    (result.stderr or '')[-300:])):
            shutil.rmtree(folder, ignore_errors=True)
            return None, line
        try:
            report = formats.check_directory(directory, self.args['bits'])
        except (OSError, ValueError) as error:
            self.gate.check(False, '%s %s : dossier non conforme au lecteur : %s' % (where, output, error))
            report = None
        shutil.rmtree(folder, ignore_errors=True)
        return report, line


def compare(gate, where, report, line, case):
    _name, _points, _ids, _k, want, _oracle = case
    sp, manifest = report['decoded'], report['manifest']
    counts = manifest['counts']
    gate.check_eq(line.get('counts'), dict(nodes=counts['nodes'], balls=counts['balls'], supports=counts['supports'],
                                           prior=counts['prior']), '%s : comptes de la ligne d\'etat' % where)
    try:
        mine = oracle_document(sp)
    except (ValueError, KeyError, IndexError) as error:
        gate.check(False, '%s : conversion aux conventions de l\'oracle : %s' % (where, error))
        return
    found = attach_fraction.first_difference(hierarchy_fraction.project(mine), hierarchy_fraction.project(want), 'doc')
    gate.check(found is None, '%s : fichier contre oracle : %s' % (where, found))


def sphere(r2, shift):
    return [(x + shift, y + shift, z + shift) for x in range(-3, 4) for y in range(-3, 4) for z in range(-3, 4)
            if x * x + y * y + z * z == r2]


def sphere9_25():
    """25 des 30 sites de x^2 + y^2 + z^2 = 9 translates de (3, 3, 3) : les cinq derniers non axiaux retires (temoin
    sphere9 de tests/supports/hierarchy_test.cpp)."""
    kept, dropped = [], 0
    for p in reversed(sphere(9, 3)):
        axial = sum(1 for c in p if c == 3) == 2
        if not axial and dropped < 5:
            dropped += 1
            continue
        kept.append(p)
    return kept


def shell_cases(gate, runner):
    """Plafond de coquille : refus de l'appel entier a 25 sites, FULL conforme, sphere5 admise. Rend le nombre de
    refus constates."""
    points = sphere9_25()
    if not gate.check(len(points) == 25, 'sphere9 : %d sites' % len(points)):
        return 0
    ids = [cs.NONE - 3 * i for i in range(len(points))]
    refusals = 0
    for k in (1, 2):
        for workers in (1, 4):
            folder = tempfile.mkdtemp(prefix='s', dir=runner.root)
            xyz, names = cs.write_inputs(folder, points, ids)
            d = os.path.join(folder, 'D')
            result, rows = cs.run_cli(cs.cli_argv(runner.args['cli'], xyz, names, d, k, workers=workers,
                                                  output='supports'), timeout=120)
            line = rows[0] if len(rows) == 1 and rows[0] else {}
            want = dict(output='supports', status='unsupported_degeneracy', reason='support_shell_capacity',
                        stage='compute', publication='none', manifest_sha256=None)
            gate.check(result.code == 2 and dict((key, line.get(key)) for key in want) == want,
                       'sphere9 K%d W%d : %s, ligne %r' % (k, workers, result.describe(), rows))
            gate.check(not os.path.lexists(d) and not os.path.lexists(d + '.pending'),
                       'sphere9 K%d W%d : D ou D.pending present apres le refus' % (k, workers))
            refusals += result.code == 2
            shutil.rmtree(folder, ignore_errors=True)
        report, _line = runner.publish('sphere9 K%d' % k, points, ids, k, 1, 'full')
        gate.check(report is not None, 'sphere9 K%d : FULL refuse' % k)
    report, _line = runner.publish('sphere5 K1', sphere(5, 2), list(range(24)), 1, 4, 'supports')
    if gate.check(report is not None, 'sphere5 K1 : supports refuse'):
        gate.check_eq(report['manifest']['counts']['max_supports_per_ball'], 828, 'sphere5 K1 : supports de la '
                      'boule centrale')
    return refusals


def main():
    gate = mhgp11_gate.Gate('cli_supports_oracle')
    try:
        o = options(sys.argv)
    except Usage as error:
        print('usage : %s' % error)
        return mhgp11_gate.REFUSAL
    clouds = test_supports.fixtures() + test_supports.family_clouds() + [attach_fraction.WITNESS_K10]
    cases, compared, excluded = hierarchy_fraction.oracle_cases(gate, o['bits'], clouds)
    t = dict((name, 0) for name in FLOORS)
    t.update(clouds=compared, tetra=0, high=0)
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-supports-') as root:
        runner = Runner(gate, o, root)
        for at, case in enumerate(cases):
            name, points, ids, k, want, _oracle = case
            workers, reverse = (1, False) if at % 2 == 0 else (4, True)
            where = '%s K%d W%d' % (name, k, workers)
            report, line = runner.publish(where, points, ids, k, workers, 'supports', reverse=reverse)
            if report is None:
                continue
            compare(gate, where, report, line, case)
            t['orders'] += 1
            t['high'] += k >= 6
            for ball in want['balls']:
                t['balls'] += 1
                t['extended'] += ball['m'] > ball['qmin']
                t['multiple'] += len(ball['supports']) > 1
                t['tetra'] += sum(len(q) == 4 for q in ball['supports'])
            if k <= 4:
                full, _ = runner.publish(where, points, ids, k, workers, 'full', reverse=not reverse)
                if full is not None:
                    gate.check_eq(full['manifest']['tree_k_sha256'], report['manifest']['tree_k_sha256'],
                                  '%s : tree_k_sha256 de full et de supports' % where)
                    t['signatures'] += 1
        refusals = shell_cases(gate, runner)
    print('cli_supports_oracle_couverture bits=%d nuages=%d exclus=%d ordres=%d boules=%d etendues=%d multiples=%d '
          'tetraedres=%d ordres_6plus=%d signatures=%d refus=%d'
          % (o['bits'], t['clouds'], excluded, t['orders'], t['balls'], t['extended'], t['multiple'], t['tetra'],
             t['high'], t['signatures'], refusals))
    below = [name for name in FLOORS if t[name] < o['floors'].get(name, 0)]
    if gate.failures == 0 and (below or refusals != 4):
        print('PLANCHER cli_supports_oracle : %s sous %s, refus %d sur 4'
              % (', '.join(below), json.dumps(o['floors'], sort_keys=True), refusals))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
