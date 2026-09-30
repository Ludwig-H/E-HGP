from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
MANIFEST_SHA = 'ead3d95007524f53748e3750c232729e46847a6f51908ceabc5a5e15142dccc4'
SOURCE_SHA = '4d15bde450b6bbde3a761a9ea70cafaed0bbefd0793eef04d38d841503ad1428'

def need(ok, label):
    if not ok:
        raise RuntimeError(label)

def unique_pairs(pairs):
    value = {}
    for key, entry in pairs:
        need(key not in value, 'duplicate JSON key')
        value[key] = entry
    return value

def parse(raw):
    return json.loads(raw, object_pairs_hook=unique_pairs)

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

files = {p.name: p.read_bytes() for p in ROOT.iterdir() if p.is_file()}
need(set(files) == {'README.md','check.py','capture.json','manifest.json','read.py'}, 'packet inventory')
need(sha(files['manifest.json']) == MANIFEST_SHA, 'manifest pin')
manifest = parse(files['manifest.json'])
need(set(manifest) == {'schema','scope','files'} and manifest['schema'] == 1, 'manifest schema')
need(manifest['scope'] == 'Fraction_fixture_only_not_native', 'scope')
need(set(manifest['files']) == {'README.md','check.py','capture.json'}, 'covered inventory')
for name, digest in manifest['files'].items():
    need(sha(files[name]) == digest, 'file hash: '+name)
need(sha(files['check.py']) == SOURCE_SHA, 'source pin')
capture = parse(files['capture.json'])
need(set(capture) == {'schema','scope','acquisition_start_utc','acquisition_end_utc','cwd','source_before_sha256','source_after_sha256','commands'}, 'capture schema')
need(capture['schema'] == 1 and capture['scope'] == manifest['scope'], 'capture scope')
need(capture['source_before_sha256'] == capture['source_after_sha256'] == SOURCE_SHA, 'source before/after')
need(capture['cwd'] == '/workspaces/E-HGP', 'capture cwd')
need(capture['acquisition_start_utc'] == '2026-09-30 18:05:34 UTC' and capture['acquisition_end_utc'] == '2026-09-30 18:05:35 UTC', 'acquisition times')
need(len(capture['commands']) == 2, 'two captured commands')
outputs = []
source = files['check.py'].decode('utf-8')
for index, record in enumerate(capture['commands']):
    need(set(record) == {'argv','exit_code','combined_output','wall_time_seconds'}, 'command schema')
    options = ['-B'] + (['-O'] if index else [])
    need(record['argv'] == ['python3']+options+['/tmp/q12-band-audit.5GR9R6SK/check.py'], 'captured argv')
    need(record['exit_code'] == 0, 'captured return code')
    need(isinstance(record['wall_time_seconds'], (int,float)) and record['wall_time_seconds'] >= 0, 'wall time')
    output = parse(record['combined_output'])
    need(output['status'] == 'PASS' and output['K'] == 3 and output['eta_prime'] == '1/8', 'fixture scope')
    need(output['classes'] == output['distinct_centers'] == len(output['rows']) == 15 and output['points_in_cap'] == 6, 'fixture inventory')
    # Re-run the already pinned immutable bytes, not a mutable source path.
    replay = subprocess.run([sys.executable]+options+['-c',source], capture_output=True, text=True, timeout=10)
    need(replay.returncode == 0 and replay.stderr == '', 'replay return code/stderr')
    need(replay.stdout == record['combined_output'], 'replay exact output')
    outputs.append(record['combined_output'])
need(outputs[0] == outputs[1], 'normal/-O equality')
print(json.dumps({'status':'PASS','classes':15,'captured_commands':2,'replayed_commands':2,'native':False,'manifest_sha256':MANIFEST_SHA}, sort_keys=True))
