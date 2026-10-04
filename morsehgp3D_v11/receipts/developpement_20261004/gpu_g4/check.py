#!/usr/bin/env python3
"""Rejeu du recu GPU G4 du 4 octobre (claudegpu1 a claudegpu6) : arrets certifies, verdicts, identite des prises,
meilleurs temps, composants de l'executeur et metriques Nsight Compute citees par le README. Bibliotheque standard
seule ; tient sous python3 -O (aucun assert). Code 0 : conforme."""
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent / 'sessions'
FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')
TARGET = 'ehgp-v7-3b1d496aed430749ea7e049f'
STATUS = {'claudegpu1': 'failed_remote', 'claudegpu2': 'failed_remote', 'claudegpu3': 'completed',
          'claudegpu4': 'completed', 'claudegpu5': 'completed', 'claudegpu6': 'completed'}
# Meilleurs temps : (session, rapport, mode, regime) -> ng00, ng01, ng02 (mediane a froid, meilleure passe a chaud).
BEST = {('claudegpu5', 'gpu_k5', 'cpu', 'froid'): (368.6, 294.1, 374.7),
        ('claudegpu5', 'gpu_k5', 'cpu', 'chaud'): (363.3, 274.6, 330.0),
        ('claudegpu6', 'gpu_k5', 'cpu', 'chaud'): (342.7, 288.0, 373.7),
        ('claudegpu6', 'gpu_k5', 'gpu', 'froid'): (434.8, 392.4, 423.4),
        ('claudegpu5', 'gpu_k5', 'gpu', 'froid'): (451.7, 380.4, 436.5),
        ('claudegpu5', 'gpu_k5', 'gpu', 'chaud'): (394.0, 322.9, 380.1),
        ('claudegpu5', 'gpu_k10', 'cpu', 'froid'): (2489.9, 1896.8, 2112.6),
        ('claudegpu5', 'gpu_k10', 'cpu', 'chaud'): (2437.2, 1796.5, 2029.5),
        ('claudegpu6', 'gpu_k10', 'gpu', 'froid'): (2398.6, 1843.4, 2009.4),
        ('claudegpu5', 'gpu_k10', 'gpu', 'froid'): (2401.1, 1796.7, 2048.1),
        ('claudegpu5', 'gpu_k10', 'gpu', 'chaud'): (2348.5, 1761.9, 1987.2)}
# Composants de l'executeur GPU, ng00, medianes a froid (ms) : session, rapport -> champs du lot.
FIELDS = ('device_init_ns', 'count_ns', 'fill_ns', 'download_ns', 'levels_ns', 'gather_ns')
PARTS = {('claudegpu3', 'gpu_k5'): (43.0, 37.6, 35.4, 21.9, 15.3, 10.5),
         ('claudegpu4', 'gpu_k5'): (32.6, 34.4, 18.7, 5.5, 15.2, 2.4),
         ('claudegpu5', 'gpu_k5'): (27.7, 32.8, 13.7, 5.6, 15.2, 2.4),
         ('claudegpu6', 'gpu_k5'): (32.2, 37.6, 17.6, 2.3, 5.6, 2.5),
         ('claudegpu3', 'gpu_k10_leaf24'): (None, 223.0, 199.6, 107.1, 66.7, 18.7),
         ('claudegpu6', 'gpu_k10'): (11.0, 218.1, 112.9, 7.8, 21.4, 3.1)}
# Nsight Compute, ng00, K = 5, feuilles 16 : (session, noyau) -> duree, registres, occupation atteinte, fils actifs.
NCU = {('claudegpu3', 'count_kernel'): ('44.38 ms', '210 register/thread', '16.63 %', '3.36 '),
       ('claudegpu3', 'fill_kernel'): ('40.84 ms', '164 register/thread', '19.85 %', '3.43 '),
       ('claudegpu5', 'count_kernel'): ('38.53 ms', '168 register/thread', '21.56 %', '3.43 '),
       ('claudegpu6', 'fill_kernel'): ('21.58 ms', '168 register/thread', '2.12 %', '4.10 ')}
CHECKS = 0


def need(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        print('recu_gpu_verdict refus : ' + reason)
        sys.exit(1)


def load(*parts):
    return json.loads(HERE.joinpath(*parts).read_text())


def median(values):
    v = sorted(values)
    return v[len(v) // 2]


def main():
    for session, status in STATUS.items():
        receipt = load(session, 'receipt.json')
        need(receipt['status'] == status, session + ' statut ' + receipt['status'])
        need(receipt['closure'] == 'stopped' and receipt['observed_after']['status'] == 'TERMINATED' and
             receipt['observed_after']['name'] == TARGET, session + ' arret certifie sur la cible')
    need(load('claudegpu2', 'gpu_k5_report.json')['verdict'] == 'refus', 'claudegpu2 refusee')
    cold = warm = 0
    for session in ('claudegpu3', 'claudegpu4', 'claudegpu5', 'claudegpu6'):
        for path in sorted(HERE.joinpath(session).glob('*_report.json')):
            report = json.loads(path.read_text())
            need(report['verdict'] == 'conforme' and not report['refusals'], path.name + ' conforme')
            for row in report['cold'] + report['warm']:
                need(row['code'] == 0 and row['dump_sha256'] == report['identity'][row['frame']], 'prise identique')
            cold += len(report['cold'])
            warm += len(report['warm'])
    need((cold, warm) == (372, 84), 'nombre de prises')
    for (session, name, mode, regime), expected in BEST.items():
        report = load(session, name + '_report.json')
        for frame, value in zip(FRAMES, expected):
            if regime == 'froid':
                got = median([r['summary']['wall_ms'] for r in report['cold'] if r['frame'] == frame and r['mode'] == mode])
            else:
                got = report['warm_medians_ms']['warm|%s|w48|%s' % (frame, mode)]['best_wall_ms']
            need(round(got, 1) == value, '%s %s %s %s %s' % (session, name, mode, regime, frame))
    for (session, name), expected in PARTS.items():
        report = load(session, name + '_report.json')
        batches = [r['summary']['batch'] for r in report['cold'] if r['frame'] == 'lidar_ng00' and r['mode'] == 'gpu']
        for field, value in zip(FIELDS, expected):
            if value is not None:
                need(round(median([b[field] for b in batches]) / 1e6, 1) == value, '%s %s %s' % (session, name, field))
    for (session, kernel), (duration, registers, achieved, active) in NCU.items():
        metrics = load(session, 'prof_k5', 'gpu_profile.json')['ncu'][kernel]['metrics']
        need((metrics['Duration'], metrics['Registers Per Thread'], metrics['Achieved Occupancy'],
              metrics['Avg. Active Threads Per Warp']) == (duration, registers, achieved, active), session + ' ' + kernel)
    print('recu_gpu_verdict conforme controles%d' % CHECKS)


if __name__ == '__main__':
    main()
