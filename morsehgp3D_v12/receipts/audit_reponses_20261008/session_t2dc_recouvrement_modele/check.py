#!/usr/bin/env python3
"""Comptabilite conditionnelle sur 18 journaux FULL deja admis ; aucun moteur."""
import argparse
from collections import defaultdict
import hashlib
import importlib.util
import json
from pathlib import Path
import statistics as stats
import subprocess

HERE = Path(__file__).resolve().parent
LIMIT = 100_000_000


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--returned', type=Path, required=True)
    ap.add_argument('--admission', type=Path)
    args = ap.parse_args()
    pin = json.loads((HERE / 'capture.json').read_text())
    old = args.repo / pin['formula_reader']['path']
    if digest(old.read_bytes()) != pin['formula_reader']['sha256']:
        raise ValueError('lecteur historique different')
    spec = importlib.util.spec_from_file_location('model_k', old)
    model_k = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(model_k)
    need, sha = model_k.need, model_k.sha
    admission = args.admission or args.repo / pin['admission']['path']
    for name, expected in pin['admission']['files'].items():
        need(sha((admission / name).read_bytes()) == expected, 'preuve admission differente: ' + name)
    admitted = json.loads((admission / 'results.json').read_text())
    for source in pin['sources']:
        raw = subprocess.check_output(['git', '-C', str(args.repo), 'show',
                                       source['commit'] + ':' + source['path']])
        need(sha(raw) == source['sha256'], 'source differente')
    report_raw = (args.returned / 'report.json').read_bytes()
    need(sha(report_raw) == pin['report_sha256'], 'rapport different')
    report = json.loads(report_raw)
    runs = report['steps']['informations']['full']
    need(len(runs) == 18, 'cohorte FULL')
    data = defaultdict(dict)
    seen = set()
    for item in runs:
        frame, arm, turn = item['frame'], item['arm'], item['round']
        key = (frame, arm, turn)
        need(frame in pin['frames'] and arm in ('avant', 'apres') and
             type(turn) is int and turn in range(3) and key not in seen, 'case FULL')
        seen.add(key)
        name = f'logs/i_full_r{turn}_{frame}_{arm}.log'
        raw = (args.returned / name).read_bytes()
        need(sha(raw) == pin['logs_sha256'][name], 'journal different')
        # La ligne de commande reste privee ; aucune sortie ni copie dans le recu.
        rows = [json.loads(line) for line in raw.splitlines() if line.startswith(b'{')]
        run = item['run']
        need(rows == run['rows'] and type(run['code']) is int and run['code'] == 0,
             'journal/rapport/code')
        full = [row for row in rows if row['phase'] == 'full']
        need([row['pass'] for row in full] == list(range(10)), 'dix passes ordonnees')
        warm = []
        for row in full[1:]:
            need((row['trame'], row['sites'], row['kmax'], row['threads'], row['voie']) ==
                 (frame, pin['frames'][frame], 5, 48, 'device'), 'configuration FULL')
            st = row['etapes_ns']
            # Meme formule que K ; nouvelle cohorte, jamais son main() a cinq processus.
            residual = row['wall_ns'] - sum(st[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR'))
            need(residual >= 0, 'residu negatif')
            prefix = st['P'] + st['C'] + st['raccord']
            perfect = prefix + max(st['G'], st['TMVR'])
            with_residual = perfect + residual
            # Verification algebrique independante, avant toute agregation.
            need(perfect == max(prefix + st['G'], prefix + st['TMVR']) and
                 with_residual == row['wall_ns'] - min(st['G'], st['TMVR']), 'identite comptable')
            warm.append(dict(wall=row['wall_ns'], scenario_sans_residu=perfect,
                             scenario_avec_residu=with_residual, residu=residual,
                             P=st['P'], C=st['C'], raccord=st['raccord'], G=st['G'], TMVR=st['TMVR'],
                             prefix=prefix, manque_100ms=max(0, with_residual - LIMIT)))
        data[(arm, frame)][turn] = warm
    need(len(data) == 6 and all(set(v) == {0, 1, 2} for v in data.values()), 'cohorte incomplete')

    def summarize(processes, field):
        values = [row[field] for rows in processes.values() for row in rows]
        return dict(mediane_ns=stats.median(values), max_brut_ns=max(values),
                    max_medianes_processus_ns=max(stats.median(row[field] for row in rows)
                                                 for rows in processes.values()))

    groups = {}
    for arm in ('avant', 'apres'):
        frames = {}
        for frame in pin['frames']:
            processes = data[(arm, frame)]
            metrics = {field: summarize(processes, field) for field in next(iter(processes.values()))[0]}
            ref = admitted['tables']['FULL5'][frame][arm]
            need(metrics['wall']['mediane_ns'] == ref['median_ns'] and
                 metrics['wall']['max_medianes_processus_ns'] == ref['max_process_median_ns'] and
                 ref['processes'] == 3 and ref['warm_passes'] == 27, 'agregat admission different')
            flat = [row for rows in processes.values() for row in rows]
            counts = {field: dict(passes_moins_100ms=sum(row[field] < LIMIT for row in flat),
                                 processus_mediane_moins_100ms=sum(
                                     stats.median(row[field] for row in rows) < LIMIT
                                     for rows in processes.values()))
                      for field in ('wall', 'scenario_avec_residu')}
            frames[frame] = dict(mesures=metrics, comptes=counts,
                                 passes_TMVR_superieur_G=sum(row['TMVR'] > row['G'] for row in flat))
        aggregate = {}
        for field in ('wall', 'scenario_avec_residu'):
            values = [v['mesures'][field] for v in frames.values()]
            aggregate[field] = dict(mediane_des_medianes_ns=stats.median(v['mediane_ns'] for v in values),
                                   maximum_medianes_processus_ns=max(v['max_medianes_processus_ns'] for v in values),
                                   maximum_brut_ns=max(v['max_brut_ns'] for v in values),
                                   trames_mediane_moins_100ms=sum(v['mediane_ns'] < LIMIT for v in values),
                                   trames_max_medianes_moins_100ms=sum(v['max_medianes_processus_ns'] < LIMIT for v in values),
                                   passes_moins_100ms=sum(v['comptes'][field]['passes_moins_100ms'] for v in frames.values()))
        groups[arm] = dict(par_trame=frames, agregats=aggregate)
    paired = {}
    for frame in pin['frames']:
        paired[frame] = {}
        for field in ('wall', 'C', 'G', 'TMVR', 'scenario_avec_residu'):
            diffs = [data[('apres', frame)][turn][p][field] - data[('avant', frame)][turn][p][field]
                     for turn in range(3) for p in range(9)]
            paired[frame][field] = dict(mediane_apres_moins_avant_ns=stats.median(diffs))
    for name, expected in pin['logs_sha256'].items():
        need(sha((args.returned / name).read_bytes()) == expected, 'journal modifie pendant lecture')
    need(sha((args.returned / 'report.json').read_bytes()) == pin['report_sha256'], 'rapport mobile')
    result = dict(execution_native=False, scenario_conditionnel_seulement=True,
                  processus=18, passes_totales=180, passes_chaudes=162,
                  catalogue37_exclu_du_FULL=True, groupes=groups, differences_appariees=paired)
    print(json.dumps(result, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
