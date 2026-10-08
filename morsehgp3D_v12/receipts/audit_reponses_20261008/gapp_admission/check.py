#!/usr/bin/env python3
"""Closed G-APP archive and independent aggregate reread; no engine/cloud/build."""
import argparse,csv,hashlib,importlib.util,io,json,math,random,re,statistics as st,subprocess,sys,types
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def sha(b):return hashlib.sha256(b).hexdigest()
def need(ok,why):
    if not ok:raise ValueError(why)
def load(raw):
    def pairs(p):
        d={}
        for k,v in p:need(k not in d,'duplicate JSON key');d[k]=v
        return d
    return json.loads(raw,object_pairs_hook=pairs,parse_constant=lambda _: (_ for _ in ()).throw(ValueError('nonfinite')))
def gm(xs):return math.exp(sum(math.log(x) for x in xs)/len(xs))
def ci(xs):
    rng=random.Random(20261008);logs=[math.log(x) for x in xs];n=len(xs);draws=[]
    for _ in range(10000):draws.append(math.exp(sum(logs[rng.randrange(n)] for _ in range(n))/n))
    draws.sort();return [draws[int(.025*9999)],draws[math.ceil(.975*9999)]]
def positive(x):return type(x) in (int,float) and math.isfinite(x) and x>0

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True);ap.add_argument('--session',type=Path,required=True);a=ap.parse_args();c=load((HERE/'capture.json').read_text())
    hp=next(p for p in c['helpers'] if '/session_l1_recuperation/' in p);sp=importlib.util.spec_from_file_location('arc',a.repo/hp);lib=importlib.util.module_from_spec(sp);sp.loader.exec_module(lib)
    for p,v in c['helpers'].items():lib.same_hash((a.repo/p).read_bytes(),v)
    for p,v in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),v)
    fs=lib.files((a.session/'results/results.tar.gz').read_bytes());lib.manifest(fs,c['manifest_count'])
    for p,v in c['archive_files'].items():lib.same_hash(fs[p],v)
    pkg=lib.files((a.session/'package/package.tar.gz').read_bytes());prefix='morsehgp3D_v12/microbancs/mes_g_appareil/'
    pilot=types.ModuleType('gapp');pilot.__file__='pinned/pilote_g_appareil.py';exec(compile(pkg[prefix+'pilote_g_appareil.py'],'pinned_gapp','exec'),pilot.__dict__)
    need(pilot.auto_test()==[],'delivered judge self-test')
    sh=next(p for p in c['helpers'] if p.endswith('/source_check.py'));py=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
    p=subprocess.run(py+[str(a.repo/sh),'--repo',str(a.repo),'--package',str(a.session/'package/package.tar.gz'),'--plan',str(a.session/'package/plan.json'),'--capture',str(HERE/'capture.json')],check=True,capture_output=True,text=True)
    need(load(p.stdout)['files_exact']==366,'source proof')
    commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));need(commands==c['commands'],'commands')
    for i,name in enumerate(c['command_meta']):
        meta=dict(l.split('=',1) for l in fs[f'results/cmd/{i:03d}_{name}/meta.txt'].decode().splitlines() if '=' in l)
        need(all(meta[k]==v for k,v in c['command_meta'][name].items()),'command closure')
    plan=load((a.session/'package/plan.json').read_text());args=plan['commands'][1]['argv']
    for k,v in c['configuration'].items():need(args.count(k)==1 and args[args.index(k)+1]==v,'plan config')
    base='results/cmd/001_g_app_pilote/files/g_app/'
    camp=load(fs[base+'campagne.json']);report=load(fs[base+'rapport_g_appareil.json']);build=load(fs[base+'construction.json'])
    need(build['bits']=='21' and build['appareil'] is True,'build regime')
    for k,h in build['sources'].items():need(sha(pkg[prefix+k])==h,'micro source')
    for k,h in build['portees'].items():need(sha(pkg['morsehgp3D_v12/'+k])==h,'ported source')
    for h in [*build['binaires'].values(),build['bibliotheque']]:need(type(h) is str and re.fullmatch('[0-9a-f]{64}',h),'build digest')
    for k in ('bits','nvcc','gxx','binaires','sources','portees','bibliotheque','appareil'):need(report['construction'][k]==build[k],'report build')
    need(type(camp['processus']) is int and camp['processus']==5 and type(camp['passes']) is int and camp['passes']==5,'campaign cardinality')
    need(camp['appareil'] is True and camp['empreintes_stables'] is True and camp['isolation_ok'] is True,'declared closure/isolation')
    names=['ng00','mediane','max'];need(set(camp['trames'])==set(names),'frames')
    tags=[f'{n}_p{i}' for i in range(5) for n in names]+['mutant_cote_nul']
    need([x['prise'] for x in camp['isolation']]==tags and all(x['avant']==x['apres']=='' for x in camp['isolation']),'isolation endpoints')
    expected_sites=dict(ng00=39885,mediane=64740,max=99099);inputs=dict(ng00='lidar_ng00',mediane='kitti_ng_02_001606',max='kitti_ng_08_002119')
    for n,b in inputs.items():
        xyz=next(x for x in c['data_declared'] if x['name']==b+'.u32le');ids=next(x for x in c['data_declared'] if x['name']==b+'.ids.u32le')
        need(xyz['size']==12*expected_sites[n] and ids['size']==4*expected_sites[n],'declared input size')
    measures=['census','sondes','propositions'];table={};counts={};seen=set();allaa=[]
    for name in names:
        takes=camp['trames'][name];need(type(takes) is list and len(takes)==5,'process count');vals={m:[] for m in measures};infos=[]
        for i,take in enumerate(takes):
            filename=f'{name}_p{i}.jsonl';need(take['journal']==filename and filename not in seen,'process label');seen.add(filename)
            raw=fs[base+'journaux/'+filename];need(sha(raw)==take['journal_sha256'] and raw.decode().splitlines()==take['lignes'],'raw summary identity')
            need(type(take['code']) is int and take['code']==0 and fs[base+'journaux/'+filename.removesuffix('.jsonl')+'.stderr']==b'','native return')
            rows=[load(x) for x in raw.decode().splitlines()];need([x['phase'] for x in rows]==['recolte','appareil']+['passe']*6+['transferts','identite'],'phase sequence')
            recolte,device=rows[:2];identity=rows[-1];passes=rows[2:8]
            need(recolte['trame']==name and recolte['k']==5 and type(recolte['k']) is int and recolte['fils']==48 and type(recolte['fils']) is int and recolte['sites']==expected_sites[name] and recolte['recolte_ok'] is True,'native config')
            need(identity['identite'] is True and identity['appareil'] is True,'identity')
            for block in ['census','sondes','propositions']:
                for k,v in identity[block].items():
                    if k=='coherentes':need(v is True,'probe coherent')
                    else:need(type(v) is int and v>=0,'identity counter type')
                    if k.startswith('ecarts_') or k=='non_resolues':need(v==0,'identity mismatch/unresolved')
            cc=identity['census'];ss=identity['sondes'];pp=identity['propositions']
            need(cc['requetes']==cc['comparees_hote_hd']==cc['comparees_appareil']==recolte['requetes']==recolte['attendues'],'census coverage')
            need(ss['comparees_appareil']==recolte['representants'] and ss['reussies_produit']==ss['reussies_appareil']==ss['premieres_sondes_reussies_g']==recolte['premieres_sondes_reussies'],'probes coverage')
            need(pp['abouties']==pp['parties']==recolte['representants']-recolte['premieres_sondes_reussies'],'proposal coverage')
            for j,row in enumerate(passes):
                need(type(row['passe']) is int and row['passe']==j and row['echauffement'] is (j==0) and row['ok'] is True,'pass sequence')
                for m in measures:
                    for key in ['hote_ms','appareil_ms']:
                        v=row[m][key];need(type(v) is list and len(v)==2 and all(positive(x) for x in v),'times')
            for m in measures:
                hot=passes[1:];h=st.median(x[m]['hote_ms'][0] for x in hot);d=st.median(x[m]['appareil_ms'][0] for x in hot)
                v=dict(h=h,d=d,ratio=d/h,aa_h=st.median(x[m]['hote_ms'][1]/x[m]['hote_ms'][0] for x in hot),aa_d=st.median(x[m]['appareil_ms'][1]/x[m]['appareil_ms'][0] for x in hot))
                if m!='sondes':
                    need(all(positive(x[m]['hote_hd_ms']) for x in hot),'CPU HD time');v['hd']=st.median(x[m]['hote_hd_ms'] for x in hot);v['hd_ratio']=v['hd']/h
                vals[m].append(v)
            infos.append({k:recolte[k] for k in ('sites','requetes','distinctes','representants')});infos[-1]['parties']=pp['parties']
        need(all(x==infos[0] for x in infos),'work stable across processes');counts[name]=infos[0];table[name]={}
        for m,values in vals.items():
            row=dict(cpu_produit_ms=st.median(x['h'] for x in values),gpu_noyau_ms=st.median(x['d'] for x in values),ratio_gpu_cpu=gm([x['ratio'] for x in values]),ic95=ci([x['ratio'] for x in values]),aa_cpu=gm([x['aa_h'] for x in values]),aa_gpu=gm([x['aa_d'] for x in values]))
            if m!='sondes':row.update(cpu_hd_ms=st.median(x['hd'] for x in values),ratio_hd_produit=gm([x['hd_ratio'] for x in values]))
            e=report['trames'][name]['mesures'][m];mapping={'cpu_produit_ms':'t_hote_ms','gpu_noyau_ms':'t_app_ms','ratio_gpu_cpu':'rapport','aa_cpu':'aa_hote','aa_gpu':'aa_app'}
            need(all(math.isclose(row[k],e[v],rel_tol=2e-15,abs_tol=0) for k,v in mapping.items()) and all(math.isclose(x,y,rel_tol=2e-15,abs_tol=0) for x,y in zip(row['ic95'],e['ic95'])),'independent aggregate')
            need(.9<=row['aa_cpu']<=1.1 and .9<=row['aa_gpu']<=1.1,'AA');allaa.extend([row['aa_cpu'],row['aa_gpu']]);table[name][m]=row
    need({Path(p).name for p in fs if p.startswith(base+'journaux/') and p.endswith('.jsonl')}==seen|{'mutant_cote_nul.jsonl'},'journal cohort')
    mut=camp['mutant'];need(type(mut['code']) is int and mut['code']==1 and mut['identite'] is False and mut['journal']=='mutant_cote_nul.jsonl','mutant return')
    mr=[load(x) for x in fs[base+'journaux/mutant_cote_nul.jsonl'].decode().splitlines()];mi=mr[-1]
    need(mi['phase']=='identite' and mi['identite'] is False and mi['census']['ecarts_hote_hd']==mi['census']['ecarts_appareil']==232162,'causal native mismatch')
    judged=pilot.juger(camp)
    differences=[]
    def equivalent(x,y,path=''):
        need(type(x) is type(y),'report type '+path)
        if isinstance(x,dict):
            need(set(x)==set(y),'report keys '+path)
            for k in x:equivalent(x[k],y[k],path+'/'+k)
        elif isinstance(x,list):
            need(len(x)==len(y),'report length '+path)
            for j,(u,v) in enumerate(zip(x,y)):equivalent(u,v,path+'/'+str(j))
        elif type(x) is float:
            need(math.isclose(x,y,rel_tol=2e-15,abs_tol=0),'report float '+path)
            if x!=y:differences.append({'path':path,'replay':x,'report':y,'ulps':abs(x-y)/math.ulp(y)})
        else:need(x==y,'report value '+path)
    equivalent(judged,{k:report[k] for k in judged})
    need(judged['verdict']=='rejete' and judged['refus']==[] and len(judged['rejets'])==3,'rule outcome')
    rr=load((a.session/'receipt.json').read_text());cl=c['closure']
    for k,v in cl.items():
        if k not in ('errors_count','done','before_status','after_status'):need(type(rr[k]) is type(v) and rr[k]==v,'closure')
    need(rr['commit']==c['source_git'] and rr['commands']==commands and len(rr['errors'])==0 and int((a.session/'DONE').read_text())==0,'receipt source/exit')
    need(rr['observed_before_stop']['status']=='RUNNING' and rr['observed_after']['status']=='TERMINATED','stop')
    need([{k:d[k] for k in ('name','size','sha256')} for d in rr['data_files']]==c['data_declared'] and rr['data_manifest_sha256']==c['data_manifest_sha256'],'data metadata')
    for p,v in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),v)
    print(json.dumps(dict(source=c['source_git'],sources=366,manifest_entries=87,nominal_processes=15,passes_nominal=90,warm=75,mutant_processes=1,verdict=judged['verdict'],aa_min=min(allaa),aa_max=max(allaa),counts=counts,table=table,identity_differences=0,unresolved=0,mutant_differences=232162,isolated_endpoints=32,report_float_differences=differences,native_calls_by_audit=0),ensure_ascii=False,indent=2,sort_keys=True))
if __name__=='__main__':main()
