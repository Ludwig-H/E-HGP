#!/usr/bin/env python3
"""Read-only size/work census of dated deposits in a closed native case.

Writes a NEW private JSON report only. No geometry, fit, EOM, numerical
deposition implementation or runtime comparison is performed. The tiny
Fraction self-test checks the multiplicity identity, not geometric anchors.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from fractions import Fraction as Q
import gzip
import hashlib
from itertools import combinations
import json
import math
from pathlib import Path
import statistics
import sys
import time


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def read(path):
    if str(path).endswith('.gz'):
        with gzip.open(path, 'rt') as stream:
            return json.load(stream)
    return json.loads(Path(path).read_text())


def beta(row):
    need(set(row) == {'num', 'den'}, 'exact beta schema')
    a, b = int(row['num']), int(row['den'])
    need(a >= 0 and b > 0, 'nonnegative squared radius')
    return Q(a, b)


def event(row):
    need(type(row['node']) is int and row['node'] >= 0, 'FULL node ID')
    return row['node'], beta(row['beta'])


def selftest():
    """Independent expand-then-sum versus multiplicity algebra, all exact."""
    fixtures = coface_checks = point_checks = 0
    for n in range(3, 9):
        for k in range(1, min(4, n-1) + 1):
            cofaces = list(combinations(range(n), k+1))
            facets = sorted({face for sigma in cofaces for face in combinations(sigma, k)})
            # Arbitrary consistent assignments test the identity without
            # assuming geometry or the dimension-three group bound.
            for regime in ('integers', 'fractions'):
                owners = {face: (i % 4, Q((i % 7)+1, 3)) for i, face in enumerate(facets)}
                weights = [Q(i+1) if regime == 'integers' else Q(i+1, i+2) for i in range(len(cofaces))]
                scores = defaultdict(Q)
                for sigma, weight in zip(cofaces, weights):
                    for face in combinations(sigma, k):
                        scores[face] += weight
                expanded = defaultdict(Q)
                totals = defaultdict(Q)
                for face, score in scores.items():
                    for x in face:
                        expanded[owners[face], x] += score
                        totals[x] += score
                direct = defaultdict(Q)
                shortcut_totals = defaultdict(Q)
                for sigma, weight in zip(cofaces, weights):
                    omitted = {x: owners[tuple(y for y in sigma if y != x)] for x in sigma}
                    counts = Counter(omitted.values())
                    for owner, count in counts.items():
                        for x in sigma:
                            multiplicity = count - (omitted[x] == owner)
                            explicit = sum(x in face and owners[face] == owner for face in combinations(sigma, k))
                            need(multiplicity == explicit and multiplicity >= 0, 'integer multiplicity identity')
                            if multiplicity:
                                direct[owner, x] += weight * multiplicity
                            point_checks += 1
                    for x in sigma:
                        shortcut_totals[x] += k * weight
                    coface_checks += 1
                need(dict(direct) == dict(expanded), 'exact grouped numerators')
                need(dict(totals) == dict(shortcut_totals), 'exact point denominator shortcut')
                old_mass = defaultdict(Q)
                for face, score in scores.items():
                    old_mass[owners[face]] += sum((score / totals[x] for x in face), Q())
                new_mass = defaultdict(Q)
                for (owner, x), value in direct.items():
                    new_mass[owner] += value / totals[x]
                need(dict(old_mass) == dict(new_mass) and sum(new_mass.values()) == n, 'exact deposit mass conservation')
                fixtures += 1
    return dict(status='passed', fixtures=fixtures, coface_checks=coface_checks,
                integer_multiplicity_checks=point_checks,
                scope='exact integer/Fraction algebra only; arbitrary event assignments, no geometric or EOM gate')


def measure(directory):
    started = time.monotonic()
    paths = {name: directory / name for name in ('native.json', 'measure_z1.json.gz', 'command.json', 'weighted_m50_z2.json.gz')}
    need(all(path.is_file() for path in paths.values()), 'completed case files required, including final m50/z2 output')
    source = Path(__file__).resolve()
    executable = Path(sys.executable).resolve()
    pins = {str(path): sha(path) for path in (*paths.values(), source, executable)}
    command = read(paths['command.json'])
    need(command['returncode'] == 0 and command['stdout_sha256'] == pins[str(paths['native.json'])], 'native command binding')
    payload = read(paths['native.json'])
    need(payload['schema'] == 'mhgp9_weighted_full_attachment_export_v1' and payload['status'] == 'completed', 'completed attachment export')
    weighted = payload['weighted']; native = weighted['native']; k = native['k']
    need(weighted['status'] == native['status'] == 'completed' and command['k'] == k, 'completed FULL/order binding')
    need(k in (5, 10) and native['point_count'] == 1200, 'declared two-case diagnostic domain')
    need(directory.name == command['case'] + '_k' + str(k), 'case identity')
    model = read(paths['measure_z1.json.gz'])
    count = len(payload['attachments'])
    need(all(len(model[key]) == count for key in ('facets', 'scores', 'masses', 'attachments', 'leaf_birth_betas')), 'measure dimensions')
    events = {}; masses = defaultdict(list); multiplicities = Counter(); by_face = {}
    for row, face, score, mass, attachment, birth in zip(payload['attachments'], model['facets'], model['scores'],
                                                        model['masses'], model['attachments'], model['leaf_birth_betas']):
        key = event(row)
        need(face == row['vertices'] == sorted(set(face)) and len(face) == k, 'native/measure facet binding')
        need(key == event(attachment) and key[1] == beta(birth), 'native/measure exact attachment binding')
        need(math.isfinite(score) and score > 0 and math.isfinite(mass) and 0 < mass <= 1+1e-12, 'positive complete-boundary measure')
        facet = tuple(face); need(facet not in by_face, 'distinct facets')
        by_face[facet] = key
        events.setdefault(key, set()).update(face)
        multiplicities[key] += 1; masses[key].append(mass)
    cofaces = weighted['cofaces']; catalogue = weighted['catalogue']; nodes = native['nodes']
    groups = Counter(); supports = Counter(); outside_checked = old_work = new_work = 0
    for sigma in cofaces:
        vertices = tuple(sigma['vertices'])
        need(len(vertices) == k+1 and list(vertices) == sorted(set(vertices)), 'canonical coface')
        ball = catalogue[sigma['ball']]
        masks = [mask for mask in ball['minimal_support_masks'] if mask & sigma['shell_mask'] == mask]
        need(masks, 'support contained in this coface')
        chosen = min(masks, key=lambda mask: (mask.bit_count(), mask))
        support = {x for bit, x in enumerate(ball['shell']) if chosen & (1 << bit)}
        need(support <= set(vertices) and 1 <= len(support) <= 4, 'chosen dimension-three support')
        owners = [by_face[vertices[:i] + vertices[i+1:]] for i in range(k+1)]
        g = len(set(owners)); need(g <= len(support)+1 <= 5, 'at most five events per coface')
        outside = [owner for x, owner in zip(vertices, owners) if x not in support]
        radius = beta(sigma['beta'])
        need(radius == beta(ball['beta']) and outside and len(set(outside)) == 1, 'common outside-support event')
        need(all(owner[1] == radius for owner in outside), 'outside-support MEB identity')
        anchor = payload['anchors'][sigma['ball']]
        need(anchor is not None, 'captured coface anchor for these two cases')
        while nodes[anchor]['successor'] is not None and beta(nodes[nodes[anchor]['successor']]['level']) <= radius:
            anchor = nodes[anchor]['successor']
        need(outside[0] == (anchor, radius), 'same normalized closed FULL component')
        groups[g] += 1; supports[len(support)] += 1; outside_checked += len(outside)
        old_work += k*(k+1); new_work += (k+1)*g
    D, J, C = len(events), sum(map(len, events.values())), len(cofaces)
    deposits = [math.fsum(terms) for terms in masses.values()]
    result = dict(schema='mhgp9_deposit_group_census_v1', status='passed', case=directory.name, k=k, exp_z=1,
        n=native['point_count'], cofaces=C, facets=count, deposits=D, point_deposit_incidences=J,
        facet_point_incidences=k*count, full_nodes=len(nodes), full_roots=len(native['roots']),
        weighted_internal_nodes=len(model['children']), weighted_total_nodes=count+len(model['children']),
        weighted_edges=sum(map(len, model['children'].values())),
        facet_to_deposit_ratio=count/D, incidence_ratio=k*count/J,
        facets_per_deposit_median=statistics.median(multiplicities.values()), facets_per_deposit_max=max(multiplicities.values()),
        max_facet_mass=max(model['masses']), max_deposit_mass=max(deposits),
        deposits_mass_ge20=sum(value >= 20 for value in deposits), deposits_mass_ge50=sum(value >= 50 for value in deposits),
        group_histogram=dict(sorted(groups.items())), chosen_support_histogram=dict(sorted(supports.items())),
        maximum_groups=max(groups), mean_groups=sum(g*c for g,c in groups.items())/C,
        outside_support_omissions_checked=outside_checked, outside_support_same_event_cofaces=C,
        work=dict(naive_K_Kplus1_C=old_work, grouped_sum_Kplus1_g=new_work,
                  event_group_lookups_Kplus1_C=(k+1)*C, existing_score_then_point_accumulation=(k+1)*C+k*count,
                  grouped_plus_event_lookups=(k+1)*C+new_work, naive_to_grouped_ratio=old_work/new_work),
        illustrative_typed_bytes=dict(old_point_ids_plus_facet_score_mass=4*k*count+16*count,
            new_point_ids_scores_plus_deposit_mass=12*J+8*D,
            scope='uint32 IDs, binary64 scores/masses; excludes topology, dates, containers, alignment and transient work'),
        measure_gzip_bytes=paths['measure_z1.json.gz'].stat().st_size,
        command=command, source_path=str(source), source_sha256=pins[str(source)], python=sys.version,
        pins_before=pins, arithmetic_selftest=selftest(),
        geometry_independently_recomputed=False, EOM_executed=False, runtime_gain_claimed=False,
        scope='read-only captured sizes and work counts, not whole campaign qualification',
        limitations=['Support/census/attachment provenance is the native capture; no independent MEB oracle here.',
                    'The old K(K+1)C expansion is not the factorized current implementation.',
                    'Exact algebra self-test does not certify binary64 regrouping, EOM, labels or runtime.',
                    'Both observed maximum deposit masses are below m; large-deposit pruning requires separate tests.'])
    after = {path: sha(path) for path in pins}
    need(after == pins, 'input/source/executable changed during census')
    result.update(pins_after=after, wall_seconds=time.monotonic()-started)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest:
        need(not args.case and not args.output, 'selftest is separate')
        print(json.dumps(selftest(), sort_keys=True))
    else:
        need(args.case and args.output and not args.output.exists(), 'case and NEW output required')
        # Preserve the supplied case identity even when root relocated it
        # behind a symlink; hashes bind the actual contents before/after.
        result = measure(args.case.absolute())
        result['driver_argv'] = [sys.executable, *sys.argv]
        with args.output.open('x') as stream:
            json.dump(result, stream, sort_keys=True, indent=2, allow_nan=False)
            stream.write('\n')
        print(json.dumps({key: result[key] for key in ('status', 'case', 'facets', 'deposits',
            'point_deposit_incidences', 'group_histogram', 'work', 'source_sha256', 'wall_seconds')}, sort_keys=True))
