#!/usr/bin/env python3
"""Preuve statique et modele borne ; aucun moteur ni compilateur execute."""
import argparse
import hashlib
import itertools
import json
import subprocess
import tempfile
from pathlib import Path


def need(condition, message):
    if not condition:
        raise SystemExit(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True, type=Path)
    ap.add_argument('--tmv', required=True, type=Path)
    ap.add_argument('--headers', type=Path, default=Path('/usr/include/c++/13'))
    args = ap.parse_args()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())
    roots = {'tmv': args.tmv, 'headers': args.headers, 'receipt': here}

    def verify():
        for row in cap['artifacts']:
            p = roots[row['root']] / row['path']
            need(digest(p.read_bytes()) == row['sha256'], 'hash: ' + row['path'])

    verify()
    for pin in (cap['main_pin'], cap['prototype_base']):
        got = subprocess.check_output(['git', 'ls-tree', pin, cap['source_path']], cwd=args.repo)
        need(not got, 'source deja livree au pin ' + pin)
    source = (args.tmv / 'repo5' / cap['source_path']).read_text()
    patch = (args.tmv / 'patch_tour_TMV.diff').read_text()
    start = patch.index('diff --git a/' + cap['source_path'] + ' b/' + cap['source_path'])
    block = patch[start:].split('\ndiff --git ', 1)[0]
    reconstructed = '\n'.join(x[1:] for x in block.splitlines() if x.startswith('+') and not x.startswith('+++')) + '\n'
    need(reconstructed == source, 'source differente du patch epingle')
    need('equal = equal || s == 0;' in source, 'comparateur modifie')
    for row in cap['artifacts']:
        if row['path'].endswith('flags.make'):
            flags = (args.tmv / row['path']).read_text()
            need('_GLIBCXX_DEBUG' not in flags and '_GLIBCXX_ASSERTIONS' not in flags, 'configuration changee')
    with tempfile.TemporaryDirectory(prefix='audit-birth-comparator-') as tmp:
        out = Path(tmp) / cap['source_path']
        out.parent.mkdir(parents=True)
        out.write_text(source)
        subprocess.run(['git', 'apply', '--check', str(here / 'proposition.patch')], cwd=tmp, check=True)

    # Preappel exact de la macro debug sur des valeurs distinctes.
    centres = [(1, 0, 0), (11, 0, 0)]
    s = (centres[0] > centres[0]) - (centres[0] < centres[0])
    old_flag = s == 0
    need(old_flag and not (s < 0), 'modele de la macro')
    # Apres tri pur, les egalites forment un intervalle contigu : controle adjacent complet.
    cases = 0
    for n in range(2, 8):
        for xs in itertools.product(range(3), repeat=n):
            ordered = sorted(xs)
            adjacent = any(a == b for a, b in zip(ordered, ordered[1:]))
            need(adjacent == (len(set(xs)) < n), 'controle adjacent incomplet')
            cases += 1
    # Deux boules de rayon 1 et centres distincts, au meme niveau exact.
    points = [(0, 0, 0), (2, 0, 0), (10, 0, 0), (12, 0, 0)]
    for centre, pair in zip(centres, (points[:2], points[2:])):
        need(all(sum((x - y) ** 2 for x, y in zip(p, centre)) == 1 for p in pair), 'rayon')
        need(all(sum((x - y) ** 2 for x, y in zip(p, centre)) > 1 for p in points if p not in pair), 'autre paire')
    verify()
    result = {'scope': 'static_and_python_only', 'artifact_hashes_before_after': len(cap['artifacts']),
              'source_reconstructed_from_pinned_patch': True, 'patch_apply_check': True,
              'distinct_centres': True, 'irreflexive_predicate_returns_false': True,
              'old_equal_flag_after_debug_precheck': old_flag, 'adjacent_check_cases': cases,
              'four_site_same_radius_witness': True, 'native_debug_configuration_executed': False}
    print(json.dumps(result, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
