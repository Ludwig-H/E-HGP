#!/usr/bin/env python3
"""Local archive, Git and exact arm reconstruction; no controller or native call."""
from pathlib import Path
import json,hashlib,subprocess,importlib.util,sys,csv,io,tarfile,re,datetime
sys.dont_write_bytecode=True

def run(REPO,S,HERE):
 pin=lambda b:{'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
 helper='morsehgp3D_v12/receipts/audit_reponses_20261008/session_l1_recuperation/check.py'
 spec=importlib.util.spec_from_file_location('archive_helper',REPO/helper);lib=importlib.util.module_from_spec(spec);spec.loader.exec_module(lib)
 need=lib.need
 r=json.loads((S/'receipt.json').read_text());pre=json.loads((S/'preflight.json').read_text());planraw=(S/'package/plan.json').read_bytes();plan=json.loads(planraw)
 archive=(S/'results/results.tar.gz').read_bytes();fs=lib.files(archive);manifest=lib.role(fs,'MANIFEST.sha256');count=len(manifest.decode().splitlines());lib.manifest(fs,count)
 commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));need(r['commands']==commands,'commands vs receipt')
 need(r['commit']=='4171b2653fa5cb903102a3fc44572bcb3330b3e8','source pin')
 need(r['results_sha256']==pin(archive)['sha256'] and r['results_bytes']==len(archive),'archive receipt')
 need(r['status']=='completed' and r['worker_exit_code']==0 and r['results_verified'] is True,'outcome')
 need(r['targeted_shutdown_certified'] is True and r['stop_exit_code']==0 and not r['errors'] and int((S/'DONE').read_text())==0,'closure')
 need(r['observed_before_stop']['status']=='RUNNING' and r['observed_after']['status']=='TERMINATED','stop')
 base='results/cmd/001_t2d_b2_pilote/files/t2d_b2/';published=REPO/'morsehgp3D_v12/receipts/g4_t2db2_20261008/resultats/cmd/001_t2d_b2_pilote/files/t2d_b2'
 selected={p:v for p,v in fs.items() if p.startswith(base)}
 need(all((published/p[len(base):]).read_bytes()==raw for p,raw in selected.items() if p.endswith('.jsonl')),'published vs returned JSONL bytes')
 scopes=['morsehgp3D_v12/'+x for x in ['src/','bench/','tests/','cmake/','CMakeLists.txt','microbancs/mes_t2d_b2/','microbancs/outils/lecteur_full.py','microbancs/outils/banc_full.py']]
 before_git=subprocess.check_output(['git','-C',str(REPO),'rev-parse','72f622a55']).decode().strip()
 source={};sources={}
 for label,path,commit in [('after',S/'package/package.tar.gz',r['commit']),('before',Path(pre['data_dir'])/'v12_src_72f622a55.tar.gz',before_git)]:
  raw=path.read_bytes()
  with tarfile.open(fileobj=io.BytesIO(raw)) as t:
   d={m.name:t.extractfile(m).read() for m in t.getmembers() if m.isfile() and any(m.name.startswith(q) if q.endswith('/') else m.name==q for q in scopes)}
  args=[commit+':'+p for p in sorted(d)];out=subprocess.check_output(['git','-C',str(REPO),'cat-file','--batch'],input=('\n'.join(args)+'\n').encode());at=0
  for name in sorted(d):
   e=out.index(b'\n',at);n=int(out[at:e].split()[2]);at=e+1;obj=out[at:at+n];at+=n+1;need(obj==d[name],'Git source '+name)
  need(at==len(out) and path.read_bytes()==raw,'source closure')
  inv=''.join(hashlib.sha256(v).hexdigest()+'  '+p+'\n' for p,v in sorted(d.items()))
  source[label]={'git':commit,'archive':pin(raw),'scopes':scopes,'files_exact':len(d),'inventory_sha256':hashlib.sha256(inv.encode()).hexdigest()};sources[label]=d
 need(source['after']['archive']['sha256']==r['package_sha256'] and pin(planraw)['sha256']==r['plan_sha256'],'package/plan pins')
 argv=plan['commands'][1]['argv'];need(source['before']['archive']['sha256']==argv[argv.index('--avant-sha256')+1],'before CLI')
 report=json.loads(fs[base+'rapport_t2d_b2.json']);bras=json.loads(sources['after']['morsehgp3D_v12/microbancs/mes_t2d_b2/bras_t2d_b2.json'])
 need(report['construction']['archive_avant_sha256']==source['before']['archive']['sha256'],'before report')
 need(report['construction']['bras_sha256']==pin(sources['after']['morsehgp3D_v12/microbancs/mes_t2d_b2/bras_t2d_b2.json'])['sha256'],'arms pin')
 arm_pins={}
 for arm,desc in bras['bras'].items():
  state=dict(sources['before']);pins={}
  for rel,d in sorted(desc['fichiers'].items()):
   path='morsehgp3D_v12/'+rel;raw=state[path];need(pin(raw)['sha256']==d['sha256_avant'],'arm input');text=raw.decode()
   for sub in d['substitutions']:
    need(text.count(sub['cherche'])==1,'arm substitution');text=text.replace(sub['cherche'],sub['remplace'])
   raw=text.encode();need(pin(raw)['sha256']==d['sha256_apres'],'arm output');state[path]=raw;pins[rel]=pin(raw)
  arm_pins[arm]=pins
  if arm=='apres':
   a={p:v for p,v in sources['after'].items() if p.startswith('morsehgp3D_v12/src/')};b={p:v for p,v in state.items() if p.startswith('morsehgp3D_v12/src/')};
   mismatch=[q for q in sorted(set(a)|set(b)) if a.get(q)!=b.get(q)]
   need(mismatch==['morsehgp3D_v12/src/tower/pipeline.hpp','morsehgp3D_v12/src/tower/pipeline_run.cpp'],'unexpected source mismatch')
   def stripped(v):
    v=re.sub(r'//[^\n]*','',v)
    return ''.join(v.split())
   for q in mismatch:
    text=a[q].decode();text,num=re.subn(r'#ifdef MHGP12_REGION_HOOKS\n.*?#endif\n','',text,flags=re.S);need(num==1,'hook block')
    text,num=re.subn(r'MHGP12_REGION_HOOK\([^;\n]+\);','',text);need(num==(3 if q.endswith('.cpp') else 0),'hook calls')
    need(stripped(text)==stripped(b[q].decode()),'nonhook native delta')
   exact_native=len(a)-len(mismatch)
   native_total=len(a)
 need(sources['before']['morsehgp3D_v12/bench/full_probe.cpp']==sources['after']['morsehgp3D_v12/bench/full_probe.cpp'],'probe changed')
 ctest=fs['results/cmd/000_socle_ctest/stdout'].decode();passed=len(re.findall(r'Test\s+#\d+:.*?\.\.\.\s+Passed\b',ctest));skipped=len(re.findall(r'Test\s+#\d+:.*?\.\.\.\s+\*\*\*Skipped\b',ctest));failed=len(re.findall(r'Test\s+#\d+:.*?\.\.\.\s+\*\*\*Failed\b',ctest))
 cap={'observed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'source_git':r['commit'],'helpers':{helper:pin((REPO/helper).read_bytes())},'local_files':{p:pin((S/p).read_bytes()) for p in ['receipt.json','DONE','package/plan.json','package/package.tar.gz','results/results.tar.gz','preflight.json']},'manifest':pin(manifest),'manifest_count':count,'commands':commands,'closure':{k:r[k] for k in ['status','worker_exit_code','results_verified','targeted_shutdown_certified','stop_exit_code','stop_attempts','guest_guard_intact','reserve_released','data_verified_remote']},'errors_count':len(r['errors']),'done':int((S/'DONE').read_text()),'stop':{k:{a:v for a,v in r[k].items() if a in ['status','lastStartTimestamp','lastStopTimestamp']} for k in ['observed_before_stop','observed_after']},'data_declared':[{k:d[k] for k in ['name','size','sha256']} for d in r['data_files']],'data_manifest_sha256':r['data_manifest_sha256'],'source':source,'arm_postimages':{k:{'files':len(v),'inventory_sha256':hashlib.sha256(json.dumps(v,sort_keys=True).encode()).hexdigest()} for k,v in arm_pins.items()},'after_native_files_equal':exact_native,'after_native_files_total':native_total,'native_nonidentical_hooks_only':mismatch,'report_pin':pin(fs[base+'rapport_t2d_b2.json']),'published_jsonl_files_equal':sum(p.endswith('.jsonl') for p in selected),'socle':{'passed':passed,'skipped':skipped,'failed':failed},'primary_pins':{p:pin(fs[p]) for p in ['results/commands.tsv','results/cmd/000_socle_ctest/stdout','results/cmd/001_t2d_b2_pilote/stdout','results/cmd/002_mutants_index_num_tour/stdout']}}
 return cap
