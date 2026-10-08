#!/usr/bin/env python3
"""Diagnostic descriptif des horloges A ; pas de moteur ni modèle de gain.
Usage: check.py DEPOT RETOUR_A
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import statistics as st
import subprocess
import sys

HERE = Path(__file__).resolve().parent
C = json.loads((HERE / 'capture.json').read_text())
REPO, RAW = map(Path, sys.argv[1:3])


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def stat(xs):
    return dict(minimum=min(xs), mediane=st.median(xs), maximum=max(xs))


def derived(x):
    e, r = x['etapes_ns'], x['recouvrement']
    ends = x['fins_par_ordre_ns']
    need(len(ends) == 5 and all(len(a) == 5 for a in ends), 'forme des fins')
    need(all(type(t) is int and t >= 0 for a in ends for t in a), 'type fin')
    need(r['fin_g_ns'] == e['G'] and r['queue_ns'] == e['TMVR'], 'liaison G/queue')
    need(r['fin_ns'] == r['fin_g_ns'] + r['queue_ns'], 'fin interne')
    need(r['tour_ns'] >= r['fin_ns'] >= max(map(max, ends)), 'enveloppe interne')
    last = max(map(max, ends))
    terminal = [f'{i+1}:{"GTMVR"[j]}' for i, a in enumerate(ends) for j, t in enumerate(a) if t == last]
    delta = x['wall_ns'] - sum(e[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR'))
    outer = x['wall_ns'] - (e['P'] + e['C'] + r['tour_ns'])
    extra = r['tour_ns'] - r['fin_ns']
    need(e['raccord'] == 0 and delta == extra + outer and outer >= 0, 'identité du résidu')
    return dict(mur_ns=x['wall_ns'], G_ns=e['G'], queue_ns=e['TMVR'],
                residu_ns=delta, tour_moins_fin_ns=extra, transitions_ns=outer,
                fin_moins_dernier_marqueur_ns=r['fin_ns']-last,
                noyau5_moins_g5_ns=ends[4][1]-ends[4][0],
                dernier_moins_noyau5_ns=last-ends[4][1],
                terminal=terminal, fins=ends)


def summarize(rows):
    d = [derived(x) for x in rows]
    keys = [k for k in d[0] if k not in ('terminal', 'fins')]
    return dict(passes=len(rows), sous_100ms=sum(x['wall_ns'] < 100_000_000 for x in rows),
                derniers_marqueurs=dict(sorted(Counter(t for a in d for t in a['terminal']).items())),
                stats={k:stat([a[k] for a in d]) for k in keys},
                fins_medianes_ns=[[st.median(a['fins'][i][j] for a in d) for j in range(5)] for i in range(5)])


def main():
    for path, expected in C['sources'].items():
        b = subprocess.check_output(['git', 'show', C['source_pin']+':'+path], cwd=REPO)
        need(sha(b) == expected, 'source changée')
    b = subprocess.check_output(['git', 'show', C['doc_pin']+':'+C['doc_path']], cwd=REPO)
    need(sha(b) == C['doc_sha256'], 'documentation changée')
    data, raw_bytes = {}, {}
    for path, expected in C['logs'].items():
        b = (RAW / path).read_bytes(); raw_bytes[path] = b
        need(sha(b) == expected, 'journal changé')
        rows = [json.loads(s) for s in b.decode().splitlines()]
        full = [x for x in rows if x['phase'] == 'full']
        n = 74 if '/v12set/' in path else 10
        need(len(full) == n and [x['pass'] for x in full] == list(range(n)), 'passes')
        need(len([x for x in rows if x['phase'] == 'liberation']) == n, 'libérations')
        need(all(x['status'] == 'ok' and x['kmax'] == 5 and x['threads'] == 48 for x in full), 'régime')
        need(all((x.get('etapes_schema') == 'recouvert') == ('apres' in path) for x in full), 'route')
        data[path] = full
    need(len(data) == 36, 'cohorte des journaux')
    result = dict(source_pin=C['source_pin'], journaux=len(data), native_calls=0, ng={}, v12set={})
    for frame in ('ng00', 'ng01', 'ng02'):
        arms = {}
        for arm in ('avant', 'apres'):
            groups = [data[f'journaux/k5/{frame}_{arm}_t{i:02d}.jsonl'][1:] for i in range(5)]
            rows = [x for group in groups for x in group]
            need(all(x['trame'] == frame for x in rows), 'trame')
            arms[arm] = dict(mur_ns=stat([x['wall_ns'] for x in rows]),
                             medianes_processus_ns=[st.median(x['wall_ns'] for x in group) for group in groups])
            if arm == 'apres':
                arms[arm]['diagnostic'] = summarize(rows)
        result['ng'][frame] = arms
    by_frame = {}
    for arm in ('avant', 'apres'):
        groups = [data[f'journaux/v12set/{arm}_r{i}.jsonl'] for i in range(3)]
        names = [x['trame'] for x in groups[0][:37]]
        need(len(set(names)) == 37, '37 noms uniques')
        need(all([x['trame'] for x in group] == (names[i:]+names[:i])*2 for i,group in enumerate(groups)), 'rotation et ordre de visite')
        for group in groups:
            for x in group[37:]:
                by_frame.setdefault((arm, x['trame']), []).append(x)
        rows = [x for group in groups for x in group[37:]]
        medians = [st.median(x['wall_ns'] for x in by_frame[arm, name]) for name in names]
        result['v12set'][arm] = dict(passes=len(rows), medianes_trames_ns=stat(medians),
                                    medianes_trames_sous_100ms=sum(x < 100_000_000 for x in medians),
                                    toutes_passes_sous_100ms=sum(all(x['wall_ns'] < 100_000_000 for x in by_frame[arm,name]) for name in names))
        if arm == 'apres':
            result['v12set'][arm]['diagnostic'] = summarize(rows)
            result['v12set'][arm]['par_trame'] = {name:dict(mur_ns=stat([x['wall_ns'] for x in by_frame[arm,name]]),
                derniers_marqueurs=summarize(by_frame[arm,name])['derniers_marqueurs']) for name in names}
    for path, b in raw_bytes.items():
        need((RAW / path).read_bytes() == b, 'journal devenu mutable')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
