"""Four tiny existing-binary calls, audit only. No compilation or GCP."""
from pathlib import Path
import hashlib
import json
import subprocess
import time

P = Path(__file__).resolve().parent
OLD = Path('/tmp/mhgp10-r2/entrees_cli')
NEW = Path('/tmp/mhgp10-r2/tete-verif')
targets = {
    'entrees_binary': OLD/'build/mhgp10_cluster',
    'entrees_cli': OLD/'src/morsehgp3D_v10/cli/mhgp10_cluster.cpp',
    'entrees_outputs': OLD/'src/morsehgp3D_v10/src/core/cli_output.hpp',
    'entrees_head_header': OLD/'src/morsehgp3D_v10/src/head/head.hpp',
    'tete_binary': NEW/'build/mhgp10_cluster',
    'tete_cli': NEW/'src/morsehgp3D_v10/cli/mhgp10_cluster.cpp',
    'tete_head_source': NEW/'src/morsehgp3D_v10/src/head/head.cpp',
    'tete_head_header': NEW/'src/morsehgp3D_v10/src/head/head.hpp',
    'tete_configs': P/'late_config.txt',
    'input': P/'five.u32le',
    'probe': Path(__file__).resolve(),
}
def pin():
    return {key:hashlib.sha256(path.read_bytes()).hexdigest() for key,path in targets.items()}

before = pin()
calls = []
binary = str(targets['tete_binary'])
input_path = str(targets['input'])
cases = [
    ('control_k2', ['--k=2','--mcs=2','--threads=1']),
    ('numeric_refusal_k1', ['--k=1','--mcs=1','--threads=1']),
    ('late_config_numeric_refusal', ['--k=1','--configs='+str(P/'late_config.txt'),'--threads=1']),
    ('exact_collision', ['--k=2','--mcs=2','--threads=1','--tree='+str(P/'exact_collision.i32le')]),
]
for name,args in cases:
    out = P/(name+'.i32le')
    argv = [binary,input_path,str(out),*args]
    start = time.monotonic()
    r = subprocess.run(argv,capture_output=True,text=True,timeout=5)
    written = []
    for path in sorted(P.glob(name+'.i32le*')):
        raw = path.read_bytes()
        written.append({'name':path.name,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),
                        'prefix_hex':raw[:32].hex()})
    calls.append({'case':name,'argv':argv,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr,
                  'wall_seconds':time.monotonic()-start,'outputs':written})
after = pin()
control,refusal,late,collision = calls
refusal_line = '{"status":"unsupported_degeneracy","reason":"numeric_domain","k":1}'
findings = {
    'hashes_stable':before == after,
    'entrees_binary_still_equals_published_collision_binary':before['entrees_binary'] == 'a776427321ae27d3ccde41dffbf03fa8601144926a99ac727bb78a205bb04781',
    'control_writes_valid_size':control['returncode'] == 0 and len(control['outputs']) == 1 and control['outputs'][0]['bytes'] == 20,
    'numeric_outcome_propagated_to_refusal':refusal['returncode'] == 2 and refusal_line in refusal['stdout'].splitlines() and refusal['outputs'] == [],
    'later_configuration_refuses_before_any_output':late['returncode'] == 2 and refusal_line in late['stdout'].splitlines() and late['outputs'] == [],
    'collision_still_corrupts_labels_with_success':collision['returncode'] == 0 and len(collision['outputs']) == 1 and collision['outputs'][0]['bytes'] != 20 and collision['outputs'][0]['prefix_hex'].startswith('6c6576656c7320'),
}
print(json.dumps({'status':'OBSERVATION_CONFIRMED' if all(findings.values()) else 'OBSERVATION_DIFFERS',
                  'scope':'four tiny calls on existing tete-verif cluster only; entrees observed by same published hashes',
                  'native_calls':len(calls),'engine_built':False,'engine_modified':False,'GCP_used':False,
                  'targets':{key:str(path) for key,path in targets.items()},
                  'hashes_before':before,'hashes_after':after,'findings':findings,'calls':calls},indent=2))
