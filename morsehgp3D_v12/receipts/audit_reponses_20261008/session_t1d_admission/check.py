#!/usr/bin/env python3
"""Relecture metadata locale : archives, Git, journaux JSON, juge et bootstrap. Aucun moteur ni cloud."""
from pathlib import Path
import argparse, csv, hashlib, importlib.util, io, json, math, random, re, statistics, subprocess, sys, tarfile, tempfile
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
PREFIX='morsehgp3D_v12/'
PINS={'t1d':'5f8e777cffbeddfe90e92fc616a920c28c1b985c','t1d2':'c31beaf2200d1a7f8eed09b0c1c7c0f6af3b74f3'}
BEFORE='4171b2653fa5cb903102a3fc44572bcb3330b3e8'
BASE='results/cmd/000_t1d_flux/files/t1d/'
SIZES={'ng00':39885,'ng01':35551,'ng02':45845,'u8000':8000,'u16000':16000,'u32000':32000}
DATA={'ng00':'lidar_ng00','ng01':'lidar_ng01','ng02':'lidar_ng02','u8000':'uniform_u18_n8000','u16000':'uniform_u18_n16000','u32000':'uniform_u18_n32000'}
def need(ok,why):
 if not ok: raise ValueError(why)
def pin(b): return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def git(repo,commit,p): return subprocess.check_output(['git','-C',str(repo),'show',commit+':'+p])
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
def source(repo,path,commit):
 raw=path.read_bytes();scopes=[PREFIX+p for p in ('src/','bench/','tests/','cmake/','CMakeLists.txt','microbancs/outils/lecteur_full.py')]
 with tarfile.open(fileobj=io.BytesIO(raw)) as t:
  fs={m.name:t.extractfile(m).read() for m in t if m.isfile() and any(m.name.startswith(s) if s.endswith('/') else m.name==s for s in scopes)}
 names=sorted(fs);out=subprocess.check_output(['git','-C',str(repo),'cat-file','--batch'],input=('\n'.join(commit+':'+n for n in names)+'\n').encode());at=0
 for n in names:
  e=out.index(b'\n',at);size=int(out[at:e].split()[2]);at=e+1;need(out[at:at+size]==fs[n],'source Git '+n);at+=size+1
 need(at==len(out) and path.read_bytes()==raw,'source stable')
 inv=''.join(hashlib.sha256(fs[n]).hexdigest()+'  '+n+'\n' for n in names)
 return {'commit':commit,'archive':pin(raw),'scopes':scopes,'files_exact':len(fs),'inventory_sha256':pin(inv.encode())['sha256']},fs

def envelope(repo,s,helper,label):
 r=json.loads((s/'receipt.json').read_text());planraw=(s/'package/plan.json').read_bytes();plan=json.loads(planraw)
 raw=(s/'results/results.tar.gz').read_bytes();fs=helper.files(raw);man=helper.role(fs,'MANIFEST.sha256');n=len(man.decode().splitlines());helper.manifest(fs,n)
 need(r['commit']==PINS[label] and r['results_sha256']==pin(raw)['sha256'] and r['results_bytes']==len(raw),'receipt source/results')
 need(r['status']=='completed' and r['worker_exit_code']==0 and r['results_verified'] is True and not r['errors'],'session outcome')
 need(int((s/'DONE').read_text())==0 and r['targeted_shutdown_certified'] is True and r['stop_exit_code']==0,'stop certified')
 need(r['observed_before_stop']['status']=='RUNNING' and r['observed_after']['status']=='TERMINATED','stop observation')
 commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'))
 need(commands==r['commands'] and len(commands)==1 and commands[0]['name']=='t1d_flux' and commands[0]['exit_code']=='0','command closure')
 need(len(plan['commands'])==1 and plan['commands'][0]['name']=='t1d_flux','plan cohort')
 argv=plan['commands'][0]['argv'];args={k:argv[i+1] for i,k in enumerate(argv[:-1]) if k in ('--tours','--passes','--fils','--avant-sha256')}
 need(args['--tours']=='10' and args['--passes']=='10' and args['--fils']=='48','plan parameters')
 after,src=source(repo,s/'package/package.tar.gz',PINS[label]);pre=json.loads((s/'preflight.json').read_text())
 before,before_src=source(repo,Path(pre['data_dir'])/'v12_src_4171b2653.tar.gz',BEFORE)
 need(after['archive']['sha256']==r['package_sha256'] and before['archive']['sha256']==args['--avant-sha256'] and pin(planraw)['sha256']==r['plan_sha256'],'package/plan input')
 declared={d['name']:d for d in r['data_files']}
 for c,stem in DATA.items():
  need(declared[stem+'.u32le']['size']==12*SIZES[c] and declared[stem+'.ids.u32le']['size']==4*SIZES[c],'declared size '+c)
 report=json.loads(fs[BASE+'report.json']);published=repo/PREFIX/'receipts/g4_t1d_20261008'/label/'resultats/cmd/000_t1d_flux/files/t1d'
 selected={n:v for n,v in fs.items() if n.startswith(BASE+'logs/') and re.fullmatch(r'(c_r\d+_ng\d+_(?:avant|avant_bis|apres)|id_(?:ng\d+|u\d+)_k\d+_(?:cpu|dev)|flux_ng\d+_k\d+_(?:libre|1_\d+)|ful1_(?:avant|apres)_(?:ng\d+|u\d+)_k\d+)\.log',Path(n).name)}
 need(len(selected)==168,'measurement journals count')
 pub_report=json.loads((published/'report.json').read_text())
 need(all(pub_report[k]==report[k] for k in ('options','rule','verdict')) and all(pub_report['steps'][k]==report['steps'][k] for k in ('identity','slices','ful1','campaign','binaries','binaries_after')),'published numeric report')
 ctest=fs[BASE+'logs/ctest.log'].decode();gate_counts={k:len(re.findall(r'Test\s+#\d+:.*?\.\.\.\s+'+v+r'\b',ctest)) for k,v in [('passed','Passed'),('failed',r'\*\*\*Failed'),('skipped',r'\*\*\*Skipped')]}
 need(gate_counts=={'passed':741,'failed':0,'skipped':0},'native gates primary')
 for label_gpu in ('avant','apres'):
  body=fs[BASE+'logs/gpu_apps_'+label_gpu+'.log'].decode().split('\n',1)[1];out,err=body.split('\n--- stderr ---\n',1);need(not out.strip() and not err.strip(),'GPU endpoint primary')
 for name in ('device_open','device_open_budget'):
  out=fs[BASE+'logs/'+name+'.log'].decode().split('\n',1)[1].split('\n--- stderr ---\n',1)[0].rstrip()
  need(out==report['steps']['gates'][name]['stdout'].rstrip(),'device gate primary')
 cap={'source_git':r['commit'],'gates_primary':gate_counts,'published_report':pin((published/'report.json').read_bytes()),'files':{p:pin((s/p).read_bytes()) for p in ('receipt.json','DONE','preflight.json','package/plan.json','package/package.tar.gz','results/results.tar.gz')},'manifest':pin(man),'manifest_entries':n,'commands':commands,'closure':{k:r[k] for k in ('status','worker_exit_code','results_verified','targeted_shutdown_certified','stop_exit_code','stop_attempts','reserve_released','guest_guard_intact')},'done':0,'errors_count':0,'before_stop':'RUNNING','after_stop':'TERMINATED','sources':{'before':before,'after':after},'report':pin(fs[BASE+'report.json']),'data_declared':[{k:d[k] for k in ('name','size','sha256')} for d in r['data_files']],'measurement_logs':{'count':len(selected),'inventory_sha256':pin(''.join(pin(v)['sha256']+'  '+Path(n).name+'\n' for n,v in sorted(selected.items())).encode())['sha256']},'critical_sources':{p:pin(src[PREFIX+p]) for p in ('bench/g4_catalogue_t1d.py','bench/g4_catalogue_t1d_judge.py','bench/g4_catalogue_flux_lecteur.py','bench/g4_catalogue_flux_judge.py','bench/g4_catalogue_flux.py','bench/g4_catalogue_device.py','bench/catalogue_probe.cpp','bench/full_probe.cpp','microbancs/outils/lecteur_full.py')}}
 return cap,report,fs,src,published

def judge_modules(src,root,patch=None):
 for n,v in src.items():
  if n.startswith(PREFIX+'bench/') and n.endswith('.py') or n==PREFIX+'microbancs/outils/lecteur_full.py':
   dest=root/n;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(v)
 if patch:
  subprocess.run(['git','apply','--check',str(patch)],cwd=root,check=True,capture_output=True)
  subprocess.run(['git','apply',str(patch)],cwd=root,check=True,capture_output=True)
 for name in list(sys.modules):
  if name.startswith('g4_catalogue_'): del sys.modules[name]
 return module('t1d_judge',root/PREFIX/'bench/g4_catalogue_t1d_judge.py'),module('lf',root/PREFIX/'microbancs/outils/lecteur_full.py')

def boot(logs):
 rng=random.Random(20261008);n=len(logs);bs=sorted(sum(rng.choices(logs,k=n))/n for _ in range(10000))
 return dict(gm=math.exp(sum(logs)/n),low=math.exp(bs[250]),high=math.exp(bs[9749]))

def numerical(label,report,fs,T,LF,published):
 L,J=T.L,T.J;s=report['steps'];need(report['options']['rounds']==10 and report['options']['passes']==10 and report['options']['threads']==48 and report['options']['essai'] is False,'report config')
 need(report['rule']==T.RULE,'rule pin');need(set((x['case'],x['k']) for x in s['identity'])==set(J.IDENTITY_CASES) and len(s['identity'])==9,'identity cohort')
 checked=set();counts={};prefixes=[];catalogue_passes=0;full_passes=0
 def raw(run,name):
  nonlocal catalogue_passes,full_passes
  need(name not in checked,'journal reused');checked.add(name)
  blob=fs[BASE+'logs/'+name+'.log'].decode();header,body=blob.split('\n',1);stdout,stderr=body.split('\n--- stderr ---\n',1)
  need(not stderr.strip() and header.startswith('$ ') and header.endswith(' '+' '.join(run['options'])),'raw command or stderr '+name)
  rows,problem=LF.read_rows(stdout);need(not problem and rows==run['rows'] and type(run['code']) is int and run['timeout'] is False and type(run['bad_lines']) is int and run['bad_lines']==0,'raw rows '+name)
  # Public receipt stores the report; native stdout is bound to the closed return archive.

  catalogue_passes+=sum(r['phase']=='catalogue' and r['status']=='ok' for r in rows);full_passes+=sum(r['phase']=='full' for r in rows)
  return stdout,header
 def cat(run,spec,name,case):
  _,header=raw(run,name);words=header.split();need(Path(words[2]).name==DATA[case]+'.u32le' and Path(words[3]).name==DATA[case]+'.ids.u32le','catalogue input metadata')
  arm='avant' if name.endswith(('_avant','_avant_bis')) else 'apres';need(Path(words[1]).name=='mhgp12_catalogue_probe' and Path(words[1]).parent.name=='b_'+arm,'catalogue arm')
  state,why,p=L.read_catalogue(run,spec);need(state in ('ok','refus'),'catalogue '+name+' '+why)
  need(all(r['sites']==SIZES[case] for r in p['passes']),'sites '+name)
  if p['failed'] is not None:
   r=p['failed'];need(all(L.is_int(r[x]) for x in L.CAT_INTS) and r['sites']==SIZES[case] and r['path']==spec['path'] and r['coord_bits']==21 and r['kmax']==spec['k'] and r['leaf']==24 and r['threads']==48,'refusal config')
  return state,why,p
 refs={}
 for e in s['identity']:
  c,k=e['case'],e['k'];parts={}
  for path,n in [('cpu',1),('device',3)]:
   st,why,p=cat(e[path],L.catalogue_spec(path,k,48,n,True,True,path=='device'),'id_%s_k%d_%s'%(c,k,'dev' if path=='device' else 'cpu'),c);need(st=='ok','identity success');parts[path]=p
  ref=parts['cpu']['passes'][0];refs[c,k]=ref
  for p in parts.values():
   need(all(d['catalogue_sha256']==J.F2_DIGESTS[c+':'+str(k)] for d in p['digests']),'F2')
   need(all(all(r[x]==ref[x] for x in L.COUNTS) and r['ledger']==ref['ledger'] for r in p['passes']),'counts/ledger')
   need(all(d==parts['cpu']['complets'][0] and d['table_ecarts']==0 for d in p['complets']),'levels/table')
 slices={};expected={(c,k,0) for c,k in T.SLICE_CASES};budgets=[]
 for e in s['slices']:
  key=(e['case'],e['k'],e['budget']);need(key not in slices,'slice duplicate');slices[key]=e
 for c,k in T.SLICE_CASES:
  free=slices[c,k,0];st,why,p=cat(free['run'],T.spec_free(k,48),'flux_%s_k%d_libre'%(c,k),c);need(st=='ok','free slice')
  held=p['tranches'][-1]['device_bytes'];need(held>0,'held')
  for fraction in [(0,1)]+list(T.FRACTIONS):
   num,den=fraction;budget=held*num//den;expected.add((c,k,budget))
   if num:
    e=slices[c,k,budget];st,why,p=cat(e['run'],T.spec_budget(k,48,budget),'flux_%s_k%d_%d_%d'%(c,k,num,den),c)
   need(st=='ok' or why=='resource_exhausted/memory_budget','slice outcome')
   need(all(d['catalogue_sha256']==J.F2_DIGESTS[c+':'+str(k)] for d in p['digests']),'prefix F2')
   need(all(all(r[x]==refs[c,k][x] for x in L.COUNTS) for r in p['passes']),'prefix counts')
   need(len(p['tranches'])==len(p['passes'])==len(p['digests']),'complete prefix')
   prev=0
   for r,t in zip(p['passes'],p['tranches']):
    need(t['device_bytes']==r['device']['device_bytes'],'device bytes linkage')
    need(t['device_bytes']<=t['device_peak']<=budget and prev<=t['device_peak'] if budget else t['device_peak']==0,'device cumulative peak')
    prev=t['device_peak']
   budgets.append(dict(case=c,k=k,budget=budget,state=st,complete_passes=len(p['passes']),peak=max((t['device_peak'] for t in p['tranches']),default=0),held=max((t['device_bytes'] for t in p['tranches']),default=0),slices=max((t['finish_slices'] for t in p['tranches']),default=0),streamed=max((t['arena_streamed'] for t in p['tranches']),default=0)))
 need(set(slices)==expected and len(slices)==42,'budget cohort')
 need(len(s['ful1'])==18 and set((e['arm'],e['case'],e['k']) for e in s['ful1'])=={(a,c,k) for a in ('avant','apres') for c,k in J.FUL1_CASES},'FULL cohort')
 fhash={};fulls=[]
 for e in s['ful1']:
  a,c,k=e['arm'],e['case'],e['k'];run=e['run'];stdout,header=raw(run,'ful1_%s_%s_k%d'%(a,c,k));schema='recouvert' if label=='t1d' else 'sequentiel'
  need(run['options']==L.full_options(k,48,2) and (('--sequentiel' in run['options'])==(schema=='sequentiel')),'native FULL schema CLI')
  words=header.split();parts=words[2].split('=',1)[1].split(',');need(words[2].startswith('--trame=') and len(parts)==3 and Path(parts[0]).name==DATA[c]+'.u32le' and Path(parts[1]).name==DATA[c]+'.ids.u32le' and parts[2]==c and Path(words[1]).name=='mhgp12_full_probe' and Path(words[1]).parent.name=='b_'+a,'FULL input/arm metadata')
  expected_full=dict(voie='appareil',k=k,fils=48,passes=2,empreinte=True,trames=[(c,SIZES[c])],budget_appareil='partage',bits=21,schema=schema)
  parsed=LF.parse_output(run['code'],stdout,expected_full);need(parsed['etat']=='ok','FULL independent '+a+c+str(k)+' '+str(parsed.get('raison')))
  rows=[r for r in run['rows'] if r['phase']=='full'];hs={r['full_sha256'] for r in rows};need(len(hs)==1,'stable FULL');digest=hs.pop();fhash[a,c,k]=digest
  need(c+':'+str(k) not in J.FUL1_SESSION_K or digest==J.FUL1_SESSION_K[c+':'+str(k)],'FULL session K')
  fulls.append(dict(arm=a,case=c,k=k,schema=schema,cold_wall_ns=rows[0]['wall_ns'],warm_wall_ns=rows[1]['wall_ns']))
 need(all(fhash['avant',c,k]==fhash['apres',c,k] for c,k in J.FUL1_CASES),'FULL before/after')
 table={};walls={};residual=[]
 for e in s['campaign']:
  a,c,r=e['arm'],e['frame'],e['round'];need(type(r) is int and 0<=r<10 and a in T.ARMS and c in T.FRAMES,'campaign domain');key=(r,c,a);need(key not in table,'duplicate campaign')
  order=list(T.ARMS[r%3:]+T.ARMS[:r%3]);order=order[::-1] if r%2 else order
  need(type(e['position']) is int and order[e['position']]==a,'order position')
  st,why,p=cat(e['run'],T.spec_campaign(48,10),'c_r%d_%s_%s'%(r,c,a),c);need(st=='ok','campaign success')
  need(all(all(row[x]==refs[c,5][x] for x in L.COUNTS) for row in p['passes']),'campaign counts')
  for row,extra in zip(p['passes'],p['sorties']):
   ns=T.stage_total(row,extra);need(ns<=row['wall_ns'],'C stages');residual.append(row['wall_ns']-ns)
  times=[row['wall_ns'] for row in p['passes'][1:]];table[key]=statistics.median(times);walls[key]=times
 need(len(table)==90 and len(checked)==168,'complete cohort')
 stats={};absolute={}
 for c in T.FRAMES:
  stats[c]={name:boot([math.log(table[r,c,b]/table[r,c,a]) for r in range(10)]) for name,a,b in [('lot','avant','apres'),('A/A','avant','avant_bis')]}
  absolute[c]={a:dict(median_process_ns=statistics.median([table[r,c,a] for r in range(10)]),median_pooled_ns=statistics.median([v for r in range(10) for v in walls[r,c,a]]),max_process_median_ns=max(table[r,c,a] for r in range(10)),max_warm_ns=max(v for r in range(10) for v in walls[r,c,a])) for a in T.ARMS}
  for name,vals in stats[c].items():
   ref=report['verdict']['stats']['levers'][name]['frames'][c]
   need(all(vals[k]==ref[k] for k in vals),'bootstrap exact')
 need(all(v['lot']['high']<=1.01 and abs(v['A/A']['gm']-1)<=.015 for v in stats.values()),'cost and AA')
 return dict(measurement_processes=len(checked),catalogue_success_passes=catalogue_passes,full_passes=full_passes,campaign_warm_passes=810,identity_cases=9,full_cases_before_after_equal=9,full_session_K_cases=6,stats=stats,catalogue_absolute=absolute,C_residual_min_ns=min(residual),slices=budgets,slice_budget_success=sum(x['budget']>0 and x['state']=='ok' for x in budgets),slice_budget_refusal=sum(x['state']=='refus' for x in budgets),real_success_prefixes_before_refusal=sum(x['complete_passes']>0 and x['state']=='refus' for x in budgets),full_schema='recouvert' if label=='t1d' else 'sequentiel',binaries=s['binaries'],binaries_after=s['binaries_after'],worker_verdict=report['verdict']['verdict'])

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--repo',type=Path,required=True);ap.add_argument('--sessions',type=Path,required=True);ap.add_argument('--patch',type=Path,required=True);ap.add_argument('--write',action='store_true');args=ap.parse_args();args.patch=args.patch.resolve()
 helper_path=args.repo/PREFIX/'receipts/audit_reponses_20261008/session_l1_recuperation/check.py';helper=module('archive_helper',helper_path)
 capture={'helper':pin(helper_path.read_bytes()),'port_c31_patch':pin(args.patch.read_bytes()),'sessions':{}};results={}
 for label in PINS:
  cap,report,fs,src,published=envelope(args.repo,args.sessions/('v12.20261008.'+label),helper,label);capture['sessions'][label]=cap
  with tempfile.TemporaryDirectory() as td:
   T,LF=judge_modules(src,Path(td));original=T.judge(report);need(original==report['verdict'],'original verdict replay exact');results[label]=numerical(label,report,fs,T,LF,published)
  # Port c31 is used unchanged on its own source; old5f keeps its original schema refusal.
  current={p:git(args.repo,PINS['t1d2'],p) for p in src if p.startswith(PREFIX+'bench/') and p.endswith('.py') or p==PREFIX+'microbancs/outils/lecteur_full.py'}
  with tempfile.TemporaryDirectory() as td:
   T,LF=judge_modules(current,Path(td),args.patch);rejudged=T.judge(report)
   need(rejudged['verdict']==report['verdict']['verdict'] and rejudged['rejected']==[],'hardened verdict')
   need(rejudged==report['verdict'] if label=='t1d2' else len(rejudged['refused'])==18,'hardened report scope')
   results[label]['hardened_verdict']=rejudged['verdict'];results[label]['hardened_refusals']=len(rejudged['refused'])
 if args.write:
  (HERE/'capture.json').write_text(json.dumps(capture,ensure_ascii=False,separators=(',',':'))+'\n');(HERE/'results.json').write_text(json.dumps(results,ensure_ascii=False,separators=(',',':'))+'\n')
 else:
  need(capture==json.loads((HERE/'capture.json').read_text()),'capture changed');need(results==json.loads((HERE/'results.json').read_text()),'results changed')
 print(json.dumps({k:{x:v[x] for x in ('measurement_processes','catalogue_success_passes','full_passes','worker_verdict','hardened_verdict')} for k,v in results.items()}))
if __name__=='__main__':main()
