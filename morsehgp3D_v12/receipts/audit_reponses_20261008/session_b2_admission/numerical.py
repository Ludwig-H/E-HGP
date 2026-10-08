#!/usr/bin/env python3
"""Exact external cohort and all 373 JSONL; source judge plus independent absolute statistics."""
import json,hashlib,importlib.util,statistics,sys,math,random
from pathlib import Path
sys.dont_write_bytecode=True

def run(REPO,SESSION,HERE):
 ROOT=HERE/'returned'
 def need(v,m):
  if not v:raise ValueError(m)
 def imp(p,n):
  s=importlib.util.spec_from_file_location(n,p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
 p=imp(HERE/'source/microbancs/mes_t2d_b2/pilote_t2d_b2.py','b2_pilot')
 r=json.loads((ROOT/'rapport_t2d_b2.json').read_text())
 conf=json.loads((SESSION/'package/plan.json').read_text())['commands'][1]['argv']
 for k,v in [('--fils','48'),('--passes','8'),('--processus','10')]:need(conf.count(k)==1 and conf[conf.index(k)+1]==v,'external config')
 manifest=json.loads((REPO/'morsehgp3D_v12/receipts/audit_reponses_20261008/session_t2da_admission/capture.json').read_text());sites={'ng00':39885,'ng01':35551,'ng02':45845,**{c['name']:c['sites'] for c in manifest['cohort']}}
 expected={}
 def add(g,frames,arms,turns,k,P,digest,schema='recouvert'):
  for t in frames:
   for b in arms:
    for i in range(turns):
     name=b+('_t%02d'%i if g in ('k5','grande','sequentiel') else '')+'.jsonl';rel='journaux/'+g+'/'+t+'/'+name
     expected[rel]=(g,t,b,i,p.attendu(t,sites[t],k,48,P,digest,schema))
 add('identite',p.TRAMES,p.BRAS_CONSTRUITS,1,5,2,True)
 add('k5',p.TRAMES,p.BRAS_JUGES,10,5,8,False)
 add('grande',[p.GRANDE],p.BRAS_JUGES,3,5,8,False)
 add('k10',p.LIDAR,['avant','apres'],1,10,5,False)
 add('sequentiel',p.SEQ_TRAMES,p.SEQ_BRAS,3,5,8,False,'sequentiel')
 records={}
 def walk(x):
  if isinstance(x,dict):
   if 'journal' in x:
    need(x['journal'] not in records,'duplicate report journal');records[x['journal']]=x
   else:
    for v in x.values():walk(v)
  elif isinstance(x,list):
   for v in x:walk(v)
 walk(r)
 need(records.keys()==expected.keys(),'external cohort mismatch')
 need({str(f.relative_to(ROOT)) for f in (ROOT/'journaux').rglob('*.jsonl')}==set(expected),'archive extra/missing journal')
 need(r['campagne_k5']['fils']==48 and r['campagne_k5']['passes']==8 and r['campagne_k5']['tours_demandes']==10,'configuration')
 rows={};pins={};ident={}
 for name,(group,frame,arm,tour,e) in expected.items():
  take=records[name];raw=(ROOT/name).read_bytes();sha=hashlib.sha256(raw).hexdigest();need(sha==take['journal_sha256'],'hash '+name)
  need(type(take['code']) is int and take['code']==0 and take['valide'] is True and take['etat']=='ok','take status')
  lu=p.lf.parse_output(take['code'],raw.decode('ascii'),e);need(lu['etat']=='ok','LF '+name+' '+lu['raison']);rr=lu['passes'];rows[name]=rr;pins[name]={'sha256':sha,'bytes':len(raw)}
  if group=='sequentiel':
   summary={'murs_ns':[v['wall_ns'] for v in rr],'g_ns':[v['etapes_ns']['G'] for v in rr],'tables_ns':[v['g_ns']['tables'] for v in rr],'resolution_ns':[v['g_ns']['resolution'] for v in rr]};summary['g_median_ns']=statistics.median(summary['g_ns'][1:])
  else:summary=p.resume(rr)
  summary['mur_ns']=statistics.median(summary['murs_ns'][1:])
  need(all(take[k]==v for k,v in summary.items()),'raw vs report '+name)
  if group=='identite':ident.setdefault(frame,set()).update(v['full_sha256'] for v in rr)
 need(all(len(v)==1 for v in ident.values()),'FUL1 discrepancy')
 need(ident['ng00']=={p.FUL1_NG00_K5},'FUL1 reference')
 shas={a:r['construction']['binaires'][a]['sha256'] for a in p.BRAS_JUGES}
 need(shas['avant']==shas['avant_bis'] and r['campagne_k5']['binaires_apres']==shas,'ELF final/A-A')
 for v in shas.values():need(type(v)is str and len(v)==64 and set(v)<=set('0123456789abcdef'),'ELF syntax')
 need(all(r['environnement'][x]['gpu_apps']=='' for x in ('avant','apres')),'GPU isolation decisive')
 need(r['informations']['environnement_fin']['gpu_apps']=='','GPU isolation information')
 judged=p.juger(r,str(ROOT),verifier_journaux=True)
 rng=random.Random(20261008)
 for frame in p.TRAMES:
  for label,before,after in p.LEVIERS+p.INFORMATIFS+(p.CONTROLE_AA,):
   logs=[]
   for tour in range(10):
    a=rows['journaux/k5/'+frame+'/'+after+'_t%02d.jsonl'%tour][1:]
    b=rows['journaux/k5/'+frame+'/'+before+'_t%02d.jsonl'%tour][1:]
    logs.append(math.log(statistics.median(x['wall_ns'] for x in a)/statistics.median(x['wall_ns'] for x in b)))
   boot=sorted(sum(logs[rng.randrange(10)] for _ in range(10))/10 for _ in range(10000))
   stat={'moyenne_geometrique':math.exp(sum(logs)/10),'ic95':[math.exp(boot[250]),math.exp(boot[9749])],'rapports':[math.exp(x) for x in logs]}
   need(stat==judged['cas'][frame][label],'raw bootstrap discrepancy')
 for lines in r['construction']['cmake'].values():
  need(all(x in lines for x in ['CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21','MHGP12_ENABLE_CUDA:BOOL=ON']),'build configuration')
 def differences(a,b,path=''):
  if isinstance(a,tuple):a=list(a)
  if isinstance(b,tuple):b=list(b)
  if type(a)!=type(b):return [(path,a,b)]
  if isinstance(a,dict):
   need(a.keys()==b.keys(),'judgment keys');return sum((differences(a[k],b[k],path+'/'+k) for k in a),[])
  if isinstance(a,(list,tuple)):
   need(len(a)==len(b),'judgment count');return sum((differences(x,y,path+'/'+str(i)) for i,(x,y) in enumerate(zip(a,b))),[])
  return [] if a==b else [(path,a,b)]
 def stat(group,frame,arm):
  rr=[v[1:] for n,v in rows.items() if expected[n][:3]==(group,frame,arm)]
  warm=[v for a in rr for v in a];meds=[statistics.median(v['wall_ns'] for v in a) for a in rr]
  return {'processes':len(rr),'warm_full':len(warm),'median_process_medians_ns':statistics.median(meds),'pooled_median_ns':statistics.median(v['wall_ns'] for v in warm),'max_process_median_ns':max(meds),'max_warm_ns':max(v['wall_ns'] for v in warm),'warm_over_100ms':sum(v['wall_ns']>100000000 for v in warm),'stage_medians_not_additive_ns':{k:statistics.median(v['etapes_ns'][k] for v in warm) for k in ['P','C','G','TMVR']},'cpu_ns_median':statistics.median(v['cpu_ns'] for v in warm),'rss_lifetime_max_bytes':max(v['rss_max_octets'] for v in warm),'active_budget_peak_max_bytes':max(v['pic_octets'] for v in warm),'device_capacity_max_bytes':max(v['appareil_octets'] for v in warm),'device_peak_max_bytes':max(v['pic_appareil_octets'] for v in warm),'pinned_capacity_max_bytes':max(v['epinglee_octets'] for v in warm)}
 out={'source':'4171b2653fa5cb903102a3fc44572bcb3330b3e8','processes':len(rows),'full_passes':sum(map(len,rows.values())),'decisive_warm':sum(len(v)-1 for n,v in rows.items() if expected[n][0]=='k5'),'judgment':judged,'worker_differences':differences(judged,r['jugement']),'identity':{k:sorted(v) for k,v in ident.items()},'elf_initial':shas,'elf_after_decisive':r['campagne_k5']['binaires_apres'],'journal_pins':pins,'k5':{f:{a:stat('k5',f,a) for a in p.BRAS_JUGES} for f in p.TRAMES},'k10':{f:{a:stat('k10',f,a) for a in ('avant','apres')} for f in p.LIDAR},'grande':{a:stat('grande',p.GRANDE,a) for a in p.BRAS_JUGES},'sequentiel':{f:{a:stat('sequentiel',f,a) for a in p.SEQ_BRAS} for f in p.SEQ_TRAMES}}
 out['journal_inventory_sha256']=hashlib.sha256(json.dumps(out.pop('journal_pins'),sort_keys=True).encode()).hexdigest()
 out_judgment=out.pop('judgment')
 out['verdicts']=out_judgment['verdicts'];out['refus']=out_judgment['refus'];out['controle_aa']=out_judgment['controle_aa']
 out['statistics']={t:{k:{x:v[x] for x in ['moyenne_geometrique','ic95']} for k,v in d.items() if k in ['lot_b2','tables','relecture','census','A/A']} for t,d in out_judgment['cas'].items()}
 out['k5']={k:{a:v[a] for a in ['avant','apres']} for k,v in out['k5'].items()}
 out['grande']={a:out['grande'][a] for a in ['avant','apres']}
 out.pop('sequentiel');out['sequentiel_processes_admitted']=24
 out['warm_information']=318
 return out
