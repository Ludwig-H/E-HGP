#!/usr/bin/env python3
"""MES-C3: hash/source/fermeture locale, sans contrôleur, donnée ni moteur."""
import argparse,csv,importlib.util,io,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--session',type=Path,required=True);ap.add_argument('--repo',type=Path,required=True);a=ap.parse_args()
    c=json.loads((HERE/'capture.json').read_text())
    helper=next(p for p in c['helpers'] if '/session_l1_recuperation/' in p)
    sp=importlib.util.spec_from_file_location('archive_reader',a.repo/helper);lib=importlib.util.module_from_spec(sp);sp.loader.exec_module(lib)
    for p,pin in c['helpers'].items():lib.same_hash((a.repo/p).read_bytes(),pin)
    for p,pin in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),pin)
    fs=lib.files((a.session/'results/results.tar.gz').read_bytes());lib.manifest(fs,c['manifest_count'])
    for p,pin in c['archive_files'].items():lib.same_hash(fs[p],pin)
    commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'))
    lib.need(commands==c['commands'],'commandes')
    plan=json.loads((a.session/'package/plan.json').read_text())
    pkg=lib.files((a.session/'package/package.tar.gz').read_bytes())
    for p,pin in c['source_files'].items():
        lib.same_hash(pkg[p],pin)
        blob=subprocess.check_output(['git','-C',str(a.repo),'show',c['source_git']+':'+p])
        lib.need(pkg[p]==blob,'source différente')
    for i,name in enumerate(c['command_meta']):
        base=f'results/cmd/{i:03d}_{name}/';folder='c' if i==0 else name
        meta=dict(l.split('=',1) for l in fs[base+'meta.txt'].decode().splitlines() if '=' in l)
        lib.need(all(meta[k]==v for k,v in c['command_meta'][name].items()),'fermeture de groupe')
        report=json.loads(fs[base+'files/'+folder+('/rapport_c.json' if i==0 else '/rapport_apparie.json')])
        lib.need({k:v for k,v in report['provenance'].items() if k.endswith('_sha256')}==c['reports'][name]['provenance'],'provenance rapport')
        argv=plan['commands'][i]['argv'];args=report['parametres']['argv']
        for k,v in c['planned_configuration'][name].items():
            lib.need(argv.count(k)==1 and argv[argv.index(k)+1]==v,'option plan')
            lib.need(args.count(k)==1 and args[args.index(k)+1]==v,'option rapport')
        if i:
            lib.need([argv[j+1] for j,v in enumerate(argv[:-1]) if v=='--bras']==c['paired_arms'][name],'bras préannoncés')
        cons=fs[base+'files/'+folder+'/construction.log'].decode()
        for key,values in c['construction'][name].items():
            got=sorted(set(re.findall(r'(?:-D)?'+key+r'(?::\w+)?=([^\s]+)',cons)))
            lib.need(got==values,'construction')
        pilot='mes_c_petits/pilote_c.py' if i==0 else 'mes_apparie/pilote_apparie.py'
        for key,p in [('pilote_sha256',pilot),('lecteur_sha256','outils/lecteur_full.py')]:
            lib.need(report['provenance'][key]==c['source_files']['morsehgp3D_v12/microbancs/'+p]['sha256'],'pilote/lecteur effectif')
    source_helper=next(p for p in c['helpers'] if p.endswith('/source_check.py'))
    py=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
    subprocess.run(py+[str(a.repo/source_helper),'--repo',str(a.repo),'--package',str(a.session/'package/package.tar.gz'),'--plan',str(a.session/'package/plan.json'),'--capture',str(HERE/'capture.json')],check=True)
    delta=subprocess.check_output(['git','-C',str(a.repo),'diff','--name-only',c['C2_source_git'],c['source_git'],'--','morsehgp3D_v12/src/','morsehgp3D_v12/bench/full_probe.cpp']).decode().splitlines()
    lib.need(delta==c['native_delta_vs_C2'],'delta source C2')
    r=json.loads((a.session/'receipt.json').read_text());cl=c['closure']
    for k,v in cl.items():
        if k not in ('errors_count','done','before_status','after_status'):
            lib.need(type(r[k]) is type(v) and r[k]==v,'clôture')
    lib.need(r['commit']==c['source_git'] and r['commands']==commands,'source/commande')
    lib.need(len(r['errors'])==cl['errors_count']==0 and int((a.session/'DONE').read_text())==cl['done']==0,'issue')
    lib.need(r['observed_before_stop']['status']==cl['before_status']=='RUNNING' and r['observed_after']['status']==cl['after_status']=='TERMINATED','arrêt')
    lib.need(r['package_sha256']==c['package_sha256'] and r['plan_sha256']==c['plan_sha256'],'déclaration paquet/plan')
    data=[{k:v[k] for k in ('name','size','sha256')} for v in r['data_files']]
    lib.need(data==c['data_declared'] and r['data_manifest_sha256']==c['data_manifest_sha256']==c['local_files']['package/data/SHA256SUMS']['sha256'],'données déclarées')
    for p,pin in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),pin)
    print('MES-C3: 357 sources Git, 504 entrées, trois commandes0, arrêt certifié; admission séparée')

if __name__=='__main__':main()
