#!/usr/bin/env python3
"""Diagnostic MES-C K5 : seulement les six journaux complets, aucun moteur."""
import argparse
from fractions import Fraction
import hashlib
import importlib.util
import json
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent
STAGES = ('P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R')
SUBC = ('parcours', 'feuilles', 'emission', 'fin_etage', 'transferts', 'publication')


def need(ok, message):
    if not ok:
        raise ValueError(message)


def pinned(path, pin):
    raw = path.read_bytes()
    need(len(raw) == pin['bytes'] and hashlib.sha256(raw).hexdigest() == pin['sha256'], 'pin ' + path.name)
    return raw


def median(values):
    return statistics.median(values)


def metrics(row):
    out = dict(wall=row['wall_ns'], cpu=row['cpu_ns'], **row['etapes_ns'])
    out.update({'C/' + k: row['c_ns'][k] for k in SUBC})
    out['sans_C'] = out['wall'] - out['C']
    out['reste_mur'] = out['wall'] - sum(out[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR'))
    out['reste_C'] = out['C'] - sum(row['c_ns'].values())
    out['cpu/mur'] = Fraction(out['cpu'], out['wall'])
    out['C/mur'] = Fraction(out['C'], out['wall'])
    need(out['reste_mur'] >= 0 and out['reste_C'] >= 0, 'fenetres disjointes sur ces bruts')
    return out


def per_case(function, cases, samples):
    # Differences/ratios formed on matching (cloud, warm turn), before either aggregation.
    return {c['etiquette']: median(function(samples[c['etiquette']][j]) for j in range(2)) for c in cases}


def ms(value):
    return round(float(value) / 1_000_000, 9)


def med(samples, cases, key):
    return median(per_case(lambda r: r[key], cases, samples).values())


def table(columns, rows):
    return dict(columns=columns, rows=rows)


def run(args):
    cap = json.loads((HERE / 'capture.json').read_text())
    for name, pin in cap['sources'].items():
        raw = subprocess.check_output(['git', '-C', args.repo, 'show', cap['source_git'] + ':morsehgp3D_v12/' + name])
        need(len(raw) == pin['bytes'] and hashlib.sha256(raw).hexdigest() == pin['sha256'], 'source ' + name)
    reader_path = args.reader or HERE.parent / 'mes_c_contrelecture/reader.py'
    pinned(reader_path, cap['reader'])
    admission_cap = json.loads(pinned(reader_path.with_name('capture.json'), cap['reader_capture']))
    spec = importlib.util.spec_from_file_location('pinned_mes_c_reader', reader_path)
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    lf, reasons = reader.load_sources(args.repo, admission_cap)
    cases = [c for c in reader.cases_of(admission_cap) if c['groupe'] not in reader.HARD]
    real = [c for c in cases if c['groupe'] == 'reel']
    need(len(cases) == 147 and len(real) == 132, 'cohorte')
    samples = {}
    hashes = {}
    for spec in reader.specifications(admission_cap):
        if spec['kind'] != 'session' or spec['k'] != 5:
            continue
        raw = pinned(args.bruts / spec['file'], cap['raws'][spec['file']])
        # Code zero is inferred from the pinned pilot/report, not a separately archived process code.
        state = reader.strict_process(0, raw.decode('ascii'), spec, admission_cap, lf, reasons)
        need(state['etat'] == 'ok' and len(state['passes']) == 441, 'Session K5 complete')
        key = spec['voie'] + ':' + str(spec['fils'])
        samples[key] = {}
        for i, case in enumerate(cases):
            mine = state['passes'][i::147]
            need(len(mine) == 3, 'tours par cas')
            hashes.setdefault(case['etiquette'], set()).update(r['full_sha256'] for r in mine)
            samples[key][case['etiquette']] = [metrics(r) for r in mine[1:]]
    need(len(samples) == 6 and all(len(h) == 1 for h in hashes.values()), 'identites communes')
    outer, subc, paired, threads, bins, bin_deltas = [], [], [], [], [], []
    outer_names = ('wall', 'P', 'C', 'G', 'TMVR', 'T', 'M', 'V', 'R', 'sans_C')
    pair_names = ('wall', 'P', 'C', 'G', 'TMVR', 'sans_C') + tuple('C/' + k for k in SUBC)
    for group, chosen in [('reel', real), ('total', cases)]:
        for key, rows in samples.items():
            outer.append([group, len(chosen), key] + [ms(med(rows, chosen, k)) for k in outer_names] +
                         [round(float(med(rows, chosen, 'C/mur')) * 100, 6),
                          round(float(med(rows, chosen, 'cpu/mur')), 6)])
            subc.append([group, key] + [ms(med(rows, chosen, 'C/' + k)) for k in SUBC] +
                        [ms(med(rows, chosen, 'reste_C'))])
        for w in (1, 4, 48):
            cpu, gpu = samples['cpu:' + str(w)], samples['appareil:' + str(w)]
            delta = {c['etiquette']: [{k: cpu[c['etiquette']][i][k] - gpu[c['etiquette']][i][k]
                                      for k in pair_names} for i in range(2)] for c in chosen}
            values = per_case(lambda x: x['wall'], chosen, delta)
            ratio = {c['etiquette']: [{'ratio': Fraction(cpu[c['etiquette']][i]['wall'],
                                                       gpu[c['etiquette']][i]['wall'])}
                                     for i in range(2)] for c in chosen}
            # Additive mean decomposition is not a sum of medians.
            mean_wall = sum(values.values()) / len(chosen)
            mean_c = sum(per_case(lambda x: x['C'], chosen, delta).values()) / len(chosen)
            paired.append([group, w, sum(v > 0 for v in values.values()),
                           round(float(med(ratio, chosen, 'ratio')), 6)] +
                          [ms(med(delta, chosen, k)) for k in pair_names] +
                          [ms(mean_wall), ms(mean_c), ms(mean_wall - mean_c)])
        for mode in ('cpu', 'appareil'):
            for before, after in ((1, 4), (4, 48)):
                a, b = samples[mode + ':' + str(before)], samples[mode + ':' + str(after)]
                keys = ('wall', 'C', 'G', 'TMVR') + tuple('C/' + k for k in SUBC)
                delta = {c['etiquette']: [{k: b[c['etiquette']][i][k] - a[c['etiquette']][i][k]
                                          for k in keys} for i in range(2)] for c in chosen}
                dw = per_case(lambda x: x['wall'], chosen, delta)
                dc = per_case(lambda x: x['C'], chosen, delta)
                threads.append([group, mode, before, after, sum(v > 0 for v in dw.values()),
                                sum(v > 0 for v in dc.values())] + [ms(med(delta, chosen, k)) for k in keys])
    for low, high in ((100, 300), (301, 1000), (1001, 3000), (3001, 10000)):
        chosen = [c for c in real if low <= c['sites'] <= high]
        for key, rows in samples.items():
            bins.append([f'{low}..{high}', len(chosen), key] +
                        [ms(med(rows, chosen, k)) for k in ('wall', 'C', 'G', 'TMVR')] +
                        [ms(med(rows, chosen, 'C/' + k)) for k in ('parcours', 'feuilles', 'emission', 'fin_etage')])
        for mode in ('cpu', 'appareil'):
            a, b = samples[mode + ':4'], samples[mode + ':48']
            delta = {c['etiquette']: [{k: b[c['etiquette']][i][k] - a[c['etiquette']][i][k]
                                      for k in ('wall', 'C', 'G', 'TMVR')} for i in range(2)] for c in chosen}
            dw = per_case(lambda x: x['wall'], chosen, delta)
            bin_deltas.append([f'{low}..{high}', len(chosen), mode, sum(v > 0 for v in dw.values())] +
                              [ms(med(delta, chosen, k)) for k in ('wall', 'C', 'G', 'TMVR')])
    return dict(source=cap['source_git'], scope=dict(K=5, configurations=6, cases=147, real=132,
                passes_complete=2646, warm_used=1764, warm_per_case=2, new_engine_runs=0),
                medians_ms=table(['groupe', 'nuages', 'voie:fils'] + list(outer_names) + ['C_share_percent', 'cpu_over_wall'], outer),
                c_medians_ms=table(['groupe', 'voie:fils'] + list(SUBC) + ['reste_C'], subc),
                cpu_minus_device=table(['groupe', 'fils', 'device_faster_cases', 'median_paired_cpu_over_device'] +
                  ['median_delta_' + k + '_ms' for k in pair_names] +
                  ['mean_delta_wall_ms', 'mean_delta_C_ms', 'mean_delta_sans_C_ms'], paired),
                threads_after_minus_before_ms=table(['groupe', 'voie', 'avant', 'apres', 'FULL_slower_cases', 'C_slower_cases'] +
                  ['median_delta_' + k for k in ('wall', 'C', 'G', 'TMVR') + tuple('C/' + k for k in SUBC)], threads),
                real_size_bins_medians_ms=table(['sites', 'nuages', 'voie:fils', 'wall', 'C', 'G', 'TMVR',
                  'C/parcours', 'C/feuilles', 'C/emission', 'C/fin_etage'], bins),
                real_size_bins_W48_minus_W4_ms=table(['sites', 'nuages', 'voie', 'FULL_slower_cases',
                  'median_delta_wall', 'median_delta_C', 'median_delta_G', 'median_delta_TMVR'], bin_deltas))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--bruts', type=Path, required=True)
    parser.add_argument('--reader', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = run(args)
    if args.check:
        need(result == json.loads((HERE / 'results.json').read_text()), 'resultats differents')
        print('MES-C K5: six Sessions, 147 nuages/132 reels, 1764 passes chaudes ; tableaux verifies')
    else:
        print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))


if __name__ == '__main__':
    main()
