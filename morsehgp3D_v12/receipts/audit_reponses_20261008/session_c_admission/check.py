"""Admission MES-C archivée, avec lecteur indépendant figé ; aucun moteur."""
import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import statistics


def sha(data):
    return hashlib.sha256(data).hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--evidence', required=True, help='rapport_c.json et brut/ extraits des résultats')
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    capture = json.loads((here / 'capture.json').read_text())
    frozen = here.parent / 'mes_c_contrelecture'
    need(sha((frozen / 'reader.py').read_bytes()) == capture['reader_sha256'], 'lecteur modifie')
    need(sha((frozen / 'capture.json').read_bytes()) == capture['reader_capture_sha256'], 'cohorte modifiee')
    spec = importlib.util.spec_from_file_location('frozen_mes_c_reader', frozen / 'reader.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    expected = audit.loads((frozen / 'capture.json').read_text())
    evidence = Path(args.evidence)
    raw_bytes = {}
    for name, item in capture['files'].items():
        data = (evidence / name).read_bytes()
        need(len(data) == item['bytes'] and sha(data) == item['sha256'], 'preuve modifiee ' + name)
        raw_bytes[name] = data
    need(set(p.name for p in (evidence / 'brut').iterdir()) ==
         {Path(p).name for p in raw_bytes if p.startswith('brut/')}, 'inventaire des bruts')
    report = audit.loads(raw_bytes['rapport_c.json'].decode())
    raws = {Path(name).name: data.decode('ascii') for name, data in raw_bytes.items() if name.startswith('brut/')}
    lf, reasons = audit.load_sources(args.repo, expected)
    admitted = audit.review(report, raws, expected, lf, reasons)
    need(not admitted['differences'], 'ecart recalcul/rapport')
    table = {}
    for key, value in admitted['configurations'].items():
        groups = {}
        for group in ('tous', 'reel', 'uniform', 'clusters8', 'slab'):
            values = [v for v in value['valeurs'].values() if group == 'tous' or v['groupe'] == group]
            groups[group] = dict(nuages=len(values),
                                 mediane_ns=statistics.median(v['chaud_ns'] for v in values),
                                 maximum_ns=max(v['chaud_ns'] for v in values),
                                 mediane_cpu_ns=statistics.median(v['cpu_ns'] for v in values))
        table[key] = dict(groupes=groups, droites=value['droites'])
    hard = report['difficiles']
    refusals = [{k: row[k] for k in ('nom', 'voie', 'k', 'etat', 'raison')} for row in hard if row['etat'] == 'refus']
    interrupted = [audit.loads(s) for s in raws['session_appareil_10_1.jsonl'].splitlines()]
    phases = Counter(row['phase'] for row in interrupted)
    need(phases == {'open': 1, 'full': 393, 'liberation': 392}, 'prefixe expire')
    need(interrupted[-1]['phase'] == 'full' and interrupted[-1]['pass'] == 392, 'dernier prefixe')
    result = dict(source_commit=capture['source_commit'], archive_sha256=capture['archive_sha256'],
                  criteres=admitted['criteres'], verdict=admitted['verdict'], differences=admitted['differences'],
                  processus_prevus=admitted['processus_prevus'], processus_joues=admitted['processus_joues'],
                  etats=dict(Counter(admitted['etats'].values())), cohorte_complete=admitted['cohorte_complete'],
                  passes_completes=admitted['passes_completes'], passes_chaudes=admitted['passes_chaudes'],
                  controles_declares=report['controles'], refus_difficiles=refusals,
                  non_joues=admitted['non_joues'], configurations=table,
                  expiration=dict(configuration='appareil:10:1', secondes=report['configurations']['appareil:10:1']['secondes'],
                                  phases=dict(phases), derniere_passe_full=392, passes_admises=0,
                                  limite='solde du delai global, pas le delai des cas difficiles'),
                  codes=admitted['codes'], empreintes_instables=0, native_calls=0)
    for name, data in raw_bytes.items():
        need((evidence / name).read_bytes() == data, 'preuve devenue mutable ' + name)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
