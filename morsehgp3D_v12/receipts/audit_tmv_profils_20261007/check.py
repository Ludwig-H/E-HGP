#!/usr/bin/env python3
"""Relecture des traces de profils 24/32 ; aucun moteur ni payload lu."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
CASES = {'u8000_k5', 'u16000_k5', 'u32000_k5', 'ng00_k5', 'ng01_k5', 'ng02_k5',
         'ng00_k10', 'ng01_k10', 'ng02_k10'}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(root, source_tree):
    source = root / 'repo3/morsehgp3D_v12'
    before, nfiles = source_tree(source)
    codes = dict(re.findall(r'^(\w+)=(-?\d+) ', (root / 'final/codes.txt').read_text(), re.M))
    profiles = {}
    for bits in (24, 32):
        cache = (root / f'build{bits}c/CMakeCache.txt').read_text()
        for key, value in [('CMAKE_BUILD_TYPE:STRING', 'Release'), ('MHGP12_COORD_BITS:STRING', str(bits)),
                           ('MHGP12_MODULES:STRING', ''), ('CMAKE_HOME_DIRECTORY:INTERNAL', str(source))]:
            require(key + '=' + value + '\n' in cache, f'cache profil {bits} : {key}')
        log = (root / f'final/ctest{bits}.log').read_text()
        passed = len(re.findall(r'^\s*\d+/652 Test\s+#\d+:.*\bPassed\b', log, re.M))
        skipped = len(re.findall(r'^\s*\d+/652 Test\s+#\d+:.*Skipped', log, re.M))
        require((passed, skipped) == (651, 1) and '0 tests failed out of 652' in log, f'CTest {bits}')
        cases = []
        for part in 'abc':
            log = (root / f'final/m0_{bits}_{part}.log').read_text()
            rows = re.findall(r'^(\w+_k\d+) graines conforme$', log, re.M)
            require(len(rows) == 3 and f'mes_m0_ok profil={bits} cas=3 modes=1 juge_emst=non' in log, 'lot graines')
            cases += rows
        require(len(cases) == 9 and set(cases) == CASES, 'cohorte graines')
        log = (root / f'final/m0_{bits}_v12.log').read_text()
        require(re.findall(r'^(\w+_k\d+) v12 conforme$', log, re.M) == ['ng00_k5'] and
                f'mes_m0_ok profil={bits} cas=1 modes=1 juge_emst=non' in log, 'cohorte cibles v12')
        keys = [f'build{bits}', f'ctest{bits}'] + [f'm0_semantique_{bits}_{part}' for part in ('a', 'b', 'c', 'v12')]
        require(all(codes.get(key) == '0' for key in keys), 'codes de campagne')
        profiles[str(bits)] = dict(ctest_passed=passed, ctest_skipped=skipped, seeds_cases=sorted(cases),
                                  v12_cases=['ng00_k5'], command_codes={key: 0 for key in keys})
    after, _ = source_tree(source)
    require(before == after, 'sources modifiees pendant lecture')
    return dict(profiles=profiles, source_tree_sha256=before, source_files=nfiles, native_replayed=False,
                payloads_read=False, gcp_used=False)


def packaging(root, repository, base):
    """Extraction des seuls fichiers du patch depuis un commit ; application temporaire."""
    patch = (root / 'patch_tour_TMV.diff').resolve()
    rows = subprocess.check_output(['git', 'apply', '--numstat', str(patch)], text=True).splitlines()
    paths = [row.split('\t', 2)[2] for row in rows]
    require(len(set(paths)) == len(paths), 'chemins dupliques')
    for rel in paths:
        require(rel.startswith('morsehgp3D_v12/') and '..' not in Path(rel).parts, 'chemin hors produit')
    tracked = set(subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', base],
                                         cwd=repository, text=True).splitlines())
    with tempfile.TemporaryDirectory(prefix='audit-tmvr-packaging-') as directory:
        target = Path(directory)
        for rel in paths:
            if rel in tracked:
                data = subprocess.check_output(['git', 'show', f'{base}:{rel}'], cwd=repository)
                dest = target / rel
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(data)
        subprocess.run(['git', 'apply', '--check', str(patch)], cwd=target, check=True)
        subprocess.run(['git', 'apply', str(patch)], cwd=target, check=True)
        different, digest = {}, hashlib.sha256()
        for rel in sorted(paths):
            actual, prototype = sha(target / rel), sha(root / 'repo3' / rel)
            digest.update((rel + '\0' + actual + '\n').encode())
            if actual != prototype:
                different[rel] = dict(result_sha256=actual, prototype_sha256=prototype)
        return dict(baseline_commit=base, patch_sha256=sha(patch), file_count=len(paths),
                    equal_count=len(paths) - len(different), different=different,
                    applied_files_sha256=digest.hexdigest(), apply_check=True, apply=True,
                    g_judge_not_in_patch='morsehgp3D_v12/tests/tower/g_determinism.py' not in paths,
                    registry_in_patch='morsehgp3D_v12/src/tower/registry_branches.cpp' in paths)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prototype', required=True, type=Path)
    parser.add_argument('--git-repository', type=Path, help='rejouer aussi le conditionnement en dossier temporaire')
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    prior = HERE.parent / 'audit_tmv_traces_20261007/check.py'
    require(sha(prior) == capture['prior_reader_sha256'], 'lecteur anterieur modifie')
    source_tree = runpy.run_path(str(prior))['tree']
    for rel, expected in capture['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier modifie : ' + rel)
    result = inspect(args.prototype, source_tree)
    require(result == capture['result'], 'resultat different de la capture')
    require(result['source_tree_sha256'] == capture['baseline_tree_sha256'], 'arbre different de la capture u21')
    for rel, expected in capture['critical_sources_sha256_before'].items():
        require(sha(args.prototype / 'repo3/morsehgp3D_v12' / rel) == expected, 'source critique differente')
    if args.git_repository:
        expected = capture['packaging']['result']
        require(packaging(args.prototype, args.git_repository, expected['baseline_commit']) == expected,
                'conditionnement different de la capture')
    for rel, expected in capture['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier modifie pendant lecture : ' + rel)
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
