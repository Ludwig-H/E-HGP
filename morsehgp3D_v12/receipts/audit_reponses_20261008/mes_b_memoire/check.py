#!/usr/bin/env python3
"""MES-B : coherence du lecteur sur JSON fictifs, sans moteur ni campagne."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def witnesses(p, t):
    def zero(row):
        row['wall_ns'] = 0
        for block in ('etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns'):
            row[block] = dict.fromkeys(row[block], 0)

    def memory(row, key, value):
        row['memoire_octets'][key] = value

    mutations = {
        'nominal': None,
        'mur_nul': zero,
        'etage_absent': lambda r: r['memoire_octets'].pop('G'),
        'pic_global_incoherent': lambda r: memory(r, 'C', [4, 9]),
        'usage_superieur_pic': lambda r: memory(r, 'TMVR', [9, 8]),
        'memoire_booleenne': lambda r: memory(r, 'P', [True, 2]),
        'usage_diminue_pic_conserve': lambda r: memory(r, 'G', [1, 4]),
        'gpu_capacite_superieure_pic': lambda r: r.update(pic_appareil_octets=6),
        'epinglee_superieure_pic_hote': lambda r: r.update(epinglee_octets=11),
        'pic_suivant_inferieur_usage': lambda r: memory(r, 'G', [3, 3]),
    }
    result = {}
    for name, mutation in mutations.items():
        state = p.parse_output(0, t.device_output(mutate=mutation), t.CASE, t.SITES, t.LABEL)
        result[name] = dict(etat=state['etat'], raison=state['raison'])
    state = p.parse_output(0, t.device_output(passes=1, mutate=lambda r: r.update(sites=0)),
                           dict(t.CASE, passes=1), 0, t.LABEL)
    result['sites_nuls'] = dict(etat=state['etat'], raison=state['raison'])
    shared = dict(phase='open', status='ok', reason='none', wall_ns=5, budget_appareil='partage')
    state = p.parse_output(0, t.device_output(open_row=shared), t.CASE, t.SITES, t.LABEL)
    result['ouverture_partagee'] = dict(etat=state['etat'], raison=state['raison'])
    rows = [json.loads(line) for line in t.device_output().splitlines()][1:]
    for row in rows:
        if row['phase'] == 'full':
            row.update(voie='cpu', appareil_octets=0, epinglee_octets=0, pic_appareil_octets=0)
    state = p.parse_output(0, t.dump(rows), dict(t.CASE, voie='cpu'), t.SITES, t.LABEL)
    result['cpu_sans_memoire_gpu'] = dict(etat=state['etat'], raison=state['raison'])
    result['pentes_non_positives'] = [p.slope([(1_000_000, 0), (2_000_000, 1)]),
                                     p.slope([(0, 1), (2_000_000, 1)])]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=HERE.parents[3])
    parser.add_argument('--run-gates', action='store_true')
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    source = {}
    for rel, pin in cap['sources_sha256'].items():
        data = subprocess.check_output(['git', 'show', cap['pin'] + ':' + rel], cwd=args.repo)
        need(sha(data) == pin, 'source Git differente : ' + rel)
        source[rel] = data
    with tempfile.TemporaryDirectory(prefix='audit-mesb-memory-') as folder:
        root = Path(folder)
        for rel, data in source.items():
            path = root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        bench = root / 'morsehgp3D_v12/microbancs/mes_b_scenes'
        sys.path.insert(0, str(bench))
        t = load('mesb_memory_fixture', bench / 'test_pilote_b.py')
        original = witnesses(t.pilote_b, t)
        holes = ('gpu_capacite_superieure_pic', 'epinglee_superieure_pic_hote', 'pic_suivant_inferieur_usage')
        for name, row in original.items():
            if name != 'pentes_non_positives':
                need(row['etat'] == ('ok' if name in ('nominal', 'cpu_sans_memoire_gpu',
                                                      'usage_diminue_pic_conserve', *holes) else 'illisible'),
                     'temoin different : ' + name)
        need(original['pentes_non_positives'] == [None, None], 'garde log(0)')
        gates = []
        if args.run_gates:
            for record in cap['gate_results']:
                command = [sys.executable, *record['command'][1:-1],
                           str(root / 'morsehgp3D_v12' / record['command'][-1])]
                done = subprocess.run(command, capture_output=True, text=True, timeout=60,
                                      env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
                need(done.returncode == record['code'] and done.stdout == record['stdout'] and
                     done.stderr == record['stderr'], 'porte Python differente')
                gates.append({'code': done.returncode, 'stdout': done.stdout.strip()})
        patch = HERE / 'coherence_memoire.patch'
        for check in (True, False):
            command = ['git', 'apply'] + (['--check'] if check else []) + [str(patch)]
            subprocess.run(command, cwd=root, check=True, capture_output=True)
        need(sha((bench / 'pilote_b.py').read_bytes()) == cap['candidate_pilot_sha256'], 'patch different')
        corrected = witnesses(load('mesb_memory_candidate', bench / 'pilote_b.py'), t)
        for name, row in corrected.items():
            if name != 'pentes_non_positives':
                need(row['etat'] == ('ok' if name in ('nominal', 'cpu_sans_memoire_gpu',
                                                      'usage_diminue_pic_conserve') else 'illisible'),
                     'correction insuffisante : ' + name)
        need(corrected['pentes_non_positives'] == [None, None], 'garde de pente perdue')
    result = dict(original=original, proposed=corrected, sources=len(source), native_executed=False)
    need(result == cap['result'], 'capture differente')
    print(json.dumps(dict(result=result, gates=gates), ensure_ascii=False, sort_keys=True))


if __name__ == '__main__':
    main()
