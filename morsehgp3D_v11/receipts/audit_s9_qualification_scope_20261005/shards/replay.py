#!/usr/bin/env python3
"""Addendum de selection : archive B + declarations S9, sans CMake/natif/cloud."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
REPO = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path('/workspaces/E-HGP')
ARCHIVE = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else Path(
    '/workspaces/.ehgp-sessions/v11.20261005.claudequalb/results/results.tar.gz')
PIN = 'd26328fe2216133ba40385a9673325bbdcf03ea7'
ARCHIVE_SHA = '8faaacf3c58cf1b6ebf5ff96e2d454de5b4c684f881fb28ce7015a6bdafae3c7'
S9_SHA = {
    'morsehgp3D_v11/cmake/gates.cmake': '8abaa4b9a533f0988cd2a4b436665905c3ad1be92a4f33f360da2fce34c1d70a',
    'morsehgp3D_v11/tests/cli/tests.cmake': 'e4715bfb2410f8baf0bd659c464e397aec5765695190d2a12a03cda30e36f140',
    'morsehgp3D_v11/tests/points/tests.cmake': 'fab2d65223c63c72f62dd9f2bffe631e83a6a84607f8c6ee17d02c6ce296175a'}


def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)


def source(path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', PIN + ':' + path])


def selected(test, args):
    for option, negate, labels in [('-R', False, False), ('-E', True, False),
                                  ('-L', False, True), ('-LE', True, True)]:
        if option not in args:
            continue
        pattern = args[args.index(option) + 1]
        matches = any(re.search(pattern, value) for value in
                      (test['labels'] if labels else [test['name']]))
        if matches == negate:
            return False
    return True


for path, sha in S9_SHA.items():
    need(hashlib.sha256(source(path)).hexdigest() == sha,
         'declaration S9 differente de la projection c977 : ' + path)
matrix_bytes = source('morsehgp3D_v11/tools/g4_matrix.json')
configs = {item['name']: item for item in json.loads(matrix_bytes)['configurations']}
need(hashlib.sha256(ARCHIVE.read_bytes()).hexdigest() == ARCHIVE_SHA, 'archive B differente')
s9 = []


def gate(name, labels, twin=False):
    s9.append(dict(name=name, labels=labels))
    if twin:
        s9.append(dict(name=name + '_opt', labels=labels))


for group in ['ancestors', 'determinism', 'sort_refusal', 'refusals', 'budget', 'inventaire']:
    gate('mhgp11_points_unit_' + group, ['unit', 'fast'])
gate('mhgp11_points_fixtures', ['fast'], True)
gate('mhgp11_points_oracle', ['oracle', 'fast'], True)
gate('mhgp11_cli_points', ['fast'], True)
gate('mhgp11_points_vs_python', ['long'])
for frame in ['ng00', 'ng01', 'ng02']:
    gate('mhgp11_points_vs_python_lidar_' + frame + '_k5', ['lidar', 'long'])
    gate('mhgp11_points_lidar_' + frame + '_k5', ['lidar'], True)
for size in [8000, 16000, 32000]:
    gate('mhgp11_points_scale' + str(size), ['scale' + str(size)], True)

families = []
with tarfile.open(ARCHIVE, 'r:gz') as archive:
    for base, sanitizer, shard_prefix in [('gcc_asan_ubsan', 'address,undefined', 'gcc_asan_scale'),
                                          ('gcc_tsan', 'thread', 'gcc_tsan_')]:
        shard_names = [name for name in configs if name != base and name.startswith(shard_prefix)]
        names = [base] + shard_names
        need(len(shard_names) == (2 if base == 'gcc_asan_ubsan' else 6), 'nombre de lots')
        need(all(configs[name].get('sanitizer') == sanitizer for name in names), 'instrumentation')
        need(all(configs[name]['cmake_options'] == configs[base]['cmake_options'] for name in names),
             'profil/options differents entre lots')
        prefix = 'results/cmd/000_matrice/files/matrix/' + base + '/'
        tests_bytes = archive.extractfile(prefix + 'tests.json').read()
        result_bytes = archive.extractfile(prefix + 'result.json').read()
        tests, result = json.loads(tests_bytes), json.loads(result_bytes)
        all_names = {test['name'] for test in tests}
        missing = {test['test'] for test in result['not_run']}
        old_sets = {name: {test['name'] for test in tests if selected(test, configs[name]['ctest_args'])}
                    for name in names}
        s9_sets = {name: {test['name'] for test in s9 if selected(test, configs[name]['ctest_args'])}
                   for name in names}
        old_union, s9_union = set().union(*old_sets.values()), set().union(*s9_sets.values())
        need(old_union == all_names, 'union historique incomplete')
        need(sum(map(len, old_sets.values())) == len(old_union), 'intersection historique non vide')
        need(len(old_sets[base]) == 742 and len(old_union - old_sets[base]) == 82, '82 portes restaurees')
        need(missing <= (old_union - old_sets[base]), 'ancienne absence non couverte par les lots')
        need(len(s9_union) == 24 and sum(map(len, s9_sets.values())) == 24, 'projection S9 non disjointe/complete')
        need(len(s9_sets[base]) == 12, 'portes courtes S9')
        need(all(len(old_sets[name]) >= configs[name].get('min_tests', 1) for name in shard_names),
             'plancher impossible sur inventaire B')
        families.append(dict(base=base, historical_inventory_count=len(tests),
            historical_union_count=len(old_union), historical_disjoint=True,
            historical_restored_count=82, historical_missing_count=len(missing),
            historical_missing_covered=True, s9_static_projected_total=28,
            s9_static_selected_union=sorted(s9_union), s9_static_selected_count=len(s9_union),
            s9_static_disjoint=True,
            lots=[dict(configuration=name, ctest_args=configs[name]['ctest_args'],
                       historical_selected_count=len(old_sets[name]),
                       historical_missing_selected_count=len(old_sets[name] & missing),
                       min_tests=configs[name].get('min_tests'), s9_selected=sorted(s9_sets[name]))
                  for name in names],
            tests_sha256=hashlib.sha256(tests_bytes).hexdigest(),
            result_sha256=hashlib.sha256(result_bytes).hexdigest()))

diff_names = {test['name'] for test in s9 if '_vs_python' in test['name']}
diff_selected = {name: sorted(test['name'] for test in s9 if test['name'] in diff_names and
                            selected(test, config['ctest_args'])) for name, config in configs.items()}
need(len(diff_names) == 4 and not any(diff_selected.values()), 'selection differentiels changee')
summary = dict(schema='ehgp.v11.audit.g4_shard_projection.v1', source_commit=PIN,
    shards_introduced_commit='a7711b506f8bc7fb691d82a392a060714fd18443',
    matrix_sha256=hashlib.sha256(matrix_bytes).hexdigest(), s9_declarations_sha256=S9_SHA,
    historical_source_commit='b319efc8477fec234afc0b31e86f8a43e3023641',
    historical_archive_sha256=ARCHIVE_SHA, sanitizer_families=families,
    s9_differentials_not_selected=sorted(diff_names),
    limitations=['projection sur inventaires B historiques, pas inventaire courant CTest',
                 'S9 expansion statique des declarations CMake inchangees depuis c977',
                 'selection preparee : aucun nouveau succes G4 ni delai valide'],
    native_runs_by_auditor=0, cloud_calls_by_auditor=0, qualification_claimed=False)
serialized = json.dumps(summary, ensure_ascii=False, sort_keys=True, indent=2) + '\n'
if '--capture' in sys.argv:
    (HERE / 'summary.json').write_text(serialized)
else:
    need((HERE / 'summary.json').read_text() == serialized, 'projection differente de la capture')
print(json.dumps(dict(source_commit=PIN, restored_each_sanitizer=82,
    union_disjoint=True, old_missing_covered=True, s9_selected_each_sanitizer=24,
    s9_differentials_excluded=4, qualification_claimed=False), sort_keys=True))
