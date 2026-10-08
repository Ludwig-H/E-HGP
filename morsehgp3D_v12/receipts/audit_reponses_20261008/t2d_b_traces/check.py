#!/usr/bin/env python3
"""Contrelecture de traces archivées, sans construire ni lancer de moteur."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

COPIED = ('CMakeLists.txt', 'cmake', 'src', 'cli', 'bench', 'tests', 'tools', 'reference', 'docs')


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree(root):
    h, count = hashlib.sha256(), 0
    for name in COPIED:
        origin = root / name
        paths = [origin] if origin.is_file() else []
        for folder, folders, files in os.walk(origin):
            folders[:] = sorted(x for x in folders if x != '__pycache__')
            paths += [Path(folder) / x for x in sorted(files) if not x.endswith('.pyc')]
        for path in sorted(paths):
            h.update(path.relative_to(root).as_posix().encode() + b'\0' + sha(path).encode() + b'\n')
            count += 1
    return {'files': count, 'sha256': h.hexdigest()}


def review(e):
    log = (e / 'logs/ctest_lot9020.log').read_text()
    tests = re.findall(r'^\s*\d+/699 Test\s+#(\d+): (\S+) .*?(Passed|\*\*\*Skipped)\s+([\d.]+) sec$', log, re.M)
    need(len(tests) == len({x[0] for x in tests}) == 699, 'cohorte CTest')
    skipped = [x[1] for x in tests if x[2] == '***Skipped']
    need(skipped == ['mhgp12_support_lidar_sentinel'], 'sentinelle')
    need('100% tests passed, 0 tests failed out of 699' in log, 'bilan CTest')
    need('Total Test time (real) = 618.60 sec' in log, 'durée CTest')
    state = (e / 'logs/ctest_lot9020.etat').read_text()
    need('build ok 04:52:50' in state and 'fin 0 05:03:09' in state, 'codes externes CTest')
    wanted = ['mhgp12_index_guarded_witness_census']
    wanted += ['mhgp12_tower_proposal_' + x for x in ('witnesses', 'random', 'routes', 'inventaire')]
    wanted += ['mhgp12_tower_scale' + str(n) for n in (8000, 16000, 32000)]
    passed = {x[1] for x in tests if x[2] == 'Passed'}
    need(set(wanted) <= passed, 'portes nouvelles ou échelle absentes')
    identities = json.loads((e / 'runs/identite_ful1_w3_902041f66.json').read_text())
    cases = [(f'lidar_ng0{i}', k) for k in (5, 10) for i in range(3)]
    cases += [(f'uniform_u18_n{n}', 5) for n in (8000, 16000, 32000)]
    need(identities['fils'] == '3' and identities['tous_identiques'] is True, 'régime FUL1')
    need([(r['trame'], r['k']) for r in identities['cas']] == cases, 'cohorte FUL1')
    for r in identities['cas']:
        a, b = r['base'], r['lot']
        need(type(a['code']) is int and type(b['code']) is int and a['code'] == b['code'] == 0, 'codes FUL1')
        need(r['identique'] is True and a['full_sha256'] == b['full_sha256'], 'empreintes différentes')
        need(re.fullmatch('[0-9a-f]{64}', a['full_sha256']) is not None, 'empreinte malformée')
        need(type(a['sites']) is int and a['sites'] == b['sites'] > 0, 'sites différents')
    need((e / 'logs/identite_ful1.log').read_text().endswith('fin 0 05:09:35\n'), 'fin FUL1')
    source = e / 'source'
    current_tree = tree(source)
    report = json.loads((e / 'runs/mutants_index.json').read_text())
    manifest_path = source / 'tests/mutants/index.json'
    manifest = json.loads(manifest_path.read_text())
    need(report['schema'] == 'mhgp12.mutants.v1' and report['module'] == 'index', 'format mutants')
    need(type(report['code']) is int and report['code'] == 0 and report['temoin'] == 'vert', 'issue mutants')
    need(report['sources_sha256'] == current_tree['sha256'], 'arbre des mutants différent')
    need(report['manifeste_sha256'] == sha(manifest_path), 'manifeste différent')
    rows = report['mutants']
    need(report['plancher'] == len(rows) == 22, 'plancher index')
    need([x['id'] for x in rows] == [x['id'] for x in manifest['mutants']], 'cohorte index différente')
    need(all(x['verdict'] == 'TUE' and x['detail'] == 'code' for x in rows), 'causes index')
    for row, mutation in zip(rows, manifest['mutants']):
        need(row['juge'] == mutation['porte'], 'porte index différente')
    need('index code=0 05:22:09' in (e / 'logs/mutants_locaux.log').read_text(), 'code extérieur index')
    for rel in ('b_lot/CMakeCache.txt', 'b_base/CMakeCache.txt'):
        cache = (e / rel).read_text()
        for setting in ('CMAKE_BUILD_TYPE:STRING=Release', 'MHGP12_COORD_BITS:STRING=21', 'MHGP12_ENABLE_CUDA:BOOL=OFF'):
            need(setting in cache, 'configuration différente')
    old = {p.relative_to(e / 'base8da_src').as_posix(): sha(p) for p in (e / 'base8da_src').rglob('*') if p.is_file()}
    new = {p.relative_to(e / 'base902_src').as_posix(): sha(p) for p in (e / 'base902_src').rglob('*') if p.is_file()}
    need(old == new and len(old) == 128, 'src des deux bases différents')
    return {'ctest': {'passed': 698, 'skipped': skipped, 'failed': 0, 'wall_seconds': 618.60},
            'ful1_summary_equal_pairs': 9, 'ful1_processes_code_zero': 18,
            'index_mutants_code_killed': 22, 'tree': current_tree,
            'base_src_equal_files': 128, 'num_tower': 'not_qualified_in_this_capture'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads(Path(__file__).with_name('capture.json').read_text())
    for rel, pin in cap['files'].items():
        p = args.evidence / rel
        need(p.stat().st_size == pin['bytes'] and sha(p) == pin['sha256'], 'pin différent : ' + rel)
    result = review(args.evidence)
    need(result == cap['result'], 'résultat différent')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
