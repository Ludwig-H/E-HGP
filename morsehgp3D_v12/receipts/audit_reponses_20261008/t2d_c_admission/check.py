#!/usr/bin/env python3
"""Rejeu juge pur + faux lanceur T2d-C ; sources sauvegardees, aucun moteur execute."""
import argparse
import ast
import copy
import difflib
import hashlib
import json
from pathlib import Path
import types

HERE = Path(__file__).resolve().parent
SOURCES = {
    'c8dd': ('audit-t2dc-judge-c8dd9bc9.py', 'c8dd9bc9f236d7b292859b8fcb3c47c6685e125616025db07c8eaa3626286ff2'),
    '14eb': ('audit-t2dc-judge-new.py', '14eb9c8a15428fb7da0dc9f15daef49412caf72384360c4ec6697e562a687463'),
    'pilot': ('audit-t2dc-pilot-cdde6f84.py', 'cdde6f84ff260bcf3717ab2c599588d80e8b0d407e1833e474b3f05c1a830199'),
    'base': ('audit-t2dc-base-c477ffd8.py', 'c477ffd80533a7e770ae382b9062ca1372699b6ea07a4db4178a4a934e7ae677'),
    'fixture': ('cpu_synthetic_stdout.jsonl', '3b6c4261acd93fff79018706d9bd2c9b91e4a1d5fdc519dcf392b1982c9b365e'),
}
GUARD = '''    reference = m.get('reference') or {}
    if not isinstance(reference, dict) or not all(is_hex(reference.get(f)) for f in
            ('digest', 'niveaux_sha256', 'table_sha256')):
        out['refused'].append('mutant appareil : reference incomplete')
        return
    if code == 0:
        ds, fs = run.get('digests'), run.get('complets')
        if not isinstance(ds, list) or not ds or not all(is_hex(d) for d in ds) or \\
                not isinstance(fs, list) or len(fs) != len(ds) or any(
                    not isinstance(f, dict) or not is_hex(f.get('niveaux_sha256')) or
                    not is_hex(f.get('table_sha256')) or not is_int(f.get('table_ecarts')) or
                    not 0 <= f['table_ecarts'] < 2 ** 64 for f in fs):
            out['refused'].append('mutant appareil : comparaison incomplete')
            return
'''


def require(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load(source):
    ns = {'__name__': 'audit_t2dc_judge'}
    exec(compile(source, '<juge epingle>', 'exec'), ns)
    return ns


def functions(source, names, ns=None):
    nodes = [n for n in ast.parse(source).body if isinstance(n, ast.FunctionDef) and n.name in names]
    require(len(nodes) == len(names), 'fonctions absentes')
    if ns is not None:
        exec(compile(ast.Module(body=nodes, type_ignores=[]), '<fonctions originales epinglees>', 'exec'), ns)
    return {n.name: ast.get_source_segment(source, n) for n in nodes}


def summaries(judge, cases):
    rows = []
    for name, report in cases:
        out = judge(report)
        rows.append({'case': name, 'verdict': out['verdict'], 'refused_count': len(out['refused']),
                     'rejected_count': len(out['rejected']), 'first_refusal': next(iter(out['refused']), None),
                     'mutant': out['stats'].get('mutant')})
    return rows


def witnesses(ns):
    good = ns['synthetic_report']()
    cases = [('nominal_du_juge', good)]
    report = copy.deepcopy(good)
    for entry in report['steps']['campaign']:
        for row in entry['run']['passes']:
            row.pop('stages', None)
    cases.append(('toutes_etapes_absentes', report))
    report = copy.deepcopy(good)
    for entry in report['steps']['campaign']:
        for row in entry['run']['passes']:
            row.update(path='cpu', coord_bits=18, kmax=10, threads=1, reason='memory_budget')
            row['pass'] = 0
    cases.append(('voie_et_metadonnees_incompatibles', report))
    report = copy.deepcopy(good)
    report['steps']['mutant']['run'] = {'code': 0, 'digests': [], 'complets': []}
    cases.append(('mutant_code0_sans_empreinte', report))
    return cases


class Fake:
    data = '/synthetic-input-not-read'

    def __init__(self, text):
        self.text = text

    def run(self, name, command, timeout):
        return {'stdout': self.text, 'code': 0, 'timeout': False, 'seconds': 0}


def upstream(text, fixture):
    base = {}
    functions(text['base'], ['unique_object', 'reject_constant'], base)
    ns = {'json': json, 'Path': Path, 'base': types.SimpleNamespace(**{k: base[k] for k in
          ['unique_object', 'reject_constant']}), 'DATA': {'ng00': 'lidar_ng00'},
          'STAGE_DIAG': ('traversal_ns', 'count_ns', 'fill_ns', 'levels_ns', 'sort_ns', 'assemble_ns', 'table_ns')}
    functions(text['pilot'], ['parse', 'probe_run'], ns)
    functions(text['14eb'], ['is_int', 'is_hex', 'run_complete', 'check_mutant'], ns)
    empty = ns['probe_run'](Fake(''), 'witness', 'no-binary-executed', 'ng00', 5, 48,
                            ['--device', '--digest', '--digest-complet', '--passes=2'])
    out = {'refused': [], 'rejected': [], 'stats': {}}
    ns['check_mutant']({'mutant': {'applied': True, 'built': True, 'run': empty,
        'reference': {'digest': 'a' * 64, 'niveaux_sha256': 'b' * 64, 'table_sha256': 'c' * 64}}}, out)
    cpu = ns['probe_run'](Fake(fixture.decode()), 'witness', 'no-binary-executed', 'ng00', 5, 48,
                          ['--device', '--passes=3'])
    result = {
        'empty_stdout': {'unreadable': empty['unreadable'], 'code': empty['code'], 'digests': empty['digests'],
                         'complets': empty['complets'], 'judge': out},
        'cpu_fixture_as_device': {'requested_path': 'device', 'requested_threads': 48,
            'actual_paths': sorted({r['path'] for r in cpu['passes']}),
            'actual_threads': sorted({r['threads'] for r in cpu['passes']}),
            'passes': len(cpu['passes']), 'unreadable': cpu['unreadable'],
            'run_complete': ns['run_complete'](cpu, 3, False)}}
    require(out == {'refused': [], 'rejected': [], 'stats': {'mutant': 'tue'}}, 'temoin mutant amont non reproduit')
    require(result['cpu_fixture_as_device'] == {'requested_path': 'device', 'requested_threads': 48,
            'actual_paths': ['cpu'], 'actual_threads': [1], 'passes': 3, 'unreadable': 0, 'run_complete': True},
            'temoin metadonnees amont non reproduit')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay', type=Path, required=True, help='dossier de sauvegarde des cinq entrees epinglees')
    parser.add_argument('--capture', action='store_true')
    args = parser.parse_args()
    raw = {key: (args.replay / filename).read_bytes() for key, (filename, _) in SOURCES.items()}
    require(all(sha(raw[k]) == pin for k, (_, pin) in SOURCES.items()), 'source ou fixture non epinglee')
    text = {key: data.decode() for key, data in raw.items() if key != 'fixture'}
    cases, pure = {}, {}
    for key in ['c8dd', '14eb']:
        ns = load(text[key])
        cases[key] = witnesses(ns)
        pure[key] = summaries(ns['judge'], cases[key])
    require([r['verdict'] for r in pure['c8dd']] == ['adopte'] * 4, 'temoins c8dd non reproduits')
    require([r['verdict'] for r in pure['14eb']] == ['adopte', 'refuse', 'adopte', 'adopte'],
            'evolution 14eb non reproduite')
    anchor = "    reference = m.get('reference') or {}\n    differs = code != 0"
    require(text['14eb'].count(anchor) == 1, 'ancre du correctif non unique')
    proposed = text['14eb'].replace(anchor, GUARD + '    differs = code != 0')
    fixed = summaries(load(proposed)['judge'], [cases['14eb'][0], cases['14eb'][-1]])
    require([r['verdict'] for r in fixed] == ['adopte', 'refuse'], 'correctif cible non confirme')
    patch = ''.join(difflib.unified_diff(text['14eb'].splitlines(True), proposed.splitlines(True),
        fromfile='a/bench/g4_catalogue_flux_judge.py', tofile='b/bench/g4_catalogue_flux_judge.py'))
    result = {'pins': {k: {'file': n, 'bytes': len(raw[k]), 'sha256': h} for k, (n, h) in SOURCES.items()},
              'native_executed': False, 'gcp_used': False, 'pure_judge': pure,
              'upstream': upstream(text, raw['fixture']), 'partial_fix_against_14eb': fixed,
              'patched_sha256': sha(proposed.encode()),
              'campaign_processes': len(cases['c8dd'][0][1]['steps']['campaign']),
              'campaign_passes': sum(len(e['run']['passes']) for e in cases['c8dd'][0][1]['steps']['campaign'])}
    capture = {'result': result, 'causal_source_excerpts': {
        'c8dd': functions(text['c8dd'], ['run_complete', 'stage_values', 'check_mutant']),
        '14eb_stage_values': functions(text['14eb'], ['stage_values']),
        'pilot': functions(text['pilot'], ['parse', 'probe_run'])}}
    require(all((args.replay / SOURCES[k][0]).read_bytes() == b for k, b in raw.items()), 'source modifiee pendant lecture')
    if args.capture:
        require(not (HERE / 'capture.json').exists(), 'capture deja presente : ne pas ecraser un recu historique')
        (HERE / 'capture.json').write_text(json.dumps(capture, ensure_ascii=False, indent=2, sort_keys=True) + '\n')
        (HERE / 'mutant_comparaison_complete.patch').write_text(patch)
    else:
        require(capture == json.loads((HERE / 'capture.json').read_text()), 'resultat different de la capture')
        require((HERE / 'mutant_comparaison_complete.patch').read_text() == patch, 'patch different')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))


if __name__ == '__main__':
    main()
