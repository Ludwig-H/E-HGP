#!/usr/bin/env python3
"""Contre-calcul independant des 111 chaudes A/37, apres admission ; aucun moteur."""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import statistics as st

PINS = {'journaux/v12set/apres_r0.jsonl': '12ec9f78635334ea02e0df160b53a97caf627fa5b7834ce034bb767da2c48a1f', 'journaux/v12set/apres_r1.jsonl': '7ca55b086a01a9ff1531f14b7e01778957e23d69eba28b793b90520d3d654c18', 'journaux/v12set/apres_r2.jsonl': '29950857e4f198a859e7899ccf307d5e4ec2a06832b601f873e276d940fbadd2'}
PHASES = ('G', 'noyau', 'M', 'V', 'R')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--returned', type=Path, required=True)
    a = ap.parse_args()
    frames = defaultdict(list)
    phase_ends = {key: Counter() for key in PHASES}
    final_end = Counter()
    for name, expected in PINS.items():
        raw = (a.returned / name).read_bytes()
        need(hashlib.sha256(raw).hexdigest() == expected, 'brut different')
        rows = [json.loads(line) for line in raw.splitlines() if line.startswith(b'{')]
        full = [r for r in rows if r.get('phase') == 'full']
        need(len(full) == 74 and [r['pass'] for r in full] == list(range(74)), 'cohorte de passes')
        need([r['trame'] for r in full[:37]] == [r['trame'] for r in full[37:]], 'deux tours de37')
        for row in full[37:]:
            et, rc, ends = (row[k] for k in ('etapes_ns', 'recouvrement', 'fins_par_ordre_ns'))
            need(row['kmax'] == 5 and row['threads'] == 48 and len(ends) == 5 and
                 all(len(x) == 5 for x in ends), 'profil ou marqueurs')
            last = max(max(x) for x in ends)
            need(max(x[0] for x in ends) == rc['fin_g_ns'] == et['G'], 'finG')
            need(last <= rc['fin_ns'] <= rc['tour_ns'] and
                 rc['queue_ns'] == max(0, rc['fin_ns'] - rc['fin_g_ns']), 'fins/queue')
            d = dict(wall=row['wall_ns'], P=et['P'], C=et['C'], G=et['G'], queue=rc['queue_ns'],
                     tour=rc['tour_ns'], ouverture=rc['ouverture_ns'],
                     tour_moins_fin=rc['tour_ns'] - rc['fin_ns'],
                     fin_moins_dernier_marqueur=rc['fin_ns'] - last,
                     reste_mur=row['wall_ns'] - et['P'] - et['C'] - rc['tour_ns'],
                     reste_partition=row['wall_ns'] - sum(et.values()))
            need(d['reste_mur'] >= 0 and
                 d['reste_partition'] == d['tour_moins_fin'] + d['reste_mur'], 'residus')
            frames[row['trame']].append(d)
            for j, phase in enumerate(PHASES):
                value = max(x[j] for x in ends)
                key = '+'.join(str(i + 1) for i, x in enumerate(ends) if x[j] == value)
                phase_ends[phase][key] += 1
            key = '+'.join('%d:%s' % (i + 1, PHASES[j]) for i, x in enumerate(ends)
                           for j, value in enumerate(x) if value == last)
            final_end[key] += 1
    need(len(frames) == 37 and all(len(rows) == 3 for rows in frames.values()), 'trois chaudes par trame')
    flat = [r for rows in frames.values() for r in rows]
    def describe(key):
        medians = [st.median(r[key] for r in rows) for rows in frames.values()]
        return dict(mediane_111_ns=st.median(r[key] for r in flat),
                    mediane_37_medianes_ns=st.median(medians), maximum_37_medianes_ns=max(medians),
                    maximum_111_ns=max(r[key] for r in flat))
    result = dict(scope='37 trames, 3 processus, seconde visite seule : 111 chaudes apres',
                  execution_native=False, pins=PINS, derniers_ordres_par_phase=phase_ends,
                  dernier_marqueur_global=final_end, statistiques_ns={k: describe(k) for k in flat[0]},
                  au_dessus_100ms=dict(passes=sum(r['wall'] > 100_000_000 for r in flat),
                    medianes_trames=sum(st.median(r['wall'] for r in rows) > 100_000_000 for rows in frames.values()),
                    trames_avec_depassement=sum(max(r['wall'] for r in rows) > 100_000_000 for rows in frames.values())))
    for name, expected in PINS.items():
        need(hashlib.sha256((a.returned / name).read_bytes()).hexdigest() == expected, 'brut mobile')
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
