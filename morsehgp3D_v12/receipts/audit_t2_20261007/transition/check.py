#!/usr/bin/env python3
"""Contre-lecture bornée indépendante ; vrais appels CLI, données synthétiques en temporaire."""
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
from fractions import Fraction as F

ROOT = Path(__file__).resolve().parents[4]
V12 = ROOT / 'morsehgp3D_v12'
PIN = '274592a30f6961cb7702125dcd2f031ff22b7df2'
READER = V12 / 'reference/transition_catalogue.py'
NONE = 2**32 - 1


def require(value, detail):
    if not value:
        raise RuntimeError(detail)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def sphere(points):
    """Centre par résolution du Gram rationnel, sans utiliser les formules du lecteur."""
    a = points[0]
    edges = [tuple(p[j] - a[j] for j in range(3)) for p in points[1:]]
    dot = lambda u, v: sum(x*y for x, y in zip(u, v))
    rows = [[F(dot(u, v)) for v in edges] + [F(dot(u, u), 2)] for u in edges]
    n = len(edges)
    for col in range(n):
        pivot = next((r for r in range(col, n) if rows[r][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        factor = rows[col][col]
        rows[col] = [x/factor for x in rows[col]]
        for r in range(n):
            if r != col:
                factor = rows[r][col]
                rows[r] = [x-factor*y for x, y in zip(rows[r], rows[col])]
    weights = [row[-1] for row in rows]
    if not all(w > 0 for w in weights) or sum(weights) >= 1:
        return None
    center = tuple(F(a[j]) + sum(weights[i]*edges[i][j] for i in range(n)) for j in range(3))
    radius = sum((x-y)**2 for x, y in zip(center, a))
    return center, radius


def exhaustive(points, k):
    """Toutes les parties de cardinal 2..4 ; identités rationnelles et populations exhaustives."""
    by_key = {}
    for q in range(2, min(4, len(points)) + 1):
        for ids in itertools.combinations(range(len(points)), q):
            key = sphere([points[i] for i in ids])
            if key is None:
                continue
            by_key.setdefault(key, []).append(ids)
    balls = []
    for key, supports in by_key.items():
        center, radius = key
        powers = [sum((x-y)**2 for x, y in zip(center, p))-radius for p in points]
        inner = tuple(i for i, power in enumerate(powers) if power < 0)
        shell = tuple(i for i, power in enumerate(powers) if power == 0)
        q = min(map(len, supports))
        if len(inner) + q <= k + 1:
            balls.append({'key': key, 'q': q, 'inner': inner, 'shell': shell,
                          'supports': [s for s in supports if len(s) == q]})
    return balls


def morton(p):
    return sum(((p[axis] >> bit) & 1) << (3*bit+axis) for bit in range(32) for axis in range(3))


def dump(points, balls, k, convention, reverse=False, frame=b'audit', change=None):
    order = sorted(range(len(points)), key=lambda i: morton(points[i]), reverse=reverse)
    indices = {old: new for new, old in enumerate(order)}
    rows = []
    for b in balls:
        support_key = (lambda s: tuple(sorted(indices[i] for i in s))) if convention == 'v11' else (
            lambda s: tuple(sorted(points[i] for i in s)))
        chosen = min(b['supports'], key=support_key)
        rows.append({'key': b['key'], 'q': b['q'], 's': tuple(sorted(indices[i] for i in chosen)),
                     'inner': tuple(sorted(indices[i] for i in b['inner'])),
                     'shell': tuple(sorted(indices[i] for i in b['shell'])),
                     'sort': support_key(chosen)})
    rows.sort(key=lambda r: (r['key'][1], r['sort']))
    if change == 'drop':
        rows.pop(0)
    elif change == 'duplicate':
        rows.insert(0, rows[0])
    elif change == 'omit_inner':
        row = next(r for r in rows if r['inner'])
        row['inner'] = row['inner'][1:]
    ranks = {level: i+1 for i, level in enumerate(sorted({r['key'][1] for r in rows}))}
    records, offsets, values = [], [0], []
    for row in rows:
        records.append(struct.pack('<8I', ranks[row['key'][1]], len(row['inner']), len(row['shell']), row['q'],
                                   *(row['s']+(NONE,)*(4-row['q']))))
        values.extend(row['inner']+row['shell'])
        offsets.append(len(values))
    sections = [('SITEXYZ', 12, len(points), b''.join(struct.pack('<3I', *points[i]) for i in order)),
                ('BALLS', 32, len(rows), b''.join(records)),
                ('POPOFF', 8, len(offsets), b''.join(struct.pack('<Q', x) for x in offsets)),
                ('POPVAL', 4, len(values), b''.join(struct.pack('<I', x) for x in values)),
                ('NLEVELS', 8, 1, struct.pack('<Q', len(ranks)+1))]
    bits = max(1, max((max(p).bit_length() for p in points), default=0))
    out = bytearray(struct.pack('<8s6IQ24s', b'MHGP12DP', 1, 1, bits, k, 0, len(sections), len(points), frame))
    for tag, elem, count, data in sections:
        out += struct.pack('<8sIIQ', tag.encode(), elem, 0, count) + data + b'\0'*(-len(data) % 8)
    return bytes(out)


def interpreter():
    return [sys.executable, '-B', '-S'] + (['-O'] if sys.flags.optimize else [])


def invoke(directory, name, ref, cand, expected):
    paths = [directory / (name+'_'+side+'.bin') for side in ('ref', 'cand')]
    for path, data in zip(paths, (ref, cand)):
        path.write_bytes(data)
    report_path = directory / (name+'.json')
    done = subprocess.run(interpreter()+[str(READER), *map(str, paths), '--rapport', str(report_path)],
                          capture_output=True, text=True, timeout=30, check=False)
    require(done.returncode == expected, (name, done.returncode, expected, done.stdout, done.stderr))
    result = {'case': name, 'code': done.returncode, 'line': done.stdout.splitlines()[0] if done.stdout else '',
              'inputs_sha256': [sha(ref), sha(cand)], 'bytes': [len(ref), len(cand)]}
    if report_path.exists():
        report = json.loads(report_path.read_text())
        result.update({'ecarts': report['ecarts'], 'appariees': report['croise']['appariees'],
                       'sstar_differents': report['croise']['sstar_differents'], 'trame': report['trame']})
    else:
        require('transition_catalogue_refus' in done.stderr, 'refus sans diagnostic')
    return result


def main():
    paths = [READER, V12/'reference/transition_temoins.py', V12/'reference/test_transition_catalogue.py',
             V12/'reference/fixtures/transition_catalogue.json', V12/'reference/README.md',
             V12/'reference/tests.cmake', V12/'cmake/gates.cmake', V12/'docs/CONTRAT_CATALOGUE.md',
             V12/'microbancs/mes_m3_m4_tour/common/format.hpp']
    paths.extend(sorted((V12/'reference/hgp12_ref').glob('*.py')))
    before = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in paths}
    for p in paths:
        committed = subprocess.check_output(['git', 'show', PIN+':'+str(p.relative_to(ROOT))], cwd=ROOT)
        require(sha(committed) == before[str(p.relative_to(ROOT))], 'source différente du pin')
    clouds = [([(0, 1, 1), (1, 0, 1), (1, 1, 0)], 1),
              ([(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)], 2),
              ([(1, 0, 0), (2, 1, 0), (1, 2, 0), (0, 1, 0), (1, 1, 0)], 2),
              ([(0, 5, 5), (10, 5, 5), (5, 10, 5), (5, 2, 9), (5, 2, 1)], 2),
              ([(3, 4, 5), (4, 3, 5), (4, 7, 5), (7, 4, 5)], 2),
              ([(3, 4, 5), (3, 5, 4), (3, 5, 6), (5, 7, 4), (7, 4, 5)], 3)]
    rng = random.Random(120710)
    for _ in range(4):
        points = set()
        while len(points) < 6:
            points.add(tuple(rng.randrange(16) for _ in range(3)))
        clouds.append((sorted(points), 4))
    results, balls_total, counts_q = [], 0, {2: 0, 3: 0, 4: 0}
    with tempfile.TemporaryDirectory(prefix='ehgp_transition_audit_') as temporary:
        directory = Path(temporary)
        for cidx, (base, k) in enumerate(clouds):
            width = max(max(p) for p in base)
            for variant in range(4):
                scale = (2**32-1)//max(1, width) if variant == 2 else 1
                shift = 2**21-1-width if variant == 1 else (2**32-1-width if variant == 3 else 0)
                points = [tuple(scale*x+shift for x in p) for p in base]
                balls = exhaustive(points, k)
                for b in balls:
                    counts_q[b['q']] += 1
                balls_total += len(balls)
                results.append(invoke(directory, 'exact_%d_%d' % (cidx, variant),
                                      dump(points, balls, k, 'v11'), dump(points, balls, k, 'v12', reverse=True), 0))
        points, k = clouds[2]
        balls = exhaustive(points, k)
        ref, cand = dump(points, balls, k, 'v11'), dump(points, balls, k, 'v12')
        for change, expected in [('drop', 1), ('duplicate', 1), ('omit_inner', 1)]:
            results.append(invoke(directory, change, ref, dump(points, balls, k, 'v12', change=change), expected))
        # Limite déclarée, pas un défaut : la même omission dans les deux populations passe.
        results.append(invoke(directory, 'common_inner_omission_declared_limit',
                              dump(points, balls, k, 'v11', change='omit_inner'),
                              dump(points, balls, k, 'v12', change='omit_inner'), 0))
        results.append(invoke(directory, 'different_ascii_frames_refused', ref,
                              dump(points, balls, k, 'v12', frame=b'autre'), 2))
        # Identités brutes distinctes remplacées par le même caractère U+FFFD.
        results.append(invoke(directory, 'different_invalid_frames_accepted',
                              dump(points, balls, k, 'v11', frame=b'audit\xff'),
                              dump(points, balls, k, 'v12', frame=b'audit\xfe'), 0))
        # Entier Python : le compteur qui fait déborder l'addition du Reader C++ est refusé ici.
        bad_count = bytearray(cand)
        struct.pack_into('<Q', bad_count, 64+16, 2**62-1)
        results.append(invoke(directory, 'huge_section_refused', ref, bytes(bad_count), 2))
    require(all(counts_q[q] > 0 for q in counts_q), 'cardinal absent')
    official = []
    test = V12/'reference/test_transition_catalogue.py'
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', PYTHONOPTIMIZE=str(int(bool(sys.flags.optimize))))
    listing = subprocess.check_output(interpreter()+[str(test), '--list-mutants'], env=env, text=True)
    commands = [([], 0)] + [(['--inject='+line.split()[0]], 0 if line.endswith('equivalent') else 4)
                            for line in listing.splitlines()]
    for args, expected in commands:
        done = subprocess.run(interpreter()+[str(test)]+args, env=env, capture_output=True, text=True,
                              timeout=30, check=False)
        require(done.returncode == expected, (args, done.returncode, done.stdout, done.stderr))
        official.append({'args': args, 'code': done.returncode, 'stdout': done.stdout.splitlines()})
    after = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in paths}
    require(before == after, 'sources modifiées pendant la capture')
    output = {'schema': 'ehgp.audit.transition.v1', 'pin': PIN, 'source_sha256': before,
              'source_git_pin_and_before_after_equal': True, 'script_sha256': sha(Path(__file__).read_bytes()),
              'independent_catalogues': 40, 'independent_balls': balls_total, 'independent_by_q': counts_q,
              'cli_cases': results, 'official_small_tests': official,
              'scope': 'Fraction exhaustive n<=6, vrais main ; aucun LiDAR, natif, GCP ou qualification FULL'}
    print(json.dumps(output, indent=1, sort_keys=True))


if __name__ == '__main__':
    main()
