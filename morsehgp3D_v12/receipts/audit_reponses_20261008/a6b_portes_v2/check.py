#!/usr/bin/env python3
"""Pins sources et blocs de traces déjà produits ; aucun programme natif lancé."""
import hashlib
import json
from pathlib import Path
import sys


def main():
    here = Path(__file__).resolve().parent
    source = Path(sys.argv[1])
    c = json.loads((here / 'capture.json').read_text())
    for p, h in c['sources'].items():
        if hashlib.sha256((source / p).read_bytes()).hexdigest() != h:
            raise ValueError('source changée : ' + p)
    b = (source / 'traces/gate_blocks.json').read_bytes()
    if hashlib.sha256(b).hexdigest() != c['traces']['selected_blocks_sha256']:
        raise ValueError('traces changées')
    blocks = json.loads(b)
    for name, block in blocks.items():
        if 'Test Passed.' not in block:
            raise ValueError('bloc incomplet : ' + name)
    print(json.dumps({'source_pins': len(c['sources']), 'closed_gate_blocks': sorted(blocks),
                      'full_ctest_closed_at_capture': False, 'native_invocations_by_audit': 0},
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
