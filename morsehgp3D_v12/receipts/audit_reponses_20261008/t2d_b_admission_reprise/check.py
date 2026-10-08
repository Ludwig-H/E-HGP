import argparse,copy,hashlib,importlib.util,json,sys,tempfile,contextlib,io,subprocess
from types import SimpleNamespace
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
ap=argparse.ArgumentParser();ap.add_argument('--sources',type=Path,required=True);ap.add_argument('--repo',type=Path,required=True);ap.add_argument('--check',action='store_true');args=ap.parse_args()
cap=json.loads((HERE/'capture.json').read_text());base=args.sources
for name,info in cap['source'].items():
 raw=(base/name).read_bytes()
 if len(raw)!=info['bytes'] or hashlib.sha256(raw).hexdigest()!=info['sha256']:raise ValueError('source drift')
spec=importlib.util.spec_from_file_location('B',base/'pilote_t2d_b.py');B=importlib.util.module_from_spec(spec);spec.loader.exec_module(B)
fixture=args.repo/cap['fixture']['path'];hp=args.repo/cap['prior_harness']['path']
for path,info in [(fixture,cap['fixture']),(hp,cap['prior_harness'])]:
 if hashlib.sha256(path.read_bytes()).hexdigest()!=info['sha256']:raise ValueError('dependency drift')
old=hp.read_text()
fragment=old[old.index('template=['):old.index('# The tiny proposed patch')]
fragment=fragment.replace('B.lire_full(str(p),0,2)','B.lire_full(str(p),0,2,"ng00",48)').replace('pic_appareil_octets=2048','pic_appareil_octets=0')
fragment=fragment.replace('if recompute:take.update(B.lire_prise(str(path),0,5,48,10))','if recompute:\n   try:take.update(B.lire_prise(str(path),0,5,48,10))\n   except ValueError:take.update(g_ns=1,murs_ns=[1]*10,diagnostics=rows[9]["diagnostics"])')
exec(compile(fragment,'published_harness_adapted','exec'))
result['native_G_admitted']=B.lire_prise(str(fixture),0,5,1,2)['valide']
result['full_residuals']=[]
with tempfile.TemporaryDirectory() as t:
 for name in ('nominal','ignored_blocks','memory_used_gt_peak','tmvr_substages_gt_envelope','G_parts_gt_envelope'):
  rows=B.journal_full_synthetique();row=rows[1]
  if name=='ignored_blocks':row.update(c_ns=None,g_ns={'tables':False},hors_mur_ns='invalid',cpu_ns=False,rss_max_octets={'bad':1},appareil_octets=-1,epinglee_octets=True,pic_appareil_octets=3.5)
  elif name=='memory_used_gt_peak':row['memoire_octets']['C'][0]=row['memoire_octets']['C'][1]+1
  elif name=='tmvr_substages_gt_envelope':row['etapes_ns']['T']=row['etapes_ns']['TMVR']+1
  elif name=='G_parts_gt_envelope':row['g_ns']['tables']=row['etapes_ns']['G']+1
  q=Path(t)/'full.jsonl';q.write_text(''.join(json.dumps(x)+'\n' for x in rows))
  try:got=B.lire_full(str(q),0,2,'ng00',3);result['full_residuals'].append({'case':name,'admitted':got['valide']})
  except (ValueError,TypeError,KeyError) as e:result['full_residuals'].append({'case':name,'admitted':False,'reason':str(e)})
# Reconstruct the declared six arms from pinned Git, without writing/building product.
manifest=json.loads((base/'bras_t2d_b.json').read_text());transforms=0
for arm,spec in manifest['bras'].items():
 for rel,entry in spec['fichiers'].items():
  raw=subprocess.check_output(['git','show','902041f66:morsehgp3D_v12/'+rel],cwd=args.repo)
  if hashlib.sha256(raw).hexdigest()!=entry['sha256_avant']:raise ValueError('arm before hash')
  body=raw.decode()
  for edit in entry['substitutions']:
   if body.count(edit['cherche'])!=1:raise ValueError('arm anchor')
   body=body.replace(edit['cherche'],edit['remplace'])
  if hashlib.sha256(body.encode()).hexdigest()!=entry['sha256_apres']:raise ValueError('arm after hash')
  transforms+=1
result['source_transforms']=transforms
# Execute only the scheduling loop; the probe and binary hash are stubbed explicitly.
old_take,old_hash=B.prise,B.sha256
try:
 B.prise=lambda *a,**k:{};B.sha256=lambda p:'d'*64
 with tempfile.TemporaryDirectory() as t:
  report={'construction':{'binaires':{n:{'chemin':n} for n in B.BRAS_JUGES}}}
  B.etape_campagne(SimpleNamespace(sortie=t,fils=48,passes=10,processus=10),report)
  hist={arm:[0]*8 for arm in B.BRAS_JUGES}
  turns=report['campagne_k5']['trames'][B.TRAMES[0]]
  for turn in turns:
   for pos,arm in enumerate(turn):hist[arm][pos]+=1
  if any(sum(h[::2])!=5 or sum(h[1::2])!=5 or min(h)!=1 or max(h)!=2 for h in hist.values()):raise ValueError('schedule parity')
  result['ten_turn_position_counts']=hist
finally:B.prise,B.sha256=old_take,old_hash
with contextlib.redirect_stdout(io.StringIO()) as out:code=B.etape_auto_test()
if code!=0:raise ValueError('developer auto-test')
result['developer_auto_test']={'code':code,'summary':out.getvalue().strip()}
if args.check and result!=json.loads((HERE/'results.json').read_text()):raise ValueError('result differs')
print(json.dumps(result,sort_keys=True,indent=2))
