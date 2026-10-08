#!/usr/bin/env python3
"""R1 : relecture locale sources/metadonnees/journaux ; aucun natif/cloud/payload."""
from pathlib import Path
import argparse,csv,hashlib,importlib.util,io,json,math,random,re,statistics,subprocess,sys,tarfile,tempfile
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
PREFIX='morsehgp3D_v12/'
SOURCE='47feedc96ee4b4cb85b5a7c590f38c874bb04855'
BEFORE='5f8e777cffbeddfe90e92fc616a920c28c1b985c'
BASE='results/cmd/001_r1_pilote/files/r1/'
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(b):return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def load(name,p):
 s=importlib.util.spec_from_file_location(name,p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def boot(xs):
 rng=random.Random(20261008);n=len(xs);bs=sorted(sum(xs[rng.randrange(n)] for _ in xs)/n for _ in range(10000));return dict(tours=n,moyenne_geometrique=math.exp(sum(xs)/n),ic95=[math.exp(bs[250]),math.exp(bs[9749])])
def summary(groups):
 med=[statistics.median(x) for x in groups];warm=[v for x in groups for v in x]
 return dict(processes=len(groups),warm_passes=len(warm),median_process_ns=statistics.median(med),median_pooled_ns=statistics.median(warm),max_process_median_ns=max(med),max_warm_ns=max(warm),warm_over_100ms=sum(v>100000000 for v in warm))
def run(args):
 s=args.session;r=json.loads((s/'receipt.json').read_text());planraw=(s/'package/plan.json').read_bytes();plan=json.loads(planraw);pre=json.loads((s/'preflight.json').read_text())
 helper_path=args.repo/PREFIX/'receipts/audit_reponses_20261008/session_l1_recuperation/check.py';helper=load('archive_helper',helper_path)
 common_path=HERE.parent/'session_t1d_admission/check.py';common=load('t1d_common',common_path)
 raw=(s/'results/results.tar.gz').read_bytes();fs=helper.files(raw);man=helper.role(fs,'MANIFEST.sha256');helper.manifest(fs,289)
 need(r['commit']==SOURCE and r['results_sha256']==pin(raw)['sha256'] and r['results_bytes']==len(raw),'source/archive')
 need(r['status']=='completed' and r['worker_exit_code']==0 and r['results_verified'] is True and not r['errors'] and int((s/'DONE').read_text())==0,'outcome')
 need(r['targeted_shutdown_certified'] is True and r['stop_exit_code']==0 and r['observed_before_stop']['status']=='RUNNING' and r['observed_after']['status']=='TERMINATED','certified stop')
 commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'));need(commands==r['commands'] and [c['name'] for c in commands]==['socle_ctest','r1_pilote','lidar_ctest','mutants_tour'] and all(c['exit_code']=='0' for c in commands),'commands')
 argv=plan['commands'][1]['argv'];opts={k:argv[i+1] for i,k in enumerate(argv[:-1]) if k in ('--base','--avant-sha256','--fils','--passes','--tours','--tours-grandes','--tours-k10','--passes-k10','--delai')}
 need(opts['--base']=='5f8e777cf' and opts['--fils']=='48' and opts['--tours']=='5' and opts['--passes']=='10' and opts['--delai']=='600','effective plan')
 need([opts.get(k) for k in ('--tours-grandes','--tours-k10','--passes-k10')]==['6','3','5'] and '--essai' not in argv,'explicit large/K10 CLI')
 before,bsrc=common.source(args.repo,Path(pre['data_dir'])/'v12_src_5f8e777cf.tar.gz',BEFORE)
 after,src=common.source(args.repo,s/'package/package.tar.gz',SOURCE)
 need(src[PREFIX+'bench/full_probe.cpp']==bsrc[PREFIX+'bench/full_probe.cpp'],'unchanged FULL emitter')
 need(before['archive']['sha256']==opts['--avant-sha256'] and after['archive']['sha256']==r['package_sha256'] and pin(planraw)['sha256']==r['plan_sha256'],'package/plan')
 extra=['microbancs/mes_r1/pilote_r1.py','microbancs/mes_r1/README.md','microbancs/mes_r1/test_pilote_r1.py','microbancs/outils/banc_full.py']
 with tarfile.open(s/'package/package.tar.gz') as t:
  for path in extra:
   b=t.extractfile(PREFIX+path).read();need(b==common.git(args.repo,SOURCE,PREFIX+path),'pilot package Git');src[PREFIX+path]=b
 cohort_path=args.repo/PREFIX/'receipts/audit_reponses_20261008/session_t2da_admission/capture.json';cohort=json.loads(cohort_path.read_text())['cohort'];cases=[[c['name'],c['sites']] for c in cohort]
 old_path=args.repo/PREFIX/'receipts/audit_reponses_20261008/session_m_provenance/capture.json';old=json.loads(old_path.read_text());name='g4_kitti_v12set_xyz.tar';vold=next(d for d in old['data_declared'] if d['name']==name);vnew=next({k:d[k] for k in ('name','size','sha256')} for d in r['data_files'] if d['name']==name);need(vold==vnew,'same externally qualified cohort archive')
 expected=dict(cas=cases,fils=48,passes=10,tours=5,tours_grandes=6,tours_k10=3,passes_k10=5,essai=False,base='5f8e777cf')
 report=json.loads(fs[BASE+'rapport_r1.json']);need(report['cohorte_demandee']==expected,'external cohort')
 gates={}
 for idx,name in ((0,'socle_ctest'),(2,'lidar_ctest'),(3,'mutants_tour')):
  b=fs['results/cmd/%03d_%s/stdout'%(idx,name)].decode();gates[name]={key:len(re.findall(r'Test\s+#\d+:.*?\.\.\.\s+'+pattern+r'\b',b)) for key,pattern in [('passed','Passed'),('skipped',r'\*\*\*Skipped'),('failed',r'\*\*\*Failed')]}
 needed={n:v for n,v in fs.items() if n.startswith(BASE) and (n.endswith('.jsonl') or n.endswith('.jsonl.err') or n==BASE+'rapport_r1.json')}
 need(sum(n.endswith('.jsonl') for n in needed)==112,'native journal count')
 args.snapshot.mkdir(parents=True,exist_ok=True)
 for n,v in needed.items():
  p=args.snapshot/'returned'/n[len(BASE):];p.parent.mkdir(parents=True,exist_ok=True)
  if p.exists():need(p.read_bytes()==v,'snapshot changed')
  else:p.write_bytes(v)
 with tempfile.TemporaryDirectory() as td:
  root=Path(td)
  for path in ['microbancs/mes_r1/pilote_r1.py','microbancs/outils/banc_full.py','microbancs/outils/lecteur_full.py']:
   p=root/PREFIX/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(src[PREFIX+path])
  P=load('r1_pilot',root/PREFIX/'microbancs/mes_r1/pilote_r1.py');LF=P.lf
  need(P.verifier_cohorte(report,expected,False)=='','closed cohort validator')
  judgment=P.juger(report,str(args.snapshot/'returned'),True,False,expected)
  diffs=[]
  def compare(a,b,path=''):
   if type(a)is dict and type(b)is dict:
    need(set(a)==set(b),'judgment keys');[compare(a[k],b[k],path+'/'+k) for k in a]
   elif type(a)is list and type(b)is list:
    need(len(a)==len(b),'judgment array');[compare(x,y,path+'/'+str(i)) for i,(x,y) in enumerate(zip(a,b))]
   elif a!=b:
    need(type(a)is float and type(b)is float and abs(a-b)<=max(math.ulp(a),math.ulp(b)),'judgment mismatch '+path);diffs.append(dict(path=path,replay=a,worker=b))
  compare(judgment,report['jugement']);need(judgment['verdict']=='adopte' and not judgment['refus'],'judgment')
  groups=P.prises_du_rapport(report);need(len(groups)==112 and len({p['journal'] for p in groups})==112,'unique native groups')
  rows={};total=0
  for p in groups:
   b=fs[BASE+p['journal']];need(pin(b)['sha256']==p['journal_sha256'] and type(p['code'])is int and p['code']==0 and fs[BASE+p['journal']+'.err'].strip()==b'','native provenance')
   parsed=LF.parse_output(p['code'],b.decode(),p['attendu']);need(parsed['etat']=='ok' and P.resume(parsed['passes'])==p['resume'],'strict FULL replay')
   rows[p['journal']]=parsed['passes'];total+=len(parsed['passes'])
  need(total==1441,'FULL pass count')
  absolute={};stats={}
  for stage,k in [('ng',5),('k10',10)]:
   absolute[stage]={}
   for name,tours in report[stage]['trames'].items():
    absolute[stage][name]={a:summary([[r['wall_ns'] for r in rows[t[a]['journal']][1:]] for t in tours]) for a in P.BRAS}
    for a,tail in [('apres',''),('avant_bis','_aa')]:
     xs=[math.log(statistics.median([r['wall_ns'] for r in rows[t[a]['journal']][1:]])/statistics.median([r['wall_ns'] for r in rows[t['avant']['journal']][1:]])) for t in tours]
     stats[('k10_' if k==10 else '')+name+tail]=boot(xs)
  large_names=report['grandes']['trames'];large={a:{n:[] for n in large_names} for a in P.BRAS};large_rows={a:[] for a in P.BRAS}
  for t in report['grandes']['tours']:
   for a in P.BRAS:
    rs=rows[t[a]['journal']][len(large_names):];need(len(rs)==len(large_names),'large warm rounds')
    for n,rw in zip(t['ordre'],rs):large[a][n].append(rw['wall_ns']);large_rows[a].append(rw)
  for a,tail in [('apres',''),('avant_bis','_aa')]:
   xs=[sum(math.log(large[a][n][t]/large['avant'][n][t]) for n in report['grandes']['tours'][t]['ordre'])/len(large_names) for t in range(6)];stats['grandes'+tail]=boot(xs)
  for key,val in stats.items():
   section='information_k10' if key.startswith('k10_') else 'statistiques';ref=judgment['cas'][section][key]
   need(val==ref,'independent bootstrap '+key)
  grande_summary={}
  for a,byname in large.items():
   ms=[statistics.median(v) for v in byname.values()];warm=[v for vs in byname.values() for v in vs]
   grande_summary[a]=dict(frames=len(byname),warm_passes=len(warm),median_frame_medians_ns=statistics.median(ms),median_pooled_ns=statistics.median(warm),max_frame_median_ns=max(ms),max_warm_ns=max(warm),frames_median_over_100ms=sum(v>100000000 for v in ms),passes_over_100ms=sum(v>100000000 for v in warm),per_frame_median_ns={n:statistics.median(v) for n,v in byname.items()})
  bins={a:v['sha256'] for a,v in report['construction']['binaires'].items()}
  for desc in report['construction']['binaires'].values():
   need(set(['CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21','MHGP12_ENABLE_CUDA:BOOL=ON'])<=set(desc['cmake']),'build config')
  need(bins==report['ng']['binaires_apres']==report['k10']['binaires_apres'] and bins['avant']==bins['avant_bis'],'ELF closure after K10')
  need(report['construction']['archive_avant_sha256']==before['archive']['sha256'],'constructed base archive')
  # FUL1 is an identity of the FULL canonical serialization, not of R CSR arrays.
  identity={k:{a:p['resume']['empreintes'] for a,p in group.items()} for k,group in report['identite']['prises'].items()}
  native_delta=[p.removeprefix(PREFIX) for p in sorted(set(src)|set(bsrc)) if p.startswith(PREFIX+'src/') and src.get(p)!=bsrc.get(p)]
  need(native_delta==['src/tower/registry_branches.cpp'],'isolated product change')
  identity_passes=sum(len(rows[p['journal']]) for group in report['identite']['prises'].values() for p in group.values());need(identity_passes==100,'identity passes')
  result=dict(processes=112,full_passes=1441,decisive_warm_passes=783,informative_k10_warm_passes=108,identity_passes=100,verdict=judgment['verdict'],bootstrap=stats,worker_float_differences=diffs,absolute_ng=absolute['ng'],absolute_k10=absolute['k10'],large=grande_summary,elf=bins,native_changed_from_before=native_delta,native_delta_pins={p:{'before':pin(bsrc[PREFIX+p]),'after':pin(src[PREFIX+p])} for p in native_delta},identity_sha256=pin(json.dumps(identity,sort_keys=True).encode())['sha256'])
 cap=dict(source_git=SOURCE,before_git=BEFORE,helpers={'../session_t1d_admission/check.py' if p==common_path else str(p.relative_to(args.repo)):pin(p.read_bytes()) for p in [helper_path,common_path,cohort_path,old_path]},files={p:pin((s/p).read_bytes()) for p in ['receipt.json','DONE','preflight.json','package/plan.json','package/package.tar.gz','results/results.tar.gz']},manifest=pin(man),manifest_entries=289,closure={k:r[k] for k in ['status','worker_exit_code','results_verified','targeted_shutdown_certified','stop_exit_code','stop_attempts','reserve_released','guest_guard_intact']},commands=commands,errors_count=0,done=0,stop_before='RUNNING',stop_after='TERMINATED',sources=dict(before=before,after=after),critical_source_pins={p:pin(src[PREFIX+p]) for p in ['bench/full_probe.cpp','microbancs/mes_r1/pilote_r1.py','microbancs/mes_r1/test_pilote_r1.py','microbancs/outils/lecteur_full.py','microbancs/outils/banc_full.py']},plan=expected,gates_primary=gates,data_declared=[{k:d[k] for k in ('name','size','sha256')} for d in r['data_files']],report_pin=pin(fs[BASE+'rapport_r1.json']),journals_inventory_sha256=pin(''.join(pin(v)['sha256']+'  '+n+'\n' for n,v in sorted(needed.items())).encode())['sha256'])
 return cap,result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--session',type=Path,required=True);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--write',action='store_true');a=p.parse_args();a.repo=a.repo.resolve();a.session=a.session.resolve();a.snapshot=a.snapshot.resolve();cap,res=run(a)
 for name,obj in [('capture.json',cap),('results.json',res)]:
  target=HERE/name
  if a.write:target.write_text(json.dumps(obj,ensure_ascii=False,separators=(',',':'))+'\n')
  else:need(obj==json.loads(target.read_text()),name+' changed')
 print(json.dumps({k:res[k] for k in ['processes','full_passes','decisive_warm_passes','informative_k10_warm_passes','verdict','worker_float_differences']}))
if __name__=='__main__':main()
