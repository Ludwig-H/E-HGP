#!/usr/bin/env python3
"""Small read-only LIVE check and targeted corruptions of copied r1 evidence."""
import argparse
import copy
import json
from pathlib import Path
import readback as r


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,
        default=Path('/workspaces/E-HGP/build/v9-full-nsys-session-20260927-r1'))
    args = parser.parse_args()
    summary,public = r.collect(args.session)
    r.c.need(summary['status'] == 'failed' and summary['failure_replay'] and
             summary['command_count'] == 0 and not summary['FULL_executed'] and
             not summary['CUDA_profile_executed'],'LIVE r1 failure replay')
    host_dir = args.session/'full_host'
    host = r.read(host_dir/'receipt.json')
    verdict = r.read(args.session/'verdict.json')
    after = r.read(args.session/'after_stop.json')
    output = host_dir/'received/output'
    vm = r.read(output/'receipt.json')
    manifest = r.read(host_dir/'source_manifest.json')
    before,closing = (r.read(output/name) for name in ('sources_before.json','sources_after.json'))
    native,_ = r.c.runtime()
    def check(value):
        r.failed_worker(value,host,verdict,manifest,before,closing,native.TARGET,host['worker_sha256'])
    check(vm)
    positive,rejected = 2,0
    def reject(call):
        nonlocal rejected
        try:call()
        except (ValueError,KeyError,TypeError):rejected += 1;return
        raise ValueError('corrupted fixture was accepted')
    for key,value in (('schema','other'),('generation','other'),('worker_sha256','0'*64),
            ('source_manifest_sha256','0'*64),('target',{}),('scope','other'),('status','completed'),
            ('commands',[{}]),('FULL_executed',True),('CUDA_profile_executed',True),
            ('reports',{}),('sources_stable',False),('error','another error')):
        changed = copy.deepcopy(vm);changed[key] = value
        reject(lambda:check(changed))
    reject(lambda:r.failed_worker(vm,host,verdict,manifest,before,{},native.TARGET,host['worker_sha256']))
    changed = copy.deepcopy(verdict);changed['generation'] = 'other'
    reject(lambda:r.stop_check(native,host,changed,after))
    changed = copy.deepcopy(after);changed['status'] = 'RUNNING'
    reject(lambda:r.stop_check(native,host,verdict,changed))
    changed = copy.deepcopy(after);changed['metadata'] = {'secret':'excluded'}
    r.c.need('metadata' not in r.projection(changed) and
             set(public) == {'vm/'+name for name in r.TEXT_FILES} |
                {'README.md','launch.json','verdict.json','after_stop.json','SUMMARY.json','PRIVATE_LINKS.json'},
             'explicit publication allowlist')
    positive += 1
    reject(lambda:r.unique([('a',1),('a',2)]))
    print(json.dumps(dict(status='PASS',positive=positive,rejected=rejected,
                         original_evidence_changed=False,cloud_calls=0),sort_keys=True))


if __name__ == '__main__':main()
