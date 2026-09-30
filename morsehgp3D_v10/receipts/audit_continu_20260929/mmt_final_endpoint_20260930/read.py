from pathlib import Path
import base64, hashlib, json, subprocess, sys

def need(ok,label):
    if not ok:
        raise RuntimeError(label)

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def unique(items):
    result = {}
    for key,value in items:
        need(key not in result,'duplicate JSON key')
        result[key] = value
    return result

def parse(raw):
    return json.loads(raw,object_pairs_hook=unique)

root = Path(__file__).resolve().parent
files = {p.name:p.read_bytes() for p in root.iterdir() if p.is_file()}
need(set(files) == {'README.md','check.py','mmt_snapshot.py','capture.json','manifest.json','read.py'},'inventory')
need(sha(files['manifest.json']) == 'b349515d14eb9e2008824baed744cc2aaf03cc5fb46b3ed53c94229a74fdb525','manifest pin')
manifest = parse(files['manifest.json'])
need(set(manifest) == {'schema','files'} and manifest['schema'] == 1,'manifest schema')
need(set(manifest['files']) == {'README.md','check.py','mmt_snapshot.py','capture.json'},'covered inventory')
for name,digest in manifest['files'].items():
    need(sha(files[name]) == digest,'file hash: '+name)
cap = parse(files['capture.json'])
need(set(cap) == {'schema','start_utc','end_utc','pins_before','pins_after','commands'} and cap['schema'] == 1,'capture schema')
need(cap['start_utc'] == cap['end_utc'] == '2026-09-30 19:52:13 UTC','times')
need(cap['pins_before'] == cap['pins_after'],'before/after pins')
expected = (
    '3869463015e46eb54957eaa103ccfb46660714bb8fc70a810c20f058330f548e  /tmp/mmt-final-endpoint.ysBNJToa/check.py\n'
    '93f6acd0de4146a20bc8078c7d339468a5f118a331e8ac8e39f4107021f52818  /tmp/mmt-final-endpoint.ysBNJToa/mmt_snapshot.py\n'
    '93f6acd0de4146a20bc8078c7d339468a5f118a331e8ac8e39f4107021f52818  build/v10-verrou-points/revision_cible/majorites_continues/mmt.py\n')
need(cap['pins_before'] == expected,'source inventory and identities')
need(len(cap['commands']) == 2,'commands')
outputs = []
for index,row in enumerate(cap['commands']):
    options = ['-B']+(['-O'] if index else [])
    need(set(row) == {'argv','exit_code','combined_output'},'command schema')
    need(row['argv'] == ['python3']+options+['/tmp/mmt-final-endpoint.ysBNJToa/check.py'] and row['exit_code'] == 0,'argv/code')
    data = parse(row['combined_output'])
    need(set(data) == {'status','scope','source_sha256','cloud','K','rows'},'output schema')
    need(data['status'] == 'PASS' and data['K'] == 2 and data['cloud'] == [[0,0,0],[2,0,0]],'geometry scope')
    need(data['source_sha256'] == sha(files['mmt_snapshot.py']),'actual source pin')
    need(len(data['rows']) == 6,'three parameters/two paths')
    need([r['agrees'] for r in data['rows']] == [False,True,True,True,True,True],'causal mismatch/control')
    need(data['rows'][0]['actual'] == {'4/3':'1'} and data['rows'][0]['analytic_sup'] == {'1':'-1/10','5/3':'1'},'exact endpoint values')
    need(data['rows'][1]['actual'] == data['rows'][1]['analytic_sup'] == {'1':'139/120'},'critical-point control')
    need(data['rows'][2]['actual'] == data['rows'][2]['analytic_sup'] == {'4/3':'1'},'recommended-parameter control')
    need(data['rows'][3]['actual'] == data['rows'][0]['analytic_sup'],'endpoint positive control')
    replay = subprocess.run([sys.executable]+options+['-c',files['check.py'].decode(),'--snapshot-b64',base64.b64encode(files['mmt_snapshot.py']).decode()],capture_output=True,text=True,timeout=10)
    need(replay.returncode == 0 and replay.stderr == '' and replay.stdout == row['combined_output'],'immutable snapshot replay')
    outputs.append(replay.stdout)
need(outputs[0] == outputs[1],'normal/-O identity')
print(json.dumps({'status':'PASS','endpoint_mismatch':1,'unchanged_controls':2,'positive_control_fixed':True,'native':False},sort_keys=True))
