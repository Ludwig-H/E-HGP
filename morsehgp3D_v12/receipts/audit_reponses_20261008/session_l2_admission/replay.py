#!/usr/bin/env python3
"""Rejoue uniquement les JSON de la campagne L2, aucun moteur ni réseau."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def need(value, text):
    if not value:
        raise ValueError(text)


def compact(d):
    cases = []
    for c in d['cas']:
        row = {k: c[k] for k in ('nom','etiquette','fils','k','voie','sites','expected','etat','issue','raison','empreinte','statistiques')}
        row['passes_completes'] = len(c['passes'])
        row['open_ns'] = c.get('open_ns')
        if c['passes']:
            last = c['passes'][-1]
            row['etapes_derniere_ns'] = last['etapes_ns']
            row['memoire_derniere_octets'] = last['memoire_octets']
            row['hors_mur_derniere_ns'] = dict(last['hors_mur_ns'], liberation=last['liberation_ns'])
        cases.append(row)
    allp = [p for c in d['cas'] for p in c['passes']]
    return {k:d[k] for k in ('bruts_admis','conditions','criteres','verdict','empreintes','codes',
                            'qualification_campagne')} | {
        'processus':len(cases), 'scenes':len({c['nom'] for c in cases}),
        'succes':sum(c['etat']=='ok' for c in cases), 'refus':sum(c['etat']=='refus' for c in cases),
        'passes_demandees':sum(c['expected'] for c in cases), 'passes_completes':len(allp),
        'passes_chaudes':sum(max(0,c['passes_completes']-1) for c in cases),
        'cpu_null':sum(p['cpu_ns'] is None for p in allp),
        'rss_null':sum(p['rss_max_octets'] is None for p in allp), 'cas':cases}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo',type=Path,required=True)
    ap.add_argument('--results',type=Path,required=True)
    ap.add_argument('--plan',type=Path,required=True)
    ap.add_argument('--archive',type=Path,required=True)
    args = ap.parse_args()
    pin = json.loads((HERE/'capture.json').read_text())
    reader = HERE.parent/'session_l2_contrelecture/reader.py'
    need(sha(reader.read_bytes())==pin['reader_sha256'],'lecteur changé')
    spec=importlib.util.spec_from_file_location('strict_l2',reader)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    previous=reader.with_name('capture.json')
    need(sha(previous.read_bytes())==pin['reader_capture_sha256'],'capture lecteur changée')
    cap=module.decode(previous.read_bytes())
    reasons=module.sources(args.repo,cap)
    plan=args.plan.read_bytes()
    report=(args.results/'rapport_b.json').read_bytes()
    need(sha(args.archive.read_bytes())==pin['archive_sha256'],'archive différente')
    need(sha(plan)==pin['plan_sha256'] and sha(report)==pin['report_sha256'],'plan/rapport différent')
    paths=sorted((args.results/'brut').iterdir())
    need(all(p.is_file() and not p.is_symlink() for p in paths),'brut non régulier')
    raws={p.name:p.read_bytes() for p in paths}
    inventory=''.join(f'{sha(v)}  {k}\n' for k,v in sorted(raws.items()))
    need(sha(inventory.encode())==pin['raw_inventory_sha256'],'bruts différents')
    meta=cap['sessions']['L2']
    specs,opt=module.cohort(module.decode(plan),meta['sites'])
    result=module.review(module.decode(report),raws,specs,opt,meta['manifest_sha256'],reasons)
    out=compact(result)
    need(out['bruts_admis'],'refus admission')
    need(out==json.loads((HERE/'mesures.json').read_text()),'résumé différent')
    need(args.plan.read_bytes()==plan and (args.results/'rapport_b.json').read_bytes()==report and
         all(p.read_bytes()==raws[p.name] for p in paths),'mutation pendant lecture')
    print(json.dumps({'bruts_admis':True,'normal_optimized_same_expected':True,
        'processus':out['processus'],'passes_completes':out['passes_completes'],
        'passes_chaudes':out['passes_chaudes'],'verdict':out['verdict']},sort_keys=True))


if __name__=='__main__':
    main()
