#!/usr/bin/env python3
"""Cohorte L2 CPU et altérations JSON uniquement ; aucune sonde."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import reader as r

def dump(rows):
  return b''.join(json.dumps(v).encode()+b'\n' for v in rows)


def fixture(spec):
  rows = []
  if spec['voie'] == 'appareil':
    rows.append(dict(phase='open',status='ok',reason='none',wall_ns=7,budget_appareil='separe'))
  for i in range(spec['expected']):
    wall = spec['sites'] * (2000 if i == 0 else 1000)
    row = dict(phase='full',trame=spec['etiquette'],voie='device' if spec['voie']=='appareil' else 'cpu',
         status='ok',coord_bits=21,kmax=spec['k'],threads=48,sites=spec['sites'],wall_ns=wall,
         etapes_ns=dict(P=10,C=20,G=20,raccord=10,TMVR=30,T=5,M=5,V=5,R=5),
         c_ns={k:1 for k in r.CAT},g_ns=dict(tables=5,resolution=5),
         hors_mur_ns=dict(validation=4,empreinte=int(spec['empreinte'])),
         pic_octets=16*spec['sites']+100,cpu_ns=2*wall,rss_max_octets=16*spec['sites']+200,
         appareil_octets=8 if spec['voie']=='appareil' else 0,
         epinglee_octets=4 if spec['voie']=='appareil' else 0,
         pic_appareil_octets=16 if spec['voie']=='appareil' else 0)
    n=16*spec['sites']
    row['memoire_octets']=dict(P=[n+1,n+3],C=[n+10,n+100],G=[n+20,n+25],raccord=[n+20,n+20],TMVR=[n+30,n+50])
    row['pass']=i
    if spec['empreinte']:
      row['full_sha256']=hashlib.sha256((spec['nom']+str(spec['k'])).encode()).hexdigest()
    rows += [row,{'phase':'liberation','pass':i,'liberation_ns':3}]
  return rows+[dict(phase='exit',status='ok',reason='none')]


def main():
  ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True)
  ap.add_argument('--plan',type=Path,required=True);args=ap.parse_args()
  cap=r.decode((r.HERE/'capture.json').read_bytes());meta=cap['sessions']['L2']
  reasons=r.sources(args.repo,cap)
  plan=args.plan.read_bytes();r.need(r.sha(plan)==meta['plan_sha256'],'plan')
  specs,opt=r.cohort(r.decode(plan),meta['sites'])
  r.need(len(specs)==5 and all(c['voie']=='cpu' and c['k']==5 and c['expected']==1 for c in specs),'cohorte L2')
  report=dict(mesure='MES-B',regime='b',verdict='non tenu',controles=[],
    environnement={t:dict(cmake='synthetique',nvcc='synthetique',gpu='synthetique',gpu_apps='',
      noyau='synthetique',fils_hote=48,memoire_hote='synthetique') for t in ('avant','apres')},
    provenance=dict(cmake=['synthetique'],sonde_sha256='a'*64,
      pilote_sha256=cap['sources']['morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py']['sha256'],
      manifeste_sha256=meta['manifest_sha256']),
    parametres=dict(fils=48,budget_octets=160<<30,budget_appareil_octets=88<<30,
      delai_global_s=float(opt['--delai-global']),series=[],argv=r.decode(plan)['commands'][0]['argv'][2:]),
    cas=[],duree_s=1.0,empreintes={},criteres={k:dict(etat='non evalue',detail=[]) for k in ('B1','B2','B3','B4')})
  raws={}
  for case in specs:
    rows=fixture(case);tag=f"{case['nom']}_k5_cpu"
    raws[tag+'.jsonl'],raws[tag+'.err']=dump(rows),b''
    entry={k:v for k,v in case.items() if k!='expected'}
    entry.update(etat='ok',raison='',passes=[dict(rows[0],liberation_ns=3)],prevision_s=1.0,
      code=0,secondes=1.0,pic_nvidia_smi_mio=None)
    report['cas'].append(entry)
  review=lambda rep,rs:r.review(rep,rs,specs,opt,meta['manifest_sha256'],reasons)
  answer=review(report,raws)
  r.need(answer['bruts_admis'] and all(v=='non evalue' for v in answer['criteres'].values()),'synthétique L2')
  r.need(all(c['statistiques']['mediane_chaude_ns'] is None for c in answer['cas']),'froid promu chaud')
  base=fixture(specs[0])
  mutations={
    'schema403':lambda x:x.pop('memoire_octets'),
    'bool':lambda x:x.update(cpu_ns=True),'u64':lambda x:x.update(wall_ns=1<<64),
    'zero':lambda x:x.update(wall_ns=0),'extra':lambda x:x.update(inconnu=1),
    'sites':lambda x:x.update(sites=x['sites']-1),'threads':lambda x:x.update(threads=1),
    'bits':lambda x:x.update(coord_bits=32),'mode':lambda x:x.update(voie='device'),
    'cpu_device':lambda x:x.update(appareil_octets=1,pic_appareil_octets=1),
    'wall':lambda x:x['etapes_ns'].update(P=x['wall_ns']),
    'tmvr':lambda x:x['etapes_ns'].update(T=31),'g':lambda x:x['g_ns'].update(tables=20),
    'missingmem':lambda x:x['memoire_octets'].pop('G'),
    'membool':lambda x:x['memoire_octets']['P'].__setitem__(0,True),
    'membound':lambda x:x['memoire_octets']['P'].__setitem__(0,16*x['sites']-1),
    'memusage':lambda x:x['memoire_octets']['P'].__setitem__(0,x['memoire_octets']['P'][1]+1),
    'frontiere':lambda x:x['memoire_octets']['C'].__setitem__(0,x['memoire_octets']['G'][1]+1),
    'maxpic':lambda x:x.update(pic_octets=x['pic_octets']+1),
    'raccord':lambda x:x['memoire_octets']['raccord'].__setitem__(1,x['memoire_octets']['raccord'][1]+1),
  }
  rejected=0
  for change in mutations.values():
    rows=copy.deepcopy(base);change(rows[0])
    try:r.stream(dump(rows),0,specs[0],reasons)
    except r.Refusal:rejected+=1
  for raw,code in [(dump(base),False),(dump(base[:-1]),0),(dump(base[:-2]+[base[-1]]),0),
      (dump(base).replace(b'"pass": 0',b'"pass": false',1),0),
      (dump(base).replace(b'"wall_ns":',b'"extra": NaN, "wall_ns":',1),0)]:
    try:r.stream(raw,code,specs[0],reasons)
    except r.Refusal:rejected+=1
  r.need(rejected==len(mutations)+5,'mutation survivante')
  for key in ('cpu_ns','rss_max_octets'):
    rows=copy.deepcopy(base);rows[0][key]=None;ans=r.stream(dump(rows),0,specs[0],reasons)
    r.need(ans['conditions'] and ans['passes'][0][key] is None,'null perdu')
  for status,reason in [('unsupported_degeneracy','wide_leaf'),('resource_exhausted','memory_budget')]:
    rep,rs=copy.deepcopy(report),dict(raws);c=specs[0];tag=f"{c['nom']}_k5_cpu"
    rep['cas'][0].update(etat='refus',raison=status+'/'+reason,code=2,passes=[])
    rs[tag+'.jsonl']=dump([dict(phase='exit',status=status,reason=reason)])
    ans=review(rep,rs)
    r.need(ans['bruts_admis'] and ans['cas'][0]['etat']=='refus','refus CPU')
  rep,rs=copy.deepcopy(report),dict(raws);c=specs[-1];tag=f"{c['nom']}_k5_cpu"
  rep['cas'][-1].update(etat='non_joue',raison='delai : synthetique',passes=[])
  for key in ('code','secondes','pic_nvidia_smi_mio'):rep['cas'][-1].pop(key)
  rs.pop(tag+'.jsonl');rs.pop(tag+'.err')
  r.need(review(rep,rs)['bruts_admis'],'non joué conservé')
  rep=copy.deepcopy(report);rep['cas'].pop()
  try:review(rep,raws)
  except r.Refusal:pass
  else:raise ValueError('cohorte tronquée admise')
  tiny=dict(specs[0],sites=10,empreinte=True,expected=2)
  rows=fixture(tiny);broken=copy.deepcopy(rows);broken[0].pop('full_sha256')
  try:r.stream(dump(broken),0,tiny,reasons)
  except r.Refusal:pass
  else:raise ValueError('empreinte demandée absente')
  partial=rows[:2]+[dict(phase='exit',status='resource_exhausted',reason='memory_budget')]
  ans=r.stream(dump(partial),2,tiny,reasons)
  r.need(ans['etat']=='refus' and len(ans['passes'])==1 and
    r.statistics_case(ans['passes'])['regime_derniere']=='froide','préfixe froid avant refus')
  print(json.dumps(dict(synthetic_cohort=5,corruptions_refused=rejected,refusal_types=2,
    nulls_preserved=2,missing_case_refused=True,unplayed_preserved=True,
    requested_digest_enforced=True,partial_cold_refusal_preserved=True,native_execution=False),sort_keys=True))


if __name__=='__main__':main()
