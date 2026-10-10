#!/usr/bin/env python3
"""Replay saved final G4 status metadata; no cloud or engine invocation."""
from pathlib import Path
import hashlib,json,sys
def need(ok,msg):
    if not ok:raise ValueError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()
here=Path(__file__).resolve().parent
need(len(sys.argv)==3,'usage: check.py SNAPSHOT RECEIPT_MESB1O')
snap,receipt=map(Path,sys.argv[1:]);cap=json.loads((here/'capture.json').read_text())
for name,h in cap['raw_files'].items():
    need(Path(name).name==name and sha((snap/name).read_bytes())==h,'raw pin '+name)
r=json.loads(receipt.read_text());target=r['target'];generation=r['generation']
need(sha(json.dumps(target,sort_keys=True).encode())==cap['target_sha256'],'target pin')
need(sha(generation.encode())==cap['generation_sha256'],'generation pin')
for phase in ['before','after']:
    v=json.loads((snap/(phase+'.stdout')).read_text())
    need(v['name']==target['instance'] and v['zone'].split('/')[-1]==target['zone'],'target identity')
    need(v['lastStartTimestamp']==generation and v.get('labels',{}).get('project')=='e-hgp','generation and label')
    need(v['status']==cap[phase]=='TERMINATED','terminated')
need(cap['guard_exit_code']==0 and cap['stop_requested'] is False,'guard result')
need((snap/'guard.stderr').read_bytes()==b'','guard stderr')
inv=json.loads((snap/'inventory.stdout').read_text())
need(sum(x['status']!='TERMINATED' for x in inv)==cap['active_ehgp_instances']==0,'active inventory')
print(json.dumps({'observed_utc':cap['observed_utc'],'before':cap['before'],'after':cap['after'],'active_ehgp_instances':0,'stop_requested':False,'replay_only':True}))
