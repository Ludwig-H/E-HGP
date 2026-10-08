#!/usr/bin/env python3
"""Patch isolé et modèle de vivacité des tampons radix ; aucun moteur natif."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def require(ok, why):
    if not ok:
        raise ValueError(why)


class Scratch:
    """Capacités abstraites persistantes ; ne simule pas l'allocateur CUDA."""
    def __init__(self):
        self.keys = [[], []]
        self.vals = [[], []]

    def load(self, keys, vals):
        n = len(keys)
        for buf in self.keys + self.vals:
            buf.extend([None] * max(0, n - len(buf)))
        self.keys[0][:n] = keys
        self.vals[0][:n] = vals

    def ordered(self, n, width):
        # La diversité de chaque octet caractérise les passes non identiques,
        # indépendamment de la réduction ET/OU des noyaux de la source.
        active = [d for d in range(width)
                  if len({(k >> (8*d)) & 255 for k in self.keys[0][:n]}) > 1]
        cur = 0
        for d in active:
            buckets = [[] for _ in range(256)]
            for key, val in zip(self.keys[cur][:n], self.vals[cur][:n]):
                buckets[(key >> (8*d)) & 255].append((key, val))
            pairs = [pair for bucket in buckets for pair in bucket]
            self.keys[cur ^ 1][:n] = [k for k, _ in pairs]
            self.vals[cur ^ 1][:n] = [v for _, v in pairs]
            cur ^= 1
        return cur, len(active)


def call(shared, positions, f3, support, abort=None):
    """positions puis F3 stable ; table S* indexée par rang canonique."""
    n = len(positions)
    original = (positions[:], f3[:], support[:])
    shared.load(positions, list(range(n)))
    cp, np = shared.ordered(n, 16)
    by_position = shared.vals[cp][:n]
    require(by_position == sorted(range(n), key=lambda i: positions[i]), 'positions')
    keys = Scratch()
    keys.load([f3[i] for i in by_position], by_position)
    ck, _ = keys.ordered(n, 8)
    canonical = keys.vals[ck][:n]
    require(canonical == sorted(range(n), key=lambda i: (f3[i], positions[i])), 'F3 order')
    # Fin de lecture de shared : seuls keys et les données séparées restent utiles
    # à check/repli/emit. Un refus entre phases n'impose aucune restauration ici.
    if abort == 'before_table':
        return None
    table_keys = [support[i] for i in canonical]
    shared.load(table_keys, list(range(n)))
    ct, nt = shared.ordered(n, 16)
    table = shared.vals[ct][:n]  # copie, comme take_cast, aucune vue sur le tampon
    expected = sorted(range(n), key=lambda i: table_keys[i])
    require(table == expected, 'table values')
    # Comparaison à une table distincte (ancien agencement), plus oracle sort Python.
    dedicated = Scratch()
    dedicated.load(table_keys, list(range(n)))
    fresh_ct, _ = dedicated.ordered(n, 16)
    require(table == dedicated.vals[fresh_ct][:n], 'old/new mismatch')
    require((positions, f3, support) == original, 'immutable input overwritten')
    require(canonical == keys.vals[ck][:n], 'final order aliased')
    if abort == 'after_table':
        return None
    return {'cp': cp, 'ct': ct, 'passes_positions': np, 'passes_table': nt,
            'canonical': canonical, 'table': table}


def model():
    shared = Scratch()
    witness = call(shared, [0, 1], [0, 0], [256, 0])
    require(witness['ct'] == 1 and witness['table'] == [1, 0], 'ct=1 witness')
    # Combinaisons des parités cp/ct, y compris un octet commun à toutes les clés.
    cases = [([0, 0], [0, 0], [0, 0]), ([0, 1], [0, 0], [0, 0]),
             ([0, 0], [0, 0], [0, 1]), ([0, 1], [0, 0], [256, 0]),
             ([], [], []), ([7], [9], [11])]
    rng = random.Random(20261008)
    sizes = [0, 1, 2, 17, 31, 32, 33, 63, 129, 1024, 1025]
    for j in range(220):
        n = sizes[j % len(sizes)]
        # Variantes de nombres d'octets actifs et clés communes ; certaines égalités
        # éprouvent la stabilité. Aucune hypothèse géométrique sur ces clés abstraites.
        keys = lambda bits: [rng.getrandbits(bits) << 8 for _ in range(n)]
        cases.append((keys((j % 5)*8), keys(16), keys(((j+2) % 5)*8)))
    retained = []
    combinations = set()
    interrupted = 0
    for i, (p, f, s) in enumerate(cases):
        saved = [(x['canonical'][:], x['table'][:]) for x in retained]
        if i % 5 == 0:
            require(call(shared, p, f, s, 'before_table') is None, 'early abort')
            require(call(shared, p, f, s, 'after_table') is None, 'late abort')
            interrupted += 2
        out = call(shared, p, f, s)
        combinations.add((out['cp'], out['ct']))
        for earlier, old in zip(retained, saved):
            require((earlier['canonical'], earlier['table']) == old, 'earlier output overwritten')
        retained.append(out)
    require(combinations == {(0, 0), (0, 1), (1, 0), (1, 1)}, 'parity coverage')
    demands = {}
    for n in (0, 1, 1024, 1025, 1000000):
        t = max(1, (n+1023)//1024)
        fields = 2*16*n + 2*4*n + 4*256*t + 4*256 + 2*16*t + 2*16
        formula = 40*n + 1056*t + 1056
        require(fields == formula, 'radix requested bytes')
        demands[str(n)] = formula
    return {'completed_calls': len(cases)+1, 'interrupted_between_phases': interrupted,
            'cp_ct': [list(x) for x in sorted(combinations)], 'ct1_witness': witness,
            'logical_requested_bytes': demands, 'native_execution': False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    pins = json.loads((HERE/'pins.json').read_text())
    spec = pins['patch']
    before = subprocess.check_output(['git', 'show', pins['base_commit']+':'+spec['path']], cwd=args.repo)
    sha = lambda b: hashlib.sha256(b).hexdigest()
    require(sha(before) == spec['before_sha256'], 'source hash')
    with tempfile.TemporaryDirectory(prefix='radix-reuse-') as tmp:
        root = Path(tmp)
        target = root/spec['path']
        target.parent.mkdir(parents=True)
        target.write_bytes(before)
        for options in (['--check'], []):
            subprocess.run(['git', 'apply', *options, str(HERE/'proposition.patch')],
                           cwd=root, check=True, capture_output=True)
        after = target.read_bytes()
        require(sha(after) == spec['after_sha256'], 'patched hash')
        subprocess.run(['git', 'apply', '--reverse', '--check', str(HERE/'proposition.patch')],
                       cwd=root, check=True, capture_output=True)
    out = model()
    out.update({'patch_apply_check': True, 'patch_reverse_check': True,
                'patched_sha256': spec['after_sha256']})
    print(json.dumps(out, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
