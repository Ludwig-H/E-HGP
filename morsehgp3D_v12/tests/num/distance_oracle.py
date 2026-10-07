"""Distance oracle: Fraction polarization, old q1 and independent Wide; no native call in selftest."""
from fractions import Fraction
import hashlib
import itertools
import json
import random
import re
import subprocess
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def squared(pair):
    a, b = pair
    # Polarization over rationals: no fixed-width differences or native coefficients.
    value = sum((Fraction(x)*x + Fraction(y)*y - 2*Fraction(x)*y for x, y in zip(a, b)), Fraction())
    require(value.denominator == 1, 'integral distance')
    return value.numerator


def cases(bits):
    m = (1 << bits)-1
    corners = list(itertools.product((0, m), repeat=3))
    pairs = list(itertools.product(corners, repeat=2))
    for axis in range(3):
        for step in (0, 1, 2, 32767, 32768, 65535, 65536, m//2, m-1, m):
            b, near = [0, 0, 0], [m, m, m]
            b[axis], near[axis] = step, m-step
            pairs.extend((((0, 0, 0), tuple(b)), (tuple(b), (0, 0, 0)), ((m, m, m), tuple(near))))
    rng = random.Random(20261003)
    for index in range(256):
        width = m+1 if index % 2 else 32
        a, b = (tuple(rng.randrange(width) for _ in range(3)) for _ in range(2))
        pairs.extend(((a, b), (b, a), (a, a), ((a[2], a[0], a[1]), (b[2], b[0], b[1]))))
    require(len(pairs) == 1178, 'distance fixture inventory')
    return pairs


def encode(pair):
    return ' '.join(map(str, pair[0]+pair[1]))+'\n'


def model_line(pair, bits):
    value = squared(pair)
    return 'ok %d %0*x %064x' % (value, 48, value, value)


def validate(lines, pairs, bits):
    require(lines and lines[0] == 'bits %d' % bits, 'profile header')
    require(len(lines) == len(pairs)+1, 'one result per request')
    width = 48
    pattern = re.compile(r'ok ([0-9]{1,15}) ([0-9a-f]{%d}) ([0-9a-f]{64})' % width)
    checks = 2
    for line, pair in zip(lines[1:], pairs):
        match = pattern.fullmatch(line)
        require(match is not None, 'bounded exact distance fields')
        decimal, old, wide = match.groups()
        native = int(decimal)
        require(str(native) == decimal, 'canonical native decimal')
        require(0 <= native < (1 << 50), 'native distance budget')
        expected = squared(pair)
        require(native == expected, 'native distance versus Fraction')
        require(int(old, 16) == expected, 'old q1 versus Fraction')
        require(int(wide, 16) == expected, 'Wide q1 versus Fraction')
        require((native == 0) == (pair[0] == pair[1]), 'identity of indiscernibles')
        checks += 7
    return checks


def facts(bits):
    m = (1 << bits)-1
    require(squared(((0, 0, 0), (m, m, m))) == 3*m*m, 'opposite corners')
    require(squared(((m, m, m), (m-1, m, m))) == 1, 'last-bit negative difference')
    require(squared(((1, 2, 3), (4, 6, 3))) == 25, '3-4-5 triangle')
    require(squared(((m-3, m-4, m), (m, m, m))) == 25, 'translated triangle')
    require((3*m*m).bit_length() == 2*bits+2, 'maximal distance bit length')
    return 5


def model(bits):
    pairs = cases(bits)
    lines = ['bits %d' % bits]+[model_line(pair, bits) for pair in pairs]
    checks = validate(lines, pairs, bits)+facts(bits)
    # Every numeric channel is judged independently, including a coordinated corruption of all three.
    index = 2
    mutations = []
    for field in (1, 2, 3):
        parts = lines[index].split()
        parts[field] = str(int(parts[field])+1) if field == 1 else format(int(parts[field], 16)+1, '0%dx' % len(parts[field]))
        mutations.append((index, ' '.join(parts)))
    parts = lines[index].split()
    bad_value = int(parts[1])+1
    mutations.append((index, 'ok %d %0*x %064x' % (bad_value, 48, bad_value, bad_value)))
    mutations.extend(((0, 'bits 19'), (index, lines[index]+' extra'), (index, 'ok 0'),
                      (index, lines[index].replace('ok ', 'ok -', 1)), (index, ' '),
                      (index, lines[index].replace('ok ', 'ok 0', 1)),
                      (index, 'ok %d %032x %064x' % (1 << 50, 0, 0))))
    corrupted = []
    for at, line in mutations:
        bad = list(lines)
        bad[at] = line
        corrupted.append(bad)
    corrupted.extend((lines[:-1], lines+[lines[-1]]))
    for bad in corrupted:
        try:
            validate(bad, pairs, bits)
        except ValueError:
            continue
        raise ValueError('distance corruption accepted')
    return dict(bits=bits, requests=len(pairs), checks=checks, corruptions=len(corrupted),
                input_sha256=hashlib.sha256(''.join(map(encode, pairs)).encode()).hexdigest())


def run(exe):
    header = subprocess.run([exe], input='', capture_output=True, text=True, timeout=10, check=False)
    require(header.returncode == 0 and not header.stderr and header.stdout in ('bits 21\n', 'bits 24\n'),
            'native profile probe')
    bits = int(header.stdout.split()[1])
    pairs = cases(bits)
    payload = ''.join(map(encode, pairs))
    result = subprocess.run([exe], input=payload, capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0 and not result.stderr, 'native distance success')
    checks = validate(result.stdout.splitlines(), pairs, bits)+facts(bits)
    refusals = []
    for axis in range(6):
        for value in (-1, 1 << bits):
            coordinates = [0]*6
            coordinates[axis] = value
            refusals.append((' '.join(map(str, coordinates))+'\n', 'coordinate_out_of_domain'))
    refusals.extend((('0 0\n', 'input_unreadable'), ('x 0 0 0 0 0\n', 'input_unreadable'),
                     ('9223372036854775808 0 0 0 0 0\n', 'input_unreadable')))
    for text, reason in refusals:
        refused = subprocess.run([exe], input=text, capture_output=True, text=True, timeout=10, check=False)
        require(refused.returncode == 2 and not refused.stderr and
                refused.stdout == 'bits %d\nrefused %s\n' % (bits, reason), 'native domain/input refusal')
        checks += 1
    print(json.dumps(dict(bits=bits, requests=len(pairs), checks=checks, refusals=len(refusals),
                          input_sha256=hashlib.sha256(payload.encode()).hexdigest()), sort_keys=True))


def main():
    if sys.argv[1:] == ['--selftest']:
        print(json.dumps(dict(profiles=[model(bits) for bits in (21, 24)], native_calls=0), sort_keys=True))
        return
    require(len(sys.argv) == 2, 'one probe path or --selftest')
    run(sys.argv[1])


if __name__ == '__main__':
    main()
