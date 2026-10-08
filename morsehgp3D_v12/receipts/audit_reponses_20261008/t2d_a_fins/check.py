#!/usr/bin/env python3
"""Modèle d'intervalles abstraits ; aucune horloge, aucun moteur exécuté."""
import argparse
import hashlib
import json
from pathlib import Path


def need(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree(root):
    paths = sorted(p for p in (root / 'src').rglob('*') if p.is_file())
    body = ''.join(f'{sha(p)}  {p.relative_to(root).as_posix()}\n' for p in paths)
    return {'files': len(paths), 'sha256': hashlib.sha256(body.encode()).hexdigest()}


def tail(begin, end, boundary):
    return max(0, end - max(begin, boundary))


def model():
    cases = differences = 0
    # tG <= début réel ; Gend est déjà publié avant la charge. Aucune unité physique.
    for tg in range(6):
        for gap in range(6):
            for duration in range(6):
                begin, end = tg + gap, tg + gap + duration
                for gend in range(21):
                    shifted = tail(tg, tg + duration, gend)
                    actual = tail(begin, end, gend)
                    need(shifted <= actual, 'translation doit sous-compter ou égaler')
                    need(tail(begin, end, gend) == actual, 'vraies bornes')
                    cases += 1
                    differences += shifted != actual
    witness = {'g_task_end': 10, 'leaf_begin': 20, 'leaf_end': 30, 'global_g_end': 25,
               'charged': tail(10, 20, 25), 'exact': tail(20, 30, 25)}
    need(witness['charged'] == 0 and witness['exact'] == 5, 'témoin')
    return {'abstract_cases': cases, 'strict_under_counts': differences, 'witness': witness,
            'native_execution': False, 'performance_measurement': False}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--prototype', type=Path, required=True)
    ap.add_argument('--baseline', type=Path, required=True)
    args = ap.parse_args()
    pin = json.loads(Path(__file__).with_name('capture.json').read_text())
    expected = pin['source_tree']
    before = [tree(p) for p in (args.prototype, args.baseline)]
    need(before == [expected, expected], 'arbre src changé')
    for path, digest in pin['files'].items():
        need(sha(args.prototype / path) == digest, f'pin changé: {path}')
    out = model()
    need([tree(p) for p in (args.prototype, args.baseline)] == before, 'sources changées pendant lecture')
    out['sources_verified'] = True
    print(json.dumps(out, sort_keys=True))


if __name__ == '__main__':
    main()
