#!/usr/bin/env python3
"""Lecteur du recu ecart_v10_v11 : empreintes, identite des sorties et medianes. Aucune execution native."""
import hashlib, json, statistics, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def need(value, message):
    if not value:
        raise SystemExit('refus : ' + message)


def hashes():
    lines = (HERE / 'SHA256SUMS').read_text().splitlines()
    need(lines, 'SHA256SUMS vide')
    for line in lines:
        digest, name = line.split('  ', 1)
        need(hashlib.sha256((HERE / name).read_bytes()).hexdigest() == digest, 'empreinte ' + name)
    return len(lines)


def main():
    files = hashes()
    records = json.loads((HERE / 'records.json').read_text())
    need(len(records) >= 30, 'mesures absentes')
    need(all(r['rc'] == 0 and r['status'] == 'ok' and r['output_sha256'] for r in records), 'run en echec')
    # Identite octet pour octet : une seule empreinte par entree, toutes builds, modes et W confondus.
    digests = {}
    for r in records:
        digests.setdefault((r['case'], r['kmax']), set()).add(r['output_sha256'])
    need(all(len(v) == 1 for v in digests.values()), 'sorties differentes')
    rows = {}
    for r in records:
        rows.setdefault((r['case'], r['build'], r['mode'], r['workers']), []).append(r)
    print('cas build mode W n mur_ms domaine_ms foret_ms cpu_s')
    for key in sorted(rows):
        group = rows[key]
        med = lambda f: statistics.median(x[f] for x in group)
        print(*key, len(group), '%.0f' % med('wall_ms'), '%.0f' % med('domain_ms'), '%.0f' % med('forest_ms'),
              '%.2f' % med('cpu_s'))
    print('recu_ok fichiers=%d mesures=%d entrees=%d' % (files, len(records), len(digests)))


if __name__ == '__main__':
    main()
