#!/usr/bin/env python3
"""GAPP2: closed archive + actual cohorts + independent arithmetic; no native/cloud/build."""
import argparse,csv,hashlib,importlib.util,io,json,math,re,statistics as st,subprocess,sys,types
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True);ap.add_argument('--session',type=Path,required=True);a=ap.parse_args()
    c=json.loads((HERE/'capture.json').read_text());modules={}
    for path,pin in c['helpers'].items():
        raw=(a.repo/path).read_bytes()
        if len(raw)!=pin['bytes'] or hashlib.sha256(raw).hexdigest()!=pin['sha256']:raise ValueError('helper pin')
        if path.endswith('/source_check.py'):continue
        mod=types.ModuleType(path);mod.__file__=str(a.repo/path);exec(compile(raw,path,'exec'),mod.__dict__);modules[path]=mod
    lib=next(v for k,v in modules.items() if '/session_l1_recuperation/' in k)
    old=next(v for k,v in modules.items() if '/gapp_admission/' in k)
    need,load,sha,gm,ci,positive=old.need,old.load,old.sha,old.gm,old.ci,old.positive
    for p,v in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),v)
    fs=lib.files((a.session/'results/results.tar.gz').read_bytes());lib.manifest(fs,c['manifest_count'])
    for p,v in c['archive_files'].items():lib.same_hash(fs[p],v)
    pkg=lib.files((a.session/'package/package.tar.gz').read_bytes());prefix='morsehgp3D_v12/microbancs/mes_g_appareil/'
    sh=next(p for p in c['helpers'] if p.endswith('/source_check.py'));py=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
    proof=subprocess.run(py+[str(a.repo/sh),'--repo',str(a.repo),'--package',str(a.session/'package/package.tar.gz'),'--plan',str(a.session/'package/plan.json'),'--capture',str(HERE/'capture.json')],check=True,capture_output=True,text=True)
    need(load(proof.stdout)['files_exact']==367,'source proof')
    pilot=types.ModuleType('gapp2');pilot.__file__='pinned/pilote_g_appareil.py';exec(compile(pkg[prefix+'pilote_g_appareil.py'],'pinned_gapp2','exec'),pilot.__dict__)
    need(pilot.auto_test()==[],'delivered judge self-test')
    commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));need(commands==c['commands'],'commands')
    for i,name in enumerate(c['command_meta']):
        meta=dict(l.split('=',1) for l in fs[f'results/cmd/{i:03d}_{name}/meta.txt'].decode().splitlines() if '=' in l)
        need(all(meta[k]==v for k,v in c['command_meta'][name].items()),'command closure')
    plan=load((a.session/'package/plan.json').read_text());args=plan['commands'][1]['argv']
    for k,v in c['configuration'].items():need(args.count(k)==1 and args[args.index(k)+1]==v,'plan config')
    base='results/cmd/001_g_app_pilote/files/g_app/'
    camp=load(fs[base+'campagne.json']);report=load(fs[base+'rapport_g_appareil.json']);build=load(fs[base+'construction.json'])
    need(build['bits']=='21' and build['appareil'] is True,'build regime')
    need(len(build['commandes'])==6 and all(type(q['code'])is int and q['code']==0 for q in build['commandes']),'construction codes')
    for q in build['commandes']:
        flags=['-O3','-arch=sm_120','-fmad=false'] if q['nom'].endswith('_nvcc') else ['-O3','-DNDEBUG','-DMHGP12_COORD_BITS=21']
        need(all(f in q['argv'] for f in flags),'construction options')
    thresholds={'D2':{'total':.10,'propositions':.20},'D1':{'total':.15,'propositions':.50,'neutralite_hote':1.05}}
    need(report['seuils']==pilot.SEUILS==thresholds and report['regle']=='REGLE_G_APPAREIL_2','preregistered rule')
    for k,h in build['sources'].items():need(sha(pkg[prefix+k])==h,'micro source')
    for k,h in build['portees'].items():need(sha(pkg['morsehgp3D_v12/'+k])==h,'ported source')
    for h in [*build['binaires'].values(),build['bibliotheque']]:need(type(h)is str and re.fullmatch('[0-9a-f]{64}',h),'build digest')
    for k in ('bits','nvcc','gxx','binaires','sources','portees','bibliotheque','appareil'):need(report['construction'][k]==build[k],'report build')
    need(type(camp['processus'])is int and camp['processus']==5 and type(camp['passes'])is int and camp['passes']==5,'campaign cardinality')
    need(camp['appareil'] is True and camp['empreintes_stables'] is True and camp['isolation_ok'] is True,'declared closure/isolation')
    names=['ng00','mediane','max'];need(set(camp['trames'])==set(names),'frames')
    tags=[f'{n}_p{i}' for i in range(5) for n in names]+['mes_g_app_mutant_cote_nul','mes_g_app_mutant_l4','information_k10']
    need([x['prise'] for x in camp['isolation']]==tags and all(x['avant']==x['apres']=='' for x in camp['isolation']),'isolation endpoints')
    sites=dict(ng00=39885,mediane=64740,max=99099,ng00_k10=39885)
    for n,b in dict(ng00='lidar_ng00',mediane='kitti_ng_02_001606',max='kitti_ng_08_002119').items():
        xyz=next(x for x in c['data_declared'] if x['name']==b+'.u32le');ids=next(x for x in c['data_declared'] if x['name']==b+'.ids.u32le')
        need(xyz['size']==12*sites[n] and ids['size']==4*sites[n],'declared size')
    measures=['census','sondes','propositions','propositions_l4','propositions_l4f32'];seen=set()
    def numbers(d):
        for k,v in d.items():
            if type(v)is dict:numbers(v)
            elif k=='coherentes':need(v is True,'probe coherence')
            else:need(type(v)is int and v>=0,'unsigned counter')
    def take(t,name,k,n,filename,mutant=None):
        need(t['journal']==filename and filename not in seen,'unique label');seen.add(filename)
        raw=fs[base+'journaux/'+filename]
        need(raw.decode().splitlines()==t['lignes'],'raw summary')
        if 'journal_sha256' in t:need(sha(raw)==t['journal_sha256'],'raw hash')
        need(type(t['code'])is int and t['code']==(1 if mutant=='cote_nul' else 0),'external code')
        need(fs[base+'journaux/'+filename.removesuffix('.jsonl')+'.stderr']==b'','stderr')
        rows=[load(x) for x in raw.decode().splitlines()]
        need([x['phase'] for x in rows]==['recolte','appareil']+['passe']*(n+1)+['transferts','identite'],'ordered closed phases')
        rc,device=rows[:2];identity=rows[-1];passes=rows[2:2+n+1]
        for field,value in [('k',k),('fils',48),('sites',sites[name])]:need(type(rc[field])is int and rc[field]==value,'config')
        need(rc['trame']==name and rc['recolte_ok'] is True,'collection')
        need(identity['appareil'] is True and identity['identite'] is (mutant!='cote_nul'),'identity flag/code')
        for block in measures:numbers(identity[block])
        cc,ss,pp=(identity[b] for b in measures[:3]);parties=pp['parties']
        need(cc['requetes']==cc['comparees_hote_hd']==cc['comparees_appareil']==rc['requetes']==rc['attendues'],'census coverage')
        need(ss['comparees_appareil']==rc['representants'] and ss['reussies_produit']==ss['reussies_appareil']==ss['premieres_sondes_reussies_g']==rc['premieres_sondes_reussies'],'probe coverage')
        need(pp['abouties']==parties==rc['representants']-rc['premieres_sondes_reussies'],'proposal coverage')
        for block in measures:
            for key,value in identity[block].items():
                if key.startswith('ecarts_') or key in ['coquilles_larges','non_resolues','issues_differentes_p64']:
                    expected=232162 if mutant=='cote_nul' and block=='census' and key in ['ecarts_hote_hd','ecarts_appareil'] else 0
                    need(value==expected,'identity difference')
        for block in measures[2:]:
            b=identity[block];need(set(b['mecanismes'])=={'t1','certificat','repli_sans_proposition','repli_certificat'} and sum(b['mecanismes'].values())==parties,'mechanism accounting')
            need(set(b['issues'])=={'table','census','refus'} and sum(b['issues'].values())==parties and b['issues']['refus']==0,'outcome accounting')
            need(b['issues']==pp['issues'],'outcome equality')
        need(identity['propositions_l4']['entieres']<=parties,'integer route count')
        vals={}
        for j,row in enumerate(passes):
            need(type(row['passe'])is int and row['passe']==j and row['echauffement'] is (j==0) and row['ok'] is True,'pass sequence')
            for m in measures:
                for key in ['hote_ms','appareil_ms']:
                    v=row[m][key];need(type(v)is list and len(v)==2 and all(positive(x) for x in v),'timings')
        hot=passes[1:]
        for m in measures:
            vals[m]={'h':st.median(x[m]['hote_ms'][0] for x in hot),'d':st.median(x[m]['appareil_ms'][0] for x in hot),'aa_h':st.median(x[m]['hote_ms'][1]/x[m]['hote_ms'][0] for x in hot),'aa_d':st.median(x[m]['appareil_ms'][1]/x[m]['appareil_ms'][0] for x in hot)}
        h=sum(vals[m]['h'] for m in measures[:3]);d=vals['census']['d']+vals['sondes']['d'];ph=vals['propositions']['h']
        ratios={'D2':{'total':(d+vals['propositions_l4f32']['d'])/h,'propositions':vals['propositions_l4f32']['d']/ph},'D1':{'total':(d+vals['propositions_l4']['d'])/h,'propositions':vals['propositions_l4']['d']/ph,'neutralite_hote':vals['propositions_l4']['h']/ph}}
        return {'measures':vals,'ratios':ratios,'counts':{key:rc[key] for key in ['sites','requetes','distinctes','representants']},'identity':identity,'device':device}
    table={};allaa=[];counts={};identities={}
    def close(x,y):return math.isclose(x,y,rel_tol=2e-15,abs_tol=0)
    for name in names:
        ts=camp['trames'][name];need(type(ts)is list and len(ts)==5,'exact process count')
        vs=[take(t,name,5,5,f'{name}_p{i}.jsonl') for i,t in enumerate(ts)]
        need(all(v['counts']==vs[0]['counts'] and v['identity']==vs[0]['identity'] and {k:x for k,x in v['device'].items() if not k.endswith('_ms')}=={k:x for k,x in vs[0]['device'].items() if not k.endswith('_ms')} for v in vs),'stable work/identity/device')
        counts[name]=vs[0]['counts'];identities[name]=vs[0]['identity'];table[name]={'measures':{},'ratios':{}}
        for m in measures:
            e={'cpu_ms':st.median(v['measures'][m]['h'] for v in vs),'gpu_ms':st.median(v['measures'][m]['d'] for v in vs),'aa_cpu':gm([v['measures'][m]['aa_h'] for v in vs]),'aa_gpu':gm([v['measures'][m]['aa_d'] for v in vs])}
            mapping={'cpu_ms':'t_hote_ms','gpu_ms':'t_app_ms','aa_cpu':'aa_hote','aa_gpu':'aa_app'}
            need(all(close(e[k],report['trames'][name]['mesures'][m][v]) for k,v in mapping.items()),'independent timing aggregate')
            if m in measures[:4]:allaa.append(e['aa_cpu'])
            if m in ['census','sondes','propositions_l4','propositions_l4f32']:allaa.append(e['aa_gpu'])
            table[name]['measures'][m]=e
        for design in ['D2','D1']:
            table[name]['ratios'][design]={}
            for metric in vs[0]['ratios'][design]:
                xs=[v['ratios'][design][metric] for v in vs];e={'ratio':gm(xs),'ic95':ci(xs),'threshold':pilot.SEUILS[design][metric]}
                e['passes']=e['ic95'][1]<=e['threshold'];published=report['trames'][name]['rapports'][design][metric]
                need(close(e['ratio'],published['rapport']) and all(close(x,y) for x,y in zip(e['ic95'],published['ic95'])),'independent ratio/CI')
                table[name]['ratios'][design][metric]=e
    need(all(.9<=x<=1.1 for x in allaa),'decisive AA')
    need(set(camp['mutants'])=={'cote_nul','l4'},'mutant cohort')
    mutants={m:take(camp['mutants'][m],'ng00',5,1,'mes_g_app_mutant_'+m+'.jsonl',m) for m in ['cote_nul','l4']}
    mi=mutants['l4']['identity'];mp=mi['propositions']['parties'];replis={m:mi[m]['mecanismes']['repli_certificat']+mi[m]['mecanismes']['repli_sans_proposition'] for m in measures[3:]}
    need(all(n==429726 and n/mp>.001 for n in replis.values()) and mp==847125,'L4 mutant criterion')
    k10=take(camp['information_k10'],'ng00_k10',10,2,'information_k10.jsonl')
    need({Path(p).name for p in fs if p.startswith(base+'journaux/') and p.endswith('.jsonl')}==seen and len(seen)==18,'journal cohort')
    judged=pilot.juger(camp);diff=[]
    def equal(x,y,path=''):
        need(type(x)is type(y),'report type '+path)
        if type(x)is dict:
            need(set(x)==set(y),'report keys '+path)
            for k in x:equal(x[k],y[k],path+'/'+k)
        elif type(x)is list:
            need(len(x)==len(y),'report length '+path)
            for j,(u,v) in enumerate(zip(x,y)):equal(u,v,path+'/'+str(j))
        elif type(x)is float:
            need(close(x,y),'report float '+path)
            if x!=y:diff.append({'path':path,'replay':x,'report':y,'ulps':abs(x-y)/math.ulp(y)})
        else:need(x==y,'report value '+path)
    equal(judged,{k:report[k] for k in judged})
    need(judged['verdicts']=={'D2':'rejete','D1':'rejete'} and not judged['refus'],'rejection without refusal')
    need(all(any(not e['passes'] for metrics in table.values() for e in metrics['ratios'][d].values()) for d in ['D2','D1']),'independent threshold rejection')
    rr=load((a.session/'receipt.json').read_text())
    for k,v in c['closure'].items():
        if k not in ('errors_count','done','before_status','after_status'):need(type(rr[k])is type(v) and rr[k]==v,'closure')
    need(rr['commit']==c['source_git'] and rr['commands']==commands and not rr['errors'] and int((a.session/'DONE').read_text())==0,'receipt/exit')
    need(rr['observed_before_stop']['status']=='RUNNING' and rr['observed_after']['status']=='TERMINATED','certified stop')
    need(rr['package_sha256']==c['package_sha256'] and rr['plan_sha256']==c['plan_sha256'],'receipt sources')
    need([{k:d[k] for k in ('name','size','sha256')} for d in rr['data_files']]==c['data_declared'] and rr['data_manifest_sha256']==c['data_manifest_sha256'],'input metadata')
    for p,v in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),v)
    print(json.dumps({'source':c['source_git'],'manifest_entries':92,'sources':367,'nominal_k5_processes':15,'nominal_k5_passes':90,'nominal_k5_warm':75,'information_k10_processes':1,'information_k10_passes':3,'mutant_processes':2,'mutant_passes':4,'all_passes':97,'isolated_endpoints':36,'verdicts':judged['verdicts'],'decision':judged['decision'],'refus':judged['refus'],'aa_min':min(allaa),'aa_max':max(allaa),'counts':counts,'identities':identities,'table':table,'k10':k10,'mutants':{'cote_nul':{'code':1,'census_mismatches':232162},'l4':{'code':0,'identity':True,'fallback_count':429726,'parts':847125,'fallback_fraction':429726/847125}},'report_float_differences':diff,'native_calls_by_audit':0},ensure_ascii=False,indent=2,sort_keys=True))
if __name__=='__main__':main()
