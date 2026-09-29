"""Fusion des sessions G4 d'une campagne de test preenregistree executee par lot_runner.py.

Verifie que toutes les sessions ont tourne sous le meme preenregistrement (sha256) et le meme plan, qu'aucune scene
n'a ete calculee deux fois, et que chaque scene presente porte toutes les methodes du preenregistrement. Ecrit :
  - OUT/results.csv et OUT/run.json (segments = les sessions), directement lisibles par decide.py ;
  - OUT/done.u32le : conteneur (en-tete u64 = longueur) de la liste des scenes faites, pour la session suivante.

  python3 merge_sessions.py --prereg PREREG.json --out DIR SESSION_DIR [SESSION_DIR ...]
Codes : 0 (fusion ecrite, campagne complete ou non) ; 2 refus (sessions incoherentes).
"""
import argparse
import csv
import hashlib
import json
import os
import struct
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--prereg', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('sessions', nargs='+')
    a = ap.parse_args()
    prereg = json.load(open(a.prereg))
    psha = hashlib.sha256(open(a.prereg, 'rb').read()).hexdigest()
    names = {m['name'] for m in prereg['methods']}
    rows, segments, seen, columns = [], [], {}, None
    for d in a.sessions:
        info = json.load(open(os.path.join(d, 'run.json')))
        if info['prereg_sha256'] != psha:
            print('REFUS %s : autre preenregistrement' % d)
            return 2
        segments.append(info)
        with open(os.path.join(d, 'results.csv')) as f:
            r = csv.DictReader(f)
            columns = columns or r.fieldnames
            by_unit = {}
            for row in r:
                by_unit.setdefault(row['unit'], []).append(row)
        for unit, rs in by_unit.items():
            if unit in seen:
                print('REFUS scene calculee deux fois : %s (%s et %s)' % (unit, seen[unit], d))
                return 2
            if {x['method'] for x in rs} != names or len(rs) != len(names):
                print('REFUS scene incomplete : %s dans %s' % (unit, d))
                return 2
            seen[unit] = d
            rows += rs
    plans = {s['plan_sha256'] for s in segments}
    if len(plans) != 1:
        print('REFUS plans differents')
        return 2
    os.makedirs(a.out, exist_ok=True)
    with open(os.path.join(a.out, 'results.csv'), 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=columns)
        w.writeheader()
        for row in rows:
            w.writerow(row)
    total = segments[0]['scenes']
    info = dict(prereg=os.path.basename(a.prereg), prereg_sha256=psha, plan_sha256=plans.pop(), scenes=total,
                computed=len(seen), complete=len(seen) == total, segments=segments)
    with open(os.path.join(a.out, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    payload = '\n'.join(sorted(seen)).encode()
    with open(os.path.join(a.out, 'done.u32le'), 'wb') as f:
        f.write(struct.pack('<Q', len(payload)) + payload + b'\0' * ((-(8 + len(payload))) % 4))
    print('%d scenes sur %d fusionnees depuis %d session(s)' % (len(seen), total, len(segments)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
