#!/usr/bin/env python3
"""Objets Git et lecteurs Python uniquement ; aucun moteur HGP ni appel cloud."""
import argparse, contextlib, copy, hashlib, importlib.util, inspect, io, json, os, subprocess, sys, tempfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
P='morsehgp3D_v12/'
def need(ok,msg):
    if not ok: raise RuntimeError(msg)
def sha(b): return hashlib.sha256(b).hexdigest()
def module(path):
    s=importlib.util.spec_from_file_location('audit_helper',path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def replay(repo,gc_git=None):
    pin=json.loads((HERE/'pins.json').read_text())
    def git(*args,cwd=repo): return subprocess.check_output(['git',*args],cwd=cwd)
    source={p:git('show',pin['gc2']+':'+p) for p in pin['sources']}
    need({p:sha(b) for p,b in source.items()}==pin['sources'],'sources Git differentes')
    for p,h in pin['dependencies'].items(): need(sha((repo/p).read_bytes())==h,'dependance differente: '+p)
    need(git('diff','--name-only',pin['base'],pin['gc2'],'--',P).decode().splitlines()==pin['changed_paths'],'delta Git different')
    for p,h in pin['untouched_trees'].items():
        need(all(git('rev-parse',v+':'+p).decode().strip()==h for v in (pin['base'],pin['gc2'])),'arbre modifie: '+p)
    if gc_git:
        for p in pin['tower_paths']: need(git('show',pin['prototype']+':'+p,cwd=gc_git)==source[p],'prototype different')
    pub=repo/(P+'receipts/audit_reponses_20261007/t2c_pilote_proposition')
    helper=module(pub/'check.py');m=helper.load(source[P+'microbancs/mes_t2c_g/pilote_t2c.py'],'livre')
    out={'tower_sources_match_captured_45976':len(pin['tower_paths']),'core_catalogue_unchanged':True,'native_runs':0}
    with tempfile.TemporaryDirectory(prefix='gc_livraison_') as td:
        root=Path(td);judge=[]
        for name in ('g_determinism.py','g_fausse_sonde.py'):
            p=root/name;p.write_bytes(source[P+'tests/tower/'+name]);p.chmod(0o700)
        py=[sys.executable,'-B','-S']+(['-O'] if sys.flags.optimize else [])
        for mode,code in [('',0),('k1_seul',2),('ordre_double',2),('empreinte_courte',2),('sans_sortie',2),
                          ('sortie_ordre_booleen',2),('sortie_ordre_flottant',2),('travail_8',1),('objet_8',1)]:
            got=subprocess.run(py+[str(root/'g_determinism.py'),str(root/'g_fausse_sonde.py'),'audit',
                '--uniform=10,20261007,18','--k=5','--threads=1,8'],env=dict(os.environ,MHGP12_FAUSSE_SONDE=mode),
                capture_output=True,text=True,timeout=10)
            need(got.returncode==code,(mode,got.stdout,got.stderr));judge.append([mode or 'nominal',code])
        out['juge_G']=judge
        old,capture=helper.fixtures();trials=helper.trials(m,root,old,capture);native=helper.native_formats(m,root)
        # Seule adaptation de la cohorte synthetique publiee : minimum huit -> dix tours.
        body=inspect.getsource(helper.campaigns)
        need(body.count('"tours_demandes": 8')==1 and body.count('range(8)')==1,'cohorte publiee modifiee')
        body=body.replace('"tours_demandes": 8','"tours_demandes": 10').replace('range(8)','range(10)')
        namespace=dict(helper.__dict__);exec(compile(body,'campaign_10','exec'),namespace)
        campaign=namespace['campaigns'](m,root,old,capture)
        with contextlib.redirect_stdout(io.StringIO()): need(m.etape_auto_test()==0,'auto-test')
        real_auto,real_report=m.etape_auto_test,m.etape_rapport;gates=[]
        for status in (0,1):
            events=[];m.etape_auto_test=lambda:events.append('auto') or status
            m.etape_rapport=lambda *_:events.append('rapport') or {'verdicts':{},'refus':[]}
            with contextlib.redirect_stdout(io.StringIO()):
                code=m.main(['pilote','rapport','--sortie',str(root/'out'),'--travail',str(root),'--processus','10'])
            need(events==(['auto','rapport'] if status==0 else ['auto']),'garde rapport');gates.append([status,code,events])
        with contextlib.redirect_stdout(io.StringIO()):
            default=m.main(['pilote','rapport','--sortie',str(root/'out'),'--travail',str(root)])
        m.etape_auto_test,m.etape_rapport=real_auto,real_report
        need(default==2 and m.REGLE_T2C['processus_min']==10,'defaut/minimum change')
        out['pilote']={'takes':[[x['case'],x['valide']] for x in trials],'formats':native,'campaigns':campaign,
            'auto_test':0,'report_guards':gates,'default_cli':default,'minimum_tours':10,
            'collecte_catalogue':hasattr(m,'prise_catalogue')}
        # D6 : vrai format avant/apres, puis corruption des seuls diagnostics ajoutes.
        rel=P+'microbancs/mes_d6_profils/pilote_d6.py';d=helper.load(source[rel],'d6_livre')
        target=root/rel;target.parent.mkdir(parents=True);target.write_bytes(source[rel])
        for extra in (['--check'],[]):
            subprocess.run(['git','apply',*extra,str(HERE/'d6_g_schemas_proposed.patch')],cwd=root,check=True,capture_output=True)
        fixed=helper.load(target.read_bytes(),'d6_propose');expected=dict(passes=2,coord_bits=21,kmax=5,threads=1,leaf=24)
        def admits(mod,rows,config=expected):return mod.summarize(0,rows,'tour_g','resolution_sha256',config)['ok']
        before,after=([json.loads(line) for line in (pub/(name+'.jsonl')).read_bytes().splitlines()] for name in ('before','after'))
        need(admits(d,before) and not admits(d,after),'incompatibilite non reproduite')
        need(admits(fixed,before) and admits(fixed,after),'vrai format refuse par proposition')
        names=set(after[0]['diagnostics'])-set(before[0]['diagnostics']);projected=copy.deepcopy(after)
        for row in projected:
            if row['phase']=='tour_g':
                for key in names:row['diagnostics'].pop(key)
        need(admits(d,projected),'diagnostic causal different')
        refused=[]
        def reject(name,rows):need(not admits(fixed,rows),'corruption admise: '+name);refused.append(name)
        for key in sorted(names):
            rows=copy.deepcopy(after);rows[0]['diagnostics'].pop(key);reject('missing_'+key,rows)
            rows=copy.deepcopy(after);value=rows[0]['diagnostics'][key]
            if type(value) is list:value[0]=False
            else:rows[0]['diagnostics'][key]=False
            reject('bool_'+key,rows)
            if type(after[0]['diagnostics'][key]) is list:
                rows=copy.deepcopy(after);rows[0]['diagnostics'][key].pop();reject('length_'+key,rows)
        rows=copy.deepcopy(after);rows[0]['diagnostics']['extra']=0;reject('unknown',rows)
        rows=copy.deepcopy(after);rows[1]['diagnostics']=copy.deepcopy(before[1]['diagnostics']);reject('mix_after_before',rows)
        rows=copy.deepcopy(before);rows[1]['diagnostics']=copy.deepcopy(after[1]['diagnostics']);reject('mix_before_after',rows)
        for label,value in [('negative',-1),('float',0.0),('overflow',1<<64)]:
            rows=copy.deepcopy(after);rows[0]['diagnostics']['prepare_ns']=value;reject(label,rows)
        # K=1 de forme uniquement : tableaux K-1 vides, aucun nouveau calcul geometrique.
        k1=copy.deepcopy(after);k1=[r for r in k1 if r['phase']!='ordre' or r['k']==1]
        for row in k1:
            if row['phase']=='tour_g':
                row['kmax']=1
                for key in ('order_ns','pass_ns'):row['diagnostics'][key]=row['diagnostics'][key][:1]
                for key in ('table_ns','join_ns'):row['diagnostics'][key]=[]
        need(admits(fixed,k1,dict(expected,kmax=1)),'K1 de forme refuse')
        out['D6']={'old_native_admitted':True,'gc_native_admitted_live':False,'diagnostic_projection_causal_only':True,
            'proposed_native_before_after':[True,True],'extra_diagnostics':sorted(names),'refused_mutations':refused,
            'synthetic_K1_admitted':True,'patched_sha256':sha(target.read_bytes())}
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,default=HERE.parents[3]);p.add_argument('--gc-git',type=Path);p.add_argument('--check',action='store_true');a=p.parse_args();out=replay(a.repo.resolve(),a.gc_git)
    if a.check:need(out==json.loads((HERE/'results.json').read_text()),'resultats differents');print('gc_livraison_ok: sources, lecteurs et proposition D6 ; aucun natif')
    else:print(json.dumps(out,indent=1,sort_keys=True))
