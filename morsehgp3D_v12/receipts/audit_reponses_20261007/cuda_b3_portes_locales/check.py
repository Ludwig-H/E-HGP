#!/usr/bin/env python3
"""Relit les traces B3 et leurs sources ; ne lance aucun binaire de la v12."""
import collections
import hashlib
import json
import os
from pathlib import Path
import re
import sys

COPIED = ('CMakeLists.txt', 'cmake', 'src', 'cli', 'bench', 'tests', 'tools', 'reference', 'docs')


def need(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree_digest(source):
    out, count = hashlib.sha256(), 0
    for name in COPIED:
        origin = source / name
        paths = [origin] if origin.is_file() else []
        for folder, folders, files in os.walk(origin):
            folders[:] = sorted(x for x in folders if x != '__pycache__')
            paths += [Path(folder) / x for x in sorted(files) if not x.endswith('.pyc')]
        for path in sorted(paths):
            out.update(str(path.relative_to(source)).encode() + b'\0')
            out.update(sha(path).encode() + b'\n')
            count += 1
    return out.hexdigest(), count


def campaign(base, folder, expected, source):
    build = base / folder
    cache = (build / 'CMakeCache.txt').read_text()
    for line in ('MHGP12_COORD_BITS:STRING=21', 'MHGP12_MODULES:STRING=catalogue',
                 'MHGP12_ENABLE_CUDA:BOOL=OFF',
                 'CMAKE_HOME_DIRECTORY:INTERNAL=' + str(source)):
        need(line in cache.splitlines(), 'configuration differente : ' + line.split('=')[0])
    sanitizer = 'address,undefined' if folder == 'asanB3' else 'thread'
    for target in ('mhgp12', 'mhgp12_catalogue_device_unit', 'mhgp12_catalogue_unit', 'mhgp12_catalogue_probe'):
        flags = (build / ('CMakeFiles/' + target + '.dir/flags.make')).read_text()
        need('-fsanitize=' + sanitizer in flags, 'instrumentation absente : ' + target)
        if folder == 'asanB3':
            need('-fno-sanitize-recover=all' in flags, 'recuperation UBSan possible')
    for target in ('mhgp12_catalogue_device_unit', 'mhgp12_catalogue_unit', 'mhgp12_catalogue_probe'):
        link = (build / ('CMakeFiles/' + target + '.dir/link.txt')).read_text()
        need('-fsanitize=' + sanitizer in link, 'runtime non lie : ' + target)
    log = (build / 'Testing/Temporary/LastTest.log').read_text()
    starts = list(re.finditer(r'^\d+/\d+ Testing: (\S+)$', log, re.M))
    names = [m[1] for m in starts]
    need(len(names) == len(set(names)) == expected, 'effectif des traces')
    for i, match in enumerate(starts):
        section = log[match.start():starts[i + 1].start() if i + 1 < len(starts) else len(log)]
        need('Test Passed.' in section and 'run_expect_verdict conforme' in section,
             'porte non conforme : ' + match[1])
    need(not re.search(r'WARNING: (?:Address|Thread)Sanitizer|ERROR: AddressSanitizer|runtime error:|'
                       r'mhgp12_porte_sautee|run_expect_verdict sautee|Test Failed\.', log),
         'rapport sanitizer, saut ou echec')
    need('device_open : appareil indisponible (device_unavailable), voie appareil non jouee' in log,
         'branche device_open non attestee')
    props = (build / 'CTestTestfile.cmake').read_text().splitlines()
    labels = {}
    for line in props:
        match = re.match(r'set_tests_properties\(\[=\[(.*?)\]=\].*? LABELS "([^"]*)"', line)
        if match:
            labels[match[1]] = match[2].split(';')
    if folder == 'asanB3':
        need(set(names) == {name for name, tags in labels.items() if 'long' not in tags},
             'selection ASan differente de -LE long')
    else:
        expected_names = {name for name in labels if name.startswith('mhgp12_catalogue_device_unit_')}
        expected_names |= {'mhgp12_catalogue_unit_determinism', 'mhgp12_catalogue_scale8000'}
        need(set(names) == expected_names, 'selection TSan differente')
    summary_file = 'ctest_fast.log' if folder == 'asanB3' else 'ctest.log'
    summary = (build / summary_file).read_text()
    need(f'100% tests passed, 0 tests failed out of {expected}' in summary, 'resume CTest divergent')
    controls = {name: int(n) for name, n in re.findall(r'^test (\w+) controles=(\d+) echecs=0', log, re.M)}
    return dict(passed=expected, skipped_in_selected_tests=0, cuda_enabled=False,
                device_open='device_unavailable', native_control_counts=controls,
                unselected_long=sorted(name for name, tags in labels.items() if 'long' in tags),
                sanitizer=sanitizer)


def main():
    base = Path(sys.argv[1]).resolve()
    source = base / 'frozenB3/morsehgp3D_v12'
    capture = json.loads(Path(__file__).with_name('capture.json').read_text())
    pinned = capture['artifact_hashes']
    need(len(pinned) == 47, 'inventaire des artefacts different')
    def check_pins():
        for relative, expected in pinned.items():
            need(sha(base / relative) == expected, 'artefact change : ' + relative)
    check_pins()
    report = json.loads((base / 'mutB3/report.json').read_text())
    manifest = source / 'tests/mutants/catalogue.json'
    source_hash, count = tree_digest(source)
    need(source_hash == report['sources_sha256'], 'sources distinctes de la campagne mutants')
    need(sha(manifest) == report['manifeste_sha256'], 'manifeste distinct')
    declared = json.loads(manifest.read_text())['mutants']
    need({m['id'] for m in declared} == {m['id'] for m in report['mutants']}, 'mutants absents')
    need(report['code'] == 0 and report['temoin'] == 'vert' and len(report['mutants']) == 15,
         'campagne mutants non conforme')
    need(all(m['verdict'] == 'TUE' for m in report['mutants']), 'mutant non tue')
    details = dict(collections.Counter(m['detail'] for m in report['mutants']))
    need(details == {'code': 14, 'signal': 1}, 'causes de destruction differentes')
    # Tous les budgets natifs joues ici ont leur cache implicite nul.
    cache_files = ['tests/catalogue/' + f for f in ('unit.cpp', 'device_unit.cpp',
                   'device_finish_test.cpp', 'device_pipeline_test.cpp')]
    cache_files += ['bench/catalogue_probe.cpp']
    constructors = 0
    for path in cache_files:
        calls = re.findall(r'MemoryBudget\s+\w+\(([^;]+)\);', (source / path).read_text())
        need(calls and all(',' not in call for call in calls), 'budget avec cache a relire : ' + path)
        constructors += len(calls)
    need('explicit MemoryBudget(u64 limit_bytes, u64 cache_bytes = 0)' in
         (source / 'src/core/buffer.hpp').read_text(), 'defaut cache change')
    result = dict(source_tree_sha256=source_hash, source_files=count,
                  mutants=dict(code=0, witness='vert', count=15, killed_by=details),
                  asan=campaign(base, 'asanB3', 44, source),
                  tsan=campaign(base, 'tsanB3', 12, source),
                  budgets_without_cache_argument=constructors, active_block_cache=False,
                  setarch_R='declared_in_RAPPORT_only', native_executed_by_auditor=False)
    need(tree_digest(source)[0] == source_hash, 'sources changees pendant lecture')
    check_pins()
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
