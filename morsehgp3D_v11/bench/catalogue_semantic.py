"""Strict, bounded-memory decoder of MHGP11CAT1; profile-independent exact semantic digest.

The geometry remains in source integer units. Only the compiled profile and integer limb padding are
removed; rational levels are reduced. This is an encoding comparison, not an independent geometry oracle.
"""
import hashlib
import math
import mmap
from pathlib import Path
import struct


SCHEMA = 'ehgp.v11.catalogue_semantic.v1'
NONE = 2**32 - 1
LIMIT = 8 * 1024**3
WORD = struct.Struct('<Q')
BALL = struct.Struct('<8Q')


def need(value, message):
    if not value:
        raise ValueError(message)


def natural(value):
    payload = value.to_bytes(max(1, (value.bit_length() + 7) // 8), 'little')
    return WORD.pack(len(payload)) + payload


def decode(data, expected_bits, expected_k, expected_count):
    size, cursor = len(data), 10
    need(34 <= size <= LIMIT and data[:10] == b'MHGP11CAT1', 'canonical signature/size')

    def words(count=1):
        nonlocal cursor
        need(count >= 0 and cursor + count * 8 <= size, 'canonical truncated words')
        result = struct.unpack_from('<%dQ' % count, data, cursor)
        cursor += count * 8
        return result

    bits, kmax, sites = words(3)
    need(bits in (18, 21, 24) and bits == expected_bits and kmax == expected_k and
         1 <= kmax <= 12 and sites == expected_count and 0 < sites < NONE, 'canonical header')
    need(sites <= (size - cursor) // 40, 'canonical site count')
    start = cursor
    previous = -1
    identities = set()
    for _ in range(sites):
        x, y, z, weight, identity = words(5)
        need(max(x, y, z) < 2**bits and weight == 1 and identity <= NONE, 'canonical site domain')
        need(identity not in identities, 'canonical duplicate point ID')
        identities.add(identity)
        # Same Morton integer for all profiles; leading zero bits never change the order.
        key = sum(((point >> bit) & 1) << (3 * bit + axis)
                  for axis, point in enumerate((x, y, z)) for bit in range(bits))
        need(key > previous, 'canonical site order/duplicate')
        previous = key
    h = hashlib.sha256(SCHEMA.encode() + b'\0' + WORD.pack(kmax) + WORD.pack(sites))
    for begin in range(start, cursor, 1 << 20):
        h.update(data[begin:min(cursor, begin + (1 << 20))])
    level_count, = words()
    need(1 <= level_count <= NONE and level_count <= (size - cursor) // 48, 'canonical level count')
    h.update(WORD.pack(level_count))
    budgets = (8 * bits + 12, 6 * bits + 8)
    previous_level = None
    for index in range(level_count):
        values = []
        for budget in budgets:
            negative, limbs = words(2)
            expected_limbs = 2 if budget <= 127 else (budget + 63) // 64
            need(negative == 0 and limbs == expected_limbs, 'canonical integer header')
            value = sum(word << (64 * i) for i, word in enumerate(words(limbs)))
            need(value < 2**budget, 'canonical integer budget')
            values.append(value)
        numerator, denominator = values
        need(denominator > 0 and (numerator == 0) is (index == 0), 'canonical level sign/zero')
        divisor = math.gcd(numerator, denominator)
        level = numerator // divisor, denominator // divisor
        need(previous_level is None or previous_level[0] * level[1] < level[0] * previous_level[1],
             'canonical levels not strictly increasing')
        previous_level = level
        h.update(natural(level[0]) + natural(level[1]))
    tail = cursor
    balls, = words()
    need(balls < NONE and balls <= (size - cursor) // 72, 'canonical ball count')
    ball_start = cursor
    previous_ball = (0, ())
    incidences = 0
    for _ in range(balls):
        q, p, m, rank, *support = words(8)
        need(2 <= q <= 4 and p + q <= kmax + 1 and q <= m <= sites and
             1 <= rank < level_count, 'canonical ball fields')
        active = support[:q]
        need(active == sorted(set(active)) and active[-1] < sites and support[q:] == [NONE] * (4 - q),
             'canonical support')
        current = rank, tuple(support)
        need(current > previous_ball and rank <= previous_ball[0] + 1, 'canonical ball order/rank')
        previous_ball = current
        incidences += p + m
    need(previous_ball[0] == level_count - 1, 'canonical unused level')
    offsets_start = cursor
    need(cursor + (balls + 1 + incidences) * 8 == size, 'canonical size/trailing bytes')
    offset, = words()
    need(offset == 0, 'canonical initial offset')
    for index in range(balls):
        _, p, m, *_ = BALL.unpack_from(data, ball_start + 64 * index)
        after, = words()
        need(after == offset + p + m, 'canonical population offset')
        offset = after
    for index in range(balls):
        q, p, m, _, *support = BALL.unpack_from(data, ball_start + 64 * index)
        inner = set()
        previous = -1
        for _ in range(p):
            site, = words()
            need(previous < site < sites, 'canonical interior order/domain')
            previous = site
            inner.add(site)
        previous, found = -1, set()
        for _ in range(m):
            site, = words()
            need(previous < site < sites and site not in inner, 'canonical shell order/domain/disjointness')
            previous = site
            if site in support[:q]:
                found.add(site)
        need(len(found) == q, 'canonical support outside shell')
    need(cursor == size and offsets_start >= ball_start, 'canonical EOF')
    for begin in range(tail, size, 1 << 20):
        h.update(data[begin:min(size, begin + (1 << 20))])
    return {'schema': SCHEMA, 'sha256': h.hexdigest(), 'coord_bits': bits, 'kmax': kmax,
            'sites': sites, 'levels': level_count, 'balls': balls, 'incidences': incidences}


def inspect(path, bits, kmax, count):
    path = Path(path)
    need(34 <= path.stat().st_size <= LIMIT, 'canonical file size')
    with path.open('rb') as source, mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
        return decode(data, bits, kmax, count)
