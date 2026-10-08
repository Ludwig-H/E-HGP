import argparse,copy,json,sys,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description='Rejeu Python seul, sources externes epinglees ; aucune sonde.')
parser.add_argument('--sources',type=Path,required=True)
parser.add_argument('--fixture',type=Path,required=True)
parser.add_argument('--check',action='store_true')
args=parser.parse_args()
cap=json.loads((HERE/'capture.json').read_text())
for name,pin in cap['sources'].items():
 b=(args.sources/name).read_bytes()
 if len(b)!=pin['bytes'] or hashlib.sha256(b).hexdigest()!=pin['sha256']:raise RuntimeError('source drift: '+name)
sys.path.insert(0,str(args.sources))
import g4_catalogue_flux_judge as J
import g4_catalogue_flux_lecteur as L
import g4_catalogue_flux_selftest as S
import g4_catalogue_flux as P

def judge(name,report):
 out=J.judge(report)
 return dict(case=name,verdict=out['verdict'],mutant=out['stats'].get('mutant'),refused=len(out['refused']),rejected=len(out['rejected']),first=(out['refused']+out['rejected'])[:1])
res={'developer_cases':[],'independent':[]}
for name,report,expected in S.cases():
 got=judge(name,report)
 if got['verdict']!=expected:raise RuntimeError('developer test differs: '+name)
 res['developer_cases'].append([name,got['verdict']])
# The base producer fixture stays byte-for-byte unchanged. Only subprocess output is supplied by a stub.
fixture=args.fixture
if True:
 b=fixture.read_bytes()
 if hashlib.sha256(b).hexdigest()!='3b6c4261acd93fff79018706d9bd2c9b91e4a1d5fdc519dcf392b1982c9b365e':raise RuntimeError('fixture drift')
 class Stub:
  data='synthetic_data'
  def run(self,*args):return {'stdout':b.decode(),'code':0,'timeout':False,'seconds':0.0}
 spec=L.catalogue_spec('device',5,48,3,True,True)
 run=P.probe_run(Stub(),'fixture','not_executed','ng00',spec)
 state,why,_=L.read_catalogue(run,spec)
 res['fixture_cpu_as_device']={'sha256':hashlib.sha256(b).hexdigest(),'state':state,'why':why}
for name in ['nominal','mutant_unknown_reason','mutant_failed_metadata','mutant_status_reason_mismatch','extra_campaign_arm','ful1_bool_code','ful1_bool_release']:
 r=S.synthetic_report()
 if name in ('mutant_unknown_reason','mutant_failed_metadata','mutant_status_reason_mismatch'):
  S.mutant_invariant(r)
  rows=r['steps']['mutant']['run']['rows']
  if name=='mutant_unknown_reason':
   rows[1]['reason']=rows[2]['reason']='not_a_reason'
  elif name=='mutant_status_reason_mismatch':
   rows[1]['reason']=rows[2]['reason']='memory_budget'
  else:
   rows[1].update(path='cpu',coord_bits=18,kmax=10,threads=1,leaf=16,wall_ns=False)
   rows[1]['pass']=False
 elif name=='extra_campaign_arm':
  e=copy.deepcopy(r['steps']['campaign'][0]);e['arm']='unknown_arm';r['steps']['campaign'].append(e)
 elif name=='ful1_bool_code':
  S.entry(r,'ful1',arm='apres',case='ng00',k=5)['run']['code']=False
 elif name=='ful1_bool_release':
  S.entry(r,'ful1',arm='apres',case='ng00',k=5)['run']['rows'][2]['pass']=False
 res['independent'].append(judge(name,r))
if any(r['verdict']!='adopte' for r in res['independent']):raise RuntimeError('counterexample result differs')
if args.check:
 if res!=json.loads((HERE/'results.json').read_text()):raise RuntimeError('results differ')
 print('verified: 39 developer cases, native historical fixture refused, 7 independent reports')
else:
 print(json.dumps(res,ensure_ascii=False,sort_keys=True,indent=2))
