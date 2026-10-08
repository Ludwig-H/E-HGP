"""Contre-JSON du lecteur commun epingle ; aucune sonde, aucun nuage."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', required=True, type=Path)
    args = parser.parse_args()
    pins = json.loads((HERE / 'pins.json').read_text())
    with tempfile.TemporaryDirectory(prefix='audit-common-full-') as folder:
        folder = Path(folder)
        for src in pins['sources']:
            raw = subprocess.check_output(['git', '-C', str(args.repo), 'show',
                                           pins['pin'] + ':morsehgp3D_v12/' + src['path']])
            if hashlib.sha256(raw).hexdigest() != src['sha256']:
                raise RuntimeError('source drift: ' + src['path'])
            dest = folder / src['path']
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(raw)
        tools = folder / 'microbancs/outils'
        lf = load('lecteur_full', tools / 'lecteur_full.py')
        tests = load('audit_fixture_common', tools / 'test_lecteur_full.py')
        results = {}
        good = tests.device_output()
        scenarios = [
            ('nominal', 0, good), ('code_bool', False, good), ('code_float', 0.0, good),
            ('raison_inconnue', 2, tests.device_output(passes=1, end={
                'phase': 'exit', 'status': 'resource_exhausted', 'reason': 'not_a_reason'})),
            ('couple_incoherent', 2, tests.device_output(passes=1, end={
                'phase': 'exit', 'status': 'unsupported_degeneracy', 'reason': 'memory_budget'})),
            ('code_refus_float', 2.0, tests.device_output(passes=1, end={
                'phase': 'exit', 'status': 'resource_exhausted', 'reason': 'memory_budget'})),
        ]
        for name, code, text in scenarios:
            state = lf.parse_output(code, text, tests.ATTENDU)
            results[name] = {'etat': state['etat'], 'raison': state['raison'], 'passes': len(state['passes'])}
        mutations = {
            'appareil_sup_pic': lambda row: row.update(appareil_octets=9),
            'epinglee_sup_pic_hote': lambda row: row.update(epinglee_octets=11),
            'pic_suivant_sous_usage_precedent': lambda row: row['memoire_octets'].update(G=[2, 3]),
            'liberation_valide_entre_etages': lambda row: row['memoire_octets'].update(G=[1, 4]),
            'cpu_null': lambda row: row.update(cpu_ns=None),
            'rss_null': lambda row: row.update(rss_max_octets=None),
        }
        for name, mutate in mutations.items():
            state = lf.parse_output(0, tests.device_output(mutate=mutate), copy.deepcopy(tests.ATTENDU))
            results[name] = {'etat': state['etat'], 'raison': state['raison'], 'passes': len(state['passes'])}
        expected = {name: 'ok' for name in results}
        expected.update(raison_inconnue='refus', couple_incoherent='refus', code_refus_float='refus',
                        cpu_null='illisible', rss_null='illisible')
        for name, state in results.items():
            if state['etat'] != expected[name]:
                raise RuntimeError('counterexample changed: ' + name)
        subprocess.run(['git', 'apply', '-p2', str(HERE / 'coherence_proposed.patch')], cwd=folder, check=True)
        patched = load('patched_lecteur_full', tools / 'lecteur_full.py')
        after = {}
        for name, code, text in scenarios:
            after[name] = patched.parse_output(code, text, tests.ATTENDU)['etat']
        for name, mutate in mutations.items():
            after[name] = patched.parse_output(0, tests.device_output(mutate=mutate), tests.ATTENDU)['etat']
        for name in ('code_bool', 'code_float', 'code_refus_float', 'appareil_sup_pic',
                     'epinglee_sup_pic_hote', 'pic_suivant_sous_usage_precedent'):
            if after[name] != 'illisible':
                raise RuntimeError('partial patch ineffective: ' + name)
        if after['nominal'] != 'ok' or after['liberation_valide_entre_etages'] != 'ok':
            raise RuntimeError('partial patch rejects valid fixture')
        print(json.dumps({'avant': results, 'apres_patch_partiel': after}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
