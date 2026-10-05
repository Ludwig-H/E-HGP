#!/usr/bin/env python3
"""Projection de selecteurs sur preuves B historiques ; aucune execution native/cloud."""
from collections import Counter
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
PIN = 'c97776ea8b8730bc41c8da4b137879cb8b544c2c'
ARCHIVE_SHA = '8faaacf3c58cf1b6ebf5ff96e2d454de5b4c684f881fb28ce7015a6bdafae3c7'


def need(condition, detail):
    if not condition:
        raise RuntimeError(detail)


def source(path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', PIN + ':' + path])


def selected(test, arguments):
    name, labels = test['name'], test['labels']
    if '-R' in arguments and not re.search(arguments[arguments.index('-R') + 1], name):
        return False
    if '-E' in arguments and re.search(arguments[arguments.index('-E') + 1], name):
        return False
    if '-L' in arguments and not any(re.search(arguments[arguments.index('-L') + 1], label)
                                      for label in labels):
        return False
    if '-LE' in arguments and any(re.search(arguments[arguments.index('-LE') + 1], label)
                                  for label in labels):
        return False
    return True


def module_counts(names):
    return dict(sorted(Counter(name.split('_')[1] for name in names).items()))


paths = ['morsehgp3D_v11/tools/g4_matrix.json', 'morsehgp3D_v11/tests/points/tests.cmake',
         'morsehgp3D_v11/tests/cli/tests.cmake', 'morsehgp3D_v11/cmake/gates.cmake']
sources = {path: source(path) for path in paths}
matrix = json.loads(sources[paths[0]])
configs = {item['name']: item for item in matrix['configurations']}
points = sources[paths[1]].decode()
cli = sources[paths[2]].decode()
helper = sources[paths[3]].decode()
need('GROUPS ancestors determinism sort_refusal refusals budget LABELS fast' in points,
     'groupes points modifies')
need('if(NOT "long" IN_LIST gate_labels)' in helper, 'regle des jumelles -O modifiee')
need('mhgp11_python_gate(mhgp11_cli_points ' in cli, 'porte CLI points absente')

# Projection statique separee : expansion des declarations CMake, pas un inventaire CTest courant.
s9 = []


def gate(name, labels, twin=False):
    s9.append(dict(name=name, labels=labels))
    if twin:
        s9.append(dict(name=name + '_opt', labels=labels))


for group in ['ancestors', 'determinism', 'sort_refusal', 'refusals', 'budget', 'inventaire']:
    gate('mhgp11_points_unit_' + group, ['unit', 'fast'])
for name, labels in [('fixtures', ['fast']), ('oracle', ['oracle', 'fast'])]:
    need('mhgp11_python_gate(mhgp11_points_' + name + ' ' in points, 'porte points absente : ' + name)
    gate('mhgp11_points_' + name, labels, True)
need('mhgp11_python_gate(mhgp11_points_vs_python ' in points, 'differentiel synthetique absent')
gate('mhgp11_points_vs_python', ['long'])
need('mhgp11_python_gate(mhgp11_points_vs_python_lidar_${frame}_k5 ' in points,
     'differentiels lidar absents')
for frame in ['ng00', 'ng01', 'ng02']:
    gate('mhgp11_points_vs_python_lidar_' + frame + '_k5', ['lidar', 'long'])
need('mhgp11_python_gate(mhgp11_points_scale${n} ' in points, 'portes echelle absentes')
for size in [8000, 16000, 32000]:
    gate('mhgp11_points_scale' + str(size), ['scale' + str(size)], True)
need('mhgp11_python_gate(mhgp11_points_lidar_${frame}_k5 ' in points, 'portes CLI lidar absentes')
for frame in ['ng00', 'ng01', 'ng02']:
    gate('mhgp11_points_lidar_' + frame + '_k5', ['lidar'], True)
gate('mhgp11_cli_points', ['fast'], True)
need(len(s9) == 28, 'projection S9 attendue : 28 portes')

need(hashlib.sha256(ARCHIVE.read_bytes()).hexdigest() == ARCHIVE_SHA, 'archive B differente')
historical = []
with tarfile.open(ARCHIVE, 'r:gz') as archive:
    for config in ['gcc_asan_ubsan', 'gcc_tsan']:
        prefix = 'results/cmd/000_matrice/files/matrix/' + config + '/'
        tests_bytes = archive.extractfile(prefix + 'tests.json').read()
        result_bytes = archive.extractfile(prefix + 'result.json').read()
        tests, result = json.loads(tests_bytes), json.loads(result_bytes)
        removed = sorted(test['name'] for test in tests if not selected(test, configs[config]['ctest_args']))
        missing = sorted(item['test'] for item in result['not_run'])
        missing_removed = sorted(set(missing) & set(removed))
        missing_retained = sorted(set(missing) - set(removed))
        need(len(tests) == 824 and len(removed) == 82, 'inventaire historique/retraits inattendus')
        need(not missing_retained, 'une ancienne porte manquante reste selectionnee')
        historical.append(dict(configuration=config, archive_member_prefix=prefix,
            tests_sha256=hashlib.sha256(tests_bytes).hexdigest(),
            result_sha256=hashlib.sha256(result_bytes).hexdigest(), historical_inventory_count=len(tests),
            historical_retained_count=len(tests)-len(removed), historical_removed_count=len(removed),
            removed_by_module=module_counts(removed), historical_removed=removed,
            historical_not_run_count=len(missing), historical_not_run_removed=missing_removed,
            historical_not_run_retained=missing_retained))

projection = []
for config in ['gcc_asan_ubsan', 'gcc_tsan', 'gcc_release', 'bits21', 'bits24', 'poison', 'release_long']:
    kept = sorted(test['name'] for test in s9 if selected(test, configs[config]['ctest_args']))
    excluded = sorted(test['name'] for test in s9 if not selected(test, configs[config]['ctest_args']))
    projection.append(dict(configuration=config, ctest_args=configs[config]['ctest_args'],
                           selected_count=len(kept), excluded_count=len(excluded),
                           selected=kept, excluded=excluded))

summary = dict(schema='ehgp.v11.audit.g4_scope_projection.v1', source_commit=PIN,
    source_sha256={path: hashlib.sha256(value).hexdigest() for path, value in sources.items()},
    historical_source_commit='b319efc8477fec234afc0b31e86f8a43e3023641',
    historical_archive_sha256=ARCHIVE_SHA, historical_sanitizer_filter_projection=historical,
    s9_static_gate_projection=projection, s9_projected_gate_count=len(s9),
    runtime_current_inventory_claimed=False, new_g4_qualification_claimed=False,
    native_runs_by_auditor=0, cloud_calls_by_auditor=0)
serialized = json.dumps(summary, ensure_ascii=False, sort_keys=True, indent=2) + '\n'
if '--capture' in sys.argv:
    (HERE / 'summary.json').write_text(serialized)
else:
    need((HERE / 'summary.json').read_text() == serialized, 'projection differente de la capture')
print(json.dumps(dict(source_commit=PIN, historical_removed_each_sanitizer=82,
    historical_retained_each_sanitizer=742, old_missing_retained_each_sanitizer=0,
    s9_projected_gates=28, s9_sanitizer_selected=12, s9_release_nonlong_selected=24,
    s9_release_long_selected=0, native_or_cloud_runs=0), sort_keys=True))
