#!/usr/bin/env python3
"""Admet seulement les journaux appariés C3 déjà retournés ; aucun moteur."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import statistics
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def commanded(command):
    argv = command['argv'][2:]
    need(len(argv) % 2 == 0, 'arguments du plan')
    args, arms = {}, {}
    for key, value in zip(argv[::2], argv[1::2]):
        if key == '--bras':
            name, sep, options = value.partition('=')
            need(sep and name not in arms, 'bras du plan')
            arms[name] = options.split()
        else:
            need(key not in args, 'option répétée')
            args[key] = value
    cfg = dict(reference=args['--reference'], aa=args['--aa'], trames=args['--trames'].split(','),
               k=int(args['--k']), fils=int(args['--fils']), voie=args['--voie'],
               tours=int(args['--tours']), passes=int(args['--passes']), essai=False)
    need('--archive-v12set' not in args and '--essai' not in args, 'portée sans Session informative')
    return cfg, arms


def judge(campaign, cfg, arms, helpers):
    cases = {}
    for arm in sorted(arms):
        if arm == cfg['reference']:
            continue
        frames = {label: helpers.estimate([row[arm]['mur_chaud_ns'] / row[cfg['reference']]['mur_chaud_ns']
                                           for row in campaign[label]]) for label in cfg['trames']}
        cases[arm] = dict(trames=frames)
    off = [label for label in cfg['trames'] if abs(cases[cfg['aa']]['trames'][label]['rapport'] - 1) > 0.015]
    if off:
        return dict(verdict='refuse', refus=['A/A hors de la fenetre de +/- 1.5 % : ' + ', '.join(off)], cas=cases)
    for arm, case in cases.items():
        case['verdict'] = 'controle A/A' if arm == cfg['aa'] else (
            'adopte' if all(v['ic95'][1] < 1 for v in case['trames'].values()) else 'rejete')
    return dict(verdict='juge', refus=[], cas=cases)


def review(folder, spec, plan, cap, pa, h):
    cfg, arms = commanded(plan['commands'][spec['command_index']])
    need(h.exact(cfg, spec['configuration']) and h.exact(arms, spec['arms']), 'configuration capturée')
    need(sha(folder / 'rapport_apparie.json') == spec['report_sha256'], 'rapport épinglé')
    report = h.load(folder / 'rapport_apparie.json')
    need(report['mesure'] == 'pilote_apparie' and 'session_v12set' not in report, 'mesure')
    need(h.exact({k: report['parametres'].get(k) for k in cfg}, cfg), 'configuration rapport/plan')
    need(h.exact(report['regle_bras'], arms) and h.exact(report['regle'], pa.REGLE_APPARIEE), 'bras/règle')
    frames = {k: dict(sites=v['entries']) for k, v in cap['input_sizes'].items()}
    need(h.exact(report['trames'], frames), 'effectifs issus des tailles déclarées')
    labels = cfg['trames']; names = sorted(arms)
    for key in ('campagne', 'identite', 'ordres'):
        need(type(report[key]) is dict and set(report[key]) == set(labels), 'cohorte ' + key)
    for label in labels:
        need(set(report['identite'][label]) == set(arms), 'bras identité')
        need(type(report['campagne'][label]) is list and len(report['campagne'][label]) == cfg['tours'], 'tours')
        need(all(type(row) is dict and set(row) == set(arms) for row in report['campagne'][label]), 'bras par tour')
        rotation = [[names[(t + j) % len(names)] for j in range(len(names))] for t in range(cfg['tours'])]
        need(h.exact(report['ordres'][label], rotation), 'rotation des bras')
    paths = {f'journaux/identite/{label}_{arm}.jsonl' for label in labels for arm in arms}
    paths |= {f'journaux/campagne/{label}/{arm}_t{t:02d}.jsonl'
              for label in labels for arm in arms for t in range(cfg['tours'])}
    need({str(p.relative_to(folder)) for p in (folder / 'journaux').rglob('*.jsonl')} == paths, 'journaux exacts')
    inventory = {}; records = {}; identities = {}; campaign = {label: [] for label in labels}
    def take(label, arm, rec, relative, passes, digest):
        path = folder / relative
        need(type(rec) is dict and type(rec.get('code')) is int and rec['code'] == 0, 'code processus')
        inventory[relative] = sha(path)
        need(rec.get('journal') == path.name and rec.get('journal_sha256') == inventory[relative], 'hash/nom prise')
        expected = dict(voie=cfg['voie'], k=cfg['k'], fils=cfg['fils'], passes=passes, empreinte=digest,
                        trames=[(label, frames[label]['sites'])], budget_appareil='partage', bits=21,
                        schema='sequentiel' if '--sequentiel' in arms[arm] else 'recouvert')
        state = pa.lf.parse_output(rec['code'], path.read_text(encoding='ascii'), expected)
        need(state['etat'] == rec.get('etat') == 'ok', 'flux non conforme ' + relative)
        rows = state['passes']; records[relative] = rows
        for row in rows:
            need(row['pic_appareil_octets'] == 0, 'budget appareil séparé inattendu')
            need(row['epinglee_octets'] <= row['pic_octets'] and row['appareil_octets'] <= row['pic_octets'],
                 'capacités supérieures au pic actif partagé')
        reconstructed = h.summary(rows)
        reconstructed['empreintes'] = sorted({r['full_sha256'] for r in rows}) if digest else []
        need(all(h.exact(rec.get(k), v) for k, v in reconstructed.items()), 'résumé non conforme')
        return reconstructed
    for label in labels:
        digests = set()
        for arm in arms:
            rec = take(label, arm, report['identite'][label][arm], f'journaux/identite/{label}_{arm}.jsonl', 2, True)
            need(len(rec['empreintes']) == 1, 'identité par prise'); digests.update(rec['empreintes'])
        need(len(digests) == 1, 'identité entre bras'); identities[label] = next(iter(digests))
        for t, row in enumerate(report['campagne'][label]):
            campaign[label].append({arm: take(label, arm, row[arm], f'journaux/campagne/{label}/{arm}_t{t:02d}.jsonl',
                                               cfg['passes'], False) for arm in arms})
    if cfg['voie'] == 'appareil':
        for when in ('avant', 'apres'):
            need(pa.bf.environment_ok(report.get('environnement', {}).get(when, {})), 'environnement ' + when)
    calculated = judge(campaign, cfg, arms, h); differences = []
    patched = pa.judge(report, str(folder))
    need(h.statistic_equal(calculated, patched, differences, 'independent/patched'), 'juge indépendant/corrigé')
    need(h.statistic_equal(patched, report['jugement'], differences, 'patched/published'), 'jugement publié')
    resources = {}
    for label in labels:
        resources[label] = {}
        for arm in arms:
            processes = [records[f'journaux/campagne/{label}/{arm}_t{t:02d}.jsonl'] for t in range(cfg['tours'])]
            warm = [p for rows in processes for p in rows[1:]]
            summaries = [row[arm] for row in campaign[label]]
            resources[label][arm] = dict(
                warm_passes=len(warm), cold_median_ns=statistics.median(rows[0]['wall_ns'] for rows in processes),
                warm_median_process_medians_ns=statistics.median(x['mur_chaud_ns'] for x in summaries),
                stage_medians_ns={s: statistics.median(x['etapes_ns'][s] for x in summaries) for s in summaries[0]['etapes_ns']},
                cpu_median_ns=statistics.median(x['cpu_ns'] for x in summaries),
                active_budget_peak_bytes=max(p['pic_octets'] for p in warm),
                process_rss_max_bytes=max(p['rss_max_octets'] for p in warm),
                device_capacity_max_bytes=max(p['appareil_octets'] for p in warm),
                pinned_capacity_max_bytes=max(p['epinglee_octets'] for p in warm))
    inventory_hash = hashlib.sha256(json.dumps(inventory, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    need(inventory_hash == spec['journal_inventory_sha256'], 'inventaire épinglé')
    need(all(sha(folder / path) == wanted for path, wanted in inventory.items()), 'brut modifié pendant lecture')
    need(sha(folder / 'rapport_apparie.json') == spec['report_sha256'], 'rapport modifié pendant lecture')
    return dict(configuration=cfg, arms=arms, journals=len(paths), passes=sum(map(len, records.values())),
                warm_decisive_passes=sum(x['warm_passes'] for v in resources.values() for x in v.values()),
                identities=identities, statistics=calculated, resources=resources, roundoff=differences,
                journal_inventory_sha256=inventory_hash, report_sha256=spec['report_sha256'],
                initial_elf_sha256=report['provenance'].get('sonde_sha256'),
                final_elf_hash_present='sonde_fin_sha256' in report['provenance'], final_elf_closed=False)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, required=True); p.add_argument('--returned', type=Path, required=True)
    p.add_argument('--plan', type=Path, required=True); p.add_argument('--provenance', type=Path, required=True)
    p.add_argument('--check', action='store_true')
    args = p.parse_args(); cap = json.loads((HERE / 'capture.json').read_text())
    helper = HERE.parent / 'session_m_apparie/check.py'
    need(sha(helper) == cap['helper_sha256'], 'lecteur M commun figé')
    h = module(helper, 'paired_audit_helpers')
    need(sha(args.plan) == cap['plan_sha256'], 'plan')
    plan = h.load(args.plan)
    metadata = {x['name']: x for x in h.load(args.provenance)['data_declared']}
    for label, sizes in cap['input_sizes'].items():
        need(sizes['xyz_bytes'] == 12 * sizes['entries'] and sizes['ids_bytes'] == 4 * sizes['entries'], 'tailles')
        for kind, suffix in (('xyz', '.u32le'), ('ids', '.ids.u32le')):
            item = metadata['lidar_' + label + suffix]
            need(item['size'] == sizes[kind + '_bytes'] and item['sha256'] == sizes[kind + '_sha256_declared'],
                 'métadonnées des entrées déclarées')
    with tempfile.TemporaryDirectory(prefix='audit-c3-paired-') as tmp:
        prepared = Path(tmp)
        for path, expected in cap['source_files'].items():
            body = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['source_pin'] + ':' + path])
            need(hashlib.sha256(body).hexdigest() == expected, 'source')
            target = prepared / path; target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(body)
        for relative, expected in cap['patches'].items():
            patch = args.repo / 'morsehgp3D_v12/receipts/audit_reponses_20261008' / relative
            need(sha(patch) == expected, 'correctif')
            subprocess.run(['git', 'apply', '--check', str(patch)], cwd=tmp, check=True, capture_output=True)
            subprocess.run(['git', 'apply', str(patch)], cwd=tmp, check=True, capture_output=True)
        for path, expected in cap['readers_after'].items():
            need(sha(prepared / path) == expected, 'lecteur renforcé')
        pa = h.module(prepared / 'morsehgp3D_v12/microbancs/mes_apparie/pilote_apparie.py', 'paired_c3')
        result = dict(source_pin=cap['source_pin'], native_executed=False,
                      runs={mode: review(args.returned / spec['relative_folder'], spec, plan, cap, pa, h)
                            for mode, spec in cap['runs'].items()})
        result['cross_path_identity_equal'] = h.exact(result['runs']['cpu']['identities'],
                                                     result['runs']['appareil']['identities'])
    if args.check:
        need(h.exact(result, h.load(HERE / 'results.json')), 'résultats différents')
    print(json.dumps(result, sort_keys=True, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
