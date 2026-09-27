#!/usr/bin/env python3
"""Small offline checks of the diagnostic worker, using the original FULL evidence."""
import contextlib
from copy import deepcopy
from datetime import datetime,timezone
import io
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import time
from types import SimpleNamespace
from unittest.mock import patch
import worker as w

ROOT = Path(__file__).resolve().parents[3]
RECEIPT = ROOT/'morsehgp3D_v9/receipts/g4_core_warm_20260927'


def main():
    positive,rejected = 0,0
    def yes(value,label):
        nonlocal positive
        w.need(value,label);positive += 1
    def no(call,label):
        nonlocal rejected
        try:
            call()
        except (ValueError,OSError,KeyError):
            rejected += 1
        else:
            raise ValueError('negative admitted: '+label)
    def forbidden(*_args,**_kwargs):
        raise ValueError('selftest forbids real subprocesses')
    manifest = json.loads((RECEIPT/'source_manifest.json').read_bytes())
    payload = w.load_payload(ROOT,manifest)
    case = json.loads((RECEIPT/'plan.json').read_bytes())['cases'][0]
    raw = (RECEIPT/'vm/probe_0.stdout').read_text()
    source_pins = {str(p):w.sha(p) for p in (Path(w.__file__),Path(__file__),ROOT/'gcp-migration/tower_worker_v9.py',
        ROOT/'gcp-migration/full_probe_worker_v7.py')}
    with tempfile.TemporaryDirectory(prefix='mhgp9-full-nsys-test-') as name, \
         patch.object(subprocess,'run',side_effect=forbidden),patch.object(subprocess,'Popen',side_effect=forbidden):
        folder = Path(name)
        probe = w.native_probe(raw,payload,case)
        yes(w.native_probe('Nsight starting\n'+raw+'\nReport generated\n',payload,case) == probe,'native JSON inside profiler logs')
        no(lambda:w.native_probe(raw+raw,payload,case),'duplicate native output')
        no(lambda:w.native_probe('report generated\n',payload,case),'missing native output')
        for field,value in (('tower_digest','0'*16),('presentation_digest','0'*16),('status','refused')):
            changed = deepcopy(probe);changed[field] = value
            no(lambda:w.native_probe(json.dumps(changed),payload,case),'changed '+field)
        changed = deepcopy(probe);changed['frames']['results'][3]['orders'][4]['parents'] += 1
        no(lambda:w.native_probe(json.dumps(changed),payload,case),'only fourth FULL result changed')
        no(lambda:w.load_payload(ROOT,dict(manifest,**{'gcp-migration/tower_worker_v9.py':'0'*64})),'unpinned validator')
        artifact = folder/'artifact';artifact.write_bytes(b'qualified bytes')
        pins = {str(artifact):w.sha(artifact)}
        yes(w.pinned_artifacts(pins) == pins,'exact existing artifact')
        no(lambda:w.pinned_artifacts({str(artifact):'0'*64}),'wrong artifact hash')
        no(lambda:w.pinned_artifacts({str(folder/'absent'):'0'*64}),'missing artifact')
        link = folder/'link';link.symlink_to(artifact)
        no(lambda:w.pinned_artifacts({str(link):w.sha(artifact)}),'symlink artifact')
        dbpath = folder/'trace.sqlite'
        with sqlite3.connect(dbpath) as db:
            db.execute('CREATE TABLE CUPTI_ACTIVITY_KIND_KERNEL (start INTEGER,end INTEGER)')
        no(lambda:w.sqlite_summary(dbpath),'empty kernel table')
        with sqlite3.connect(dbpath) as db:
            db.execute('INSERT INTO CUPTI_ACTIVITY_KIND_KERNEL VALUES (1,2)')
        yes(w.sqlite_summary(dbpath) == {'kernel_tables':{'CUPTI_ACTIVITY_KIND_KERNEL':1}},'actual kernel activity required')
        argv = payload.probe_command(w.BINARY.parent,w.OLD_ROOT/'source',case)
        command = w.profile_command(folder/'nsys',folder/'report',argv)
        yes(command[8:] == argv and argv[:2] == [str(w.BINARY),str(w.INPUT)] and
            '--frames=4' in argv and '--sample=none' in command and '--cpuctxsw=none' in command,
            'profile wraps exactly the existing four-frame FULL command')
        # A missing historical binary must fail before downloading or invoking
        # any profiler. The actual lifecycle guard predicate is retained.
        root = folder/'source';root.mkdir()
        manifest_path = folder/'source_manifest.json'
        manifest_path.write_bytes((RECEIPT/'source_manifest.json').read_bytes())
        helper = payload.load_helper(ROOT)
        now = time.time();generation = datetime.fromtimestamp(now-10,timezone.utc).isoformat()
        mark = dict(payload.TARGET,schema='e-hgp.guard-mark.v1',mark='double_guard_verified',
                    generation=generation,max_run_seconds='3600',guest_shutdown_minutes='30',date_utc=generation)
        schedule = dict(MODE='poweroff',USEC=str(int((now+1700)*1000000)))
        guard = folder/'guard';guard.write_text(''.join(k+'='+v+'\n' for k,v in mark.items()))
        class Collector:
            def __init__(self,*_args):self.commands=[]
            def command(self,*_args):raise ValueError('unexpected command before artifact pin')
        args = SimpleNamespace(source_root=root,source_manifest=manifest_path,source_manifest_sha256=w.MANIFEST_PIN,
            output=folder/'output',guard_mark=guard,guard_mark_sha256=w.sha(guard),generation=generation,
            session_deadline_epoch=int(schedule['USEC'])/1000000,closing_margin_seconds=300,**payload.TARGET)
        with patch.object(w,'load_payload',return_value=payload),patch.object(payload,'load_helper',return_value=helper), \
             patch.object(payload,'source_map',return_value=manifest),patch.object(helper,'Worker',Collector), \
             patch.object(helper,'scheduled_text',return_value=''.join(k+'='+v+'\n' for k,v in schedule.items())), \
             patch.object(w,'pinned_artifacts',side_effect=ValueError('missing historical binary')), \
             contextlib.redirect_stdout(io.StringIO()):
            code = w.execute(args)
        result = json.loads((args.output/'receipt.json').read_bytes())
        yes(code == 1 and result['error'] == 'ValueError: missing historical binary' and result['commands'] == [] and
            not (folder/'tools').exists() and result['FULL_executed'] is False,'missing binary prevents every command/download')
    yes(source_pins == {p:w.sha(p) for p in source_pins},'source closure')
    print(json.dumps(dict(status='passed',positive=positive,rejected=rejected,GCP_used=False,
                         real_subprocesses=0,source_sha256=source_pins),sort_keys=True))


if __name__ == '__main__':
    main()
