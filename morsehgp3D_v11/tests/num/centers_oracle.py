"""Gram/Fraction oracle for lexicographic global centers, independent of native N/D products."""
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'catalogue'))
from fraction_model import circumsphere, require


def encode(pair):
    a, b = pair
    slots = a+((0, 0, 0),)*(4-len(a))+b+((0, 0, 0),)*(4-len(b))
    return ' '.join(map(str, (len(a), len(b), *(v for p in slots for v in p))))+'\n'


def expected(pair):
    a, b = (circumsphere(p) for p in pair)
    if a is None or b is None:
        return 'degenerate'
    return 'ok %d' % ((a[0] > b[0])-(a[0] < b[0]))


def cases(bits):
    m = (1 << bits)-1
    supports = [((0,0,0),), ((2,2,0),), ((2,2,2),), ((m,m,m),),
                ((0,0,0),(4,4,0)), ((4,4,0),(0,0,0)),
                ((0,0,0),(4,0,0),(0,4,0)), ((0,4,0),(4,0,0),(0,0,0)),
                ((0,0,0),(4,4,0),(4,0,4),(0,4,4)),
                ((0,0,0),(m,1,0),(m-1,1,0)), ((m-1,0,0),(m,0,0)),
                ((0,0,0),(0,0,0)), ((0,0,0),(1,0,0),(2,0,0)),
                ((0,0,0),(2,0,0),(0,2,0),(2,2,0))]
    result = list(itertools.product(supports, repeat=2))
    rng = random.Random(20261003)
    for i in range(240):
        width = m+1 if i % 3 == 0 else 32
        a, b = [tuple(tuple(rng.randrange(width) for _ in range(3)) for _ in range(rng.randrange(1,5)))
                for _ in range(2)]
        result.extend(((a,b),(b,a),(a,tuple(reversed(a)))))
    return result


def validate(lines, pairs, bits):
    require(lines[0] == 'bits %d' % bits, 'profile header')
    require(len(lines) == len(pairs)+1, 'one result per comparison')
    checks = 2
    for line, pair in zip(lines[1:], pairs):
        require(line == expected(pair), 'global center order')
        checks += 1
    return checks


def model(bits):
    pairs = cases(bits)
    lines = ['bits %d' % bits]+[expected(p) for p in pairs]
    checks = validate(lines, pairs, bits)
    # Fixed geometric facts are not inferred from the emitted expected table.
    require(expected((pairs[0][0], ((1,0,0),))) == 'ok -1', 'origin before positive x')
    require(circumsphere(((0,0,0),(4,0,0),(0,4,0)))[0] == (2,2,0), 'right triangle center')
    m = (1 << bits)-1
    outside = circumsphere(((0,0,0),(m,1,0),(m-1,1,0)))[0]
    require(outside[1] < 0 and outside[0]*2 == 2*m-1, 'outside center and half-coordinate')
    corruptions = 0
    for wanted in ('ok -1', 'ok 0', 'ok 1', 'degenerate'):
        index = lines.index(wanted)
        bad = list(lines); bad[index] = 'ok 1' if wanted != 'ok 1' else 'ok -1'
        try:
            validate(bad, pairs, bits)
        except ValueError:
            corruptions += 1
        else:
            raise ValueError('mutated sign accepted')
    require(corruptions == 4, 'all sign/degeneracy corruptions detected')
    return dict(bits=bits, cases=len(pairs), checks=checks+4, corruptions=corruptions,
                input_sha256=hashlib.sha256(''.join(map(encode,pairs)).encode()).hexdigest())


def main():
    if sys.argv[1:] == ['--selftest']:
        print(json.dumps(dict(profiles=[model(b) for b in (18,21,24)], native_calls=0), sort_keys=True))
        return
    require(len(sys.argv) == 2, 'one probe path or --selftest')
    exe = sys.argv[1]
    header = subprocess.run([exe], input='', capture_output=True, text=True, timeout=10, check=False)
    require(header.returncode == 0 and not header.stderr and header.stdout in ('bits 18\n','bits 21\n','bits 24\n'),
            'native profile probe')
    bits = int(header.stdout.split()[1]); pairs = cases(bits)
    result = subprocess.run([exe], input=''.join(map(encode,pairs)), capture_output=True, text=True,
                            timeout=30, check=False)
    require(result.returncode == 0 and not result.stderr, 'native comparison success')
    checks = validate(result.stdout.splitlines(), pairs, bits)
    samples = [('0 1 '+'0 '*24+'\n', 2, 'parameter_out_of_range'),
               ('1 5 '+'0 '*24+'\n', 2, 'parameter_out_of_range'),
               ('1 1 -1 '+'0 '*23+'\n', 2, 'coordinate_out_of_domain'),
               ('1 1 %d ' % (1 << bits)+'0 '*23+'\n', 2, 'coordinate_out_of_domain'),
               ('1 1 0\n', 2, 'input_unreadable'), ('1\n', 2, None)]
    for payload, code, reason in samples:
        run = subprocess.run([exe], input=payload, capture_output=True, text=True, timeout=10, check=False)
        require(run.returncode == code and not run.stderr, 'native refusal not signal')
        if reason:
            require(run.stdout.splitlines() == ['bits %d' % bits, 'refused '+reason], 'refusal identity')
    print(json.dumps(dict(bits=bits, cases=len(pairs), checks=checks, refusals=len(samples),
                         input_sha256=hashlib.sha256(''.join(map(encode,pairs)).encode()).hexdigest()), sort_keys=True))


if __name__ == '__main__':
    main()
