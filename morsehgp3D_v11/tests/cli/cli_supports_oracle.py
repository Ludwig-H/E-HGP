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
  - le fichier decode (MHGP11SP version 2), ramene aux conventions de l'oracle (sites et ids en ordre
    lexicographique, niveaux et centres exacts tires de S*, boules d'un noeud par (niveau, centre)), egale la
    projection de canonical(k, ids) sur l'arbre couvrant d'ordre K (decision de l'utilisateur du 6 octobre 2026) :
    naissances et boules ayant au moins une union reussie du Kruskal compresse, renumerotees, S* seul (plus petite arite, puis SiteIdx de Morton),
    sur k, n, sites, ids, noeuds (level, parent, children, kind, post, balls, birth_center) et boules (node, level,
    center, role, p, m, qmin, components, prior, supports). Les comptes de Q_b ne sont plus publies.
Pour chaque (nuage, K) avec K <= 4 : un appel --sortie=full sur la meme entree ; son tree_k_sha256 egale celui de la
sortie supports, lui-meme juge contre la serialisation version 2 du lecteur (signature commune des sorties, docs/
SORTIES.md, paragraphe 8).
Coquilles larges : 25 des 30 sites de x^2 + y^2 + z^2 = 9 (temoin sphere9 de S6b), K1 et K2, un et quatre fils, puis
sphere5 (24 sites) : admis par l'arbre couvrant (aucune enumeration de Q_b, aucun plafond de 24 sites), un S* par
boule.

Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Dernieres lignes :
    cli_supports_oracle_couverture bits=<b> nuages=<c> exclus=<x> ordres=<o> boules=<w> etendues=<e> multiples=<m>
        tetraedres=<t> ordres_6plus=<h> signatures=<s> larges=<r>
    cli_supports_oracle_ok controles=<n>
Python 3.10 nu, bibliotheque standard seule, aucun assert ; aucun bytecode cree (PYTHONDONTWRITEBYTECODE).
"""
import json
import os
import shutil
import sys
import tempfile
from fractions import Fraction

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
# Champs compares d'une boule de l'arbre couvrant (MHGP11SP version 2) : sans les comptes de Q_b.
SPANNING_BALL = ('node', 'level', 'center', 'role', 'p', 'm', 'qmin', 'components', 'prior', 'supports')
SPANNING_NODE = ('level', 'parent', 'children', 'kind', 'post', 'balls', 'birth_center')


def star(supports):
    """S* parmi les supports (listes de coordonnees) : plus petite arite, puis ordre lexicographique des SiteIdx, ici
    des cles de Morton croissantes (SiteIdx = rang de Morton), comme le catalogue."""
    q = min(len(x) for x in supports)
    return min((x for x in supports if len(x) == q), key=lambda x: sorted(formats.morton(tuple(p)) for p in x))


def spanning(doc):
    """Select from the complete oracle document only; never reselect a native result."""
    nodes, balls = doc['nodes'], doc['balls']
    coords = [tuple(p) for p in doc['sites']]
    if len(coords) != doc['n'] or len(set(coords)) != len(coords):
        raise ValueError('spanning: inconsistent sites')
    ranks = {p:i for i,p in enumerate(sorted(coords,key=formats.morton))}
    stars = []
    for ball in balls:
        qs = ball['supports']
        if not qs or any(not 2<=len(q)<=4 or len(set(map(tuple,q)))!=len(q) or
                         any(tuple(p) not in ranks for p in q) for q in qs):
            raise ValueError('spanning: invalid support arity/sites')
        qmin = min(map(len,qs))
        if ball['qmin'] != qmin:
            raise ValueError('spanning: inconsistent qmin')
        stars.append(min((q for q in qs if len(q)==qmin),
                         key=lambda q: tuple(sorted(ranks[tuple(p)] for p in q))))
    def ball_key(b):
        s = tuple(sorted(ranks[tuple(p)] for p in stars[b]))
        return (Fraction(balls[b]['level']), s+(doc['n'],)*(4-len(s)))
    kept, groups, births = set(), {}, {}
    for b,ball in enumerate(balls):
        v = ball['node']
        if not isinstance(v,int) or not 0<=v<len(nodes):
            raise ValueError('spanning: invalid node')
        role = ball['role']
        if role == 'naissance':
            if doc['k']==1 or nodes[v]['kind']!=1 or v in births or ball['prior'] or ball['components']!=0 or \
                    Fraction(ball['level'])!=Fraction(nodes[v]['level']):
                raise ValueError('spanning: inconsistent birth')
            births[v]=b
            kept.add(b)
        elif role == 'fusion':
            prior = ball['prior']
            if nodes[v]['kind']!=2 or Fraction(ball['level'])!=Fraction(nodes[v]['level']) or \
                    not prior or prior!=sorted(set(prior)) or len(prior)!=ball['components'] or \
                    not set(prior)<=set(nodes[v]['children']):
                raise ValueError('spanning: inconsistent fusion/prior')
            groups.setdefault(v,[]).append(b)
        elif role != 'interne':
            raise ValueError('spanning: invalid role')
    if {v for v,node in enumerate(nodes) if node['kind']==1} != set(births):
        raise ValueError('spanning: missing birth')
    for v,node in enumerate(nodes):
        if node['kind']!=2:
            continue
        children=node['children']
        if len(children)<2 or children!=sorted(set(children)):
            raise ValueError('spanning: invalid multifusion children')
        parent={c:c for c in children}
        def find(c):
            while parent[c]!=c:
                parent[c]=parent[parent[c]]
                c=parent[c]
            return c
        for b in sorted(groups.get(v,[]),key=ball_key):
            prior=balls[b]['prior']
            changed=False
            for c in prior[1:]:
                a,z=find(prior[0]),find(c)
                if a!=z:
                    parent[max(a,z)]=min(a,z)
                    changed=True
            if changed:
                kept.add(b)
        if len({find(c) for c in children})!=1:
            raise ValueError('spanning: incomplete multifusion')
    ordered=sorted(kept)  # output keeps the oracle's postorder/level/center convention
    new={old:j for j,old in enumerate(ordered)}
    out={key:doc.get(key) for key in ('k','n','sites','ids')}
    out['nodes']=[]
    for node in nodes:
        row={key:node.get(key) for key in SPANNING_NODE}
        row['balls']=[new[b] for b in node['balls'] if b in new]
        out['nodes'].append(row)
    out['balls']=[]
    for b in ordered:
        row={key:balls[b].get(key) for key in SPANNING_BALL}
        row['supports']=[stars[b]]
        out['balls'].append(row)
    return out


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
        coords = [sorted(list(pts[s]) for s in q) for q in supports]
        first = sp.prior_at[b]
        components = sp.prior_count[b] if sp.role[b] == formats.ROLE_MERGE else int(sp.role[b] == formats.ROLE_INTERNAL)
        balls.append(dict(
            node=sp.node_of[b], level=str(levels[b]), center=[str(c) for c in centers[b]],
            role=ROLE_NAMES[sp.role[b]], p=sp.p[b], m=sp.m[b], qmin=sp.qmin[b], components=components,
            prior=list(sp.prior[first:first + sp.prior_count[b]]), supports=coords))
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
    gate.check_eq(sp.version, 2, '%s : MHGP11SP version 2 (arbre couvrant)' % where)
    try:
        expected = spanning(want)
    except (ValueError, KeyError, IndexError) as error:
        gate.check(False, '%s : projection Kruskal de l\'oracle : %s' % (where, error))
        return
    found = attach_fraction.first_difference(mine, expected, 'doc')
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
    """Coquilles larges, admises par l'arbre couvrant (plus d'enumeration de Q_b, plus de plafond de 24 sites) : 25 des
    30 sites de la sphere de rayon carre 9, K1 et K2, un et quatre fils, puis sphere5 (24 sites, 828 supports en
    version 1) : chaque boule publie son seul S*. Rend le nombre de cas admis."""
    points = sphere9_25()
    if not gate.check(len(points) == 25, 'sphere9 : %d sites' % len(points)):
        return 0
    ids = [cs.NONE - 3 * i for i in range(len(points))]
    admitted = 0
    for k in (1, 2):
        for workers in (1, 4):
            report, _line = runner.publish('sphere9 K%d W%d' % (k, workers), points, ids, k, workers, 'supports')
            if gate.check(report is not None, 'sphere9 K%d W%d : supports refuse' % (k, workers)):
                counts = report['manifest']['counts']
                # La boule centrale de 25 sites n'est pas une arete de l'arbre couvrant a K1 et K2 (des boules plus
                # petites relient deja ses sites) : l'appel est admis, chaque boule publiee porte son seul S*.
                gate.check(counts['supports'] == counts['balls'] and counts['roles']['merge'] >= 1,
                           'sphere9 K%d W%d : un S* par boule, au moins une fusion' % (k, workers))
                admitted += 1
    # Temoin de l'audit be8085ec1 : trois sites equidistants, trois boules de fusion au meme plateau a K1 ; Kruskal en
    # garde deux (le filtre de role en gardait trois, un cycle).
    for workers in (1, 4):
        report, _line = runner.publish('triangle K1 W%d' % workers, [(0, 0, 0), (1, 1, 0), (1, 0, 1)], [7, 3, 11], 1,
                                       workers, 'supports')
        if gate.check(report is not None, 'triangle K1 W%d : supports refuse' % workers):
            gate.check_eq(report['manifest']['counts']['balls'], 2, 'triangle K1 W%d : deux supports' % workers)
    report, _line = runner.publish('sphere5 K1', sphere(5, 2), list(range(24)), 1, 4, 'supports')
    if gate.check(report is not None, 'sphere5 K1 : supports refuse'):
        counts = report['manifest']['counts']
        gate.check_eq(counts['supports'], counts['balls'], 'sphere5 K1 : un S* par boule')
    return admitted


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
        admitted = shell_cases(gate, runner)
    print('cli_supports_oracle_couverture bits=%d nuages=%d exclus=%d ordres=%d boules=%d etendues=%d multiples=%d '
          'tetraedres=%d ordres_6plus=%d signatures=%d larges=%d'
          % (o['bits'], t['clouds'], excluded, t['orders'], t['balls'], t['extended'], t['multiple'], t['tetra'],
             t['high'], t['signatures'], admitted))
    below = [name for name in FLOORS if t[name] < o['floors'].get(name, 0)]
    if gate.failures == 0 and (below or admitted != 4):
        print('PLANCHER cli_supports_oracle : %s sous %s, coquilles larges admises %d sur 4'
              % (', '.join(below), json.dumps(o['floors'], sort_keys=True), admitted))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
