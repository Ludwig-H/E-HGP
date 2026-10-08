#!/usr/bin/env python3
"""B1p: read closed local metadata and Git; never execute a probe/controller."""
import argparse,csv,importlib.util,io,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def main():
    ap=argparse.ArgumentParser()
    for name in ('repo','session','previous-session'):ap.add_argument('--'+name,type=Path,required=True)
    a=ap.parse_args();c=json.loads((HERE/'capture.json').read_text())
    hp=next(p for p in c['helpers'] if '/session_l1_recuperation/' in p)
    # Check the reused reader before importing it.
    import hashlib
    for p,pin in c['helpers'].items():
        b=(a.repo/p).read_bytes()
        if len(b)!=pin['bytes'] or hashlib.sha256(b).hexdigest()!=pin['sha256']:raise ValueError('helper pin')
    spec=importlib.util.spec_from_file_location('archive_reader',a.repo/hp)
    lib=importlib.util.module_from_spec(spec);spec.loader.exec_module(lib)
    for p,pin in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),pin)
    fs=lib.files((a.session/'results/results.tar.gz').read_bytes());lib.manifest(fs,c['manifest_count'])
    for p,pin in c['archive_files'].items():lib.same_hash(fs[p],pin)
    commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'))
    lib.need(commands==c['commands'],'command table')
    meta=dict(l.split('=',1) for l in fs['results/cmd/000_mes_b/meta.txt'].decode().splitlines() if '=' in l)
    lib.need(all(meta[k]==v for k,v in c['command_meta'].items()),'command closure')
    pkg=lib.files((a.session/'package/package.tar.gz').read_bytes())
    for p,pin in c['source_files'].items():
        lib.same_hash(pkg[p],pin)
        git=subprocess.check_output(['git','-C',str(a.repo),'show',c['source_git']+':'+p])
        lib.need(pkg[p]==git,'source pin')
    sh=next(p for p in c['helpers'] if p.endswith('/source_check.py'))
    py=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
    subprocess.run(py+[str(a.repo/sh),'--repo',str(a.repo),'--package',str(a.session/'package/package.tar.gz'),'--plan',str(a.session/'package/plan.json'),'--capture',str(HERE/'capture.json')],check=True)
    base='results/cmd/000_mes_b/files/b/'
    report=json.loads(fs[base+'rapport_b.json']);prov=report['provenance']
    lib.need({k:v for k,v in prov.items() if k.endswith('_sha256')}==c['report_provenance'],'report hashes')
    lib.need(prov['pilote_sha256']==c['source_files']['morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py']['sha256'],'pilot hash')
    lib.need(all(re.fullmatch('[0-9a-f]{64}',v) for v in c['report_provenance'].values()),'hash syntax')
    plan=json.loads((a.session/'package/plan.json').read_text());argv=plan['commands'][0]['argv'];args=report['parametres']['argv']
    for k,v in c['configuration'].items():
        for seq in (argv,args):lib.need(seq.count(k)==1 and seq[seq.index(k)+1]==v,'configuration')
    for k in ('--cas','--series'):
        lib.need(argv.count(k)==args.count(k)==1 and argv[argv.index(k)+1]==args[args.index(k)+1],'case configuration')
    lib.need(report['parametres']['schema']=='recouvert','report schema')
    cons=fs[base+'construction.log'].decode()
    for key,values in c['construction_fields'].items():
        lib.need(sorted(set(re.findall(r'(?:-D)?'+key+r'(?::\w+)?=([^\s]+)',cons)))==values,'build flags')
    r=json.loads((a.session/'receipt.json').read_text());cl=c['closure']
    for k,v in cl.items():
        if k not in ('errors_count','done','before_status','after_status'):
            lib.need(type(r[k]) is type(v) and r[k]==v,'closure field')
    lib.need(r['commit']==c['source_git'] and r['commands']==commands,'source/commands')
    lib.need(len(r['errors'])==cl['errors_count']==0 and int((a.session/'DONE').read_text())==cl['done']==0,'errors/DONE')
    lib.need(r['observed_before_stop']['status']==cl['before_status']=='RUNNING' and r['observed_after']['status']==cl['after_status']=='TERMINATED','stop')
    lib.need(r['package_sha256']==c['package_sha256'] and r['plan_sha256']==c['plan_sha256'],'package/plan')
    data=[{k:d[k] for k in ('name','size','sha256')} for d in r['data_files']]
    lib.need(data==c['data_declared'] and r['data_manifest_sha256']==c['data_manifest_sha256']==c['local_files']['package/data/SHA256SUMS']['sha256'],'data declarations')
    oldref=c['L1r_reference'];raw=(a.repo/oldref['capture_path']).read_bytes();lib.same_hash(raw,oldref['capture_pin'])
    lib.need(json.loads(raw)['recovery_and_new_run']['receipt']==oldref['receipt_pin'],'old receipt pin')
    oldraw=(a.previous_session/'receipt.json').read_bytes();lib.same_hash(oldraw,oldref['receipt_pin']);old=json.loads(oldraw)
    lib.need(old['commit']==oldref['source_git'] and old['data_files']==r['data_files'] and oldref['data_declarations_identical'] is True,'L1r input declarations')
    diff=subprocess.check_output(['git','-C',str(a.repo),'diff','--name-only',oldref['source_git'],c['source_git'],'--','morsehgp3D_v12/src/']).decode().splitlines()
    lib.need(diff==c['native_delta_vs_L1r'] and len(diff)==39,'native source delta')
    for p,pin in c['local_files'].items():lib.same_hash((a.session/p).read_bytes(),pin)
    lib.same_hash((a.previous_session/'receipt.json').read_bytes(),oldref['receipt_pin'])
    print('B1p verified: 359 Git sources, 72 archive entries, 45 unchanged L1r input declarations, command0/DONE0 and certified stop; timing admission separate')
if __name__=='__main__':main()
