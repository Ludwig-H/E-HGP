#!/usr/bin/env python3
"""MEB proof decoder: normalize rational centers/levels, preserve local supports and census populations."""
from fractions import Fraction
import hashlib
from math import comb
import struct

from catalogue_semantic import natural
from index_semantic import BUDGET, LOGICAL as CENSUS_LOGICAL, integer, need, tree_shape

MAGIC = b'MHGP11MEB1'
QUERIES = 48
MEB_LOGICAL = {'presentations', 'nondegenerate', 'positive', 'containing', 'comparisons', 'point_tests', 'diameter_pairs'}
SEARCH = 'exact_diameter_then_q34_v1'
TIMES = ('meb_ns', 'census_ns', 'wrapper_ns', 'reference_ns')
NONE = 2**32 - 1


def part(ordinal, count):
    group, size = ordinal // 12, 1 + ordinal % 12
    start = (group * count // 4 + (size - 1) * (count // 48 + 1)) % count
    step = 1 if group % 2 == 0 else count // 12
    return sorted((start + i * step) % count for i in range(size))


def validate_events(events, bits, count):
    need(count >= 12 and len(events) == QUERIES + 4 and [v['phase'] for v in events] ==
         ['cloud', 'index'] + ['query'] * QUERIES + ['summary', 'exit'], 'phases/inventaire')
    cloud, index, summary, end = events[0], events[1], events[-2], events[-1]
    need(all(type(cloud[k]) is int and cloud[k] == count for k in ('sites', 'points')), 'entree entiere')
    for key in ('read_ns', 'cloud_ns', 'peak_reserved_bytes', 'reserved_after_bytes'):
        integer(cloud[key], 'cloud ' + key)
    need(cloud['reserved_after_bytes'] == 28 * count + 8 <= cloud['peak_reserved_bytes'] <= BUDGET, 'budget cloud')
    need(index['status'] == end['status'] == 'ok' and index['reason'] == end['reason'] == 'none', 'refus natif')
    need(type(index['coord_bits']) is int and index['coord_bits'] == bits and
         type(index['leaf_size']) is int and index['leaf_size'] == 8, 'parametres index')
    for key in ('wall_ns', 'peak_reserved_bytes', 'reserved_after_bytes', 'nodes', 'max_depth', 'node_bytes'):
        integer(index[key], 'index ' + key)
    need((index['nodes'], index['max_depth']) == tree_shape(count) and index['node_bytes'] == 40 and
         index['reserved_after_bytes'] == index['peak_reserved_bytes'] ==
         cloud['reserved_after_bytes'] + 40 * index['nodes'] <= BUDGET, 'construction index exacte')
    totals = dict(queries=48, complete=0, saturated=0, reserved_after_bytes=index['reserved_after_bytes'])
    totals.update(dict.fromkeys(TIMES, 0))
    sizes = [0] * 4
    for ordinal, event in enumerate(events[2:-2]):
        size, threshold = 1 + ordinal % 12, (5, 10, 13)[ordinal % 3]
        need(all(type(event[k]) is int and event[k] == v for k, v in
                 (('ordinal', ordinal), ('size', size), ('threshold', threshold))), 'identite requete')
        need(event['status'] == 'ok' and event['reason'] == 'none' and event['reference_ok'] is True and
             event.get('meb_search') == SEARCH,
             'MEB/scan/wrapper non conforme')
        q = integer(event['support_size'], 'support_size', 5)
        need(1 <= q <= size and (q == 1) is (size == 1), 'support local strict')
        sizes[q - 1] += 1
        for key in TIMES:
            totals[key] += integer(event[key], key)
        for key in ('interior', 'shell'):
            integer(event[key], key, count + 1)
        kind = event['kind']
        need(kind in ('complete', 'saturated') and event['interior'] + event['shell'] <= count and
             ((kind == 'complete' and event['interior'] < threshold and event['shell'] >= q) or
              (kind == 'saturated' and event['interior'] == threshold and event['shell'] == 0)), 'population')
        totals[kind] += 1
        delta = 4 * (event['interior'] + event['shell'])
        for stage, copies in (('meb', 0), ('census', 1), ('wrapper', 2)):
            peak = integer(event[stage + '_peak_bytes'], stage + '_peak_bytes')
            after = integer(event[stage + '_after_bytes'], stage + '_after_bytes')
            need(peak == after == index['reserved_after_bytes'] + copies * delta <= BUDGET, 'memoire ' + stage)
        m, c = event['meb_logical'], event['census_logical']
        for values, keys in ((m, MEB_LOGICAL), (c, CENSUS_LOGICAL)):
            need(set(values) == keys and all(type(v) is int and 0 <= v < 2**64 for v in values.values()), 'compteurs')
        lower = 1 if q <= 2 else 2 + sum(comb(size, r) for r in range(3, q))
        upper = 1 if q <= 2 else lower - 1 + comb(size, q)
        need(m['diameter_pairs'] == comb(size, 2), 'auxiliary diameter work')
        need(lower <= m['presentations'] <= upper and m['containing'] == 1 and m['comparisons'] == 0 and
             1 <= m['positive'] <= m['nondegenerate'] <= m['presentations'] and
             (m['positive'] == 1 if q <= 2 else m['positive'] >= 2) and
             m['positive'] + size - 1 <= m['point_tests'] <= size * m['positive'], 'travail MEB arrete')
        need(c['passes'] == 2 and c['nodes'] > 0, 'travail census deux passes')
    need(all(type(summary[k]) is int and summary[k] == v for k, v in totals.items()) and
         summary['support_sizes'] == sizes and all(type(v) is int for v in summary['support_sizes']), 'resume')
    need(sizes[0] == 4 and totals['complete'] >= 4, 'planchers MEB')
    if count >= 8000:
        need(totals['saturated'] >= 1, 'plancher saturation')
    return dict(totals, support_sizes=sizes)


def inspect(path, bits, count, events):
    totals = validate_events(events, bits, count)
    semantic = hashlib.sha256(b'ehgp.v11.meb_semantic.v1\0')
    raw, size = hashlib.sha256(), 0
    with path.open('rb') as stream:
        def take(length):
            nonlocal size
            data = stream.read(length)
            need(len(data) == length, 'canonique tronque')
            raw.update(data)
            size += length
            return data

        def word():
            return struct.unpack('<Q', take(8))[0]

        def exact(budget, positive=False):
            negative, limbs = word(), word()
            need(negative in (0, 1) and limbs == (2 if budget <= 127 else (budget + 63) // 64), 'entier forme')
            value = sum(word() << (64 * i) for i in range(limbs))
            need(value < 2**budget and (not negative or value > 0), 'entier domaine')
            value = -value if negative else value
            need(not positive or value > 0, 'denominateur positif')
            return value

        def feed(values):
            for value in values:
                semantic.update(natural(value))

        need(take(10) == MAGIC and word() == bits and word() == count and word() == QUERIES, 'entete')
        feed((count, QUERIES))
        for ordinal, event in enumerate(events[2:-2]):
            cardinal, threshold = word(), word()
            need(cardinal == event['size'] and threshold == event['threshold'], 'parametres')
            selection = [word() for _ in range(12)]
            need(selection == part(ordinal, count) + [NONE] * (12 - cardinal), 'partie canonique')
            q, support = word(), [word() for _ in range(4)]
            need(q == event['support_size'] and support[:q] == sorted(set(support[:q])) and
                 support[q:] == [NONE] * (4 - q) and set(support[:q]) <= set(selection[:cardinal]), 'support local')
            positions = [selection[:cardinal].index(site) for site in support[:q]]
            rank, start = 0, 0
            for depth, position in enumerate(positions):
                rank += sum(comb(cardinal - i - 1, q - depth - 1) for i in range(start, position))
                start = position + 1
            expected = 1 if q <= 2 else 2 + rank + sum(comb(cardinal, r) for r in range(3, q))
            need(event['meb_logical']['presentations'] == expected, 'rang exact du support arretant')
            feed((cardinal, threshold, *selection, q, *support))
            anchor = [word() for _ in range(3)]
            need(max(anchor) < 2**bits, 'ancre domaine')
            numerator = [exact(5 * bits + 5) for _ in range(3)]
            denominator = exact(4 * bits + 5, True)
            level_n, level_d = exact(8 * bits + 12), exact(6 * bits + 8, True)
            need(level_n >= 0 and (level_n == 0) is (q == 1) and
                 sum(n * n for n in numerator) * level_d == level_n * denominator**2, 'sphere/niveau')
            center = [Fraction(a * denominator + n, denominator) for a, n in zip(anchor, numerator)]
            need(all(0 <= x <= 2**bits - 1 for x in center), 'centre MEB hors domaine')
            for rational in center + [Fraction(level_n, level_d)]:
                feed((int(rational.numerator < 0), abs(rational.numerator), rational.denominator))
            kind, ni, nu = word(), word(), word()
            need(kind == int(event['kind'] == 'saturated') and [ni, nu] == [event['interior'], event['shell']], 'census')
            feed((kind, ni, nu))
            inner, found, selected_found = set(), set(), set()
            for group, length in enumerate((ni, nu)):
                last = -1
                for _ in range(length):
                    site = word()
                    need(last < site < count and (group == 0 or site not in inner), 'population ordre/disjonction')
                    if group == 0:
                        need(site not in support[:q], 'support dans interieur strict')
                        inner.add(site)
                    elif site in support[:q]:
                        found.add(site)
                    last = site
                    if site in selection[:cardinal]:
                        selected_found.add(site)
                    feed((site,))
            need(kind == 1 or (len(found) == q and len(selected_found) == cardinal), 'partie/support hors boule')
        need(not stream.read(1), 'octets supplementaires')
    return dict(sha256=semantic.hexdigest(), raw_sha256=raw.hexdigest(), bytes=size, meb_search=SEARCH, **totals)


def work_signature(row):
    events = row['events']
    return (events[1]['nodes'], events[1]['max_depth'], tuple(
        (tuple(e['meb_logical'][k] for k in sorted(MEB_LOGICAL)),
         tuple(e['census_logical'][k] for k in sorted(CENSUS_LOGICAL))) for e in events[2:-2]))
