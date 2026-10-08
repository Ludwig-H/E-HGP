#!/usr/bin/env python3
"""Replay the frozen T2dB3 receipt using Python only; never run a native probe."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def need(value,message):
 if not value:raise ValueError(message)

def pin(raw):return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--repo',required=True,type=Path)
 ap.add_argument('--snapshot',required=True,type=Path)
 a=ap.parse_args();session=a.snapshot/'session'
 capture=json.loads((HERE/'capture.json').read_text())
 for name,wanted in capture['pins'].items():need(pin((session/name).read_bytes())==wanted,'frozen primary '+name)
 result=subprocess.run([sys.executable,'-B','-S',str(HERE/'protocol.py'),'--repo',str(a.repo),'--session',str(session),'--before-archive',str(a.snapshot/'before.tar.gz')],capture_output=True,text=True,check=True)
 protocol=json.loads(result.stdout);need(protocol['protocol_verified']and not protocol['native_executed'],'protocol replay')
 spec=importlib.util.spec_from_file_location('t2db3_admission',HERE/'admit.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 observed=module.read(a.repo,session);frozen=json.loads((HERE/'results.json').read_text())
 need(observed==frozen,'frozen results projection')
 print(json.dumps(dict(protocol_verified=True,primary_results_replayed=True,verdicts=observed['verdicts'],decisive_warm_passes=observed['decisive_warm_passes'],native_journals=observed['native_journals'],strict_identity_admission_complete=observed['strict_evidence_admission_complete'],shutdown_certified=observed['closure']['targeted_shutdown_certified'],native_executed=False,payload_read=False),sort_keys=True))
if __name__=='__main__':main()
