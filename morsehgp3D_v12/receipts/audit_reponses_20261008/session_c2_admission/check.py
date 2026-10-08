#!/usr/bin/env python3
"""MES-C2 : lecteur 55ca inchangé, nouveau plan figé. Aucun moteur ou payload.
Usage: check.py DEPOT RETOUR_MESC2 RAPPORT_MESC
"""
from collections import Counter
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import statistics as st
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
C = json.loads((HERE / 'capture.json').read_text())
REPO, RAW, OLD = map(Path, sys.argv[1:4])


def sha(data):
    return hashlib.sha256(data).hexdigest()


def need(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    frozen = HERE.parent / 'mes_c_contrelecture'
    need(sha((frozen / 'reader.py').read_bytes()) == C['reader_sha256'], 'lecteur modifié')
    need(sha((frozen / 'capture.json').read_bytes()) == C['base_capture_sha256'], 'cohorte de base modifiée')
    spec = importlib.util.spec_from_file_location('frozen_mes_c', frozen / 'reader.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    expected = audit.loads((frozen / 'capture.json').read_text())
    plan_bytes = (HERE / 'plan.json').read_bytes()
    need(sha(plan_bytes) == C['plan_sha256'], 'plan changé')
    argv = audit.loads(plan_bytes)['commands'][0]['argv'][2:]
    options = {k: None if '{' in v else v for k, v in zip(argv[::2], argv[1::2])}
    need(options == C['command_options'], 'options non dérivées du plan')
    need(C['configuration'] == dict(expected['configuration'], threads=[4, 48]), 'autre dérive de protocole')
    # Entrées du lecteur, pas nouvelle version de son code : C2 était planifiée sans W1.
    expected.update(commit=C['source_commit'], sources=C['sources'], configuration=C['configuration'],
                    command_options=C['command_options'])
    files = {str(p.relative_to(RAW)): p.read_bytes() for p in sorted(RAW.rglob('*')) if p.is_file()}
    need(len(files) == C['returned_files'] and
         sha(''.join(sha(b) + ' ' + n + '\n' for n, b in sorted(files.items())).encode()) == C['returned_tree_sha256'],
         'inventaire de preuves modifié')
    need(sha(files['rapport_c.json']) == C['report_sha256'], 'rapport modifié')
    report = audit.loads(files['rapport_c.json'].decode())
    raws = {Path(n).name: b.decode('ascii') for n, b in files.items() if n.startswith('brut/')}
    lf, reasons = audit.load_sources(REPO, expected)
    admitted = audit.review(report, raws, expected, lf, reasons)
    need(not admitted['differences'] and not admitted['controles'], 'recalcul/rapport ou contrôle en écart')
    need(admitted['cohorte_complete'], 'cohorte incomplète')
    need(sha(OLD.read_bytes()) == C['old_report_sha256'], 'rapport MES-C de comparaison changé')
    old = audit.loads(OLD.read_text())
    prior = HERE.parent / 'session_c_admission/results.json'
    need(sha(prior.read_bytes()) == C['old_admission_sha256'], 'admission historique changée')
    old_admitted = audit.loads(prior.read_text())
    need(not old_admitted['differences'], 'ancienne admission non conforme')
    table, resources, comparison = {}, {}, {}
    hash_matches, logical_comparisons = 0, 0
    for key, value in admitted['configurations'].items():
        vals = list(value['valeurs'].values())
        groups = {}
        for group in ('tous', 'reel', 'uniform', 'clusters8', 'slab'):
            xs = [v for v in vals if group == 'tous' or v['groupe'] == group]
            groups[group] = dict(nuages=len(xs), mediane_ns=st.median(v['chaud_ns'] for v in xs),
                                 maximum_ns=max(v['chaud_ns'] for v in xs),
                                 mediane_cpu_ns=st.median(v['cpu_ns'] for v in xs))
        table[key] = dict(groupes=groups, droites=value['droites'])
        rows = [audit.loads(s) for s in raws['session_' + key.replace(':', '_') + '.jsonl'].splitlines()]
        full = [x for x in rows if x['phase'] == 'full']
        warm = full[147:]
        resources[key] = dict(passes_chaudes=len(warm), cpu_chaud_total_ns=sum(x['cpu_ns'] for x in warm),
                              cpu_chaud_mediane_ns=st.median(x['cpu_ns'] for x in warm),
                              pic_budget_hote_max=max(x['pic_octets'] for x in warm),
                              pic_budget_appareil_max=max(x['pic_appareil_octets'] for x in warm),
                              capacite_appareil_max=max(x['appareil_octets'] for x in warm),
                              epinglee_max=max(x['epinglee_octets'] for x in warm),
                              rss_processus_max=max(x['rss_max_octets'] for x in full))
        previous = old['configurations'].get(key)
        if previous and previous['etat'] == 'ok':
            before = previous['valeurs']
            need(set(before) == set(value['valeurs']), 'cohorte comparative différente')
            for name, x in value['valeurs'].items():
                need(x['sites'] == before[name]['sites'] and x['groupe'] == before[name]['groupe'],
                     'métadonnées comparatives différentes')
                need(x['empreintes'] == before[name]['empreintes'], 'empreintes inter-session différentes')
                hash_matches += 1
            logical_comparisons += 1
            comparison[key] = {}
            for group, select in [('tous', lambda x: True), ('reel', lambda x: x['groupe'] == 'reel'),
                                  ('reel_le150', lambda x: x['groupe'] == 'reel' and x['sites'] <= 150),
                                  ('reel_ge5000', lambda x: x['groupe'] == 'reel' and x['sites'] >= 5000)]:
                selected = [(before[n], x) for n, x in value['valeurs'].items() if select(x)]
                comparison[key][group] = dict(nuages=len(selected),
                    avant_mediane_ns=st.median(a['chaud_ns'] for a, b in selected),
                    apres_mediane_ns=st.median(b['chaud_ns'] for a, b in selected),
                    mediane_rapports_par_nom=st.median(b['chaud_ns']/a['chaud_ns'] for a, b in selected))
    hard = []
    for e in report['difficiles']:
        row = {k: e[k] for k in ('nom', 'voie', 'k', 'fils', 'sites', 'etat', 'raison')}
        if e['etat'] == 'ok':
            row['chaud_ns'] = e['passes'][1]['mur_ns']
        hard.append(row)
    result = dict(source_commit=C['source_commit'], archive_sha256=C['archive_sha256'],
                  lecteur_sha256=C['reader_sha256'], criteres=admitted['criteres'], verdict=admitted['verdict'],
                  controles=admitted['controles'], differences=admitted['differences'],
                  processus_prevus=admitted['processus_prevus'], processus_joues=admitted['processus_joues'],
                  etats=dict(Counter(admitted['etats'].values())), cohorte_complete=admitted['cohorte_complete'],
                  passes_completes=admitted['passes_completes'], passes_chaudes=admitted['passes_chaudes'],
                  non_joues=admitted['non_joues'], configurations=table, ressources=resources, difficiles=hard,
                  comparaison_descriptive=comparison, configurations_communes=logical_comparisons,
                  empreintes_communes_comparees=hash_matches, codes=admitted['codes'], native_calls=0)
    for name, b in files.items():
        need((RAW / name).read_bytes() == b, 'preuve devenue mutable')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
