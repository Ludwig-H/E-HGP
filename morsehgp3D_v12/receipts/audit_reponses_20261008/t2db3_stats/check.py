#!/usr/bin/env python3
"""T2dB3 : statistiques indépendantes depuis 300 JSONL, pas d'admission des identités."""
import argparse,hashlib,json,math,random,statistics,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
FRAMES=('ng00','ng01','ng02','kitti_ng_02_001606','kitti_ng_08_001176')
SITES=(39885,35551,45845,64740,67114)
ARMS=('avant','avant_bis','cles','balayage','transfert','apres')
PAIRS=(('lot_b3','avant','apres'),('cles','avant','cles'),('balayage','avant','balayage'),('transfert','avant','transfert'),('cles_apres_transfert','transfert','cles'),('balayage_apres_cles','cles','apres'),('A/A','avant','avant_bis'))
M=statistics.median

def need(ok,why):
 if not ok:raise ValueError(why)
def unique(pairs):
 d={}
 for k,v in pairs:need(k not in d,'duplicate key');d[k]=v
 return d
def reject(x):raise ValueError('nonfinite '+x)
def load(p):return json.loads(p.read_text(),object_pairs_hook=unique,parse_constant=reject)
def sha(b):return hashlib.sha256(b).hexdigest()
def manifest(raw):
 paths=sorted(raw.rglob('*.jsonl'));rows=[[p.relative_to(raw).as_posix(),p.stat().st_size,sha(p.read_bytes())]for p in paths]
 return rows,sha(json.dumps(rows,separators=(',',':')).encode())
def fields(p):
 r={'FULL':p['wall_ns'],**{k:p['etapes_ns'][k]for k in ('P','C','G','TMVR')},'transfer':p['c_ns']['transferts'],'open':p['g_ns']['ouverture'],'tables':p['g_ns']['tables']}
 r['rest']=r['FULL']-sum(r[k]for k in ('P','C','G','TMVR'));need(r['rest']>=0,'wall partition');return r

def run(raw,repo):
 cap=load(HERE/'capture.json');rows,h=manifest(raw);need(h==cap['journal_manifest_sha256']and len(rows)==cap['files']==300 and sum(r[1]for r in rows)==cap['raw_bytes'],'raw manifest')
 expected={f'{f}/{a}_t{i:02}.jsonl'for f in FRAMES for a in ARMS for i in range(10)};need({r[0]for r in rows}==expected,'cohort')
 for name,h in cap['sources']:need(sha(subprocess.check_output(['git','show',cap['base_git']+':'+name],cwd=repo))==h,'source '+name)
 rng=random.Random(20261008);out={'scope':'statistics only; identity/provenance admission separate','processes':300,'full_passes':2400,'hot_passes':2100,'rule':{'seed':20261008,'bootstrap':10000,'AA_window':0.015,'all_upper_CI_strictly_below':1.0,'passes':8,'warm_start':1,'processes_per_arm_frame':10,'percentile_indices':[250,9749]},'arms':ARMS,'frames':{}}
 for frame,sites in zip(FRAMES,SITES):
  samples={};med={}
  for arm in ARMS:
   samples[arm]=[]
   for i in range(10):
    lines=[json.loads(s,object_pairs_hook=unique,parse_constant=reject)for s in (raw/frame/f'{arm}_t{i:02}.jsonl').read_text(encoding='ascii').splitlines()]
    need(len(lines)==18 and lines[-1]==dict(phase='exit',status='ok',reason='none'),'sequence end')
    need(lines[0]['phase']=='open' and lines[0]['status']=='ok' and lines[0]['budget_appareil']=='partage','open')
    ps=[]
    for j in range(8):
     p,free=lines[1+2*j:3+2*j];need(p['phase']=='full' and free['phase']=='liberation' and p['pass']==free['pass']==j,'pass order')
     need(p['trame']==frame and p['sites']==sites and p['voie']=='device' and p['status']=='ok' and p['coord_bits']==21 and p['kmax']==5 and p['threads']==48 and p['etapes_schema']=='recouvert'and'full_sha256'not in p,'configuration')
     need(type(p['wall_ns'])is int and p['wall_ns']>0 and p['etapes_ns']['TMVR']==p['recouvrement']['queue_ns'],'wall');fields(p);ps.append(p)
    samples[arm].append(ps[1:])
   med[arm]=[M(p['wall_ns']for p in ps)for ps in samples[arm]]
  d={'sites':sites,'process_medians_ns':[med[a]for a in ARMS],'arm_medians_ns':{a:M(med[a])for a in ARMS},'comparisons':{}}
  for name,a,b in PAIRS:
   logs=[math.log(y/x)for x,y in zip(med[a],med[b])];boot=sorted(sum(logs[rng.randrange(10)]for _ in range(10))/10 for _ in range(10000));gm=math.exp(sum(logs)/10);ci=[math.exp(boot[250]),math.exp(boot[9749])]
   delta={k:0 for k in fields(samples[a][0][0])}
   for xs,ys in zip(samples[a],samples[b]):
    for x,y in zip(xs,ys):
     xf,yf=fields(x),fields(y)
     for k in delta:delta[k]+=yf[k]-xf[k]
   need(delta['FULL']==sum(delta[k]for k in ('P','C','G','TMVR','rest')),'additive paired delta')
   d['comparisons'][name]={'GM':gm,'CI95':ci,'paired_rounds_b_faster':sum(y<x for x,y in zip(med[a],med[b])),'paired_sum_delta_ns':delta,'paired_passes':70}
  out['frames'][frame]=d
 aa=[d['comparisons']['A/A']['GM']for d in out['frames'].values()];out['AA']={'GMs':aa,'max_abs_deviation':max(abs(x-1)for x in aa),'passes':all(abs(x-1)<=.015 for x in aa)}
 out['statistical_rule']={name:{'fails_on':[frame for frame,d in out['frames'].items()if d['comparisons'][name]['CI95'][1]>=1.0],'all_frames_pass':all(d['comparisons'][name]['CI95'][1]<1.0 for d in out['frames'].values())}for name in ('lot_b3','cles','balayage')}
 need(manifest(raw)[1]==cap['journal_manifest_sha256'],'closing manifest');return out
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--raw',type=Path,required=True);p.add_argument('--repo',type=Path,required=True);p.add_argument('--write',action='store_true');a=p.parse_args();r=run(a.raw,a.repo)
 if a.write:(HERE/'results.json').write_text(json.dumps(r,separators=(',',':'))+'\n')
 else:need(json.loads(json.dumps(r))==load(HERE/'results.json'),'results changed')
 print(json.dumps({'status':'ok','AA':r['AA'],'statistical_rule':r['statistical_rule']}))
