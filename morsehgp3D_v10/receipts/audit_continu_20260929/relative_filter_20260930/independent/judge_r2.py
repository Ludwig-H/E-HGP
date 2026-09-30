"""Exact Fraction containment/status judge of a bounded relative filter."""
from fractions import Fraction as F
from collections import Counter, defaultdict
import json
from math import isfinite, nextafter, inf
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True


def require(value, message):
    if not value:
        raise ValueError(message)


def unique(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def load(text):
    def reject_constant(value):
        raise ValueError('nonfinite JSON constant: '+value)
    return json.loads(text, object_pairs_hook=unique, parse_constant=reject_constant)


def interval(bits):
    require(type(bits) is list and len(bits) == 2 and
            all(type(v) is int and 0 <= v < 2**64 for v in bits), 'interval bits')
    floats = [struct.unpack('<d', struct.pack('<Q', v))[0] for v in bits]
    require(all(isfinite(v) for v in floats) and floats[0] <= floats[1], 'finite ordered interval')
    return tuple(map(F, floats)), floats


def judge(requests, answers):
    require(len(requests) == len(answers) and len(requests) > 1500, 'panel completeness')
    counts = Counter()
    nearest = defaultdict(list)
    for index, (request, actual) in enumerate(zip(requests, answers)):
        require(type(request['id']) is int and request['id'] == index and
                actual['op'] == request['op'], 'paired identity')
        counts[request['op']] += 1
        if request['op'] == 'C':
            require(actual['status'] == 'READY', 'conversion refusal')
            (lo, hi), floats = interval(actual['value'])
            value = F(request['n'])
            require(lo <= value <= hi, 'conversion excludes true integer at '+str(index))
            if F(float(value)) == value:
                require(lo == hi == value, 'exact integer conversion widened')
            else:
                require(nextafter(floats[0], inf) == floats[1], 'conversion not adjacent')
            continue
        require(actual['status'] in ('INTERIOR', 'EXTERIOR', 'AMBIGU'), 'unexpected query refusal')
        require(type(actual['rho']) is list and len(actual['rho']) == 3, 'rho missing')
        rho = [F(n, request['d']) for n in request['n']]
        for coordinate, bits in zip(rho, actual['rho']):
            (lo, hi), _ = interval(bits)
            require(lo <= coordinate <= hi, 'relative centre enclosure')
        radius = sum(v*v for v in rho)
        (rlo, rhi), _ = interval(actual['radius'])
        (dlo, dhi), _ = interval(actual['distance'])
        require(0 <= rlo <= radius <= rhi and dlo >= 0, 'radius enclosure')
        if request['op'] == 'S':
            v = [F(x-a) for x, a in zip(request['point'], request['anchor'])]
            distance = sum((x-c)**2 for x, c in zip(v, rho))
        else:
            lo = [F(x-a) for x, a in zip(request['lo'], request['anchor'])]
            hi = [F(x-a) for x, a in zip(request['hi'], request['anchor'])]
            distance = sum(max(l-c, c-h, F(0))**2 for l, h, c in zip(lo, hi, rho))
            require(actual['status'] != 'INTERIOR', 'minimum box used as all-inside certificate')
        require(dlo <= distance <= dhi, 'distance enclosure at '+str(index))
        status = actual['status']
        counts[status] += 1
        if status == 'EXTERIOR':
            require(distance > radius and dlo > rhi, 'unsafe exterior at '+str(index))
        elif status == 'INTERIOR':
            require(distance < radius and dhi < rlo, 'unsafe interior at '+str(index))
        else:
            require(not (dlo > rhi or (request['op'] == 'S' and dhi < rlo)), 'unnecessary ambiguous state')
        if request.get('exact_contact'):
            require(distance == radius and status == 'AMBIGU', 'lost contact at '+str(index))
            counts['contacts'] += 1
        if 'equal_to' in request:
            require(actual == answers[request['equal_to']], 'translation changes native bits')
            counts['translations'] += 1
        if 'nearest_group' in request:
            nearest[request['nearest_group']].append((distance, request['site_id'], dlo, dhi))
    rank_checks = 0
    for rows in nearest.values():
        exact = sorted(rows)
        for k in range(1, len(rows)+1):
            upper = sorted(row[3] for row in rows)[k-1]
            kept = sorted(row for row in rows if row[2] <= upper)
            require([row[1] for row in kept[:k]] == [row[1] for row in exact[:k]],
                    'upper order statistic loses exact nearest tie')
            rank_checks += 1
    require(counts['contacts'] >= 100 and counts['EXTERIOR'] >= 50 and
            counts['INTERIOR'] >= 20 and counts['translations'] == 40, 'non-vacuity floors')
    return dict(status='PASS', requests=len(requests), counts=dict(counts),
                exact_rank_checks=rank_checks,
                scope='bounded converter/site/box filter; no native nearest, index, fallback or FULL')


if __name__ == '__main__':
    try:
        requests = load(Path(sys.argv[1]).read_text())
        answers = [load(line) for line in Path(sys.argv[2]).read_text().splitlines()]
        print(json.dumps(judge(requests, answers), sort_keys=True))
    except (ValueError, KeyError, TypeError, IndexError, ZeroDivisionError) as error:
        print(json.dumps(dict(status='FAIL', error=str(error)), sort_keys=True))
        raise SystemExit(1)
