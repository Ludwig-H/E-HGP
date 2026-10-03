"""Protocol-only replay: captured G4 metadata shapes, simulated CMake, Python fixture children."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,sys
from unittest.mock import patch
ROOT=Path(__file__).resolve().parent
SOURCE=ROOT/'snapshot2'
MODULE=SOURCE/'morsehgp3D_v11'
sys.path[:0]=[str(MODULE/'bench'),str(MODULE/'tests/tower')]
os.environ['PYTHONDONTWRITEBYTECODE']='1'
sys.dont_write_bytecode=True
import full_paired as paired
REAL=Path('/workspaces/.ehgp-sessions/v11.20261003.paired650r2/results/extracted/results')
STAGE=ROOT/'stage2';STAGE.mkdir(exist_ok=True)
DATA=STAGE/'data';DATA.mkdir(exist_ok=True)
BUILDS=STAGE/'build/matrix';BUILDS.mkdir(parents=True,exist_ok=True)
OUT=STAGE/'out';OUT.mkdir(exist_ok=True)
QUAL=STAGE/'qualification/matrix';(QUAL/'bits21').mkdir(parents=True,exist_ok=True)
PRIOR=STAGE/'prior';PRIOR.mkdir(exist_ok=True)
SOURCE_PIN='commit:6503c95abeb7822a5efd61e25babc2c0b60b79cc'
os.environ.update(V11_SOURCE_PIN=SOURCE_PIN,V11_SRC=str(SOURCE),V11_BUILD=str(STAGE/'build'),V11_PACKAGE_SHA256='f'*64,V11_GENERATION='protocol-fake')
CHILD='''#!/usr/bin/env python3
# Python protocol fixture only, no native engine or performance measurement.
from pathlib import Path
import json,sys
sys.path[:0]=%r
sys.dont_write_bytecode=True
import full_campaign_test as model
import full_bench_semantic_test as codec
points=[(0,0,0),(2,0,0),(4,0,0),(6,0,0),(8,0,0)]
model.VALUE=codec.fixture(points,5)
mode=int(sys.argv[11]);workers=int(sys.argv[10])
v=model.events(bits=21,kmax=5,workers=workers,optimizations=mode)
v[0].update(sites=5,points=5)
d=v[1];d.update(catalogue_balls=10,catalogue_incidences=30)
d['catalogue_work'].update(emitted=10,incidences=30,judged=10,max_leaf=5)
d['execution'].update(compact_records=10,compact_population=30)
e=v[2]
e['census_workspace_reserved_bytes']=20*e['census_workspaces']
e['regular_vertical_reserved_bytes']=40
for k,o in enumerate(e['orders'],1):
 o['lookup_reserved_bytes']=4*(5 if k==1 else 10)
e['lookup_reserved_bytes']=sum(o['lookup_reserved_bytes'] for o in e['orders'])
e['peak_reserved_bytes']=400+e['memo_reserved_bytes']+e['parallel']['lane_memo_reserved_bytes']+e['census_workspace_reserved_bytes']+40
if mode&8192:
 e.update(population_lookup=True,concurrent_orders=True,phases=dict.fromkeys(model.driver.acceleration.PHASES,20),forest_ns=110,wall_ns=300)
 for k,o in enumerate(e['orders'],1):
  if k>1:o['vertical_parallel'].update(vertical_batches=1,max_vertical_batch=o['births'],vertical_resolutions=0)
if 'baseline_build' in sys.argv[0]:
 for key in model.driver.acceleration.FIELDS:e.pop(key)
 for o in e['orders']:o['work'].pop('population_hits')
Path(sys.argv[3]).write_bytes(codec.encode(model.VALUE,21)[0])
for event in v:print(json.dumps(event))
'''%[str(MODULE/'bench'),str(MODULE/'tests/tower')]
EXE=BUILDS/'bits21/build/mhgp11_full_bench';EXE.parent.mkdir(parents=True,exist_ok=True);EXE.write_text(CHILD);EXE.chmod(0o755)
current_prov=json.loads((REAL/'cmd/001_bits21/files/matrix/bits21/build_provenance.json').read_text())
for row in current_prov['files']:
 if row['path']=='mhgp11_full_bench':row.update(sha256=paired.base.digest(EXE),size=EXE.stat().st_size)
CACHE=EXE.parent/'CMakeCache.txt';CACHE.write_text(next(e['text'] for e in current_prov['files'] if e['path']=='CMakeCache.txt'))
(QUAL/'bits21/build_provenance.json').write_text(json.dumps(current_prov))
for name in ['summary.json','bits21/tests.json']:
 shutil.copyfile(REAL/'cmd/001_bits21/files/matrix'/name,QUAL/name)
context=json.loads((REAL/'cmd/000_prior/files/prior_context.json').read_text())
for name,row in context['copied'].items():
 src=REAL/'cmd/000_prior/files'/Path(row['path']).name
 dst=PRIOR/src.name;shutil.copyfile(src,dst);row['path']=str(dst)
for key in ['qualification_path','supplement_path','bits21_provenance_path']:
 context[key]=str(PRIOR/Path(context[key]).name)
(PRIOR/'prior_context.json').write_text(json.dumps(context))
manifest={'schema':'protocol-fake-inputs','cases':[]}
for name in ['lidar_ng00','lidar_ng01','lidar_ng02']:
 coord=DATA/(name+'.xyz');ids=DATA/(name+'.ids');coord.write_bytes(b'five-point-input');ids.write_bytes(b'five-point-ids')
 manifest['cases'].append({'name':name,'count':5,'coordinates':coord.name,'point_ids':ids.name,'sha256':paired.base.digest(coord),'ids_sha256':paired.base.digest(ids)})
(DATA/'manifest.json').write_text(json.dumps(manifest))
args=argparse.Namespace(source=SOURCE,builds=BUILDS,data=DATA,out=OUT,work=STAGE/'build/paired_work',qualification=QUAL/'summary.json',prior_context=PRIOR/'prior_context.json',budget_seconds=1320,build_timeout_seconds=180,build_threads=16)
real_load=paired.full.profiles.load
# The sole known parser defect is bypassed in this isolated harness; all other protocol code is unchanged.
def load(path):
 if Path(path).name=='tests.json':return json.loads(Path(path).read_text())
 return real_load(path)
real_run=subprocess.run
compile_calls=[]
def compile_or_child(argv,**kwargs):
 if argv[0]=='cmake':
  compile_calls.append(list(argv))
  if '--build' in argv:
   d=Path(argv[argv.index('--build')+1]);d.mkdir(parents=True,exist_ok=True)
   shutil.copyfile(CACHE,d/'CMakeCache.txt');binary=d/'mhgp11_full_bench';binary.write_text(CHILD);binary.chmod(0o755)
  return subprocess.CompletedProcess(argv,0,b'protocol simulated build\n',b'')
 return real_run(argv,**kwargs)
with patch.object(paired.full.profiles,'load',side_effect=load),patch.object(paired.full.profiles,'inputs',return_value=(manifest,paired.base.digest(DATA/'manifest.json'))),patch.object(paired.baseline.subprocess,'run',side_effect=compile_or_child):
 code=paired.run(args)
report=json.loads((OUT/'full_paired.json').read_text())
if code!=0 or report['conforming'] is not True or len(report['runs'])!=81:
 raise SystemExit(json.dumps({'exit':code,'conforming':report.get('conforming'),'runs':len(report['runs']),'bad':[r for r in report['runs'] if r['status']!='ok']},default=str)[:4000])
print(json.dumps({'ok':True,'scope':'protocol only; artificial five-point metadata, CMake simulated, Python children','runs':len(report['runs']),'compile_calls':len(compile_calls),'native_cpp':0,'source_unchanged':json.loads((OUT/'source_before.json').read_text())==json.loads((OUT/'source_after.json').read_text()),'cache_modes':sorted({r['semantic_reuse']['mode'] for r in report['runs']}),'equal_bytes':all(c['status']=='equal' for c in report['comparisons'])}))
