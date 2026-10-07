#!/usr/bin/env python3
"""Contre-JSON du vrai juge capturé ; aucun outil CUDA, moteur, build ou donnée réelle.

--source-dir désigne les deux scripts épinglés g4_catalogue_{judge,device}.py.
"""
import argparse
import copy
import hashlib
import importlib
import json
import pathlib
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', type=pathlib.Path, required=True)
    args = parser.parse_args()
    here = pathlib.Path(__file__).resolve().parent
    capture = json.loads((here / 'capture.json').read_text())
    names = ('g4_catalogue_judge.py', 'g4_catalogue_device.py')
    sources = {n: (args.source_dir / n).read_bytes() for n in names}
    hashes = {n: hashlib.sha256(data).hexdigest() for n, data in sources.items()}
    require(hashes == capture['scripts_sha256'], 'sources différentes de la capture')
    cases = {}
    with tempfile.TemporaryDirectory(prefix='audit-cuda-judge-') as folder:
        temp = pathlib.Path(folder)
        for name, data in sources.items():
            (temp / name).write_bytes(data)
        sys.path.insert(0, str(temp))
        judge = importlib.import_module('g4_catalogue_judge')
        driver = importlib.import_module('g4_catalogue_device')

        def probe(name, mutate, expected):
            report = judge.good_report()
            mutate(report)
            result = judge.judge(report)
            require(result['verdict'] == expected, (name, result['verdict']))
            cases[name] = dict(verdict=result['verdict'], refused=result['refused'], rejected=result['rejected'])

        def runs(report):
            return report['steps']['device_timing_k5'] + report['steps']['device_timing_k10']

        def missing_stages(report):
            for run in runs(report):
                for row in run['run']['passes']:
                    row.pop('stages')

        def duplicate_processes(report):
            for run in runs(report):
                run['process'] = 0

        def missing_counts(report):
            for entry in report['steps']['identity']:
                for key in ('cpu', 'device'):
                    for row in entry[key]['passes']:
                        for field in ('balls', 'incidences', 'levels', 'ledger'):
                            row.pop(field)

        def one_digest(report):
            for entry in report['steps']['cpu_timing']:
                entry['run']['digests'] = entry['run']['digests'][:1]

        def negative_times(report):
            for run in runs(report):
                for row in run['run']['passes']:
                    row['wall_ns'] = -1
                    row['stages'] = {key: -1 for key in row['stages']}

        def changed_mutant_criterion(report):
            for mutant in report['steps']['mutants']:
                for field in ('unit_code', 'ng00_code', 'ng00_digest', 'reference_digest', 'run', 'code'):
                    mutant.pop(field, None)
                mutant.update(critere='temps', timeout=True)

        def boundary(report):
            for run in report['steps']['device_timing_k5']:
                for row in run['run']['passes']:
                    row['wall_ns'] = 45_000_000

        def one_slow_process(report):
            for row in report['steps']['device_timing_k5'][0]['run']['passes']:
                row['wall_ns'] = 45_000_001

        mutations = [
            ('baseline', lambda report: None, 'adopte'),
            ('budget_exactly_45ms', boundary, 'adopte'),
            ('one_process_median_45ms_plus_1ns', one_slow_process, 'rejete'),
            ('missing_all_stages', missing_stages, 'adopte'),
            ('duplicated_process_ids', duplicate_processes, 'adopte'),
            ('missing_identity_counts_ledger', missing_counts, 'adopte'),
            ('cpu_one_digest_for_ten_passes', one_digest, 'adopte'),
            ('negative_times', negative_times, 'adopte'),
            ('mutant_identity_relabelled_timeout', changed_mutant_criterion, 'adopte'),
            ('gpu_busy_after', lambda report: report['steps'].update(gpu_quiet_after=False), 'adopte'),
        ]
        for name, mutate, expected in mutations:
            probe(name, mutate, expected)

        # Exerce le vrai parseur et summary : la première passe perd une réécriture,
        # puis le pilote produit exactement le même résumé que pour les trois passes correctes.
        raw = [dict(phase='catalogue', status='ok', reason='none', wall_ns=38_000_000,
                    balls=10, incidences=40, levels=9, ledger={'nodes': 5},
                    diagnostics=copy.deepcopy(judge.FAKE_DIAG), device=copy.deepcopy(judge.FAKE_DEVICE))
               for _ in range(3)]
        summarize = lambda rows: driver.summary(driver.parse_probe('\n'.join(json.dumps(row) for row in rows)))
        good = summarize(raw)
        raw[0]['device']['rewritten_device'] = 0
        bad = summarize(raw)
        require(good == bad, 'le résumé conserve maintenant la première passe : capture dépassée')
        cases['first_pass_rewrite_loss_discarded'] = dict(summaries_identical=True,
                                                        changed_pass=0, physical_passes_kept=1)
        require(sources == {name: (temp / name).read_bytes() for name in names}, 'copie temporaire modifiée')
    require(sources == {name: (args.source_dir / name).read_bytes() for name in names}, 'sources modifiées')
    print(json.dumps(dict(scripts_sha256=hashes, cases=cases, engine_run=False, gpu_run=False,
                          scope='JSON inventés ; fonctions du juge et du pilote capturés'),
                     sort_keys=True, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
