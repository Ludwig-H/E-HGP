#!/usr/bin/env python3
"""March qualification/schedule/real FULL decoding, with all child processes simulated."""
import argparse
import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT/'bench'), str(ROOT/'tests/tower')]
import full_march as driver
import march_variants as variants
from full_parallel_collector_test import stream, VALUE
from full_bench_semantic_test import encode

full = driver.full
CHECKS = 0
COUNTS = dict(campaigns=0, children=0, decodes=0, corruptions=0, provenance=0, interruptions=0)


def check(value, label):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(label)


def refuses(call, label):
    try:
        call()
    except (ValueError, OSError, KeyError, TypeError):
        COUNTS['corruptions'] += 1
        check(True, label)
    else:
        raise ValueError('accepted corruption: '+label)


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value))


def text_record(path, text):
    blob = text.encode()
    return dict(path=path, text=text, size=len(blob), sha256=hashlib.sha256(blob).hexdigest())


def selected_rows(mutant=False, modules=None):
    if mutant:
        modules = modules or [p.stem for p in (ROOT/'tests/mutants').glob('*.json')]
        names = ['mhgp11_mutants_'+name+suffix for name in modules
                 for suffix in ('', '_manifest', '_manifest_opt')]
        labels = ['mutant']
    else:
        names = sorted(variants.REQUIRED)+['mhgp11_fixture_%03d' % i for i in range(540-len(variants.REQUIRED))]
        labels = ['oracle', 'unit']
    return [dict(name=name, labels=labels[:], disabled=False) for name in sorted(names)]


def fake_build(root, name, bits, variant, march):
    folder, build = root/'matrix'/name, root/'builds'/name/'build'
    folder.mkdir(parents=True)
    build.mkdir(parents=True)
    records = []
    for file in ('mhgp11_full_bench', 'libmhgp11.a'):
        blob = (file+name).encode()
        (build/file).write_bytes(blob)
        records.append(dict(path=file, size=len(blob), sha256=hashlib.sha256(blob).hexdigest()))
    entries = {'MHGP11_COORD_BITS': str(bits), 'MHGP11_MARCH': march,
               'CMAKE_INTERPROCEDURAL_OPTIMIZATION': 'OFF', 'CMAKE_BUILD_TYPE': 'Release',
               'CMAKE_CXX_FLAGS': '', 'CMAKE_CXX_FLAGS_RELEASE': '-O3 -DNDEBUG',
               'MHGP11_SANITIZE': 'OFF', 'MHGP11_TSAN': 'OFF', 'MHGP11_POISON': 'OFF'}
    cache = ''.join(k+':STRING='+v+'\n' for k, v in entries.items())
    records.append(text_record('CMakeCache.txt', cache))
    flags = '-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror '+('-march='+march+' ' if march else '')+'-std=c++20'
    for target in ('mhgp11', 'mhgp11_full_bench'):
        records.append(text_record('CMakeFiles/'+target+'.dir/flags.make', 'CXX_FLAGS = '+flags+'\n'))
        records.append(text_record('CMakeFiles/'+target+'.dir/link.txt', '/usr/bin/g++ -O3 main.o libmhgp11.a -o program\n'))
    provenance = dict(schema='ehgp.v11.build_provenance.v1', complete=True, errors=[], files=records)
    write(folder/'build_provenance.json', provenance)
    mutant = name in ('mutant_parent', variants.MUTANTS)
    selected = selected_rows(mutant)
    write(folder/'tests.json', selected)
    config = dict(name=name, status='ok', compiler='g++', compiler_path='/usr/bin/g++', compiler_version='GCC test',
                  cmake_options=variants.options(bits, march),
                  ctest_args=(variants.MUTANT_SELECTION if mutant else variants.RELEASE_SELECTION)[:],
                  tests=dict(selected=len(selected), passed=len(selected), failed=0, not_run=0,
                             ctest_total=len(selected), ctest_failed=0))
    return folder, build, config, provenance


def provenance_tests(root):
    for name, (bits, variant, march) in variants.VARIANTS.items():
        folder, build, config, value = fake_build(root, name, bits, variant, march)
        output = variants.build_record(folder, build, config, bits, variant, march)
        COUNTS['provenance'] += 1
        check(output['coord_bits'] == bits and output['march'] == march and output['build_variant'] == variant,
              'build profile and variant')
    folder, build, config, value = fake_build(root, 'test', 21, 'v3', 'x86-64-v3')
    for text in ('-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -std=c++20',
                 '-O3 -march=native', '-O3 -march=x86-64-v3 -march=x86-64', '-Ofast', '-flto'):
        refuses(lambda: variants.flags('CXX_FLAGS = '+text+'\n', 'x86-64-v3'), 'effective flags')
    mutations = [('MHGP11_MARCH:STRING=x86-64-v3', 'MHGP11_MARCH:STRING='),
                 ('MHGP11_COORD_BITS:STRING=21', 'MHGP11_COORD_BITS:STRING=24'),
                 ('CMAKE_INTERPROCEDURAL_OPTIMIZATION:STRING=OFF', 'CMAKE_INTERPROCEDURAL_OPTIMIZATION:STRING=ON'),
                 ('CMAKE_CXX_FLAGS:STRING=', 'CMAKE_CXX_FLAGS:STRING=-march=native'),
                 ('MHGP11_SANITIZE:STRING=OFF', 'MHGP11_SANITIZE:STRING=ON')]
    for old, new in mutations:
        bad = copy.deepcopy(value)
        row = next(r for r in bad['files'] if r['path'] == 'CMakeCache.txt')
        row.update(text_record(row['path'], row['text'].replace(old, new)))
        write(folder/'build_provenance.json', bad)
        refuses(lambda: variants.build_record(folder, build, config, 21, 'v3', 'x86-64-v3'), 'coherently rehashed cache')
    for mutation in ('hash', 'duplicate', 'missing_link', 'lto_link'):
        bad = copy.deepcopy(value)
        if mutation == 'hash':
            bad['files'][0]['sha256'] = 'f'*64
        elif mutation == 'duplicate':
            bad['files'].append(bad['files'][0])
        elif mutation == 'missing_link':
            bad['files'] = [r for r in bad['files'] if not r['path'].endswith('mhgp11.dir/link.txt')]
        else:
            row = next(r for r in bad['files'] if r['path'].endswith('mhgp11.dir/link.txt'))
            row.update(text_record(row['path'], '/usr/bin/g++ -flto main.o'))
        write(folder/'build_provenance.json', bad)
        refuses(lambda: variants.build_record(folder, build, config, 21, 'v3', 'x86-64-v3'), mutation)
    write(folder/'build_provenance.json', value)
    write(folder/'tests.json', [])
    refuses(lambda: variants.build_record(folder, build, config, 21, 'v3', 'x86-64-v3'), 'missing exact gates')
    folder, _build, _config, _value = fake_build(root, 'mutant_parent', 18, 'v3', 'x86-64-v3')
    source = root/'source'
    (source/'tests/mutants').mkdir(parents=True)
    (source/'CMakeLists.txt').write_text('-DMHGP11_MARCH=${MHGP11_MARCH}')
    (source/'tests/mutants/run_mutants.py').write_text('self.args.cmake_arg + list(options)')
    modules = ['num', 'core', 'cloud', 'index', 'tower', 'catalogue', 'sched']
    for module in modules:
        write(source/'tests/mutants'/(module+'.json'), dict(mutants=[dict(options=[])]))
    manifest = source/'tests/mutants/num.json'
    write(folder/'tests.json', selected_rows(True, modules))
    log = ''.join('%d/21 Testing: mhgp11_mutants_%s\nCommand: cmake --cmake-arg=-DMHGP11_MARCH=x86-64-v3\nTest Passed.\n'
                  % (i+1, name) for i, name in enumerate(modules))
    (folder/'LastTest.log').write_text(log)
    parent = dict(status='ok', cmake_options=variants.options(18, 'x86-64-v3')+['-DMHGP11_MUTANT_JOBS={threads}'],
                  ctest_args=['-L', '^mutant$'],
                  tests=dict(selected=21, passed=21, failed=0, not_run=0, ctest_total=21, ctest_failed=0))
    proof = variants.mutant_inheritance(folder, parent, source)
    check(proof['individual_clone_flags_observed'] is False and len(proof['modules']) == 7, 'inheritance scope explicit')
    for text in (log.replace('x86-64-v3', ''), log.replace('Test Passed.', 'Test Failed.'), log+log):
        (folder/'LastTest.log').write_text(text)
        refuses(lambda: variants.mutant_inheritance(folder, parent, source), 'mutant command/pass/unique')
    (folder/'LastTest.log').write_text(log)
    write(manifest, dict(mutants=[dict(options=['-DMHGP11_MARCH='])]))
    refuses(lambda: variants.mutant_inheritance(folder, parent, source), 'mutant option overrides march')



def qualification_tests(root):
    configs = []
    for name, (bits, variant, march) in variants.VARIANTS.items():
        _folder, _build, config, _value = fake_build(root, name, bits, variant, march)
        configs.append(config)
    folder, _build, config, _value = fake_build(root, variants.MUTANTS, 18, 'v3', 'x86-64-v3')
    config['cmake_options'].append('-DMHGP11_MUTANT_JOBS={threads}')
    configs.append(config)
    manifests = sorted((ROOT/'tests/mutants').glob('*.json'))
    log = ''.join('%d/21 Testing: mhgp11_mutants_%s\nCommand: cmake --cmake-arg=-DMHGP11_MARCH=x86-64-v3\nTest Passed.\n' %
                  (i+1, manifest.stem) for i, manifest in enumerate(manifests))
    (folder/'LastTest.log').write_text(log)
    summary = dict(schema='ehgp.v11.g4_matrix_summary.v1', complete=True, conforming=True,
                   exit_code=0, signals=[], configurations=configs)
    args = argparse.Namespace(qualification=root/'matrix/summary.json', builds=root/'builds')
    write(args.qualification, summary)
    builds, proof = variants.checked_builds(args)
    check(len(builds) == 4 and len(proof['modules']) == len(manifests), 'all real source inheritance anchors')
    for kind in ('false_exit', 'signal', 'incomplete', 'missing', 'duplicate', 'compiler', 'parent_failed',
                 'parent_bool', 'wrong_options', 'release_selector', 'parent_selector'):
        bad = copy.deepcopy(summary)
        if kind == 'false_exit':
            bad['exit_code'] = False
        elif kind == 'signal':
            bad['signals'] = [15]
        elif kind == 'incomplete':
            bad['complete'] = False
        elif kind == 'missing':
            bad['configurations'].pop()
        elif kind == 'duplicate':
            bad['configurations'].append(bad['configurations'][0])
        elif kind == 'compiler':
            bad['configurations'][0]['compiler_version'] = 'another compiler'
        elif kind == 'parent_failed':
            bad['configurations'][-1]['tests'].update(passed=20, failed=1)
        elif kind == 'parent_bool':
            bad['configurations'][-1]['tests']['failed'] = False
        elif kind == 'wrong_options':
            bad['configurations'][0]['cmake_options'][2] = '-DMHGP11_MARCH=native'
        else:
            bad['configurations'][-1 if kind == 'parent_selector' else 0]['ctest_args'].extend(['-E', 'new_gate'])
        write(args.qualification, bad)
        refuses(lambda: variants.checked_builds(args), 'qualification '+kind)
    write(args.qualification, summary)
    original = json.loads((folder/'build_provenance.json').read_text())
    for mode in ('parent_cache', 'parent_flags', 'parent_provenance'):
        bad = copy.deepcopy(original)
        if mode == 'parent_provenance':
            bad['complete'] = False
        else:
            name = 'CMakeCache.txt' if mode == 'parent_cache' else 'CMakeFiles/mhgp11.dir/flags.make'
            row = next(r for r in bad['files'] if r['path'] == name)
            row.update(text_record(name, row['text'].replace('x86-64-v3', 'x86-64')))
        write(folder/'build_provenance.json', bad)
        refuses(lambda: variants.checked_builds(args), mode)
    write(folder/'build_provenance.json', original)
    inventory_tests(args, summary)


def inventory_tests(args, summary):
    for config in summary['configurations'][:1]+summary['configurations'][-1:]:
        path = args.qualification.parent/config['name']/'tests.json'
        original = json.loads(path.read_text())
        for kind in ('short', 'duplicate', 'disabled', 'bool_disabled', 'labels', 'wrong_selector', 'unknown'):
            bad = copy.deepcopy(original)
            if kind == 'short':
                bad.pop()
            elif kind == 'duplicate':
                bad[1] = bad[0]
            elif kind == 'disabled':
                bad[0]['disabled'] = True
            elif kind == 'bool_disabled':
                bad[0]['disabled'] = 0
            elif kind == 'labels':
                bad[0]['labels'].append(False)
            elif kind == 'wrong_selector':
                bad[0]['labels'] = [] if config['name'] == variants.MUTANTS else ['mutant']
            else:
                bad[0]['name'] = 'mhgp11_new_fake_gate'
            write(path, bad)
            refuses(lambda: variants.checked_builds(args), config['name']+' inventory '+kind)
        write(path, original)
        for field in ('ctest_total', 'ctest_failed'):
            bad = copy.deepcopy(summary)
            next(c for c in bad['configurations'] if c['name'] == config['name'])['tests'][field] += 1
            write(args.qualification, bad)
            refuses(lambda: variants.checked_builds(args), config['name']+' count '+field)
        write(args.qualification, summary)
    # Replacing an extra gate while keeping the floor/count coherent must still differ from peers.
    name = next(iter(variants.VARIANTS))
    path = args.qualification.parent/name/'tests.json'
    original = json.loads(path.read_text())
    bad = copy.deepcopy(original)
    bad[0]['name'] = 'mhgp11_new_fake_gate'
    write(path, bad)
    refuses(lambda: variants.checked_builds(args), 'same size, changed nonrequired gate across variants')
    write(path, original)


def plan_tests():
    import importlib.util
    spec = importlib.util.spec_from_file_location('march_matrix_validation', ROOT/'tools/g4_matrix.py')
    matrix = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = matrix
    spec.loader.exec_module(matrix)
    config = matrix.load_matrix(ROOT/'bench/march_matrix.json')
    check({c['name'] for c in config['configurations']} == set(variants.VARIANTS) | {variants.MUTANTS}, 'matrix identity')
    check(sum(c['threads'] for c in config['configurations']) == 48, 'declared CPU allocation')
    check(all(c['env'] == {'CXXFLAGS':'', 'LDFLAGS':''} for c in config['configurations']), 'ambient compiler flags cleared')
    plan = json.loads((ROOT/'bench/plans/full_march_g4.json').read_text())
    check(sum(c['timeout_seconds'] for c in plan['commands'])+120 == 2220, 'declared guarded command budget')
    check(len(driver.schedule()) == 24 and len(set(map(driver.identity, driver.schedule()))) == 24, 'paired schedule cardinality')
    for repeat in (0, 1):
        rows = [r for r in driver.schedule() if r['repetition'] == repeat]
        check(sum(rows[i]['build_variant'] == 'baseline' for i in range(0, len(rows), 2)) == 3,
              'balanced first variant per repetition')
    first, second = driver.schedule()[:12], driver.schedule()[12:]
    check(all(a['build_variant'] != b['build_variant'] for a, b in zip(first, second)), 'pair order reverses')


def campaign(root, label, *, mode=4095, mutation=None, process=None, interrupt=None, deadline=False):
    COUNTS['campaigns'] += 1
    directory = root/label
    directory.mkdir()
    args = argparse.Namespace(builds=directory/'builds', data=directory/'data', out=directory/'out',
                              work=directory/'work', qualification=directory/'qualification.json',
                              repetitions=2, optimizations=mode, leaf_size=16, budget_seconds=680)
    args.qualification.write_text('{}')
    builds = {(variant, bits): dict(configuration=name, coord_bits=bits, build_variant=variant,
               path=str(directory/name), sha256=hashlib.sha256(name.encode()).hexdigest())
              for name, (bits, variant, _march) in variants.VARIANTS.items()}
    requested = driver.schedule(2, mode)
    cases = [dict(name=name, count=5, coordinates=name+'.xyz', point_ids=name+'.ids',
                  sha256=hashlib.sha256((name+'xyz').encode()).hexdigest(),
                  ids_sha256=hashlib.sha256((name+'ids').encode()).hexdigest())
             for name in sorted({r['case'] for r in requested})]
    seen, snapshots = [], []
    decode, save = full.semantic.decode, base_save()

    def child(argv, **kwargs):
        ordinal = len(seen)
        req = requested[ordinal]
        seen.append(req)
        COUNTS['children'] += 1
        check(argv[0] == builds[req['build_variant'], req['coord_bits']]['path'], 'selected binary')
        check(Path(argv[3]).parent.name == req['build_variant'], 'disjoint variant payload directory')
        check(kwargs['timeout'] == full.TIMEOUT and kwargs['check'] is False, 'bounded process')
        if interrupt == 'before':
            raise KeyboardInterrupt('before checkpoint')
        if process == 'launch' and ordinal == 0:
            raise OSError('fake launcher')
        if process == 'timeout' and ordinal == 0:
            raise subprocess.TimeoutExpired(argv, full.TIMEOUT, output=b'partial')
        events = stream(req)
        if mutation:
            mutation(events, ordinal)
        Path(argv[3]).write_bytes(encode(VALUE, req['coord_bits'])[0])
        code = 2 if process == 'refused' and ordinal == 0 else 0
        return subprocess.CompletedProcess(argv, code, '\n'.join(json.dumps(e) for e in events).encode(), b'')

    def counted_decode(*values):
        COUNTS['decodes'] += 1
        if interrupt == 'after':
            raise KeyboardInterrupt('after checkpoint')
        return decode(*values)

    def saved(path, value):
        save(path, value)
        snapshots.append(json.loads(path.read_text()))

    patches = [patch.object(variants, 'checked_builds', return_value=(builds, {'individual_clone_flags_observed': False})),
               patch.object(driver.profiles, 'inputs', return_value=({'cases': cases}, 'b'*64)),
               patch.object(full.subprocess, 'run', side_effect=child),
               patch.object(full.semantic, 'decode', side_effect=counted_decode),
               patch.object(driver.base, 'save', side_effect=saved)]
    if deadline:
        patches.append(patch.object(driver.time, 'monotonic', side_effect=[0]+[1000]*100))
    with contextlib.ExitStack() as stack:
        for p in patches:
            stack.enter_context(p)
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        try:
            result = driver.run(args)
        except KeyboardInterrupt:
            check(interrupt is not None, 'unexpected interruption')
            result = 'interrupted'
            COUNTS['interruptions'] += 1
    report = json.loads((args.out/'full_march.json').read_text())
    check(snapshots[-1] == report, 'real saved checkpoint is final observed state')
    return result, report, snapshots, seen


def base_save():
    return driver.base.save


def campaign_tests(root):
    before = COUNTS['decodes']
    code, report, snapshots, seen = campaign(root, 'happy')
    check(code == 0 and report['conforming'] and len(seen) == 24, 'all paired runs complete')
    check(COUNTS['decodes']-before == 12, 'separate variant caches decode twelve contexts')
    check(sum(r['semantic_reuse']['mode'] == 'reused' for r in report['runs']) == 12, 'twelve full-rehash reuses')
    check(all(c['status'] == 'equal' for c in report['comparisons']), 'geometry/raw/paid work equality')
    check(snapshots[0]['runs'] == [] and snapshots[2]['runs'][0]['status'] == 'pending_semantic', 'snapshots do not alias')
    for row in report['runs']:
        check(row['semantic_cache_scope'] == row['build_variant'], 'cache scope')
    for process in ('launch', 'timeout', 'refused'):
        code, bad, _shots, seen = campaign(root, process, process=process)
        check(code == 1 and len(seen) == 24 and len(bad['runs']) == 24 and not bad['not_run'], 'failures never suppress sibling')
        check(bad['runs'][0]['status'] == {'launch': 'launch_error', 'timeout': 'timeout', 'refused': 'refused'}[process], 'failure retained')
    def bad_current(events, ordinal):
        if ordinal == 12:
            events[2]['orders'][0]['work']['singleton_hits'] += 1
    code, bad, _shots, _seen = campaign(root, 'cache_guard', mutation=bad_current)
    check(code == 1 and bad['runs'][12]['status'] == 'invalid_output', 'cached bytes never bypass current work')
    def work_difference(events, ordinal):
        if ordinal == 0:
            events[2]['orders'][0]['work']['part_meb_presentations'] += 1
    code, bad, _shots, _seen = campaign(root, 'work_difference', mutation=work_difference)
    check(code == 1 and bad['runs'][0]['status'] == 'ok' and bad['comparisons'][0]['status'] == 'different',
          'valid events but different paid work cannot pass')
    code, bad, _shots, seen = campaign(root, 'budget', deadline=True)
    check(code == 1 and not seen and len(bad['not_run']) == 24 and bad['complete'], 'all budget omissions preserved')
    for position, expected in (('before', 0), ('after', 1)):
        code, bad, _shots, _seen = campaign(root, 'interrupt_'+position, interrupt=position)
        check(code == 'interrupted' and not bad['complete'] and not bad['conforming'] and
              len(bad['launch_intents']) == 1 and len(bad['runs']) == expected, 'interruption checkpoint inventory')
        if expected:
            check(bad['runs'][0]['status'] == 'pending_semantic', 'pending decoder evidence retained')
    requested = driver.schedule()
    for mutation in ('raw', 'semantic', 'work'):
        bad = copy.deepcopy(report['runs'])
        if mutation in ('raw', 'semantic'):
            bad[1]['semantic']['raw_sha256' if mutation == 'raw' else 'sha256'] = 'f'*64
        else:
            bad[1]['events'][1]['catalogue_work']['prefixes'] += 1
        check(driver.comparisons(bad, requested)[0]['status'] == 'different', 'comparison '+mutation)
        COUNTS['corruptions'] += 1
    refuses(lambda: driver.comparisons(report['runs']+[report['runs'][0]], requested), 'duplicate variant request')
    refuses(lambda: driver.schedule(True), 'boolean repetitions')
    refuses(lambda: driver.schedule(1), 'unpaired repetition floor')


def main():
    with tempfile.TemporaryDirectory(prefix='full-march-model-') as directory:
        root = Path(directory)
        provenance_tests(root/'provenance')
        qualification_tests(root/'qualification')
        campaign_tests(root)
        plan_tests()
    print('full_march_collector_verdict conforme '+
          ' '.join(k+str(v) for k, v in COUNTS.items())+' checks'+str(CHECKS)+' native0')


if __name__ == '__main__':
    main()
