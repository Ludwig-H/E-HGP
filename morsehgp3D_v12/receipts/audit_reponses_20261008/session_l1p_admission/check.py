#!/usr/bin/env python3
"""Port explicite L1r -> MES-B1p recouvert. JSON/sources Git seuls, aucune sonde."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import types

HERE=Path(__file__).resolve().parent


def need(ok,why):
    if not ok: raise ValueError(why)


def sha(raw): return hashlib.sha256(raw).hexdigest()


def obj(path): return json.loads(path.read_text())


def load(path,name):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m);return m


def raw_files(folder):
    paths=sorted((folder/'brut').iterdir())
    need(all(p.is_file() and not p.is_symlink() for p in paths),'raw files')
    data={p.name:p.read_bytes() for p in paths}
    inv=''.join(f'{sha(v)}  {k}\n' for k,v in sorted(data.items()))
    return data,sha(inv.encode())


def main(repo,folder,plan_path,metadata_path):
    cap=obj(HERE/'capture.json');dep=HERE.parent
    for path,h in cap['dependencies'].items():need(sha((dep/path).read_bytes())==h,'dependency '+path)
    blobs={}
    for path,h in cap['sources'].items():
        raw=subprocess.check_output(['git','-C',str(repo),'show',cap['source_pin']+':'+path])
        need(sha(raw)==h,'source '+path);blobs[path]=raw
    plan=plan_path.read_bytes();report=(folder/'rapport_b.json').read_bytes();metadata=metadata_path.read_bytes()
    need(sha(plan)==cap['plan_sha256'] and sha(report)==cap['report_sha256'] and sha(metadata)==cap['metadata_sha256'],'input pins')
    raws,inventory=raw_files(folder)
    need(len(raws)==cap['raw_files'] and inventory==cap['raw_inventory_sha256'],'raw inventory')
    historical_path=dep/'session_l1r_contrelecture/reader.py';text=historical_path.read_text()
    # Le format historique n'est pas muté : son enveloppe/cohorte/statistique est porté en mémoire.
    modifications=[("plan['commands'][1]['argv']","plan['commands'][0]['argv']"),
        ("('fils','budget_octets','budget_appareil_octets','delai_global_s','series','argv')",
         "('fils','budget_octets','budget_appareil_octets','delai_global_s','series','argv','schema')"),
        ("457d0e6ff6f27daca0eb22f5852fe6b1e53fce1611c330fd3931a5b071c8c49d",
         cap['sources']['morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py'])]
    for before,after in modifications:
        need(text.count(before)==1,'port pattern');text=text.replace(before,after)
    reader=types.ModuleType('l1p_envelope');reader.__file__=str(historical_path)
    exec(compile(text,str(historical_path)+' [port L1p]', 'exec'),reader.__dict__)
    with tempfile.TemporaryDirectory(prefix='audit-l1p-reader-') as td:
        source=Path(td)/'lecteur_full.py';source.write_bytes(blobs['morsehgp3D_v12/microbancs/outils/lecteur_full.py'])
        lf=load(source,'lf_l1p')
        def full(row,i,case):
            expected=dict(voie=case['voie'],k=case['k'],fils=case['fils'],passes=case['expected'],
                empreinte=case['empreinte'],trames=[(case['etiquette'],case['sites'])],
                budget_appareil='separe',bits=21,schema='recouvert')
            why=lf.check_full(row,i,expected)
            reader.need(not why,why)
            reader.need(16*case['sites']<=row['pic_octets']<=160*(1<<30),'host active budget')
            reader.need(row['epinglee_octets']<=row['pic_octets'],'pinned capacity > host peak')
            reader.need(row['appareil_octets']<=row['pic_appareil_octets']<=88*(1<<30),'device capacity/peak/budget')
            reader.need(all(pair[0]>=16*case['sites'] for pair in row['memoire_octets'].values()),'resident inputs')
            if not case['empreinte']:reader.need(row['hors_mur_ns']['empreinte']==0,'digest outside plan')
        reader.full=full
        previous_cap=obj(dep/'session_l1r_contrelecture/capture.json')
        meta=previous_cap['sessions']['L1r'];specs,opt=reader.cohort(reader.decode(plan),meta['sites'])
        parsed=reader.decode(report)
        reader.need(parsed['parametres']['schema']=='recouvert','report schema')
        reasons=dict(re.findall(r'^MHGP12_REASON\((\w+),\s*(\w+),',blobs['morsehgp3D_v12/src/core/reasons.def'].decode(),re.M))
        result=reader.review(parsed,raws,specs,opt,meta['manifest_sha256'],reasons)
        reader.need(result['bruts_admis'],'report/raw admission failed: '+str(result['conditions']))
        # Relecture de chaque flux par LF complet en plus de l'enveloppe indépendante historique.
        for entry,case in zip(parsed['cas'],specs):
            if entry['etat']=='non_joue':continue
            tag=f"{case['nom']}_k{case['k']}_{case['voie']}"
            expected=dict(voie=case['voie'],k=case['k'],fils=48,passes=case['expected'],empreinte=case['empreinte'],
                trames=[(case['etiquette'],case['sites'])],budget_appareil='separe',bits=21,schema='recouvert')
            check=lf.parse_output(entry['code'],raws[tag+'.jsonl'].decode('ascii'),expected)
            need(all(reader.same(entry[k],v) for k,v in check.items()),'full LF vs report')
        # Quelques corruptions causales du raccord neuf, pas une nouvelle qualification générale de LF.
        case=next(c for c in specs if c['voie']=='appareil')
        tag=f"{case['nom']}_k{case['k']}_{case['voie']}"
        rows=[reader.decode(line) for line in raws[tag+'.jsonl'].splitlines()]
        original=next(row for row in rows if row['phase']=='full')
        attacks=[('schema_ancien',lambda p:p.__setitem__('etapes_schema','sequentiel')),
            ('k_bool',lambda p:p.__setitem__('kmax',True)),
            ('indice_bool',lambda p:p.__setitem__('pass',False)),
            ('digest_absent',lambda p:p.pop('full_sha256')),
            ('capacite_appareil',lambda p:p.__setitem__('appareil_octets',p['pic_appareil_octets']+1)),
            ('capacite_epinglee',lambda p:p.__setitem__('epinglee_octets',p['pic_octets']+1)),
            ('depasse_budget',lambda p:p.__setitem__('pic_appareil_octets',(88<<30)+1)),
            ('date_G',lambda p:p['fins_par_ordre_ns'][0].__setitem__(0,p['recouvrement']['fin_ns']))]
        killed=[]
        full(original,0,case)
        for name,mutate in attacks:
            row=json.loads(json.dumps(original));mutate(row)
            try:full(row,0,case)
            except reader.Refusal:killed.append(name)
        need(len(killed)==len(attacks),'mutation accepted')
    old_compact=load(dep/'session_l1r_admission/replay.py','old_compact')
    out=old_compact.compact(result)
    old=obj(dep/'session_l1r_admission/mesures.json');old_cases={(c['nom'],c['k'],c['voie']):c for c in old['cas']}
    comparisons=[]
    for row,case in zip(out['cas'],result['cas']):
        previous=old_cases[(case['nom'],case['k'],case['voie'])]
        need(previous['sites']==case['sites'] and previous['expected']==case['expected'],'historical cohort')
        same_issue=previous['etat']==case['etat'] and previous['issue']==case['issue']
        need(same_issue,'historical issue changed')
        if case['passes']:
            last=case['passes'][-1]
            row['recouvrement_derniere_ns']=last['recouvrement']
            row['fenetres_derniere_ns']=last['fenetres_ns']
            row['c_derniere_ns']=last['c_ns']
            row['fins_par_ordre_derniere_ns']=last['fins_par_ordre_ns']
            comparisons.append({'nom':case['nom'],'k':case['k'],'voie':case['voie'],'sites':case['sites'],
                'regime':row['statistiques']['regime_derniere'],'mur_l1r_ns':previous['statistiques']['derniere_ns'],
                'mur_l1p_ns':row['statistiques']['derniere_ns'],
                'rapport_descriptif':row['statistiques']['derniere_ns']/previous['statistiques']['derniere_ns']})
    need(out['empreintes']==old['empreintes'],'historical measured FULL identities differ')
    out.update(schema='recouvert',cache_defaut_octets=8<<30,comparaison_descriptive_l1r=comparisons,
               identites_l1r_l1p_egales=True,corruptions_refusees=killed,
               source=cap['source_pin'],held_cache_mesure=False,
               temps_cpu='cumul processus, pas mur CPU pur pour voie appareil')
    need(plan_path.read_bytes()==plan and (folder/'rapport_b.json').read_bytes()==report and
         metadata_path.read_bytes()==metadata and raw_files(folder)[1]==inventory,'input changed during replay')
    return out


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True)
    ap.add_argument('--returned',type=Path,required=True);ap.add_argument('--plan',type=Path,required=True)
    ap.add_argument('--metadata',type=Path,required=True);ap.add_argument('--check',action='store_true');args=ap.parse_args()
    value=main(args.repo,args.returned,args.plan,args.metadata)
    if args.check:need(value==obj(HERE/'mesures.json'),'stored measures differ')
    print(json.dumps(value,indent=2,sort_keys=True,ensure_ascii=False))
