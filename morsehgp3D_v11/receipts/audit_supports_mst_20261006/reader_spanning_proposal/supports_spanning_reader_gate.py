#!/usr/bin/env python3
"""Porte stdlib autonome du lecteur SPv2 : triangle MST, cycle, composantes disjointes."""
import argparse
import hashlib
import json
import struct
import sys


def column(values, size):
    raw = b''.join(value.to_bytes(size, 'little') for value in values)
    return raw + b'\0' * (-len(raw) % 8)


def fixture(points, pairs, bits=21):
    """K1 ; feuilles XYZ lexicographiques, sites Morton ; un plateau de fusion."""
    n, b = len(points), len(pairs)
    lex = sorted(range(n), key=lambda i: points[i])
    node = {site: v for v, site in enumerate(lex)}
    sections = [
        b''.join(column([p[axis] for p in points], 4) for axis in range(3)) + column(list(range(10, 10 + n)), 4),
        column([n] * n + [2**32 - 1], 4) + column([0] * n + [1], 4) +
        column([0] * n + [2], 1) + column([0] * n + [b], 4),
        column([1] * b, 4) + column([2] * b, 4) + column([1] * b, 1) +
        column([0] * b, 1) + column([2] * b, 1),
        column([2] * b, 1) + column([i for pair in pairs for i in pair], 4),
        column([v for pair in pairs for v in sorted(node[i] for i in pair)], 4),
    ]
    offsets = [136]
    for part in sections:
        offsets.append(offsets[-1] + len(part))
    words = [2, bits, 1, n, n + 1, n, b, b, 2 * b, 2 * b] + offsets
    return b'MHGP11SP' + struct.pack('<16Q', *words) + b''.join(sections)


def cases():
    triangle = [(0, 0, 0), (1, 1, 0), (1, 0, 1)]
    square = [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]
    return [
        ('triangle_mst_two_supports', fixture(triangle, [(0, 1), (0, 2)]), True),
        ('triangle_cycle_three_supports', fixture(triangle, [(0, 1), (0, 2), (1, 2)]), False),
        ('square_disconnected_children', fixture(square, [(0, 1), (2, 3)]), False),
    ]


def outcomes(reader):
    rows = []
    for name, data, expected in cases():
        try:
            f = reader.read_supports(data, 21)
            verdict = {'accepted': True, 'decoded_balls': f.B, 'prior': list(f.prior)}
        except ValueError as error:
            verdict = {'accepted': False, 'reason': str(error)}
        rows.append(dict(case=name, expected_accept=expected, file_bytes=len(data),
                         file_sha256=hashlib.sha256(data).hexdigest(), **verdict))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bench', required=True, help='Dossier contenant mhgp11_formats.py et ses deux dependances')
    args = parser.parse_args()
    sys.path.insert(0, args.bench)
    import mhgp11_formats
    rows = outcomes(mhgp11_formats)
    print(json.dumps(rows, indent=2, sort_keys=True))
    ok = all(row['accepted'] == row['expected_accept'] for row in rows)
    if ok:
        print('supports_spanning_reader_verdict conforme cas3')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
