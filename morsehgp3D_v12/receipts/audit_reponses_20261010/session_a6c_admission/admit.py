#!/usr/bin/env python3
"""Admission A6c depuis JSONL uniquement ; aucun appel de sonde."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,math,statistics,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;PREFIX='morsehgp3D_v12/'
def need(x,w):
 if not x:raise ValueError(w)
def load(n,p):
 s=importlib.util.spec_from_file_location(n,p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def run(repo,returned,protocol_path,source_root):
 meta=json.loads(protocol_path.read_text())
 for p,v in meta['source_pins'].items():need(hashlib.sha256((source_root/PREFIX/p).read_bytes()).hexdigest()==v['sha256'],'source pin '+p)
 P=load('a6c_pilot',source_root/PREFIX/'microbancs/mes_t2d_a6c/pilote_t2d_a6c.py');LF=P.lf
 old=load('r1_arithmetic',repo/PREFIX/'receipts/audit_reponses_20261008/session_r1_admission/check.py')
 report=json.loads((returned/'rapport_t2d_a6c.json').read_text());expected=meta['cohort']
 need(report['regle']==meta['rule'],'preannounced rule unchanged')
 need(report.get('cohorte_demandee')==expected,'external cohort declaration')
 judgment=P.juger(report,str(returned),True,False,expected)
 diffs=[]
 def compare(a,b,path=''):
  if type(a)is dict and type(b)is dict:
   need(set(a)==set(b),'judgment keys');[compare(a[k],b[k],path+'/'+k) for k in a]
  elif type(a)is list and type(b)is list:
   need(len(a)==len(b),'judgment array');[compare(x,y,path+'/'+str(i))for i,(x,y)in enumerate(zip(a,b))]
  elif a!=b:
   need(type(a)is float and type(b)is float and abs(a-b)<=max(math.ulp(a),math.ulp(b)),'judgment mismatch '+path);diffs.append(dict(path=path,replay=a,worker=b))
 compare(judgment,report['jugement'])
 why=P.verifier_cohorte(report,expected,False)
 if why:return dict(cohort_admitted=False,why=why,judgment=judgment,worker_float_differences=diffs)
 groups=P.prises_du_rapport(report);need(len(groups)==85 and len({p['journal']for p in groups})==85,'85 unique processes')
 rows={};total=0
 for p in groups:
  name=p['journal'];path=(returned/name).resolve();need(path.is_relative_to(returned.resolve()),'external journal')
  raw=path.read_bytes();need(hashlib.sha256(raw).hexdigest()==p['journal_sha256']and type(p['code'])is int and p['code']==0 and not Path(str(path)+'.err').read_bytes().strip(),'native code/hash/stderr')
  parsed=LF.parse_output(p['code'],raw.decode(),p['attendu']);need(parsed['etat']=='ok'and P.resume(parsed['passes'])==p['resume'],'strict FULL replay')
  resident=16*sum(dict(p['attendu']['trames']).values())
  for row in parsed['passes']:
   need(row['pic_octets']>=resident and row['epinglee_octets']<=row['pic_octets']and row['appareil_octets']<=row['pic_octets'],'shared active memory')
   need(all(x[0]>=resident for x in row['memoire_octets'].values()),'resident input per stage')
  rows[name]=parsed['passes'];total+=len(parsed['passes'])
 need(total==1306,'1306 FULL passes')
 absolute={};stats={}
 for name,tours in report['ng']['trames'].items():
  absolute[name]={arm:old.summary([[r['wall_ns']for r in rows[t[arm]['journal']][1:]]for t in tours])for arm in P.BRAS}
  for arm,tail in [('apres',''),('avant_bis','_aa')]:
   xs=[math.log(statistics.median([r['wall_ns']for r in rows[t[arm]['journal']][1:]])/statistics.median([r['wall_ns']for r in rows[t['avant']['journal']][1:]]))for t in tours];stats[name+tail]=old.boot(xs)
 names=report['grandes']['trames'];large={arm:{n:[]for n in names}for arm in P.BRAS}
 for t in report['grandes']['tours']:
  for arm in P.BRAS:
   warm=rows[t[arm]['journal']][len(names):];need(len(warm)==len(names),'complete warm large round')
   for n,row in zip(t['ordre'],warm):large[arm][n].append(row['wall_ns'])
 for arm,tail in [('apres',''),('avant_bis','_aa')]:
  logs=[sum(math.log(large[arm][n][t]/large['avant'][n][t])for n in report['grandes']['tours'][t]['ordre'])/len(names)for t in range(expected['tours_grandes'])];stats['grandes'+tail]=old.boot(logs)
 for key,value in stats.items():need(value==judgment['cas']['statistiques'][key],'independent bootstrap '+key)
 large_summary={}
 for arm,byname in large.items():
  med=[statistics.median(v)for v in byname.values()];flat=[v for vs in byname.values()for v in vs]
  large_summary[arm]=dict(frames=len(names),warm_passes=len(flat),median_frame_medians_ns=statistics.median(med),median_pooled_ns=statistics.median(flat),max_frame_median_ns=max(med),max_warm_ns=max(flat),frames_median_over_100ms=sum(x>100000000 for x in med),passes_over_100ms=sum(x>100000000 for x in flat),per_frame_median_ns={n:statistics.median(v)for n,v in byname.items()})
 bins={arm:b['sha256']for arm,b in report['construction']['binaires'].items()};need(bins==report['ng']['binaires_apres']and bins['avant']==bins['avant_bis'],'ELF closure/AA')
 need(report['construction']['archive_avant_sha256']==meta['source_packages']['before']['archive']['sha256'],'built baseline archive')
 for b in report['construction']['binaires'].values():need(set(['CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21','MHGP12_ENABLE_CUDA:BOOL=ON'])<=set(b['cmake']),'native build profile')
 ident=report['identite']['prises'];need(sum(len(rows[p['journal']])for g in ident.values()for p in g.values())==100,'identity full passes')
 hashes={k:{arm:p['resume']['empreintes']for arm,p in group.items()}for k,group in ident.items()}
 identity_ok=all((g['avant']==g['apres']) if k=='v12set_k5' else len({x for v in g.values() for x in v})==1 for k,g in hashes.items())
 need(identity_ok==judgment['identite_ok'],'independent FUL1 identity')
 aa_bad=any(abs(stats[n]['moyenne_geometrique']-1)>0.015 for n in ['grandes_aa','ng00_aa','ng01_aa','ng02_aa'])
 env_bad=any(report.get('environnement',{}).get(n,{}).get('gpu_apps')!='' for n in ['avant','apres'])
 predicted='refuse' if aa_bad or env_bad else 'adopte' if identity_ok and stats['grandes']['ic95'][1]<0.95 and all(stats[n]['ic95'][1]<1.01 for n in P.TRAMES) else 'rejete'
 need(predicted==judgment['verdict'],'independent preannounced decision')
 cold_k10={n:{arm:rows[ident[n+'_k10'][arm]['journal']][0]['wall_ns']for arm in ['avant','apres']}for n in P.TRAMES}
 need(report['decision_chaine']==P.decision_chaine(expected),'chain diagnostic')
 return dict(chain_declared_frames=len(report['decision_chaine']),chain_declared_engaged=sum(d['chaine'] for d in report['decision_chaine'].values()),cohort_admitted=True,processes=85,full_passes=1306,decisive_warm_passes=783,identity_passes=100,verdict=judgment['verdict'],judgment=judgment,worker_float_differences=diffs,absolute_ng=absolute,large=large_summary,k10_identity_cold_only_ns=cold_k10,elf=bins,identity_sha256=hashlib.sha256(json.dumps(hashes,sort_keys=True).encode()).hexdigest())
