#!/usr/bin/env python3
"""Small LIVE R2 replay plus copied-receipt corruptions; no cloud or evidence writes."""
import argparse
from copy import deepcopy
import json
from pathlib import Path
from unittest.mock import patch
import readback as r


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,default=Path('/workspaces/E-HGP/build/v9-full-nsys-session-20260927-r2'))
    args = parser.parse_args()
    summary,public = r.collect(args.session)
    r.c.need(summary['status'] == 'completed' and summary['semantic_replay'],'LIVE native trace validated')
    host = r.read(args.session/'full_host/receipt.json');verdict = r.read(args.session/'verdict.json')
    output = args.session/'full_host/received/output';vm = r.read(output/'receipt.json')
    manifest = r.read(args.session/'full_host/source_manifest.json')
    native,_ = r.c.runtime()
    cases,_ = native.validate_snapshot(args.session/'full_host/snapshot.tar.gz',manifest)
    remote = Path(host['remote_directory']);commands = r.recipes(remote,native.payload,cases,vm['tool_paths'])
    positive,rejected = 1,0
    def no(call):
        nonlocal rejected
        try:call()
        except (ValueError,KeyError,TypeError):rejected += 1;return
        raise ValueError('corruption admitted')
    def identity(value,h=host,v=verdict):return r.worker_identity(value,h,v,native.TARGET,host['worker_sha256'])
    def semantic(value):return r.closed_full(value,output,remote,manifest,native.payload,cases,commands)
    for field,value in (('generation','other'),('worker_sha256','0'*64),('target',{}),('status','failed'),
            ('source_manifest_sha256','0'*64),('binary_origin','inherited'),('useful_budget_seconds',601)):
        changed = deepcopy(vm);changed[field] = value;no(lambda:identity(changed))
    failed = dict(vm,status='failed',error='fixture failure')
    failed_host = dict(host,status='worker_failed',worker_exit_code=1)
    failed_verdict = dict(verdict,status='worker_failed',controller_exit=1)
    r.c.need(identity(failed,failed_host,failed_verdict) is False,'failure not promoted');positive += 1
    for field,value in (('binary_sha256','0'*64),('artifacts_after',{}),('activity',{}),
                        ('native_argv',[]),('preflight',{})):
        changed = deepcopy(vm);changed[field] = value;no(lambda:semantic(changed))
    changed = deepcopy(vm);changed['reports']['full_trace.sqlite']['sha256'] = '0'*64
    no(lambda:semantic(changed))
    original_read = r.read
    with patch.object(r,'read',side_effect=lambda path:{} if Path(path).name == 'compiled_dependencies_after.json'
                      else original_read(path)):
        no(lambda:semantic(vm))
    changed = deepcopy(verdict);changed['generation'] = 'other'
    no(lambda:r.stop_check(native,host,changed,r.read(args.session/'after_stop.json')))
    row = deepcopy(vm['commands'][-2]);row['argv'][-1] = '--wrong'
    no(lambda:r.check_command(output,'profile',row,lambda _path:None,commands['profile']))
    after = r.read(args.session/'after_stop.json');after['metadata'] = {'secret':'excluded'}
    r.c.need('metadata' not in r.projection(after) and
             not any(name.endswith(('.sqlite','.nsys-rep','.u32le')) or name.endswith('qualified_probe') for name in public),
             'text-only publication');positive += 1
    diagnostics = r.trace_diagnostics(output/'full_trace.sqlite')
    r.c.need(len(diagnostics['events']) == 14 and len(diagnostics['warnings']) == 2 and
             diagnostics['trace_completeness_certified'] is False and 'DIAGNOSTICS.json' in public,
             'warnings preserved despite empty stderr');positive += 1
    no(lambda:r.unique([('a',1),('a',2)]))
    print(json.dumps(dict(status='PASS',positive=positive,rejected=rejected,cloud_calls=0,
                         original_evidence_changed=False),sort_keys=True))


if __name__ == '__main__':main()
