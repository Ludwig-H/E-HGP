#!/usr/bin/env python3
"""Strict decoder of the bounded index query record, independent of native memory layouts."""
import hashlib
import struct

MAGIC = b'MHGP11IDX1'
LOGICAL = {'nodes', 'bounds', 'point_tests', 'inside_blocks', 'outside_blocks', 'passes'}
QUERIES = 64
BUDGET = 8 * 1024**3


def need(condition, message):
    if not condition:
        raise ValueError(message)


def integer(value, label, upper=2**64):
    need(type(value) is int and 0 <= value < upper, label)
    return value


def support(ordinal, count):
    group = ordinal // 4
    start = group * count // 16
    step = 1 if group % 2 == 0 else count // 4
    q = 1 + ordinal % 4
    return [(start + i * step) % count if i < q else 2**32 - 1 for i in range(4)]


def radix_shape(points, leaf=8):
    """Noeuds et profondeur (racine a 1) de l'arbre radix de Morton des positions distinctes, sans parcours natif.

    Chaque plage de plus de leaf cles est coupee au plus haut bit qui differe entre ses extremites (levier V3)."""
    keys = sorted({sum(((value >> bit) & 1) << (3 * bit + axis)
                        for axis, value in enumerate(point) for bit in range(value.bit_length())) for point in points})

    def shape(begin, end):
        if end - begin <= leaf:
            return 1, 1
        top = 1 << ((keys[begin] ^ keys[end - 1]).bit_length() - 1)
        split = next(i for i in range(begin, end) if keys[i] & top)
        left, right = shape(begin, split), shape(split, end)
        return 1 + left[0] + right[0], 1 + max(left[1], right[1])
    return shape(0, len(keys))


def coordinates_shape(data, leaf=8):
    """Forme radix d'un fichier de coordonnees xyz u32 petit-boutistes (12 octets par point)."""
    need(len(data) % 12 == 0 and data, 'coordonnees xyz u32')
    values = struct.unpack('<%dI' % (len(data) // 4), data)
    return radix_shape(zip(values[0::3], values[1::3], values[2::3]), leaf)


def validate_events(events, bits, count, shape=None):
    need(len(events) == QUERIES + 4 and [v['phase'] for v in events] ==
         ['cloud', 'index'] + ['query'] * QUERIES + ['summary', 'exit'], 'phases/inventaire')
    cloud, index, summary, end = events[0], events[1], events[-2], events[-1]
    need(cloud['sites'] == cloud['points'] == count and type(cloud['sites']) is int, 'entree entiere')
    for key in ('read_ns', 'cloud_ns', 'peak_reserved_bytes'):
        integer(cloud[key], 'cloud ' + key)
    need(type(cloud['reserved_after_bytes']) is int and
         cloud['reserved_after_bytes'] == 28 * count + 8 <= cloud['peak_reserved_bytes'] <= BUDGET, 'budget cloud')
    need(index['status'] == end['status'] == 'ok' and index['reason'] == end['reason'] == 'none', 'refus natif')
    need(type(index['coord_bits']) is int and index['coord_bits'] == bits and index['leaf_size'] == 8,
         'parametres index')
    for key in ('wall_ns', 'peak_reserved_bytes', 'reserved_after_bytes', 'nodes', 'max_depth'):
        integer(index[key], 'index ' + key)
    need(0 < index['reserved_after_bytes'] <= index['peak_reserved_bytes'] <= BUDGET and
         0 < index['nodes'] < 2 * count and 0 < index['max_depth'] <= 3 * bits + 1, 'index/memoire')
    need((shape is None or (index['nodes'], index['max_depth']) == shape) and
         type(index['node_bytes']) is int and index['node_bytes'] == 40 and
         index['reserved_after_bytes'] == index['peak_reserved_bytes'] ==
         cloud['reserved_after_bytes'] + index['node_bytes'] * index['nodes'], 'construction/plafond exacts')
    arities, kinds, query_ns, reference_ns = [0] * 4, {'complete': 0, 'saturated': 0, 'degenerate': 0}, 0, 0
    for ordinal, event in enumerate(events[2:-2]):
        q, threshold = 1 + ordinal % 4, (5, 10, 13)[ordinal % 3]
        need(all(type(event[k]) is int and event[k] == v for k, v in
                 (('ordinal', ordinal), ('arity', q), ('threshold', threshold))), 'identite requete')
        integer(event['factory_ns'], 'factory_ns')
        if event['status'] == 'degenerate':
            need(q in (3, 4), 'degenerescence impossible')
            kinds['degenerate'] += 1
            continue
        need(event['status'] == 'ok' and event['reason'] == 'none' and event['reference_ok'] is True,
             'requete/scan non conforme')
        for key in ('wall_ns', 'reference_ns', 'peak_reserved_bytes', 'reserved_after_bytes', 'interior', 'shell'):
            integer(event[key], 'requete ' + key)
        need(index['reserved_after_bytes'] <= event['reserved_after_bytes'] <= event['peak_reserved_bytes'] <= BUDGET,
             'budget requete')
        kind = event['kind']
        need(kind in ('complete', 'saturated') and event['interior'] + event['shell'] <= count, 'population')
        need(event['reserved_after_bytes'] == event['peak_reserved_bytes'] ==
             index['reserved_after_bytes'] + 4 * (event['interior'] + event['shell']), 'reservations census exactes')
        need((kind == 'complete' and event['interior'] < threshold) or
             (kind == 'saturated' and event['interior'] == threshold and event['shell'] == 0), 'saturation')
        values = event['logical']
        need(set(values) == LOGICAL and all(type(v) is int and 0 <= v < 2**64 for v in values.values()) and
             values['passes'] == 2 and values['nodes'] > 0, 'travail cumule des deux passes')
        arities[q - 1] += 1
        kinds[kind] += 1
        query_ns += event['wall_ns']
        reference_ns += event['reference_ns']
    expected = dict(queries=sum(arities), complete=kinds['complete'], saturated=kinds['saturated'],
                    degenerate=kinds['degenerate'], query_ns=query_ns, reference_ns=reference_ns,
                    reserved_after_bytes=index['reserved_after_bytes'])
    need(all(type(summary[k]) is int and summary[k] == v for k, v in expected.items()) and
         summary['arities'] == arities and all(type(v) is int for v in summary['arities']), 'resume decompte')
    need(arities[:2] == [16, 16] and min(arities[2:]) >= 1 and kinds['complete'] >= 16, 'planchers requetes')
    if count >= 8000:
        need(kinds['saturated'] >= 1, 'plancher saturation')
    return expected


def inspect(path, bits, count, events, shape=None):
    totals = validate_events(events, bits, count, shape)
    semantic = hashlib.sha256(b'ehgp.v11.index_query_semantic.v1\0')
    raw = hashlib.sha256()
    size = 0
    with path.open('rb') as stream:
        def take(length, semantic_part=True):
            nonlocal size
            data = stream.read(length)
            need(len(data) == length, 'canonique tronque')
            raw.update(data)
            if semantic_part:
                semantic.update(data)
            size += length
            return data

        def word():
            return struct.unpack('<Q', take(8))[0]

        need(take(10, False) == MAGIC and struct.unpack('<Q', take(8, False))[0] == bits, 'format/profil')
        need(word() == count and word() == QUERIES, 'population/requetes canoniques')
        for ordinal, event in enumerate(events[2:-2]):
            need(word() == event['arity'] and word() == event['threshold'], 'parametres canoniques')
            need([word() for _ in range(4)] == support(ordinal, count), 'supports canoniques')
            degenerate = word()
            need(degenerate == (1 if event['status'] == 'degenerate' else 0), 'degenerescence canonique')
            if degenerate:
                continue
            need(word() == (1 if event['kind'] == 'saturated' else 0), 'kind canonique')
            sizes = [word(), word()]
            need(sizes == [event['interior'], event['shell']], 'comptes canoniques')
            # Each accepted complete query has fewer than threshold interior sites; bounded scratch <=12 IDs.
            inside = set()
            for group, length in enumerate(sizes):
                previous = -1
                for _ in range(length):
                    site = word()
                    need(previous < site < count and (group == 0 or site not in inside), 'IDs canoniques')
                    if group == 0:
                        inside.add(site)
                    previous = site
        need(not stream.read(1), 'octets surnumeraires')
    return dict(sha256=semantic.hexdigest(), raw_sha256=raw.hexdigest(), bytes=size, **totals)


def work_signature(row):
    events = row['events']
    return (events[1]['nodes'], events[1]['max_depth'], tuple(
        ('degenerate',) if e['status'] == 'degenerate' else tuple(e['logical'][k] for k in sorted(LOGICAL))
        for e in events[2:-2]))
