from pathlib import Path
import hashlib, json, subprocess, sys

def need(ok, message):
    if not ok:
        raise RuntimeError(message)

def pairs(items):
    result = {}
    for key, value in items:
        need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result

def parse(raw):
    return json.loads(raw, object_pairs_hook=pairs)

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

root = Path(__file__).resolve().parent
files = {p.name:p.read_bytes() for p in root.iterdir() if p.is_file()}
need(set(files) == {'README.md','check.py','capture.json','manifest.json','read.py'}, 'inventory')
need(sha(files['manifest.json']) == '028a1de5ddf62e7e2f8f79455fd1de944d686d28c973c654cc86c871aa746d66', 'manifest pin')
manifest = parse(files['manifest.json'])
need(set(manifest) == {'schema','files'} and manifest['schema'] == 1, 'manifest schema')
need(set(manifest['files']) == {'README.md','check.py','capture.json'}, 'covered inventory')
for name, digest in manifest['files'].items():
    need(sha(files[name]) == digest, 'hash: '+name)
capture = parse(files['capture.json'])
need(set(capture) == {'schema','scope','acquisition_start_utc','acquisition_end_utc','source_before_sha256','source_after_sha256','commands'}, 'capture schema')
need(capture['schema'] == 1 and capture['scope'] == 'exact_geometric_MEB_and_one_branch_MMt_not_native_execution', 'scope')
need(capture['source_before_sha256'] == capture['source_after_sha256'] == sha(files['check.py']), 'source before/after')
need(capture['acquisition_start_utc'] == '2026-09-30 18:30:13 UTC' and capture['acquisition_end_utc'] == '2026-09-30 18:30:14 UTC', 'acquisition times')
need(len(capture['commands']) == 2, 'capture count')
outputs = []
for index, record in enumerate(capture['commands']):
    need(set(record) == {'argv','exit_code','combined_output','wall_time_seconds'}, 'command schema')
    options = ['-B'] + (['-O'] if index else [])
    need(record['argv'] == ['python3']+options+['/tmp/mmt-numeric-audit.P1edfJiD/check.py'], 'argv')
    need(record['exit_code'] == 0, 'captured code')
    result = parse(record['combined_output'])
    need(result['status'] == 'PASS' and result['K'] == result['n'] == 4, 'fixture')
    need(result['beta_num_bits'] == result['W_num_bits'] == 142, 'overflow scope')
    # Replay a private immutable snapshot via -c, not the live source path.
    live = subprocess.run([sys.executable]+options+['-c',files['check.py'].decode()], capture_output=True, text=True, timeout=10)
    need(live.returncode == 0 and live.stderr == '' and live.stdout == record['combined_output'], 'exact replay')
    outputs.append(live.stdout)
need(outputs[0] == outputs[1], 'normal/-O equality')
print(json.dumps({'status':'PASS','commands':2,'W_num_bits':142,'native_execution':False}, sort_keys=True))
