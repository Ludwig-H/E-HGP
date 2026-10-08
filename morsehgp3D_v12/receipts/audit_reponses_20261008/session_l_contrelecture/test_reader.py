#!/usr/bin/env python3
"""JSON synthetiques seulement ; aucune sonde, compilation ou donnee geometrique."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
sys.dont_write_bytecode = True
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
    row['pass']=i
    if spec['empreinte']:
      row['full_sha256']=hashlib.sha256((spec['nom']+str(spec['k'])).encode()).hexdigest()
    rows += [row,{'phase':'liberation','pass':i,'liberation_ns':3}]
  return rows+[dict(phase='exit',status='ok',reason='none')]


def main():
  repo=Path(__file__).resolve().parents[4]
  cap=r.decode((r.HERE/'capture.json').read_bytes())
  reasons=r.sources(repo,cap)
  # Port epingle ; seule evaluation de fonctions pures, jamais main/run/build.
  pin=cap['sources']['morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py']
  raw=subprocess.check_output(['git','show',pin['pin']+':morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py'],cwd=repo)
  p={'__name__':'audit_pure_functions'}
  exec(compile(raw,'pilote_epingle','exec'),p)
  plans=Path('/workspaces/E-HGP/build/v12-data-20261007/plans')
  sessions=0
  for name,meta in cap['sessions'].items():
    planraw=(plans/('plan_b_'+name.lower()+'.json')).read_bytes()
    r.need(r.sha(planraw)==meta['plan_sha256'],'plan')
    specs,opt=r.cohort(r.decode(planraw),meta['sites'])
    used=set()
    r.need([p['label_of'](c['nom'],used) for c in specs]==[c['etiquette'] for c in specs],'etiquettes')
    report=dict(mesure='MES-B',regime='b',verdict='tenu',controles=[],
          environnement={t:dict(cmake='synthetique',nvcc='synthetique',gpu='synthetique',gpu_apps='',
            noyau='synthetique',fils_hote=48,memoire_hote='synthetique') for t in ('avant','apres')},
          provenance=dict(cmake=['synthetique'],sonde_sha256='a'*64,pilote_sha256=pin['sha256'],
                  manifeste_sha256=meta['manifest_sha256']),
          parametres=dict(fils=48,budget_octets=160<<30,budget_appareil_octets=88<<30,
             delai_global_s=float(opt['--delai-global']),series=[s.split(',') for s in opt.get('--series','').split(';') if s],
             argv=r.decode(planraw)['commands'][0]['argv'][2:]),cas=[],duree_s=1.0)
    raws={}
    for c in specs:
      rows=fixture(c)
      tag=f"{c['nom']}_k{c['k']}_{c['voie']}"
      raws[tag+'.jsonl'],raws[tag+'.err']=dump(rows),b''
      entry={k:v for k,v in c.items() if k!='expected'}
      body=rows[int(c['voie']=='appareil'):-1]
      entry.update(etat='ok',raison='',passes=[dict(body[i],liberation_ns=3) for i in range(0,len(body),2)],
            prevision_s=1.0,code=0,secondes=1.0,pic_nvidia_smi_mio=None)
      if c['voie']=='appareil': entry['open_ns']=7
      report['cas'].append(entry)
    def finish(rep):
      rep['empreintes']=p['digest_controls'](rep['cas'])[0]
      rep['criteres']=p['verdicts'](rep['cas'],rep['parametres']['series'])
      states=[x['etat'] for x in rep['criteres'].values()]
      rep['verdict']='tenu' if all(rep['criteres'][k]['etat']!='non evalue' for k in ('B1','B2')) and all(s in ('tenu','non evalue') for s in states) else 'non tenu'
    finish(report)
    review=lambda rep,rs: r.review(rep,rs,specs,opt,meta['manifest_sha256'],reasons)
    answer=review(report,raws)
    r.need(answer['bruts_admis'],'synthetique complet '+name)
    for item in answer['cas']:
      st=item['statistiques']
      r.need(st['regime_derniere']==('chaude' if item['expected']>1 else 'froide'),'regime')
      r.need((st['mediane_chaude_ns'] is None)==(item['expected']==1),'froid promu chaud')
    sessions+=1
    if name!='L1': continue
    altered=copy.deepcopy(report);altered['cas'][0]['raison']='cause inventee'
    r.need(not review(altered,raws)['bruts_admis'],'raison de rapport inventee')
    altered=copy.deepcopy(report);altered['cas'].pop()
    try:review(altered,raws)
    except r.Refusal:pass
    else:raise ValueError('cas disparu de la cohorte')
    base=fixture(specs[0])
    mutations={
      'schema902':lambda x:x.update(memoire_octets={}), 'bool':lambda x:x.update(cpu_ns=True),
      'u64':lambda x:x.update(wall_ns=1<<64), 'negative':lambda x:x.update(cpu_ns=-1),
      'zero':lambda x:x.update(wall_ns=0), 'digest':lambda x:x.pop('full_sha256'),
      'bits':lambda x:x.update(coord_bits=24), 'sites':lambda x:x.update(sites=x['sites']-1),
      'threads':lambda x:x.update(threads=1), 'label':lambda x:x.update(trame='autre'),
      'wall':lambda x:x['etapes_ns'].update(P=x['wall_ns']),
      'tmvr':lambda x:x['etapes_ns'].update(T=31), 'g':lambda x:x['g_ns'].update(tables=20),
      'resident':lambda x:x.update(pic_octets=15*x['sites']),
      'pinned':lambda x:x.update(epinglee_octets=x['pic_octets']+1),
      'capacity':lambda x:x.update(appareil_octets=17),
      'host_budget':lambda x:x.update(pic_octets=(160<<30)+1),
      'device_budget':lambda x:x.update(pic_appareil_octets=(88<<30)+1),
    }
    rejected=0
    for mutate in mutations.values():
      rows=copy.deepcopy(base);mutate(rows[1])
      try: r.stream(dump(rows),0,specs[0],reasons)
      except r.Refusal: rejected+=1
    for data,code in ((dump(base).replace(b'"wall_ns": 7',b'"wall_ns": 7, "wall_ns": 7'),0),
             (dump(base).replace(b'"cpu_ns":',b'"cpu_ns": NaN, "unused":',1),0),
             (dump(base[1:]),0),(dump(base[:-1]),0),(dump(base),False),
             (dump(base[:-2]+[base[-1]]),0)):
      try:r.stream(data,code,specs[0],reasons)
      except r.Refusal:rejected+=1
    r.need(rejected==len(mutations)+6,'mutation survivante')
    for key in ('cpu_ns','rss_max_octets'):
      rows=copy.deepcopy(base);rows[1][key]=None
      result=r.stream(dump(rows),0,specs[0],reasons)
      r.need(result['conditions'] and result['passes'][0][key] is None,'null transforme/admis')
    partial=base[:3]+[dict(phase='exit',status='resource_exhausted',reason='memory_budget')]
    partial=r.stream(dump(partial),2,specs[0],reasons)
    r.need(partial['etat']=='refus' and len(partial['passes'])==1,'prefixe conforme perdu')
    r.need(r.statistics_case(partial['passes'])['regime_derniere']=='froide','prefixe froid promu')
    for index,state in ((12,'refus'),(11,'refus'),(11,'echec'),(17,'non_joue')):
      rep,rs=copy.deepcopy(report),dict(raws);entry=rep['cas'][index];c=specs[index]
      tag=f"{c['nom']}_k{c['k']}_{c['voie']}";entry['passes']=[];entry['etat']=state
      if state=='non_joue':
        for k in ('code','secondes','pic_nvidia_smi_mio','open_ns'):entry.pop(k,None)
        entry['raison']='delai : synthetique';rs.pop(tag+'.jsonl');rs.pop(tag+'.err')
      else:
        status,reason,code=('resource_exhausted','memory_budget',2) if state=='refus' else ('invariant_violated','tower_invariant',3)
        entry.update(code=code,raison=status+'/'+reason if state=='refus' else
                     f'sortie {status}/{reason}, code {code}, 0 passes')
        if state=='echec':entry.pop('open_ns',None)
        rs[tag+'.jsonl']=dump([fixture(c)[0],dict(phase='exit',status=status,reason=reason)])
      finish(rep);answer=review(rep,rs)
      r.need(answer['bruts_admis'] and answer['cas'][index]['etat']==state,'issue non preservee')
      if index==12:r.need(answer['criteres']['B4']=='non tenu','refus K10 ignore')
      if index==11:r.need(answer['criteres']['B1']==('tenu' if state=='refus' else 'non tenu'),'B1 seuil10M')
    print(json.dumps(dict(mutations_refusees=rejected,nulls_preserves=2,issues_controlees=4,
              corpus='synthetique uniquement'),sort_keys=True))
  print(json.dumps(dict(cohortes_synthetiques=sessions,aucune_mesure_reelle_admise=True),sort_keys=True))


if __name__=='__main__':main()
