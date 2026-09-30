from pathlib import Path
import hashlib, json, subprocess, sys

def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def unique(items):
    out = {}
    for key, value in items:
        need(key not in out, 'duplicate key')
        out[key] = value
    return out

def parse(raw):
    return json.loads(raw, object_pairs_hook=unique)

root = Path(__file__).resolve().parent
files = {p.name:p.read_bytes() for p in root.iterdir() if p.is_file()}
need(set(files) == {'README.md','check.py','capture.json','manifest.json','read.py'}, 'inventory')
need(sha(files['manifest.json']) == '59c147c22c0cf6cbddd9442b15b56dd79c8815b6182efa7d3737f4779aa1e9f4', 'manifest')
manifest = parse(files['manifest.json'])
need(set(manifest) == {'schema','files'} and manifest['schema'] == 1, 'manifest schema')
need(set(manifest['files']) == {'README.md','check.py','capture.json'}, 'covered inventory')
for name, digest in manifest['files'].items():
    need(sha(files[name]) == digest, 'file: '+name)
cap = parse(files['capture.json'])
need(set(cap) == {'schema','start_utc','end_utc','source_before_sha256','source_after_sha256','commands'}, 'capture schema')
need(cap['schema'] == 1 and cap['source_before_sha256'] == cap['source_after_sha256'] == sha(files['check.py']), 'before/after')
need(len(cap['commands']) == 2, 'commands')
outputs = []
for index, row in enumerate(cap['commands']):
    options = ['-B'] + (['-O'] if index else [])
    need(set(row) == {'argv','exit_code','combined_output'}, 'command schema')
    need(row['argv'] == ['python3']+options+['/tmp/mmt-k5-annex.h6H8Cli0/check.py'] and row['exit_code'] == 0, 'argv/code')
    data = parse(row['combined_output'])
    need(data['status'] == 'PASS' and data['K'] == data['n'] == 5 and data['W_num_bits'] == 142, 'scope')
    result = subprocess.run([sys.executable]+options+['-c',files['check.py'].decode()], capture_output=True, text=True, timeout=10)
    need(result.returncode == 0 and result.stderr == '' and result.stdout == row['combined_output'], 'snapshot replay')
    outputs.append(result.stdout)
need(outputs[0] == outputs[1], 'normal/-O equality')
print(json.dumps({'status':'PASS','K':5,'W_num_bits':142,'native_execution':False},sort_keys=True))
