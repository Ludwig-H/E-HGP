#!/usr/bin/env python3
"""Cloture partielle repo5 : u24 complet, u32 construction et CTest seulement."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy

HERE = Path(__file__).resolve().parent
CASES = {'u8000_k5', 'u16000_k5', 'u32000_k5', 'ng00_k5', 'ng01_k5', 'ng02_k5',
         'ng00_k10', 'ng01_k10', 'ng02_k10'}


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
    codes_text = (root / 'final5/codes.txt').read_text()
    profiles = {}
    for bits in (24, 32):
        cache = (root / f'build{bits}e/CMakeCache.txt').read_text()
        for key, value in [('CMAKE_BUILD_TYPE:STRING', 'Release'), ('MHGP12_COORD_BITS:STRING', str(bits)),
                           ('MHGP12_ENABLE_CUDA:BOOL', 'OFF'), ('MHGP12_MODULES:STRING', ''),
                           ('CMAKE_HOME_DIRECTORY:INTERNAL', str(source))]:
            require(key + '=' + value + '\n' in cache, 'configuration : ' + key)
        keys = {f'build{bits}', f'ctest{bits}'}
        if bits == 24:
            keys |= {*(f'chaine_{bits}_{p}' for p in 'abc'),
                     *(f'm0_semantique_{bits}_{m}' for m in ('graines', 'v12'))}
        else:
            require(not re.search(r'^(?:chaine_32_|m0_semantique_32_)', codes_text, re.M),
                    'nouvelle cloture u32 : reviser la portee')
        codes = [(k, int(v), t) for k, v, t in re.findall(r'^(\w+)=(-?\d+) (\d\d:\d\d:\d\d)$',
                 codes_text, re.M) if k in keys]
        require(len(codes) == len(keys) and {k for k, _, _ in codes} == keys and
                all(v == 0 for _, v, _ in codes), 'codes manquants, repetes ou en echec')
        log = (root / f'final5/ctest{bits}.log').read_text()
        passed = len(re.findall(r'^\s*\d+/681 Test\s+#\d+:.*\bPassed\b', log, re.M))
        skipped = re.findall(r'^\s*\d+/681 Test\s+#\d+:\s*(\S+) .*Skipped', log, re.M)
        require((passed, len(skipped)) == (680, 1) and '0 tests failed out of 681' in log,
                'CTest incomplet')
        chains = []
        for part in ('abc' if bits == 24 else ''):
            log = (root / f'final5/chaine_{bits}_{part}.log').read_text()
            cases = re.findall(r'^(\w+_k\d+) chaine conforme$', log, re.M)
            require(len(cases) == 3 and f'mes_m0_chaine_ok profil={bits} cas=3' in log,
                    'lot chaine incomplet')
            chains += cases
        require(bits == 32 or (len(chains) == 9 and set(chains) == CASES), 'cohorte chaine')
        modes = []
        for mode in (('graines', 'v12') if bits == 24 else ()):
            log = (root / f'final5/m0_{bits}_{mode}.log').read_text()
            require(re.findall(r'^(\w+_k\d+) (graines|v12) conforme$', log, re.M) == [('ng00_k5', mode)]
                    and f'mes_m0_ok profil={bits} cas=1 modes=1 juge_emst=non' in log,
                    'MES-M0 incomplet')
            modes.append(mode)
        profiles[str(bits)] = dict(codes={k: dict(code=v, utc=t) for k, v, t in codes},
                                   ctest_passed=passed, ctest_skipped=skipped,
                                   chain_cases=sorted(chains), m0_case='ng00_k5' if modes else None, m0_modes=modes,
                                   emst_judge_in_m0=False, cuda_enabled=False)
    require(capture['stop_report_excerpt'] in (root / 'RAPPORT.md').read_text(), 'declaration arret modifiee')
    for part in 'abc':
        require((root / f'final5/chaine_32_{part}.log').read_bytes() == b'', 'sortie chaine u32 nouvelle')
    for mode in ('graines', 'v12'):
        require(not (root / f'final5/m0_32_{mode}.log').exists(), 'MES-M0 u32 nouveau')
    require(list(tree(source)) == before, 'sources modifiees pendant lecture')
    return dict(profiles=profiles, native_replayed=False, payloads_read=False, gcp_used=False,
                input_byte_identity_rehashed=False, new_quantization_demonstrated=False,
                combined_gc_tmvr_qualified=False, repo5_stopped_utc='01:22:17',
                stop_evidence='declaration RAPPORT + traces closes/incompletes, signal non rejoue')


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
