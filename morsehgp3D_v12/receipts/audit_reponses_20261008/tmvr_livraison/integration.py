"""Lecteur des journaux nommes de l'integration TMVR ; aucun binaire execute."""
import hashlib
from pathlib import Path
import re


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise ValueError(message)


def rows(text, count):
    found = re.findall(r'^\s*\d+/(\d+) Test\s+#\d+:\s*(\S+)(.*)$', text, re.M)
    require(len(found) == count and len({name for _, name, _ in found}) == count and
            all(int(n) == count for n, _, _ in found), 'selection incomplete ou repetee')
    passed = {name for _, name, tail in found if re.search(r'\bPassed\b', tail)}
    skipped = {name for _, name, tail in found if re.search(r'\bSkipped\b', tail)}
    require(passed.isdisjoint(skipped) and len(passed | skipped) == count and
            f'0 tests failed out of {count}' in text, 'resultat CTest absent ou en echec')
    return passed, skipped


def inspect(root, cap):
    root = Path(root)
    for name, expected in cap['files_sha256'].items():
        require(sha(root / name) == expected, 'journal modifie : ' + name)
    passed, skipped = rows((root / 'build_v12_u21.ctest_tmv.log').read_text(), 717)
    old_passed, old_skipped = rows((root / 'v12_tour_TMV/final6/ctest21.log').read_text(), 691)
    require(len(passed) == 716 and skipped == old_skipped == {'mhgp12_support_lidar_sentinel'},
            'reussites ou sauts differents')
    extra = passed - old_passed
    require(len(extra) == 26 and not old_passed - passed and
            all(name.startswith('mhgp12_reference_diff_v10') for name in extra), 'perimetre different')
    conf = (root / 'build_v12_u21.conf_tmv.log').read_text()
    require('748 portes enregistrees' in conf and 'bits = 21' in conf and
            "portes diff_v11 du catalogue absentes (MHGP12_V11_CATALOGUE_DIR='')" in conf and
            "portes diff_v11 de la tour absentes (MHGP12_V11_TOWER_DIR='')" in conf, 'configuration differente')
    build = (root / 'build_v12_u21.build_tmv.log').read_text()
    for name in ('mhgp12_tower_forest', 'mhgp12_tower_dumps', 'mhgp12_tower_chain', 'mhgp12_tower_forest_oracle'):
        require('Built target ' + name in build, 'construction sans cible ' + name)
    for name, expected in cap['files_sha256'].items():
        require(sha(root / name) == expected, 'journal modifie pendant lecture : ' + name)
    result = dict(registered_at_configuration=748, selected=717, passed=716,
                  skipped=sorted(skipped), failed=0, added_vs_repo6=sorted(extra),
                  removed_vs_repo6=[], process_exit_code_captured=False,
                  outer_ctest_command_captured=False, native_replayed=False,
                  full_lidar_chain_replayed_here=False, gpu_qualified=False)
    require(result == cap['result'], 'resultat different')
    return result
