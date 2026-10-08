#!/usr/bin/env python3
"""FULLN: extraction indépendante des JSONL publics; aucun moteur ni payload."""
import argparse,collections,hashlib,json,re,statistics,subprocess
from pathlib import Path
from fractions import Fraction
HERE=Path(__file__).resolve().parent

def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def obj(pairs):
 out={}
 for k,v in pairs:
  need(k not in out,'duplicate JSON key');out[k]=v
 return out
def median(xs):return statistics.median(xs)
def load(p):return json.loads(p.read_text(),object_pairs_hook=obj)

def run(raw,repo):
 cap=load(HERE/'capture.json');pins={n:(size,h)for n,size,h in cap['raw_files']}
 need(set(p.name for p in raw.glob('*.jsonl'))==set(pins),'raw cohort')
 for name,h in cap['sources']:
  need(sha(subprocess.check_output(['git','show',cap['base_git']+':'+name],cwd=repo))==h,'source '+name)
 report_path,h=cap['public_report'];need(sha((repo/report_path).read_bytes())==h,'public report pin');report=load(repo/report_path)
 groups=collections.defaultdict(lambda:collections.defaultdict(list));allrows=[];cold_open=[];free={}
 for name,(size,h) in pins.items():
  data=(raw/name).read_bytes();need(len(data)==size and sha(data)==h,'raw pin '+name)
  rows=[json.loads(line,object_pairs_hook=obj)for line in data.decode('ascii').splitlines()]
  need(rows[-1]==dict(phase='exit',status='ok',reason='none'),'exit '+name)
  full=[r for r in rows if r['phase']=='full'];lib=[r for r in rows if r['phase']=='liberation'];seq=name.startswith('v12set_')
  group='v12set_k5_appareil' if seq else 'k5_cpu' if name.startswith('cpu_') else 'k10_appareil' if name.startswith('k10_') else 'k5_appareil'
  count=74 if seq else 10 if group=='k5_appareil' else 5
  need(len(full)==len(lib)==count,'passes '+name)
  device=group!='k5_cpu';need(len(rows)==2*count+1+int(device),'rows '+name)
  if device:
   need(rows[0]['phase']=='open' and rows[0]['status']=='ok' and rows[0]['budget_appareil']=='partage','open '+name);cold_open.append(rows[0]['wall_ns'])
  body=rows[int(device):-1]
  for i,p in enumerate(full):
   need(body[2*i]==p and body[2*i+1]==lib[i] and p['pass']==lib[i]['pass']==i,'order '+name)
   need(p['status']=='ok' and p['voie']==('device' if device else 'cpu') and p['coord_bits']==21 and p['threads']==48 and p['kmax']==(10 if group=='k10_appareil' else 5) and p['etapes_schema']=='recouvert','configuration '+name)
   need(type(p['wall_ns']) is int and p['wall_ns']>0 and re.fullmatch('[0-9a-f]{64}',p['full_sha256']),'wall/digest')
   e=p['etapes_ns'];need(e['raccord']==0 and sum(e.values())<=p['wall_ns'] and e['TMVR']==p['recouvrement']['queue_ns'],'partition')
   allrows.append(p);free[id(p)]=lib[i]['liberation_ns']
  if seq:
   order=[p['trame']for p in full[:37]];need(len(set(order))==37 and order==[p['trame']for p in full[37:]],'sequence')
   for i,n in enumerate(order):groups[group][n].append([full[i],full[i+37]])
  else:need(len({p['trame']for p in full})==1,'single frame');groups[group][full[0]['trame']].append(full)
 stats={};warm={}
 for group,frames in groups.items():
  stats[group]={};warm[group]={}
  for frame,runs in frames.items():
   ps=[p for run in runs for p in run[1:]];warm[group][frame]=ps
   need(len(runs)==(5 if group in ('k5_appareil','v12set_k5_appareil')else 3),'process count')
   need(len({p['full_sha256']for run in runs for p in run})==1,'digest consistency')
   d=dict(mediane_ns=median(p['wall_ns']for p in ps),max_medianes_ns=max(median(p['wall_ns']for p in run[1:])for run in runs),max_ns=max(p['wall_ns']for p in ps),premiere_ns=median(run[0]['wall_ns']for run in runs),etapes_ns={s:median(p['etapes_ns'][s]for p in ps)for s in ps[0]['etapes_ns']},c_ns={s:median(p['c_ns'][s]for p in ps)for s in ps[0]['c_ns']},cpu_ns=median(p['cpu_ns']for p in ps),pic_octets=max(p['pic_octets']for p in ps),sites=ps[0]['sites'],valeurs=len(ps))
   need(d==report['statistiques'][group][frame],'published statistics '+group+'/'+frame);stats[group][frame]=d
 need(len(stats['v12set_k5_appareil'])==37 and all(set(stats[g])=={'ng00','ng01','ng02'}for g in stats if g!='v12set_k5_appareil'),'frame cohort')
 for n in ('ng00','ng01','ng02'):
  digest=groups['k5_appareil'][n][0][0]['full_sha256']
  need(digest==groups['k5_cpu'][n][0][0]['full_sha256'],'CPU GPU identity')
 out={'processes':len(pins),'full_passes':len(allrows),'retained_passes':sum(len(ps)for fs in warm.values()for ps in fs.values()),'ng_columns':['frame','sites','hot_median_ns','max_process_median_ns','max_hot_ns','first_median_ns','P_median_ns','C_median_ns','G_median_ns','queue_median_ns','cpu_median_ns'],'ng':{}}
 for group in ('k5_appareil','k5_cpu','k10_appareil'):
  out['ng'][group]=[[n,d['sites'],d['mediane_ns'],d['max_medianes_ns'],d['max_ns'],d['premiere_ns'],*[d['etapes_ns'][s]for s in ('P','C','G','TMVR')],d['cpu_ns']]for n,d in sorted(stats[group].items())]
 v=stats['v12set_k5_appareil'];out['v12set_columns']=['frame','sites','hot_median_ns','max_hot_ns','first_visit_median_ns'];out['v12set']=[[n,d['sites'],d['mediane_ns'],d['max_ns'],d['premiere_ns']]for n,d in sorted(v.items())]
 out['summary']=dict(v12set_median_ns=median(d['mediane_ns']for d in v.values()),worst_frame_median_ns=max(d['mediane_ns']for d in v.values()),maximum_judged_ns=max(d['max_medianes_ns']for d in v.values()),frames_below100ms=sum(d['mediane_ns']<100000000 for d in v.values()),frames_all_hot_below100ms=sum(d['max_ns']<100000000 for d in v.values()),verdict='non tenu')
 out['focus']={}
 for frame in ('kitti_ng_00_003624','kitti_ng_00_001896','kitti_ng_08_002119'):
  ps=warm['v12set_k5_appareil'][frame];noq=[p['wall_ns']-p['etapes_ns']['TMVR']for p in ps]
  sums={'wall':sum(p['wall_ns']for p in ps),**{s:sum(p['etapes_ns'][s]for p in ps)for s in ('P','C','G','TMVR')}};sums['residual']=sums['wall']-sum(sums[s]for s in ('P','C','G','TMVR'))
  out['focus'][frame]=dict(count=len(ps),stage_medians_ns=stats['v12set_k5_appareil'][frame]['etapes_ns'],transfer_median_ns=median(p['c_ns']['transferts']for p in ps),sums_ns=sums,fixed_rest_zero_queue_median_ns=median(noq),fixed_rest_zero_queue_max_ns=max(noq),fixed_rest_zero_queue_above100ms=sum(x>100000000 for x in noq),excluded_medians_ns={'validation':median(p['hors_mur_ns']['validation']for p in ps),'digest':median(p['hors_mur_ns']['empreinte']for p in ps),'free':median(free[id(p)]for p in ps)})
 out['cpu_catalogue_share']={}
 for frame,ps in warm['k5_cpu'].items():
  cs=[p['etapes_ns']['C']for p in ps];walls=[p['wall_ns']for p in ps];ratios=[Fraction(c,w)for c,w in zip(cs,walls)]
  need(all(c>100000000 for c in cs),'CPU C exceeds100ms on each warm pass')
  out['cpu_catalogue_share'][frame]=dict(count=len(ps),C_min_ns=min(cs),C_max_ns=max(cs),C_median_ns=median(cs),wall_median_ns=median(walls),C_sum_ns=sum(cs),wall_sum_ns=sum(walls),all_C_gt100ms=True,mean_pass_ratios=float(sum(ratios)/len(ratios)),ratio_of_sums=float(Fraction(sum(cs),sum(walls))),median_pass_ratios=float(median(ratios)),ratio_of_medians=median(cs)/median(walls))
 out['summary']['fixed_rest_zero_queue_frames_below100ms']=sum(median(p['wall_ns']-p['etapes_ns']['TMVR']for p in ps)<100000000 for ps in warm['v12set_k5_appareil'].values())
 for name,(size,h) in pins.items():need(sha((raw/name).read_bytes())==h,'closing pin '+name)
 return out
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--raw',type=Path,required=True);a.add_argument('--repo',type=Path,required=True);a.add_argument('--write',action='store_true');x=a.parse_args();r=run(x.raw,x.repo)
 if x.write:(HERE/'results.json').write_text(json.dumps(r,separators=(',',':'))+'\n')
 else:need(r==load(HERE/'results.json'),'results changed')
 print(json.dumps({'status':'ok','processes':r['processes'],'full_passes':r['full_passes'],'retained_passes':r['retained_passes'],'summary':r['summary']}))
