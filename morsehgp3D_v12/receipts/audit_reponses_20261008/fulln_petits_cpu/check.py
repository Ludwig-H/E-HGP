#!/usr/bin/env python3
"""Diagnostic FULLN K5 sur mêmes nuages; JSON publics, aucun moteur/payload."""
import argparse,collections,hashlib,json,statistics,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
M=statistics.median

def need(ok,why):
 if not ok:raise ValueError(why)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def load(p):return json.loads(p.read_text())
def sums(ps):
 d={'wall':sum(p['wall_ns']for p in ps),**{s:sum(p['etapes_ns'][s]for p in ps)for s in ('P','C','G','TMVR')}}
 d.update({'c_'+s:sum(p['c_ns'][s]for p in ps)for s in ps[0]['c_ns']})
 d['wall_rest']=d['wall']-sum(d[s]for s in ('P','C','G','TMVR'));d['C_rest']=d['C']-sum(d[k]for k in d if k.startswith('c_'))
 need(d['wall_rest']>=0 and d['C_rest']>=0,'nonnegative residual');return d

def run(raw,repo):
 cap=load(HERE/'capture.json');path,h=cap['public_metadata'];need(sha(repo/path)==h,'cohort pin');meta=load(repo/path)
 cases={r[1]:dict(zip(meta['cases_columns'],r))for r in meta['cases']};need(len(cases)==159,'cohort159')
 for name,h in cap['sources']:need(hashlib.sha256(subprocess.check_output(['git','show',cap['base_git']+':'+name],cwd=repo)).hexdigest()==h,'source '+name)
 allraw={}
 for name,h in cap['raw']:
  need(sha(raw/name)==h,'raw pin '+name);rows=[json.loads(s)for s in (raw/name).read_text().splitlines()];need(rows[-1]==dict(phase='exit',status='ok',reason='none'),'exit')
  ps=[p for p in rows if p['phase']=='full'];need(all(p['status']=='ok'and p['coord_bits']==21 and p['kmax']==5 and p['pass']==i for i,p in enumerate(ps)),'passes');allraw[name]=ps
 configs={};case_stats={};small={g:sorted(k for k,v in cases.items()if v['sites']<=150 and v['groupe']==g)for g in ('reel','uniform','clusters8','slab')}
 need([len(small[g])for g in small]==[20,1,1,1],'small cohort')
 for way in ('cpu','appareil'):
  for w in (4,48):
   conf=f'{way}:{w}';ps=allraw[f'002_mes_c/files/c/brut/session_{way}_5_{w}.jsonl'];need(len(ps)==441,'regular passes');by=collections.defaultdict(list)
   for p in ps:
    need(p['threads']==w and p['voie']==('cpu'if way=='cpu'else'device'),'configuration');need(p['sites']==cases[p['trame']]['sites'],'sites');by[p['trame']].append(p)
   need(len(by)==147 and all(len(v)==3 for v in by.values()),'regular 147x3');configs[conf]={k:v[1:]for k,v in by.items()};case_stats[conf]={k:M(p['wall_ns']for p in v[1:])for k,v in by.items()}
 for label in set(k for values in small.values()for k in values):
  need(len({p['full_sha256']for conf in configs.values()for p in conf[label]})==1,'identity '+label)
 out={'source':'8a0716e74','scope':'K5; one process/configuration, three tours, last two retained; admission separate','real_columns':['label','name','sites','cpu4_median_ns','cpu48_median_ns','device4_median_ns','device48_median_ns'],'real':[],'groups':{},'pairs':{}}
 for k in small['reel']:out['real'].append([k,cases[k]['nom'],cases[k]['sites'],*[case_stats[c][k]for c in ('cpu:4','cpu:48','appareil:4','appareil:48')]])
 for g,labels in small.items():
  out['groups'][g]={}
  for conf,clouds in configs.items():
   ps=[p for label in labels for p in clouds[label]];out['groups'][g][conf]={'clouds':len(labels),'hot_passes':len(ps),'median_cloud_medians_ns':M(case_stats[conf][k]for k in labels),'max_cloud_median_ns':max(case_stats[conf][k]for k in labels),'sums_ns':sums(ps)}
 for a,b in [('cpu:4','cpu:48'),('appareil:4','appareil:48'),('cpu:48','appareil:48'),('cpu:4','appareil:4')]:
  deltas=[case_stats[b][k]-case_stats[a][k]for k in small['reel']];ratios=[case_stats[b][k]/case_stats[a][k]for k in small['reel']]
  sa=out['groups']['reel'][a]['sums_ns'];sb=out['groups']['reel'][b]['sums_ns'];out['pairs'][a+' -> '+b]={'paired_clouds':20,'b_faster':sum(x<0 for x in deltas),'equal':sum(x==0 for x in deltas),'median_paired_delta_ns':M(deltas),'median_paired_ratio':M(ratios),'difference_sums_ns':{k:sb[k]-sa[k]for k in sa},'divisor_for_means':40}
 out['hard100']={}
 for family in ('lattice','line','sphere'):
  out['hard100'][family]={};digests=set()
  for way in ('cpu','appareil'):
   ps=allraw[f'002_mes_c/files/c/brut/synth_{family}_n100_k5_{way}.jsonl'];need(len(ps)==2 and all(p['threads']==48 and p['sites']==100 for p in ps),'hard100');digests.update(p['full_sha256']for p in ps)
   out['hard100'][family][way]={'hot_passes':1,'sums_ns':sums(ps[1:])}
  need(len(digests)==1,'hard100 identity')
 out['ng_cpu']={}
 for i in range(3):
  name=f'ng{i:02d}';ps=[p for j in range(3)for p in allraw[f'001_mes_full/files/full/brut/cpu_{name}_r{j}.jsonl'][1:]];need(len(ps)==12 and all(p['trame']==name and p['threads']==48 and p['voie']=='cpu'for p in ps),'ng CPU12')
  s=sums(ps);out['ng_cpu'][name]={'hot_passes':12,'sums_ns':s,'leaf_largest_passes':sum(p['c_ns']['feuilles']==max(p['c_ns'].values())for p in ps),'leaf_C_ratio_of_sums':s['c_feuilles']/s['C'],'leaf_FULL_ratio_of_sums':s['c_feuilles']/s['wall'],'fixed_other_C_minus_leaves_min_ns':min(p['etapes_ns']['C']-p['c_ns']['feuilles']for p in ps),'fixed_other_wall_minus_leaves_median_ns':M(p['wall_ns']-p['c_ns']['feuilles']for p in ps),'fixed_other_wall_minus_leaves_min_ns':min(p['wall_ns']-p['c_ns']['feuilles']for p in ps),'fixed_other_wall_minus_leaves_above100ms':sum(p['wall_ns']-p['c_ns']['feuilles']>100000000 for p in ps)}
 for n,h in cap['raw']:need(sha(raw/n)==h,'closing '+n)
 return out
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--raw',type=Path,required=True);a.add_argument('--repo',type=Path,required=True);a.add_argument('--write',action='store_true');x=a.parse_args();r=run(x.raw,x.repo)
 if x.write:(HERE/'results.json').write_text(json.dumps(r,separators=(',',':'))+'\n')
 else:need(r==load(HERE/'results.json'),'results changed')
 print(json.dumps({'status':'ok','real_clouds':len(r['real']),'ng_leaf_largest':sum(v['leaf_largest_passes']for v in r['ng_cpu'].values()),'paired_clouds':[v['paired_clouds']for v in r['pairs'].values()]}))
