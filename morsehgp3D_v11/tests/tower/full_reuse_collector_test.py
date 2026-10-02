#!/usr/bin/env python3
"""Actual FULL artifact decoding/reuse with mocked native children; every current event remains checked."""
import copy
from pathlib import Path
import subprocess
import tempfile
import sys
from unittest.mock import patch
import json

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'bench'))
import full_campaign as driver
from full_campaign_test import arguments, encode, events, request, VALUE

CHECKS = 0


def need(value, why):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(why)


def main():
    attempts, decodes = 0, 0
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-summary-') as folder:
        root = Path(folder); args = arguments(root,'full'); args.work.mkdir(); args.optimizations = 3
        case = dict(name='test',count=3,coordinates='xyz',point_ids='ids',sha256='d'*64,ids_sha256='e'*64)
        raw = encode(VALUE,21)[0]
        original = driver.semantic.decode
        def counted(data,*parameters):
            nonlocal decodes
            decodes += 1
            return original(data,*parameters)
        def attempt(cache, *, mode='ok', current_case=None):
            nonlocal attempts
            attempts += 1
            checkpoints = []
            current_request = dict(request(optimizations=3),repetition=attempts-1)
            def child(argv,**_kwargs):
                value = events(optimizations=3)
                if mode == 'counts': value[2]['orders'][0]['nodes'] -= 1
                if mode == 'time': value[2]['forest_ns'] = 900
                Path(argv[3]).write_bytes(raw + (b'extra' if mode == 'bytes' else b''))
                return subprocess.CompletedProcess(argv,0,'\n'.join(json.dumps(e) for e in value).encode(),b'')
            with patch.object(driver.subprocess,'run',side_effect=child), patch.object(driver.semantic,'decode',side_effect=counted):
                if mode == 'cleanup':
                    with patch.object(Path,'unlink',side_effect=OSError('cleanup refused')):
                        result = driver.measure(root/'fake',current_case or case,current_request,args,
                            lambda r: checkpoints.append(copy.deepcopy(r)),semantic_cache=cache)
                else:
                    result = driver.measure(root/'fake',current_case or case,current_request,args,
                        lambda r: checkpoints.append(copy.deepcopy(r)),semantic_cache=cache)
            need(len(checkpoints) == 1 and checkpoints[0]['status'] == 'pending_semantic' and
                 'semantic' not in checkpoints[0], 'native result checkpoint precedes reuse/decoding')
            return result
        cache = driver.reuse.SummaryCache()
        first = attempt(cache)
        need(first['status'] == 'ok' and len(cache) == 1 and decodes == 1,'first result decoded and published')
        second = attempt(cache)
        need(second['status'] == 'ok' and decodes == 1 and second['semantic'] == first['semantic'],'same bytes reuse')
        evidence = second['semantic_reuse']
        need(evidence['mode'] == 'reused' and evidence['raw_sha256'] == second['semantic']['raw_sha256'] and
             evidence['bytes'] == len(raw) and evidence['hash_wall_seconds'] >= 0 and
             evidence['decode_wall_seconds'] == 0 and evidence['current_attempt'][4] == 1 and
             evidence['source_attempt'][4] == 0,'actual rehash and skipped decode declared')
        second['semantic']['orders'][0]['nodes'] = 999
        third = attempt(cache)
        need(third['semantic'] == first['semantic'] and decodes == 1,'no alias into stored summary')
        invalid = attempt(cache,mode='counts')
        need(invalid['status'] == 'invalid_output' and invalid['semantic_reuse']['mode'] == 'reused' and
             len(cache) == 1 and decodes == 1,'wrong current counts rejected despite cached valid payload')
        invalid = attempt(cache,mode='time')
        need(invalid['status'] == 'invalid_output' and len(cache) == 1 and decodes == 1,'current timing always checked')
        invalid = attempt(cache,mode='bytes')
        need(invalid['status'] == 'invalid_output' and decodes == 2 and len(cache) == 1,'changed payload decoded and rejected')
        changed = attempt(cache,current_case=dict(case,sha256='f'*64))
        need(changed['status'] == 'ok' and changed['semantic_reuse']['mode'] == 'decoded' and
             decodes == 3 and len(cache) == 2,'same output different input context cannot reuse')
        empty = driver.reuse.SummaryCache()
        invalid = attempt(empty,mode='counts')
        need(invalid['status'] == 'invalid_output' and len(empty) == 0,'failed event-to-summary validation not published')
        invalid = attempt(empty,mode='cleanup')
        need(invalid['status'] == 'artifact_error' and len(empty) == 0,'failed cleanup not published')
        recovered = attempt(empty)
        need(recovered['status'] == 'ok' and recovered['semantic_reuse']['mode'] == 'decoded' and len(empty) == 1,
             'next process decodes after failed publication')
        pending = driver.reuse.SummaryCache(); checkpoints = []
        def interrupted_child(argv,**_kwargs):
            Path(argv[3]).write_bytes(raw)
            return subprocess.CompletedProcess(argv,0,'\n'.join(json.dumps(e) for e in events(optimizations=3)).encode(),b'')
        with patch.object(driver.subprocess,'run',side_effect=interrupted_child), \
                patch.object(driver.semantic,'decode',side_effect=KeyboardInterrupt):
            try:
                driver.measure(root/'fake',case,request(optimizations=3),args,
                    lambda r: checkpoints.append(copy.deepcopy(r)),semantic_cache=pending)
            except KeyboardInterrupt: pass
            else: raise ValueError('decode interruption swallowed')
        need(len(pending) == 0 and len(checkpoints) == 1 and checkpoints[0]['status'] == 'pending_semantic' and
             Path(checkpoints[0]['argv'][3]).is_file(),'interrupted decoder preserves native attempt and artifact')
    need(attempts == 10 and decodes == 6 and CHECKS >= 21,'coverage floor')
    print('full_reuse_collector_verdict conforme attempts%d decodes%d interruptions1 checks%d native0' % (attempts,decodes,CHECKS))


if __name__ == '__main__':
    main()
