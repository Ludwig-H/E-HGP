from pathlib import Path
import json,subprocess,hashlib,datetime,platform,sys
out=Path(__file__).resolve().parent
live=Path('/workspaces/E-HGP/build/v11-development-20261002')
baseline='9df77494732b03ddf11dbcf1dcb11d96bef54a3b'
pin=json.loads((out/'SOURCE_BEFORE.json').read_text())['head']
def show(commit,path): return subprocess.check_output(['git','-C',str(live),'show',commit+':'+path])
def sha(b): return hashlib.sha256(b).hexdigest()
rows=[]
for p in sorted((out/'sources/morsehgp3D_v11/src/num').iterdir()):
 rel='morsehgp3D_v11/src/num/'+p.name
 b=show(baseline,rel); dest=out/'baseline9df'/rel; dest.parent.mkdir(parents=True,exist_ok=True); dest.write_bytes(b)
 rows.append({'path':rel,'sha256':sha(b),'size':len(b)})
extra=[]
for rel in ('morsehgp3D_v11/README.md','morsehgp3D_v11/docs/PROVENANCE.md'):
 b=show(pin,rel); dest=out/'sources'/rel; dest.parent.mkdir(parents=True,exist_ok=True); dest.write_bytes(b)
 liveb=(live/rel).read_bytes()
 extra.append({'path':rel,'sha256':sha(b),'size':len(b),'live_matches_pin':liveb==b})
(out/'BASELINE.json').write_text(json.dumps({'commit':baseline,'files':rows},indent=2)+'\n')
(out/'SOURCE_EXTRA_BEFORE.json').write_text(json.dumps({'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commit':pin,'sources':extra},indent=2)+'\n')
(out/'DIFF_num.patch').write_bytes(subprocess.check_output(['git','-C',str(live),'diff',baseline,pin,'--','morsehgp3D_v11/src/num','morsehgp3D_v11/tests/num/candidate_test.cpp','morsehgp3D_v11/tests/num/probe.cpp','morsehgp3D_v11/tests/num/fraction_oracle.py','morsehgp3D_v11/tests/mutants/num.json']))
checks=[]
for row in json.loads((out/'SOURCE_BEFORE.json').read_text())['sources']:
 checks.append({'path':row['path'],'captured_matches_pin':sha(show(pin,row['path']))==row['sha256']})
(out/'PIN_CHECK.json').write_text(json.dumps({'commit':pin,'checks':checks},indent=2)+'\n')
if not all(x['captured_matches_pin'] for x in checks): raise RuntimeError('Snapshot differs from git pin')
(out/'ENVIRONMENT.json').write_text(json.dumps({'python':sys.version,'platform':platform.platform(),'native_executed':False,'GCP_used':False},indent=2)+'\n')
print(json.dumps({'baseline':baseline,'baseline_files':len(rows),'git_pin_checks':len(checks),'extra':len(extra)}))
