#!/usr/bin/env python3
"""Contre-exemples publics B3 : JSONL synthétiques uniquement, aucune sonde."""
from pathlib import Path
import argparse,copy,hashlib,importlib.util,json,sys,tempfile
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def need(ok,why):
 if not ok:raise ValueError(why)
def load(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def run(snapshot, proposed_source):
 cap=json.loads((snapshot/'capture.json').read_text());source=snapshot/'source'
 for n,p in cap['sources'].items():
  b=(source/n).read_bytes();need(len(b)==p['bytes']and hashlib.sha256(b).hexdigest()==p['sha256'],'source '+n)
 pilot=load('audit_b3',source/'microbancs/mes_t2d_b3/pilote_t2d_b3.py');proposed=load('audit_b3_proposed',proposed_source/'microbancs/mes_t2d_b3/pilote_t2d_b3.py');need(pilot.REGLE_T2D_B3==proposed.REGLE_T2D_B3,'unchanged rule');fixture=load('audit_full_fixture',source/'microbancs/outils/test_lecteur_full.py')
 frames=dict(pilot.LIDAR);frames.update(zip(pilot.MOYENNES,[64740,67114]));effects={'avant':100000000,'avant_bis':100000000,'cles':95000000,'balayage':94000000,'transfert':101000000,'apres':90000000}
 env={'cmake':'synthetic','nvcc':'synthetic','gpu':'synthetic','gpu_apps':''}
 hashes={n:hashlib.sha256(n.encode()).hexdigest()for n in pilot.BRAS_JUGES};hashes['avant_bis']=hashes['avant']
 report={'schema':'ehgp.v12.t2d_b3_pilote.v2','regle':pilot.REGLE_T2D_B3,
  'construction':{'bras_sha256':hashlib.sha256((source/'microbancs/mes_t2d_b3/bras_t2d_b3.json').read_bytes()).hexdigest(),'binaires':{n:{'sha256':h}for n,h in hashes.items()}},
  'environnement':{'avant':env,'apres':env},'trames':{n:{'sites':v}for n,v in frames.items()},
  'campagne_k5':{'fils':48,'passes':10,'tours_demandes':10,'binaires_apres':hashes,'trames':{}},
  'identite':{'trames':{}},'resolution':{'trames':{}}}
 with tempfile.TemporaryDirectory(prefix='b3-public-fixtures-')as td:
  root=Path(td);counter=0;identity_files=[]
  def take(frame,arm,passes,digest):
   nonlocal counter
   rel='journal_%03d.jsonl'%counter;counter+=1;rows=[dict(phase='open',status='ok',reason='none',wall_ns=5,budget_appareil='partage')]
   wall=effects[arm]
   for i in range(passes):
    r=fixture.overlapped_row(i,wall);r.update(trame=frame,sites=frames[frame],pic_octets=1<<30,rss_max_octets=2<<30,pic_appareil_octets=0,cpu_ns=20*wall,memoire_octets=dict(P=[1<<27,1<<28],C=[1<<28,1<<30],tour=[1<<29,1<<30]))
    if digest:r['full_sha256']=pilot.FUL1_NG00_K5 if frame=='ng00'else hashlib.sha256(frame.encode()).hexdigest()
    else:r.pop('full_sha256')
    rows += [r,dict(phase='liberation',pass_=i,liberation_ns=3)]
   rows.append(dict(phase='exit',status='ok',reason='none'));raw=fixture.dump(rows);(root/rel).write_text(raw)
   parsed=pilot.lf.parse_output(0,raw,pilot.attendu(frame,frames[frame],5,48,passes,digest));need(parsed['etat']=='ok',parsed['raison'])
   (root/(rel+'.err')).write_bytes(b'');p=dict(journal=rel,journal_sha256=hashlib.sha256(raw.encode()).hexdigest(),stderr_sha256=hashlib.sha256(b'').hexdigest(),code=0,etat='ok',raison='',valide=True);p.update(pilot.resume(parsed['passes']));p['mur_ns']=pilot.mediane(p['murs_ns'][1:]);return p
  for frame in pilot.TRAMES:
   report['campagne_k5']['trames'][frame]=[{arm:take(frame,arm,10,False)for arm in pilot.BRAS_JUGES}for _ in range(10)]
   report['identite']['trames'][frame]={arm:take(frame,arm,2,True)for arm in pilot.BRAS_CONSTRUITS}
   identity_files.extend(root/p['journal']for p in report['identite']['trames'][frame].values())
  for frame in pilot.RES_TRAMES:
   report['resolution']['trames'][frame]={}
   for arm in pilot.BRAS_CONSTRUITS:
    rel='resolution_%s_%s.jsonl'%(frame,arm)
    scalars='prepare_ns count_ns setup_ns fill_ns workspace_ns tables_ns joins_ns resolve_ns orders_ns reste_ns workspace_bytes table_bytes peak_bytes'.split()
    diag={k:0 for k in scalars};diag.update(order_ns=[0]*5,pass_ns=[0]*5,table_ns=[0]*4,join_ns=[0]*4)
    worknames='probes first_probe_hits probe_hits_after_steps route_t1 route_cert_table route_cert_census route_fallback_table route_fallback_census fallback_no_proposal fallback_not_in_part fallback_certificate census_saturated census_complete census_sites census_sites_max census_nodes jumps_catalogue jumps_census inert_steps cell_stops birth_stops controls max_chain'.split()
    objects='births cells inert_cells extended_cells representatives'.split()
    rows=[dict(phase='tour_g',pass_=0,status='ok',reason='none',order=0,coord_bits=21,kmax=5,threads=48,sites=frames[frame],wall_ns=1,diagnostics=diag)]
    for k in range(1,6):
     obj={x:0 for x in objects};obj['births']=frames[frame] if k==1 else 0;work={x:0 for x in worknames};work['chaines']=[0]*16;rows.append(dict(phase='ordre',k=k,objet=obj,travail=work))
    rows += [dict(phase='digest',resolution_sha256=hashlib.sha256(('resolution:'+frame).encode()).hexdigest()),dict(phase='exit',status='ok',reason='none',order=0)]
    raw=fixture.dump(rows);(root/rel).write_text(raw);(root/(rel+'.err')).write_bytes(b'')
    parsed=pilot.lire_resolution(0,raw);need(parsed['valide'],'synthetic resolution');need(proposed.lire_resolution(0,raw,frames[frame],48)['valide'],'new resolution schema');parsed.update(journal=rel,journal_sha256=hashlib.sha256(raw.encode()).hexdigest(),stderr_sha256=hashlib.sha256(b'').hexdigest(),code=0);report['resolution']['trames'][frame][arm]=parsed;identity_files.append(root/rel)
  results={}
  def judge(label,r,both=True):
   results[label]={}
   for key,P in [('before',pilot),('after',proposed)] if both else [('after',proposed)]:
    j=P.juger(r,str(root),verifier_journaux=True);results[label][key]={k:j.get(k)for k in ['verdicts','refus']};results[label][key]['refus']=[x.replace(str(root),'<synthetic>')for x in results[label][key]['refus']]
   return results[label]
  nominal=judge('nominal',report);need(all(set(x['verdicts'].values())=={'adopte'}for x in nominal.values()),'nominal adoption')
  saved={p:p.read_bytes()for p in identity_files}
  for p in identity_files:p.unlink()
  judge('35_journaux_identite_resolution_absents',report)
  for p,b in saved.items():p.write_bytes(b)
  modified=copy.deepcopy(report)
  for p in modified['identite']['trames']['ng00'].values():p['journal_sha256']='0'*64;p['code']=137
  judge('FUL1_hash_faux_code137_resume_valide',modified)
  p=root/report['resolution']['trames']['ng00']['apres']['journal'];before=p.read_bytes();p.write_text('{"phase":"tour_g","status":"invariant_violated"}\n')
  judge('resolution_brut_en_echec_resume_valide',report);p.write_bytes(before)
  modified=copy.deepcopy(report)
  for phase in ['identite','resolution']:
   for group in modified[phase]['trames'].values():
    for p in group.values():
     for key in ['journal','journal_sha256','code']:p.pop(key,None)
  judge('identites_sans_chemin_hash_code',modified)
  modified=copy.deepcopy(report);modified['identite']['trames']['ng01']['apres']['ful1']=['f'*64]
  judge('controle_FUL1_resume_faux',modified)
  timing=root/report['campagne_k5']['trames']['ng00'][0]['avant']['journal'];before=timing.read_bytes();timing.write_bytes(before+b'\n');judge('controle_journal_temps_modifie',report)
  timing.write_bytes(before)
  # Une divergence d'identite reellement presente dans deux passes conformes est rejetee.
  changed=copy.deepcopy(report);entry=changed['identite']['trames']['ng01']['apres'];path=root/entry['journal'];previous=path.read_bytes();rows=[json.loads(x)for x in previous.decode().splitlines()]
  for row in rows:
   if row.get('phase')=='full':row['full_sha256']='f'*64
  raw=fixture.dump(rows);path.write_text(raw);entry['journal_sha256']=hashlib.sha256(raw.encode()).hexdigest();entry['ful1']=['f'*64]
  judge('controle_FUL1_brut_et_resume_differents_des_autres_bras',changed);path.write_bytes(previous)
  cases={
   'schema_historique':lambda r:r.update(schema='ehgp.v12.t2d_b3_pilote.v1'),
   'code_temps_booleen':lambda r:r['campagne_k5']['trames']['ng00'][0]['avant'].update(code=False),
   'code_temps_chaine':lambda r:r['campagne_k5']['trames']['ng00'][0]['avant'].update(code='0'),
   'valide_temps_chaine':lambda r:r['campagne_k5']['trames']['ng00'][0]['avant'].update(valide='yes'),
   'stderr_temps_hash_faux':lambda r:r['campagne_k5']['trames']['ng00'][0]['avant'].update(stderr_sha256='f'*64),
   'stderr_temps_hash_absent':lambda r:r['campagne_k5']['trames']['ng00'][0]['avant'].pop('stderr_sha256'),
   'resume_temps_etage_faux':lambda r:r['campagne_k5']['trames']['ng00'][0]['avant'].update(c_ns=[1]*10),
   'code_FUL1_booleen':lambda r:r['identite']['trames']['ng00']['avant'].update(code=False),
   'code_resolution_absent':lambda r:r['resolution']['trames']['ng00']['avant'].pop('code'),
   'code_resolution_booleen':lambda r:r['resolution']['trames']['ng00']['avant'].update(code=False),
   'resume_FUL1_vide_un_bras':lambda r:r['identite']['trames']['ng00']['avant'].update(ful1=[]),
   'chemin_identite_externe':lambda r:r['identite']['trames']['ng00']['avant'].update(journal='../absent.jsonl'),
   'journal_identite_reutilise':lambda r:r['identite']['trames']['ng00'].update(cles=copy.deepcopy(r['identite']['trames']['ng00']['avant'])),
  }
  for label,mutate in cases.items():
   r=copy.deepcopy(report);mutate(r);judge(label,r,False)
  def raw_change(label,phase,frame,arm,mutate):
   changed=copy.deepcopy(report);entry=changed[phase]['trames'][frame][arm];p=root/entry['journal'];raw=p.read_bytes();rows=[json.loads(x)for x in raw.decode().splitlines()];mutate(rows);text=fixture.dump(rows);p.write_text(text);entry['journal_sha256']=hashlib.sha256(text.encode()).hexdigest();judge(label,changed,False);p.write_bytes(raw)
  raw_change('FULL_une_passe_au_lieu_de_deux','identite','ng00','avant',lambda rows:rows.__delitem__(slice(3,5)))
  raw_change('resolution_deux_passes','resolution','ng00','avant',lambda rows:rows.insert(1,dict(rows[0],**{'pass':1})))
  raw_change('resolution_autre_nombre_sites','resolution','ng00','avant',lambda rows:rows[0].update(sites=frames['ng00']-1))
  raw_change('resolution_threads1','resolution','ng00','avant',lambda rows:rows[0].update(threads=1))
  raw_change('resolution_exit_absent','resolution','ng00','avant',lambda rows:rows.pop())
  r=copy.deepcopy(report);entry=r['resolution']['trames']['ng00']['avant'];p=root/(entry['journal']+'.err');p.write_text('failure\n');entry['stderr_sha256']=hashlib.sha256(p.read_bytes()).hexdigest();judge('stderr_resolution_non_vide',r,False);p.write_bytes(b'')
  for label,states in results.items():
   expected='adopte' if label=='nominal' else 'rejete' if label=='controle_FUL1_brut_et_resume_differents_des_autres_bras' else 'refuse'
   need(set(states['after']['verdicts'].values())=={expected},'new expected outcome '+label)
  for label in ['35_journaux_identite_resolution_absents','FUL1_hash_faux_code137_resume_valide','resolution_brut_en_echec_resume_valide','identites_sans_chemin_hash_code']:
   need(set(results[label]['before']['verdicts'].values())=={'adopte'},'old gap '+label)
  return dict(native_executed=False,synthetic_only=True,verifier_journaux=True,timing_processes=300,timing_full_passes=3000,ful1_identity_processes=25,ful1_identity_passes=50,resolution_processes=10,cases=results)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',required=True,type=Path);p.add_argument('--proposed-source',required=True,type=Path);p.add_argument('--out',required=True,type=Path);a=p.parse_args();r=run(a.snapshot,a.proposed_source);a.out.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(json.dumps({'cases':len(r['cases']),'nominal':r['cases']['nominal'],'native_executed':False}))
