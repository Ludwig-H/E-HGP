#!/usr/bin/env python3
"""FULL O: relecture des temps et decomposition, sans moteur ni payload."""
import argparse,collections,hashlib,importlib.util,json,statistics,tempfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
M=statistics.median

def need(ok,why):
 if not ok:raise ValueError(why)
def load(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def reader(repo,cap,key):
 p,h=cap['readers'][key];need(sha(repo/p)==h,'reader pin '+key)
 spec=importlib.util.spec_from_file_location('pinned_'+key,repo/p);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def hot(raw):
 result=collections.defaultdict(lambda:collections.defaultdict(list))
 for p in sorted(raw.glob('*.jsonl')):
  ps=[r for r in map(json.loads,p.read_text().splitlines())if r['phase']=='full']
  group='v12set_k5_appareil'if p.name.startswith('v12set')else 'k5_cpu'if p.name.startswith('cpu')else 'k10_appareil'if p.name.startswith('k10')else 'k5_appareil'
  for r in ps[37 if group.startswith('v12set')else 1:]:result[group][r['trame']].append(r)
 return result

def sums(ps):
 out={'wall':sum(p['wall_ns']for p in ps)}
 for s in ('P','C','G','TMVR'):out[s]=sum(p['etapes_ns'][s]for p in ps)
 out['residual']=out['wall']-sum(out[s]for s in ('P','C','G','TMVR'));need(out['residual']>=0,'partition')
 return out

def focus(ps):
 ss=sums(ps);k=ps[0]['kmax'];last=collections.Counter();after=collections.Counter();before=0;g_last=collections.Counter();closed=[];crit=[]
 for p in ps:
  ends=p['fins_par_ordre_ns'];need(len(ends)==k and all(len(x)==5 for x in ends),'ends schema')
  g=p['recouvrement']['fin_g_ns'];end=p['recouvrement']['fin_ns'];need(g==p['etapes_ns']['G']and end-g==p['etapes_ns']['TMVR'],'internal partition')
  rmax=max(e[4]for e in ends);need(rmax<=end and max(e[0]for e in ends)==g,'end bounds')
  last[str(1+max(range(k),key=lambda i:ends[i][4]))]+=1;g_last[str(1+max(range(k),key=lambda i:ends[i][0]))]+=1
  after[str(sum(e[4]>g for e in ends))]+=1;before+=sum(e[4]<=g for e in ends)
  closed.append(end-rmax);crit.append(max(0,max(e[1]for e in ends)-g))
 return {'hot_passes':len(ps),'sums_ns':ss,'g_open_sum_ns':sum(p['g_ns']['ouverture']for p in ps),'g_tables_sum_ns':sum(p['g_ns']['tables']for p in ps),'last_R_order_counts':dict(sorted(last.items())),'last_G_order_counts':dict(sorted(g_last.items())),'R_after_G_count_distribution':dict(sorted(after.items())),'orders_R_before_G_total':before,'R_after_G_total':len(ps)*k-before,'R_to_internal_end_sum_ns':sum(closed),'latest_kernel_after_G_sum_ns':sum(crit),'fixed_rest_zero_queue_median_ns':M(p['wall_ns']-p['etapes_ns']['TMVR']for p in ps),'fixed_rest_zero_queue_max_ns':max(p['wall_ns']-p['etapes_ns']['TMVR']for p in ps),'fixed_rest_zero_G_and_queue_median_ns':M(p['wall_ns']-p['etapes_ns']['TMVR']-p['etapes_ns']['G']for p in ps)}

def small_all(raw,public_repo,cap,repo):
 meta=load(repo/cap['petits']['public_metadata'][0]);names={r[1]:r[0]for r in meta['cases']}
 raw=raw/'002_mes_c/files/c/brut';c=cap['small_all'];p,h=c['public_report'];need(sha(public_repo/p)==h,'small public report');report=load(public_repo/p);need(all(report['criteres'][k]['etat']=='non tenu'for k in ('C1','C2','C3')),'small criteria')
 need(set(p.name for p in raw.glob('*.jsonl'))=={n for n,h in c['raw']},'small cohort')
 configs={};counts=collections.Counter();rejected=[]
 for n,h in c['raw']:
  p=raw/n;need(sha(p)==h,'small raw pin');rows=list(map(json.loads,p.read_text().splitlines()));ps=[r for r in rows if r['phase']=='full'];exit=rows[-1];need(exit['phase']=='exit','small exit')
  counts['processes']+=1;counts['full_passes']+=len(ps)
  if exit['status']!='ok':
   need(not ps and exit==dict(phase='exit',status='unsupported_degeneracy',reason='wide_leaf'),'expected rejection');rejected.append(n);counts['rejected']+=1;continue
  need(exit==dict(phase='exit',status='ok',reason='none'),'exit success');counts['ok_processes']+=1
  if n.startswith('session_'):
   _,way,k,w=n.removesuffix('.jsonl').split('_');group=way+':'+k+':'+w;values=report['configurations'][group]['valeurs'];by=collections.defaultdict(list)
   for i,r in enumerate(ps):
    need(r['pass']==i and r['status']=='ok' and r['coord_bits']==21 and r['kmax']==int(k) and r['threads']==int(w) and r['voie']==('cpu'if way=='cpu'else'device'),'small configuration');by[names[r['trame']]].append(r)
   need(len(by)==147 and all(len(v)==3 for v in by.values()) and len(ps)==441,'small visits');need(set(by)==set(values),'small frames');meds={}
   for frame,rr in by.items():
    v=values[frame];need(M(r['wall_ns']for r in rr[1:])==v['chaud_ns']and rr[0]['wall_ns']==v['premiere_ns'],'small stats');need({r['full_sha256']for r in rr}==set(v['empreintes']),'small digest');meds[frame]=v['chaud_ns']
   real=[f for f in by if values[f]['groupe']=='reel'];need(len(real)==132,'132 real');counts['hot_passes']+=294
   xs=[values[f]['sites']for f in real];ys=[meds[f]for f in real];den=len(xs)*sum(x*x for x in xs)-sum(xs)**2;beta=(len(xs)*sum(x*y for x,y in zip(xs,ys))-sum(xs)*sum(ys))/den;alpha=(sum(ys)-beta*sum(xs))/len(xs)
   expected=report['configurations'][group]['droites']['reel'];need(abs(alpha-expected['fixe_ns'])<1e-5 and abs(beta-expected['par_site_ns'])<1e-8,'OLS');configs[group]={'real_clouds':132,'real_hot_passes':264,'median_real_cloud_medians_ns':M(meds[f]for f in real),'real_OLS_intercept_ns':alpha,'real_OLS_slope_ns_per_site':beta}
  else:need(len(ps)==2 and all(r['status']=='ok' for r in ps),'hard passes');counts['hot_passes']+=1
  need(sha(p)==h,'small closing pin')
 return {'counts':dict(counts),'configurations':configs,'rejected':rejected,'C1_C2_C3':'non tenu'}

def run(raw,reference_raw,repo,public_repo):
 cap=load(HERE/'capture.json');out={}
 for path,h in cap['references']:need(sha(repo/path)==h,'reference receipt')
 for key in ('full','petits'):
  mod=reader(repo,cap,key)
  with tempfile.TemporaryDirectory(prefix='fullo-time-reader-')as tmp:
   c=json.loads(json.dumps(cap[key]));tmp=Path(tmp)
   if key=='full':c['public_report'][0]=str(public_repo/c['public_report'][0])
   (tmp/'capture.json').write_text(json.dumps(c));mod.HERE=tmp
   out[key]=mod.run(raw/'001_mes_full/files/full/brut'if key=='full'else raw,repo)
  if key=='petits':out[key]['source']=cap['base_git']
 old=reader(repo,cap,'full');old_out=old.run(reference_raw/'001_mes_full/files/full/brut',repo)
 need(old_out==load(repo/cap['references'][0][0]),'reference full statistics')
 newh=hot(raw/'001_mes_full/files/full/brut');oldh=hot(reference_raw/'001_mes_full/files/full/brut')
 for group in newh:
  need(set(newh[group])==set(oldh[group]),'same frame cohort')
  for frame in newh[group]:
   a,b=newh[group][frame],oldh[group][frame];need(len(a)==len(b),'same sample sizes')
   need(len({p['full_sha256']for p in a+b})==1,'cross-session FULL identity')
 out['comparison']={'scope':'descriptive, separate sessions, no paired acceptance test','ng_columns':['frame','old_hot_median_ns','new_hot_median_ns','ratio_new_old'],'ng':{g:[[a[0],a[2],b[2],b[2]/a[2]]for a,b in zip(old_out['ng'][g],out['full']['ng'][g])]for g in old_out['ng']}}
 out['comparison']['v12set']={'old':old_out['summary'],'new':out['full']['summary'],'all37_improved_medians':all(M(p['wall_ns']for p in newh['v12set_k5_appareil'][f])<M(p['wall_ns']for p in oldh['v12set_k5_appareil'][f])for f in newh['v12set_k5_appareil'])}
 out['decomposition']={}
 for group in newh:
  nps=[p for ps in newh[group].values()for p in ps];ops=[p for ps in oldh[group].values()for p in ps]
  a,b=focus(ops),focus(nps);out['decomposition'][group]={'old':a,'new':b,'difference_sums_ns':{s:b['sums_ns'][s]-a['sums_ns'][s]for s in a['sums_ns']},'divisor_for_means':len(nps)}
 seq=newh['v12set_k5_appareil'];out['frames']={f:focus(ps)for f,ps in seq.items()}
 out['capacity']={'zero_queue_frame_median_ns':M(d['fixed_rest_zero_queue_median_ns']for d in out['frames'].values()),'zero_queue_worst_frame_median_ns':max(d['fixed_rest_zero_queue_median_ns']for d in out['frames'].values()),'zero_queue_frames_all_hot_below100ms':sum(d['fixed_rest_zero_queue_max_ns']<100000000 for d in out['frames'].values()),'zero_G_and_queue_frames_median_below100ms':sum(d['fixed_rest_zero_G_and_queue_median_ns']<100000000 for d in out['frames'].values()),'zero_G_and_queue_worst_median_ns':max(d['fixed_rest_zero_G_and_queue_median_ns']for d in out['frames'].values())}
 # Concrete stage lower bound conditional on keeping catalogue and preparation fixed.
 out['capacity']['C_above100ms_passes']=sum(p['etapes_ns']['C']>100000000 for ps in seq.values()for p in ps)
 # Cache warm-up: visits are retained by position, not by imposing a cold label.
 out['cold_columns']=['group','frame','first_median_ns','hot_median_ns']
 out['cold']=[[g,row[0],row[5],row[2]]for g,rows in out['full']['ng'].items()for row in rows]
 out['small_all']=small_all(raw,public_repo,cap,repo)
 out['frames']={f:d for f,d in out['frames'].items()if f in ('kitti_ng_00_003624','kitti_ng_00_001896','kitti_ng_08_002119')}
 return out
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--raw',type=Path,required=True);a.add_argument('--reference-raw',type=Path,required=True);a.add_argument('--repo',type=Path,required=True);a.add_argument('--public-repo',type=Path);a.add_argument('--write',action='store_true');x=a.parse_args();r=run(x.raw,x.reference_raw,x.repo,x.public_repo or x.repo)
 if x.write:(HERE/'results.json').write_text(json.dumps(r,separators=(',',':'))+'\n')
 else:need(r==load(HERE/'results.json'),'results changed')
 print(json.dumps({'status':'ok','processes':r['full']['processes'],'passes':r['full']['full_passes'],'hot':r['full']['retained_passes'],'summary':r['full']['summary'],'capacity':r['capacity']}))
