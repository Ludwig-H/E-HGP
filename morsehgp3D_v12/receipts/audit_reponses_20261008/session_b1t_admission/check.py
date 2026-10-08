#!/usr/bin/env python3
"""MES-B1t : admission metadata/JSON et provenance, sans moteur ni payload."""
from pathlib import Path
import argparse,csv,hashlib,importlib.util,io,json,re,subprocess,sys,tarfile,tempfile,types
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
SOURCE='caf9585e4a89ba101ba86d2d46e77cb340be15bf'
PREFIX='morsehgp3D_v12/'
PUBLISHED='8f03d29bb'
PUBLIC_DIR=PREFIX+'receipts/g4_mesb1t_20261008/'
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(b):return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def load(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def run(a):
 helper_path=a.repo/PREFIX/'receipts/audit_reponses_20261008/session_l1_recuperation/check.py';h=load('archive_helper',helper_path)
 common_path=HERE.parent/'session_t1d_admission/check.py';common=load('t1d_common',common_path)
 reader_path=a.repo/PREFIX/'receipts/audit_reponses_20261008/session_l1r_contrelecture/reader.py'
 s=a.session;r=json.loads((s/'receipt.json').read_text());pre=json.loads((s/'preflight.json').read_text());planraw=(s/'package/plan.json').read_bytes();plan=json.loads(planraw)
 archive=(s/'results/results.tar.gz').read_bytes();fs=h.files(archive);manifest=h.role(fs,'MANIFEST.sha256');h.manifest(fs,82)
 need(r['commit']==SOURCE and r['results_sha256']==pin(archive)['sha256'] and r['results_bytes']==len(archive),'source/results')
 need(r['status']=='completed' and r['worker_exit_code']==0 and r['results_verified'] is True and not r['errors'] and int((s/'DONE').read_text())==0,'outcome')
 need(r['targeted_shutdown_certified'] is True and r['stop_exit_code']==0 and r['observed_before_stop']['status']=='RUNNING' and r['observed_after']['status']=='TERMINATED','certified stop')
 commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));need(commands==r['commands'] and [c['name'] for c in commands]==['mes_b_flux_identite','mes_b_l1'] and all(c['exit_code']=='0' for c in commands),'command closure')
 source,src=common.source(a.repo,s/'package/package.tar.gz',SOURCE);need(source['archive']['sha256']==r['package_sha256'] and pin(planraw)['sha256']==r['plan_sha256'],'package/plan')
 extra=['microbancs/mes_b_scenes/pilote_b.py','microbancs/outils/banc_full.py']
 with tarfile.open(s/'package/package.tar.gz') as t:
  for p in extra:
   b=t.extractfile(PREFIX+p).read();need(b==common.git(a.repo,SOURCE,PREFIX+p),'extra Git source');src[PREFIX+p]=b
 cohort_raw=(Path(pre['data_dir'])/'bundle_manifest.json').read_bytes();cohort=json.loads(cohort_raw);need(len(cohort['cases'])==15,'15 source scenes');sites={c['name']:(c['distinct'] if c.get('bundled')=='distinct' else c)['count'] for c in cohort['cases']}
 declared={d['name']:d for d in r['data_files']}
 need(declared['bundle_manifest.json']['sha256']==pin(cohort_raw)['sha256'],'declared metadata manifest')
 for c in cohort['cases']:
  entry=c['distinct'] if c.get('bundled')=='distinct' else c
  for key,sha in [('coordinates','sha256'),('point_ids','ids_sha256')]:
   d=declared[entry[key]];need(d['sha256']==entry[sha] and d['size']==entry['count']*(12 if key=='coordinates' else 4),'declared scene metadata')
 reasons=dict(re.findall(r'^MHGP12_REASON\((\w+),\s*(\w+),',src[PREFIX+'src/core/reasons.def'].decode(),re.M))
 results={};report_pins={};inventories={};elf={};outcomes={};total_full=0;full_hashes={}
 with tempfile.TemporaryDirectory() as td:
  lf_path=Path(td)/'lecteur_full.py';lf_path.write_bytes(src[PREFIX+'microbancs/outils/lecteur_full.py']);lf=load('b1t_lf',lf_path)
  for index,folder,budget in [(0,'b_identite',8),(1,'b',88)]:
   name=commands[index]['name'];base='results/cmd/%03d_%s/files/%s/'%(index,name,folder)
   report_raw=fs[base+'rapport_b.json'];report=json.loads(report_raw);report_pins[folder]=pin(report_raw)
   raws={Path(n).name:v for n,v in fs.items() if n.startswith(base+'brut/')};need(len(raws)==(4 if index==0 else 34),'raw count')
   for n,v in raws.items():
    need(common.git(a.repo,PUBLISHED,PUBLIC_DIR+'resultats/'+base.removeprefix('results/')+'brut/'+n)==v,'published raw bytes')
   public_report=json.loads(common.git(a.repo,PUBLISHED,PUBLIC_DIR+'resultats/'+base.removeprefix('results/')+'rapport_b.json'))
   need(all(public_report[k]==report[k] for k in ('cas','criteres','verdict','empreintes','provenance')),'published numerical report')
   inv=''.join(pin(v)['sha256']+'  '+n+'\n' for n,v in sorted(raws.items()));inventories[folder]={'files':len(raws),'sha256':pin(inv.encode())['sha256']}
   for n,v in {**{'rapport_b.json':report_raw},**{'brut/'+n:v for n,v in raws.items()}}.items():
    p=a.snapshot/folder/n;p.parent.mkdir(parents=True,exist_ok=True)
    if p.exists():need(p.read_bytes()==v,'snapshot mismatch')
    else:p.write_bytes(v)
   # Port only the declared command index, explicit 8/88 GiB budget, added overlap schema and source pin.
   text=reader_path.read_text();patches=[("plan['commands'][1]['argv']", "plan['commands'][%d]['argv']"%index),
    ("opt['--budget-appareil-gio'] == '88'", "opt['--budget-appareil-gio'] == '%d'"%budget),
    ("same(params['budget_appareil_octets'],88 << 30)","same(params['budget_appareil_octets'],%d << 30)"%budget),
    ("('fils','budget_octets','budget_appareil_octets','delai_global_s','series','argv')", "('fils','budget_octets','budget_appareil_octets','delai_global_s','series','argv','schema')"),
    ('457d0e6ff6f27daca0eb22f5852fe6b1e53fce1611c330fd3931a5b071c8c49d',pin(src[PREFIX+'microbancs/mes_b_scenes/pilote_b.py'])['sha256'])]
   for before,after in patches:need(text.count(before)==1,'port pattern');text=text.replace(before,after)
   reader=types.ModuleType('b1t_envelope');reader.__file__=str(reader_path);exec(compile(text,str(reader_path)+' [B1t port]','exec'),reader.__dict__)
   def expected(case):return dict(voie=case['voie'],k=case['k'],fils=case['fils'],passes=case['expected'],empreinte=case['empreinte'],trames=[(case['etiquette'],case['sites'])],budget_appareil='separe',bits=21,schema='recouvert')
   def full(row,i,case):
    why=lf.check_full(row,i,expected(case));reader.need(not why,why)
    reader.need(16*case['sites']<=row['pic_octets']<=160*(1<<30),'host active budget')
    reader.need(row['epinglee_octets']<=row['pic_octets'],'pinned > host')
    reader.need(row['appareil_octets']<=row['pic_appareil_octets']<=budget*(1<<30),'own device budget')
    reader.need(all(pair[0]>=16*case['sites'] for pair in row['memoire_octets'].values()),'resident input')
    if not case['empreinte']:reader.need(row['hors_mur_ns']['empreinte']==0,'digest outside plan')
   reader.full=full;specs,opt=reader.cohort(reader.decode(planraw),sites);need(len(specs)==(2 if index==0 else 17),'external cases');need(report['parametres']['schema']=='recouvert','overlap schema')
   reviewed=reader.review(reader.decode(report_raw),raws,specs,opt,pin(cohort_raw)['sha256'],reasons);need(reviewed['bruts_admis'],'report/raw mismatch '+str(reviewed['conditions']))
   small=[]
   for entry,case,row in zip(report['cas'],specs,reviewed['cas']):
    need(entry['etat']!='non_joue','all planned cases played');tag='%s_k%d_%s'%(case['nom'],case['k'],case['voie']);need(not raws[tag+'.err'].strip(),'stderr')
    second=lf.parse_output(entry['code'],raws[tag+'.jsonl'].decode('ascii'),expected(case));need(all(reader.same(entry[k],v) for k,v in second.items()),'LF/full envelope discrepancy')
    total_full+=len(row['passes']);last=row['passes'][-1] if row['passes'] else None
    small.append({**{k:row[k] for k in ('nom','sites','k','voie','expected','etat','issue','statistiques')},'code':entry['code'],'pic_nvidia_smi_mio':entry.get('pic_nvidia_smi_mio'),'complete_passes':len(row['passes']), 'etapes_derniere_ns':last['etapes_ns'] if last else None,'c_derniere_ns':last['c_ns'] if last else None,'prefix_before_refusal':row['etat']=='refus' and len(row['passes'])>0})
   for key,hs in reviewed['empreintes'].items():full_hashes.setdefault(key,set()).update(hs)
   elf[folder]=report['provenance']['sonde_sha256'];need(set(['CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21','MHGP12_ENABLE_CUDA:BOOL=ON'])<=set(report['provenance']['cmake']),'build config')
   results[folder]=dict(device_budget_gib=budget,processes=len(specs),full_passes=sum(c['complete_passes'] for c in small),warm_passes=sum(max(0,c['complete_passes']-1) for c in small),criteria=reviewed['criteres'],verdict=reviewed['verdict'],cas=small)
   outcomes[folder]={state:sum(c['etat']==state for c in small) for state in ('ok','refus','echec','non_joue')}
 need(len(set(elf.values()))==1 and all(len(v)==1 for v in full_hashes.values()),'identity between budgets/CPU')
 same_native=subprocess.check_output(['git','-C',str(a.repo),'diff','--name-only','47feedc96',SOURCE,'--',PREFIX+'src',PREFIX+'bench/full_probe.cpp']).decode().splitlines();need(not same_native,'source caf differs from R1 native')
 with tempfile.TemporaryDirectory() as td:
  doc=Path(td)/PUBLIC_DIR/'README.md';doc.parent.mkdir(parents=True);original=common.git(a.repo,PUBLISHED,PUBLIC_DIR+'README.md');doc.write_bytes(original)
  patch=(HERE/'documentation.patch').resolve()
  for opts in [('--check',),(),('--reverse','--check'),('--reverse',)]:subprocess.run(['git','apply',*opts,str(patch)],cwd=td,check=True,capture_output=True)
  need(doc.read_bytes()==original,'documentation patch inverse')
 cap=dict(documentation_patch=pin((HERE/'documentation.patch').read_bytes()),published_commit=subprocess.check_output(['git','-C',str(a.repo),'rev-parse',PUBLISHED]).decode().strip(),published_readme=pin(common.git(a.repo,PUBLISHED,PUBLIC_DIR+'README.md')),source_git=SOURCE,source=source,closure={k:r[k] for k in ['status','worker_exit_code','results_verified','targeted_shutdown_certified','stop_exit_code','stop_attempts','reserve_released','guest_guard_intact']},errors_count=0,done=0,stop_before='RUNNING',stop_after='TERMINATED',commands=commands,files={p:pin((s/p).read_bytes()) for p in ['receipt.json','DONE','preflight.json','package/plan.json','package/package.tar.gz','results/results.tar.gz']},manifest=pin(manifest),manifest_entries=82,critical_sources={p:pin(src[PREFIX+p]) for p in ['bench/full_probe.cpp','microbancs/mes_b_scenes/pilote_b.py','microbancs/outils/lecteur_full.py','microbancs/outils/banc_full.py','src/core/reasons.def']},helpers={'archive_helper':pin(helper_path.read_bytes()),'source_helper':pin(common_path.read_bytes()),'l1r_envelope':pin(reader_path.read_bytes())},metadata_manifest=pin(cohort_raw),cohort=[{'name':c['name'],'sites':sites[c['name']],'original_points':c['count'],'bundled':c.get('bundled')} for c in cohort['cases']],data_declared=[{k:d[k] for k in ['name','size','sha256']} for d in r['data_files']],reports=report_pins,raw_inventories=inventories,ELF_initial=elf,ELF_final_archived=False,source_equal_native_R1=True)
 output=dict(total_processes=19,total_full_passes=total_full,total_warm_passes=sum(x['warm_passes'] for x in results.values()),outcomes=outcomes,reports=results,identities={k:sorted(v) for k,v in full_hashes.items()},all_identities_equal=True)
 return cap,output

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--session',type=Path,required=True);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--write',action='store_true');a=p.parse_args();a.repo=a.repo.resolve();a.session=a.session.resolve();a.snapshot=a.snapshot.resolve();cap,res=run(a)
 for name,obj in [('capture.json',cap),('results.json',res)]:
  f=HERE/name
  if a.write:f.write_text(json.dumps(obj,ensure_ascii=False,separators=(',',':'))+'\n')
  else:need(obj==json.loads(f.read_text()),name+' changed')
 print(json.dumps({k:res[k] for k in ['total_processes','total_full_passes','outcomes','all_identities_equal']}))
if __name__=='__main__':main()
