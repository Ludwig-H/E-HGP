#!/usr/bin/env python3
"""Modele exact de la garde et du remplissage de cle ; aucun moteur execute."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

NONE = 2 ** 32 - 1
SITES = 5
TABLE = {(0, 1, NONE, NONE): 7, (0, 2, 4, NONE): 11, (0, 1, 3, 4): 19}


def lookup(support, fixed):
    if len(support) < 2 or len(support) > 4:
        return None
    if any(a >= b for a, b in zip(support, support[1:])):
        return None
    last = support[-1] if fixed else support[0]
    # u64(last) + 1 >= off.size() ; off.size() = sites + 1.
    if last + 1 >= SITES + 1:
        return None
    return TABLE.get(tuple(support) + (NONE,) * (4 - len(support)))


def need(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--main', type=Path)
    parser.add_argument('--scratch', type=Path, help='repertoire v12_tour_Gc')
    args = parser.parse_args()
    need(bool(args.main) == bool(args.scratch), '--main et --scratch ensemble')
    source_check = None
    if args.main:
        git3 = args.scratch / 'git3'
        pin = '45976be8ddc489f14494937b13e43d0ca5fa0a55'
        base = '781fbe8d13e943bc67cbcf3996d6791523f1a422'
        read = lambda repo, *options: subprocess.check_output(['git', *options], cwd=repo)
        paths = read(git3, 'diff', '--name-only', pin + '~2', pin).decode().splitlines()
        need(not any(p.startswith('morsehgp3D_v12/src/catalogue/') for p in paths), 'catalogue changed')
        for module in ('core', 'catalogue'):
            path = 'morsehgp3D_v12/src/' + module
            need(read(git3, 'ls-tree', '-r', pin, '--', path) ==
                 read(args.main, 'ls-tree', '-r', base, '--', path), 'module differs: ' + module)
        patch = read(git3, 'diff', pin + '~1', pin)
        digest = hashlib.sha256(patch).hexdigest()
        need(digest == '89d7b4f4383dace6c294e8513458469f8fa3e51ef11db92254b2ef4150d93f1e', 'pinned patch')
        source_check = {'prototype': pin, 'base_main': base, 'patch2_sha256': digest,
                        'catalogue_core_identical': True}
    checked = 0
    unchanged = 0
    for size in range(6):
        for support in itertools.product((0, 1, 2, 3, 4, 5, NONE), repeat=size):
            valid = 2 <= size <= 4 and all(a < b for a, b in zip(support, support[1:])) and support[-1] < SITES
            fixed = lookup(support, True)
            if valid:
                need(fixed == lookup(support, False), 'valid query changed')
                unchanged += 1
            else:
                need(fixed is None, 'invalid query admitted')
            checked += 1
    witnesses = []
    for support, original in (((0, 1, NONE), (0, 1)), ((0, 2, 4, NONE), (0, 2, 4))):
        need(lookup(support, False) == lookup(original, False) is not None, 'old alias')
        need(lookup(support, True) is None, 'alias persists')
        witnesses.append({'invalid_query': support, 'canonical_support': original,
                          'before': lookup(support, False), 'after': lookup(support, True)})
    print(json.dumps({'status': 'ok', 'queries_checked': checked, 'valid_queries_unchanged': unchanged,
                      'aliases': witnesses, 'native_executed': False, 'source_check': source_check},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
