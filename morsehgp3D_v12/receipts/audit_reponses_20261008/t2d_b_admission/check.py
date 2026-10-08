import argparse,copy,hashlib,importlib.util,json,subprocess,sys,tempfile
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description='Sources/JSON uniquement ; aucune sonde ou compilation.')
parser.add_argument('--sources',type=Path,required=True)
parser.add_argument('--repo',type=Path,required=True)
parser.add_argument('--check',action='store_true')
args=parser.parse_args()
cap=json.loads((HERE/'capture.json').read_text())
for name,pin in cap['sources'].items():
 raw=(args.sources/name).read_bytes()
 if len(raw)!=pin['bytes'] or hashlib.sha256(raw).hexdigest()!=pin['sha256']:raise RuntimeError('source drift: '+name)
sp=importlib.util.spec_from_file_location('B',args.sources/'pilote_t2d_b.py');B=importlib.util.module_from_spec(sp);sp.loader.exec_module(B)
fixture=args.repo/cap['fixture']['path']
if hashlib.sha256(fixture.read_bytes()).hexdigest()!=cap['fixture']['sha256']:raise RuntimeError('fixture drift')
for rel,pin in cap['producer'].items():
 raw=subprocess.check_output(['git','-C',str(args.repo),'show',pin['commit']+':morsehgp3D_v12/'+rel])
 if hashlib.sha256(raw).hexdigest()!=pin['sha256']:raise RuntimeError('producer drift: '+rel)
# Reconstruct all six arms in memory from Git; no compiler or product file modification.
manifest=json.loads((args.sources/'bras_t2d_b.json').read_text());transforms=0
for arm,spec in manifest['bras'].items():
 for rel,entry in spec['fichiers'].items():
  raw=subprocess.check_output(['git','-C',str(args.repo),'show','902041f66:morsehgp3D_v12/'+rel])
  if hashlib.sha256(raw).hexdigest()!=entry['sha256_avant']:raise RuntimeError('before hash')
  text=raw.decode()
  for edit in entry['substitutions']:
   if text.count(edit['cherche'])!=1:raise RuntimeError('substitution anchor')
   text=text.replace(edit['cherche'],edit['remplace'])
  if hashlib.sha256(text.encode()).hexdigest()!=entry['sha256_apres']:raise RuntimeError('after hash')
  transforms+=1
if transforms!=31:raise RuntimeError('transform count')
template=[json.loads(x) for x in fixture.read_text().splitlines()]
first=next(x for x in template if x['phase']=='tour_g');tail=[x for x in template if x['phase']!='tour_g']

def time_row(row,wall):
 row['wall_ns']=wall
 d=row['diagnostics'];inside=sum(d[k] for k in ('prepare_ns','count_ns','setup_ns','fill_ns','workspace_ns','orders_ns'))
 d['reste_ns']=max(0,wall-inside)

def encode(rows):return ''.join(json.dumps(r,sort_keys=True)+'\n' for r in rows).encode()
def read(rows):
 with tempfile.TemporaryDirectory() as d:
  p=Path(d)/'read.jsonl';p.write_bytes(encode(rows))
  try:return {'admitted':True,'summary':B.lire_full(str(p),0,2)}
  except (ValueError,TypeError,KeyError) as e:return {'admitted':False,'reason':str(e)}

def full_rows(way):
 rows=[dict(phase='open',status='ok',reason='none',wall_ns=5,budget_appareil='partage')]
 for i in range(2):
  rows.extend([dict(phase='full',pass_=i,trame='ng00',voie=way,status='ok',coord_bits=21,kmax=5,threads=48,sites=8,
   wall_ns=1000,etapes_ns={k:(4 if k=='TMVR' else 1) for k in ('P','C','G','raccord','TMVR','T','M','V','R')},
   c_ns={k:1 for k in ('parcours','feuilles','emission','fin_etage','transferts','publication')},
   g_ns=dict(tables=0,resolution=0),hors_mur_ns=dict(validation=0,empreinte=0),pic_octets=1024,cpu_ns=20,
   rss_max_octets=1024,appareil_octets=1024,epinglee_octets=64,pic_appareil_octets=2048,
   memoire_octets={k:[128,1024] for k in ('P','C','G','raccord','TMVR')},full_sha256='a'*64),
   dict(phase='liberation',pass_=i,liberation_ns=1)])
 for row in rows:
  if 'pass_' in row:row['pass']=row.pop('pass_')
 rows.append(dict(phase='exit',status='ok',reason='none'))
 return rows

result={'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'full':{},'campaign':[]}
result['full']['producer_device']=read(full_rows('device'))
result['full']['only_literal_changed']=read(full_rows('appareil'))
raw=[dict(phase='open',status='ok')]+[{'phase':'full','pass':False if i==0 else i,'status':'ok','voie':'appareil','wall_ns':1000,'etapes_ns':{'G':1},'full_sha256':'a'*64} for i in range(2)]+[dict(phase='exit',status='ok',reason='none')]
result['full']['missing_configuration_and_release']=read(raw)
with tempfile.TemporaryDirectory() as folder:
 root=Path(folder)
 def make_report():
  report={'construction':{'bras_sha256':B.sha256(B.BRAS_FICHIER),'binaires':{b:{'sha256':'d'*64} for b in B.BRAS_JUGES}},'campagne_k5':{'fils':48,'passes':10,'tours_demandes':10,'binaires_apres':{b:'d'*64 for b in B.BRAS_JUGES},'trames':{}}}
  for frame in B.TRAMES:
   tours=[]
   for t in range(10):
    tour={}
    for arm in B.BRAS_JUGES:
     rows=[];wall=1000000000 if arm in ('avant','avant_bis') else 800000000
     for p in range(10):
      row=copy.deepcopy(first);row.update({'pass':p,'threads':48});time_row(row,wall);rows.append(row)
     rows.extend(copy.deepcopy(tail));rows[-2]['resolution_sha256']=B.EMPREINTE_NG00_K5+'0'*48 if frame=='ng00' else 'a'*64
     path=root/f'{frame}_{t}_{arm}.jsonl';path.write_bytes(encode(rows))
     tour[arm]={'journal':path.name,'journal_sha256':B.sha256(path),'code':0,**B.lire_prise(str(path),0,5,48,10)}
    tours.append(tour)
   report['campagne_k5']['trames'][frame]=tours
  return report
 def mutate_take(take,fn,recompute):
  path=root/take['journal'];rows=[json.loads(x) for x in path.read_text().splitlines()];fn(rows);path.write_bytes(encode(rows));take['journal_sha256']=B.sha256(path)
  if recompute:take.update(B.lire_prise(str(path),0,5,48,10))
 for name in ('nominal','bad_threads','duplicate_journal','missing_frame','different_identity','stage_exceeds_wall','aa_outside'):
  report=make_report();camp=report['campagne_k5'];take=camp['trames']['ng00'][0]['apres']
  if name=='bad_threads':mutate_take(take,lambda rows:rows[0].update(threads=1),False)
  if name=='duplicate_journal':camp['trames']['ng00'][0]['apres']=copy.deepcopy(camp['trames']['ng00'][0]['avant'])
  if name=='missing_frame':del camp['trames']['ng02']
  if name=='different_identity':mutate_take(take,lambda rows:rows[-2].update(resolution_sha256='e'*64),True)
  if name in ('stage_exceeds_wall','aa_outside'):
   arm='apres' if name=='stage_exceeds_wall' else 'avant_bis';wall=1 if name=='stage_exceeds_wall' else 1200000000
   for frame in B.TRAMES:
    for tour in camp['trames'][frame]:
     mutate_take(tour[arm],lambda rows:[time_row(r,wall) for r in rows if r['phase']=='tour_g'],True)
  got=B.juger(report,str(root),verifier_journaux=True)
  summary={'case':name,'verdicts':got['verdicts'],'refus':got['refus'],'aa':got.get('controle_aa')}
  if name=='stage_exceeds_wall':summary['first_diag_count_ns']=first['diagnostics']['count_ns'];summary['mutated_wall_ns']=1
  result['campaign'].append(summary)
# The tiny proposed patch fixes only the producer literal, leaving stricter FULL admission separate.
source=(args.sources/'pilote_t2d_b.py').read_text()
old='r.get("voie") == "appareil"'
if source.count(old)!=1:raise RuntimeError('patch anchor')
patched=source.replace(old,'r.get("voie") == "device"')
if hashlib.sha256(patched.encode()).hexdigest()!=cap['patch']['sha256_after']:raise RuntimeError('patch drift')
namespace=dict(B.__dict__);exec(compile(patched,'patched_pilot','exec'),namespace)
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'full.jsonl';p.write_bytes(encode(full_rows('device')))
 if namespace['lire_full'](str(p),0,2)['valide'] is not True:raise RuntimeError('patch positive')
if args.check:
 if result!=json.loads((HERE/'results.json').read_text()):raise RuntimeError('result drift')
 print('verified: 6 arms/31 transformations; 7 complete G judgements; 3 FULL witnesses and literal patch')
else:
 print(json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2))
