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

def run(snapshot):
 cap=json.loads((HERE/'capture.json').read_text());source=snapshot/'source'
 for n,p in cap['sources'].items():
  b=(source/n).read_bytes();need(len(b)==p['bytes']and hashlib.sha256(b).hexdigest()==p['sha256'],'source '+n)
 pilot=load('audit_b3',source/'microbancs/mes_t2d_b3/pilote_t2d_b3.py');fixture=load('audit_full_fixture',source/'microbancs/outils/test_lecteur_full.py')
 frames=dict(pilot.LIDAR);frames.update(zip(pilot.MOYENNES,[64740,67114]));effects={'avant':100000000,'avant_bis':100000000,'cles':95000000,'balayage':94000000,'transfert':101000000,'apres':90000000}
 env={'cmake':'synthetic','nvcc':'synthetic','gpu':'synthetic','gpu_apps':''}
 hashes={n:hashlib.sha256(n.encode()).hexdigest()for n in pilot.BRAS_JUGES};hashes['avant_bis']=hashes['avant']
 report={'schema':'ehgp.v12.t2d_b3_pilote.v1','regle':pilot.REGLE_T2D_B3,
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
   p=dict(journal=rel,journal_sha256=hashlib.sha256(raw.encode()).hexdigest(),code=0,etat='ok',raison='',valide=True);p.update(pilot.resume(parsed['passes']));p['mur_ns']=pilot.mediane(p['murs_ns'][1:]);return p
  for frame in pilot.TRAMES:
   report['campagne_k5']['trames'][frame]=[{arm:take(frame,arm,10,False)for arm in pilot.BRAS_JUGES}for _ in range(10)]
   report['identite']['trames'][frame]={arm:take(frame,arm,2,True)for arm in pilot.BRAS_CONSTRUITS}
   identity_files.extend(root/p['journal']for p in report['identite']['trames'][frame].values())
  for frame in pilot.RES_TRAMES:
   report['resolution']['trames'][frame]={}
   for arm in pilot.BRAS_CONSTRUITS:
    rel='resolution_%s_%s.jsonl'%(frame,arm);raw=json.dumps(dict(phase='tour_g',status='ok',kmax=5,threads=48,sites=frames[frame]))+'\n'+json.dumps(dict(phase='digest',resolution_sha256=hashlib.sha256(('resolution:'+frame).encode()).hexdigest()))+'\n';(root/rel).write_text(raw)
    parsed=pilot.lire_resolution(0,raw);need(parsed['valide'],'synthetic resolution');parsed.update(journal=rel,journal_sha256=hashlib.sha256(raw.encode()).hexdigest());report['resolution']['trames'][frame][arm]=parsed;identity_files.append(root/rel)
  results={}
  def judge(label,r):
   j=pilot.juger(r,str(root),verifier_journaux=True);results[label]={k:j.get(k)for k in ['verdicts','refus']};return j
  nominal=judge('nominal',report);need(set(nominal['verdicts'].values())=={'adopte'},'nominal adoption')
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
  for k in ['35_journaux_identite_resolution_absents','FUL1_hash_faux_code137_resume_valide','resolution_brut_en_echec_resume_valide','identites_sans_chemin_hash_code']:
   need(set(results[k]['verdicts'].values())=={'adopte'},'gap '+k)
  need(set(results['controle_FUL1_resume_faux']['verdicts'].values())=={'rejete'},'false identity control')
  need(set(results['controle_journal_temps_modifie']['verdicts'].values())=={'refuse'},'time hash control')
  return dict(native_executed=False,synthetic_only=True,verifier_journaux=True,timing_processes=300,timing_full_passes=3000,ful1_identity_processes=25,ful1_identity_passes=50,resolution_processes=10,cases=results)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',required=True,type=Path);a=p.parse_args();r=run(a.snapshot);need(r==json.loads((HERE/'results.json').read_text()),'stored results changed');print(json.dumps(r,ensure_ascii=False))
