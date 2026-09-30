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
        need(key not in out, 'duplicate JSON key')
        out[key] = value
    return out

def parse(raw):
    return json.loads(raw,object_pairs_hook=unique)

root = Path(__file__).resolve().parent
files = {p.name:p.read_bytes() for p in root.iterdir() if p.is_file()}
need(set(files) == {'README.md','check.py','capture.json','manifest.json','read.py'}, 'inventory')
need(sha(files['manifest.json']) == '53958feccd27ca33efce6f8b286d43e17e2dcaae6e051cee78038426f2f52063', 'manifest pin')
manifest = parse(files['manifest.json'])
need(set(manifest) == {'schema','files'} and manifest['schema'] == 1, 'manifest schema')
need(set(manifest['files']) == {'README.md','check.py','capture.json'}, 'covered inventory')
for name, digest in manifest['files'].items():
    need(sha(files[name]) == digest, 'file hash: '+name)
cap = parse(files['capture.json'])
need(set(cap) == {'schema','scope','start_utc','end_utc','source_before_sha256','source_after_sha256','commands'}, 'capture schema')
need(cap['schema'] == 1 and cap['scope'] == 'abstract_rational_cover_trees_not_native_geometry', 'scope')
need(cap['start_utc'] == '2026-09-30 19:10:56 UTC' and cap['end_utc'] == '2026-09-30 19:10:57 UTC', 'acquisition times')
need(cap['source_before_sha256'] == cap['source_after_sha256'] == sha(files['check.py']), 'source before/after')
need(len(cap['commands']) == 10, 'reference and mutant inventory')
variants = ['', 'early-activation','ignore-virtual-parent','wrong-median-join','open-instead-closed']
labels = ['', 'W compression','W compression','valid covering-line slope','component masses compression']
references = []
mutants = []
source = files['check.py'].decode()
for index, row in enumerate(cap['commands']):
    need(set(row) == {'argv','exit_code','combined_output','wall_time_seconds'}, 'command schema')
    variant = variants[index%5]
    options = ['-B'] + (['-O'] if index >= 5 else [])
    args = [variant] if variant else []
    expected_argv = ['python3']+options+['/tmp/mmt-cover-virtual.wpxN2rCm/check.py']+args
    need(row['argv'] == expected_argv, 'captured argv')
    need(isinstance(row['wall_time_seconds'],(float,int)) and row['wall_time_seconds'] >= 0, 'wall time')
    expected_code = 3 if variant else 0
    need(row['exit_code'] == expected_code, 'captured return code')
    data = parse(row['combined_output'])
    if variant:
        need(set(data) == {'status','label','case','mutant'}, 'mutant output schema')
        need(data['status'] == 'MISMATCH' and data['mutant'] == variant and data['label'] == labels[index%5], 'causal mutant mismatch')
        mutants.append(row['combined_output'])
    else:
        need(set(data) == {'status','scope','cases','comparisons','per_seed_path_walks_in_compressor','rows'}, 'reference output schema')
        need(data['status'] == 'PASS' and data['scope'] == cap['scope'] and data['cases'] == len(data['rows']) == 108, 'case inventory')
        need(data['comparisons'] == sum(r['comparisons'] for r in data['rows']) == 15360, 'comparison inventory')
        need(data['per_seed_path_walks_in_compressor'] == 0, 'no per-seed path walk')
        for r in data['rows']:
            need(r['virtual_nodes'] <= 2*r['seeds_distinct'] and r['adjacent_lca_queries'] <= r['seeds_distinct']-1, 'virtual-size/LCA bounds')
        references.append(row['combined_output'])
    # Immutable snapshot; no import or execution of mutable shared sources.
    replay = subprocess.run([sys.executable]+options+['-c',source]+args,capture_output=True,text=True,timeout=10)
    need(replay.returncode == expected_code and replay.stderr == '' and replay.stdout == row['combined_output'], 'exact replay')
need(references[0] == references[1] and mutants[:4] == mutants[4:], 'normal/-O equality')
print(json.dumps({'status':'PASS','cases':108,'comparisons':15360,'mutants_killed':4,'replayed_commands':10,'native':False},sort_keys=True))
