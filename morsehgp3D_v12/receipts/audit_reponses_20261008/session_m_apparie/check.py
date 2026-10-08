#!/usr/bin/env python3
"""Relecture appariée FULLM : ne lance que des lecteurs Python de journaux existants."""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import statistics as stats
import sys
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ARMS = {'ref': [], 'aa': [], 'cache': ['--cache=8589934592'], 'seq': ['--sequentiel']}
LABELS = ['ng00', 'ng01', 'ng02']


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    def unique(pairs):
        obj = {}
        for key, value in pairs:
            need(key not in obj, 'JSON key repeated')
            obj[key] = value
        return obj
    return json.loads(path.read_text(), object_pairs_hook=unique,
                      parse_constant=lambda x: (_ for _ in ()).throw(ValueError(x)))


def exact(a, b):
    return json.dumps(a, sort_keys=True, allow_nan=False) == json.dumps(b, sort_keys=True, allow_nan=False)


def statistic_equal(a, b, differences, path=''):
    """Seulement pour rapprochement de calculs float, jamais pour décider le seuil ni admettre un brut."""
    if type(a) is float and type(b) is float:
        if a != b:
            delta=abs(a-b);unit=max(math.ulp(a),math.ulp(b))
            differences.append({'path':path,'delta':delta,'ulps':delta/unit})
            return delta <= 2*unit
        return True
    if type(a) is dict and type(b) is dict:
        return set(a)==set(b) and all(statistic_equal(a[k],b[k],differences,path+'/'+k) for k in a)
    if type(a) is list and type(b) is list:
        return len(a)==len(b) and all(statistic_equal(x,y,differences,path+'/'+str(i)) for i,(x,y) in enumerate(zip(a,b)))
    return exact(a,b)


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def configuration(report, metadata):
    cfg = report['parametres']
    expect = dict(reference='ref', aa='aa', trames=LABELS, tours=10, passes=10,
                  voie='appareil', k=5, fils=48, essai=False)
    need(all(exact(cfg.get(k),v) for k,v in expect.items()), 'plan configuration')
    need(exact(report['regle_bras'], ARMS), 'plan arms')
    frames = {x['name']:{'sites':x['sites']} for x in metadata['ng']}
    need(exact(report['trames'],frames), 'plan frame metadata')
    for key in ('identite','campagne','ordres'):
        need(type(report.get(key)) is dict and set(report[key])==set(LABELS), key+' cohort')
    for label in LABELS:
        need(type(report['identite'][label]) is dict and set(report['identite'][label])==set(ARMS), 'identity arms')
        rounds = report['campagne'][label]
        need(type(rounds) is list and len(rounds)==10, 'exact rounds')
        need(all(type(row) is dict and set(row)==set(ARMS) for row in rounds), 'round arms')
        names = sorted(ARMS)
        need(exact(report['ordres'][label],[[names[(t+i)%4] for i in range(4)] for t in range(10)]), 'arm rotation')
    need(type(report['session_v12set']) is dict and set(report['session_v12set'])==set(ARMS), 'information arms')


def summary(rows):
    warm = rows[1:]
    return {'mur_chaud_ns':stats.median(p['wall_ns'] for p in warm),
            'etapes_ns':{s:stats.median(p['etapes_ns'][s] for p in warm) for s in warm[0]['etapes_ns']},
            'cpu_ns':stats.median(p['cpu_ns'] for p in warm),
            'pic_octets':max(p['pic_octets'] for p in warm)}


def estimate(ratios):
    logs = [math.log(x) for x in ratios]
    rng = random.Random(20261008)
    samples = []
    for _ in range(10000):
        total = 0.0
        for _ in range(len(logs)):
            total += logs[rng.randrange(len(logs))]
        samples.append(total/len(logs))
    samples.sort()
    return {'rapport':math.exp(sum(logs)/len(logs)),
            'ic95':[math.exp(samples[250]),math.exp(samples[9749])],'tours':len(logs)}


def statistics(campaign):
    result = {}
    for arm in sorted(ARMS):
        if arm=='ref':
            continue
        frames = {label:estimate([row[arm]['mur_chaud_ns']/row['ref']['mur_chaud_ns']
                                 for row in campaign[label]]) for label in LABELS}
        result[arm]={'trames':frames}
    off = [label for label in LABELS if abs(result['aa']['trames'][label]['rapport']-1) > 0.015]
    if off:
        return {'verdict':'refuse','aa_hors':off,'cas':result}
    for arm,row in result.items():
        row['verdict']='controle A/A' if arm=='aa' else ('adopte' if all(x['ic95'][1]<1 for x in row['trames'].values()) else 'rejete')
    return {'verdict':'juge','aa_hors':[],'cas':result}


def review(folder, metadata_path, prepared):
    capture=load(HERE/'capture.json')
    for p,wanted in capture['patched'].items():
        need(sha(prepared/p)==wanted,'reader source pin')
    pa=module(prepared/'morsehgp3D_v12/microbancs/mes_apparie/pilote_apparie.py','paired_replay')
    lf=pa.lf
    need(sha(metadata_path)==capture['metadata_sha256'],'input metadata pin')
    report_hash=sha(folder/'rapport_apparie.json')
    need(report_hash==capture['report_sha256'],'report pin')
    report=load(folder/'rapport_apparie.json');metadata=load(metadata_path)
    configuration(report,metadata)
    paths={f'journaux/identite/{label}_{arm}.jsonl' for label in LABELS for arm in ARMS}
    paths|={f'journaux/campagne/{label}/{arm}_t{t:02d}.jsonl' for label in LABELS for arm in ARMS for t in range(10)}
    paths|={f'journaux/session/{arm}_r{t}.jsonl' for arm in ARMS for t in range(2)}
    need({str(p.relative_to(folder)) for p in (folder/'journaux').rglob('*.jsonl')}==paths,'exact journal cohort')
    need(exact(report['regle'],pa.REGLE_APPARIEE),'preannounced rule')
    condition=[]
    for when in ('avant','apres'):
        env=report.get('environnement',{}).get(when,{})
        if type(env) is not dict or not pa.bf.environment_ok(env):
            condition.append('environment '+when)
    records={}; hashes={}; admitted={'identite':0,'campagne':0,'session':0}; counts={k:0 for k in admitted}

    def read(path, expected, code):
        need(type(code) is int,'process code int excluding bool')
        need(path.is_file(),'missing journal '+str(path.relative_to(folder)))
        hashes[str(path.relative_to(folder))]=sha(path)
        state=lf.parse_output(code,path.read_text(encoding='ascii'),expected)
        need(state['etat']=='ok','journal '+str(path.relative_to(folder))+': '+state['raison'])
        # All probes in this campaign use the shared budget, not a separate device budget.
        for row in state['passes']:
            need(row['pic_appareil_octets']==0,'unexpected separate device peak')
            need(row['epinglee_octets'] <= row['pic_octets'],'pinned capacity exceeds active shared budget peak')
            need(row['appareil_octets'] <= row['pic_octets'],'device capacity exceeds active shared budget peak')
        return state['passes']

    def one(kind,label,arm,take,relative,n,digest):
        path=folder/relative
        need(type(take) is dict,'take object')
        need(take.get('journal')==path.name and take.get('journal_sha256')==sha(path),'take hash/name')
        need(take.get('etat')=='ok','take unsuccessful')
        rows=read(path,dict(voie='appareil',k=5,fils=48,passes=n,empreinte=digest,
                           trames=[(label,report['trames'][label]['sites'])],budget_appareil='partage',
                           bits=21,schema='sequentiel' if arm=='seq' else 'recouvert'),take.get('code'))
        computed=summary(rows);computed['empreintes']=sorted({p['full_sha256'] for p in rows}) if digest else []
        need(all(exact(take.get(key),v) for key,v in computed.items()),'take summary mismatch')
        records[relative]=rows;admitted[kind]+=1;counts[kind]+=len(rows)
        return computed

    identities={}; campaign={label:[] for label in LABELS}
    for label in LABELS:
        hashes_for_frame=set()
        for arm in ARMS:
            take=report['identite'][label][arm]
            x=one('identite',label,arm,take,f'journaux/identite/{label}_{arm}.jsonl',2,True)
            need(len(x['empreintes'])==1,'identity per arm')
            hashes_for_frame.update(x['empreintes'])
        need(len(hashes_for_frame)==1,'identity across arms')
        identities[label]=next(iter(hashes_for_frame))
        for t,row in enumerate(report['campagne'][label]):
            campaign[label].append({arm:one('campagne',label,arm,row[arm],f'journaux/campagne/{label}/{arm}_t{t:02d}.jsonl',10,False)
                                    for arm in ARMS})
    computed=statistics(campaign)
    # Secondary cross-check only: the independent decisions and statistics are already computed above.
    patched=pa.judge(report,str(folder))
    roundoff=[]
    if not condition:
        need(computed['verdict']==patched['verdict'] and statistic_equal(computed['cas'],patched['cas'],roundoff,'independent/patched'),'independent statistics vs patched judge')
    need(exact(patched['verdict'],report['jugement']['verdict']) and exact(patched['refus'],report['jugement']['refus']) and
         statistic_equal(patched['cas'],report['jugement']['cas'],roundoff,'patched/published'),'published verdict differs')
    session={}; session_resources={}; info_conditions=[]
    frames=[(x['name'],x['sites']) for x in metadata['session37']]
    need(len(frames)==37 and len(set(x[0] for x in frames))==37,'37 metadata cohort')
    for arm in ARMS:
        values={name:[] for name,_ in frames}
        session_resources[arm]=[]
        published=report['session_v12set'][arm]
        # Code/hash were not archived per information run. Successful code is conditional on this pinned pilot.
        if published.get('refus'):
            info_conditions.append(arm+': information run refused; no success code inferred')
            continue
        for tour in range(2):
            relative=f'journaux/session/{arm}_r{tour}.jsonl'
            rows=read(folder/relative,dict(voie='appareil',k=5,fils=48,passes=74,empreinte=False,trames=frames,
                      budget_appareil='partage',bits=21,schema='sequentiel' if arm=='seq' else 'recouvert'),0)
            records[relative]=rows;admitted['session']+=1;counts['session']+=len(rows)
            for p in rows[37:]:values[p['trame']].append(p['wall_ns'])
            session_resources[arm].append({'tour':tour,'passes':len(rows),'passes_chaudes':37,
                'pic_budget_actif_max_octets':max(p['pic_octets'] for p in rows),
                'rss_processus_max_octets':max(p['rss_max_octets'] for p in rows),
                'rss_derniere_passe_octets':rows[-1]['rss_max_octets'],
                'capacite_appareil_max_octets':max(p['appareil_octets'] for p in rows),
                'capacite_epinglee_max_octets':max(p['epinglee_octets'] for p in rows),
                'cpu_secondes_visites_total_ns':sum(p['cpu_ns'] for p in rows[37:])})
        walls={name:stats.median(v) for name,v in values.items()}
        recomputed=dict(trames=walls,refus=[],mediane_ns=stats.median(walls.values()),maximum_ns=max(walls.values()))
        need(exact(published,recomputed),'information summary mismatch')
        session[arm]=recomputed
    resources={}
    for label in LABELS:
        resources[label]={}
        for arm in ARMS:
            rows=[p for t in range(10) for p in records[f'journaux/campagne/{label}/{arm}_t{t:02d}.jsonl'][1:]]
            takes=[row[arm] for row in campaign[label]]
            resources[label][arm]={'passes_chaudes':len(rows),'mur_mediane_medianes_ns':stats.median(t['mur_chaud_ns'] for t in takes),
                'etapes_medianes_medianes_ns':{s:stats.median(t['etapes_ns'][s] for t in takes) for s in takes[0]['etapes_ns']},
                'cpu_mediane_medianes_ns':stats.median(t['cpu_ns'] for t in takes),'cpu_chaud_total_ns':sum(p['cpu_ns'] for p in rows),
                'pic_budget_actif_max_octets':max(p['pic_octets'] for p in rows),'rss_processus_max_octets':max(p['rss_max_octets'] for p in rows),
                'capacite_appareil_max_octets':max(p['appareil_octets'] for p in rows),'capacite_epinglee_max_octets':max(p['epinglee_octets'] for p in rows)}
    provenance=report['provenance']
    inventory=hashlib.sha256(json.dumps(hashes,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    need(inventory==capture['journal_inventory_sha256'],'raw inventory pin')
    need(sha(folder/'rapport_apparie.json')==report_hash,'report changed while reading')
    need(all(sha(folder/path)==wanted for path,wanted in hashes.items()),'journals changed while reading')
    return {'source_pin':capture['source_pin'],'report_sha256':report_hash,
            'metadata_sha256':sha(metadata_path),'admission_conditions':condition,'information_conditions':info_conditions,
            'journaux_admis':admitted,'passes_admises':counts,'identites':identities,'statistique':computed,
            'ressources_decisives':resources,'session_informative':session,'ressources_sessions':session_resources,'journal_inventory_sha256':inventory,
            'statistic_crosscheck_roundoff':roundoff,
            'ELF_initial_declare':provenance.get('sonde_sha256'),'ELF_final_present': 'sonde_fin_sha256' in provenance,
            'ELF_fermeture_certifiee':False,'qualification_campagne_complete':False,
            'limits':['environment after precedes informative Sessions','information process codes inferred conditionally from pinned pilot',
                      'active shared budget excludes inactive cached blocks; RSS cumulative process and not GPU VRAM',
                      'external source/build/binary/shutdown provenance separate']}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,required=True)
    p.add_argument('--folder',type=Path,required=True)
    p.add_argument('--metadata',type=Path,required=True)
    p.add_argument('--plan',type=Path,required=True)
    p.add_argument('--check',action='store_true')
    args=p.parse_args();cap=load(HERE/'capture.json')
    need(sha(args.plan)==cap['plan_sha256'],'plan pin')
    with tempfile.TemporaryDirectory(prefix='audit-fullm-paired-') as tmp:
        prepared=Path(tmp)
        for path,digest in cap['original'].items():
            body=subprocess.check_output(['git','-C',str(args.repo),'show',cap['source_pin']+':'+path])
            need(hashlib.sha256(body).hexdigest()==digest,'source pin')
            dest=prepared/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(body)
        receipts=args.repo/'morsehgp3D_v12/receipts/audit_reponses_20261008'
        for relative,digest in cap['patches'].items():
            patch=receipts/relative;need(sha(patch)==digest,'patch pin')
            subprocess.run(['git','apply','--check',str(patch)],cwd=prepared,check=True,capture_output=True)
            subprocess.run(['git','apply',str(patch)],cwd=prepared,check=True,capture_output=True)
        result=review(args.folder,args.metadata,prepared)
    if args.check:need(exact(result,load(HERE/'results.json')),'result differs')
    print(json.dumps(result,ensure_ascii=False,indent=2,sort_keys=True))


if __name__=='__main__':
    main()
