#!/usr/bin/env python3
"""FULLN: public metadata/source/log review only, never a native or cloud invocation."""
from pathlib import Path
import argparse,collections,copy,csv,hashlib,importlib.util,io,json,re,statistics,subprocess,sys,tarfile,tempfile,types
sys.dont_write_bytecode=True
PREFIX='morsehgp3D_v12/'
GIT='8a0716e7470197c95953b38d79f26b8d8f2379fc'
BASE='receipts/audit_reponses_20261008/mes_c_contrelecture/'
def need(x,w):
 if not x:raise ValueError(w)
def sha(b):return hashlib.sha256(b).hexdigest()
def pin(b):return dict(bytes=len(b),sha256=sha(b))
def git(repo,*args):return subprocess.check_output(['git','-C',str(repo),*args])
def source(repo,name):return git(repo,'show',GIT+':'+PREFIX+name)
def files(raw,selected=lambda p:True):
 with tarfile.open(fileobj=io.BytesIO(raw))as tar:
  out={};seen=set()
  for m in tar:
   p=Path(m.name);need(not p.is_absolute()and'..'not in p.parts and not m.issym()and not m.islnk(),'unsafe archive member')
   need(m.name not in seen,'duplicate archive member');seen.add(m.name)
   if m.isfile()and selected(m.name):out[m.name]=tar.extractfile(m).read()
  return out
def load(name,p):
 sp=importlib.util.spec_from_file_location(name,p);m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m);return m

def run(repo,session):
 receipt=json.loads((session/'receipt.json').read_bytes());pre=json.loads((session/'preflight.json').read_bytes());planraw=(session/'package/plan.json').read_bytes();plan=json.loads(planraw)
 need(receipt['commit']==pre['commit']==GIT,'source commit')
 closure_keys=['status','worker_exit_code','worker_outcome','results_verified','targeted_shutdown_certified','stop_exit_code','closure','retrieval','reserve_released','private_key_deleted','oslogin_key_removed','guest_guard_intact']
 closure={k:receipt[k]for k in closure_keys};need(closure==dict(status='completed',worker_exit_code=0,worker_outcome='exited',results_verified=True,targeted_shutdown_certified=True,stop_exit_code=0,closure='stopped',retrieval='downloaded',reserve_released=True,private_key_deleted=True,oslogin_key_removed=True,guest_guard_intact=True),'closure fields')
 need(not receipt['errors']and not receipt['warnings']and(session/'DONE').read_text().strip()=='0','closure error/DONE')
 need(receipt['observed_before_stop']['status']=='RUNNING'and receipt['observed_after']['status']=='TERMINATED','certified stop')
 closure.update(before='RUNNING',after='TERMINATED',errors=0,warnings=0,done=0)
 archive=(session/'results/results.tar.gz').read_bytes();need(sha(archive)==receipt['results_sha256']and len(archive)==receipt['results_bytes'],'result archive')
 fs=files(archive);manifest=fs['results/MANIFEST.sha256'];covered=set()
 for line in manifest.decode().splitlines():
  h,n=line.split(None,1);n='results/'+n.strip().removeprefix('./');need(n not in covered and sha(fs[n])==h,'manifest');covered.add(n)
 need(len(covered)==receipt['results_manifest_files']==156 and set(fs)-covered=={'results/MANIFEST.sha256'},'manifest closure')
 need(sum(map(len,fs.values()))==receipt['results_expanded_bytes'],'expanded archive')
 commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));need(commands==receipt['commands'],'commands closure')
 need([c['name']for c in commands]==['socle_ctest','mes_full','mes_c']and all(c['exit_code']=='0'and c['status']=='ok'for c in commands),'command outcomes')
 for i,c in enumerate(commands):need(fs['results/cmd/%03d_%s/stderr'%(i,c['name'])]==b'','command stderr')
 allowed={'--fils','--processus','--passes','--jobs','--delai','--k','--voies','--tours','--tours-difficiles','--fils-difficiles','--delai-global','--delai-cas'}
 opts=[]
 for c in plan['commands']:
  args=c['argv'];opts.append(dict(name=c['name'],timeout_seconds=c['timeout_seconds'],options={x:args[i+1]for i,x in enumerate(args[:-1])if x in allowed}));need(not any(x in args for x in ['--essai','--sequentiel','--cache','--budget-gio']),'default regime')
 package=(session/'package/package.tar.gz').read_bytes();need(sha(package)==receipt['package_sha256']==pre['package_sha256']and sha(planraw)==receipt['plan_sha256']==pre['plan_sha256'],'source/plan hashes')
 scopes=[PREFIX+x for x in ['src','tests','bench','microbancs','cmake','CMakeLists.txt']]+['gcp-migration/v12_worker.sh']
 selected=lambda p:any(p==x or p.startswith(x+'/')for x in scopes)
 packed=files(package,selected);gs=files(git(repo,'archive',GIT,*scopes));need(packed==gs and len(gs)==488,'488 source files exact')
 inv=''.join(sha(b)+'  '+n+'\n'for n,b in sorted(gs.items()));need(sha(inv.encode())=='a7044e68015f8c827d3f73a5d8feb6f562d56cbfbc9f4c96f3347516ecb0998c','runtime inventory')
 # Public cohort metadata, never the data archives or coordinate/ID payloads.
 doc=source(repo,'docs/DONNEES.md').decode();v12={}
 for line in doc.splitlines():
  if line.startswith('| `kitti_ng_'):
   c=[x.strip()for x in line.split('|')[1:-1]];v12[c[0].strip('`')]=int(c[3].replace(' ',''))
 need(len(v12)==37,'public 37 cohort');ng=dict(ng00=39885,ng01=35551,ng02=45845)
 deps={n:source(repo,n)for n in ['microbancs/mes_full/pilote_full.py','microbancs/outils/lecteur_full.py','microbancs/outils/banc_full.py']}
 with tempfile.TemporaryDirectory(prefix='fulln-readers-')as td:
  for n,b in deps.items():p=Path(td)/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b)
  F=load('fulln_pilot',Path(td)/'microbancs/mes_full/pilote_full.py');LF=F.lf
  fullroot='results/cmd/001_mes_full/files/full/';report=LF.json.loads(fs[fullroot+'rapport_full.json'],object_pairs_hook=LF.unique_object,parse_constant=LF.reject_constant)
  need(report['mesure']=='MES-FULL'and report['refus']==[],'FULL reported refusals')
  need({k:report['options'][k]for k in ['essai','fils','passes','processus','sequentiel']}==dict(essai=False,fils=48,passes=10,processus=5,sequentiel=False),'FULL configuration')
  for when in ['avant','apres']:
   env=report['environnement_'+when];need(env['gpu_apps']==''and all(type(env[k])is str and env[k]for k in ['nvcc','cmake','gpu']),'FULL environment')
  need(report['provenance']['pilote_sha256']==sha(deps['microbancs/mes_full/pilote_full.py'])and report['provenance']['lecteur_sha256']==sha(deps['microbancs/outils/lecteur_full.py']),'FULL reader provenance')
  raw_names={Path(n).name for n in fs if n.startswith(fullroot+'brut/')};wanted=set();groups={};processed=0;total=0
  for tag,key,k,np,passes,device in [('k5','k5_appareil',5,5,10,True),('k10','k10_appareil',10,3,5,True),('cpu','k5_cpu',5,3,5,False)]:
   groups[key]={n:[]for n in ng}
   for n,sites in ng.items():
    for rep in range(np):
     name='%s_%s_r%d.jsonl'%(tag,n,rep);wanted.add(name)
     # 0 is CONDITIONAL inference from the pinned producer + empty refus, not an archived process code.
     rows,why=F.parse_process(0,fs[fullroot+'brut/'+name].decode(),passes,k,device,48,[(n,sites)]);need(not why,'FULL structural replay '+name)
     groups[key][n].append(rows);processed+=1;total+=len(rows)
  groups['v12set_k5_appareil']={n:[]for n in v12};names=sorted(v12)
  for rep in range(5):
   name='v12set_r%d.jsonl'%rep;wanted.add(name);order=names[rep:]+names[:rep]
   rows,why=F.parse_process(0,fs[fullroot+'brut/'+name].decode(),74,5,True,48,[(n,v12[n])for n in order]);need(not why,'37 structural replay')
   for i,n in enumerate(order):groups['v12set_k5_appareil'][n].append([rows[i],rows[i+37]])
   processed+=1;total+=len(rows)
  need(raw_names==wanted and processed==38 and total==610,'FULL process cohort')
  stats={k:{n:F.frame_stats(runs,1)for n,runs in g.items()}for k,g in groups.items()};need(stats==report['statistiques'],'FULL all published statistics')
  refus=[];hashes=dict(k5=F.digests([groups['k5_appareil'],groups['k5_cpu']],refus,'K5'),k10=F.digests([groups['k10_appareil']],refus,'K10'),v12set=F.digests([groups['v12set_k5_appareil']],refus,'v12set'))
  need(not refus and hashes==report['empreintes'],'FULL digests CPU/GPU/repetitions')
  contract=dict(ng00_02=F.contract(stats['k5_appareil']),v12set=F.contract(stats['v12set_k5_appareil']));need(contract==report['contrat']and report['verdict']=='non tenu'and contract['ng00_02']['tenu']and not contract['v12set']['tenu'],'FULL contract replay')
  absolute={}
  for key,g in groups.items():
   absolute[key]={}
   for n,runs in g.items():
    warm=[r['wall_ns']for run in runs for r in run[1:]];meds=[statistics.median(r['wall_ns']for r in run[1:])for run in runs]
    absolute[key][n]=dict(sites=runs[0][0]['sites'],processes=len(runs),warm=len(warm),median_pooled_ns=statistics.median(warm),median_process_medians_ns=statistics.median(meds),max_process_median_ns=max(meds),max_raw_warm_ns=max(warm),over_100ms=sum(v>100000000 for v in warm))
  large=absolute['v12set_k5_appareil'];large_summary=dict(frames=37,warm=185,median_frame_medians_ns=statistics.median(x['median_pooled_ns']for x in large.values()),max_frame_median_ns=max(x['median_pooled_ns']for x in large.values()),max_raw_ns=max(x['max_raw_warm_ns']for x in large.values()),frames_under_100ms=sum(x['median_pooled_ns']<100000000 for x in large.values()),frames_all_warm_under_100ms=sum(x['max_raw_warm_ns']<100000000 for x in large.values()))
  # Explicit reuse of the independent MES-C cohort/OLS reader, pinned in Git.
  cb=source(repo,BASE+'capture.json');rb=source(repo,BASE+'reader.py');C=types.ModuleType('fulln_c_reader');exec(compile(rb,'frozen_mes_c_reader','exec'),C.__dict__);cap=C.loads(cb.decode());cap.update(commit=GIT,sources={n:sha(source(repo,n))for n in cap['sources']})
  cap['configuration'].update(threads=[4,48]);args=plan['commands'][2]['argv'][2:];cap['command_options']={k:None if'{'in v else v for k,v in zip(args[::2],args[1::2])}
  oldexpected=C.expected;C.expected=lambda spec,capture:dict(oldexpected(spec,capture),schema='recouvert');C.STAGES=('P','C','tour');_,reasons=C.load_sources(repo,cap)
  croot='results/cmd/002_mes_c/files/c/';cr=C.loads(fs[croot+'rapport_c.json'].decode());raws={Path(n).name:b.decode('ascii')for n,b in fs.items()if n.startswith(croot+'brut/')};need(cr['parametres']['schema']=='recouvert','MES-C schema')
  admitted=C.review(cr,raws,cap,LF,reasons);need(not admitted['differences']and not admitted['controles']and admitted['cohorte_complete'],'MES-C reader mismatch')
  ctable={}
  for k,v in admitted['configurations'].items():
   vals=v['valeurs'];ctable[k]=dict(droites=v['droites'],groups={})
   for group in ['all','reel','reel_le150']:
    chosen=[v for v in vals.values()if group=='all'or v['groupe']=='reel'and(group!='reel_le150'or v['sites']<=150)]
    ctable[k]['groups'][group]=dict(clouds=len(chosen),median_cloud_warm_ns=statistics.median(v['chaud_ns']for v in chosen),max_cloud_warm_ns=max(v['chaud_ns']for v in chosen))
  data={x['name']:x for x in receipt['data_files']};need(receipt['data_verified_remote']is True and receipt['data_manifest_sha256']==pre['data_manifest_sha256'],'remote input metadata verification')
  need(data[cap['data_archive']['name']]['sha256']==cap['data_archive']['sha256']and data[cap['data_archive']['name']]['size']==cap['data_archive']['size'],'small archive declared link')
  for n,sites in ng.items():
   need(data['lidar_'+n+'.u32le']['size']==12*sites and data['lidar_'+n+'.ids.u32le']['size']==4*sites,'ng declared byte counts')
  cresult=dict(verdict=admitted['verdict'],criteria=admitted['criteres'],processes=admitted['processus_joues'],full_passes=admitted['passes_completes'],warm_passes=admitted['passes_chaudes'],states=dict(collections.Counter(admitted['etats'].values())),configuration_statistics=ctable,refused_cases=[{k:r[k]for k in ['nom','sites','voie','k','etat','raison']}for r in cr['difficiles']if r['etat']!='ok'],codes=admitted['codes'],cohort_metadata=pin(cb),cohort_reader=pin(rb),metadata_archive_link=cap['metadata_archive_link'])
  buildprofile={'CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21','MHGP12_ENABLE_CUDA:BOOL=ON'}
  need(all(buildprofile<=set(r['provenance']['cmake'])for r in [report,cr]),'build profiles')
  elf={k:r['provenance']['sonde_sha256']for k,r in [('full_pre_campaign',report),('mes_c_pre_campaign',cr)]};binarylines=fs['results/provenance/binaries.sha256'].decode().splitlines();digests={line.split()[0]for line in binarylines};need(not any(h in digests for h in elf.values()),'ELF closure status changed')
  socle=fs['results/cmd/000_socle_ctest/stdout'].decode();m=re.search(r'100% tests passed, 0 tests failed out of (\d+)',socle);need(m is not None,'socle summary')
  test_lines=[x for x in socle.splitlines()if re.search(r'Test\s+#\d+:',x)];need(len(test_lines)==747 and all('Passed'in x for x in test_lines)and '***Skipped'not in socle and 'Not Run'not in socle,'socle result lines')
  return dict(source_git=GIT,source_files=488,source_inventory_sha256=sha(inv.encode()),archive=pin(archive),manifest_files=156,package=pin(package),plan=pin(planraw),commands=commands,public_plan=opts,closure=closure,socle_passed=int(m.group(1)),socle_skipped_reported=0,input_metadata_verified_remote=True,small_archive_payload_link_newly_verified=False,elf=elf,final_pilot_elf_independently_closed=False,process_codes_stderr_independently_archived=False,evidence_scope='conditional replay: process codes inferred from pinned producers and reports; no raw native codes/stderr or final pilot ELF closure',full=dict(verdict=report['verdict'],contract=contract,processes=processed,passes=total,warm_passes=sum(x['warm']for g in absolute.values()for x in g.values()),absolute=absolute,large_summary=large_summary,identity_sha256=sha(json.dumps(hashes,sort_keys=True).encode())),mes_c=cresult,native_executed=False,payload_read=False)

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',required=True,type=Path);p.add_argument('--session',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args();r=run(a.repo,a.session);a.out.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(json.dumps({k:r[k]for k in ['source_git','socle_passed','final_pilot_elf_independently_closed','process_codes_stderr_independently_archived']}));print(json.dumps({'full':r['full']['large_summary'],'mes_c':{k:r['mes_c'][k]for k in ['verdict','criteria','processes','full_passes','warm_passes','states']}}))
