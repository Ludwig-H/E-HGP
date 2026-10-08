#!/usr/bin/env python3
"""Arithmétique de la publication L1/L1R ; métadonnées seules."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REV = '302dc9fe675816b02c24507aa9a1e1915dfa5dae'
PREFIX = 'morsehgp3D_v12/receipts/g4_mesb1r_20261008/'
PATHS = ['resultats/cmd/000_archive_l1/files/l1_archive/extrait/results/cmd/000_mes_b/files/b/',
         'resultats/cmd/001_mes_b/files/b/']


def need(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', default='/workspaces/E-HGP')
    p.add_argument('--admissions', type=Path, required=True)
    a = p.parse_args()
    pins = {}
    def read(rel):
        raw = subprocess.check_output(['git', '-C', a.repo, 'show', REV + ':' + PREFIX + rel])
        pins[rel] = hashlib.sha256(raw).hexdigest()
        return raw.decode()
    read('README.md')
    reports = [json.loads(read(x + 'rapport_b.json')) for x in PATHS]
    summaries, agreements = [], []
    for index, (report, admission_name) in enumerate(zip(reports, ('session_l1_admission', 'session_l1r_admission'))):
        raw = (a.admissions / admission_name / 'mesures.json').read_bytes()
        pins[admission_name + '/mesures.json'] = hashlib.sha256(raw).hexdigest()
        admitted = json.loads(raw)
        need(len(report['cas']) == len(admitted['cas']) == 18, 'cohorte')
        for case, other in zip(report['cas'], admitted['cas']):
            for key in ('nom', 'k', 'voie', 'sites', 'etat', 'empreinte'):
                need(case[key] == other[key], 'cas différent : ' + key)
            need(len(case['passes']) == other['passes_completes'], 'passes différentes')
            if case['passes']:
                last, stats = case['passes'][-1], other['statistiques']
                for native, saved in (('wall_ns', 'derniere_ns'), ('cpu_ns', 'cpu_derniere_ns'),
                                      ('appareil_octets', 'device_capacity_last_octets'),
                                      ('epinglee_octets', 'pinned_capacity_last_octets')):
                    need(last[native] == stats[saved], 'mesure différente : ' + native)
                need(max(x['rss_max_octets'] for x in case['passes']) == stats['rss_max_octets'], 'RSS différent')
                need(max(x['pic_octets'] for x in case['passes']) == stats['hote_peak_octets'], 'pic hôte différent')
                need(last['etapes_ns'] == other['etapes_derniere_ns'], 'étapes différentes')
                if index:
                    need(last['memoire_octets'] == other['memoire_derniere_octets'], 'mémoire par étage différente')
            else:
                name = f"{case['nom']}_k{case['k']}_{case['voie']}.jsonl"
                rows = [json.loads(x) for x in read(PATHS[index] + 'brut/' + name).splitlines()]
                need([x['phase'] for x in rows] == ['open', 'exit'], 'refus avec autre preuve')
                need(set(rows[-1]) == {'phase', 'status', 'reason'} and
                     rows[-1]['reason'] == 'memory_budget', 'refus annoté différemment')
        summary = dict(processes=18, scenes=len({x['nom'] for x in report['cas']}),
                       successful_processes=sum(x['etat'] == 'ok' for x in report['cas']),
                       refusals=sum(x['etat'] == 'refus' for x in report['cas']),
                       full_passes=sum(len(x['passes']) for x in report['cas']),
                       warm_passes=sum(max(0, len(x['passes'])-1) for x in report['cas']),
                       criteria={k: v['etat'] for k, v in report['criteres'].items()})
        summaries.append(summary)
        agreements.append(admission_name)
    ratios, residuals = [], []
    for old, new in zip(reports[0]['cas'], reports[1]['cas']):
        if new['passes']:
            r = new['passes'][-1]
            ratios.append((old['passes'][-1]['wall_ns']/r['wall_ns']-1)*100)
            st = r['etapes_ns']
            residuals.append(dict(name=new['nom'], voie=new['voie'],
                                  tmvr_ns=st['TMVR']-sum(st[k] for k in ('T', 'M', 'V', 'R'))))
    time = read('resultats/cmd/001_mes_b/time.txt')
    kib = int(re.search(r'Maximum resident set size \(kbytes\): (\d+)', time)[1])
    rss = max(row['rss_max_octets'] for c in reports[1]['cas'] for row in c['passes'])
    need(kib*1024 == rss, 'unités RSS')
    src_diff = subprocess.check_output(['git', '-C', a.repo, 'diff', '--name-only', '403736300', 'ea62cd691',
                                       '--', 'morsehgp3D_v12/src'], text=True)
    need(not src_diff, 'moteur différent')
    out = dict(commit=REV, pins=pins, agrees_with_admission_columns=agreements, sessions=summaries,
               host_budget_bytes=reports[1]['parametres']['budget_octets'],
               device_budget_bytes=reports[1]['parametres']['budget_appareil_octets'],
               rss_bytes=rss, rss_decimal_GB=rss/10**9, rss_GiB=rss/2**30,
               last_wall_ratio_L1_over_L1R_percent=[min(ratios), max(ratios)], tmvr_residuals=residuals,
               refusal_budget_side='not_present_in_raw_output', engine_sources_equal=True)
    out['pins'] = {k: v for k, v in pins.items() if '/brut/' not in k}
    out['tmvr_residuals'] = dict(min_ns=min(x['tmvr_ns'] for x in residuals),
                               max_ns=max(x['tmvr_ns'] for x in residuals),
                               scion_sans_sol_ns=next(x['tmvr_ns'] for x in residuals
                                                     if x['name'] == 'forinst_scion_plot61_sans_sol'))
    capture = Path(__file__).with_name('capture.json')
    if capture.exists():
        need(out == json.loads(capture.read_text())['result'], 'résultat différent de la capture')
    print(json.dumps(out, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
