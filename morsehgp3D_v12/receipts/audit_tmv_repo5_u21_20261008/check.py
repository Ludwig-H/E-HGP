#!/usr/bin/env python3
"""Jalon u21 repo5 : lecture des traces closes, aucun moteur ni payload."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy

HERE = Path(__file__).resolve().parent
CASES = {'u8000_k5', 'u16000_k5', 'u32000_k5', 'ng00_k5', 'ng01_k5', 'ng02_k5',
         'ng00_k10', 'ng01_k10', 'ng02_k10'}
KEYS = {'build21', 'ctest21', 'm0_octets_21', 'chaine_21_a', 'chaine_21_b', 'chaine_21_c', 'mutants'}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(root, capture):
    prior = HERE.parent / 'audit_tmv_traces_20261007/check.py'
    require(sha(prior) == capture['prior_reader_sha256'], 'lecteur arbre modifie')
    tree = runpy.run_path(str(prior))['tree']
    source = root / 'repo5/morsehgp3D_v12'
    before = list(tree(source))
    require(before == capture['source_tree'], 'source differente')
    cache = (root / 'build21e/CMakeCache.txt').read_text()
    for key, value in [('CMAKE_BUILD_TYPE:STRING', 'Release'), ('MHGP12_COORD_BITS:STRING', '21'),
                       ('MHGP12_ENABLE_CUDA:BOOL', 'OFF'), ('MHGP12_MODULES:STRING', ''),
                       ('CMAKE_HOME_DIRECTORY:INTERNAL', str(source))]:
        require(key + '=' + value + '\n' in cache, 'configuration : ' + key)
    codes = [(k, int(v), t) for k, v, t in re.findall(r'^(\w+)=(-?\d+) (\d\d:\d\d:\d\d)$',
             (root / 'final5/codes.txt').read_text(), re.M) if k in KEYS]
    require(len(codes) == len(KEYS) and {k for k, _, _ in codes} == KEYS and
            all(v == 0 for _, v, _ in codes), 'codes manquants, repetes ou en echec')
    log = (root / 'final5/ctest21.log').read_text()
    passed = len(re.findall(r'^\s*\d+/681 Test\s+#\d+:.*\bPassed\b', log, re.M))
    skipped = len(re.findall(r'^\s*\d+/681 Test\s+#\d+:.*Skipped', log, re.M))
    require((passed, skipped) == (680, 1) and '0 tests failed out of 681' in log, 'CTest incomplet')
    log = (root / 'final5/m0_21.log').read_text()
    rows = re.findall(r'^(\w+_k\d+) (graines|v12) conforme$', log, re.M)
    require(len(rows) == 18 and set(rows) == {(c, m) for c in CASES for m in ('graines', 'v12')} and
            'mes_m0_ok profil=21 cas=9 modes=2 juge_emst=oui' in log, 'MES-M0 incomplet')
    chains = []
    for part in 'abc':
        log = (root / f'final5/chaine_21_{part}.log').read_text()
        cases = re.findall(r'^(\w+_k\d+) chaine conforme$', log, re.M)
        require(len(cases) == 3 and 'mes_m0_chaine_ok profil=21 cas=3' in log, 'lot chaine incomplet')
        chains += cases
    require(len(chains) == 9 and set(chains) == CASES, 'cohorte chaine')
    log = (root / 'build21e/Testing/Temporary/LastTest.log').read_text()
    require(log.count('admission etages=160 egalites=40 lignes=25383') == 1 and
            'test admission controles=685 echecs=0 plancher=170' in log, 'porte admission')
    report = json.loads((root / 'final5/mutants_tower.json').read_text())
    manifest = source / 'tests/mutants/tower.json'
    declared = json.loads(manifest.read_text())
    ids = {m['id'] for m in declared['mutants']}
    killed = report['mutants']
    require(report['sources_sha256'] == before[0] and report['manifeste_sha256'] == sha(manifest),
            'mutants sans leurs sources')
    require(report['code'] == 0 and report['temoin'] == 'vert' and report['plancher'] == 16 and
            len(ids) == len(killed) == 16 and {m['id'] for m in killed} == ids and
            all(m['verdict'] == 'TUE' and m['detail'] == 'code' for m in killed), 'mutants incomplets')
    require(list(tree(source)) == before, 'sources modifiees pendant lecture')
    return dict(codes={k: dict(code=v, utc=t) for k, v, t in codes}, ctest_passed=passed, ctest_skipped=skipped,
                m0_cases=9, m0_target_modes=2, chain_cases=sorted(chains),
                admission=dict(stages=160, equal_peaks=40, retained_rows=25383, controls=685, failures=0),
                mutants_killed_by_code=sorted(ids), cuda_enabled=False, native_replayed=False,
                payloads_read=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prototype', type=Path, required=True)
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    for rel, expected in capture['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier modifie : ' + rel)
    result = inspect(args.prototype, capture)
    require(result == capture['result'], 'resultat different')
    for rel, expected in capture['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier modifie pendant lecture : ' + rel)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
