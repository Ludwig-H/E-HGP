#!/usr/bin/env python3
"""B3 primaries and statistics, sources pinned; no native/GCP/payload execution."""
from pathlib import Path
import collections,copy,csv,hashlib,importlib.util,io,json,math,re,statistics,subprocess,sys,tarfile,tempfile
sys.dont_write_bytecode=True
PREFIX='morsehgp3D_v12/'
ROOT='results/cmd/001_t2d_b3_pilote/files/t2d_b3/'
SOURCE='545ed987e0f5a06dbb518fe42ed6c5f100f32770'
PATCH_COMMIT='0e16aaa3a'
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def pin(b):return dict(bytes=len(b),sha256=sha(b))
def load(name,p):
 s=importlib.util.spec_from_file_location(name,p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def files(raw):
 with tarfile.open(fileobj=io.BytesIO(raw))as t:
  out={}
  for m in t:
   p=Path(m.name);need(not p.is_absolute()and'..'not in p.parts and not m.issym()and not m.islnk(),'unsafe archive member')
   if m.isfile():need(m.name not in out,'duplicate member');out[m.name]=t.extractfile(m).read()
  return out
def read(repo,session):
 receipt=json.loads((session/'receipt.json').read_bytes());archive=(session/'results/results.tar.gz').read_bytes();need(pin(archive)==dict(bytes=receipt['results_bytes'],sha256=receipt['results_sha256']),'results archive')
 fs=files(archive);covered=set()
 for line in fs['results/MANIFEST.sha256'].decode().splitlines():
  h,n=line.split(None,1);n='results/'+n.strip().removeprefix('./');need(n not in covered and sha(fs[n])==h,'manifest entry');covered.add(n)
 need(len(covered)==receipt['results_manifest_files']==457 and set(fs)-covered=={'results/MANIFEST.sha256'},'manifest closure')
 need(sum(map(len,fs.values()))==receipt['results_expanded_bytes'],'expanded archive')
 commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));need(commands==receipt['commands'],'command closure')
 need([c['name']for c in commands]==['socle_ctest','t2d_b3_pilote','mutants_catalogue_tour']and all(c['exit_code']=='0'and c['status']=='ok'for c in commands),'command outcomes')
 for i,c in enumerate(commands):need(fs['results/cmd/%03d_%s/stderr'%(i,c['name'])]==b'','command stderr')
 close={k:receipt[k]for k in ['status','worker_exit_code','worker_outcome','results_verified','targeted_shutdown_certified','stop_exit_code','closure','retrieval']}
 need(close==dict(status='completed',worker_exit_code=0,worker_outcome='exited',results_verified=True,targeted_shutdown_certified=True,stop_exit_code=0,closure='stopped',retrieval='downloaded'),'session closure')
 need(receipt['commit']==SOURCE and not receipt['errors']and not receipt['warnings']and(session/'DONE').read_text().strip()=='0','session error/source')
 need(receipt['observed_before_stop']['status']=='RUNNING'and receipt['observed_after']['status']=='TERMINATED','certified shutdown');close.update(before='RUNNING',after='TERMINATED',errors=0,warnings=0,done=0)
 depnames=['microbancs/mes_t2d_b3/pilote_t2d_b3.py','microbancs/mes_t2d_b3/bras_t2d_b3.json','microbancs/outils/lecteur_full.py','microbancs/outils/banc_full.py','microbancs/mes_g_profil/profil_g.py']
 deps={n:subprocess.check_output(['git','-C',str(repo),'show',SOURCE+':'+PREFIX+n])for n in depnames}
 patch_path=PREFIX+'receipts/audit_reponses_20261008/b3_identite_proposition/proposition.patch';patch=subprocess.check_output(['git','-C',str(repo),'show',PATCH_COMMIT+':'+patch_path]);need(sha(patch)=='270a46a3ac326bbea46247f93997d1ff1b823c395ddf0d238d277692bc205956','strict resolution reader patch')
 with tempfile.TemporaryDirectory(prefix='b3-primary-review-')as td:
  work=Path(td);original=work/'original';proposed=work/'proposed';returned=work/'returned'
  for root in [original,proposed]:
   for n,b in deps.items():p=root/PREFIX/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b)
  subprocess.run(['git','apply','--check','-'],input=patch,cwd=proposed,check=True,capture_output=True);subprocess.run(['git','apply','-'],input=patch,cwd=proposed,check=True,capture_output=True)
  ppath='microbancs/mes_t2d_b3/pilote_t2d_b3.py';need(sha((proposed/PREFIX/ppath).read_bytes())=='d9910490e3fb48afc59cbb842ce6e33f040e809af67de1135f277a4b5f75c7b2','strict reader target')
  P=load('b3_original_admission',original/PREFIX/ppath);S=load('b3_strict_resolution',proposed/PREFIX/ppath);LF=P.lf
  r=json.loads(fs[ROOT+'rapport_t2d_b3.json'],object_pairs_hook=LF.unique_object,parse_constant=LF.reject_constant)
  need(r['regle']==json.loads(json.dumps(P.REGLE_T2D_B3)) and r['schema']=='ehgp.v12.t2d_b3_pilote.v1','rule/original schema')
  expected=dict(ng00=39885,ng01=35551,ng02=45845,kitti_ng_02_001606=64740,kitti_ng_08_001176=67114,kitti_ng_08_002119=99099)
  need(set(r['trames'])==set(expected)and all(type(r['trames'][n]['sites'])is int and r['trames'][n]['sites']==v for n,v in expected.items()),'external frame metadata')
  need(set(r['identite']['trames'])==set(P.TRAMES)and set(r['resolution']['trames'])==set(P.RES_TRAMES),'identity frame cohort')
  visited=set();rows={};counts=collections.Counter();resolution_hashes={};identity_hashes={}
  def raw(p,wanted,code_required=True,hash_required=True):
   need(type(p)is dict and p.get('valide')is True and p.get('journal')==wanted and wanted not in visited,'proof place/unique/validity')
   if code_required:need(type(p.get('code'))is int and p['code']==0,'strict native code')
   b=fs[ROOT+wanted]
   if hash_required:need(sha(b)==p.get('journal_sha256'),'native journal hash')
   visited.add(wanted);out=returned/wanted;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(b);return b.decode()
  def full(p,wanted,t,k,passes,digest=False,schema='recouvert'):
   text=raw(p,wanted);state=LF.parse_output(p['code'],text,P.attendu(t,expected[t],k,48,passes,digest,schema));need(state['etat']=='ok','strict FULL primary '+wanted);rr=state['passes'];rows[wanted]=rr;counts['FULL_processes']+=1;counts['FULL_passes']+=len(rr)
   if schema=='recouvert':summary=P.resume(rr)
   else:summary=dict(murs_ns=[x['wall_ns']for x in rr],g_ns=[x['etapes_ns']['G']for x in rr],tables_ns=[x['g_ns']['tables']for x in rr],resolution_ns=[x['g_ns']['resolution']for x in rr]);summary['g_median_ns']=statistics.median(summary['g_ns'][1:])
   need(all(p.get(key)==value for key,value in summary.items())and p['mur_ns']==statistics.median(summary['murs_ns'][1:]),'summary versus FULL primary');p.update(summary);return rr
  for t in P.TRAMES:
   group=r['identite']['trames'][t];need(set(group)==set(P.BRAS_CONSTRUITS),'identity arm cohort');hashes=set()
   for arm,p in group.items():
    rr=full(p,'journaux/identite/%s/%s.jsonl'%(t,arm),t,5,2,True);hashes.update(x['full_sha256']for x in rr)
   need(len(hashes)==1,'FULL identity divergence');identity_hashes[t]=next(iter(hashes))
  need(identity_hashes['ng00']==P.FUL1_NG00_K5,'ng00 reference FUL1')
  for t in P.RES_TRAMES:
   group=r['resolution']['trames'][t];need(set(group)==set(P.BRAS_CONSTRUITS),'resolution arm cohort');hashes=set()
   for arm,p in group.items():
    text=raw(p,'journaux/resolution/%s/%s.jsonl'%(t,arm),False)
    need('code'not in p and 'stderr_sha256'not in p,'resolution evidence limitation changed')
    # Conditional inference from pinned producer's valide=True, NOT an independently archived return code.
    state=S.lire_resolution(0,text,expected[t],48);need(state['valide']and state['resolution']==p['resolution'],'strict G schema and digest');hashes.add(state['resolution']);counts['G_resolution_processes']+=1;counts['G_resolution_passes']+=1
   need(len(hashes)==1,'resolution identity divergence');resolution_hashes[t]=next(iter(hashes))
  camp=r['campagne_k5'];need((camp['fils'],camp['passes'],camp['tours_demandes'])==(48,8,10)and set(camp['trames'])==set(P.TRAMES),'decisive configuration')
  for t,tours in camp['trames'].items():
   need(len(tours)==10,'ten paired rounds')
   for index,tour in enumerate(tours):
    need(set(tour)==set(P.BRAS_JUGES),'six paired arms')
    for arm,p in tour.items():full(p,'journaux/k5/%s/%s_t%02d.jsonl'%(t,arm,index),t,5,8)
  decisive_judgment=P.juger(r,str(returned),True)
  float_differences=[]
  def compare(a,b,path=''):
   if type(a)is dict and type(b)is dict:
    need(set(a)==set(b),'comparison keys '+path)
    for key in a:compare(a[key],b[key],path+'/'+key)
   elif type(a)is list and type(b)is list:
    need(len(a)==len(b),'comparison array '+path)
    for i,(x,y)in enumerate(zip(a,b)):compare(x,y,path+'/'+str(i))
   elif a!=b:
    need(type(a)is float and type(b)is float and math.isfinite(a)and math.isfinite(b)and abs(a-b)<=max(math.ulp(a),math.ulp(b)),'comparison value '+path)
    float_differences.append(dict(path=path,replay=a,published=b,tolerance='one ULP'))
  compare(json.loads(json.dumps(decisive_judgment)),r['jugement'],'judgment')
  info=r['informations'];need(len(info['grande'])==3,'largest informative rounds')
  for index,tour in enumerate(info['grande']):
   need(set(tour)==set(P.BRAS_JUGES),'largest informative arms')
   for arm,p in tour.items():full(p,'journaux/grande/%s/%s_t%02d.jsonl'%(P.GRANDE,arm,index),P.GRANDE,5,8)
  need(set(info['k10'])=={'ng00','ng01','ng02'},'K10 informative frames')
  for t,group in info['k10'].items():
   need(set(group)=={'avant','apres'},'K10 arms')
   for arm,p in group.items():full(p,'journaux/k10/%s/%s.jsonl'%(t,arm),t,10,5)
  need(set(info['sequentiel'])==set(P.SEQ_TRAMES),'sequential frames')
  for t,tours in info['sequentiel'].items():
   need(len(tours)==3,'sequential rounds')
   for index,tour in enumerate(tours):
    need(set(tour)==set(P.SEQ_BRAS),'sequential arms')
    for arm,p in tour.items():full(p,'journaux/sequentiel/%s/%s_t%02d.jsonl'%(t,arm,index),t,5,8,False,'sequentiel')
  need(set(info['profil'])==set(P.PROFIL_TRAMES),'profiling frames')
  for t,group in info['profil'].items():
   need(set(group)==set(P.PROFILES),'profiling arms')
   for arm,p in group.items():
    need(type(p['code'])is int and p['code']==0,'profile code');wanted='journaux/profil/%s/%s.jsonl'%(t,arm);need(p['journal']==wanted and wanted not in visited,'profile place');visited.add(wanted);text=fs[ROOT+wanted].decode();parsed=P.pg.lire(text);need(parsed is not None and json.loads(json.dumps(P.pg.resumer(*parsed)))==p['resume'],'profile raw versus summary');counts['G_profile_processes']+=1;counts['G_profile_passes']+=4
  raw_names={n[len(ROOT):]for n in fs if n.startswith(ROOT+'journaux/')};need(raw_names==visited and len(visited)==387,'387 unique native journals, no omitted or extra file')
  need(dict(counts)==dict(FULL_processes=373,FULL_passes=2816,G_resolution_processes=10,G_resolution_passes=10,G_profile_processes=4,G_profile_passes=16),'complete process/pass cohort')
  compare(json.loads(json.dumps(P.resume_informations(r))),r['resume_informations'],'informative_summary')
  cons=r['construction'];need(cons['bras_sha256']==sha(deps['microbancs/mes_t2d_b3/bras_t2d_b3.json'])and cons['archive_avant_sha256']=='34c1ea7ccc378389ad9e568bd60e2c48492bbac10e8f71354408acce9013b5b2','built arm sources')
  full_hashes={k:v['sha256']for k,v in cons['binaires'].items()};need(set(full_hashes)==set(P.BRAS_JUGES)and full_hashes==camp['binaires_apres']and full_hashes['avant']==full_hashes['avant_bis'],'FULL ELF before/after decisive campaign')
  need(all(P.bf.environment_ok(r['environnement'][when])for when in ['avant','apres'])and P.bf.environment_ok(info['environnement_fin']),'empty GPU environment')
  def absolute(tours,arms):
   result={}
   for arm in arms:
    allrows=[rows[tour[arm]['journal']]for tour in tours];warm=[[p['wall_ns']for p in rr[1:]]for rr in allrows];med=[statistics.median(w)for w in warm];flat=sum(warm,[]);result[arm]=dict(processes=len(tours),warm_passes=len(flat),median_process_medians_ns=statistics.median(med),median_pooled_ns=statistics.median(flat),max_process_median_ns=max(med),max_raw_warm_ns=max(flat))
   return result
  abs_ng={t:absolute(tours,P.BRAS_JUGES)for t,tours in camp['trames'].items()};large=absolute(info['grande'],P.BRAS_JUGES);k10={t:absolute([group],('avant','apres'))for t,group in info['k10'].items()}
  stats={t:{key:value for key,value in case.items()if key not in ['mur_ms_median']}for t,case in decisive_judgment['cas'].items()}
  # Independent strict inequality decision; no outlier removed, no threshold changed.
  failing={name:[t for t in P.TRAMES if stats[t][name]['ic95'][1]>=1]for name,_,_ in P.LEVIERS};need(all(failing.values())and decisive_judgment['controle_aa']['dans_la_fenetre']and not decisive_judgment['refus']and set(decisive_judgment['verdicts'].values())=={'rejete'},'independent rejection conditions')
  socle=fs['results/cmd/000_socle_ctest/stdout'].decode();mutants=fs['results/cmd/002_mutants_catalogue_tour/stdout'].decode();socle_count=re.search(r'100% tests passed, 0 tests failed out of (\d+)',socle);mutant_count=re.search(r'100% tests passed, 0 tests failed out of (\d+)',mutants);need(socle_count and mutant_count and int(socle_count[1])==747 and int(mutant_count[1])==2,'CTest summaries')
  need(len(re.findall(r'\bPassed\b',socle))==747 and '***Skipped'not in socle,'747 socle tests actually passed')
  mutant_sources={n:subprocess.check_output(['git','-C',str(repo),'show',SOURCE+':'+PREFIX+n])for n in ['CMakeLists.txt','tests/mutants/run_mutants.py','tests/mutants/catalogue.json','tests/mutants/tower.json']}
  mutant_sizes={n:len(json.loads(mutant_sources['tests/mutants/'+n+'.json'])['mutants'])for n in ['catalogue','tower']}
  need(mutant_sizes==dict(catalogue=38,tower=57),'mutant source inventory')
  need(not any(n.endswith('/mutants_catalogue.json')or n.endswith('/mutants_tower.json')for n in fs),'individual mutant reports availability changed')
  need('-DMHGP12_ENABLE_CUDA'not in mutant_sources['CMakeLists.txt'].decode().split('set(mhgp12_mutant_profile',1)[1].split('foreach(unit',1)[0],'CUDA forwarded to mutants changed')
  return dict(source_git=SOURCE,pilot_sha256=sha(deps[depnames[0]]),before_git='8a0716e74',archive=pin(archive),manifest_files=457,closure=close,commands=commands,socle_ctests=int(socle_count[1]),socle_skipped=0,mutants_ctests=2,mutants_manifest_sizes=mutant_sizes,mutants_source_pins={n:pin(b)for n,b in mutant_sources.items()},mutants_cuda_enabled=False,individual_mutants_causes_replayed=False,cohort=dict(counts),native_journals=387,decisive_processes=300,decisive_full_passes=2400,decisive_warm_passes=2100,full_native_codes_recorded=373,profil_native_codes_recorded=4,resolution_codes_independently_archived=False,native_stderr_individually_archived=False,identity_full=identity_hashes,identity_resolution_conditional=resolution_hashes,strict_G_reader_patch_sha256=sha(patch),worker_float_differences=float_differences,verdicts=decisive_judgment['verdicts'],failing_frames=failing,AA=decisive_judgment['controle_aa'],statistics=stats,absolute_decisive=abs_ng,informative_largest=large,informative_k10=k10,informative_sequential=r['resume_informations']['sequentiel'],informative_profile=r['resume_informations']['profil'],full_elf_before_and_after_decisive=full_hashes,G_elf_pre_campaign={k:v['sha256']for k,v in cons['sondes_g'].items()},profile_elf_pre_campaign={k:v['sha256']for k,v in cons['sondes_profil'].items()},strict_evidence_admission_complete=False,evidence_limit='resolution code inferred conditionally; per-native stderr absent; G/profile ELF hashes before only; informative FULL ELF not reclosed after informative runs',native_executed=False,payload_read=False)
