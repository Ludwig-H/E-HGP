#!/usr/bin/env python3
"""Statistiques independantes des murs G et des cycles instrumentes archives."""
import argparse
import collections
import hashlib
import json
import statistics
from pathlib import Path

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def replay(repo):
    pins = json.loads((HERE / 'pins.json').read_text())
    base = repo / pins['receipt'] / 'resultats/cmd/001_t2c_pilote/files/t2c'
    path = base / 'rapport_t2c.json'
    need(hashlib.sha256(path.read_bytes()).hexdigest() ==
         pins['files']['resultats/cmd/001_t2c_pilote/files/t2c/rapport_t2c.json'], 'rapport modifie')
    report = json.loads(path.read_text())
    output = {}
    for group, k, processes, passes in [('k5', 5, 10, 10), ('k10_w48', 10, 3, 3)]:
        cases = {}
        turns = report['campagne_k5']['trames'] if k == 5 else report['informations'][group]['tours']
        need(set(turns) == {'ng00', 'ng01', 'ng02'}, 'trames')
        for case in sorted(turns):
            arms = {}
            need(len(turns[case]) == processes, 'processus')
            for arm in ('avant', 'apres'):
                warm, medians = [], []
                for turn in turns[case]:
                    take = turn[arm]
                    raw = (base / take['journal']).read_bytes()
                    need(hashlib.sha256(raw).hexdigest() == take['journal_sha256'], 'journal modifie')
                    rows = [r for r in map(json.loads, raw.splitlines()) if r.get('phase') == 'tour_g']
                    need([r['pass'] for r in rows] == list(range(passes)), 'passes')
                    need(all(r['status'] == 'ok' and r['kmax'] == k and r['threads'] == 48 for r in rows), 'regime')
                    values = [r['wall_ns'] for r in rows[1:]]
                    medians.append(statistics.median(values))
                    warm.extend(values)
                arms[arm] = dict(processes=processes, warm_passes=len(warm),
                    median_process_medians_ns=statistics.median(medians),
                    max_process_median_ns=max(medians), median_all_warm_ns=statistics.median(warm),
                    max_all_warm_ns=max(warm))
            cases[case] = arms
        output[group] = cases
    profile = {}
    for group in ('profil_k5', 'profil_k5_w48'):
        config = report['informations'][group]
        take = config['tours']['ng00'][0]['profil']
        raw = (base / take['journal']).read_bytes()
        need(hashlib.sha256(raw).hexdigest() == take['journal_sha256'], 'profil modifie')
        rows = [r for r in map(json.loads, raw.splitlines()) if r.get('phase') == 'profil_g' and r['pass'] > 0]
        need([(r['pass'], r['k']) for r in rows] ==
             [(p, k) for p in range(1, config['passes']) for k in range(2, 6)], 'profil incomplet')
        cycles = collections.Counter()
        for row in rows:
            for section, data in row['sections'].items():
                cycles[section] += data['cycles']
        profile[group] = dict(instrumented_wall_ns=take['g_ns'], warm_passes=config['passes'] - 1,
            cycles=dict(sorted(cycles.items())),
            census_fraction=(cycles['census_complet'] + cycles['census_sature']) / cycles['total'])
    output['profile_ng00_warm'] = profile
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = replay(args.repo)
    if args.check:
        need(result == json.loads((HERE / 'mesures.json').read_text()), 'mesures differentes')
        print('session_j_mesures_ok: murs K5/K10 et cycles chauds recalcules ; aucun moteur')
    else:
        print(json.dumps(result, sort_keys=True, indent=2))
