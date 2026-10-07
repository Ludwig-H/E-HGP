#!/usr/bin/env python3
"""Contrelecture de traces TMVR, sans lancer outil natif ni lire les donnees/vidages."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
CASES = [f'{frame}_k{k}' for frame, ks in [('u8000', [5]), ('u16000', [5]), ('u32000', [5]),
          ('ng00', [5, 10]), ('ng01', [5, 10]), ('ng02', [5, 10])] for k in ks]
CODE_KEYS = ('ctest21', 'm0_octets_21', 'mutants', 'chaine_21_a', 'chaine_21_b', 'chaine_21_c')


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree(source):
    parsed = ast.parse((source / 'tests/mutants/run_mutants.py').read_text())
    copied = next(ast.literal_eval(node.value) for node in parsed.body if isinstance(node, ast.Assign)
                  and any(isinstance(t, ast.Name) and t.id == 'COPIED' for t in node.targets))
    result, count = hashlib.sha256(), 0
    for name in copied:
        origin = source / name
        paths = [origin] if origin.is_file() else []
        for folder, folders, files in os.walk(origin):
            folders[:] = sorted(e for e in folders if e != '__pycache__')
            paths += [Path(folder) / e for e in sorted(files) if not e.endswith('.pyc')]
        for path in sorted(paths):
            result.update(str(path.relative_to(source)).encode() + b'\0')
            result.update(sha(path).encode() + b'\n')
            count += 1
    return result.hexdigest(), count


def inspect(root):
    source = root / 'repo3/morsehgp3D_v12'
    before, count = tree(source)
    report = json.loads((root / 'final/mutants_tower.json').read_text())
    require(report['sources_sha256'] == before, 'arbre different de la campagne mutante')
    require(report['manifeste_sha256'] == sha(source / 'tests/mutants/tower.json'), 'manifeste different')
    manifest = json.loads((source / 'tests/mutants/tower.json').read_text())
    require([m['id'] for m in report['mutants']] == [m['id'] for m in manifest['mutants']], 'identifiants mutants')
    require(report['code'] == 0 and report['temoin'] == 'vert' and report['plancher'] == 15, 'campagne incomplete')
    require(len(report['mutants']) == 15 and all(m['verdict'] == 'TUE' and m['detail'] == 'code'
                                               for m in report['mutants']), 'issues mutants')
    log = (root / 'final/ctest21.log').read_text()
    passed = len(re.findall(r'^\s*\d+/652 Test\s+#\d+:.*\bPassed\b', log, re.M))
    skipped = len(re.findall(r'^\s*\d+/652 Test\s+#\d+:.*Skipped', log, re.M))
    require((passed, skipped) == (651, 1) and '0 tests failed out of 652' in log, 'bilan CTest')
    for gate in ('mhgp12_tower_forest_branches', 'mhgp12_tower_forest_oracle_gate',
                 'mhgp12_tower_forest_oracle_gate_opt', 'mhgp12_tower_forme_niveau'):
        require(re.search(r'Test\s+#\d+: ' + gate + r'\s+\.+\s+Passed', log) is not None, gate)
    cache = (root / 'build21c/CMakeCache.txt').read_text()
    for key, value in [('CMAKE_BUILD_TYPE:STRING', 'Release'), ('MHGP12_COORD_BITS:STRING', '21'),
                       ('MHGP12_MODULES:STRING', ''), ('CMAKE_HOME_DIRECTORY:INTERNAL', str(source))]:
        require(key + '=' + value + '\n' in cache, 'cache ' + key)
    m0 = (root / 'final/m0_21.log').read_text()
    observed = re.findall(r'^(\w+_k\d+) (graines|v12) conforme$', m0, re.M)
    require(len(observed) == 18 and set(observed) == {(c, mode) for c in CASES for mode in ('graines', 'v12')},
            'cohorte MES-M0')
    require('mes_m0_ok profil=21 cas=9 modes=2 juge_emst=oui' in m0, 'fin MES-M0')
    observed = []
    for part in 'abc':
        text = (root / ('final/chaine_21_' + part + '.log')).read_text()
        rows = re.findall(r'^(\w+_k\d+) chaine conforme$', text, re.M)
        require(len(rows) == 3 and 'mes_m0_chaine_ok profil=21 cas=3' in text, 'lot chaine')
        observed += rows
    require(len(observed) == 9 and set(observed) == set(CASES), 'cohorte chaine incomplete ou dupliquee')
    codes = dict(re.findall(r'^(\w+)=(-?\d+) ', (root / 'final/codes.txt').read_text(), re.M))
    require(all(codes.get(key) == '0' for key in CODE_KEYS), 'codes des six commandes')
    after, _ = tree(source)
    require(before == after, 'arbre change pendant la lecture')
    return dict(tree_sha256=before, source_files=count, tree_stable=True, ctest_selected=652, ctest_passed=passed,
                ctest_skipped=skipped, mutant_killed_by_code=15, m0_cases=9, m0_modes=2, chain_cases=sorted(observed),
                command_codes={key: 0 for key in CODE_KEYS}, engine_executed=False, payloads_read=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prototype', required=True, type=Path, help='Dossier TMV prive, jamais ecrit dans la sortie')
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    for rel, expected in cap['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier change : ' + rel)
    result = inspect(args.prototype)
    require(result == cap['result'], 'ecart au resultat capture')
    for rel, expected in cap['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier change pendant lecture : ' + rel)
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
