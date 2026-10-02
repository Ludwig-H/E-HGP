#!/usr/bin/env python3
"""Independent small tetrahedron encodings and corruptions; no native execution."""
from fractions import Fraction
import hashlib
from itertools import combinations
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'bench'))
import catalogue_semantic as semantic


CHECKS = 0
IDS = (4294967294, 17, 4000000000, 81)


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


def tetra(maximum):
    return ((0, 0, 0), (maximum, maximum, 0), (maximum, 0, maximum), (0, maximum, maximum))


def fixture(bits, maximum=7, kmax=5, factor=1):
    """All six edges, four acute faces and one strict tetrahedron; no other sites exist."""
    word = lambda value: struct.pack('<Q', value)
    result = bytearray(b'MHGP11CAT1' + word(bits) + word(kmax) + word(4))
    positions = {}
    for point, identity in zip(tetra(maximum), IDS):
        result.extend(b''.join(word(value) for value in (*point, 1, identity)))
    result.extend(word(4))
    levels = (Fraction(0), Fraction(maximum**2, 2), Fraction(2 * maximum**2, 3), Fraction(3 * maximum**2, 4))
    for i, level in enumerate(levels):
        positions['level%d' % i] = len(result)
        for value, budget in zip((factor * level.numerator, factor * level.denominator),
                                 (8 * bits + 12, 6 * bits + 8)):
            limbs = 2 if budget <= 127 else (budget + 63) // 64
            result.extend(word(0) + word(limbs))
            result.extend(b''.join(word((value >> (64 * n)) & (2**64 - 1)) for n in range(limbs)))
    supports = [s for q in (2, 3, 4) for s in combinations(range(4), q)]
    result.extend(word(len(supports)))
    positions['balls'] = len(result)
    for support in supports:
        result.extend(b''.join(word(value) for value in
                               (len(support), 0, len(support), len(support) - 1,
                                *support, *([semantic.NONE] * (4 - len(support))))))
    offset = 0
    positions['offsets'] = len(result)
    result.extend(word(0))
    for support in supports:
        offset += len(support)
        result.extend(word(offset))
    positions['population'] = len(result)
    result.extend(b''.join(word(site) for support in supports for site in support))
    return bytes(result), positions


def main():
    hashes, raw = set(), set()
    for bits in (18, 21, 24):
        for factor in (1, 2):
            data, _ = fixture(bits, factor=factor)
            decoded = semantic.decode(data, bits, 5, 4)
            need(decoded['balls'] == 11 and decoded['levels'] == 4 and decoded['incidences'] == 28,
                 'independent tetrahedron counts')
            detailed = semantic.decode(data, bits, 5, 4, arity_counts=True)
            need(detailed.pop('qmin_counts') == {'2': 6, '3': 4, '4': 1}, 'arity counts from canonical ball records')
            need(detailed == decoded, 'arity metadata changes historical semantic digest/shape')
            hashes.add(decoded['sha256'])
            raw.add(hashlib.sha256(data).hexdigest())
    need(len(hashes) == 1 and len(raw) == 6, 'profile/padding/rational representation independence')
    for bits, maximum in ((21, 2**21 - 1), (24, 2**24 - 1)):
        data, _ = fixture(bits, maximum)
        need(semantic.decode(data, bits, 5, 4)['balls'] == 11, 'high bits exercised')
    data, positions = fixture(18)
    corruptions = [('truncated', data[:-1]), ('trailing', data + bytes(8)), ('signature', b'X' + data[1:])]

    def changed(name, position, value):
        altered = bytearray(data)
        struct.pack_into('<Q', altered, position, value)
        corruptions.append((name, bytes(altered)))

    changed('profile', 10, 21)
    changed('K', 18, 10)
    changed('count', 26, 5)
    changed('coordinate', 34, 2**18)
    changed('weight', 34 + 24, 2)
    changed('duplicate_id', 34 + 40 + 32, IDS[0])
    changed('negative', positions['level1'], 1)
    changed('limb_count', positions['level1'] + 8, 64)
    changed('zero_denominator', positions['level1'] + 40 + 16, 0)
    changed('unordered_level', positions['level1'] + 16, 0)
    changed('support', positions['balls'] + 40, 0)
    changed('rank', positions['balls'] + 24, 0)
    changed('offset', positions['offsets'] + 8, 3)
    changed('population', positions['population'], 4)
    for name, payload in corruptions:
        try:
            semantic.decode(payload, 18, 5, 4)
        except ValueError:
            need(True, name + ' refused')
        else:
            raise ValueError(name + ' accepted')
    # A different valid catalogue encoding must remain distinguishable, even if its counts match.
    other, _ = fixture(18, maximum=8)
    need(semantic.decode(other, 18, 5, 4)['sha256'] not in hashes, 'geometry change hidden')
    need(len(corruptions) == 17 and CHECKS >= 27, 'semantic test floor')
    print('checks=%d' % CHECKS)
    print('catalogue_semantic_verdict conforme fixtures8 corruptions17 native0')


if __name__ == '__main__':
    main()
