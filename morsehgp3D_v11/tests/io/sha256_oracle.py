"""Differentiel du SHA-256 du module io contre hashlib (porte mhgp11_io_sha256).

    python3 sha256_oracle.py <mhgp11_io_sha256_probe>

Messages : toutes les longueurs de 0 a 200 octets (les frontieres de remplissage 55, 56, 63, 64 et 65 y sont), puis
des multiblocs (1000, 4095 a 4097, 65539 et un million d'octets). Chaque message est absorbe par la sonde en une
fois et par morceaux de 1, 7, 63, 64 et 65 octets ; chaque empreinte est comparee a hashlib.sha256. Octets
pseudo-aleatoires d'un generateur grave dans ce fichier (aucune dependance au module random).
Codes : 0 conforme, 1 desaccord, 3 plancher non atteint. Python 3.10 nu, aucun assert.
"""
import hashlib
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'support'))
import mhgp11_gate  # noqa: E402

CHUNKS = (0, 1, 7, 63, 64, 65)
LENGTHS = tuple(range(201)) + (1000, 4095, 4096, 4097, 65539, 1000000)
FLOOR = len(CHUNKS) * len(LENGTHS)


def message(length, seed):
    """Octets d'un generateur congruentiel 32 bits grave (meme suite sur toute plate-forme)."""
    state = (20261004 + 7919 * seed) & 0xFFFFFFFF
    out = bytearray(length)
    for i in range(length):
        state = (state * 1664525 + 1013904223) & 0xFFFFFFFF
        out[i] = state >> 24
    return bytes(out)


def main():
    if len(sys.argv) != 2:
        print('usage : sha256_oracle.py <sonde>')
        return mhgp11_gate.REFUSAL
    gate = mhgp11_gate.Gate('mhgp11_io_sha256')
    cases = []
    for seed, length in enumerate(LENGTHS):
        data = message(length, seed)
        for chunk in CHUNKS:
            cases.append((chunk, data))
    lines = ''.join('%d %s\n' % (chunk, data.hex() if data else '-') for chunk, data in cases)
    result = mhgp11_gate.run([sys.argv[1]], timeout=240, stdin=lines)
    gate.check_eq(result.code, 0, 'code de la sonde')
    got = result.stdout.split('\n')[:-1] if result.stdout else []
    gate.check_eq(len(got), len(cases), 'nombre de reponses')
    for (chunk, data), digest in zip(cases, got):
        gate.check_eq(digest, hashlib.sha256(data).hexdigest(), 'longueur %d, morceaux de %d' % (len(data), chunk))
    print('io_sha256 comparaisons=%d' % len(got))
    return gate.finish(floor=FLOOR + 2)


if __name__ == '__main__':
    sys.exit(main())
