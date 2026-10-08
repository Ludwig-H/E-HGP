#!/usr/bin/env python3
"""Calcul comptable sur les seuls JSON K déjà admis ; aucune exécution native."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import statistics
import tarfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--archive', type=Path, required=True)
    args = ap.parse_args()
    pin = json.loads((HERE / 'capture.json').read_text())
    admitted = HERE.parent / 'session_k_full/mesures.json'
    raw = admitted.read_bytes()
    need(sha(raw) == pin['admission_sha256'], 'admission modifiee')
    meta = json.loads(raw)
    need(sha(args.archive.read_bytes()) == meta['archive_sha256'], 'archive differente')
    frames = defaultdict(lambda: defaultdict(list))
    seen = set()
    prefix = 'results/cmd/000_mes_full/files/full/brut/'
    selected = {n for n in meta['bruts_sha256'] if n.startswith(('k5_', 'v12set_'))}
    with tarfile.open(args.archive) as tar:
        for member in tar:
            if not member.name.startswith(prefix):
                continue
            name = member.name[len(prefix):]
            if name not in selected:
                continue
            need(member.isfile() and name not in seen, 'membre non regulier/duplique')
            seen.add(name)
            body = tar.extractfile(member).read()
            need(sha(body) == meta['bruts_sha256'][name], 'brut different')
            group = 'v12set' if name.startswith('v12set_') else 'k5_appareil'
            first = 37 if group == 'v12set' else 1
            for row in map(json.loads, body.splitlines()):
                if row['phase'] != 'full' or row['pass'] < first:
                    continue
                st = row['etapes_ns']
                residual = row['wall_ns'] - sum(st[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR'))
                need(residual >= 0, 'residu negatif')
                perfect = st['P'] + st['C'] + st['raccord'] + max(st['G'], st['TMVR'])
                need(perfect + residual == row['wall_ns'] - min(st['G'], st['TMVR']), 'identite comptable')
                frames[(group, row['trame'])][name].append(dict(
                    original=row['wall_ns'], sans_residu=perfect, avec_residu=perfect + residual,
                    P_C=st['P'] + st['C']))
    need(seen == selected and len(seen) == 20, 'cohorte de processus incomplete')
    result = {}
    for group, count, per_process in (('k5_appareil', 3, 9), ('v12set', 37, 1)):
        by_frame = {}
        reference_group = 'v12set_k5_appareil' if group == 'v12set' else group
        original = {row[0]: dict(zip(meta['colonnes'][1:], row[1:])) for row in meta['mesures'][reference_group]}
        for (g, frame), processes in sorted(frames.items()):
            if g != group:
                continue
            need(len(processes) == 5 and all(len(v) == per_process for v in processes.values()), 'prises chaudes')
            stats = {}
            for scenario in ('original', 'sans_residu', 'avec_residu', 'P_C'):
                values = [v[scenario] for rows in processes.values() for v in rows]
                stats[scenario] = dict(mediane_ns=statistics.median(values), max_brut_ns=max(values),
                    max_medianes_processus_ns=max(statistics.median(v[scenario] for v in rows)
                                                 for rows in processes.values()))
            need(stats['original']['mediane_ns'] == original[frame]['mediane_ns'] and
                 stats['original']['max_medianes_processus_ns'] == original[frame]['max_medianes_ns'],
                 'agregration differente de K')
            by_frame[frame] = stats
        need(len(by_frame) == count, 'nombre de trames')
        aggregate = {}
        for scenario in ('original', 'sans_residu', 'avec_residu', 'P_C'):
            values = [d[scenario] for d in by_frame.values()]
            aggregate[scenario] = dict(
                mediane_des_medianes_ns=statistics.median(d['mediane_ns'] for d in values),
                maximum_contractuel_ns=max(d['max_medianes_processus_ns'] for d in values),
                medianes_plus_100ms=sum(d['mediane_ns'] > 100000000 for d in values),
                maxima_plus_100ms=sum(d['max_medianes_processus_ns'] > 100000000 for d in values))
        result[group] = dict(trames=count, passes_chaudes=count * 5 * per_process,
                             agregats=aggregate, par_trame=by_frame)
    need(sha(args.archive.read_bytes()) == meta['archive_sha256'], 'archive modifiee pendant lecture')
    print(json.dumps({'native_execution': False, 'conditional_accounting_only': True,
                      'groupes': result}, sort_keys=True))


if __name__ == '__main__':
    main()
