#!/usr/bin/env python3
"""Standalone reader tests using only synthetic archives/JSON. No native process or cloud."""
import copy
import importlib.util
import io
import json
from pathlib import Path
import sys
import tarfile
import tempfile
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location('reader', Path(__file__).resolve().parents[2] / 'bench/verify_full_captures.py')
reader = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(reader)
CHECKS = 0


def check(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(reason)


def refused(call, fragment):
    try:
        call()
    except (ValueError, KeyError, OSError, TypeError) as error:
        check(fragment in str(error), 'wrong refusal: ' + str(error))
    else:
        raise ValueError('not refused: ' + fragment)


def encode(value):
    return (json.dumps(value, sort_keys=True, allow_nan=False) + '\n').encode()


def archive(path, files, results=False, comment=None):
    files = dict(files)
    if results:
        files['results/MANIFEST.sha256'] = ('\n'.join(reader.sha(raw) + '  ./' + name.removeprefix('results/')
            for name, raw in sorted(files.items())) + '\n').encode()
    with tarfile.open(path, 'w:gz', format=tarfile.PAX_FORMAT,
                      pax_headers={'comment': comment} if comment else None) as tar:
        for name, raw in files.items():
            member = tarfile.TarInfo(name)
            member.size = len(raw)
            tar.addfile(member, io.BytesIO(raw))
    return files


def matrix_fixture(configs, root):
    files, actual = {}, []
    for config in configs:
        name = config['name']
        names = sorted(reader.MANDATORY) if name != 'mutants' else ['mhgp11_mutants_' + m for m in MODULES]
        count = len(names)
        cache = 'CMAKE_BUILD_TYPE:STRING=Release\nMHGP11_COORD_BITS:STRING=21\n'
        provenance = dict(schema='ehgp.v11.build_provenance.v1', complete=True, errors=[], files=[
            dict(path='CMakeCache.txt', size=len(cache.encode()), sha256=reader.sha(cache.encode()), text=cache),
            dict(path='mhgp11_full_bench', size=100, sha256='a'*64)])
        files[root + '/' + name + '/build_provenance.json'] = encode(provenance)
        files[root + '/' + name + '/tests.json'] = encode([dict(name=n, disabled=False, labels=['unit']) for n in names])
        files[root + '/' + name + '/junit.xml'] = ('<testsuite>' + ''.join('<testcase name="%s" status="run"/>' % n
                                                                         for n in names) + '</testsuite>').encode()
        actual.append(dict(config, status='ok', conforming=True, failures=[], not_run=[], threads=1, seconds=1,
                           passed_labels=dict(unit=count),
                           tests=dict(selected=count, passed=count, failed=0, not_run=0),
                           steps=[dict(name='test', status='ok', exit_code=0)]))
    summary = dict(schema='ehgp.v11.g4_matrix_summary.v1', complete=True, conforming=True, exit_code=0, signals=[],
                   requested=[c['name'] for c in configs], statuses={c['name']: 'ok' for c in configs},
                   configurations=actual)
    files[root + '/summary.json'] = encode(summary)
    return files


MODULES = ('core', 'num', 'sched', 'cloud', 'index', 'catalogue', 'tower')


def fixture(root):
    configs = [dict(name=n, cmake_options=['-DCMAKE_BUILD_TYPE=Release', '-DMHGP11_COORD_BITS=21'], ctest_args=[])
               for n in ('bits21', 'mutants')]
    supplement = [dict(configs[0], name='gcc_asan_ubsan18')]
    plan = dict(commands=[dict(name='matrice', timeout_seconds=1450), dict(name='asan18', timeout_seconds=420)])
    package_files = {'gcp-migration/v11_worker.sh': b'fixture only',
        'morsehgp3D_v11/tools/g4_matrix.json': encode(dict(configurations=configs)),
        'morsehgp3D_v11/bench/meb_asan18_matrix.json': encode(dict(configurations=supplement)),
        'morsehgp3D_v11/bench/plans/full_qualification_g4.json': encode(plan)}
    log = ''
    for module in MODULES:
        # Same ID in distinct module blocks exercises isolation of verdict inventories.
        mutants = [dict(id='shared', attendu='porte')]
        if module == 'core':
            mutants += [dict(id='compile_a', attendu='construction'), dict(id='compile_b', attendu='construction')]
        package_files['morsehgp3D_v11/tests/mutants/' + module + '.json'] = encode(dict(mutants=mutants, plancher=len(mutants)))
        log += '1/7 Testing: mhgp11_mutants_' + module + '\n'
        log += ''.join(m['id'] + ' TUE causal fixture\n' for m in mutants)
        log += 'mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d\n' % (
            module, len(mutants), len(mutants), 2 if module == 'core' else 0, len(mutants))
    package_path = root / 'source.tar.gz'
    archive(package_path, package_files, comment=reader.SOURCE)
    capture = root / 'qualification'
    (capture / 'package').mkdir(parents=True)
    (capture / 'results').mkdir()
    (capture / 'package/plan.json').write_bytes(encode(plan))
    (capture / 'package/plan.sh').write_bytes(b'fixture rendered plan\n')
    generation = '2026-10-03T14:00:00Z'
    worker = dict(schema='ehgp.v11.worker_result.v1', status='completed', fatal='', data='ok', nproc='48',
                  source='commit:' + reader.SOURCE, package_sha256=reader.digest(package_path), generation=generation,
                  plan_sha256=reader.sha(b'fixture rendered plan\n'), commands_total='2', commands_ok='2',
                  interrupted='0', overflow_files='0', overflow_unresolved='0', truncated_streams='0')
    commands = [dict(index=str(i), name=p['name'], status='ok', exit_code='0', wall_seconds='1',
                     timeout_seconds=str(p['timeout_seconds']), max_rss_kb='10') for i, p in enumerate(plan['commands'])]
    results = matrix_fixture(configs, 'results/cmd/000_matrice/files/matrix')
    results.update(matrix_fixture(supplement, 'results/cmd/001_asan18/files/matrix'))
    results['results/cmd/000_matrice/files/matrix/mutants/LastTest.log'] = log.encode()
    results['results/plan.sh'] = b'fixture rendered plan\n'
    results['results/worker.txt'] = ''.join(k + '=' + v + '\n' for k, v in worker.items()).encode()
    results['results/commands.tsv'] = ('\t'.join(commands[0]) + '\n' + ''.join('\t'.join(row.values()) + '\n'
                                                                           for row in commands)).encode()
    # A nested inventory is a payload, not a second exclusion from the root manifest.
    results['results/cmd/000_matrice/files/inner/SHA256SUMS'] = b'nested preserved\n'
    all_results = archive(capture / 'results/results.tar.gz', results, results=True)
    receipt = dict(schema='ehgp.v11.session_receipt.v1', source_kind='commit', commit=reader.SOURCE,
        worker_source='commit:' + reader.SOURCE, package_sha256=reader.digest(package_path), generation=generation,
        closing_generation=generation, observed_after=dict(status='TERMINATED', lastStartTimestamp=generation),
        closure='stopped', targeted_shutdown_certified=True, target=dict(instance='fixture', zone='zone', project='project'),
        max_run_seconds=4200, status='completed', results_verified=True, worker_exit_code=0, guest_guard_intact=True,
        start_certified=True, data_verified_remote=True, errors=[], results_skipped_members=[],
        overflow=dict(evicted=[], truncated_streams=[]), plan_sha256=reader.sha(encode(plan)),
        worker_plan_sha256=worker['plan_sha256'], results_sha256=reader.digest(capture / 'results/results.tar.gz'),
        results_bytes=(capture / 'results/results.tar.gz').stat().st_size, results_manifest_files=len(results),
        worker=worker, commands=commands)
    (capture / 'receipt.json').write_bytes(encode(receipt))
    target = dict(name='fixture', status='RUNNING', zone='https://example/zones/zone', lastStartTimestamp=generation,
        machineType='https://example/projects/project/zones/zone/machineTypes/g4-standard-48',
        scheduling=dict(provisioningModel='SPOT', instanceTerminationAction='STOP', automaticRestart=False,
                        maxRunDuration=dict(nanos=0, seconds='4200')), capture_source=dict(sha256='e'*64))
    (capture / 'target_running_minimal.json').write_bytes(encode(target))
    return package_path, capture, results, receipt, target


def qualification(root):
    package, capture, results, receipt, target = fixture(root)
    result = reader.verify(capture, None, package, reader.SOURCE)
    check(result['conforming'] and len(result['qualification']['mutants']) == 7 and
          result['qualification']['mutants'][0]['expected_compile_refusals'] == 2, 'positive synthetic complete QUAL')
    for key, value, fragment in [('targeted_shutdown_certified', False, 'targeted stop'),
        ('closing_generation', 'another', 'generation mismatch'), ('worker_exit_code', True, 'failed/incomplete'),
        ('commit', '0'*40, 'source/package'), ('results_manifest_files', 0, 'manifest count')]:
        changed = dict(receipt); changed[key] = value
        (capture / 'receipt.json').write_bytes(encode(changed))
        refused(lambda: reader.verify(capture, None, package, reader.SOURCE), fragment)
    (capture / 'receipt.json').write_bytes(encode(receipt))
    altered = dict(target, machineType=target['machineType'].replace('g4-standard-48', 'n2-standard-48'))
    (capture / 'target_running_minimal.json').write_bytes(encode(altered))
    refused(lambda: reader.verify(capture, None, package, reader.SOURCE), 'captured G4-48')
    (capture / 'target_running_minimal.json').write_bytes(encode(target))
    alternate=dict(target);alternate.pop('capture_source')
    alternate.update(original_capture_sha256='e'*64,original_capture_path='/fixture/describe.stdout')
    reader.generation_and_target(receipt,alternate)
    check(True,'both captured minimal target provenance shapes')
    changed = dict(results)
    key = 'results/cmd/000_matrice/files/matrix/bits21/junit.xml'
    changed[key] = changed[key].replace(b'status="run"', b'status="notrun"', 1)
    path = root / 'bad_junit.tar.gz'; archive(path, changed, results=True)
    with_reader = reader.Archive(path, result=True)
    spec = reader.Archive(package)
    refused(lambda: reader.matrix(with_reader, 'results/cmd/000_matrice/files/matrix',
        spec.json('morsehgp3D_v11/tools/g4_matrix.json'), True), 'JUnit execution')
    with_reader.close(); spec.close()
    changed = dict(results)
    key = 'results/cmd/000_matrice/files/matrix/mutants/LastTest.log'
    changed[key] = changed[key].replace(b'dont_signal=0', b'dont_signal=1', 1)
    path = root / 'bad_mutant.tar.gz'; archive(path, changed, results=True)
    with_reader = reader.Archive(path, result=True); spec = reader.Archive(package)
    refused(lambda: reader.mutant_summary(with_reader, 'results/cmd/000_matrice/files/matrix', spec), 'mutant causes/counts')
    with_reader.close(); spec.close()
    # Deliberately omit only the nested inventory from the root inventory.
    path = root / 'missing_inventory.tar.gz'
    files = dict(results)
    listed = {k: v for k, v in files.items() if not k.endswith('/SHA256SUMS')}
    files['results/MANIFEST.sha256'] = ('\n'.join(reader.sha(v) + '  ./' + k.removeprefix('results/')
                                               for k, v in listed.items()) + '\n').encode()
    archive(path, files)
    refused(lambda: reader.Archive(path, result=True), 'not exhaustive')


def paired():
    manifest=dict(schema='mhgp11.catalogue_benchmark_inputs.v1',cases=[])
    input_pins=[]
    for case,count in reader.COUNTS.items():
        xyz,ids=reader.REUSE1_INPUTS[case]
        manifest['cases'].append(dict(name=case,count=count,coordinates=case+'.u32le',point_ids=case+'.ids.u32le',
            sha256=xyz,ids_sha256=ids,unit_site_weights=True,duplicate_sites=0,profile='quantized_u18_input_only'))
        input_pins.extend([dict(name=case+'.u32le',sha256=xyz,size=12*count),
                           dict(name=case+'.ids.u32le',sha256=ids,size=4*count)])
    check(set(reader.input_manifest(manifest,input_pins))==set(reader.COUNTS),'six pinned reuse1 digests/cardinalities')
    altered=copy.deepcopy(manifest);altered_pins=copy.deepcopy(input_pins)
    altered['cases'][0]['sha256']='0'*64;altered_pins[0]['sha256']='0'*64
    refused(lambda:reader.input_manifest(altered,altered_pins),'not the pinned reuse1')
    requests = reader.calendar()
    check(len(requests) == len({reader.identity(r) for r in requests}) == 81, '81 unique invocations')
    check(sum(r['build_variant'] == 'baseline' for r in requests) == 27, '27 baseline/54 current')
    for case in reader.COUNTS:
        for workers in (1, 8, 48):
            group = [r for r in requests if r['case'] == case and r['workers'] == workers]
            check(all({group[3*r+i]['build_variant'] for r in range(3)} == dict(reader.VARIANTS).keys()
                      for i in range(3)), 'rotating producer order')
    cases = {name: dict(count=count, coordinates=name+'.u32le', point_ids=name+'.ids.u32le',
                        sha256='a'*64, ids_sha256='b'*64) for name, count in reader.COUNTS.items()}
    base_pin = dict(source_commit=reader.BASELINE, source_manifest_sha256='c'*64, configuration='baseline',
                    path='/fixture/baseline', sha256='d'*64, bytes=10, cache_sha256='e'*64, provenance_sha256='f'*64)
    current_pin = dict(base_pin, source_commit=reader.SOURCE, configuration='bits21', path='/fixture/current')
    report = dict(schema='ehgp.v11.full_paired_campaign.v1', complete=True, conforming=True, build_status='ok',
                  requested=requests, requested_runs=81, not_run=[], runs=[], launch_intents=[], builds=[base_pin,current_pin])
    first = {}
    for request in requests:
        pin = base_pin if request['build_variant'] == 'baseline' else current_pin
        case = cases[request['case']]
        argv = [pin['path'], '/fixture/'+case['coordinates'], '/fixture/'+case['point_ids'], '/fixture/out.bin',
                '5', '16', '256', '0', str(2**32-1), str(8*1024**3), str(request['workers']), str(request['optimizations'])]
        orders = [dict(work=dict(steps=0)) for _ in range(5)]
        events = [dict(phase='cloud', sites=case['count'], points=case['count']),
                  dict(phase='domain', index_ns=10, domain_ns=20),
                  dict(request, phase='full', status='ok', reason='none', wall_ns=40,index_ns=10,domain_ns=20,
                       forest_ns=10, peak_reserved_bytes=20,reserved_after_bytes=10,cpu_seconds=.001,
                       orders=orders,concurrent_orders=bool(request['optimizations']&8192),
                       population_lookup=bool(request['optimizations']&4096),phases=dict(regular_ns=1)),
                  dict(phase='exit', status='ok', reason='none')]
        attempt = list(reader.identity(request))
        row = dict(request, build_pin=pin, build_pin_after=pin, status='ok', exit_code=0, stderr='', errors=[],
                   whole_input=True, count=case['count'], argv=argv, events=events,
                   stdout='\n'.join(json.dumps(e) for e in events), full_ms=.00004, process_wall_seconds=.1,
                   semantic_wall_seconds=.01, semantic=dict(sha256='1'*64, raw_sha256='2'*64, bytes=100, orders=[{}]*5),
                   artifact_comparison=dict(status='equal_bytes' if request['case'] in first else 'reference', raw_sha256='2'*64),
                   semantic_reuse=dict(current_attempt=attempt, source_attempt=first.get(request['case'],attempt),
                       raw_sha256='2'*64, bytes=100, mode='reused' if request['case'] in first else 'decoded',
                       schema='ehgp.v11.semantic_reuse.v1', context=dict(format='MHGP11FUL1',
                           decoder_version='ehgp.v11.full_semantic.v1',decoder_sha256='3'*64,
                           coord_bits=21,kmax=5,count=case['count'],xyz_sha256=case['sha256'],ids_sha256=case['ids_sha256'])))
        first.setdefault(request['case'],attempt)
        report['runs'].append(row)
        report['launch_intents'].append(dict(request,build_pin=pin,whole_input=True,argv=argv,
                                             input_sha256=case['sha256'],ids_sha256=case['ids_sha256']))
    pins = dict(binary=dict(sha256='d'*64,size=10),cache=dict(sha256='e'*64),provenance_sha256='f'*64,decoder_sha256='3'*64)
    check(len(reader.paired_rows(report,cases,reader.SOURCE,pins))==27, 'positive synthetic 81paired attempts')
    changes = [
        (lambda r:r['runs'].pop(), 'invocations missing'),
        (lambda r:r['runs'][0]['semantic'].update(raw_sha256='3'*64), 'byte comparison record'),
        (lambda r:r['runs'][0].update(build_pin_after=dict(r['runs'][0]['build_pin'],sha256='0'*64)), 'pin drift'),
        (lambda r:r['launch_intents'][0].update(ids_sha256='0'*64), 'attempt verdict/input'),
        (lambda r:r['runs'][0]['semantic_reuse'].update(current_attempt=['lost_variant']), 'reuse context'),
        (lambda r:r['runs'][0]['semantic_reuse']['context'].update(ids_sha256='0'*64), 'exact input/decoder'),
        (lambda r:r['runs'][0]['semantic_reuse'].update(source_attempt=['lost_variant']), 'semantic origin'),
        (lambda r:r['runs'][0].update(stderr='runtime error'), 'attempt verdict/input'),
        (lambda r:r['runs'][0].update(full_ms=float('inf')), 'full_ms: domain'),
    ]
    for mutate, fragment in changes:
        corrupt = copy.deepcopy(report);mutate(corrupt)
        refused(lambda:reader.paired_rows(corrupt,cases,reader.SOURCE,pins),fragment)


def failed_qualification(root):
    package,capture,results,receipt,_target=fixture(root)
    main_root='results/cmd/000_matrice/files/matrix'
    summary=reader.load(results[main_root+'/summary.json'])
    summary.update(conforming=False,exit_code=1)
    config=summary['configurations'][1]
    config.update(conforming=False,status='failed',failures=['mhgp11_mutants_core'])
    config['tests'].update(passed=config['tests']['selected']-1,failed=1)
    config['passed_labels']['unit']-=1
    config['steps'][0].update(status='failed',exit_code=8)
    summary['statuses']['mutants']='failed'
    results[main_root+'/summary.json']=encode(summary)
    results[main_root+'/mutants/junit.xml']=results[main_root+'/mutants/junit.xml'].replace(
        b'<testcase name="mhgp11_mutants_core" status="run"/>',
        b'<testcase name="mhgp11_mutants_core" status="fail"><failure/></testcase>')
    log=results[main_root+'/mutants/LastTest.log'].decode().replace('shared TUE causal fixture',
        'shared INVALIDE fixture launch failed',1)
    log='\n'.join(line for line in log.splitlines()if not line.startswith('mutants_ok module=core'))+'\n'
    results[main_root+'/mutants/LastTest.log']=log.encode()
    receipt['commands'][0].update(status='failed',exit_code='1')
    commands=receipt['commands']
    results['results/commands.tsv']=('\t'.join(commands[0])+'\n'+''.join('\t'.join(r.values())+'\n'for r in commands)).encode()
    receipt['worker'].update(status='failed',commands_ok='1')
    results['results/worker.txt']=''.join(k+'='+v+'\n'for k,v in receipt['worker'].items()).encode()
    archive(capture/'results/results.tar.gz',results,results=True)
    receipt.update(status='failed_remote',worker_exit_code=1,
        results_sha256=reader.digest(capture/'results/results.tar.gz'),
        results_bytes=(capture/'results/results.tar.gz').stat().st_size)
    (capture/'receipt.json').write_bytes(encode(receipt))
    refused(lambda:reader.verify(capture,None,package,reader.SOURCE),'failed/incomplete')
    result=reader.verify(capture,None,package,reader.SOURCE,inspect_failed=True)
    check(result['integrity_verified'] and result['conforming']is False and
          result['qualification']['matrix_conforming']is False and
          result['qualification']['mutants'][0]['unsuccessful'][0]['verdict']=='INVALIDE',
          'closed failure preserved; never becomes complete qualification')


def floors_and_override(root):
    configs=[dict(name='bits21',cmake_options=['-DCMAKE_BUILD_TYPE=Release','-DMHGP11_COORD_BITS=21'],
                  ctest_args=[],require_labels=['unit','concurrency'])]
    files=matrix_fixture(configs,'results/matrix')
    summary=reader.load(files['results/matrix/summary.json'])
    summary.update(conforming=False,exit_code=3)
    summary['statuses']['bits21']='floor_violated'
    summary['configurations'][0].update(conforming=False,status='floor_violated')
    files['results/matrix/summary.json']=encode(summary)
    path=root/'floor.tar.gz';archive(path,files,results=True)
    payload=reader.Archive(path,result=True)
    refused(lambda:reader.matrix(payload,'results/matrix',dict(configurations=configs),True),'matrix incomplete')
    report,counts=reader.matrix(payload,'results/matrix',dict(configurations=configs),True,inspect_failed=True)
    check(report['conforming']is False and counts[0]['failed']==0 and
          counts[0]['missing_required_labels']==['concurrency'],'native passing inventory does not erase label floor')
    payload.close()
    original=dict(note='source fixture',configurations=configs)
    override=copy.deepcopy(original)
    override['note']='recorded runtime correction'
    override['configurations'][0]['require_labels']=['unit','oracle']
    raw=encode(override);path=root/'runtime_matrix.json';path.write_bytes(raw)
    template=dict(note='source plan',commands=[dict(argv=[]),dict(argv=['matrix','--matrix',
        '{src}/morsehgp3D_v11/bench/full_paired_bits21_matrix.json']),dict(argv=[])])
    actual=copy.deepcopy(template);actual['note']='runtime note'
    actual['commands'][1]['argv'][-1]='{data}/'+reader.PAIRED_MATRIX_NAME
    receipt=dict(data_files=[dict(name=reader.PAIRED_MATRIX_NAME,sha256=reader.sha(raw),size=len(raw))])
    # Replace only the approved digest with this tiny synthetic fixture's digest. Production retains
    # the literal ED843... allowlist; no captured file or actual reader source is modified by this test.
    with patch.object(reader,'PAIRED_MATRIX_SHA256',reader.sha(raw)):
        value=reader.paired_matrix_plan(template,actual,receipt,path)
        check(reader.checked_runtime_matrix(original,value)==override,'exact labels/note runtime delta admitted')
        corrupt=copy.deepcopy(value);corrupt['configurations'][0]['ctest_args']=['-R','other']
        refused(lambda:reader.checked_runtime_matrix(original,corrupt),'changed filters/options')
        path.write_bytes(raw+b' ')
        refused(lambda:reader.paired_matrix_plan(template,actual,receipt,path),'input hash/size')
        path.write_bytes(raw)
        changed=copy.deepcopy(actual);changed['commands'][1]['argv'][0]='different launcher'
        refused(lambda:reader.paired_matrix_plan(template,changed,receipt,path),'changed beyond approved')
        changed=copy.deepcopy(actual);changed['commands'][1]['argv'][-1]='{data}/other.json'
        refused(lambda:reader.paired_matrix_plan(template,changed,receipt,path),'not the approved')


def main():
    for raw, fragment in [(b'{"same":1,"same":2}', 'duplicate JSON'), (b'{"x":NaN}', 'nonfinite JSON')]:
        refused(lambda:reader.load(raw),fragment)
    for path in ('../out', '/absolute', 'x/../out', 'x//out', '././out', 'x\\out'):
        refused(lambda:reader.canonical(path), 'path')
    for value in (True,-1,2**64,float('nan')):
        refused(lambda:reader.number(value,'counter',True),'counter')
    with tempfile.TemporaryDirectory(prefix='mhgp11-capture-reader-') as directory:
        root=Path(directory)
        qualification(root)
        (root/'failed').mkdir()
        failed_qualification(root/'failed')
        floors_and_override(root)
    paired()
    print(json.dumps(dict(schema='ehgp.v11.full_capture_reader_selftest.v1',checks=CHECKS,status='ok',
                          scope='synthetic receipts/archives only; no product, build, subprocess or cloud'),sort_keys=True))


if __name__=='__main__':
    main()
