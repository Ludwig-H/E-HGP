#!/usr/bin/env python3
"""Contre-audit M5/M6 : vrais pilotes, frontieres externes simulees, aucun GPU.

Les fabriques officielles fournissent des bases completes. Mutations propres a
l'auditeur pour M6 ; avant/apres causal des corrections historiques M5/M6.
Les rapports temporaires sont detruits ; seules les observations utiles sortent.
"""
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types
from unittest.mock import patch

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
MICRO = ROOT / 'morsehgp3D_v12/microbancs'
PIN = 'f601b36ac'
BEFORE = '2b2113264^'


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def git_bytes(revision, relative):
    return subprocess.check_output(['git', 'show', revision + ':' + relative], cwd=ROOT)


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def old_m6(code):
    module = types.ModuleType('old_m6_audit')
    module.__file__ = str(MICRO / 'mes_m6_session/run_m6.py')
    exec(compile(code, module.__file__, 'exec'), module.__dict__)
    return module


def m5_causal(old_code):
    factory = load(MICRO / 'mes_m5_parcours/tests/test_juge_m5.py', 'm5_reprise_factory')
    original = factory.charger
    selected = [factory.INJECTIONS[0]] + factory.INJECTIONS[26:34]
    result = []
    for version in ('before', 'after'):
        def loader(path, name):
            if version == 'before' and Path(path).name == 'g4_traversal_bench.py':
                # La fabrique copie le microbanc dans /tmp/juge-m5-*/ avant tout.
                need(str(path).startswith(tempfile.gettempdir() + '/juge-m5-'), 'copie non temporaire')
                Path(path).write_bytes(old_code)
            return original(path, name)
        with patch.object(factory, 'charger', side_effect=loader), patch.object(
                subprocess, 'run', side_effect=RuntimeError('commande externe interdite')):
            for name, scenario, options, _ in selected:
                code, report, _journal = factory.jouer(dict(scenario, nom=name), options)
                expected = 'adopte' if version == 'before' or name == 'temoin_complet' else 'refuse'
                need(code == 0 and report['verdict'] == expected, (version, name, code, report['verdict']))
                result.append({'version': version, 'cas': name, 'code': code, 'verdict': report['verdict'],
                               'refus': len(report['refused'])})
    return result


def m6_tests(old_code):
    factory = load(MICRO / 'mes_m6_session/tests/test_juge_m6.py', 'm6_reprise_factory')
    observations = []
    with tempfile.TemporaryDirectory(prefix='audit-reprise-m6-') as temporary, patch.object(
            subprocess, 'run', side_effect=RuntimeError('commande externe interdite')):
        code, _stdout, base, folder, _simulation = factory.passage(Path(temporary), 'base')
        need(code == 0 and len(base['runs']) == 9, 'base M6 refusee')
        payloads = {p.name: p.read_bytes() for p in folder.glob('m6_*.jsonl')}
        need(len(payloads) == 9, 'neuf prises de base requises')

        historical = [
            ('provenance_vide', lambda r: r.update(binary_sha256='', sources_sha256={})),
            ('isolation_contredite', lambda r: r['runs'][0]['isolation_before'].update(
                code=9, processes='processus_synthetique', quiet=True)),
            ('refus_explicite', lambda r: r.update(refusals=['binaire modifie pendant les prises']))]
        for version in ('before', 'after'):
            for name, mutate in historical:
                report = copy.deepcopy(base)
                mutate(report)
                (folder / 'm6_report.json').write_text(json.dumps(report))
                if version == 'before':
                    with patch.object(factory, 'charger', side_effect=lambda: old_m6(old_code)):
                        code, answer = factory.rejuger(folder)
                else:
                    code, answer = factory.rejuger(folder)
                need(code == (0 if version == 'before' else 3), (version, name, code))
                observations.append({'version': version, 'cas': name, 'code': code,
                                     'verdict': answer['verdict'], 'prises': answer['prises'],
                                     'refus': len(answer['refus'])})

        cases = ('temoin_complet', 'indice_false', 'indice_float', 'indices_float_sans_fichiers',
                 'schema_inconnu_refus_explicite', 'code_compilation_false', 'code_prise_false')
        for name in cases:
            report = copy.deepcopy(base)
            for filename, data in payloads.items():
                (folder / filename).write_bytes(data)
            if name == 'indice_false':
                report['runs'][0]['process'] = False
            elif name == 'indice_float':
                report['runs'][0]['process'] = 0.0
            elif name == 'indices_float_sans_fichiers':
                for row in report['runs']:
                    row['process'] = float(row['process'])
                report['summary_median_us'] = {mode: {} for mode in ('spin', 'yield', 'blocking')}
                for filename in payloads:
                    (folder / filename).unlink()
            elif name == 'schema_inconnu_refus_explicite':
                report.update(schema='ehgp.v12.mes_m6.v999', refusals=['binaire modifie pendant les prises'])
            elif name == 'code_compilation_false':
                report['compile']['code'] = False
            elif name == 'code_prise_false':
                report['runs'][0]['code'] = False
            (folder / 'm6_report.json').write_text(json.dumps(report))
            code, answer = factory.rejuger(folder)
            expected_takes = 0 if name == 'indices_float_sans_fichiers' else (
                8 if name in ('indice_false', 'indice_float') else 9)
            need(code == 0 and answer['verdict'] == 'mes_m6_ok' and answer['prises'] == expected_takes,
                 (name, code, answer))
            observations.append({'version': 'after', 'cas': name, 'code': code, 'verdict': answer['verdict'],
                                 'prises': answer['prises'], 'lignes': answer['lignes'],
                                 'fichiers_jsonl_presents': len(list(folder.glob('m6_*.jsonl'))),
                                 'refus': answer['refus'], 'limites_declarees': len(answer['non_rejouable']),
                                 'mediane_context_open_spin_us': (answer.get('medianes_us') or {}).get(
                                     'spin', {}).get('context_open')})
    return observations


def main():
    files = []
    for part in ('mes_m5_parcours', 'mes_m6_session', 'mes_m2_feuille'):
        files.extend(sorted((MICRO / part).rglob('*.py')))
    files.append(MICRO / 'mes_m6_session/mes_m6_session_cost.cu')
    before = {str(p.relative_to(ROOT)): digest(p.read_bytes()) for p in files}
    for path, value in before.items():
        need(digest(git_bytes(PIN, path)) == value, 'source hors pin : ' + path)
    old_paths = ['morsehgp3D_v12/microbancs/mes_m5_parcours/scripts/g4_traversal_bench.py',
                 'morsehgp3D_v12/microbancs/mes_m6_session/run_m6.py']
    old = [git_bytes(BEFORE, p) for p in old_paths]
    result = {'pin': subprocess.check_output(['git', 'rev-parse', PIN], cwd=ROOT, text=True).strip(),
              'avant_correctif': subprocess.check_output(['git', 'rev-parse', BEFORE], cwd=ROOT, text=True).strip(),
              'scope': 'preuves synthetiques des vrais pilotes ; aucune execution GPU ou mesure de vitesse',
              'sources_sha256': before, 'anciens_juges_sha256': dict(zip(old_paths, map(digest, old))),
              'm5_avant_apres': m5_causal(old[0]), 'm6': m6_tests(old[1])}
    need(before == {str(p.relative_to(ROOT)): digest(p.read_bytes()) for p in files}, 'source modifiee')
    print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
