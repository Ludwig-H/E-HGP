"""Foret FULL bornee : attendu Gamma_k de Definition, sans voie constructive.

Le chargement prive du paquet importe model.py et definition.py uniquement : ni
__init__, ni constructive, ni judge. Les coupes sont relues depuis les noeuds du
produit, puis comparees aux balayages exhaustifs de toutes les k/(k+1)-parties.
"""
import copy
import importlib.util
import itertools as it
import json
import math
from functools import lru_cache
from pathlib import Path
import subprocess
import sys
import types

import descent_oracle as data


REFERENCE = Path(__file__).resolve().parents[2]/'reference'/'hgp11_ref'
PACKAGE = 'forest_definition_reference'
package = types.ModuleType(PACKAGE)
package.__path__ = [str(REFERENCE)]
sys.modules[PACKAGE] = package
for leaf in ('model', 'definition'):
    spec = importlib.util.spec_from_file_location(PACKAGE+'.'+leaf, REFERENCE/(leaf+'.py'))
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
definition = sys.modules[PACKAGE+'.definition']
require, equal, integer, rational = data.require, data.equal, data.integer, data.rational
WORK = ('classified_cells', 'replayed_cells', 'plateaus', 'trace_resolutions', 'unions',
        'touched_components', 'continuations', 'center_comparisons', 'ancestor_hops',
        'vertical_descents', 'vertical_checks', 'birth_presentations', 'ancestor_queries',
        'ancestor_activations', 'ancestor_unions', 'ancestor_find_steps')


@lru_cache(maxsize=128)
def reference(points):
    return definition.Definition(points)


@lru_cache(maxsize=128)
def truth(records, kmax):
    points, identifiers = data.model.prepared(records)
    expected = reference(points)
    return points, identifiers, tuple(expected.order(k) for k in range(1,kmax+1))


@lru_cache(maxsize=256)
def geometric_work(sites, k):
    """Objets traites, pas nombre de supports essayes par une strategie MEB."""
    truth_order = reference(sites).order(k)
    births = sum(not n.children for n in truth_order.nodes)
    work = dict(classified_cells=0, replayed_cells=0, plateaus=0, trace_resolutions=0,
                unions=births-1, birth_presentations=births, vertical_descents=births if k > 1 else 0,
                vertical_checks=sum(len(n.children) for n in truth_order.nodes) if k > 1 else 0)
    work['cells'] = dict(combinations=0,passes=0,trace_tests=0,meb_calls=0)
    work['classification'] = dict(combinations=0,examined=0,meb_calls=0)
    work.update(ancestor_hops=0,ancestor_queries=work['vertical_descents']+work['vertical_checks'],
                ancestor_activations=0,ancestor_unions=0)
    if k > 1:
        last = max(n.level for n in truth_order.nodes)
        active = [n for n in reference(sites).order(k-1).nodes if n.children and n.level <= last]
        work['ancestor_activations'] = len(active)
        work['ancestor_unions'] = sum(len(n.children) for n in active)
    levels = set()
    parent = [None]*len(truth_order.nodes)
    for i,node in enumerate(truth_order.nodes):
        for child in node.children:
            parent[child] = i
    reached = {}
    for ball in data.model.all_balls(sites):
        if not ball.p+ball.qmin-1 <= k <= ball.p+len(ball.shell):
            continue
        work['classified_cells'] += 1
        strict_parts = [a for a in it.combinations(ball.shell,k-ball.p) if reference(sites).beta(a) < ball.level]
        strict = len(strict_parts)
        for a in strict_parts:
            part = tuple(sorted(ball.inner+a))
            before = reference(sites).node_at(k,part,reference(sites).beta(part))
            while parent[before] is not None and truth_order.nodes[parent[before]].level < ball.level:
                before = parent[before]
            after = reference(sites).node_at(k,part,ball.level)
            reached.setdefault((ball.level,after),set()).add(before)
        m, t = len(ball.shell), k-ball.p
        combinations = math.comb(m,t)
        work['classification']['combinations'] += combinations
        if t != m and t >= ball.qmin:
            examined = 0
            for a in it.combinations(ball.shell,t):
                examined += 1
                if reference(sites).beta(a) < ball.level:
                    break
            work['classification']['examined'] += examined
            work['classification']['meb_calls'] += examined
        repetitions = int(strict > 0)  # Seul le rejeu materialise exhaustivement les traces en deux passes.
        work['cells']['combinations'] += repetitions*math.comb(m,t)
        if t != m and m != ball.qmin:
            work['cells']['passes'] += 2*repetitions
            work['cells']['trace_tests'] += 2*repetitions*math.comb(m,t)
            if t >= ball.qmin:
                work['cells']['meb_calls'] += 2*repetitions*math.comb(m,t)
        if strict:
            work['replayed_cells'] += 1
            work['trace_resolutions'] += strict
            levels.add(ball.level)
    require(work['classified_cells']-work['replayed_cells'] == (births if k > 1 else 0),
            'classification T3 contre naissances Gamma')
    work['plateaus'] = len(levels)
    work['continuations'] = sum(len(group) == 1 for group in reached.values())
    work['touched_components'] = sum(len(group) for group in reached.values())
    return work


def retained_minimum(orders):
    # Tailles minimales des champs : ForestNode24, NodeIdx4, BirthEntry8 ; pas de supposition sur padding.
    return sum(24*o['node_capacity']+4*o['edge_capacity']+8*o['births']+
               (4*o['node_capacity'] if o['order'] > 1 else 0) for o in orders)


def parents(nodes):
    out = [None]*len(nodes)
    for parent, node in enumerate(nodes):
        for child in node['children']:
            integer(child, len(nodes)-1)
            require(out[child] is None and child != parent, 'parent unique')
            require(rational(nodes[child]['level']) < rational(node['level']), 'plateau atomique')
            out[child] = parent
    return out


def cut(nodes, parent, level, closed):
    before = (lambda x: x <= level) if closed else (lambda x: x < level)
    return [i for i, node in enumerate(nodes) if before(rational(node['level'])) and
            (parent[i] is None or not before(rational(nodes[parent[i]]['level'])))]


def center_of(seed, sites, balls):
    require(type(seed) is dict and seed.keys() == {'site', 'ball'}, 'seed champs')
    if seed['site'] is not None:
        require(seed['ball'] is None, 'double seed')
        return tuple(data.F(v) for v in sites[integer(seed['site'], len(sites)-1)]), data.F(0)
    ball = balls[integer(seed['ball'], len(balls)-1)]
    sphere = definition.circumsphere([sites[i] for i in ball['support']])
    require(sphere is not None and sphere[1] == rational(ball['level']), 'boule de naissance')
    return sphere


def meb_work(work):
    require(type(work) is dict and work.keys() == set(data.MEB), 'ledger MEB champs')
    for value in work.values():
        integer(value)
    require(work['containing'] <= work['positive'] <= work['nondegenerate'] <= work['presentations'],
            'ledger MEB sous comptes')


def judge_work(work):
    require(type(work) is dict and work.keys() == set(WORK+('cells','classification','descent')), 'ledger foret champs')
    for key in WORK:
        integer(work[key])
    cells = work['cells']
    require(type(cells) is dict and cells.keys() == {'combinations','passes','trace_tests','meb_calls','meb'},
            'ledger cellules champs')
    for key in ('combinations','passes','trace_tests','meb_calls'):
        integer(cells[key])
    meb_work(cells['meb'])
    require(cells['meb_calls'] <= cells['trace_tests'] and cells['meb']['containing'] >= cells['meb_calls'],
            'ledger cellules sous comptes')
    classification = work['classification']
    require(type(classification) is dict and classification.keys() == {'combinations','examined','meb_calls','meb'},
            'ledger classification champs')
    for key in ('combinations','examined','meb_calls'):
        integer(classification[key])
    meb_work(classification['meb'])
    require(classification['examined'] <= classification['combinations'] and
            classification['meb_calls'] == classification['examined'] and
            classification['meb']['containing'] >= classification['meb_calls'],'classification sous comptes')
    descent = work['descent']; data.ledger(descent)
    require(descent['census_calls']+descent['catalogue_hits'] == descent['steps'], 'ledger descent populations')
    require(descent['interior_steps']+descent['trace_steps'] <= descent['steps'], 'ledger descent transitions')
    require(descent['part_meb']['containing'] >= descent['steps'], 'ledger MEB des parties')
    require(descent['trace_meb_calls'] <= descent['candidate_traces'] and
            descent['trace_meb']['containing'] >= descent['trace_meb_calls'], 'ledger MEB des traces')
    require(descent['steps'] >= work['trace_resolutions']+work['vertical_descents'], 'descentes agregees')
    return 1+len(WORK)+len(cells)+len(classification)+len(data.ledger(descent))


def judge_orders(orders, expected, sites, balls):
    """Les coupes jugees sont des lectures de structure, pas une nouvelle API native de coupe."""
    require(type(orders) is list and len(orders) == len(expected), 'inventaire ordres')
    counts = dict(orders=0, nodes=0, births=0, merges=0, nary=0, cuts=0, verticals=0, checks=0)
    for order, truth_order in zip(orders, expected):
        require(type(order) is dict and order.keys() == {'order','nodes','lower','root','ledger','births',
                                                       'node_capacity','edge_capacity'}, 'ordre champs')
        counts['checks'] += equal(order['order'], truth_order.k)
        counts['checks'] += judge_work(order['ledger'])
        nodes = order['nodes']
        require(type(nodes) is list and len(nodes) == len(truth_order.nodes), 'inventaire noeuds')
        births = sum(not n.children for n in truth_order.nodes)
        counts['checks'] += equal(order['births'],births)+equal(order['node_capacity'],2*births-1)
        counts['checks'] += equal(order['edge_capacity'],2*births-2)
        counts['checks'] += equal(order['ledger']['birth_presentations'],births)
        child_count = sum(len(n.children) for n in truth_order.nodes) if truth_order.k > 1 else 0
        counts['checks'] += equal(order['ledger']['vertical_checks'],child_count)
        for key, value in geometric_work(tuple(sites),truth_order.k).items():
            if key in ('cells','classification'):
                counts['checks'] += sum(equal(order['ledger'][key][field],v) for field,v in value.items())
            else:
                counts['checks'] += equal(order['ledger'][key],value)
        counts['checks'] += equal(order['ledger']['touched_components'],
                                 sum(len(n.children) for n in truth_order.nodes)+order['ledger']['continuations'])
        if truth_order.k > 1:
            lower_n = len(expected[truth_order.k-2].nodes)
            l = order['ledger']
            bound = 2*(lower_n.bit_length()-1)*(l['ancestor_queries']+2*l['ancestor_unions'])
            require(l['ancestor_find_steps'] <= bound,'union par taille borne la profondeur de find')
        else:
            equal(order['ledger']['ancestor_find_steps'],0)
        parent = parents(nodes)
        wanted_parent = [None]*len(nodes)
        for v, node in enumerate(truth_order.nodes):
            for child in node.children:
                wanted_parent[child] = v
        roots = [v for v, p in enumerate(wanted_parent) if p is None]
        require(len(roots) == 1, 'reference : racine unique')
        counts['checks'] += equal(order['root'], roots[0])+equal(parent, wanted_parent)
        for i, (node, wanted) in enumerate(zip(nodes, truth_order.nodes)):
            require(type(node) is dict and node.keys() == {'level','seed','parent','children'}, 'noeud champs')
            require(rational(node['level']) == wanted.level, 'niveau du noeud')
            counts['checks'] += equal(node['children'], list(wanted.children))+equal(node['parent'], parent[i])
            if wanted.center is None:
                counts['checks'] += equal(node['seed'], None)
                counts['merges'] += 1
                counts['nary'] += len(wanted.children) > 2
            else:
                center, level = center_of(node['seed'], sites, balls)
                require(center == wanted.center and level == wanted.level, 'centre canonique exact de naissance')
                counts['births'] += 1
            counts['nodes'] += 1
        lower = None if truth_order.lower is None else list(truth_order.lower)
        counts['checks'] += equal(order['lower'], lower)
        counts['verticals'] += len(lower or ())
        for snapshot in truth_order.cuts:
            for is_closed, components in ((False, snapshot.opened), (True, snapshot.closed)):
                counts['checks'] += equal(cut(nodes,parent,snapshot.level,is_closed), sorted(c[0] for c in components))
                counts['cuts'] += 1
        counts['orders'] += 1
    return counts


def judge(row, req, bits):
    require(type(row) is dict and row.keys() == {'status','reason','coord_bits','kmax','sites','site_ids',
                                                'balls','orders','forest_memory','owner_after'}, 'reponse champs')
    for key, value in (('coord_bits', bits), ('kmax', req['kmax']), ('owner_after', 0)):
        equal(row[key], value)
    memory = row['forest_memory']
    require(type(memory) is dict and memory.keys() == {'after','peak'}, 'memoire champs')
    equal(memory['after'], 0); integer(memory['peak'], req['budget'])
    reason = refusal(req, bits)
    if reason:
        status = ('resource_exhausted' if reason == 'memory_budget' else 'unsupported_degeneracy'
                  if reason == 'multiplicity_unsupported' else 'invalid_input')
        checks = sum(equal(row[key], value) for key, value in (
            ('status',status), ('reason',reason), ('orders',[]), ('sites',[]), ('site_ids',[]), ('balls',None)))
        return dict(orders=0, nodes=0, births=0, merges=0, nary=0, cuts=0, verticals=0, checks=checks+5)
    equal(row['status'], 'ok'); equal(row['reason'], 'none')
    require(memory['peak'] > 0, 'foret possedee non vacante')
    sites, identifiers, expected = truth(tuple(req['records']), req['kmax'])
    equal(row['sites'], [list(p) for p in sites]); equal(row['site_ids'], [list(ids) for ids in identifiers])
    _, _, _, wanted_balls = data.geometry(tuple(req['records']), req['kmax'])
    balls = copy.deepcopy(row['balls'])
    require(type(balls) is list, 'catalogue tableau')
    for ball in balls:
        ball['level'] = data.encoded(rational(ball['level']))
    checks = equal(balls, wanted_balls)
    counts = judge_orders(row['orders'], expected, sites, balls)
    require(memory['peak'] >= retained_minimum(row['orders']), 'pic inferieur aux buffers finaux vivants')
    counts['checks'] += checks+10
    return counts


def requests(bits):
    rows = []
    def add(name, points, kmax):
        rows.append(dict(name=name, records=data.fixtures.records(tuple(points)), kmax=kmax, budget=1 << 27))
    add('singleton', [(0,0,0)], 1)
    add('line024', [(0,0,0),(2,0,0),(4,0,0)], 3)
    add('line01269', [(x,0,0) for x in (0,1,2,6,9)], 5)
    add('square_center', [(0,0,0),(4,0,0),(0,4,0),(4,4,0),(2,2,0)], 5)
    add('square_plain', [(0,0,0),(4,0,0),(0,4,0),(4,4,0)], 4)
    add('global_q3_nonfirst_shell', [(5,5,0),(2,1,5),(10,5,5),(2,9,5),(5,9,8)], 5)
    add('two_components_same_plateau', [(0,0,0),(2,0,0),(4,0,0),(20,0,0),(22,0,0),(24,0,0)], 5)
    add('line12', [(i,0,0) for i in range(12)], 12)
    add('line13_K12', [(i,0,0) for i in range(13)], 12)
    for fixture in data.fixtures.fixtures(bits):
        if fixture.name in ('pair', 'right_triangle', 'acute_triangle', 'regular_tetra', 'obtuse_prefix',
                             'extended_q4', 'octa_center', 'cube', 'extended_q3', 'maximum_tetra',
                             'close_levels', 'random0', 'random1'):
            add(fixture.name, fixture.points, min(5,len(fixture.points)))
    add('wide_shell14', [(0,5,5),(1,2,5),(1,8,5),(2,1,5),(2,9,5),(5,0,5),(5,10,5),
                          (8,1,5),(8,9,5),(9,2,5),(9,8,5),(10,5,5),(5,5,0),(5,5,10)], 3)
    for name in ('line024', 'square_center', 'maximum_tetra'):
        original = next(r for r in rows if r['name'] == name)
        reverse = copy.deepcopy(original); reverse['name'] += '_reverse'
        reverse['records'] = reverse['records'][::-1]; rows.append(reverse)
    base = next(r for r in rows if r['name'] == 'pair')
    for name, changes in (
        ('empty', dict(records=())), ('coordinate', dict(records=((1 << bits,0,0,7),))),
        ('duplicate_id', dict(records=((0,0,0,7),(2,0,0,7)))),
        ('weight', dict(records=((0,0,0,7),(0,0,0,8)))),
        ('kmax_zero', dict(kmax=0)), ('kmax_large', dict(kmax=13)), ('kmax_above_n', dict(kmax=3)),
        ('no_forest_memory', dict(budget=0))):
        req = copy.deepcopy(base); req.update(changes); req['name'] = name; rows.append(req)
    return rows


def refusal(req, bits):
    records = req['records']
    if not records:
        return 'empty_input'
    if any(any(v < 0 or v >= 1 << bits for v in r[:3]) for r in records):
        return 'coordinate_out_of_domain'
    if len({r[3] for r in records}) != len(records):
        return 'duplicate_point_id'
    if not 1 <= req['kmax'] <= 12:
        return 'kmax_out_of_range'
    if len({r[:3] for r in records}) != len(records):
        return 'multiplicity_unsupported'
    if req['kmax'] > len(records):
        return 'parameter_out_of_range'
    if req['budget'] == 0:
        return 'memory_budget'
    return None


def run(executable):
    profile = subprocess.run([executable,'--profile'], capture_output=True, text=True, timeout=10)
    require(profile.returncode == 0 and not profile.stderr, 'processus profil')
    bits = data.parse(profile.stdout)['coord_bits']; integer(bits,24); require(bits in (18,21,24), 'profil')
    reqs = requests(bits)
    encoded = ''.join('%d %d %d\n' % (r['kmax'],r['budget'],len(r['records']))+
                      ''.join(' '.join(map(str,p))+'\n' for p in r['records']) for r in reqs)
    process = subprocess.run([executable], input=encoded, capture_output=True, text=True, timeout=180)
    require(process.returncode == 0 and not process.stderr, 'processus natif')
    lines = process.stdout.splitlines(); require(len(lines) == len(reqs), 'nombre de reponses')
    rows = [data.parse(line) for line in lines]
    counts = [judge(row,req,bits) for row,req in zip(rows,reqs)]
    totals = {key: sum(c[key] for c in counts) for key in counts[0]}
    require(totals['orders'] == 120 and totals['nodes'] == 867 and totals['births'] == 673 and
            totals['merges'] == 194 and totals['nary'] == 121 and totals['cuts'] == 1882 and
            totals['verticals'] == 673 and totals['checks'] >= 26000, 'planchers foret')
    print(json.dumps(dict(verdict='conforme', bits=bits, requests=len(reqs),
                         refusals=sum(row['status'] != 'ok' for row in rows), **totals), sort_keys=True))


if __name__ == '__main__':
    try:
        require(len(sys.argv) == 2, 'usage forest_oracle.py executable')
        run(sys.argv[1])
    except (ValueError, KeyError, TypeError, IndexError, subprocess.SubprocessError) as error:
        print('REFUS '+str(error), file=sys.stderr)
        raise SystemExit(1)
