#!/usr/bin/env python3
"""Contrelecture des 27 mutants repo6, sans compilation ni execution native."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy

HERE = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(root, cap):
    prior = HERE.parent / 'audit_tmv_traces_20261007/check.py'
    require(sha(prior) == cap['prior_reader_sha256'], 'lecteur arbre modifie')
    tree = runpy.run_path(str(prior))['tree']
    source = root / 'repo6/morsehgp3D_v12'
    before = list(tree(source))
    require(before == cap['source_tree'], 'source differente')
    report = json.loads((root / 'final6/mutants_tower.json').read_text())
    manifest = source / 'tests/mutants/tower.json'
    declared = json.loads(manifest.read_text())
    ids = [m['id'] for m in declared['mutants']]
    killed = report['mutants']
    require(report['sources_sha256'] == before[0] and report['manifeste_sha256'] == sha(manifest),
            'rapport sans ses sources')
    require(report['schema'] == 'mhgp12.mutants.v1' and report['module'] == 'tower' and
            report['code'] == 0 and report['temoin'] == 'vert' and report['plancher'] == 27 and
            declared['plancher'] == 27 and len(ids) == len(set(ids)) == len(killed) == 27 and
            {m['id'] for m in killed} == set(ids) and
            all(m['verdict'] == 'TUE' and m['detail'] == 'code' for m in killed), 'mutants incomplets')
    codes = re.findall(r'^mutants=(-?\d+) (\d\d:\d\d:\d\d)$', (root / 'final6/codes.txt').read_text(), re.M)
    require(codes == [('0', '02:10:22')], 'code final absent, repete ou different')
    require('mutants_ok module=tower mutants=27 tues=27' in (root / 'final6/mutants.log').read_text(),
            'marqueur final absent')
    require(cap['stop_report_excerpt'] in (root / 'RAPPORT.md').read_text(), 'declaration arret modifiee')
    build24 = (root / 'final6/build24.log').read_text()
    require('Terminated' in build24 and 'Error 2' in build24, 'trace interruption absente')
    require(not re.search(r'^(?:build24|ctest24|build32|ctest32|chaine_(?:24|32)_|m0_semantique_(?:24|32)_)',
                          (root / 'final6/codes.txt').read_text(), re.M), 'nouvelle cloture de profil large')
    require(not (root / 'final6/ctest24.log').exists() and not (root / 'final6/cfg32.log').exists(),
            'nouvelle etape large : reviser la portee')
    require(list(tree(source)) == before, 'source modifiee pendant lecture')
    return dict(code=0, utc='02:10:22', witness='vert', mutant_count=27, killed_by_code=sorted(ids),
                native_replayed=False, wide_profiles_qualified=False, gpu_qualified=False, gcp_used=False,
                stop_declared_utc='02:10:44', final6_status='interrompu_pendant_construction_24',
                source_failure_demonstrated=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prototype', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    for rel, expected in cap['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'piece modifiee : ' + rel)
    result = inspect(args.prototype, cap)
    require(result == cap['result'], 'resultat different')
    for rel, expected in cap['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'piece modifiee pendant lecture : ' + rel)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
