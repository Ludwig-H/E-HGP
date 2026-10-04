#!/usr/bin/env python3
"""Summary reuse controls, real Python decoders and mocked native children; no native execution."""
import argparse
import contextlib
import copy
from dataclasses import replace
import hashlib
import io
import json
import mmap
import os
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from bench_profiles_collector_test import events, row
from bench_parallel_collector_test import environment, setup
from bench_semantic_test import fixture
import catalogue_profiles as profiles
import semantic_cache as reuse

CHECKS = 0


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


def rejected(callback, message, errors=(ValueError, OSError, TypeError)):
    try:
        callback()
    except errors:
        need(True, message)
    else:
        raise ValueError('accepted: '+message)


def context(bits=18):
    return reuse.Context('MHGP11CAT1', profiles.semantic.SCHEMA+';arity_counts=true',
        reuse.decoder_digest([Path(profiles.semantic.__file__)]), bits, 5, 4, 'a'*64, 'b'*64)


def identity(ordinal):
    return ['fixture', 18, 5, 0, ordinal, 0, False]


def helper(root):
    initial_checks = CHECKS
    path = root/'canonical.bin'; path.write_bytes(fixture(18)[0])
    ctx, cache, calls = context(), reuse.SummaryCache(), 0
    def decode(data):
        nonlocal calls
        calls += 1
        need(type(data) is mmap.mmap, 'decoder receives mapping, not whole bytes copy')
        rejected(lambda: data.__setitem__(0, 0), 'read-only decoder mapping', (TypeError,))
        return profiles.semantic.decode(data, 18, 5, 4, arity_counts=True)
    first_identity = identity(0)
    ticket = cache.inspect(path, ctx, first_identity, decode)
    original = ticket.summary
    need(len(cache) == 0 and ticket.evidence['mode'] == 'decoded' and calls == 1, 'inspection does not publish')
    ticket.summary.pop('qmin_counts')
    first_identity[0] = 'changed outside cache'
    evidence = ticket.evidence; evidence['source_attempt'][0] = 'changed copy'
    need(ticket.summary == original and ticket.evidence['source_attempt'] == identity(0), 'ticket owns small summary and identity')
    unpublished = cache.inspect(path, ctx, identity(1), decode)
    need(unpublished.evidence['mode'] == 'decoded' and calls == 2, 'unpublished result cannot be reused')
    cache.publish(ticket)
    hit = cache.inspect(path, ctx, identity(2), decode,
                        expected_sha256=hashlib.sha256(path.read_bytes()).hexdigest(), expected_bytes=path.stat().st_size)
    need(calls == 2 and len(cache) == 1 and hit.summary == original, 'validated summary reused without decoder')
    need(hit.evidence['mode'] == 'reused' and hit.evidence['source_attempt'] == identity(0) and
         hit.evidence['current_attempt'] == identity(2), 'source reference retained with current identity')
    need(hit.evidence['decode_wall_seconds'] == 0 and hit.evidence['hash_wall_seconds'] >= 0 and
         ticket.evidence['decode_wall_seconds'] >= 0 and hit.evidence['context'] == vars(ctx), 'real current costs and complete context')
    value = hit.summary; value['qmin_counts']['4'] = 999
    need(cache.inspect(path, ctx, identity(3), decode).summary == original, 'nested summary has no alias')
    cache.publish(hit)
    rejected(lambda: cache.publish(hit), 'ticket cannot be published twice')
    rejected(lambda: reuse.SummaryCache().publish(unpublished), 'foreign cache ticket')
    rejected(lambda: cache.inspect(path, ctx, identity(4), decode, expected_sha256='0'*64), 'corrupted raw hash refused')
    rejected(lambda: cache.inspect(path, ctx, identity(4), decode, expected_bytes=path.stat().st_size+1), 'wrong raw size refused')
    need(calls == 2 and len(cache) == 1, 'failed expected hash/size cannot become a decode')
    return CHECKS-initial_checks


def keys_and_rereads(root):
    initial_checks = CHECKS
    path = root/'keys.bin'; path.write_bytes(b'key fixture')
    ctx, cache, calls = context(), reuse.SummaryCache(), 0
    def decode(data):
        nonlocal calls
        calls += 1
        return dict(length=len(data), digest=hashlib.sha256(data).hexdigest())
    def inspect(c=ctx):
        ticket = cache.inspect(path, c, identity(calls), decode)
        cache.publish(ticket)
        return ticket
    inspect()
    changes = dict(format='OTHER_FORMAT', decoder_version='version2', decoder_sha256='c'*64, coord_bits=21,
                   kmax=6, count=5, xyz_sha256='d'*64, ids_sha256='e'*64)
    for key, value in changes.items():
        need(inspect(replace(ctx, **{key: value})).evidence['mode'] == 'decoded', 'context key '+key)
    need(calls == 9 and len(cache) == 9, 'all context members participate')
    path.write_bytes(b'key fixturf')
    need(inspect().evidence['mode'] == 'decoded', 'same-length raw bytes differ')
    path.write_bytes(b'key fixture extended')
    need(inspect().evidence['mode'] == 'decoded', 'raw length participates')
    # The changed byte is beyond the first hash block; a cached head/mtime/filename would miss it.
    path.write_bytes(b'x'*(2*reuse.BLOCK+31))
    before = inspect(); before_stat = path.stat()
    with path.open('r+b') as stream:
        stream.seek(reuse.BLOCK+17); stream.write(b'y')
    os.utime(path, ns=(before_stat.st_atime_ns, before_stat.st_mtime_ns))
    after = inspect()
    need(after.evidence['mode'] == 'decoded' and before.evidence['raw_sha256'] != after.evidence['raw_sha256'] and
         before.summary != after.summary, 'entire payload rehashed despite same size/path/mtime')
    return CHECKS-initial_checks


def transactions(root):
    initial_checks = CHECKS
    path = root/'transaction.bin'; path.write_bytes(b'abc')
    ctx = context(); cache = reuse.SummaryCache(capacity=2)
    for exception in (ValueError, OSError, KeyboardInterrupt):
        def fail(_data): raise exception('decoder stopped')
        rejected(lambda: cache.inspect(path, ctx, identity(0), fail), 'decoder failure/interruption', (exception,))
        need(len(cache) == 0, 'failed decode never inserted')
    for value in ({'nan': float('nan')}, {'too_large': 'x'*reuse.SUMMARY_LIMIT}, {1: 'not JSON-preserving'}, []):
        rejected(lambda: cache.inspect(path, ctx, identity(0), lambda _data: value), 'summary representation refused')
        need(len(cache) == 0, 'invalid summary never inserted')
    def modified(_data):
        stamp = path.stat()
        path.write_bytes(b'abd')
        os.utime(path, ns=(stamp.st_atime_ns, stamp.st_mtime_ns+1))
        return dict(valid=True)
    rejected(lambda: cache.inspect(path, ctx, identity(0), modified), 'concurrent file change refused')
    need(len(cache) == 0, 'modified input not inserted')
    path.write_bytes(b'')
    rejected(lambda: cache.inspect(path, ctx, identity(0), lambda _data: {}), 'empty input refused')
    path.write_bytes(b'A')
    def load(label, publish=True):
        path.write_bytes(label.encode())
        ticket = cache.inspect(path, ctx, [label], lambda data: dict(text=data[:].decode()))
        if publish: cache.publish(ticket)
        return ticket
    load('A'); load('B')
    need(load('A').evidence['mode'] == 'reused', 'positive bounded cache hit')
    load('C', publish=False)
    need(len(cache) == 2 and load('A').evidence['mode'] == 'reused', 'unpublished miss does not evict')
    load('C')
    need(len(cache) == 2 and load('B').evidence['mode'] == 'reused', 'bounded FIFO retains second entry')
    need(load('A').evidence['mode'] == 'decoded' and len(cache) == 2, 'FIFO eviction falls back to full decoder')
    for invalid in (0, 65, True, -1, '64'):
        rejected(lambda: reuse.SummaryCache(invalid), 'cache capacity refused')
    for invalid in ([], {}, [object()], ['x'*4097]):
        rejected(lambda: cache.inspect(path, ctx, invalid, lambda _data: {}), 'identity refused')
    return CHECKS-initial_checks


def dependencies(root):
    initial_checks = CHECKS
    a, b = root/'decoder.py', root/'dependency.py'
    a.write_bytes(b'decoder source\n'); b.write_bytes(b'dependency source\n')
    first = reuse.decoder_digest([a, b])
    need(first == reuse.decoder_digest([b, a]), 'dependency order canonicalized')
    b.write_bytes(b'dependency changed\n')
    need(first != reuse.decoder_digest([a, b]), 'transitive decoder dependency participates')
    rejected(lambda: reuse.decoder_digest([]), 'empty dependency inventory')
    rejected(lambda: reuse.decoder_digest([a, a]), 'duplicate dependency name')
    rejected(lambda: reuse.decoder_digest([root/'missing.py']), 'missing dependency')
    for key, value in dict(coord_bits=True, kmax=0, count=0, xyz_sha256='A'*64,
                           ids_sha256='x', decoder_sha256='0'*63, format='', decoder_version='').items():
        rejected(lambda: replace(context(), **{key: value}), 'invalid context '+key)
    return CHECKS-initial_checks


def collector(root):
    args = argparse.Namespace(work=root, data=root)
    case = dict(name='cache', coordinates='xyz', point_ids='ids', count=4, sha256='a'*64, ids_sha256='b'*64)
    cache, snapshots, calls, attempts = reuse.SummaryCache(), [], 0, 0
    native_mode = 'ok'
    def child(argv, **kwargs):
        nonlocal attempts
        attempts += 1
        need(kwargs['timeout'] == 30 and not kwargs['check'], 'bounded mocked child')
        values = events()
        if native_mode == 'counts': values[1]['balls'] = 10
        if native_mode == 'q4': values[1]['work']['q4_levels'] = 0
        if native_mode == 'logical': values[1]['logical']['nodes'] = -1
        code = {'refused': 2, 'failed': 3, 'signal': -15}.get(native_mode, 0)
        data = fixture(18)[0]
        Path(argv[3]).write_bytes(data[:-1] if native_mode == 'damaged' else data)
        return subprocess.CompletedProcess(argv, code, '\n'.join(json.dumps(e) for e in values).encode(),
                                           b'warning' if native_mode == 'stderr' else b'')
    real_decode = profiles.semantic.decode
    def decode(*arguments, **kwargs):
        nonlocal calls
        calls += 1
        return real_decode(*arguments, **kwargs)
    def measure(selected_cache=cache, repetition=0):
        return profiles.measure(root/'fake', case, 18, 5, args, lambda r: snapshots.append(copy.deepcopy(r)),
                                repetition=repetition, semantic_cache=selected_cache)
    with patch.object(profiles.subprocess, 'run', side_effect=child), patch.object(profiles.semantic, 'decode', side_effect=decode):
        disabled = measure(None)
        need(disabled['status'] == 'ok' and 'semantic_reuse' not in disabled and 'semantic_reuse_requested' not in disabled,
             'default path and metadata unchanged')
        first = measure(repetition=1); second = measure(repetition=2)
        need(first['status'] == second['status'] == 'ok' and calls == 2 and len(cache) == 1, 'actual catalogue decoder reused')
        need(first['semantic_reuse']['mode'] == 'decoded' and second['semantic_reuse']['mode'] == 'reused' and
             second['semantic_reuse']['source_attempt'] == first['semantic_reuse']['current_attempt'], 'catalogue source evidence')
        need(first['semantic'] == second['semantic'] and first['qmin_counts'] == second['qmin_counts'] == {'2': 6, '3': 4, '4': 1},
             'qmin pop cannot alter cached summary')
        need(all(r['status'] == 'pending_semantic' and 'semantic_reuse' not in r for r in snapshots),
             'checkpoint precedes any semantic promotion')
        for native_mode in ('counts', 'q4', 'logical', 'stderr', 'refused', 'failed', 'signal', 'damaged'):
            result = measure(repetition=3)
            need(result['status'] != 'ok' and len(cache) == 1, 'failed current attempt cannot borrow cached verdict: '+native_mode)
            if native_mode in ('counts', 'q4'):
                need(result['semantic_reuse']['mode'] == 'reused', 'current event rejected after a real cache hit')
            else:
                need('semantic_reuse' not in result, 'invalid precondition or decode has no reuse evidence')
        for native_mode in ('counts', 'q4', 'logical', 'stderr', 'refused', 'failed', 'signal', 'damaged'):
            empty = reuse.SummaryCache(); result = measure(empty)
            need(result['status'] != 'ok' and len(empty) == 0, 'failed first attempt cannot populate cache')
        native_mode = 'ok'; empty = reuse.SummaryCache()
        with patch.object(profiles.base, 'digest', return_value='0'*64):
            result = measure(empty)
        need(result['status'] == 'artifact_error' and len(empty) == 0, 'collected raw hash corruption not trusted')
        empty = reuse.SummaryCache()
        with patch.object(Path, 'unlink', side_effect=OSError('cleanup failed')):
            result = measure(empty)
        need(result['status'] == 'artifact_error' and len(empty) == 0, 'cleanup failure never publishes origin')
        empty = reuse.SummaryCache()
        with patch.object(profiles.semantic, 'decode', side_effect=KeyboardInterrupt):
            rejected(lambda: measure(empty), 'decoder interruption preserved', (KeyboardInterrupt,))
        need(len(empty) == 0 and snapshots[-1]['status'] == 'pending_semantic' and
             Path(snapshots[-1]['argv'][3]).exists(), 'interrupted attempt keeps pending checkpoint and artifact')
    return attempts


def campaign_option(root):
    args, manifest, builds = environment(root, 'campaign')
    args.reuse_semantic = True
    seen = []
    def measure(_exe, case, bits, kmax, _args, checkpoint, *, semantic_cache):
        need(type(semantic_cache) is reuse.SummaryCache and semantic_cache.capacity == 64, 'campaign owns bounded cache')
        seen.append(semantic_cache)
        result = row(case['name'], bits, kmax)
        checkpoint(result)
        return result
    with setup(manifest, builds), patch.object(profiles, 'measure', side_effect=measure), contextlib.redirect_stdout(io.StringIO()):
        code = profiles.run(args)
    report = json.loads((args.out/'profiles.json').read_text())
    need(code == 0 and len(seen) == 36 and all(c is seen[0] for c in seen), 'one cache for exactly one campaign')
    need(report['semantic_reuse']['capacity'] == 64 and
         all(r['semantic_reuse_requested'] for r in report['launch_intents']), 'campaign option published before launch')
    args, manifest, builds = environment(root, 'campaign_again'); args.reuse_semantic = True
    previous = seen[0]; seen.clear()
    with setup(manifest, builds), patch.object(profiles, 'measure', side_effect=measure), contextlib.redirect_stdout(io.StringIO()):
        profiles.run(args)
    need(seen[0] is not previous, 'no cache escapes campaign ownership')
    return 2


def main():
    with tempfile.TemporaryDirectory(prefix='mhgp11_semantic_cache_') as folder:
        root = Path(folder)
        h, k, t, d, c, r = helper(root), keys_and_rereads(root), transactions(root), dependencies(root), collector(root), campaign_option(root)
    print(json.dumps(dict(helper=h, keys=k, transactions=t, dependencies=d, attempts=c, campaigns=r,
                          checks=CHECKS, native=0), sort_keys=True))
    print('semantic_cache_verdict conforme helper%d keys%d transactions%d dependencies%d attempts%d campaigns%d native0' %
          (h, k, t, d, c, r))


if __name__ == '__main__':
    main()
