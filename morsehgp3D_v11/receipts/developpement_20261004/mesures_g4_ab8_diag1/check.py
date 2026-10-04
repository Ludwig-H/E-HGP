#!/usr/bin/env python3
"""Rejeu du recu des mesures G4 du 4 octobre (claudeab8, claudediag1) : arret certifie, rapports W1 apparies, K = 10,
W24 epingle contre W48. Bibliotheque standard seule ; tient sous python3 -O (aucun assert). Code 0 : conforme."""
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')
# Rapports B/A a W1 (une paire par trame), mur puis passe unique, arrondis a 4 decimales : gravure du README.
W1 = {('claudeab8', 'base', 'q3'): ((0.9870, 0.9822), (0.9904, 0.9838), (0.9893, 0.9850)),
      ('claudeab8', 'q3', 'new'): ((0.9997, 1.0028), (1.0058, 1.0035), (1.0012, 0.9984)),
      ('claudediag1', 'qr', 'qr2'): ((0.9972, 0.9982), (1.0044, 1.0041), (0.9995, 0.9981)),
      ('claudediag1', 'qr', 'r1'): ((0.9994, 0.9986), (1.0029, 1.0020), (0.9988, 0.9988)),
      ('claudediag1', 'r1', 'new'): ((1.0030, 1.0136), (1.0135, 1.0143), (1.0103, 1.0120))}
# Medianes de mur (ms) par trame : K = 10 a feuilles 16 et 24 (W48), K = 5 a W24 epingle et W48 libre.
WALLS = {'k10_leaf16': (3284.69, 2506.39, 2738.83), 'k10_leaf24': (2506.13, 1822.46, 2064.5),
         'w24_pinned': (574.91, 442.59, 527.67), 'w48_free': (408.36, 296.06, 359.7)}
CHECKS = 0


def need(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        print('recu_mesures_verdict refus : ' + reason)
        sys.exit(1)


def load(*parts):
    return json.loads(HERE.joinpath('sessions', *parts).read_text())


def w1_ratio(report, frame, a, b, field):
    takes = {t['variant']: t['summary'] for t in report['timings'] if t['workers'] == '1' and t['frame'] == frame}
    pick = (lambda s: s['wall_ns']) if field == 'wall' else (lambda s: s['domain_detail']['single_pass_ns'])
    return round(pick(takes[b]) / pick(takes[a]), 4)


def main():
    statuses = {'claudeab8': 'completed', 'claudediag1': 'failed_remote'}
    for session, status in statuses.items():
        receipt = load(session, 'receipt.json')
        need(receipt['status'] == status, session + ' statut ' + receipt['status'])
        need(receipt['observed_after']['status'] == 'TERMINATED' and receipt['closure'] == 'stopped',
             session + ' arret certifie')
        need(receipt['observed_after']['name'] == 'ehgp-v7-3b1d496aed430749ea7e049f', session + ' cible exacte')
    ab8, diag1 = load('claudeab8', 'ab_report.json'), load('claudediag1', 'ab_report.json')
    need(ab8['verdict'] == 'conforme' and not ab8['refusals'], 'claudeab8 conforme')
    # claudediag1 : seul refus, la porte de style (fonction de 110 lignes, corrigee depuis par publish_timings).
    need(diag1['refusals'] == ['step ctest_new code 8 quiet True', 'ctest not all passed'] and
         diag1['tests']['summary'] == '99% tests passed, 2 tests failed out of 678', 'claudediag1 refus de style seul')
    reports = {'claudeab8': ab8, 'claudediag1': diag1}
    for report in reports.values():
        for t in report['timings']:
            need(t['code'] == 0 and t['summary']['status'] == 'ok' and t['dump_sha256'] == report['identity'][t['frame']],
                 'prise conforme et sortie identique')
    for (session, a, b), expected in W1.items():
        for frame, (wall, single) in zip(FRAMES, expected):
            need(w1_ratio(reports[session], frame, a, b, 'wall') == wall, '%s %s->%s %s mur' % (session, a, b, frame))
            need(w1_ratio(reports[session], frame, a, b, 'single') == single,
                 '%s %s->%s %s passe unique' % (session, a, b, frame))
    for name, expected in WALLS.items():
        rows = {r['name']: r for r in load('claudediag1', name + '.json')['rows']}
        for frame, wall in zip(FRAMES, expected):
            need(rows[frame]['wall_median_ms'] == wall and all(t['ok'] for t in rows[frame]['takes']),
                 name + ' ' + frame)
    print('recu_mesures_verdict conforme controles%d' % CHECKS)


if __name__ == '__main__':
    main()
