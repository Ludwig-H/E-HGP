#!/usr/bin/env python3
"""Read returned A6 JSON only, close the external plan, then retain absolute FULL times.

No driver main, environment command, build, probe or cloud invocation is used.
The only subprocesses are Git reads and application of the published parser patch
to a temporary copy. No coordinates or serialized forests are opened.
"""
import argparse
import hashlib
import importlib.util
import json
import math
import random
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ARMS = ('avant', 'avant_bis', 'apres')
PILOT = 'morsehgp3D_v12/microbancs/mes_t2d_a6/pilote_t2d_a6.py'
PATCH = 'morsehgp3D_v12/receipts/audit_reponses_20261008/a6_pilote_admission/proposition.patch'


def need(condition, message):
    if not condition:
        raise ValueError(message)


def unique(pairs):
    out = {}
    for key, value in pairs:
        need(key not in out, 'duplicate JSON key')
        out[key] = value
    return out


def load(raw):
    def constant(_):
        raise ValueError('nonfinite JSON')
    return json.loads(raw, object_pairs_hook=unique, parse_constant=constant)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def diff(a, b, path=''):
    if type(a) is not type(b):
        return [{'path': path, 'replay': a, 'worker': b}]
    if isinstance(a, dict):
        need(a.keys() == b.keys(), 'judgment keys ' + path)
        return sum((diff(a[k], b[k], path + '/' + k) for k in a), [])
    if isinstance(a, list):
        need(len(a) == len(b), 'judgment length ' + path)
        return sum((diff(x, y, path + '/' + str(i)) for i, (x, y) in enumerate(zip(a, b))), [])
    return [] if a == b else [{'path': path, 'replay': a, 'worker': b}]


def bootstrap(logs):
    rng = random.Random(20261008)
    means = sorted(sum(logs[rng.randrange(len(logs))] for _ in logs) / len(logs) for _ in range(10000))
    return {'tours': len(logs), 'moyenne_geometrique': math.exp(sum(logs) / len(logs)),
            'ic95': [math.exp(means[250]), math.exp(means[9749])]}


def absolute(groups):
    rows = [row for group in groups for row in group]
    meds = [statistics.median(row['wall_ns'] for row in group) for group in groups]
    wall = [row['wall_ns'] for row in rows]
    fields = {k: statistics.median(row['etapes_ns'][k] for row in rows)
              for k in ('P', 'C', 'G', 'raccord', 'TMVR')}
    return {'processes': len(groups), 'warm_full': len(rows),
            'median_process_medians_ns': statistics.median(meds), 'max_process_median_ns': max(meds),
            'pooled_median_ns': statistics.median(wall), 'max_warm_ns': max(wall),
            'warm_over_100ms': sum(v > 100000000 for v in wall),
            'pooled_stage_medians_ns_not_additive': fields,
            'cpu_median_ns': statistics.median(row['cpu_ns'] for row in rows),
            'active_budget_peak_max_bytes': max(row['pic_octets'] for row in rows),
            'rss_lifetime_max_bytes': max(row['rss_max_octets'] for row in rows),
            'device_peak_max_bytes': max(row['pic_appareil_octets'] for row in rows),
            'pinned_capacity_max_bytes': max(row['epinglee_octets'] for row in rows)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True, type=Path)
    ap.add_argument('--report', required=True, type=Path)
    ap.add_argument('--out', required=True, type=Path)
    args = ap.parse_args()
    meta = load((HERE / 'capture.json').read_bytes())['reader_metadata']
    plan = {'cas': [[c['name'], c['sites']] for c in meta['cohort']], 'essai': False,
            **{name: int(meta['configuration']['--' + name.replace('_', '-')])
               for name in ('fils', 'passes', 'tours', 'tours_grandes')}}
    report_raw = args.report.read_bytes()
    report = load(report_raw)
    journal_root = args.report.parent.resolve()
    patch = subprocess.check_output(['git', '-C', str(args.repo), 'show', 'ce81936fc:' + PATCH])
    patch_pin = digest(patch)
    with tempfile.TemporaryDirectory(prefix='audit-a6-reader-') as name:
        temp = Path(name)
        for path, pin in meta['packages']['after']['critical'].items():
            raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', meta['packages']['after']['source_git'] + ':' + path])
            need(digest(raw) == pin['sha256'] and len(raw) == pin['bytes'], 'source pin')
            target = temp / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(raw)
        original = module(temp / PILOT, 'a6_original')
        old = original.juger(report, str(journal_root), verifier=True, essai=False)
        subprocess.run(['git', 'apply', '--check', '-'], cwd=temp, input=patch, check=True)
        subprocess.run(['git', 'apply', '-'], cwd=temp, input=patch, check=True)
        strict = module(temp / PILOT, 'a6_strict')
        need(strict.verifier_cohorte(report, plan, False) == '', 'external cohort rejected')
        new = strict.juger(report, str(journal_root), verifier=True, essai=False, plan=plan)
        need(old == new, 'closing cohort changed the original judgment')
        all_rows, journal_pins, seen = {}, {}, set()
        full_count = 0
        for take in strict.prises_du_rapport(report):
            relative = take['journal']
            path = (journal_root / relative).resolve()
            need(path.is_relative_to(journal_root) and path not in seen, 'journal path/uniqueness')
            seen.add(path)
            raw = path.read_bytes()
            need(digest(raw) == take['journal_sha256'], 'journal hash')
            expected = {**take['attendu'], 'trames': [tuple(x) for x in take['attendu']['trames']]}
            read = strict.lf.parse_output(take['code'], raw.decode('ascii'), expected)
            need(read['etat'] == take['etat'] == 'ok', 'full admission')
            need(strict.resume(read['passes']) == take['resume'], 'summary versus raw')
            all_rows[relative] = read['passes']
            journal_pins[relative] = {'bytes': len(raw), 'sha256': digest(raw)}
            full_count += len(read['passes'])
        need(len(all_rows) == 85 and full_count == 1306, 'global counts')
        ng, large, stats = {}, {}, {}
        for frame, turns in report['ng']['trames'].items():
            ng[frame] = {arm: absolute([all_rows[t[arm]['journal']][1:] for t in turns]) for arm in ARMS}
            for arm, suffix in (('apres', ''), ('avant_bis', '_aa')):
                logs = [math.log(statistics.median(r['wall_ns'] for r in all_rows[t[arm]['journal']][1:]) /
                                 statistics.median(r['wall_ns'] for r in all_rows[t['avant']['journal']][1:]))
                        for t in turns]
                stats[frame + suffix] = bootstrap(logs)
        large_turns = report['grandes']['tours']
        n = len(meta['large_cohort'])
        for frame in report['grandes']['trames']:
            large[frame] = {}
            for arm in ARMS:
                groups = [[all_rows[t[arm]['journal']][n + t['ordre'].index(frame)]] for t in large_turns]
                large[frame][arm] = absolute(groups)
        for arm, suffix in (('apres', ''), ('avant_bis', '_aa')):
            logs = [sum(math.log(a['wall_ns'] / b['wall_ns']) for a, b in
                        zip(all_rows[t[arm]['journal']][n:], all_rows[t['avant']['journal']][n:])) / n
                    for t in large_turns]
            stats['grandes' + suffix] = bootstrap(logs)
        need(stats == new['cas']['statistiques'], 'independent raw-derived statistics')
        large_summary = {}
        for arm in ARMS:
            frame_meds = [large[f][arm]['median_process_medians_ns'] for f in large]
            large_summary[arm] = {
                'frames': len(frame_meds), 'warm_passes': n * len(large_turns),
                'median_frame_medians_ns': statistics.median(frame_meds),
                'max_frame_median_ns': max(frame_meds),
                'max_warm_ns': max(large[f][arm]['max_warm_ns'] for f in large),
                'frame_medians_over_100ms': sum(v > 100000000 for v in frame_meds),
                'warm_over_100ms': sum(large[f][arm]['warm_over_100ms'] for f in large)}
        output = {'parser_base_git': meta['packages']['after']['source_git'],
                  'report': {'bytes': len(report_raw), 'sha256': digest(report_raw)},
                  'patch_sha256': patch_pin, 'external_plan': plan,
                  'processes': len(all_rows), 'full_passes': full_count, 'decisive_warm': 783,
                  'judgment': new, 'worker_judgment_differences': diff(new, report['jugement']),
                  'journal_pins': journal_pins, 'ng': ng, 'large': large, 'large_summary': large_summary,
                  'elf_initial': {a: report['construction']['binaires'][a]['sha256'] for a in ARMS},
                  'elf_final': report['ng']['binaires_apres'], 'native_calls': 0}
        need(args.report.read_bytes() == report_raw, 'report changed during reading')
        for relative, pin in journal_pins.items():
            need(digest((journal_root / relative).read_bytes()) == pin['sha256'], 'journal changed during reading')
        args.out.write_text(json.dumps(output, ensure_ascii=False, sort_keys=True, indent=2) + '\n')
        print(json.dumps({'processes': 85, 'full_passes': full_count, 'verdict': new['verdict'],
                          'worker_differences': len(output['worker_judgment_differences']),
                          'ng_after_ms': {f: ng[f]['apres']['median_process_medians_ns'] / 1e6 for f in ng},
                          'large_after': large_summary['apres']}, ensure_ascii=False))


if __name__ == '__main__':
    main()
