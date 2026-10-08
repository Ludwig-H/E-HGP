#!/usr/bin/env python3
"""Independent FULLM cohort/statistics; pinned legacy and strengthened process readers."""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import statistics as st
import subprocess
import tempfile
import types

HERE=Path(__file__).resolve().parent
GROUPS=('k5_appareil','k10_appareil','k5_cpu','v12set_k5_appareil')
FRAMES=('ng00','ng01','ng02')
LIMIT=100_000_000

def need(ok,msg):
    if not ok:raise ValueError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()
def json_bytes(b):
    def pairs(items):
        out={}
        for k,v in items:need(k not in out,'duplicate JSON key');out[k]=v
        return out
    def bad(x):raise ValueError('nonfinite JSON '+x)
    return json.loads(b.decode('utf-8','strict'),object_pairs_hook=pairs,parse_constant=bad)
def equal(a,b):
    if type(a) is not type(b):return False
    if isinstance(a,dict):return set(a)==set(b) and all(equal(v,b[k]) for k,v in a.items())
    if isinstance(a,list):return len(a)==len(b) and all(equal(x,y) for x,y in zip(a,b))
    return a==b and (type(a) is not float or math.isfinite(a))
def load(path,name):
    sp=importlib.util.spec_from_file_location(name,path);mod=importlib.util.module_from_spec(sp);sp.loader.exec_module(mod)
    return mod

def readers(patch,repo):
    patch=patch.resolve()
    c=json_bytes((HERE/'capture.json').read_bytes())
    bodies={}
    for path,h in c['sources'].items():
        bodies[path]=subprocess.check_output(['git','show',c['source_commit']+':morsehgp3D_v12/'+path],cwd=repo)
        need(sha(bodies[path])==h,'source pin '+path)
    for path,h in c['proof_sources'].items():
        need(sha(subprocess.check_output(['git','show',c['source_commit']+':morsehgp3D_v12/'+path],cwd=repo))==h,'proof source pin '+path)
    need(sha(patch.read_bytes())==c['reader_patch_sha256'],'reader proposal pin')
    lfpath='morsehgp3D_v12/microbancs/outils/lecteur_full.py'
    raw=bodies['microbancs/outils/lecteur_full.py']
    old=types.ModuleType('legacy_full_reader');exec(compile(raw,lfpath,'exec'),old.__dict__)
    with tempfile.TemporaryDirectory(prefix='audit-fullm-reader-') as tmp:
        for name,body in bodies.items():
            p=Path(tmp)/'morsehgp3D_v12'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(body)
        p=Path(tmp)/lfpath
        pilot=load(Path(tmp)/'morsehgp3D_v12/microbancs/mes_full/pilote_full.py','pinned_full_pilot')
        for check in (True,False):subprocess.run(['git','apply']+(['--check'] if check else [])+[str(patch)],cwd=tmp,check=True,capture_output=True)
        improved=p.read_bytes();need(sha(improved)==c['reader_candidate_sha256'],'reader postimage')
    new=types.ModuleType('strengthened_full_reader');exec(compile(improved,lfpath,'exec'),new.__dict__)
    return old,new,pilot

def specs(metadata):
    cohort=metadata['session37'];names=[x['name'] for x in cohort]
    need(len(names)==len(set(names))==len({n[-23:] for n in names})==37,'37 unique labels')
    ng={x['name']:x['sites'] for x in metadata['ng']};need(set(ng)==set(FRAMES),'ng cohort')
    sizes=dict(ng,**{x['name']:x['sites'] for x in cohort})
    need(all(type(x) is int and 0<x<2**32 for x in sizes.values()),'site metadata')
    out={}
    for tag,group,k,n,p,device in [('k5',GROUPS[0],5,5,10,True),('k10',GROUPS[1],10,3,5,True),('cpu',GROUPS[2],5,3,5,False)]:
        for rep in range(n):
            for name in FRAMES[rep%3:]+FRAMES[:rep%3]:out[f'{tag}_{name}_r{rep}.jsonl']=(group,k,device,[name]*p)
    for rep in range(5):out[f'v12set_r{rep}.jsonl']=(GROUPS[3],5,True,(names[rep:]+names[:rep])*2)
    return out,sizes

def process(raw,spec,sizes,old,new):
    group,k,device,names=spec
    sequence=list(dict.fromkeys(names))
    want=dict(voie='appareil' if device else 'cpu',k=k,fils=48,passes=len(names),empreinte=True,
              trames=[(name[-23:],sizes[name]) for name in sequence],budget_appareil='partage',bits=21,schema='recouvert')
    need(raw.isascii(),'non ASCII native output')
    # Code 0 is inferred later from a closed report with no refusals under the pinned pilot, not an external record.
    baseline=old.parse_output(0,raw.decode('ascii'),want)
    strict=new.parse_output(0,raw.decode('ascii'),want)
    need(baseline['etat']=='ok','legacy admission: '+baseline['raison'])
    need(strict['etat']=='ok','strengthened admission: '+strict['raison'])
    need(equal(baseline,strict),'reader output delta')
    rows=strict['passes'];floor=16*sum(sizes[n] for n in set(names))
    need(all(r['pic_octets']>=floor for r in rows),'peak below resident input bytes')
    need(all(r['pic_appareil_octets']==0 and r['appareil_octets']<=r['pic_octets'] and r['epinglee_octets']<=r['pic_octets'] for r in rows),'shared budget capacity/peak')
    return rows

def stats(runs):
    hot=[row for run in runs for row in run[1:]]
    medians=[st.median(row['wall_ns'] for row in run[1:]) for run in runs]
    first=st.median(run[0]['wall_ns'] for run in runs)
    official=dict(mediane_ns=st.median(r['wall_ns'] for r in hot),max_medianes_ns=max(medians),
                  max_ns=max(r['wall_ns'] for r in hot),premiere_ns=first,
                  etapes_ns={k:st.median(r['etapes_ns'][k] for r in hot) for k in hot[0]['etapes_ns']},
                  c_ns={k:st.median(r['c_ns'][k] for r in hot) for k in hot[0]['c_ns']},
                  cpu_ns=st.median(r['cpu_ns'] for r in hot),pic_octets=max(r['pic_octets'] for r in hot),
                  sites=hot[0]['sites'],valeurs=len(hot))
    extended=dict(sites=hot[0]['sites'],processes=len(runs),warm_passes=len(hot),
                  pooled_median_ns=official['mediane_ns'],median_process_medians_ns=st.median(medians),
                  max_process_median_ns=max(medians),max_raw_ns=official['max_ns'],first_median_ns=first,
                  stage_medians_ns=official['etapes_ns'],catalogue_medians_ns=official['c_ns'],
                  cpu_median_ns=official['cpu_ns'],cpu_warm_total_ns=sum(r['cpu_ns'] for r in hot),
                  memory_budget_peak_bytes=official['pic_octets'],rss_process_peak_bytes=max(r['rss_max_octets'] for r in hot),
                  device_capacity_bytes=max(r['appareil_octets'] for r in hot),pinned_capacity_bytes=max(r['epinglee_octets'] for r in hot),
                  warm_over_100ms=sum(r['wall_ns']>LIMIT for r in hot))
    return official,extended

def reconstruct(raws,metadata,old,new,pilot):
    expected,sizes=specs(metadata);need(set(raws)==set(expected),'missing/extra process')
    groups={g:{} for g in GROUPS};passes=0
    for name,spec in expected.items():
        rows=process(raws[name],spec,sizes,old,new);passes+=len(rows)
        group,_,_,names=spec
        if group==GROUPS[3]:
            for i,n in enumerate(names[:37]):groups[group].setdefault(n,[]).append([rows[i],rows[i+37]])
        else:groups[group].setdefault(names[0],[]).append(rows)
    fingerprints={}
    for label,selected in [('k5',(GROUPS[0],GROUPS[2])),('k10',(GROUPS[1],)),('v12set',(GROUPS[3],))]:
        seen={}
        for g in selected:
            for f,runs in groups[g].items():seen.setdefault(f,set()).update(r['full_sha256'] for run in runs for r in run)
        need(all(len(h)==1 for h in seen.values()),'digest mismatch '+label)
        fingerprints[label]={f:next(iter(h)) for f,h in seen.items()}
    official,details={},{ }
    for g,frames in groups.items():
        official[g]={};details[g]={}
        for f,runs in frames.items():
            official[g][f],details[g][f]=stats(runs)
            need(equal(official[g][f],pilot.frame_stats(runs,1)),'statistics differ from pinned pilot')
    contracts={}
    for label,g in [('ng00_02',GROUPS[0]),('v12set',GROUPS[3])]:
        d=official[g];median=st.median(v['mediane_ns'] for v in d.values());maximum=max(v['max_medianes_ns'] for v in d.values())
        contracts[label]=dict(mediane_ns=median,maximum_ns=maximum,trames=len(d),tenu=median<=LIMIT and maximum<=LIMIT)
        need(equal(contracts[label],pilot.contract(d)),'contract differs from pilot')
    computed=dict(statistiques=official,empreintes=fingerprints,contrat=contracts,
                  verdict='tenu' if all(v['tenu'] for v in contracts.values()) else 'non tenu')
    warm=sum(v['warm_passes'] for d in details.values() for v in d.values())
    need((len(raws),passes,warm)==(38,610,392),'cohort totals')
    aggregates={}
    for g,d in details.items():
        aggregates[g]={'frames':len(d),'warm_passes':sum(v['warm_passes'] for v in d.values()),
            'median_frame_pooled_medians_ns':st.median(v['pooled_median_ns'] for v in d.values()),
            'median_frame_process_medians_ns':st.median(v['median_process_medians_ns'] for v in d.values()),
            'max_process_median_ns':max(v['max_process_median_ns'] for v in d.values()),
            'max_raw_ns':max(v['max_raw_ns'] for v in d.values()),
            'frames_pooled_median_over_100ms':sum(v['pooled_median_ns']>LIMIT for v in d.values()),
            'frames_max_process_median_over_100ms':sum(v['max_process_median_ns']>LIMIT for v in d.values()),
            'passes_over_100ms':sum(v['warm_over_100ms'] for v in d.values())}
    return computed,dict(processes=38,passes=610,warm_passes=392,groups=details,aggregates=aggregates)

def review(report,computed,c):
    need(type(report) is dict and set(report)=={'mesure','budget_ns','options','refus','environnement_avant',
        'environnement_apres','provenance','empreintes','statistiques','contrat','verdict','duree_s'},'report fields')
    need(report['mesure']=='MES-FULL' and equal(report['budget_ns'],LIMIT),'measurement/budget')
    need(equal(report['refus'],[]),'pilot refused/incomplete')
    options=report['options'];want=dict(fils=48,processus=5,passes=10,jobs=44,delai=900,essai=False,sequentiel=False,sonde=None)
    need(type(options) is dict and set(options)==set(want)|{'src','travail','donnees','sortie','archive_v12set'},'options fields')
    for k,v in want.items():need(equal(options[k],v),'option '+k)
    for k in ('src','travail','donnees','sortie','archive_v12set'):need(type(options[k]) is str and options[k],'option path')
    for when in ('avant','apres'):
        env=report['environnement_'+when]
        need(type(env) is dict and set(env)=={'nvcc','cmake','gpu','gpu_apps'},'environment fields')
        need(env['gpu_apps']=='','GPU not known idle '+when)
        need(all(type(env[k]) is str and env[k].strip() for k in ('nvcc','cmake','gpu')),'environment incomplete')
    p=report['provenance'];need(type(p) is dict and set(p)=={'sonde_sha256','pilote_sha256','lecteur_sha256','cmake'},'provenance fields')
    need(p['pilote_sha256']==c['sources']['microbancs/mes_full/pilote_full.py'] and p['lecteur_sha256']==c['sources']['microbancs/outils/lecteur_full.py'],'source provenance')
    h=p['sonde_sha256'];need(type(h) is str and len(h)==64 and set(h)<=set('0123456789abcdef'),'declared binary hash')
    need(type(p['cmake']) is list and p['cmake'] and all(type(x) is str for x in p['cmake']),'CMake metadata')
    for key,value in [('CMAKE_BUILD_TYPE','Release'),('MHGP12_COORD_BITS','21'),('MHGP12_ENABLE_CUDA','ON')]:
        need(any(x.startswith(key+':') and x.split('=',1)[-1]==value for x in p['cmake']),'CMake '+key)
    need(type(report['duree_s']) in (int,float) and math.isfinite(report['duree_s']) and report['duree_s']>=0,'campaign duration')
    for key,value in computed.items():need(equal(report[key],value),'report mismatch '+key)

def main():
    p=argparse.ArgumentParser();p.add_argument('--returned',type=Path,required=True);p.add_argument('--reader-patch',type=Path,required=True)
    p.add_argument('--repo',type=Path,required=True);p.add_argument('--plan',type=Path,required=True)
    a=p.parse_args();c=json_bytes((HERE/'capture.json').read_bytes());mraw=(HERE/'input_metadata.json').read_bytes()
    need(sha(a.plan.read_bytes())==c['plan_sha256'],'plan pin')
    need(sha(mraw)==c['input_metadata_sha256'],'metadata pin');metadata=json_bytes(mraw)
    report_raw=(a.returned/'rapport_full.json').read_bytes();need(sha(report_raw)==c['report_sha256'],'report pin');report=json_bytes(report_raw)
    paths=sorted((a.returned/'brut').glob('*.jsonl'));raws={p.name:p.read_bytes() for p in paths}
    need(report.get('provenance',{}).get('sonde_sha256')==c['declared_probe_sha256'],'probe declaration pin')
    old,new,pilot=readers(a.reader_patch,a.repo);computed,result=reconstruct(raws,metadata,old,new,pilot);review(report,computed,c)
    result.update(admission='complete',reader_versions_agree=True,verdict=computed['verdict'],contracts=computed['contrat'],
                  fingerprints=computed['empreintes'],codes='zero inferred from refus=[] under pinned producer',
                  files_sha256={'rapport_full.json':sha(report_raw),**{k:sha(v) for k,v in raws.items()}})
    need(sorted((a.returned/'brut').glob('*.jsonl'))==paths and (a.returned/'rapport_full.json').read_bytes()==report_raw and all(p.read_bytes()==raws[p.name] for p in paths),'modified during reading')
    print(json.dumps(result,sort_keys=True,separators=(',',':')))
if __name__=='__main__':main()
