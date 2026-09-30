"""Hash-first portable reader; no native calls. Replays only the Fraction proof."""
from datetime import datetime
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parent
INVENTORY={'check.py','PROTOCOL.txt','record.py','README.md','verify.py','receipt.json',
           'transport_preflight.txt','logs/normal.stdout','logs/normal.stderr',
           'logs/optimized.stdout','logs/optimized.stderr'}
SOURCE={'check.py','PROTOCOL.txt','record.py'}
HISTORICAL='/tmp/mhgp10-uniform-band-contact-proof-20260930.J4jFCdDk'

def require(ok,message):
    if not ok: raise ValueError(message)

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def utc(value):
    parsed=datetime.fromisoformat(value)
    require(parsed.utcoffset() is not None and parsed.utcoffset().total_seconds()==0,'UTC missing')
    return parsed

def manifest():
    lines=(ROOT/'SHA256SUMS').read_text().splitlines()
    pins={}
    for line in lines:
        h,name=line.split('  ',1)
        require(len(h)==64 and all(c in '0123456789abcdef' for c in h),'bad SHA')
        require(name in INVENTORY and name not in pins,'unknown/duplicate inventory')
        pins[name]=h
    actual={str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}
    require(actual==INVENTORY|{'SHA256SUMS'} and set(pins)==INVENTORY,'inventory mismatch')
    require(all(sha(ROOT/n)==h for n,h in pins.items()),'hash mismatch')
    return pins

def validate_result(out):
    d=json.loads(out)
    require(set(d)=={'status','K','cases','catalogue_balls_per_case','gamma_vertices_per_case',
                   'gamma_hyperedges_per_case','reports','wrong_value_mutants','native_calls','GCP_used','scope'},'result schema')
    require(d['status']=='UNIFORM_BAND_CONTACT_DISCONTINUITY_PROVED' and d['K']==2 and d['cases']==11,'result count/status')
    require((d['catalogue_balls_per_case'],d['gamma_vertices_per_case'],d['gamma_hyperedges_per_case'])==(10,6,4),'Gamma/catalogue counts')
    require(type(d['native_calls']) is int and d['native_calls']==0 and d['GCP_used'] is False,'scope calls')
    require(d['scope']=='affine3D exact Fraction Gamma2; strong-vote removal, not sphere splitting, native or benchmark','scope')
    names=['base_shell','base_inward_1/16','base_inward_1/256','base_inward_1/4096',
           'base_inward_1/65536','u18_shell_1024','u18_jitter1_1024',
           'family_M3_shell','family_M3_inward','family_M32_shell','family_M32_inward']
    require([r['name'] for r in d['reports']]==names,'case inventory')
    for r in d['reports']:
        interior=F(r['delta'])>0
        require(F(r['determinant'])>0 and F(r['minimum_squared_band_gap'])>0,'dimension/margin')
        require(r['vote_count_a']==(2 if interior else 3) and r['ab_p']==(1 if interior else 0),'vote census')
        require(r['ab_qmin']==2 and r['ab_strong'] is (not interior),'strong condition')
        require(r['entry_a']==r['height_ab'] and r['parts_entry_a']==r['parts_height_ab'],'owner/height')
        require(F(r['parts_height_ab'])==25*F(r['scale'])**2 if r['M'] is None else
                F(r['parts_height_ab'])==(r['M']**2+1)**2,'K-part control')
        require(len(r['full_gamma'])>=2 and all(len(c['components'])==len(c['covers']) for c in r['full_gamma']),'trace')
    require(d['wrong_value_mutants']==[
        {'case':'base_inward_1/16','correct_height':'43489/1024','kind':'drop_weak_abz_event','wrong_height':'181/4'},
        {'case':'base_shell','correct_height':'25','kind':'component_dedup','wrong_height':'171/4'}],'wrong-value mutants')
    return d

def main():
    pins=manifest()
    r=json.loads((ROOT/'receipt.json').read_text())
    require(set(r)=={'GCP_used','closed_utc','commands','frozen_utc','native_calls','schema',
                   'sources_after','sources_before','status'},'receipt schema')
    require(r['schema']=='mhgp10.uniform_band_contact.v1' and r['status']=='CAPTURED','receipt status')
    require(type(r['native_calls']) is int and r['native_calls']==0 and r['GCP_used'] is False,'receipt scope')
    require(set(r['sources_before'])==SOURCE and set(r['sources_after'])==SOURCE,'source pin inventory')
    require(r['sources_before']==r['sources_after']=={n:pins[n] for n in SOURCE},'source pins')
    require(len(r['commands'])==2,'command inventory')
    frozen,closed=utc(r['frozen_utc']),utc(r['closed_utc'])
    previous=frozen
    previous_ns=None
    for i,name in enumerate(('normal','optimized')):
        c=r['commands'][i]
        require(set(c)=={'argv','code','end_ns','end_utc','name','start_ns','start_utc','stderr','stdout'},'command schema')
        require(c['name']==name and type(c['code']) is int and c['code']==0,'command code/name')
        require(c['argv']==['/home/codespace/.python/current/bin/python3','-B']+(['-O'] if i else [])+[HISTORICAL+'/check.py'],'command argv')
        start,end=utc(c['start_utc']),utc(c['end_utc'])
        require(previous<=start<=end<=closed,'timestamp order')
        require(type(c['start_ns']) is int and type(c['end_ns']) is int and 0<c['start_ns']<=c['end_ns'],'monotonic time')
        require(previous_ns is None or previous_ns<=c['start_ns'],'monotonic command order')
        previous=end;previous_ns=c['end_ns']
        stdout=(ROOT/f'logs/{name}.stdout').read_bytes()
        stderr=(ROOT/f'logs/{name}.stderr').read_bytes()
        require(stdout==c['stdout'].encode() and stderr==c['stderr'].encode() and stderr==b'','capture relation')
        require(len(stdout)==72727,'capture size')
        validate_result(c['stdout'])
    require(r['commands'][0]['stdout']==r['commands'][1]['stdout'],'normal/O acquisition differs')
    # Replay paths are relocatable. Sources already hashed, no imported private engine.
    for optimized in (False,True):
        argv=[sys.executable,'-B']+(['-O'] if optimized else [])+[str(ROOT/'check.py')]
        p=subprocess.run(argv,cwd=ROOT,capture_output=True,check=False,timeout=20)
        require(p.returncode==0 and not p.stderr and p.stdout==r['commands'][0]['stdout'].encode(),'proof replay differs')
    require(manifest()==pins,'archive changed during replay')
    print(json.dumps({'status':'UNIFORM_BAND_CONTACT_ARCHIVE_PASS','files':len(pins),
                      'proof_cases':11,'wrong_value_mutants':2,'replays':2,'native_calls':0,'GCP_used':False},sort_keys=True))

if __name__=='__main__': main()
