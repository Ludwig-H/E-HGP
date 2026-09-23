#!/usr/bin/env python3
"""Compare deux vidages de catalogue (83 octets par boule) cle par cle.
Sortie : boules manquantes et ajoutees, ventilees par (p, q_min, u) et par
premier ordre ou la boule compte au bilan d'Euler (p+1) et fenetre de calendrier."""
import collections
import json
import sys


def load(path):
    data = open(path, 'rb').read()
    out = {}
    for i in range(0, len(data), 83):
        rec = data[i:i + 83]
        out[rec[:80]] = (rec[80], rec[81], rec[82])
    return out


def main():
    a, b = load(sys.argv[1]), load(sys.argv[2])
    kmax = int(sys.argv[3])
    missing = [a[k] for k in a if k not in b]
    added = [b[k] for k in b if k not in a]
    changed = [(a[k], b[k]) for k in a if k in b and a[k] != b[k]]

    def summary(rows):
        by = collections.Counter(rows)
        lowest = collections.Counter(p + 1 for (p, q, u) in rows)
        return {'count': len(rows), 'by_p_q_u': {f'{p},{q},{u}': c for (p, q, u), c in sorted(by.items())},
                'first_euler_order': dict(sorted(lowest.items())),
                'only_top_two_orders': all(p + 1 >= kmax - 1 for (p, q, u) in rows)}
    print(json.dumps({'reference': len(a), 'candidate': len(b), 'missing': summary(missing), 'added': summary(added),
                      'changed_fields': len(changed), 'identical': not missing and not added and not changed}))


if __name__ == '__main__':
    main()
