#!/usr/bin/env python3
"""PREPARED ONLY: immutable attempts, atomic aggregate, deferred managed signals."""
import argparse
import datetime
import hashlib
import json
import os
import re
import signal
import stat
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from types import SimpleNamespace

sys.dont_write_bytecode=True
ROOT=Path(__file__).absolute().parent
FILES={'README.txt','er_snapshot.py','r1_snapshot.py','probe.py','read.py','record.py','origin.json','protocol.json'}
TIMEOUT=15
MANAGED=('SIGINT','SIGTERM','SIGHUP','SIGQUIT')
def require(c,m):
 if not c: raise RuntimeError(m)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def utc(): return datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='microseconds').replace('+00:00','Z')
def encoded(obj): return (json.dumps(obj,sort_keys=True,indent=1,allow_nan=False)+'\n').encode('ascii')
def bootstrap(pin):
 require(type(pin) is str and re.fullmatch('[0-9a-f]{64}',pin),'external source SHA syntax')
 require(os.path.normpath(str(ROOT))==str(ROOT),'canonical lexical root')
 for p in (ROOT,)+tuple(ROOT.parents): require(stat.S_ISDIR(p.lstat().st_mode),'nonsymlink root/ancestor')
 entries=list(ROOT.iterdir()); require({p.name for p in entries}==FILES|{'SHA256SUMS'},'exact REAL source inventory')
 for p in entries: require(stat.S_ISREG(p.lstat().st_mode),'regular nonsymlink payload')
 raw=(ROOT/'SHA256SUMS').read_bytes()
 require(hashlib.sha256(raw).hexdigest()==pin,'external manifest BEFORE parsing/compile/exec')
 listed={}; payloads={}
 for line in raw.decode('ascii').splitlines():
  h,name=line.split('  ',1)
  require(re.fullmatch('[0-9a-f]{64}',h) and name in FILES and name not in listed,'manifest syntax/name/duplicate')
  listed[name]=h
 require(set(listed)==FILES,'source manifest inventory')
 for name,h in listed.items():
  payloads[name]=(ROOT/name).read_bytes()
  require(hashlib.sha256(payloads[name]).hexdigest()==h,'source SHA '+name)
 # Execute EXACT verified bytes, never importlib/SourceFileLoader/.pyc.
 namespace={'__name__':'pinned_lca_reader_r2','__file__':str(ROOT/'read.py')}
 exec(compile(payloads['read.py'],str(ROOT/'read.py'),'exec'),namespace)
 reader=SimpleNamespace(**namespace)
 checked,protocol_sha=reader.verify_package(pin)
 require(checked==listed,'compiled reader source agreement')
 return reader,listed,protocol_sha
def python_identity():
 p=Path(os.path.realpath(sys.executable))
 require(p.is_absolute() and stat.S_ISREG(p.lstat().st_mode),'actual Python binary')
 return {'path':str(p),'sha256':sha(p),'version':sys.version,'version_info':list(sys.version_info),'implementation':sys.implementation.name}
def immutable(path,data):
 with path.open('xb') as f: f.write(data); f.flush(); os.fsync(f.fileno())
def atomic(path,data):
 fd,tmp=tempfile.mkstemp(prefix='.'+path.name+'.',dir=str(path.parent))
 try:
  with os.fdopen(fd,'wb') as f: f.write(data); f.flush(); os.fsync(f.fileno())
  os.replace(tmp,path)
  dfd=os.open(str(path.parent),os.O_RDONLY|os.O_DIRECTORY)
  try: os.fsync(dfd)
  finally: os.close(dfd)
 finally:
  # Only this explicit temporary FILE; never any recursive/directory removal.
  if os.path.exists(tmp): os.unlink(tmp)

def main():
 p=argparse.ArgumentParser(); p.add_argument('--source-manifest-sha',required=True); p.add_argument('--out',required=True); a=p.parse_args()
 reader,listed,protocol_sha=bootstrap(a.source_manifest_sha)
 out=reader.canonical_absolute(a.out,'fresh capture path')
 require(not out.exists() and not out.is_symlink() and not out.is_relative_to(ROOT),'fresh output OUTSIDE source package')
 for q in out.parents: require(stat.S_ISDIR(q.lstat().st_mode),'nonsymlink output ancestor')
 require(os.name=='posix' and all(hasattr(signal,s) for s in MANAGED) and hasattr(signal,'pthread_sigmask'),'POSIX managed-signal collector')
 py=python_identity(); reader.validate_python(py)
 origin={'package_root':str(ROOT),'source_files':{n:{'path':str(ROOT/n),'sha256':listed[n]} for n in sorted(FILES)},
         'manifest':{'path':str(ROOT/'SHA256SUMS'),'sha256':a.source_manifest_sha},'executable':py}
 managed_set={getattr(signal,n) for n in MANAGED}
 old_mask=signal.pthread_sigmask(signal.SIG_BLOCK,managed_set)
 if managed_set & old_mask:
  signal.pthread_sigmask(signal.SIG_SETMASK,old_mask)
  raise RuntimeError('managed signals initially blocked: collector environment refused before capture')
 try: out.mkdir(parents=False)
 except BaseException:
  signal.pthread_sigmask(signal.SIG_SETMASK,old_mask); raise
 runs=[]
 state={'first':None,'active_index':None,'sealed':False,'capture_pin':None}
 def publish():
  # A first signal can arrive during JSON/fsync. If it changed the snapshot,
  # republish refusal. The first object is assigned ONCE, never mutated.
  while True:
   stamp=state['first']
   receipt={'schema':'er_distinct_lca_run_receipt_v2','source_manifest_sha':a.source_manifest_sha,
            'origin':origin,'protocol_sha256':protocol_sha,'timeout_seconds':TIMEOUT,'runs':runs,
            'interrupted':stamp is not None,'first_signal':stamp,'collection_complete':len(runs)==2}
   atomic(out/'run_receipt.json',encoded(receipt))
   names=['run_receipt.json']
   for mode in ('normal','optimized'):
    names.extend(mode+s for s in ('.command.json','.stdout.json','.stderr.bin','.receipt.json') if (out/(mode+s)).exists())
   raw=''.join(sha(out/n)+'  '+n+'\n' for n in sorted(names)).encode('ascii')
   atomic(out/'SHA256SUMS',raw)
   if stamp is state['first']:
    state['capture_pin']=hashlib.sha256(raw).hexdigest(); return state['capture_pin']
 def deferred(sig,_frame):
  if state['first'] is not None: return
  state['first']={'number':int(sig),'name':signal.Signals(sig).name,
                  'received_at_utc':utc(),'active_run_index':state['active_index']}
  if state['sealed']:
   # Still installed until CLI exit: a late handled signal cannot leave a
   # passing aggregate or exit code. No child/measurement remains to defer.
   publish(); raise SystemExit(128+int(sig))
 # Install all four handlers atomically with respect to those signals.
 # Restore the original mask BEFORE any child creation; children are not
 # accidentally protected by an inherited blocked managed-signal mask.
 try:
  for name in MANAGED: signal.signal(getattr(signal,name),deferred)
 finally: signal.pthread_sigmask(signal.SIG_SETMASK,old_mask)
 accepted=False
 try:
  publish()
  for index,(mode,flags) in enumerate((('normal',['-B']),('optimized',['-B','-O']))):
   if state['first'] is not None: break
   reader.verify_package(a.source_manifest_sha); require(python_identity()==py,'Python before run')
   state['active_index']=index
   argv=[py['path']]+flags+[str(ROOT/'probe.py')]
   command={'schema':'er_distinct_lca_command_v2','index':index,'mode':mode,'argv':argv,'cwd':str(ROOT),
            'timeout_seconds':TIMEOUT,'checkpoint_utc':utc(),'source_before':listed,
            'python_before':py['sha256'],'first_signal':state['first']}
   immutable(out/(mode+'.command.json'),encoded(command))
   checkpoint_sha=sha(out/(mode+'.command.json'))
   start=utc(); tick=time.monotonic(); launched=False; timed_out=False; launch_error=None
   code=None; stdout=stderr=b''
   if state['first'] is None:
    try:
     launched=True
     proc=subprocess.run(argv,cwd=str(ROOT),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=TIMEOUT,check=False)
     code,stdout,stderr=proc.returncode,proc.stdout,proc.stderr
    except subprocess.TimeoutExpired as error:
     timed_out=True; code,stdout,stderr=None,error.stdout or b'',error.stderr or b''
    except OSError as error:
     launched=False; launch_error=repr(error); code=None
   elapsed=time.monotonic()-tick; end=utc()
   immutable(out/(mode+'.stdout.json'),stdout); immutable(out/(mode+'.stderr.bin'),stderr)
   after=None; python_after=None; integrity_error=None; judge_performed=False; judge_error=None
   try:
    after={n:sha(ROOT/n) for n in sorted(FILES)}; python_after=sha(Path(py['path']))
    require(after==listed and python_after==py['sha256'],'source/Python changed during attempt')
    reader.verify_package(a.source_manifest_sha)
   except Exception as error: integrity_error=repr(error)
   if launched and type(code) is int and code==0 and not timed_out and not stderr and integrity_error is None and state['first'] is None:
    try:
     judge_performed=True; reader.same(reader.parse(stdout),reader.expected_output(),'independent post-run output')
    except Exception as error: judge_error=repr(error)
   row={'schema':'er_distinct_lca_attempt_v2','index':index,'mode':mode,'checkpoint_file':mode+'.command.json',
        'checkpoint_sha256':checkpoint_sha,'argv':argv,'cwd':str(ROOT),'timeout_seconds':TIMEOUT,
        'launched':launched,'timed_out':timed_out,'exit_code':code,'launch_error':launch_error,'integrity_error':integrity_error,
        'judge_performed':judge_performed,'judge_error':judge_error,'start_utc':start,'end_utc':end,'duration_seconds':elapsed,
        'stdout_file':mode+'.stdout.json','stdout_sha256':hashlib.sha256(stdout).hexdigest(),
        'stderr_file':mode+'.stderr.bin','stderr_sha256':hashlib.sha256(stderr).hexdigest(),
        'source_before':listed,'source_after':after,'python_before':py['sha256'],'python_after':python_after,
        'first_signal_at_receipt_creation':state['first']}
   immutable(out/(mode+'.receipt.json'),encoded(row)); runs.append(row); state['active_index']=None
   publish()
   if state['first'] is not None or not judge_performed or judge_error is not None: break
  cap_pin=publish()
  if state['first'] is None:
   try: reader.verify_capture(out,cap_pin,a.source_manifest_sha,listed,protocol_sha); accepted=True
   except Exception as error: print('capture rejected: '+str(error),file=sys.stderr)
  accepted=accepted and state['first'] is None
 finally:
  state['active_index']=None; state['sealed']=True; publish()
 # Keep handlers installed through exit, including summary publication.
 if state['first'] is not None: accepted=False
 print(json.dumps({'capture':str(out),'capture_manifest_sha':state['capture_pin'],'accepted':accepted,
                   'interrupted':state['first'] is not None},sort_keys=True))
 return 128+state['first']['number'] if state['first'] is not None else (0 if accepted else 1)

if __name__=='__main__': sys.exit(main())
