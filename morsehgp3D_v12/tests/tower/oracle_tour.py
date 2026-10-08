#!/usr/bin/env python3
"""Porte de l'oracle borne des etages T, M, V et R et de l'export FUL1 (contrat de la tour, paragraphe 9.2).

Sur la suite rapide de reference/ (nuages sans doublon : la v12 refuse les doublons, decision D8) et sur des temoins
graves, pour chacune des trois politiques de saut de la regle de la v12 de la reference (k plus petits identifiants de
I, regle de la v11 ; k plus proches du centre ; candidats voisins), l'adaptateur de test ecrit les naissances, les
cellules de fenetre non naissances (cellules inertes comprises, pont de LEM-T4) et les cibles de leurs representants
rendues par Reference.resolve_v12 (cibles << naissance >> et << cellule >>, arret a la premiere cellule de fenetre),
puis mhgp12_tower_forest_oracle joue le noyau, la contraction, les verticales et le registre. Juge, champ par champ, contre
Reference.order(k) (egal a l'etage B, donc a la definition, par la porte mhgp12_reference_resolution_v12) : noeuds
(rang du niveau, parent, enfants), racine, verticales, et coupes ouvertes et fermees a chaque niveau d'evenement (noeuds
vivants) ; puis le vidage FUL1 de chaque cas, relu par le lecteur strict (full_reader.py, profil du binaire) et
compare aux niveaux et aux centres exacts de la reference. Compteurs exacts (graves) et planchers contre le vert par
vacuite : cibles << cellule >>, cellules inertes, remontees de LEM-T6, fusions d'au moins trois enfants.

    python3 oracle_tour.py <mhgp12_tower_forest_oracle> <bits> [--fils N] [--tranche E]

Codes : 0 conforme ; 1 ecart a la reference ; 2 usage ; 3 plancher, compteur grave ou refus de l'outil.
Python 3.10 nu, aucun assert : meme code sous python3 -O.
"""
from fractions import Fraction
import os
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
sys.path.insert(0, HERE)
import hgp12_ref  # noqa: E402
from hgp12_ref import families  # noqa: E402
import full_reader  # noqa: E402

OK, DISAGREEMENT, USAGE, FLOOR = 0, 1, 2, 3
POLICIES = ('v12_indices', 'v12_proches', 'v12_voisins')
# Temoins graves en plus de la suite rapide (nom, points, K).
WITNESSES = (
    ('wit_tri_eq', [(0, 0, 0), (1, 1, 0), (1, 0, 1)], 1),            # WIT-TRI-EQ : cycle de plateau a l'ordre 1
    ('wit_six_k2', [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0),
                    (5732, 1000, 0)], 2),                            # WIT-SIX (these, paragraphe 6.1) a K = 2
    ('carre_k4', [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)], 4),   # CST-0214 : carre K1..4
)
# Faits graves des temoins : (nom, ordre) -> (naissances, fusions, arites des fusions, verticales des noeuds).
FACTS = {
    ('wit_tri_eq', 1): (3, 1, [3], None),
    ('carre_k4', 1): (4, 1, [4], None),
    ('carre_k4', 2): (4, 1, [4], [4, 4, 4, 4, 4]),
    ('carre_k4', 3): (1, 0, [], [4]),
    ('carre_k4', 4): (1, 0, [], [0]),
    # WIT-SIX : deux fusions ternaires simultanees (niveau 999956 a l'ordre 1), puis la binaire du pont ; a l'ordre 2,
    # deux ternaires simultanees puis une ternaire.
    ('wit_six_k2', 1): (6, 3, [2, 3, 3], None),
    ('wit_six_k2', 2): (7, 3, [3, 3, 3], [6, 6, 7, 7, 8, 8, 8, 8, 8, 8]),
}
# Paires de fusions de meme niveau attendues (fusions simultanees) : (nom, ordre) -> (premiere, seconde) fusion.
SIMULTANEOUS = {('wit_six_k2', 1): (6, 7), ('wit_six_k2', 2): (7, 8)}


def clouds():
    out = [(c.name, c.points, c.kmax) for c in families.fast_suite() if len(set(c.points)) == len(c.points)]
    return out + [(name, pts, k) for name, pts, k in WITNESSES]


def case_text(name, points, kmax, policy):
    """Texte d'un cas pour l'outil natif, et l'attendu de la reference."""
    ref = hgp12_ref.Reference(points, kmax, resolution=policy)
    levels = sorted(set(b.level for b in ref.balls))
    rank = dict((level, i) for i, level in enumerate(levels))
    lines = ['nuage %s %d %d %d' % (name, len(points), ref.orders, len(ref.balls))]
    lines += ['p %d %d %d' % tuple(p) for p in points]
    lines += ['b %d %s' % (rank[b.level], ' '.join(str(s) for s in b.support)) for b in ref.balls]
    expected = []
    for k in range(1, ref.orders + 1):
        births, cells = [], []
        for ball in ref.balls:
            if ball.lo <= k <= ball.hi:
                kind, reps = ref.cell(ball, k)
                (births if kind == 'birth' else cells).append((ball, reps))
        bpos = dict((b.index, i) for i, (b, _reps) in enumerate(births))
        cpos = dict((b.index, i) for i, (b, _reps) in enumerate(cells))
        lines.append('ordre %d %d %d' % (k, len(births), len(cells)))
        for b, _reps in births:
            lines.append('n %d %d' % (b.support[0] if k == 1 else b.index, rank[b.level]))
        for ball, reps in cells:
            targets = []
            for rep in reps:
                kind, index = ref.resolve_v12(tuple(sorted(ball.inner + rep)), k)
                targets.append(bpos[index] if kind == 'naissance' else cpos[index] + (1 << 31))
            lines.append('c %d %d %s' % (ball.index, rank[ball.level], ' '.join(str(t) for t in targets)))
        expected.append(ref.order(k))
    lines.append('fin')
    return '\n'.join(lines) + '\n', expected, levels


def parse_output(text):
    """Blocs de l'outil : liste de (nom, statut, ordres), ordre = (k, naissances, racine, noeuds, compteurs)."""
    cases, current, order = [], None, None
    for line in text.splitlines():
        words = line.split()
        if not words or words[0] == 'fin':
            continue
        if words[0] == 'cas':
            current = dict(name=words[1], status=None, orders=[])
            cases.append(current)
        elif words[0] == 'ordre':
            order = dict(k=int(words[1]), births=int(words[2]), root=int(words[4]), nodes=[], counters={})
            current['orders'].append(order)
        elif words[0] == 'v':
            values = [int(w) for w in words[1:]]
            order['nodes'].append((values[0], values[1], values[2], tuple(values[3:])))
        elif words[0] == 'compteurs':
            order['counters'] = dict((w.split('=')[0], int(w.split('=')[1])) for w in words[2:])
        elif words[0] == 'statut':
            current['status'] = ' '.join(words[1:])
    return cases


def cut_nodes(nodes, levels, level, closed):
    """Noeuds vivants a la coupe fermee (niveau <= level < parent) ou ouverte (niveau < level <= parent)."""
    alive = []
    for v, (rank, parent, _lower, _kids) in enumerate(nodes):
        lv = levels[rank]
        up = None if parent < 0 else levels[nodes[parent][0]]
        if (lv <= level and (up is None or up > level)) if closed else (lv < level and (up is None or up >= level)):
            alive.append(v)
    return alive


def compare_order(got, want, levels):
    """Ecarts d'un ordre : noeuds, racine, verticales, coupes."""
    gaps = []
    nodes = got['nodes']
    if len(nodes) != len(want.nodes) or got['births'] != sum(1 for n in want.nodes if not n.children):
        return ['forme : %d noeuds au lieu de %d' % (len(nodes), len(want.nodes))]
    parents = [-1] * len(want.nodes)
    for v, node in enumerate(want.nodes):
        for c in node.children:
            parents[c] = v
    for v, node in enumerate(want.nodes):
        rank, parent, lower, kids = nodes[v]
        if levels[rank] != node.level or kids != tuple(node.children) or parent != parents[v]:
            gaps.append('noeud %d' % v)
        if want.lower is not None and lower != want.lower[v]:
            gaps.append('verticale du noeud %d' % v)
    if parents.index(-1) != got['root']:
        gaps.append('racine')
    for cut in want.cuts:
        for closed, side in ((True, cut.closed), (False, cut.opened)):
            if cut_nodes(nodes, levels, cut.level, closed) != sorted(entry[0] for entry in side):
                gaps.append('coupe %s au niveau %s' % ('fermee' if closed else 'ouverte', cut.level))
    return gaps


def read_dump(path):
    """Contenu d'un vidage FUL1 de petit nuage : par ordre, liste de (parent, niveau, centre, verticale)."""
    with open(path, 'rb') as handle:
        data = handle.read()
    words = [struct.unpack_from('<Q', data, at)[0] for at in range(10, len(data), 8)]
    at = [0]

    def word():
        at[0] += 1
        return words[at[0] - 1]

    def exact():
        negative, limbs = word(), word()
        value = sum(word() << (64 * i) for i in range(limbs))
        return -value if negative else value

    _bits, kmax, sites, _points = word(), word(), word(), word()
    for _ in range(sites):
        for _ in range(5):
            word()
    orders = []
    for k in range(1, kmax + 1):
        _k, births, count, edges, _root = (word() for _ in range(5))
        nodes = []
        for v in range(count):
            parent, _begin, _cardinal = word(), word(), word()
            level = Fraction(exact(), exact())
            center = None
            if v < births:
                numerators = [exact() for _ in range(3)]
                den = exact()
                center = tuple(Fraction(n, den) for n in numerators)
            lower = word() if k > 1 else None
            nodes.append((parent, level, center, lower))
        for _ in range(edges):
            word()
        orders.append(nodes)
    return orders


def compare_dump(path, want, bits, kmax, sites):
    """Lecteur strict, puis niveaux, centres et verticales du vidage contre la reference."""
    try:
        full_reader.inspect(path, bits, kmax, sites)
    except ValueError as error:
        return ['vidage refuse par le lecteur strict : %s' % error]
    gaps = []
    for k, (nodes, order) in enumerate(zip(read_dump(path), want), 1):
        for v, (parent, level, center, lower) in enumerate(nodes):
            node = order.nodes[v]
            if level != node.level or center != node.center or (order.lower is not None and lower != order.lower[v]):
                gaps.append('vidage, ordre %d, noeud %d' % (k, v))
    return gaps


def check_facts(name, orders):
    gaps, checked = [], 0
    for (fact_name, k), (first, second) in SIMULTANEOUS.items():
        if fact_name == name:
            checked += 1
            nodes = orders[k - 1]['nodes']
            if nodes[first][0] != nodes[second][0] or len(nodes[first][3]) != 3 or len(nodes[second][3]) != 3:
                gaps.append('fusions simultanees %s ordre %d' % (name, k))
    for (fact_name, k), (births, merges, arities, lowers) in FACTS.items():
        if fact_name != name:
            continue
        checked += 1
        order = orders[k - 1]
        got_arities = sorted(len(kids) for _r, _p, _l, kids in order['nodes'] if kids)
        got_lowers = [lower for _r, _p, lower, _kids in order['nodes']] if lowers is not None else None
        if (order['births'], len(order['nodes']) - order['births'], got_arities, got_lowers) != \
                (births, merges, sorted(arities), lowers):
            gaps.append('fait grave %s ordre %d' % (name, k))
    return gaps, checked


# Compteurs EXACTS de la porte sur la suite rapide sans doublon et les temoins, toutes politiques (graves au premier
# passage, 7 octobre 2026) ; ils ne changent que si la suite, la reference ou la regle change.
EXACT = dict(cas=933, cibles_cellule=3487, faits=27, inertes=2631, multifusions=4629, remontees=105, t6_naissance=63)
FLOORS = dict(cibles_cellule=1, inertes=1, remontees=1, multifusions=1, t6_naissance=1, cas=900, faits=27)


def run_tool(tool, text, dumps, threads, slice_events):
    command = [tool, '--fils', str(threads), '--tranche', str(slice_events), '--vidages', dumps]
    done = subprocess.run(command, input=text, capture_output=True, text=True, check=False)
    return done.returncode, done.stdout, done.stderr


def judge(cases, results, dumps, bits):
    gaps, totals = [], dict(cibles_cellule=0, inertes=0, remontees=0, t6_naissance=0, multifusions=0, cas=0, faits=0)
    for number, ((name, policy, expected, levels, sites), got) in enumerate(zip(cases, results)):
        where = '%s (%s)' % (name, policy)
        if got['name'] != name or got['status'] != 'ok':
            gaps.append('%s : statut %s' % (where, got['status']))
            continue
        totals['cas'] += 1
        for order, want in zip(got['orders'], expected):
            gaps += ['%s, ordre %d : %s' % (where, order['k'], gap) for gap in compare_order(order, want, levels)]
            for key in ('cibles_cellule', 'inertes', 'remontees', 't6_naissance'):
                totals[key] += order['counters'].get(key, 0)
            totals['multifusions'] += sum(1 for _r, _p, _l, kids in order['nodes'] if len(kids) >= 3)
        path = os.path.join(dumps, str(number), 'tour.ful1')
        gaps += ['%s : %s' % (where, gap) for gap in compare_dump(path, expected, bits, len(expected), sites)]
        facts, checked = check_facts(name, got['orders'])
        gaps += ['%s : %s' % (where, gap) for gap in facts]
        totals['faits'] += checked
    return gaps, totals


def main(argv):
    args = argv[1:]
    if len(args) < 2 or not args[1].isdigit():
        print('usage : oracle_tour.py <mhgp12_tower_forest_oracle> <bits> [--fils N] [--tranche E]', file=sys.stderr)
        return USAGE
    tool, bits, threads, slice_events = args[0], int(args[1]), 2, 1 << 16
    rest = args[2:]
    while rest:
        if len(rest) < 2 or rest[0] not in ('--fils', '--tranche') or not rest[1].isdigit():
            print('option inconnue : %r' % rest, file=sys.stderr)
            return USAGE
        threads, slice_events = (int(rest[1]), slice_events) if rest[0] == '--fils' else (threads, int(rest[1]))
        rest = rest[2:]
    cases, texts = [], []
    for name, points, kmax in clouds():
        for policy in POLICIES:
            text, expected, levels = case_text(name, points, kmax, policy)
            texts.append(text)
            cases.append((name, policy, expected, levels, len(points)))
    dumps = tempfile.mkdtemp(prefix='mhgp12_tower_forest_oracle_')
    try:
        code, out, err = run_tool(tool, ''.join(texts), dumps, threads, slice_events)
        if code != 0:
            print('outil : code %d %s' % (code, err.strip()), file=sys.stderr)
            return FLOOR
        results = parse_output(out)
        if len(results) != len(cases):
            print('outil : %d cas rendus pour %d' % (len(results), len(cases)), file=sys.stderr)
            return FLOOR
        gaps, totals = judge(cases, results, dumps, bits)
    finally:
        shutil.rmtree(dumps, ignore_errors=True)
    if gaps:
        for gap in gaps[:20]:
            print(gap, file=sys.stderr)
        print('%d ecarts a la reference' % len(gaps), file=sys.stderr)
        return DISAGREEMENT
    print('compteurs ' + ' '.join('%s=%d' % (key, totals[key]) for key in sorted(totals)))
    problems = ['%s = %d sous le plancher %d' % (key, totals[key], floor)
                for key, floor in FLOORS.items() if totals[key] < floor]
    if EXACT is not None and totals != EXACT:
        problems.append('compteurs %r, graves %r' % (totals, EXACT))
    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
        return FLOOR
    print('tour_oracle_ok cas=%d politiques=%d cibles_cellule=%d inertes=%d remontees=%d'
          % (totals['cas'], len(POLICIES), totals['cibles_cellule'], totals['inertes'], totals['remontees']))
    return OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
