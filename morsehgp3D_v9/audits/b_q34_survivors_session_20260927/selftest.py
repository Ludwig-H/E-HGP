#!/usr/bin/env python3
# Explicit protocol port of b_q34_resident_session_20260927 at 03decc16c.
# The pinned lifecycle helper, guards, fixed target and cooperative join are unchanged.
"""Offline protocol tests. Synthetic receipts are never measurement evidence.

Temporary archives use the existing pinned input, never a new tracked LiDAR
payload. Git streams and process operations are mocked; cloud calls forbidden.
"""
from contextlib import redirect_stdout
from copy import deepcopy
import ast
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import signal
import subprocess
import sys
import tarfile
import tempfile
from unittest.mock import patch
sys.dont_write_bytecode = True
import common as c
import package
import session
import worker


class Checks:
    def __init__(self):
        self.positive = 0
        self.rejected = 0
        self.labels = []

    def yes(self, condition, label):
        c.need(condition, label)
        self.positive += 1

    def no(self, function, label):
        try:
            function()
        except (ValueError, KeyError, TypeError, OSError, tarfile.TarError):
            self.rejected += 1
            self.labels.append(label)
        else:
            raise ValueError('negative admitted: ' + label)


def encoded(value):
    return (json.dumps(value, sort_keys=True, allow_nan=False) + '\n').encode()


def pins(files):
    return {name: hashlib.sha256(raw).hexdigest() for name, raw in files.items()}


def archive(path, files, extra=()):
    with tarfile.open(path, 'w:gz') as target:
        for name, raw in files.items():
            item = tarfile.TarInfo(name)
            item.size = len(raw)
            target.addfile(item, io.BytesIO(raw))
        for item, raw in extra:
            target.addfile(item, io.BytesIO(raw))


def artifact_tests(directory, checks):
    commit, tree = 'a'*40, 'b'*40
    data = (c.ROOT / c.DATA_SOURCE).read_bytes()
    files = {name: (c.ROOT / name).read_bytes() for name in (
        c.HELPER, c.PREFIX + '/common.py', c.PREFIX + '/worker.py',
        c.PREFIX + '/session.py', c.PREFIX + '/package.py', c.PREFIX+'/compile_contract.py', c.PROTOTYPE + '/CMakeLists.txt',
        c.PROTOTYPE+'/probe.cpp', c.SURVIVORS+'/device_cuda.cu', c.DIRECT_GATE+'/device_gate.cu',
        'morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/device_cuda.cu', 'morsehgp3D_v9/src/gpu/witness_filter.hpp')}
    files[c.DATA] = data
    files[c.PROVENANCE] = encoded(dict(schema=c.SCHEMA, scope='S2_only_no_FULL',
        protocol_source='commit', commit=commit, tree=tree))
    target = directory / 'positive.tar.gz'
    archive(target, files)
    manifest = pins(files)
    parsed, provenance = c.unpack_readonly(target, manifest)
    checks.yes(parsed == files and provenance['commit'] == commit, 'private snapshot positive')

    # A synthetic Git batch is byte-framed and content-addressed exactly as
    # git cat-file, but no subprocess or repository write is made.
    tracked = dict(files)
    tracked[c.DATA_SOURCE] = tracked.pop(c.DATA)
    tracked.pop(c.PROVENANCE)
    blobs = {hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest(): raw
             for raw in tracked.values()}
    entries = {name: hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
               for name, raw in tracked.items()}
    calls = []

    def git(*args, stdin=None):
        calls.append(args)
        if args == ('rev-parse', '--verify', commit + '^{commit}'):
            return (commit + '\n').encode()
        if args == ('rev-parse', '--verify', commit + '^{tree}'):
            return (tree + '\n').encode()
        if args[:4] == ('ls-tree', '-r', '-z', '--full-tree'):
            return b''.join(('100644 blob ' + oid + '\t' + name + '\0').encode() for name, oid in entries.items())
        if args == ('cat-file', '--batch'):
            return b''.join(oid.encode() + b' blob ' + str(len(blobs[oid])).encode() + b'\n' + blobs[oid] + b'\n'
                            for oid in stdin.decode().splitlines())
        raise ValueError('unexpected fake git command')

    with patch.object(package, 'git', side_effect=git):
        collected = package.collect(commit)
        checks.yes(collected == files and len(calls) == 4, 'reproducible Git object stream')
    with patch.object(package, 'collect', return_value=files):
        checks.yes(package.verify_committed(target, manifest) == provenance, 'committed package positive')
        changed = dict(files)
        changed[c.PROTOTYPE + '/CMakeLists.txt'] += b'# tamper\n'
        with patch.object(package, 'collect', return_value=changed):
            checks.no(lambda: package.verify_committed(target, manifest), 'Git source reproduction')
    checks.no(lambda: package.collect('HEAD'), 'full commit required')
    bad_manifest = dict(manifest)
    bad_manifest[c.DATA] = '0'*64
    checks.no(lambda: c.unpack_readonly(target, bad_manifest), 'member digest')
    checks.no(lambda: c.unpack_readonly(target, manifest | {'ghost': '0'*64}), 'exhaustive manifest')
    checks.no(lambda: c.unpack_readonly(target, {k: v for k, v in manifest.items() if k != c.DATA}), 'missing manifest member')
    for label, transform in (
        ('short frame', lambda f: f.__setitem__(c.DATA, data[:-12])),
        ('wrong helper', lambda f: f.__setitem__(c.HELPER, b'not the pinned helper')),
        ('uncommitted provenance', lambda f: f.__setitem__(c.PROVENANCE, encoded(dict(provenance, protocol_source='worktree')))),
        ('FULL provenance', lambda f: f.__setitem__(c.PROVENANCE, encoded(dict(provenance, scope='FULL')))),
        ('duplicate provenance', lambda f: f.__setitem__(c.PROVENANCE, b'{"schema":0,"schema":1}')),
        ('required missing', lambda f: f.pop(c.PREFIX + '/worker.py'))):
        changed = dict(files)
        transform(changed)
        path = directory / (label.replace(' ', '_') + '.tar.gz')
        archive(path, changed)
        checks.no(lambda: c.unpack_readonly(path, pins(changed)), label)
    for ordinal, name in enumerate(('/absolute', '../escape', c.PREFIX + '/../escape',
            './' + c.PREFIX + '/file', c.PREFIX + '//file', 'unknown.py', c.DATA)):
        item = tarfile.TarInfo(name)
        item.size = 1
        path = directory / ('path_' + str(ordinal) + '.tar.gz')
        archive(path, files, [(item, b'x')])
        checks.no(lambda: c.unpack_readonly(path, manifest | {name: hashlib.sha256(b'x').hexdigest()}), 'unsafe path/' + name)
    for ordinal, kind in enumerate((tarfile.SYMTYPE, tarfile.LNKTYPE, tarfile.CHRTYPE, tarfile.DIRTYPE)):
        item = tarfile.TarInfo(c.PREFIX + '/link')
        item.type, item.linkname = kind, '/outside'
        path = directory / ('kind_' + str(ordinal) + '.tar.gz')
        archive(path, files, [(item, b'')])
        checks.no(lambda: c.unpack_readonly(path, manifest), 'nonregular archive/' + str(ordinal))
    duplicate = directory / 'duplicate.json'
    duplicate.write_bytes(b'{"a":{"x":1,"x":2}}')
    checks.no(lambda: c.read(duplicate), 'nested duplicate JSON')
    return files, manifest


def probe_values(width=4, first='baseline'):
    gate = dict(schema='mhgp9_survivors_compare_v1', status='passed', mode='gate', cuda_executed=True, **worker.GATE_EXACT)
    high = dict(schema='mhgp9_survivors_device_gate_v1', status='passed', mode='cuda', cuda_executed=True)
    high.update({key: 1 for key in worker.HIGH_FIELDS})
    high.update(cases=24, refused=6, empty=6, single=6)
    frame = dict(schema='mhgp9_survivors_compare_v1', status='passed', mode='frame', cuda_executed=True,
        n=39885, input_hash_u64=9245360528374966039, k=5, s=8, Q=262144, Qr=262144,
        workers_arena=width, workers_front=1, reference_workers=4, first=first,
        sequence='ABBA' if first=='baseline' else 'BAAB', common_times_ms={key:1.0 for key in
        'input index front reference common_release total'.split()}, runs=[])
    frame['common_times_ms']['total'] = 200.0
    for position in range(4):
        candidate = (first=='candidate') if position in (0,3) else (first=='baseline')
        r = dict(implementation='survivors_6af40d886' if candidate else 'resident_af369c44',
            position=position, first_cuda_call=position==0, first_implementation_call=position<2,
            output_memory_available=candidate, device='SYNTHETIC_NOT_A_MEASUREMENT')
        for name, fields in (('masses', worker.MASS_FIELDS),('counters',worker.COUNTER_FIELDS),
            ('geometry',worker.GEOMETRY_FIELDS),('arena',worker.ARENA_FIELDS),
            ('memory',worker.MEMORY_FIELDS+(worker.OUTPUT_MEMORY_FIELDS if candidate else []))):
            r[name]={key:1 for key in fields}
        m=r['masses']
        m.update(P=23686751,P3=17732794,P4=23446295,E=9122704,E3=6667094,E4=8403884,S=2043612,
            S3=2000000,S4=2000000,digest_u64=5324876275161635233,Q_actual=262144,
            R=3133819,R_live=1128166,planned=1,fallbacks=1128165,raw_pairs=103861099,
            raw_q3=17732794,raw_q4=23446295,rectangle_visits=229928699,rectangle_waves=12)
        r['counters'].update(queries=m['E'],q3=m['E3'],q4=m['E4'],waves=35,visits=537798656)
        for lane in ('3','4'):
            m['pair_q'+lane+'_rejected']=m['P'+lane]-m['S'+lane]
            r['counters']['rejected'+lane]=m['E'+lane]-m['S'+lane]
        if candidate:
            r['memory'].update(output_size=m['S'],output_capacity=m['S'],
                output_append_copy_bytes=24*m['S'],output_debug_key_bytes=0,output_host_payload_bytes=12*m['S'])
        r['times_ms']={key:1.0 for key in worker.TIMING_FIELDS+(worker.OUTPUT_TIMING_FIELDS if candidate else [])}
        r['times_ms'].update(open=10.,consume=5.,consume_outer=6.,adapter=20.,
            adapter_plus_native_release_noncontiguous=21.,operation_observed=23.)
        frame['runs'].append(r)
    return high, gate, frame


def probe_tests(checks, high, gate, frame):
    checks.yes(worker.validate_high_gate(high)==high, 'synthetic high-key CUDA gate')
    for field in worker.HIGH_FIELDS:
        checks.no(lambda field=field:worker.validate_high_gate(dict(high, **{field:0})), 'high-key nonvacuity/'+field)
    for mode, original in (('gate',gate),('frame',frame)):
        checks.yes(worker.validate_probe(original,mode)==original, 'synthetic valid '+mode)
        for field,wrong in (('schema','FULL'),('status','failed'),('mode','other'),('cuda_executed',False),('cuda_executed',1),('FULL_executed',True)):
            bad=deepcopy(original);bad[field]=wrong
            checks.no(lambda:worker.validate_probe(bad,mode),mode+'/'+field)
    for field in worker.GATE_EXACT:
        checks.no(lambda field=field:worker.validate_probe(dict(gate,**{field:0}),'gate'),'exact gate/'+field)
    for field in ('n','k','s','Q','Qr','input_hash_u64','workers_arena','workers_front','reference_workers'):
        bad=deepcopy(frame);bad[field]+=1
        checks.no(lambda:worker.validate_probe(bad,'frame'),'frame identity/'+field)
    for field in worker.MASS_FIELDS:
        # Some changing values remain valid in isolation; mismatch with other
        # three operators is independently prohibited for every mass.
        bad=deepcopy(frame);bad['runs'][0]['masses'][field]+=1
        checks.no(lambda:worker.validate_probe(bad,'frame'),'paired mass/'+field)
    for group,fields in (('counters',worker.COUNTER_FIELDS),('geometry',worker.GEOMETRY_FIELDS),('arena',worker.ARENA_FIELDS)):
        for field in fields:
            bad=deepcopy(frame);bad['runs'][0][group][field]+=1
            checks.no(lambda:worker.validate_probe(bad,'frame'),group+'/'+field)
    for field in ('first_cuda_call','first_implementation_call','output_memory_available'):
        bad=deepcopy(frame);bad['runs'][2][field]=not bad['runs'][2][field]
        checks.no(lambda:worker.validate_probe(bad,'frame'),'call scope/'+field)
    for field in ('adapter','consume_outer','operation_observed','adapter_plus_native_release_noncontiguous'):
        bad=deepcopy(frame);bad['runs'][0]['times_ms'][field]=0
        checks.no(lambda:worker.validate_probe(bad,'frame'),'unpaid time/'+field)
    for wrong in (-1.,float('nan'),float('inf'),True):
        bad=deepcopy(frame);bad['runs'][0]['times_ms']['arena']=wrong
        checks.no(lambda:worker.validate_probe(bad,'frame'),'duration/'+str(wrong))
    for group in ('memory','times_ms','masses'):
        bad=deepcopy(frame);bad['runs'][0][group]['FULL_executed']=1
        checks.no(lambda:worker.validate_probe(bad,'frame'),'unknown field/'+group)
    for field in worker.OUTPUT_MEMORY_FIELDS:
        bad=deepcopy(frame);bad['runs'][1]['memory'][field]=-1
        checks.no(lambda:worker.validate_probe(bad,'frame'),'candidate memory/'+field)
    for width in (4,48):
        for first in ('baseline','candidate'):
            f=probe_values(width,first)[2]
            checks.yes(worker.validate_probe(f,'frame',width,first)==f,'paired direction/width')
    checks.no(lambda:worker.validate_probe(frame,'frame',48),'wrong width')
    checks.no(lambda:worker.validate_probe(frame,'frame',4,'candidate'),'wrong first implementation')


def probe_order_tests(checks, high, gate, frame):
    recipes=worker.recipes(Path('/source'),Path('/build'),c)
    order=['high_keys_gate','device_gate',*['ng00_'+key for key in worker.FRAME_KEYS]]
    for failure in ('none',*order):
        calls=[]
        def command(name,argv):
            checks.yes(argv==recipes[name],'exact recipe/'+name);calls.append(name)
            if name==failure: raise ValueError('injected command failure')
            if name=='high_keys_gate': return encoded(high)
            if name=='device_gate': return encoded(gate)
            _,width,first=name.split('_')
            return encoded(probe_values(int(width[1:]),first)[2])
        if failure=='none':
            result=worker.run_probes(command,recipes,c)
            checks.yes(set(result[2])==set(worker.FRAME_KEYS),'all paired processes')
        else: checks.no(lambda:worker.run_probes(command,recipes,c),'fail-fast/'+failure)
        checks.yes(calls==(order if failure=='none' else order[:order.index(failure)+1]),'ordered calls/'+failure)
    calls=[]
    def stub(name,argv):
        calls.append(name);return encoded(dict(high,cuda_executed=False))
    checks.no(lambda:worker.run_probes(stub,recipes,c),'stub blocks paired measures')
    checks.yes(calls==['high_keys_gate'],'no later command after false CUDA')


def compile_model(root,build,manifest):
    import compile_contract as cc
    required=(c.PROTOTYPE+'/probe.cpp',c.SURVIVORS+'/device_cuda.cu',c.DIRECT_GATE+'/device_gate.cu',
        'morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/device_cuda.cu','morsehgp3D_v9/src/gpu/witness_filter.hpp')
    headers={str(root/name):manifest[name] for name in required}
    headers['/usr/include/stdint.h']='e'*64
    entries=[]
    for target,count in (cc.BUILT_TARGETS|cc.EXCLUDED_TARGETS).items():
        cwd=build/('device_gate' if target==cc.GATE else 'baseline' if target in ('resident_gen','mhgp9_q34_filtered_resident') else '')
        for ordinal in range(count):
            source=str(root/required[ordinal%len(required)])
            obj=str(cwd/'CMakeFiles'/(target+'.dir')/('unit'+str(ordinal)+'.o'))
            entries.append(dict(directory=str(cwd),file=source,arguments=['/usr/bin/c++','-O3','-DNDEBUG','-o',obj,'-c',source]))
    jobs=cc.jobs_from_entries(entries)
    texts={str(build/'compile_commands.json'):json.dumps(entries),str(build/'CMakeCache.txt'):'synthetic\n',
           str(build/'CMakeFiles'/('flags.rsp')):'-DFAKE=1\n'}
    libraries={'/cuda/lib/'+name:'a'*64 for name in ('libcudart_static.a','libcudadevrt.a','librt.a')}
    before=dict(schema=cc.SCHEMA,stage='before_build',root=str(root),build=str(build),
        generated=texts,tools={'/usr/bin/c++':'b'*64},archives=libraries,headers=headers,
        commands=[job|dict(exit_code=0,stdout='obj: '+' '.join(headers)+'\n',stderr='') for job in jobs])
    after={k:deepcopy(v) for k,v in before.items() if k!='commands'}
    after.update(stage='after_build',compiled_dependencies=dict(headers),
        dependency_files={job['object']+'.d':'obj: '+' '.join(headers)+'\n' for job in jobs},
        objects={job['object']:'c'*64 for job in jobs},
        built_archives={str(build/'baseline/libresident_gen.a'):'d'*64},
        binaries={key:dict(path=str(path),sha256='f'*64) for key,path in cc.binaries(build).items()})
    return before,after


def compile_tests(directory,checks,manifest):
    import compile_contract as cc
    root,build=Path('/synthetic/source'),Path('/synthetic/survivors_build')
    before,after=compile_model(root,build,manifest)
    checks.yes(cc.validate_closed(before,after,root,build,manifest)==after,'synthetic complete 31-TU closure')
    for label,change in (
        ('missing object',lambda b,a:a['objects'].pop(next(iter(a['objects'])))),
        ('missing dependency file',lambda b,a:a['dependency_files'].pop(next(iter(a['dependency_files'])))),
        ('changed header',lambda b,a:a['headers'].__setitem__(next(iter(a['headers'])),'0'*64)),
        ('changed response',lambda b,a:a['generated'].__setitem__(next(p for p in a['generated'] if p.endswith('.rsp')),'tamper')),
        ('missing all response files',lambda b,a:(b['generated'].pop(next(p for p in b['generated'] if p.endswith('.rsp'))),a['generated'].pop(next(p for p in a['generated'] if p.endswith('.rsp'))))),
        ('changed command options',lambda b,a:b['commands'][0]['argv'].append('-DUNPINNED')),
        ('bool command success',lambda b,a:b['commands'][0].__setitem__('exit_code',False)),
        ('missing GEN archive',lambda b,a:a['built_archives'].clear()),
        ('wrong binary path',lambda b,a:a['binaries']['compare'].__setitem__('path','/other/binary')),
        ('compiled header not pre-pinned',lambda b,a:a['compiled_dependencies'].__setitem__('/outside/new.h','e'*64)),
        ('missing native source',lambda b,a:a['compiled_dependencies'].pop(str(root/'morsehgp3D_v9/src/gpu/witness_filter.hpp')))):
        b,a=deepcopy(before),deepcopy(after);change(b,a)
        checks.no(lambda:cc.validate_closed(b,a,root,build,manifest),'compile closure/'+label)
    # Exercise actual -L/-l ordering through a response file: pinning the
    # archive in a later directory must never hide a preferred shared object.
    local=directory/'link_resolution';local.mkdir()
    first,second=local/'first',local/'second'
    first.mkdir();second.mkdir()
    link_build=local/'build'
    link_dir=link_build/'CMakeFiles'/(cc.TARGET+'.dir');link_dir.mkdir(parents=True)
    response=link_build/'libraries.rsp'
    response.write_text('-L'+str(first)+' -L'+str(second)+' -lcudart_static -lcudadevrt '+str(first/'librt.a'))
    (link_dir/'link.txt').write_text('/usr/bin/c++ @libraries.rsp')
    for name in ('libcudart_static.a','libcudadevrt.a','librt.a'):
        (first/name).write_bytes(b'synthetic archive\n')
    (second/'libcudart_static.so').write_bytes(b'later shared object\n')
    actual=cc.archives(link_build)
    checks.yes(set(actual)=={str(first/name) for name in ('libcudart_static.a','libcudadevrt.a','librt.a')},
        'first directory archive precedes later shared library')
    (first/'libcudart_static.so').write_bytes(b'preferred shared object\n')
    checks.no(lambda:cc.archives(link_build),'same-directory shared library precedes archive')
    (first/'libcudart_static.a').rename(second/'libcudart_static.a')
    checks.no(lambda:cc.archives(link_build),'earlier shared library precedes later archive')


def forbidden(*_args, **_kwargs):
    raise ValueError('selftest forbids every real subprocess')


def legacy_test(checks):
    path = c.ROOT / 'gcp-migration/selftest_full_probe_session_v7.py'
    spec = importlib.util.spec_from_file_location('cuda_waves_legacy_selftest', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    output = io.StringIO()
    with redirect_stdout(output):
        module.main()
    value = json.loads(output.getvalue())
    checks.yes(value['status'] == 'passed' and value['real_subprocesses'] == 0 and
               value['mocked_commands'] == 3 and value['GCP_used'] is False, 'legacy cleanup mocks')
    return value


def received_tests(directory, checks, input_files, high, gate, frame):
    import compile_contract as cc
    host=directory/'host';output=host/'received/output';output.mkdir(parents=True)
    (host/'guardmarks').mkdir()
    files=dict(input_files);manifest=pins(files)
    archive(host/'snapshot.tar.gz',files);c.save(host/'source_manifest.json',manifest)
    remote='/tmp/ehgp-full-v7-0123456789abcdef.ABCdef1234'
    root,build=Path(remote)/'source',Path(remote)/'survivors_build'
    c.save(host/'receipt.json',dict(remote_directory=remote))
    generation='2026-09-27T10:00:00.123456Z';legacy=c.load_legacy()
    mark=dict(c.TARGET,schema='e-hgp.guard-mark.v1',mark='double_guard_verified',generation=generation,
        max_run_seconds='3600',guest_shutdown_minutes='30',date_utc='2026-09-27T10:02:00Z')
    schedule=dict(MODE='poweroff',USEC=str(int((legacy.epoch(generation)+1900)*1000000)))
    fields=lambda value:''.join(key+'='+item+'\n' for key,item in value.items()).encode()
    (host/'guardmarks/double_guard_verified').write_bytes(fields(mark))
    (host/'guest_schedule.stdout').write_bytes(fields(schedule))
    c.save(output/'guard_evidence.json',dict(mark=mark,schedule=schedule))
    c.save(output/'sources_before.json',manifest);c.save(output/'sources_after.json',manifest)
    before,after=compile_model(root,build,manifest)
    c.save(output/'compile_before.json',before);c.save(output/'compile_after.json',after)
    prepin=dict(schema=cc.SCHEMA,status='pinned_before_build',jobs=len(before['commands']),
        headers=len(before['headers']),output_sha256=c.sha(output/'compile_before.json'))
    commands=[];measures={}
    for ordinal,(name,argv) in enumerate(worker.recipes(root,build,c).items()):
        if name=='high_keys_gate':raw=encoded(high)
        elif name=='device_gate':raw=encoded(gate)
        elif name=='prepin':raw=encoded(prepin)
        elif name.startswith('ng00_'):
            _,width,first=name.split('_');value=probe_values(int(width[1:]),first)[2]
            measures[width+'_'+first]=value;raw=encoded(value)
        else:raw=b'mocked command\n'
        (output/(name+'.stdout')).write_bytes(raw);(output/(name+'.stderr')).write_bytes(b'')
        row=dict(name=name,argv=argv,exit_code=0,group_closed=True,started_epoch=ordinal,ended_epoch=ordinal+0.5,
            stdout_sha256=c.sha(output/(name+'.stdout')),stderr_sha256=c.sha(output/(name+'.stderr')))
        commands.append(row);c.save(output/(name+'.command.json'),row)
        c.save(output/(name+'.intent.json'),dict(name=name,argv=argv,started_epoch=ordinal))
    receipt=dict(schema=c.SCHEMA,status='completed',scope='S2_only_no_FULL',FULL_executed=False,
        contract_certified=False,public_status='not_claimed',target=c.TARGET,generation=generation,
        useful_budget_seconds=worker.USEFUL_SECONDS,CUDA_installation_attempted=False,
        worker_sha256=manifest[c.PREFIX+'/worker.py'],source_manifest_sha256=c.sha(host/'source_manifest.json'),
        sources_stable=True,compilation_stable=True,provenance=json.loads(files[c.PROVENANCE]),
        compile_before_sha256=c.sha(output/'compile_before.json'),compile_after_sha256=c.sha(output/'compile_after.json'),
        commands=commands,high_gate=high,gate=gate,measures=measures)
    c.save(output/'receipt.json',receipt)
    check=lambda:session.validate_received(host,manifest,generation)
    checks.yes(check()==receipt,'fully bound synthetic paired receipt')
    baseline={path:path.read_bytes() for path in host.rglob('*') if path.is_file()}
    def trial(label,mutate):
        mutate();checks.no(check,'received/'+label)
        for path,raw in baseline.items():path.write_bytes(raw)
    def change_receipt(key,value):
        (output/'receipt.json').write_bytes(encoded(dict(receipt,**{key:value})))
    for key,value in (('FULL_executed',True),('contract_certified',True),('status','failed'),
        ('target',dict(c.TARGET,instance='different')),('generation','other'),('sources_stable',False),
        ('compilation_stable',False),('worker_sha256','0'*64),('compile_before_sha256',''),('provenance',{})):
        trial(key,lambda key=key,value=value:change_receipt(key,value))
    trial('incomplete command inventory',lambda:change_receipt('commands',commands[:-1]))
    trial('manifest closure',lambda:(output/'sources_after.json').write_bytes(encoded({})))
    trial('stream changed',lambda:(output/'ng00_w48_candidate.stdout').write_bytes(b'{}'))
    trial('missing one order',lambda:change_receipt('measures',{k:v for k,v in measures.items() if k!='w48_candidate'}))
    trial('remote traversal',lambda:(host/'receipt.json').write_bytes(encoded(dict(remote_directory=remote+'/../other'))))
    def command_mutation(field,value):
        changed=deepcopy(receipt);row=changed['commands'][-1];row[field]=value
        (output/'receipt.json').write_bytes(encoded(changed));(output/'ng00_w48_candidate.command.json').write_bytes(encoded(row))
        (output/'ng00_w48_candidate.intent.json').write_bytes(encoded(dict(name=row['name'],argv=row['argv'],started_epoch=row['started_epoch'])))
    trial('wrong recipe matching intent',lambda:command_mutation('argv',commands[-1]['argv'][:-1]+['baseline']))
    trial('failed command',lambda:command_mutation('exit_code',2))
    trial('unclosed group',lambda:command_mutation('group_closed',False))
    trial('overlapping chronology',lambda:command_mutation('started_epoch',0))
    def compile_mutation():
        changed=deepcopy(after);changed['objects'].pop(next(iter(changed['objects'])))
        (output/'compile_after.json').write_bytes(encoded(changed))
        change_receipt('compile_after_sha256',c.sha(output/'compile_after.json'))
    trial('missing object repinned',compile_mutation)
    def guard_mutation():
        bad=dict(mark,mark='guest_guard_pending')
        (host/'guardmarks/double_guard_verified').write_bytes(fields(bad))
        (output/'guard_evidence.json').write_bytes(encoded(dict(mark=bad,schedule=schedule)))
    trial('matching but uncertified guards',guard_mutation)
    checks.yes(check()==receipt,'baseline restored')


def unchanged_lifecycle_test(checks):
    old_path = c.ROOT/'morsehgp3D_v9/audits/b_q34_cuda_session_20260927/session.py'
    checks.yes(c.sha(old_path) == '20e1b9fc6a9aea435ac6876b1eef1723a76750d8f460367fa9ada50ad7fa01a0',
               'frozen source protocol pin')
    def function(path, name):
        return ast.dump(next(node for node in ast.parse(path.read_text()).body
                             if isinstance(node, ast.FunctionDef) and node.name == name), include_attributes=False)
    checks.yes(function(old_path, 'wait_owned') == function(c.HERE/'session.py', 'wait_owned'),
               'controller cooperative wait unchanged AST')
    checks.yes(function(old_path, 'owned_controller') == function(c.HERE/'session.py', 'owned_controller'),
               'guarded lifecycle adapter unchanged AST')
    checks.yes(c.sha(c.ROOT/c.HELPER) == c.HELPER_PIN, 'lifecycle helper unchanged bytes')


def wait_tests(directory, checks):
    for kind in ('success', 'timeout', 'pid_write_failure', 'handler_install_failure', 'wait_interrupt'):
        calls = []
        class Process:
            pid = 123456
            done = False
            def poll(self):
                return 0 if self.done else None
            def wait(self, timeout=None):
                calls.append(('wait', timeout))
                if timeout is not None and kind == 'timeout':
                    raise subprocess.TimeoutExpired('mocked-controller', timeout)
                if timeout is not None and kind == 'wait_interrupt':
                    raise InterruptedError('mocked interruption')
                self.done = True
                return 0
            def send_signal(self, signum):
                calls.append(('signal', signum))
        process = Process()
        installed = []
        def signal_install(signum, handler):
            installed.append((signum, handler))
            if kind == 'handler_install_failure' and len(installed) == 2:
                raise OSError('mocked signal installation')
            return signal.SIG_DFL
        def save(_path, _value):
            if kind == 'pid_write_failure':
                raise OSError('mocked PID write failure')
        error = False
        with patch.object(session.signal, 'signal', side_effect=signal_install), patch.object(session.c, 'save', side_effect=save):
            try:
                code = session.wait_owned(process, directory)
                checks.yes(code == 0 and kind in ('success', 'timeout'), 'wait returned ' + kind)
            except (OSError, InterruptedError):
                error = True
        checks.yes(process.done and any(name == 'wait' for name, _ in calls), 'controller joined ' + kind)
        checks.yes(error == (kind in ('pid_write_failure', 'handler_install_failure', 'wait_interrupt')), 'error preserved ' + kind)
        signals = [value for name, value in calls if name == 'signal']
        checks.yes(signals == ([] if kind == 'success' else [signal.SIGINT]), 'only cooperative SIGINT ' + kind)


def main():
    checks = Checks()
    paths = [c.HERE/name for name in ('common.py', 'package.py', 'worker.py', 'session.py', 'selftest.py', 'compile_contract.py')]
    paths += [c.ROOT/c.HELPER, c.ROOT/'gcp-migration/selftest_full_probe_session_v7.py']
    before = {str(path): c.sha(path) for path in paths}
    with tempfile.TemporaryDirectory(prefix='mhgp9-resident-session-selftest-') as temporary, \
         patch.object(subprocess, 'run', side_effect=forbidden), patch.object(subprocess, 'Popen', side_effect=forbidden):
        directory = Path(temporary)
        files, _manifest = artifact_tests(directory, checks)
        high, gate, frame = probe_values()
        probe_tests(checks, high, gate, frame)
        probe_order_tests(checks, high, gate, frame)
        compile_tests(directory, checks, pins(files))
        received_tests(directory, checks, files, high, gate, frame)
        wait_tests(directory, checks)
        unchanged_lifecycle_test(checks)
        legacy = legacy_test(checks)
    checks.yes(before == {str(path): c.sha(path) for path in paths}, 'source stability during offline selftest')
    print(json.dumps(dict(schema=c.SCHEMA, status='passed', positive=checks.positive,
        rejected=checks.rejected, mutation_labels=checks.labels, controller_wait_scenarios=5,
        legacy=legacy, GCP_used=False, real_subprocesses=0, measured_data=False,
        source_sha256=before), sort_keys=True))


if __name__ == '__main__':
    main()
