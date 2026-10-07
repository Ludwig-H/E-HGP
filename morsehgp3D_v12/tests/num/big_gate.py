#!/usr/bin/env python3
"""Porte mhgp12_num_big (tranche S8) : num::Big contre l'int de Python, operation par operation.

    python3 -S -B big_gate.py <mhgp12_num_big_probe>

Au moins 20 000 operations (plancher grave MIN_OPERATIONS) : addition, soustraction, multiplication, division par
defaut (divmod de Python), PGCD (math.gcd), racine entiere (math.isqrt), carre parfait, reste modulo un petit entier,
decalages, comparaison, longueur en bits. Tailles de 0 a la capacite (16384 + 1024 bits) ; cas de Knuth D (chiffre
de tete du diviseur 2^63 ou 2^64 - 1, estimation qhat = 2^64, ajout en retour), carres et voisins, motifs de mots
pleins. Refus a la capacite : toute operation dont le resultat exact depasse 17 408 bits, et toute entree plus large,
doit rendre « refused radical_sign_budget » ; un refus ailleurs est un desaccord.

Codes : 0 conforme ; 1 desaccord (premiers ecarts ecrits) ; 2 usage ; 3 plancher non atteint ou sonde en echec.
Ligne finale si conforme : num_big_verdict conforme operations=N refus_capacite=R knuth=K
Bibliotheque standard seule, aucun assert (tient sous python3 -O et -S).
"""
import math
import random
import subprocess
import sys

CAPACITY = 16384 + 1024
MIN_OPERATIONS = 20000
MIN_REFUSALS = 200
MIN_KNUTH = 2000
REFUSED = 'refused radical_sign_budget'


def hexs(v):
    return ('-' if v < 0 else '') + format(abs(v), 'x')


def sample(rng, kind=None):
    """Entier signe de taille variee jusqu'a la capacite."""
    kind = kind if kind is not None else rng.randrange(10)
    if kind == 0:
        bits = rng.randrange(0, 70)
    elif kind <= 3:
        bits = rng.randrange(0, 260)
    elif kind <= 5:
        bits = rng.randrange(0, 2200)
    elif kind <= 7:
        bits = rng.randrange(0, CAPACITY + 1)
    elif kind == 8:  # mots pleins, puissances de deux et voisins
        w = rng.randrange(1, 273)
        base = [(1 << (64 * w)) - 1, 1 << (64 * w - 1), (1 << (64 * (w - 1))) + 1][rng.randrange(3)]
        bits = None
        value = min(base, (1 << CAPACITY) - 1)
    else:
        bits = None
        value = (1 << rng.randrange(0, CAPACITY)) + rng.choice((-1, 0, 1))
    if bits is not None:
        value = rng.getrandbits(bits) if bits else 0
    return -value if rng.random() < 0.4 else value


def knuth_pair(rng):
    """Dividende et diviseur qui exercent l'estimation qhat de Knuth D (diviseur a au moins deux mots)."""
    n = rng.randrange(2, 60)
    top = rng.choice((1 << 63, (1 << 64) - 1, (1 << 63) + 1, rng.getrandbits(64) | (1 << 63), rng.getrandbits(40)))
    v = (top << (64 * (n - 1))) | rng.getrandbits(64 * (n - 1))
    if rng.random() < 0.3:
        v |= (1 << (64 * (n - 1))) - 1
    m = rng.randrange(n, min(272, n + 60))
    q = rng.getrandbits(64 * (m - n) + rng.randrange(1, 64))
    r = rng.randrange(v)
    if rng.random() < 0.3:
        r = v - 1
    u = q * v + r
    if u.bit_length() > CAPACITY:
        u >>= u.bit_length() - CAPACITY
    if rng.random() < 0.25:  # mots de tete egaux : qhat = 2^64 avant correction
        u = (v << (64 * rng.randrange(1, max(2, m - n)))) - rng.randrange(1, 1 << 64)
        u = min(abs(u), (1 << CAPACITY) - 1)
    signs = rng.randrange(4)
    return (-u if signs & 1 else u), (-v if signs & 2 else v)


def fits(v):
    return abs(v).bit_length() <= CAPACITY


def expected(op, a, b):
    if not fits(a) or (op in ('add', 'sub', 'mul', 'div', 'gcd', 'cmp') and not fits(b)):
        return REFUSED
    if op == 'add':
        r = a + b
    elif op == 'sub':
        r = a - b
    elif op == 'mul':
        r = a * b
    elif op == 'shl':
        r = a << b if a >= 0 else -((-a) << b)
    elif op == 'shr':
        r = abs(a) >> b
        r = -r if a < 0 else r
        return 'ok ' + hexs(r)
    elif op == 'div':
        q, r = divmod(a, b)
        return 'ok %s %s' % (hexs(q), hexs(r))
    elif op == 'gcd':
        return 'ok ' + hexs(math.gcd(a, b))
    elif op == 'isqrt':
        return 'ok ' + hexs(math.isqrt(a))
    elif op == 'square':
        if a < 0:
            return 'ok 0 0'
        s = math.isqrt(a)
        return 'ok %d %s' % (1 if s * s == a else 0, hexs(s))
    elif op == 'mod':
        return 'ok %d' % (a % b)
    elif op == 'cmp':
        return 'ok %d' % ((a > b) - (a < b))
    elif op == 'bits':
        return 'ok %d' % abs(a).bit_length()
    else:
        return None
    return 'ok ' + hexs(r) if fits(r) else REFUSED


def operations(seed=20261005):
    rng = random.Random(seed)
    ops, knuth = [], 0

    def push(op, a, b=None):
        ops.append((op, a, b))
    # Bords graves.
    edges = [0, 1, -1, 2, (1 << 64) - 1, 1 << 64, -(1 << 64), (1 << 128) - 1, (1 << CAPACITY) - 1,
             -((1 << CAPACITY) - 1), 1 << (CAPACITY - 1), 1 << CAPACITY, (1 << 8704) - 1]
    for a in edges:
        for b in edges:
            push('add', a, b)
            push('sub', a, b)
            push('mul', a, b)
            push('cmp', a, b)
            if b:
                push('div', a, b)
            push('gcd', a, b)
        if a >= 0:
            push('isqrt', a)
        push('square', a)
        push('bits', a)
        for k in (0, 1, 63, 64, 65, 128, CAPACITY - 1, CAPACITY):
            push('shl', a, k)
            push('shr', a, k)
    plan = (('add', 3000), ('sub', 3000), ('mul', 3000), ('div', 2000), ('knuth', 2500), ('gcd', 1500),
            ('isqrt', 1500), ('near_square', 1500), ('mod', 1000), ('shl', 700), ('shr', 500), ('cmp', 500))
    primes = (2, 3, 5, 7, 11, 13, 97, 173, 65521, 4294967291, 4294967295)
    for op, count in plan:
        for _ in range(count):
            if op == 'knuth':
                a, b = knuth_pair(rng)
                push('div', a, b)
                knuth += 1
            elif op == 'near_square':
                s = rng.getrandbits(rng.randrange(1, CAPACITY // 2 + 1))
                v = s * s + rng.choice((-1, 0, 1, 2 * s, 2 * s + 1))
                push('square', max(v, 0))
                push('isqrt', max(v, 0))
            elif op == 'isqrt':
                push('isqrt', abs(sample(rng)))
            elif op == 'mod':
                push('mod', sample(rng), rng.choice(primes))
            elif op in ('shl', 'shr'):
                push(op, sample(rng), rng.randrange(0, CAPACITY + 70))
            elif op == 'div':
                b = sample(rng)
                push('div', sample(rng), b if b else 1)
            elif op == 'mul' and rng.random() < 0.3:  # autour de la capacite
                x = rng.randrange(1, CAPACITY)
                a = rng.getrandbits(x) | (1 << (x - 1))
                y = CAPACITY - x + rng.choice((-1, 0, 1, 2))
                b = rng.getrandbits(max(y, 1)) | (1 << max(y - 1, 0))
                push('mul', a, b)
            elif op in ('add', 'sub') and rng.random() < 0.2:
                push(op, (1 << CAPACITY) - rng.randrange(1, 1 << 70), rng.randrange(0, 1 << 72))
            else:
                push(op, sample(rng), sample(rng))
    return ops, knuth


def main(argv):
    if len(argv) != 2:
        sys.stderr.write(__doc__)
        return 2
    ops, knuth = operations()
    lines = []
    for op, a, b in ops:
        lines.append('%s %s' % (op, hexs(a)) + ('' if b is None else (' %d' % b if op in ('mod', 'shl', 'shr')
                                                                         else ' ' + hexs(b))))
    try:
        run = subprocess.run([argv[1]], input='\n'.join(lines) + '\n', capture_output=True, text=True, timeout=600)
    except (OSError, subprocess.SubprocessError) as error:
        print('num_big_verdict sonde_impossible %s' % error)
        return 3
    if run.returncode != 0:
        print('num_big_verdict sonde_code=%d' % run.returncode)
        return 3
    out = run.stdout.splitlines()
    if not out or out[0] != 'capacity %d' % CAPACITY or len(out) != len(ops) + 1:
        print('num_big_verdict sortie_inattendue lignes=%d attendues=%d' % (len(out), len(ops) + 1))
        return 3
    gaps, refusals = [], 0
    for (op, a, b), got in zip(ops, out[1:]):
        want = expected(op, a, b)
        refusals += want == REFUSED
        if got != want:
            gaps.append((op, a.bit_length() if isinstance(a, int) else a, got[:80], (want or '')[:80]))
    for gap in gaps[:10]:
        print('ecart op=%s bits_a=%s obtenu=%s attendu=%s' % gap)
    if gaps:
        print('num_big_verdict desaccord ecarts=%d operations=%d' % (len(gaps), len(ops)))
        return 1
    if len(ops) < MIN_OPERATIONS or refusals < MIN_REFUSALS or knuth < MIN_KNUTH:
        print('num_big_verdict plancher operations=%d refus=%d knuth=%d' % (len(ops), refusals, knuth))
        return 3
    print('num_big_verdict conforme operations=%d refus_capacite=%d knuth=%d' % (len(ops), refusals, knuth))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
