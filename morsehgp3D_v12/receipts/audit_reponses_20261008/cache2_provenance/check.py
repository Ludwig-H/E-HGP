#!/usr/bin/env python3
"""Cache2b provenance only: closed local metadata, pinned Git sources; no engine or cloud."""
import argparse,csv,importlib.util,io,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True);ap.add_argument('--session',type=Path,required=True);a=ap.parse_args()
    c=json.loads((HERE/'capture.json').read_text());hp=next(p for p in c['helpers'] if '/session_l1_recuperation/' in p)
    sp=importlib.util.spec_from_file_location('archive_reader',a.repo/hp);lib=importlib.util.module_from_spec(sp);sp.loader.exec_module(lib)
    for p,pin in c['helpers'].items():lib.same_hash((a.repo/p).read_bytes(),pin)
    for p,pin in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),pin)
    fs=lib.files((a.session/'results/results.tar.gz').read_bytes());lib.manifest(fs,c['manifest_count'])
    for p,pin in c['archive_files'].items():lib.same_hash(fs[p],pin)
    commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));lib.need(commands==c['commands'],'commands')
    for i,name in enumerate(c['command_meta']):
        p=f'results/cmd/{i:03d}_{name}/meta.txt';m=dict(l.split('=',1) for l in fs[p].decode().splitlines() if '=' in l)
        lib.need(all(m[k]==v for k,v in c['command_meta'][name].items()),'group closure')
    pkg=lib.files((a.session/'package/package.tar.gz').read_bytes())
    for p,pin in c['source_files'].items():
        lib.same_hash(pkg[p],pin);git=subprocess.check_output(['git','-C',str(a.repo),'show',c['source_git']+':'+p]);lib.need(pkg[p]==git,'source')
    sh=next(p for p in c['helpers'] if p.endswith('/source_check.py'));py=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
    subprocess.run(py+[str(a.repo/sh),'--repo',str(a.repo),'--package',str(a.session/'package/package.tar.gz'),'--plan',str(a.session/'package/plan.json'),'--capture',str(HERE/'capture.json')],check=True)
    base='results/cmd/001_apparie_cache_ferme/files/apparie/'
    report=json.loads(fs[base+'rapport_apparie.json']);prov=report['provenance']
    lib.need({k:v for k,v in prov.items() if k.endswith('_sha256')}==c['report_provenance'],'report provenance')
    for key,p in [('pilote_sha256','mes_apparie/pilote_apparie.py'),('lecteur_sha256','outils/lecteur_full.py')]:lib.need(prov[key]==c['source_files']['morsehgp3D_v12/microbancs/'+p]['sha256'],'reader/pilot pin')
    lib.need(prov['sonde_sha256']==prov['sonde_fin_sha256'] and re.fullmatch('[0-9a-f]{64}',prov['sonde_sha256']) is not None,'ELF closing pair')
    plan=json.loads((a.session/'package/plan.json').read_text());argv=plan['commands'][1]['argv'];args=report['parametres']['argv']
    for k,v in c['paired_configuration'].items():
        for seq in (argv,args):lib.need(seq.count(k)==1 and seq[seq.index(k)+1]==v,'configuration')
    for seq in (argv,args):lib.need([seq[i+1] for i,v in enumerate(seq[:-1]) if v=='--bras']==c['paired_arms'],'arms')
    cons=fs[base+'construction.log'].decode()
    for key,values in c['construction_fields'].items():lib.need(sorted(set(re.findall(r'(?:-D)?'+key+r'(?::\w+)?=([^\s]+)',cons)))==values,'build configuration')
    config=fs['results/build/configure/argv.txt'].decode()
    lib.need('-DCMAKE_BUILD_TYPE=Release' in config and 'MHGP12_COORD_BITS' not in config and 'MHGP12_ENABLE_CUDA' not in config,'socle explicit options')
    log=fs['results/cmd/000_socle_ctest/stdout'].decode();rows=re.findall(r'\d+/\d+ Test\s+#\s*\d+:.*',log)
    stats={'selected':len(rows),'passed':sum('Passed' in x for x in rows),'skipped':sum('Skipped' in x for x in rows),'failed':sum('Failed' in x for x in rows)}
    lib.need(stats==c['ctest']=={'selected':730,'passed':730,'skipped':0,'failed':0} and '0 tests failed out of 730' in log,'primary CTest summary')
    region=[re.search(r'Test\s+#\s*\d+:\s+(\S+)',x).group(1) for x in rows if 'mhgp12_tower_region' in x]
    lib.need(region==c['region_tests_passed'] and len(region)==6,'native region gates')
    old=a.repo/c['M_data_reference']['path'];lib.same_hash(old.read_bytes(),{'bytes':old.stat().st_size,'sha256':c['M_data_reference']['sha256']})
    lib.need(json.loads(old.read_text())['data_declared']==c['data_declared'] and c['M_data_reference']['exact_same_declarations'] is True,'M input declarations')
    r=json.loads((a.session/'receipt.json').read_text());cl=c['closure']
    for k,v in cl.items():
        if k not in ('errors_count','done','before_status','after_status'):lib.need(type(r[k]) is type(v) and r[k]==v,'closure field')
    lib.need(r['commit']==c['source_git'] and r['commands']==commands,'source/commands')
    lib.need(len(r['errors'])==cl['errors_count']==0 and int((a.session/'DONE').read_text())==cl['done']==0,'error/DONE')
    lib.need(r['observed_before_stop']['status']==cl['before_status']=='RUNNING' and r['observed_after']['status']==cl['after_status']=='TERMINATED','stop')
    lib.need(r['package_sha256']==c['package_sha256'] and r['plan_sha256']==c['plan_sha256'],'package/plan')
    data=[{k:d[k] for k in ('name','size','sha256')} for d in r['data_files']]
    lib.need(data==c['data_declared'] and r['data_manifest_sha256']==c['data_manifest_sha256']==c['local_files']['package/data/SHA256SUMS']['sha256'],'data declarations')
    for p,pin in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),pin)
    print('cache2b verified: 359 Git sources, 264 manifest entries, 730 Passed, two commands0, ELF opening=closing, certified stop; numeric admission separate')
if __name__=='__main__':main()
