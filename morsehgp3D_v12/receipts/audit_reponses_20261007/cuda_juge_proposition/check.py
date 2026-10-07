#!/usr/bin/env python3
"""Rejoue seulement le pilote et son juge proposes, depuis des sources exactes ; aucun moteur."""
import argparse
import copy
import hashlib
import importlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from contextlib import redirect_stdout

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, label):
    if not condition:
        raise RuntimeError(label)


def test(judge, driver, schema):
    rows = []

    def case(name, expected, mutate=lambda report: None):
        report = judge.good_report()
        mutate(report)
        verdict = judge.judge(report)['verdict']
        require(verdict == expected, name + ': ' + verdict)
        rows.append({'case': name, 'verdict': verdict})

    def missing_stages(report):
        for entry in report['steps']['device_timing_k5']:
            for row in entry['run']['passes']:
                row.pop('stages')

    def missing_identity(report):
        for entry in report['steps']['identity']:
            for path in ('cpu', 'device'):
                for row in entry[path]['passes']:
                    for name in ('balls', 'incidences', 'levels', 'ledger'):
                        row.pop(name)

    def duplicate_process(report):
        for entry in report['steps']['device_timing_k5']:
            entry['process'] = 0

    def mask_mutants(report):
        for entry in report['steps']['mutants']:
            entry.update(critere='temps', timeout=True, code=None)

    def negative(report):
        for entry in report['steps']['device_timing_k5']:
            for row in entry['run']['passes']:
                row['wall_ns'] = -1
                row['stages'] = dict.fromkeys(row['stages'], -1)

    case('conforme_synthetique', 'adopte')
    case('etapes_absentes', 'refuse', missing_stages)
    case('processus_dupliques', 'refuse', duplicate_process)
    case('identite_absente_des_deux_cotes', 'refuse', missing_identity)
    case('un_digest_cpu_sur_dix', 'refuse', lambda r: r['steps']['cpu_timing'][0]['run'].__setitem__('digests',
         r['steps']['cpu_timing'][0]['run']['digests'][:1]))
    case('durees_negatives', 'refuse', negative)
    case('mutants_identite_changes_en_temps', 'refuse', mask_mutants)
    case('sortie_exit_absente', 'refuse', lambda r: r['steps']['identity'][0]['device'].pop('exit'))
    case('premiere_passe_reecritures_perdues', 'rejete', lambda r:
         r['steps']['identity'][0]['device']['passes'][0]['device'].__setitem__('rewritten_device', 0))
    case('temps_passe_intermediaire_reecritures_perdues', 'rejete', lambda r:
         r['steps']['device_timing_k5'][0]['run']['passes'][1]['device'].__setitem__('rewritten_device', 0))
    case('capacite_absente', 'refuse', lambda r:
         r['steps']['device_timing_k5'][0]['run']['passes'][0]['device'].pop('device_bytes'))
    case('type_booleen_duree', 'refuse', lambda r:
         r['steps']['device_timing_k5'][0]['run']['passes'][0].__setitem__('wall_ns', True))
    case('profil_errone', 'refuse', lambda r:
         r['steps']['identity'][0]['device']['passes'][0].__setitem__('coord_bits', 18))
    case('sortie_porte_non_texte', 'refuse', lambda r: r['steps']['gates'].__setitem__('device_open_stdout', 7))
    case('isolation_apres_absente_nouvelle_regle', 'refuse', lambda r: r['steps'].__setitem__('gpu_quiet_after', False))
    case('arret_precoce_sans_etapes', 'refuse', lambda r: r.__setitem__('steps', {}))

    def change_cohort(report):
        report['options'].update(processes=6, passes=11)
        entries = report['steps']['device_timing_k5']
        for entry in list(entries):
            entry['run']['passes'].append(copy.deepcopy(entry['run']['passes'][-1]))
            entry['run']['passes'][-1]['pass'] = 10
            if entry['process'] == 4:
                extra = copy.deepcopy(entry)
                extra['process'] = 5
                entries.append(extra)
    case('cohorte_6x11', 'adopte', change_cohort)
    report = judge.good_report()
    change_cohort(report)
    require(all(row['warm_values'] == 60 for row in judge.judge(report)['stats']['device_k5'].values()),
            'nombre de passes de cohorte')

    # Archive CPU native epinglee : trois passes, deux sites synthetiques, un fil.
    # Le rejeu Python ne lance pas le binaire ; aucune qualification GPU n'en decoule.
    raw = (HERE / 'cpu_format.jsonl').read_text()
    capture = json.loads((HERE / 'capture.json').read_text())
    require(sha(HERE / 'cpu_format.jsonl') == capture['native_format']['stdout_sha256'], 'archive de format')
    parsed = driver.parse_probe(raw)
    run = driver.summary(parsed)
    require(schema.valid_run(run, 3, 'cpu', 5, 24, 1, True), 'archive CPU native conforme')
    for name, variant in [
            ('ligne_null', raw + 'null\n'), ('ligne_tableau', raw + '[]\n'),
            ('ligne_texte', raw + 'garbage\n'),
            ('cle_dupliquee', raw.replace('"phase":"catalogue"', '"phase":"catalogue","phase":"catalogue"', 1)),
            ('constante_NaN', raw.replace('"phase"', '"extra":NaN,"phase"', 1)),
            ('texte_non_ASCII', raw.replace('"phase"', '"extra":"é","phase"', 1)),
            ('remplacement_UTF8', raw.replace('"phase"', '"extra":"\ufffd","phase"', 1)),
            ('unicode_echappe', raw.replace('"phase"', '"extra":"\\u00e9","phase"', 1))]:
        altered = driver.parse_probe(variant)
        require(altered['unreadable'] > 0, name)
        require(not schema.valid_run(driver.summary(altered), 3, 'cpu', 5, 24, 1, True), name + ': refus schema')
        rows.append({'case': name, 'parse': 'illisible', 'valid_run': False})
    for field, value in [('diagnostics', 7), ('device', 'x')]:
        altered_rows = [json.loads(line) for line in raw.splitlines()]
        altered_rows[0][field] = value
        altered = driver.summary(driver.parse_probe('\n'.join(json.dumps(row) for row in altered_rows)))
        require(altered['passes'][0][field] == value, 'structure invalide conservee')
        require(not schema.valid_run(altered, 3, 'cpu', 5, 24, 1, True), 'structure invalide refusee')
        rows.append({'case': field + '_structure_invalide', 'valid_run': False, 'exception': False})
    raw_rows = [json.loads(line) for line in raw.splitlines()]
    for row in raw_rows:
        if row['phase'] == 'catalogue':
            row['path'] = 'device'
    raw_rows.insert(0, {'phase': 'open', 'status': 'ok', 'reason': 'none', 'open_ns': 5})
    text = '\n'.join(json.dumps(row) for row in raw_rows)
    run = driver.summary(driver.parse_probe(text))
    require(schema.valid_run(run, 3, 'device', 5, 24, 1, True), 'adaptation GPU synthetique')
    for name, variant in [('exit_duplique', text + '\n' + json.dumps(raw_rows[-1])),
                          ('digest_avant_catalogue', '\n'.join(json.dumps(raw_rows[i]) for i in (0, 2, 1, 3, 4, 5, 6, 7))),
                          ('catalogue_apres_exit', text + '\n' + json.dumps(raw_rows[1]))]:
        require(driver.parse_probe(variant)['unreadable'] > 0, name)
        rows.append({'case': name, 'parse': 'illisible'})
    variant = copy.deepcopy(parsed)
    variant['passes'] = [copy.deepcopy(parsed['passes'][0]) for _ in range(2)]
    base = copy.deepcopy(driver.summary(variant))
    variant['passes'][0]['device']['rewritten_device'] = 1
    require(driver.summary(variant) != base, 'premiere passe conservee par summary')
    rows.append({'case': 'summary_garde_chaque_passe', 'result': 'distinct'})
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', required=True, type=Path, help='Racine v12 du prototype epingle')
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    sources = capture['source_sha256']
    for relative, expected in sources.items():
        require(sha(args.source_dir / relative.split('/', 1)[1]) == expected, 'source changee : ' + relative)
    with tempfile.TemporaryDirectory(prefix='audit-juge-cuda-') as temporary:
        root = Path(temporary)
        base = root / 'morsehgp3D_v12'
        for relative in capture['proposal_sha256']:
            suffix = relative.split('/', 1)[1]
            if relative not in sources:
                continue
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(args.source_dir / suffix, target)
        applied = subprocess.run(['git', 'apply', str(HERE / 'proposed.patch')], cwd=root, capture_output=True, text=True)
        require(applied.returncode == 0, 'application du patch : ' + applied.stderr)
        for relative, expected in capture['proposal_sha256'].items():
            require(sha(root / relative) == expected, 'proposition changee : ' + relative)
        sys.path.insert(0, str(base / 'bench'))
        judge = importlib.import_module('g4_catalogue_judge')
        driver = importlib.import_module('g4_catalogue_device')
        schema = importlib.import_module('g4_catalogue_schema')
        with redirect_stdout(io.StringIO()) as official:
            require(judge.selftest() == 0, 'auto-injections officielles')
        results = test(judge, driver, schema)
        for relative, expected in sources.items():
            require(sha(args.source_dir / relative.split('/', 1)[1]) == expected, 'source modifiee pendant lecture')
        print(json.dumps({'official': official.getvalue().splitlines(), 'cases': results,
                          'patch_applied': True, 'source_hashes_stable': True,
                          'native_format_capture': 'CPU seulement ; adaptation GPU synthetique', 'engine_executed': False, 'gcp_used': False},
                         indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
