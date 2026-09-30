"""Portable hash-first archive reader: no compilation or native invocation."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parent
OLD='/tmp/mhgp10-point-plateau-condensation-20260930.6a2Rhm5t'
BINS='/tmp/mhgp10-point-plateau-binaries-20260930.vQNNp8mf'
COMMIT='8bb4618e5c6c7c2bc8a5d7a1c2e189a4249d0047'
COMPILER='/usr/bin/x86_64-linux-gnu-g++-13'
SOURCE={
 'source/head/head.cpp':'f583da400d00571a547989a46b1690f2bb093e9e068a01897078363a92674578',
 'source/head/head.hpp':'f2710f309a201bc1094dcb49af7acc418ca33b1887f8373d8934f965d072c5e9',
 'source/points/dendrogram.cpp':'44711c93473cc31b675b5da29c07645d7c8a7cf686b540681a3da7e7b7c673d4',
 'source/points/dendrogram.hpp':'3162508b979572a6a4da0b3f0c445b09baa5e2fd77ba826a8d57b25711c42d16',
 'source/core/status.hpp':'f6983f95f60195eb5b5f73f73d9cc97b48d694d0877efd226072d6240cf84038',
 'source/core/types.hpp':'716da6aa079e46d1f9fde16e19a777228ec472bb1756ad0d7fede4655125da4c',
 'source/core/reasons.def':'0679dd4d0b4df946aa79568fd42cab744cea2c6c39bf9a3bd92a7698921a797b',
}
NATIVE_NAMES=['compiler','compile_normal','native_normal','compile_ubsan','native_ubsan']
JUDGE_NAMES=['judge_normal','judge_normal_O','judge_ubsan','judge_ubsan_O']
PINNED=set(SOURCE)|{'PROTOCOL.txt','probe.cpp','judge.py','record.py'}
FILES=PINNED|{'README.md','verify.py','run_judges.py','receipt.json','judge_runs.json',
             'preflight/receipt.json','preflight/probe.cpp','reader_preflight.json'}
FILES|={f'logs/{name}.{suffix}' for name in NATIVE_NAMES+JUDGE_NAMES for suffix in ('stdout','stderr')}
FILES|={f'preflight/logs/{name}.{suffix}' for name in ('compiler','compile_normal') for suffix in ('stdout','stderr')}

def require(ok,message):
    if not ok: raise ValueError(message)

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def load(n): return json.loads((ROOT/n).read_text())
def digest(s): return type(s) is str and re.fullmatch('[0-9a-f]{64}',s) is not None

def inventory():
    lines=(ROOT/'SHA256SUMS').read_text().splitlines(); pins={}
    for line in lines:
        m=re.fullmatch(r'([0-9a-f]{64})  ([A-Za-z0-9_./-]+)',line)
        require(m is not None,'bad manifest line')
        value,name=m.groups()
        require(name not in pins and name in FILES,'manifest inventory')
        pins[name]=value
    require(set(pins)==FILES,'manifest set incomplete')
    actual={str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}
    require(actual==FILES|{'SHA256SUMS'},'actual archive inventory')
    for name,value in pins.items(): require(sha(ROOT/name)==value,'SHA '+name)
    return pins

def stamps(c,previous=-1):
    require(type(c['start_ns']) is int and type(c['end_ns']) is int
            and previous<=c['start_ns']<=c['end_ns'],'monotonic stamps')
    a,b=dt.datetime.fromisoformat(c['start_utc']),dt.datetime.fromisoformat(c['end_utc'])
    require(a.utcoffset()==dt.timedelta(0) and b.utcoffset()==dt.timedelta(0) and a<=b,'UTC stamps')
    return c['end_ns']

def compile_argv(mode):
    flags=['-O2'] if mode=='normal' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    return [COMPILER,'-std=c++20',*flags,'-Wall','-Wextra','-Wpedantic','-Werror',
            '-I'+OLD+'/source',OLD+'/probe.cpp',OLD+'/source/head/head.cpp',
            OLD+'/source/points/dendrogram.cpp','-o',BINS+'/plateau_'+mode]

def native_receipt(r,pre=False):
    require(r['schema']=='mhgp10.point_plateau.native.v1' and r['source_commit']==COMMIT,'native schema/commit')
    require(r['GCP_used'] is False and r['shared_sources_modified'] is False,'native scope')
    require(set(r['sources_before'])==PINNED and r['sources_before']==r['sources_after'],'native source inventory/pins')
    for name,value in r['sources_before'].items():
        target='preflight/probe.cpp' if pre and name=='probe.cpp' else name
        require(sha(ROOT/target)==value,'source pinned '+target)
    for name,value in SOURCE.items(): require(r['sources_before'][name]==value,'Git source pin')
    require(r['compiler']==COMPILER and digest(r['compiler_before'])
            and r['compiler_before']==r['compiler_after'],'compiler pins')
    names=['compiler','compile_normal'] if pre else NATIVE_NAMES
    require([c['name'] for c in r['commands']]==names,'native command set/order')
    argv={'compiler':[COMPILER,'--version'],'compile_normal':compile_argv('normal'),
          'compile_ubsan':compile_argv('ubsan'),
          'native_normal':[BINS+'/plateau_normal'],'native_ubsan':[BINS+'/plateau_ubsan']}
    previous=-1; prefix='preflight/' if pre else ''
    for c in r['commands']:
        require(c['argv']==argv[c['name']],'native historical argv')
        previous=stamps(c,previous)
        require(type(c['code']) is int and c['code']==(1 if pre and c['name']=='compile_normal' else 0),'native code')
        for s in ('stdout','stderr'):
            require((ROOT/(prefix+'logs/'+c['name']+'.'+s)).read_text()==c[s],'linked native log')
        if not (pre and c['name']=='compile_normal'): require(c['stderr']=='','native stderr')
        if c['name'].startswith('compile_'): require(c['stdout']=='','compile stdout')
    frozen=dt.datetime.fromisoformat(r['source_frozen_utc']); closed=dt.datetime.fromisoformat(r['closed_utc'])
    require(frozen<=dt.datetime.fromisoformat(r['commands'][0]['start_utc'])
            <=dt.datetime.fromisoformat(r['commands'][-1]['end_utc'])<=closed,'receipt UTC bounds')
    if pre:
        require(r['status']=='FAIL' and type(r['native_invocations']) is int and r['native_invocations']==0
                and r['binary_hashes']=={},'preflight status')
        require(r['errors']==[{'type':'ValueError','error':'command failed: compile_normal'}],'preflight error')
        require('misleading-indentation' in r['commands'][1]['stderr'],'preflight cause')
    else:
        require(r['status']=='CAPTURED' and r['errors']==[] and type(r['native_invocations']) is int
                and r['native_invocations']==2,'native status')
        require(set(r['binary_hashes'])=={'normal','ubsan'},'binary inventory')
        for mode,b in r['binary_hashes'].items():
            require(b['path']==BINS+'/plateau_'+mode and digest(b['before']) and b['before']==b['after'],'binary pins')
        require(r['commands'][2]['stdout']==r['commands'][4]['stdout'],'native captures differ')

def main():
    pins=inventory()
    native_receipt(load('receipt.json'))
    native_receipt(load('preflight/receipt.json'),True)
    r=load('judge_runs.json')
    require(r['schema']=='mhgp10.point_plateau.judges.v1' and r['status']=='CAPTURED'
            and type(r['new_native_calls']) is int and r['new_native_calls']==0,'judge scope')
    names={'judge.py','run_judges.py','logs/native_normal.stdout','logs/native_ubsan.stdout'}
    require(set(r['sources_before'])==names and r['sources_before']==r['sources_after'],'judge pins inventory')
    for name,value in r['sources_before'].items(): require(sha(ROOT/name)==value,'judge pins '+name)
    require([c['name'] for c in r['commands']]==JUDGE_NAMES,'judge command set/order')
    previous=-1; outputs=[]
    for c in r['commands']:
        name=c['name']; mode='ubsan' if 'ubsan' in name else 'normal'; opt=name.endswith('_O')
        require(type(c['code']) is int and c['code']==0 and c['stderr']=='','judge code/diagnostic')
        require(c['argv'][1:]==['-B']+(['-O'] if opt else [])+[OLD+'/judge.py',OLD+'/logs/native_'+mode+'.stdout']
                and Path(c['argv'][0]).name=='python3','judge historical argv')
        previous=stamps(c,previous)
        for s in ('stdout','stderr'): require((ROOT/('logs/'+name+'.'+s)).read_text()==c[s],'linked judge log')
        report=json.loads(c['stdout'])
        require(report['status']=='EXPECTED_PLATEAU_API_NONINVARIANCE' and report['pair_checks']==576
                and report['equal_cut_checks']==20 and report['rows']==4 and report['new_native_calls']==0
                and report['GCP_used'] is False,'judge result scope')
        outputs.append(c['stdout'])
        replay=subprocess.run([sys.executable,'-B']+(['-O'] if opt else [])+
                              [str(ROOT/'judge.py'),str(ROOT/('logs/native_'+mode+'.stdout'))],
                              cwd=ROOT,text=True,capture_output=True,check=False,timeout=10)
        require(replay.returncode==0 and replay.stderr=='' and replay.stdout==c['stdout'],'judge replay')
    require(len(set(outputs))==1,'judge outputs differ')
    require(inventory()==pins,'archive changed during replay')
    print(json.dumps({'status':'PLATEAU_ARCHIVE_PASS','files':len(FILES),'native_replays':0,
                      'archive_judge_replays':4,'GCP_used':False},sort_keys=True))

if __name__=='__main__': main()
