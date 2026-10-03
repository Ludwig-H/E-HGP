#!/usr/bin/env python3
"""Lecteur de la tranche 2 : empreintes, identite contre la base de la tranche 1, medianes. Aucune execution."""
import hashlib, json, statistics
from pathlib import Path

HERE = Path(__file__).resolve().parent
FIRST = HERE.parent / 'ecart_v10_v11'


def need(value, message):
    if not value:
        raise SystemExit('refus : ' + message)


def verify(folder):
    lines = (folder / 'SHA256SUMS').read_text().splitlines()
    need(lines, 'SHA256SUMS vide')
    for line in lines:
        digest, name = line.split('  ', 1)
        need(hashlib.sha256((folder / name).read_bytes()).hexdigest() == digest, 'empreinte ' + name)
    return len(lines)


def main():
    files = verify(HERE)
    verify(FIRST)  # la base et ses empreintes de sortie restent celles du recu precedent
    base = {r['case']: r['output_sha256'] for r in json.loads((FIRST / 'records.json').read_text())}
    records = json.loads((HERE / 'records.json').read_text())
    need(len(records) == 13 and all(r['rc'] == 0 and r['status'] == 'ok' for r in records), 'mesures')
    need(all(r['output_sha256'] == base[r['case']] for r in records), 'sorties differentes de la base')
    rows = {}
    for r in records:
        rows.setdefault((r['case'], r['workers']), []).append(r)
    for key in sorted(rows):
        group = rows[key]
        med = lambda f: statistics.median(x[f] for x in group)
        births = statistics.median(x['phases_ms']['births_ns'] for x in group)
        print(*key, len(group), 'mur %.0f foret %.0f naissances %.0f' % (med('wall_ms'), med('forest_ms'), births))
    print('recu_ok fichiers=%d mesures=%d' % (files, len(records)))


if __name__ == '__main__':
    main()
