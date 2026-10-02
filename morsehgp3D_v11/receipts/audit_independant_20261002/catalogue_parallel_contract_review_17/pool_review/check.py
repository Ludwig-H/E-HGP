#!/usr/bin/env python3
"""Contrôles autonomes de pins et d'issues, aucun import ou binaire produit."""
import hashlib
import itertools
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent

def require(ok, message):
    if not ok:
        raise RuntimeError(message)

def run():
    copies = 0
    for name in ('SOURCE_BEFORE.json', 'V10_BEFORE.json'):
        for r in json.loads((ROOT / name).read_text())['files']:
            if r.get('missing'):
                continue
            b = (ROOT / r['copy']).read_bytes()
            require(len(b) == r['bytes'] and hashlib.sha256(b).hexdigest() == r['sha256'], r['path'])
            copies += 1
    # Une réduction totale fixe ne dépend pas de la permutation des issues.
    # Le choix de la première exception v10 peut dépendre de l'arrivée.
    table = (ROOT / 'sources/morsehgp3D_v11/src/core/reasons.def').read_text()
    reasons = re.findall(r'^MHGP11_REASON\((\w+),', table, re.MULTILINE)
    require('memory_budget' in reasons and 'empty_input' in reasons, 'reason table')
    priority = {name: i for i, name in enumerate(reasons)}
    outcomes = [('memory_budget', 2), ('empty_input', 2), ('session_overhead', 3)]
    merged = set()
    first = set()
    for order in itertools.permutations(outcomes):
        merged.add(min(order, key=lambda o: (o[1], priority[o[0]])))
        first.add(order[0])
    require(len(merged) == 1 and len(first) == 3, 'deterministic reduction versus first arrival')
    # Borner b+min(g,n-b) avant l'addition, même à UINT64_MAX.
    bound = 2**64 - 1
    cases = 0
    for n in (0, 1, bound - 1, bound):
        for grain in (bound // 3, bound // 2, bound - 1, bound):
            b = 0
            while b < n:
                e = b + min(grain, n - b)
                require(0 <= b < e <= n <= bound, 'range bounds')
                b = e
            require(b == n, 'exhaustive intervals')
            cases += 1
    return {'status': 'PASS', 'source_copies': copies, 'permutations': 6,
            'total_reduction_results': len(merged), 'first_arrival_results': len(first),
            'scalar_range_cases': cases, 'native_runs': 0}

if __name__ == '__main__':
    print(json.dumps(run(), sort_keys=True, indent=2))
