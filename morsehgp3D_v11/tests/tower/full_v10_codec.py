"""Common FULL serialization for frozen v10 and v11, never an expected v11 forest.

v10 lacks birth coordinates in its dump. Its OWN catalogue identifies birth
centers by the strict trace criterion, tested through Fraction MEB only. Neither
Definition.order nor the constructive oracle is called. Node numbering and level
spelling are normalized; point attachments are parsed but outside this comparison.
"""
from fractions import Fraction as F
import itertools as it
import json

import forest_oracle as geometry

COMMIT = 'c764e121aa52f2e5dd9b85fbe308c9c3511ff55e'
SCHEMA = 'ehgp.full_common.v1'
need = geometry.require


def fraction(value, base=10):
    need(type(value) is list and len(value) == 2 and all(type(x) is str for x in value), 'fraction encoding')
    numerator, denominator = (int(x,base) for x in value)
    need(numerator >= 0 and denominator > 0, 'fraction signs')
    return F(numerator,denominator)


def encoded(value):
    return [str(value.numerator),str(value.denominator)]


def morton(point):
    return sum(((v >> bit) & 1) << (3*bit+axis) for axis,v in enumerate(point) for bit in range(18))


def catalogue(text, sites):
    """Reconstruct spheres from the frozen supports, then verify their global census."""
    rows, ranks, seen = [], {}, set()
    positions = {tuple(p): i for i,p in enumerate(sites)}
    for line in text.splitlines():
        fields = line.split('|'); need(len(fields) == 4, 'v10 catalogue fields')
        header = [int(x) for x in fields[0].split()]; need(len(header) == 5, 'v10 catalogue header')
        rank, q, p, m, flags = header
        lists = []
        for field in fields[1:]:
            ids = [positions[tuple(int(c) for c in token.split(','))] for token in field.split()]
            need(ids == sorted(set(ids)), 'v10 catalogue site order'); lists.append(ids)
        support, inner, shell = lists
        need(q in (2,3,4) and len(support) == q and len(inner) == p and len(shell) == m and
             flags == int(m != q) and set(support) <= set(shell), 'v10 catalogue cardinal/flags')
        sphere = geometry.definition.circumsphere([sites[i] for i in support])
        need(sphere is not None and sphere[1] > 0, 'v10 catalogue support')
        center, level = sphere
        signs = [sum((F(p[j])-center[j])**2 for j in range(3))-level for p in sites]
        need(inner == [i for i,v in enumerate(signs) if v < 0] and
             shell == [i for i,v in enumerate(signs) if v == 0], 'v10 global census')
        need((center,level) not in seen, 'v10 duplicate sphere'); seen.add((center,level))
        need(rank >= 0 and (rank not in ranks or ranks[rank] == level), 'v10 rank level')
        ranks[rank] = level
        rows.append(dict(support=support,inner=inner,shell=shell,qmin=q,level=level,center=center,rank=rank))
    need(sorted(ranks) == list(range(len(ranks))) and list(ranks.values()) == sorted(set(ranks.values())),
         'v10 dense ordered ranks')
    keys = [(b['level'],tuple(b['support']+[2**32-1]*(4-b['qmin']))) for b in rows]
    need(keys == sorted(set(keys)), 'v10 catalogue canonical ordering')
    return rows


def birth_labels(balls, sites, k, mebs):
    if k == 1:
        return [(F(0),tuple(F(v) for v in p)) for p in sites]
    out = []
    for b in balls:
        p, m, q = len(b['inner']), len(b['shell']), b['qmin']; t = k-p
        if not p+q-1 <= k <= p+m:
            continue
        if not any(mebs.beta(part) < b['level'] for part in it.combinations(b['shell'],t)):
            out.append((b['level'],b['center']))
    return out


def old_orders(text, balls, sites, kmax):
    lines, cursor, orders = text.splitlines(), 0, []
    mebs = geometry.definition.Definition(sites)
    for k in range(1,kmax+1):
        head = lines[cursor].split(); cursor += 1
        need(len(head) == 4 and head[:2] == ['order',str(k)] and int(head[3]) == len(sites), 'v10 order')
        count = int(head[2]); need(0 < count < 10000, 'v10 bounded node count')
        nodes, lower = [], []
        for i in range(count):
            row = lines[cursor].split(); cursor += 1
            need(len(row) == 6 and row[:2] == ['node',str(i)], 'v10 node line')
            parent, low = int(row[2]), int(row[5]); lower.append(low)
            need(parent == -1 or i < parent < count, 'v10 parent forward')
            nodes.append(dict(level=fraction(row[3:5]),parent=None if parent == -1 else parent,
                              children=[],center=None))
        for i,node in enumerate(nodes):
            if node['parent'] is not None:
                nodes[node['parent']]['children'].append(i)
        labels = birth_labels(balls,sites,k,mebs)
        need(len(labels) == sum(not n['children'] for n in nodes), 'v10 birth identity count')
        for i,(level,center) in enumerate(labels):
            need(not nodes[i]['children'] and nodes[i]['level'] == level, 'v10 birth identity level/order')
            nodes[i]['center'] = center
        for point in sites:
            row = lines[cursor].split(); cursor += 1
            need(len(row) == 6 and row[0] == 'point' and tuple(map(int,row[1:4])) == tuple(point),
                 'v10 point syntax/population')
            need(0 <= int(row[4]) < count and int(row[5]) >= 0, 'v10 core syntax')
        need(k > 1 or lower == [-1]*count, 'v10 lower order1')
        orders.append(dict(order=k,nodes=nodes,lower=None if k == 1 else lower))
    need(cursor == len(lines), 'v10 trailing lines')
    need(not mebs._orders, 'birth mapping must not construct a reference forest')
    return orders


def new_orders(row, bits, sites, kmax):
    need(row['status'] == 'ok' and row['reason'] == 'none' and row['coord_bits'] == bits and
         row['kmax'] == kmax and row['sites'] == [list(p) for p in sites], 'v11 reply domain')
    need(row['owner_after'] == 0 and row['forest_memory']['after'] == 0, 'v11 reservations')
    out = []
    for k,order in enumerate(row['orders'],1):
        need(order['order'] == k, 'v11 order')
        nodes = []
        for node in order['nodes']:
            level = fraction(node['level'],16); center = None
            if node['seed'] is not None:
                seed = node['seed']
                if k == 1:
                    need(seed['ball'] is None, 'v11 site seed')
                    center = tuple(F(v) for v in sites[seed['site']])
                else:
                    need(seed['site'] is None, 'v11 ball seed')
                    ball = row['balls'][seed['ball']]
                    sphere = geometry.definition.circumsphere([sites[i] for i in ball['support']])
                    need(sphere is not None and sphere[1] == level, 'v11 birth center/level')
                    center = sphere[0]
            nodes.append(dict(level=level,center=center,parent=node['parent'],children=node['children']))
        out.append(dict(order=k,nodes=nodes,lower=order['lower']))
    need(len(out) == kmax, 'v11 order count')
    return out


def common(sites, orders):
    """Independent renumbering on EACH native result; no alignment to the other result."""
    result, previous_map, previous_nodes = [], None, None
    for k,order in enumerate(orders,1):
        need(order['order'] == k, 'common order sequence')
        nodes = order['nodes']; count = len(nodes)
        need(count > 0, 'common nonempty forest')
        births = [i for i,n in enumerate(nodes) if n['center'] is not None]
        need(births and all(bool(n['children']) is (n['center'] is None) for n in nodes), 'common node kind')
        labels = [(nodes[i]['level'],nodes[i]['center']) for i in births]
        need(len(set(labels)) == len(labels), 'common duplicate birth')
        permutation = sorted(births,key=lambda i:(nodes[i]['level'],nodes[i]['center']))
        mapping = {old:new for new,old in enumerate(permutation)}
        minimum, seen, roots = dict(mapping), set(), 0
        for i,node in enumerate(nodes):
            parent = node['parent']; roots += int(parent is None)
            need(parent is None or type(parent) is int and i < parent < count, 'common parent')
            need(node['level'] >= 0 and (node['level'] == 0) is (k == 1 and i in births), 'common zero level')
            children = node['children']; need(children == sorted(set(children)), 'common child order')
            if children:
                need(len(children) >= 2, 'common nary merge')
                for child in children:
                    need(type(child) is int and 0 <= child < i and child not in seen and
                         nodes[child]['parent'] == i and nodes[child]['level'] < node['level'], 'common strict edges')
                    seen.add(child)
                minimum[i] = min(minimum[c] for c in children)
        need(roots == 1 and len(seen) == count-1, 'common unique connected root')
        permutation += sorted((i for i in range(count) if i not in mapping),key=lambda i:(nodes[i]['level'],minimum[i]))
        mapping = {old:new for new,old in enumerate(permutation)}
        lower = order['lower']; need((lower is None) is (k == 1), 'common vertical domain')
        if lower is not None:
            need(len(lower) == count, 'common vertical count')
            for i,target in enumerate(lower):
                need(type(target) is int and target in previous_map, 'common vertical index')
                below = previous_nodes[target]; parent = below['parent']; level = nodes[i]['level']
                need(below['level'] <= level and (parent is None or level < previous_nodes[parent]['level']),
                     'common closed vertical')
        normalized = []
        for i in permutation:
            n = nodes[i]
            normalized.append(dict(level=encoded(n['level']),center=None if n['center'] is None else
                                   [encoded(x) for x in n['center']],parent=None if n['parent'] is None else mapping[n['parent']],
                                   children=sorted(mapping[c] for c in n['children']),
                                   lower=None if lower is None else previous_map[lower[i]]))
        result.append(dict(order=k,nodes=normalized)); previous_map, previous_nodes = mapping,nodes
    return (json.dumps(dict(schema=SCHEMA,sites=sites,orders=result),sort_keys=True,separators=(',',':'))+'\n').encode()
