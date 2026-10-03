"""Pinned Release variants; inspect actual binary/cache/flags and explicit mutant CLI inheritance.

The parent mutant suite is required. Deleted mutant clones are NOT claimed to have had
flags individually captured: LastTest records the forwarded option and the pinned recipe.
"""
import hashlib
from pathlib import Path
import re
import shlex

import catalogue_profiles as profiles

need, load, digest = profiles.semantic.need, profiles.load, profiles.base.digest
VARIANTS = {'baseline21': (21, 'baseline', ''), 'v3_21': (21, 'v3', 'x86-64-v3'),
            'baseline24': (24, 'baseline', ''), 'v3_24': (24, 'v3', 'x86-64-v3')}
MUTANTS = 'mutants_v3'
RELEASE_SELECTION = ['-LE', '^mutant$', '-E', '^mhgp11_reference_']
MUTANT_SELECTION = ['-L', '^mutant$']
REQUIRED = {'mhgp11_'+unit+'_'+gate+suffix for unit, gate in (
    ('num', 'fraction'), ('num', 'checked_power_fraction'), ('num', 'orientation_certificate_fraction'),
    ('tower', 'descent_fraction'), ('tower', 'forest_fraction'), ('tower', 'forest_parallel_fraction'))
    for suffix in ('', '_opt')}


def options(bits, march):
    return ['-DCMAKE_BUILD_TYPE=Release', '-DMHGP11_COORD_BITS='+str(bits), '-DMHGP11_MARCH='+march,
            '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF', '-DCMAKE_CXX_FLAGS=',
            '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG']


def exact_text(row):
    text = row['text']
    need(type(text) is str and len(text.encode()) == row['size'] and
         hashlib.sha256(text.encode()).hexdigest() == row['sha256'], 'provenance text integrity')
    return text


def entry(cache, key):
    values = re.findall(r'^'+re.escape(key)+r':[^=\n]+=([^\n]*)$', cache, re.M)
    need(len(values) == 1, 'unique cache entry '+key)
    return values[0]


def flags(text, march):
    found = re.findall(r'^CXX_FLAGS = (.*)$', text, re.M)
    need(len(found) == 1, 'compile flags missing/duplicate')
    words = shlex.split(found[0])
    expected = ['-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-std=c++20']
    if march:
        expected.append('-march='+march)
    need(sorted(words) == sorted(expected), 'effective compilation flags differ from experiment')


def inventory(folder, config, mutant=False):
    expected = MUTANT_SELECTION if mutant else RELEASE_SELECTION
    need(config['ctest_args'] == expected, 'qualification selector differs from experiment')
    counts = config['tests']
    need(all(type(counts[k]) is int for k in ('selected', 'passed', 'failed', 'not_run', 'ctest_total',
                                             'ctest_failed')) and
         counts['selected'] == counts['passed'] == counts['ctest_total'] >= (21 if mutant else 540) and
         counts['failed'] == counts['not_run'] == counts['ctest_failed'] == 0, 'qualification incomplete')
    selected = profiles.base.event_json('{"tests":'+(folder/'tests.json').read_text()+'}')['tests']
    need(type(selected) is list and len(selected) == counts['selected'], 'selected inventory size differs')
    for row in selected:
        need(type(row) is dict and set(row) == {'name', 'labels', 'disabled'} and
             type(row['name']) is str and bool(row['name']) and row['disabled'] is False and
             type(row['labels']) is list and all(type(v) is str for v in row['labels']) and
             row['labels'] == sorted(set(row['labels'])), 'malformed/disabled inventory row')
        need(('mutant' in row['labels']) == mutant and
             (mutant or not row['name'].startswith('mhgp11_reference_')), 'inventory violates selector')
    names = {row['name'] for row in selected}
    need(len(names) == len(selected), 'duplicate selected test')
    return selected, names


def build_record(folder, build, config, bits, variant, march):
    need(config['status'] == 'ok' and config['compiler'] == 'g++' and
         config['cmake_options'] == options(bits, march), 'variant configuration identity')
    _selected, names = inventory(folder, config)
    need(REQUIRED <= names, 'native exact gates missing')
    path = folder/'build_provenance.json'
    value = load(path)
    need(value['schema'] == 'ehgp.v11.build_provenance.v1' and value['complete'] is True and
         not value['errors'], 'build provenance incomplete')
    records = {r['path']: r for r in value['files']}
    need(len(records) == len(value['files']), 'duplicate provenance path')
    cache = exact_text(records['CMakeCache.txt'])
    for key, expected in {'MHGP11_COORD_BITS': str(bits), 'MHGP11_MARCH': march,
                          'CMAKE_INTERPROCEDURAL_OPTIMIZATION': 'OFF', 'CMAKE_BUILD_TYPE': 'Release',
                          'CMAKE_CXX_FLAGS': '', 'CMAKE_CXX_FLAGS_RELEASE': '-O3 -DNDEBUG',
                          'MHGP11_SANITIZE': 'OFF', 'MHGP11_TSAN': 'OFF', 'MHGP11_POISON': 'OFF'}.items():
        need(entry(cache, key) == expected, 'variant cache '+key)
    for target in ('mhgp11', 'mhgp11_full_bench'):
        flags(exact_text(records['CMakeFiles/'+target+'.dir/flags.make']), march)
        link = shlex.split(exact_text(records['CMakeFiles/'+target+'.dir/link.txt']))
        need(not any(w.startswith(('-flto', '-march', '-mtune', '@')) or w in ('-Ofast', '-ffast-math')
                     for w in link), 'unexpected link flags')
    for name in ('libmhgp11.a', 'mhgp11_full_bench'):
        file = build/name
        need(file.is_file() and file.stat().st_size == records[name]['size'] and
             digest(file) == records[name]['sha256'], 'actual build artifact changed')
    exe = records['mhgp11_full_bench']
    return dict(configuration=config['name'], coord_bits=bits, build_variant=variant, march=march,
                path=str(build/'mhgp11_full_bench'), sha256=exe['sha256'], bytes=exe['size'],
                provenance_sha256=digest(path), cache_sha256=records['CMakeCache.txt']['sha256'],
                tests_sha256=digest(folder/'tests.json'), selected_tests=len(names),
                compiler=config['compiler_path'], compiler_version=config['compiler_version'])


def mutant_inheritance(folder, config, source):
    expected = options(18, 'x86-64-v3')+['-DMHGP11_MUTANT_JOBS={threads}']
    need(config['status'] == 'ok' and config['cmake_options'] == expected, 'v3 mutant parent configuration')
    _selected, names = inventory(folder, config, mutant=True)
    provenance_path = folder/'build_provenance.json'
    provenance = load(provenance_path)
    need(provenance['complete'] is True and not provenance['errors'], 'mutant parent provenance incomplete')
    records = {r['path']: r for r in provenance['files']}
    need(len(records) == len(provenance['files']), 'duplicate mutant parent provenance')
    cache = exact_text(records['CMakeCache.txt'])
    need(entry(cache, 'MHGP11_MARCH') == 'x86-64-v3' and entry(cache, 'MHGP11_COORD_BITS') == '18' and
         entry(cache, 'CMAKE_INTERPROCEDURAL_OPTIMIZATION') == 'OFF', 'mutant parent flags cache')
    flags(exact_text(records['CMakeFiles/mhgp11.dir/flags.make']), 'x86-64-v3')
    cmake = (source/'CMakeLists.txt').read_text()
    recipe = source/'tests/mutants/run_mutants.py'
    need('-DMHGP11_MARCH=${MHGP11_MARCH}' in cmake and
         'self.args.cmake_arg + list(options)' in recipe.read_text(), 'mutant forwarding recipe changed')
    log = (folder/'LastTest.log').read_text()
    manifests = sorted((source/'tests/mutants').glob('*.json'))
    need(bool(manifests), 'mutant manifests missing')
    expected_names = {'mhgp11_mutants_'+manifest.stem+suffix for manifest in manifests
                      for suffix in ('', '_manifest', '_manifest_opt')}
    need(names == expected_names, 'mutant parent exact manifest inventory differs')
    modules = []
    for manifest in manifests:
        value = load(manifest)
        need(not any(o.startswith(('-DMHGP11_MARCH', '-DCMAKE_CXX_FLAGS', '-DCMAKE_INTERPROCEDURAL'))
                     for m in value['mutants'] for o in m.get('options', [])), 'mutant overrides compilation variant')
        name = 'mhgp11_mutants_'+manifest.stem
        sections = re.findall(r'^\d+/\d+ Testing: '+re.escape(name)+r'\n(.*?)(?=^\d+/\d+ Testing:|\Z)', log,
                              re.M | re.S)
        need(len(sections) == 1, 'unique mutant parent test '+name)
        command = re.findall(r'^Command: (.*)$', sections[0], re.M)
        need(len(command) == 1 and '--cmake-arg=-DMHGP11_MARCH=x86-64-v3' in command[0] and
             re.search(r'^Test Passed\.', sections[0], re.M), 'mutant inheritance/pass not recorded '+name)
        modules.append(dict(module=manifest.stem, manifest_sha256=digest(manifest),
                            command=command[0], inherited_march='x86-64-v3'))
    return dict(parent_configuration=MUTANTS, provenance_sha256=digest(provenance_path),
                tests_sha256=digest(folder/'tests.json'), selected_tests=len(names),
                source_cmake_sha256=digest(source/'CMakeLists.txt'),
                runner_sha256=digest(recipe), last_test_sha256=digest(folder/'LastTest.log'), modules=modules,
                individual_clone_flags_observed=False,
                scope='Passed parent suites plus explicit forwarded command and pinned recipe; clones deleted')


def checked_builds(args):
    summary = load(args.qualification)
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         summary['conforming'] is True and type(summary['exit_code']) is int and summary['exit_code'] == 0 and
         not summary.get('signals'), 'variant qualification not complete/conforming')
    configs = {c['name']: c for c in summary['configurations']}
    need(len(configs) == len(summary['configurations']) and set(configs) == set(VARIANTS) | {MUTANTS},
         'exact four variants and mutant parent required')
    result = {}
    for name, (bits, variant, march) in VARIANTS.items():
        result[variant, bits] = build_record(args.qualification.parent/name, args.builds/name/'build',
                                            configs[name], bits, variant, march)
    inventories = [inventory(args.qualification.parent/name, configs[name])[0] for name in VARIANTS]
    signatures = {tuple(sorted((r['name'], tuple(r['labels'])) for r in rows)) for rows in inventories}
    need(len(signatures) == 1, 'selected tests differ across the four compilation variants')
    need(len({(c['compiler_path'], c['compiler_version']) for c in configs.values()}) == 1,
         'different compiler versions across variants')
    proof = mutant_inheritance(args.qualification.parent/MUTANTS, configs[MUTANTS], Path(__file__).resolve().parents[1])
    return result, proof
