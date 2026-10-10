#!/usr/bin/env python3
"""Recompute bounded B3b profile summaries from four metadata-only JSONL logs."""
import hashlib,json,statistics,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
NAMES=('trace','sonde','proposition','t1','certificat','repli','census_sature','census_complet','pas','arret')
def need(ok,why):
 if not ok:raise ValueError(why)
def digest(b):return hashlib.sha256(b).hexdigest()
def run(root):
 hashes={};out={};counts={}
 for frame in ('ng00','kitti_ng_02_001606'):
  out[frame]={}
  for arm in ('avant','apres'):
   name=frame+'/'+arm+'.jsonl';raw=(root/name).read_bytes()
   need(raw.endswith(b'\n'),'closed JSONL');hashes[name]=dict(bytes=len(raw),sha256=digest(raw))
   rows=[json.loads(line)for line in raw.splitlines()];tour={};prof={};orders={};exits=[]
   for row in rows:
    if row['phase']=='tour_g':
     p=row['pass'];need(p not in tour,'duplicate pass');tour[p]=row
     need(row['status']=='ok'and row['threads']==48 and row['coord_bits']==21 and row['kmax']==5,'profile configuration')
    elif row['phase']=='ordre':
     need(row['k']not in orders,'duplicate final order');orders[row['k']]=row
    elif row['phase']=='exit':
     exits.append(row);need(row['status']=='ok'and row['reason']=='none','native status')
    else:
     need(row['phase']=='profil_g','unknown phase');key=(row['pass'],row['k']);need(key not in prof,'duplicate order');prof[key]=row
     need(set(row['sections'])==set(NAMES)|{'total'},'section cohort')
     need(all(type(s['cycles'])is int and s['cycles']>=0 and type(s['n'])is int and s['n']>=0 for s in row['sections'].values()),'cycles/counts')
     need(sum(row['sections'][n]['cycles']for n in NAMES)+row['reste']['cycles']==row['sections']['total']['cycles'],'cycle partition')
     need(row['ghz_tsc']>0,'recorded clock rate')
   need(set(tour)==set(range(4))and set(prof)=={(p,k)for p in range(4)for k in range(2,6)},'four complete passes')
   need(set(orders)==set(range(1,6))and len(exits)==1 and rows[-1]==exits[0],'orders and final status')
   for k in range(2,6):
    for pas in range(4):
     need(prof[pas,k]['sections']['trace']['n']==orders[k]['objet']['representatives'],'trace count')
     need(prof[pas,k]['sections']['sonde']['n']==orders[k]['travail']['probes'],'probe count')
   counts[frame,arm]={(p,k):{n:prof[p,k]['sections'][n]['n']for n in NAMES}for p in range(4)for k in range(2,6)}
   med=statistics.median;hot=range(1,4);r={}
   r['wall_ms']=med(tour[p]['wall_ns']/1e6 for p in hot)
   r['resolve_ms']=med(tour[p]['diagnostics']['resolve_ns']/1e6 for p in hot)
   r['sections']={}
   for n in NAMES:
    times=[sum(prof[p,k]['sections'][n]['cycles']/prof[p,k]['ghz_tsc']/1e6 for k in range(2,6))for p in hot]
    shares=[sum(prof[p,k]['sections'][n]['cycles']for k in range(2,6))/sum(prof[p,k]['sections']['total']['cycles']for k in range(2,6))for p in hot]
    r['sections'][n]=dict(thread_ms=med(times),share=med(shares),occurrences=med(sum(prof[p,k]['sections'][n]['n']for k in range(2,6))for p in hot))
   r['orders']={str(k):dict(thread_ms=med(prof[p,k]['sections']['total']['cycles']/prof[p,k]['ghz_tsc']/1e6 for p in hot),share=med(prof[p,k]['sections']['total']['cycles']/sum(prof[p,j]['sections']['total']['cycles']for j in range(2,6))for p in hot))for k in range(2,6)}
   r['work_by_order']={str(k):dict(representatives=orders[k]['objet']['representatives'],cells=orders[k]['objet']['cells'],extended_cells=orders[k]['objet']['extended_cells'],first_probe_hits=orders[k]['travail']['first_probe_hits'],probes=orders[k]['travail']['probes'])for k in range(2,6)}
   reps=sum(orders[k]['objet']['representatives']for k in range(2,6));hits=sum(orders[k]['travail']['first_probe_hits']for k in range(2,6))
   r['first_probe_hit_fraction']=hits/reps
   out[frame][arm]=r
  need(counts[frame,'avant']==counts[frame,'apres'],'unchanged discrete profile occurrences')
 return dict(logs=hashes,processes=4,passes=16,hot_passes=12,threads=48,after_is_full_B3_lot_not_adopted_keys=True,summary=out)
if __name__=='__main__':
 need(len(sys.argv)in(2,3),'check.py PROFILE_LOG_ROOT [--record]');r=run(Path(sys.argv[1]))
 if len(sys.argv)==3:
  need(sys.argv[2]=='--record','record option');(HERE/'capture.json').write_text(json.dumps(r,indent=2)+'\n')
 else:need(r==json.loads((HERE/'capture.json').read_text()),'immutable capture')
 print(json.dumps(dict(status='ok',processes=r['processes'],hot_passes=r['hot_passes'],same_counts=True,after='full B3 lot, not keys alone',native_runs=0)))
