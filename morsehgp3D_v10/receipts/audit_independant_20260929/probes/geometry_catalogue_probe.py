"""Audit borne : catalogue fige contre geometrie Fraction exhaustive du depot.

La geometrie de reference n'a aucun code commun avec les predicats C++.
Cette sonde ajoute les poids, les extremes u18 et la canonicalisation Morton
a la porte produit ; elle verifie le contrat courant SPEC, pas GEN_v2.
"""
import argparse
from collections import Counter
from fractions import Fraction
from itertools import combinations
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile


def morton(p):
    return sum(((p[a] >> b) & 1) << (3 * b + a)
               for b in range(18) for a in range(3))


def parse_dump(path):
    result = []
    for line in path.read_text().splitlines():
        head, support, inner, shell = line.split('|')
        rank, q, p, u, flags = map(int, head.split())
        coords = lambda s: tuple(tuple(map(int, t.split(','))) for t in s.split())
        result.append(dict(rank=rank, q=q, p=p, u=u, flags=flags,
                           support=coords(support), I=coords(inner), U=coords(shell)))
    return result


def signature(ball):
    return (ball['q'], ball['p'], ball['u'], ball['flags'],
            ball['support'], ball['I'], ball['U'])


def oracle(R, raw):
    counts = Counter(raw)
    positions = sorted(counts, key=morton)
    expected = []
    for b in R.critical_balls(positions):
        if b.qmin < 2:
            continue
        I = tuple(positions[i] for i in b.I)
        U = tuple(positions[i] for i in b.U)
        support = None
        for S in combinations(U, b.qmin):
            cc = R.circumcenter(S)
            if cc is not None and cc[0] == b.center and all(v > 0 for v in cc[1]):
                support = S
                break
        if support is None:
            raise RuntimeError('oracle sans support canonique')
        weighted = any(counts[p] > 1 for p in U)
        expected.append(dict(q=b.qmin, p=sum(counts[p] for p in I),
                             u=sum(counts[p] for p in U),
                             flags=int(len(U) > b.qmin) + 2 * int(weighted),
                             support=support, I=I, U=U, level=b.level))
    return expected


def scenes():
    fixtures = [
        [(0, 0, 0), (4, 0, 0), (2, 3, 0)],
        [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)],
        [(0, 0, 0), (2, 0, 0), (0, 2, 0), (0, 0, 2), (2, 2, 2)],
        [(0, 0, 0), (262143, 0, 0), (131071, 262143, 0), (131071, 131071, 262143)],
        [(0, 0, 0), (225077, 1, 0), (225068, 1, 0), (152369, 7, 1)],
        [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)],
        [(0, 1, 1), (2, 1, 1), (1, 0, 1), (1, 2, 1), (1, 1, 0), (1, 1, 2)],
    ]
    rng = random.Random(20260929)
    for i in range(9):
        P = set()
        while len(P) < 8:
            if i % 3 == 0:
                P.add(tuple(rng.choice((0, 1, 262142, 262143)) for _ in range(3)))
            elif i % 3 == 1:
                P.add((rng.randrange(262144), rng.randrange(262144), 131071))
            else:
                P.add(tuple(rng.randrange(262144) for _ in range(3)))
        fixtures.append(sorted(P))
    for i, P in enumerate(fixtures):
        for weighted in (False, True):
            raw = [p for j, p in enumerate(P)
                   for _ in range((1 + (j * 3 + i) % 4) if weighted else 1)]
            yield i, weighted, raw


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(args.source / 'reference'))
    import hgp10_ref as R
    report = dict(source='6206d1d118794c9e1cabb6faeaec2aaa77d37e5b',
                  checks=0, scenes=0, balls=0, weighted_checks=0,
                  stricter_gen_v2_surplus=0, failures=[], configurations=[])
    with tempfile.TemporaryDirectory() as temporary:
        work = Path(temporary)
        src, dump = work / 'input.u32le', work / 'catalogue.txt'
        for scene, weighted, raw in scenes():
            report['scenes'] += 1
            expected = oracle(R, raw)
            src.write_bytes(b''.join(struct.pack('<III', *p) for p in raw))
            # Les coupes forcees ne servent qu'aux petites grilles : un M<K
            # sur une boite u18 tres etendue peut produire un arbre enorme.
            force = scene in (0, 1, 2, 5, 6)
            for K, leaf, workers in ((1, 64, 1), (2, 2 if force else 12, 4),
                                     (5, 3 if force else 12, 1), (10, 64, 4)):
                want = [b for b in expected if (b['p'] < K if b['flags'] & 2
                                               else b['p'] + b['q'] <= K + 1)]
                report['stricter_gen_v2_surplus'] += sum(b['p'] + b['q'] > K + 1 for b in want)
                command = [str(args.build / 'mhgp10_catalogue'), str(src),
                           '--k=%d' % K, '--leaf=%d' % leaf,
                           '--threads=%d' % workers, '--dump=' + str(dump)]
                report['checks'] += 1
                report['weighted_checks'] += weighted
                report['configurations'].append(dict(scene=scene, weighted=weighted,
                                                     K=K, leaf=leaf, workers=workers))
                error = None
                try:
                    run = subprocess.run(command, capture_output=True, text=True, timeout=2)
                except subprocess.TimeoutExpired as expired:
                    report['failures'].append(dict(scene=scene, weighted=weighted, K=K, leaf=leaf,
                                                   workers=workers, raw=raw, error='timeout_2s',
                                                   stdout=str(expired.stdout), stderr=str(expired.stderr)))
                    args.output.write_text(json.dumps(report, indent=2) + '\n')
                    continue
                if run.returncode:
                    error = dict(returncode=run.returncode, stdout=run.stdout, stderr=run.stderr)
                else:
                    got = parse_dump(dump)
                    W, G = Counter(map(signature, want)), Counter(map(signature, got))
                    report['balls'] += len(got)
                    if W != G:
                        error = dict(missing=list((W-G).elements()), extra=list((G-W).elements()))
                    else:
                        levels = {signature(b): b['level'] for b in want}
                        previous = None
                        for b in got:
                            level = levels[signature(b)]
                            if previous is not None and ((previous[0] == b['rank']) != (previous[1] == level)
                                                         or previous[1] > level):
                                error = dict(reason='rangs incoherents')
                                break
                            previous = b['rank'], level
                if error is not None:
                    report['failures'].append(dict(scene=scene, weighted=weighted, K=K,
                                                   leaf=leaf, workers=workers, error=error, raw=raw))
                args.output.write_text(json.dumps(report, indent=2) + '\n')
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'configurations'}, separators=(',', ':')))
    return 1 if report['failures'] else (3 if report['checks'] != 128 or report['balls'] < 100 else 0)


if __name__ == '__main__':
    sys.exit(main())
