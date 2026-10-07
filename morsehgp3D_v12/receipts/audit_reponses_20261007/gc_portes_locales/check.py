#!/usr/bin/env python3
"""Relecture seulement : aucun test natif, build ou appel CTest. Python nu, sans assert."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
COPIED = ('CMakeLists.txt', 'cmake', 'src', 'cli', 'bench', 'tests', 'tools', 'reference', 'docs')


def need(ok, detail):
    if not ok:
        raise ValueError(detail)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def archived_sources(repository, pin):
    """Git archive lit un commit immuable ; ne touche pas le prototype actif."""
    data = subprocess.check_output(['git', 'archive', pin, 'morsehgp3D_v12'], cwd=repository)
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:
        return {m.name.removeprefix('morsehgp3D_v12/'): archive.extractfile(m).read()
                for m in archive if m.isfile()}


def tree(files):
    """Recette exacte du lanceur sur les fichiers archives, sans l'executer."""
    h = hashlib.sha256()
    count = 0
    for name in COPIED:
        paths = [p for p in files if (p == name or p.startswith(name + '/'))
                 and '/__pycache__/' not in p and not p.endswith('.pyc')]
        for path in sorted(paths):
            h.update(path.encode() + b'\0')
            h.update(hashlib.sha256(files[path]).hexdigest().encode() + b'\n')
            count += 1
    return count, h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scratch', required=True, type=Path)
    args = parser.parse_args()
    root = args.scratch.resolve()
    capture = json.loads((HERE / 'capture.json').read_text())
    def pins():
        for rel, expected in capture['artifact_hashes'].items():
            need(sha(root / rel) == expected, 'hash changed: ' + rel)
    pins()
    source = root / 'repo2/morsehgp3D_v12'
    files = archived_sources(root / 'git2', capture['immutable_git_pin'])
    report = json.loads((root / 'runs/mutants_tower_18.json').read_text())
    count, digest = tree(files)
    need(digest == report['sources_sha256'] == capture['source_tree_sha256'], 'tree hash')
    manifest_bytes = files['tests/mutants/tower.json']
    need(hashlib.sha256(manifest_bytes).hexdigest() == report['manifeste_sha256'], 'manifest hash')
    manifest = json.loads(manifest_bytes)
    need(report['code'] == 0 and report['temoin'] == 'vert' and report['plancher'] == 18, 'mutant result')
    entries = report['mutants']
    need(len(entries) == 18 and len({x['id'] for x in entries}) == 18, '18 unique mutants')
    need({x['id'] for x in entries} == {x['id'] for x in manifest['mutants']}, 'mutant cohort')
    need(all(x['detail'] == 'code' and x['verdict'] == 'TUE' for x in entries), 'mutants killed by code')
    mutant = next(x for x in manifest['mutants'] if x['id'] == 'empreinte_de_file_sans_interieur')
    result = next(x for x in entries if x['id'] == mutant['id'])
    need(result['juge'] == mutant['porte'] == 'mhgp12_tower_index_weak_key_resolution', 'mutant gate')
    need(files[mutant['fichier']].decode().count(mutant['cherche']) == 1, 'unique mutation site')
    need(mutant['remplace'] == '        (void)inner;', 'mutation scope')
    profiles = []
    for bits in (24, 32):
        build = root / ('build_p' + str(bits))
        journal = (root / 'runs/profils.log').read_text()
        need(all('profil ' + str(bits) + ' ' + phase + '=0' in journal
                 for phase in ('conf', 'build', 'ctest')), 'profile external return codes')
        need('warning:' not in (root / ('build_p' + str(bits) + '.build.log')).read_text(), 'build warning')
        cache = (build / 'CMakeCache.txt').read_text()
        need('MHGP12_COORD_BITS:STRING=' + str(bits) in cache, 'profile')
        need('MHGP12_MODULES:STRING=catalogue;tower' in cache, 'modules')
        need('CMAKE_HOME_DIRECTORY:INTERNAL=' + str(source) in cache, 'source directory')
        flags = (build / 'CMakeFiles/mhgp12.dir/flags.make').read_text()
        need('-DMHGP12_COORD_BITS=' + str(bits) in flags, 'compiled profile')
        log = (build / 'Testing/Temporary/LastTest.log').read_text()
        blocks = re.split(r'^\d+/\d+ Testing: ', log, flags=re.M)[1:]
        names = [b.splitlines()[0] for b in blocks]
        need(len(names) == len(set(names)) == 71, '71 unique tests')
        need(all('Test Passed.' in b and 'Test Failed.' not in b and 'Skipped' not in b for b in blocks), 'passes')
        configured = (build / 'CTestTestfile.cmake').read_text()
        configured_names = set(re.findall(r'^add_test\(\[=\[([^\n]+?)\]=\]', configured, re.M))
        excluded = re.findall(r'^set_tests_properties\(\[=\[([^\n]+?)\]=\].*LABELS "[^"]*long[^"]*"',
                              configured, re.M)
        need(len(configured_names) == 78 and set(names) == configured_names - set(excluded), 'long exclusion')
        for size, prefix in ((8000, 'a40f1b2ef8547269'), (16000, 'cf7c7745fcb4ae6e'), (32000, 'd1f08fd0dbdf48eb')):
            block = blocks[names.index('mhgp12_tower_scale' + str(size))]
            need('g_determinism_ok cas=synth_u' + str(size) + '_k5 fils=1,8 empreinte=' + prefix in block, 'scale')
            need('--uniform=' + str(size) + ',20261007,18' in block, 'input uses 18 bits')
        obj = build / 'CMakeFiles/mhgp12.dir/src/tower/passes.cpp.o'
        dep = Path(str(obj) + '.d')
        need(str(source / 'src/tower/passes.cpp') in dep.read_text(), 'passes dependency')
        need(obj.stat().st_mtime_ns > capture['passes_source_mtime_ns'], 'object newer than source capture')
        profiles.append({'bits': bits, 'configured': 78, 'passed': 71, 'skipped_selected': 0,
                         'excluded_long': excluded,
                         'end': next(x for x in log.splitlines() if x.startswith('End testing:'))})
    tsan = root / 'build_tsan2'
    cache = (tsan / 'CMakeCache.txt').read_text()
    need('MHGP12_TSAN:BOOL=ON' in cache and 'MHGP12_COORD_BITS:STRING=21' in cache, 'TSan profile')
    need('CMAKE_HOME_DIRECTORY:INTERNAL=' + str(source) in cache, 'TSan source')
    obj = tsan / 'CMakeFiles/mhgp12.dir/src/tower/passes.cpp.o'
    need(str(source / 'src/tower/passes.cpp') in Path(str(obj) + '.d').read_text(), 'TSan passes dependency')
    need(obj.stat().st_mtime_ns > capture['passes_source_mtime_ns'], 'TSan object newer than source capture')
    for target in ('mhgp12', 'mhgp12_tower_index', 'mhgp12_tower_unit', 'mhgp12_tower_probe'):
        folder = tsan / ('CMakeFiles/' + target + '.dir')
        need('-fsanitize=thread' in (folder / 'flags.make').read_text(), 'TSan compile flags')
        if target != 'mhgp12':
            need('-fsanitize=thread' in (folder / 'link.txt').read_text(), 'TSan link flags')
    unit_counts = {}
    for group, controls in (('radix_stable', 28), ('index_reference', 63), ('weak_key_resolution', 50),
                            ('support_table', 4), ('duplicate_population', 5), ('det', 62)):
        rel = 'tsan_unit_det.log' if group == 'det' else 'tsan_index_' + group + '.log'
        text = (root / 'runs' / rel).read_text()
        need('echecs=0' in text and text.rstrip().endswith('mhgp12_test_ok tests=1 controles=' + str(controls)),
             'TSan native terminal')
        need('ThreadSanitizer' not in text, 'TSan diagnostic in preserved log')
        unit_counts[group] = controls
    probes = []
    for name, threads, sites, prefix in (('u8000', 8, 8000, 'a40f1b2ef8547269'),
                                        ('ng00', 3, 39885, 'e5a81154fb1b15f1')):
        rows = [json.loads(line) for line in (root / 'runs' / ('tsan_probe_' + name + '.log')).read_text().splitlines()]
        need(rows[0]['phase'] == 'tour_g' and rows[0]['status'] == 'ok', 'TSan stage')
        need(rows[0]['threads'] == threads and rows[0]['sites'] == sites, 'TSan metadata')
        need([r['k'] for r in rows if r['phase'] == 'ordre'] == [1, 2, 3, 4, 5], 'TSan orders')
        hashes = [r['resolution_sha256'] for r in rows if r['phase'] == 'digest']
        need(len(hashes) == 1 and len(hashes[0]) == 64 and hashes[0].startswith(prefix), 'TSan digest')
        need(rows[-1] == {'phase': 'exit', 'status': 'ok', 'reason': 'none', 'order': 0}, 'TSan exit')
        probes.append({'case': name, 'threads': threads, 'sites': sites, 'digest': hashes[0]})
    pins()
    need(tree(archived_sources(root / 'git2', capture['immutable_git_pin'])) == (count, digest), 'archive changed')
    print(json.dumps({'status': 'ok', 'artifact_hashes_verified_before_after': len(capture['artifact_hashes']),
        'source_tree_files': count, 'source_tree_sha256': digest,
        'immutable_git_pin': capture['immutable_git_pin'],
        'passes_sha256': hashlib.sha256(files['src/tower/passes.cpp']).hexdigest(),
        'mutants': {'killed_by_code': 18, 'control': 'vert', 'target_mutant': result,
                    'precise_native_failed_check_archived': False}, 'profiles': profiles,
        'tsan': {'unit_controls': unit_counts, 'probes': probes, 'compiled_and_linked_with_tsan': True,
                 'external_exit_codes_archived': False, 'invocation_commands_archived': False,
                 'setarch_independently_attested': False, 'stderr_capture_independently_attested': False},
        'native_executed_by_auditor': False}, indent=2, sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, TypeError, StopIteration) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
