#!/usr/bin/env python3
"""Preuve combinatoire et application des propositions sur copies ; aucun moteur."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bits(mask):
    while mask:
        low = mask & -mask
        yield low.bit_length() - 1
        mask ^= low


def above(n, x):
    # Representation un mot de 32 bits / quatre mots de 64 bits.
    width = 32 if n == 32 else 64
    words = []
    for lo in range(0, n, width):
        cut = min(width, max(0, x + 1 - lo))
        words.append((((1 << width) - 1) << cut) & ((1 << width) - 1))
    return sum(w << (i * width) for i, w in enumerate(words))


def rows(m, relations):
    # Oracle ensembles, sans preparation symetrique des lignes live.
    pairs = dict(zip(itertools.combinations(range(m), 2), relations))
    dom = [set() for _ in range(m)]
    nbr = [set() for _ in range(m)]
    for x in range(m):
        for y in range(m):
            if x == y:
                continue
            i, j = sorted((x, y))
            r = pairs[i, j]
            if r == 0:
                nbr[x].add(y)
            elif (r == 1 and x == i) or (r == 2 and x == j):
                dom[x].add(y)
    return dom, nbr


def oracle(n, k, dom, nbr):
    m = len(dom)
    return [[sum(1 << y for y in nbr[x] if len(dom[x] | dom[y]) <= k - 1 - t)
             if x < m else 0 for x in range(n)] for t in range(3)]


def proposed(n, k, dom, nbr, mutation=''):
    m = len(dom)
    d = [sum(1 << y for y in row) for row in dom]
    b = [sum(1 << y for y in row) for row in nbr]
    # Tout le stockage a ete empoisonne, y compris les lignes inactives.
    live = [[(1 << n) - 1 for _ in range(n)] for _ in range(3)]
    for x in range(m if mutation == 'dead_rows' else n):
        for t in range(2 if mutation == 'third_row' else 3):
            live[t][x] = 0
    calls = 0
    for x in range(m):
        for y in bits(b[x] if mutation == 'double_visit' else b[x] & above(n, x)):
            calls += 1
            w = (d[x] | d[y]).bit_count()
            for t in range(3):
                limit = k - 1 - t + (mutation == 'threshold')
                if mutation == 'unsigned_threshold':
                    limit %= 1 << 32
                if w <= limit:
                    live[t][x] |= 1 << (y % 32 if mutation == 'bit32' else y)
                    if mutation != 'one_endpoint':
                        live[t][y] |= 1 << (x % 32 if mutation == 'bit32' else x)
    return live, calls


def queue(n, k, dom, nbr, live, mutation=''):
    # count_items/next_item Q2, prefixe par ligne puis file de 512 items.
    width = 5 if n == 32 or mutation == 'pack32' else 8
    mask = (1 << width) - 1
    packed = []
    for i, row in enumerate(dom):
        if len(row) > k - 1:
            continue
        next1 = live[1 if mutation == 'live1' else 0][i] & above(n, i)
        if mutation == 'no_live':
            next1 = sum(1 << j for j in nbr[i] if j > i)
        for j in bits(next1):
            packed.append(i | (j << width))
    return [(v & mask, (v >> width) & mask)
            for base in range(0, len(packed), 512) for v in packed[base:base + 512]]


def verify(n, m, k, relations, mutation=''):
    dom, nbr = rows(m, relations)
    expected = oracle(n, k, dom, nbr)
    live, calls = proposed(n, k, dom, nbr, mutation)
    edges = sum(map(len, nbr)) // 2
    need(live == expected, 'masques live')
    need(calls == edges, 'travail physique une union/arete')
    pairs = queue(n, k, dom, nbr, live, mutation)
    wanted = [(i, j) for i in range(m) if len(dom[i]) <= k - 1
              for j in sorted(nbr[i]) if j > i and len(dom[i] | dom[j]) <= k - 1]
    need(pairs == wanted, 'file Q2, ordre ou codage')
    need(all(len(dom[i] | dom[j]) <= k - 1 for i, j in pairs), 'garde Q2 non redondant')
    return edges, len(pairs)


def model():
    total = pairs = edges = 0
    for n in (32, 256):
        for x in range(n):
            need(set(bits(above(n, x))) == set(range(x + 1, n)), 'above')
        for relation in itertools.product(range(3), repeat=6):
            for k in range(1, 13):
                e, q = verify(n, 4, k, relation)
                total += 1
                edges += e
                pairs += q
    extra = []
    for n, m in ((32, 1), (32, 2), (32, 31), (32, 32), (256, 33),
                 (256, 64), (256, 65), (256, 128), (256, 129), (256, 255), (256, 256)):
        # Dominances rares ; beaucoup de paires aux rangs eleves restent jugees.
        r = tuple(1 if j == i + 1 and i % 64 == 0 else 0
                  for i, j in itertools.combinations(range(m), 2))
        extra.append((n, m, r))
        for k in range(1, 13):
            e, q = verify(n, m, k, r)
            edges += e
            pairs += q
    mutations = ('one_endpoint', 'threshold', 'unsigned_threshold', 'dead_rows',
                 'third_row', 'bit32', 'double_visit', 'no_live', 'live1', 'pack32')
    witnesses = [(32, 4, (0,) * 6), (32, 3, (0, 0, 1)), extra[-1]]
    killed = {}
    for mutation in mutations:
        for n, m, relation in witnesses:
            for k in range(1, 13):
                try:
                    verify(n, m, k, relation, mutation)
                except ValueError as exc:
                    killed[mutation] = {'N': n, 'm': m, 'K': k, 'reason': str(exc)}
                    break
            if mutation in killed:
                break
        need(mutation in killed, 'mutation survivante ' + mutation)
    return {'exhaustive_relations': 729, 'exhaustive_cases': total,
            'high_rank_and_small_cases': len(extra) * 12,
            'old_prepare_union_calls': 2 * edges, 'new_prepare_union_calls': edges,
            'q2_pairs_with_true_guard': pairs, 'model_mutants_rejected': killed}


def warp_text(text):
    # Projection textuelle MHGP12_SIMT_WARP=1, sans compilateur/preprocesseur natif.
    out, stack, active = [], [], True
    for line in text.splitlines(True):
        token = line.strip()
        if token.startswith(('#if ', '#ifdef ', '#ifndef ')):
            ours = token == '#if MHGP12_SIMT_WARP'
            stack.append((active, ours))
            if not ours and active:
                out.append(line)
        elif token == '#else':
            parent, ours = stack[-1]
            if ours:
                active = not active and parent
            elif active:
                out.append(line)
        elif token == '#endif':
            parent, ours = stack.pop()
            active = parent
            if not ours and active:
                out.append(line)
        elif active:
            out.append(line)
    need(not stack, 'directives non fermees')
    return ''.join(out)


def git_bytes(repo, pin, path):
    return subprocess.check_output(['git', 'show', pin + ':' + path], cwd=repo)


def check_sources(repo, capture):
    original = {}
    for path, digest in capture['sources'].items():
        raw = (repo / path).read_bytes()
        need(sha(raw) == digest, 'source modifiee ' + path)
        for pin in (capture['base_pin'], capture['main_before'], capture['main_after']):
            need(git_bytes(repo, pin, path) == raw, 'blob different ' + path)
        original[path] = raw
    return original


def patches(original, capture):
    names = list(capture['patches'])
    for name, digest in capture['patches'].items():
        need(sha((HERE / name).read_bytes()) == digest, 'patch modifie')
    combined = []
    for sequence in ((names[0],), (names[1],), tuple(names), tuple(reversed(names))):
        with tempfile.TemporaryDirectory(prefix='audit-cpu-live-') as directory:
            root = Path(directory)
            for path, data in original.items():
                target = root / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
            for name in sequence:
                for option in (['--check'], []):
                    subprocess.run(['git', 'apply', *option, str(HERE / name)], cwd=root, check=True,
                                   capture_output=True)
            current = {path: (root / path).read_bytes() for path in original}
            for path, data in original.items():
                need(warp_text(data.decode()) == warp_text(current[path].decode()), 'texte warp altere')
            if len(sequence) == 2:
                combined.append(current)
    need(combined[0] == combined[1], 'ordre des patches')
    return {'isolated_and_both_orders': 4, 'product_warp_text_identical': True,
            'patched_sources': {p: sha(v) for p, v in combined[0].items()}}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, default=HERE.parents[3])
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    original = check_sources(args.repo, capture)
    result = {'patches': patches(original, capture), 'model': model()}
    need(check_sources(args.repo, capture) == original, 'sources changees pendant lecture')
    if 'result' in capture:
        need(capture['result'] == result, 'resultat different de la capture')
    print(json.dumps(result, sort_keys=True, ensure_ascii=False))


if __name__ == '__main__':
    main()
