#!/usr/bin/env python3
"""Raccord des épingles seulement ; ni simulation C++ ni exécution du moteur."""
import hashlib
import json
from pathlib import Path
import sys


def main():
    here = Path(__file__).resolve().parent
    source = Path(sys.argv[1])  # racine morsehgp3D_v12 du snapshot externe
    capture = json.loads((here / 'capture.json').read_text())
    for name, expected in capture['sources'].items():
        actual = hashlib.sha256((source / name).read_bytes()).hexdigest()
        if actual != expected:
            raise ValueError('source changée : ' + name)
    print(json.dumps({'pinned_sources': len(capture['sources']),
                      'native_executed': False,
                      'historical_comparisons': capture['comparisons']},
                     sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
