#!/usr/bin/env python3
"""Read-only integrity and bounded-claim reader; effective under Python -O."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent


def need(value, why):
    if not value:
        raise ValueError(why)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(name):
    return json.loads((BASE / name).read_text())


def main():
    manifest = read('manifest.json')
    for name, sha in manifest['files'].items():
        path = Path(name)
        need(not path.is_absolute() and '..' not in path.parts, 'manifest.path')
        p = BASE / path
        need(p.is_file() and not p.is_symlink() and digest(p) == sha, f'manifest.hash:{name}')
        need(p.read_bytes()[:4] != b'\x7fELF', f'manifest.ELF:{name}')
    original = BASE / 'base_full_ball_tower.hpp.source'
    need(digest(original) == '83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366', 'base.pin')
    old = original.read_bytes()
    marker = b'class Builder {\n'
    need(old.count(marker) == 1, 'friend.unique_marker')
    expected = old.replace(marker, marker + b'  friend struct RankAtlasReference;  // Private gate access only; no changed algorithm.\n')
    need((BASE / 'source/morsehgp3D_v7/src/forest/full_ball_tower.hpp').read_bytes() == expected,
         'friend.only_change')
    cases = ['selftest', 'date', 'offset', 'mask', 'contribution', 'admission', 'permutation', 'unknown', 'missing']
    causes = {'date': 'atlas.rank_date', 'offset': 'atlas.rep_interval',
              'mask': 'gate.representative_identity_order', 'contribution': 'gate.contribution_exact',
              'admission': 'atlas.admission_base', 'permutation': 'atlas.program_order'}
    for folder, san in [('o2_r1', False), ('san_root_r1', True)]:
        receipt = read(f'{folder}/receipt.json')
        need(receipt['status'] == 'passed' and receipt['source_stable'] is True and
             receipt['commands'] == 11 and receipt['san'] is san and receipt['gcp_used'] is False and
             receipt['device_executed'] is False, f'{folder}.receipt')
        before = read(f'{folder}/sources_before.json')
        need(before == read(f'{folder}/sources_after.json'), f'{folder}.source_stable')
        for path, sha in before.items():
            actual = 'README.source' if path == 'README.md' else path
            need(actual in manifest['files'] and digest(BASE / actual) == sha, f'{folder}.source:{path}')
        commands = read(f'{folder}/commands.json')
        need([r['name'] for r in commands] == ['compiler', 'compile', *cases], f'{folder}.command_set')
        for row in commands:
            expected_code = 4 if row['name'] in causes else 2 if row['name'] in ('unknown', 'missing') else 0
            need(row['returncode'] == row['expected'] == expected_code, f'{folder}.exit:{row["name"]}')
            if san:
                need(row['environment'] == {'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}, f'{folder}.san_environment')
        flags = commands[1]['argv']
        need(all(flag in flags for flag in ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread']),
             f'{folder}.strict_flags')
        need(('-fsanitize=address,undefined' in flags) is san, f'{folder}.san_flags')
        need((BASE / folder / 'compile.stderr').read_bytes() == b'', f'{folder}.no_compile_warning')
        result = read(f'{folder}/selftest.stdout')
        need(result['status'] == 'passed' and result['public_status'] == 'not_claimed' and
             result['device_executed'] is False and result['backend'] == 'cpu_reference', f'{folder}.status')
        fixed = {'checks': 22265350, 'fixtures': 13, 'balls': 16324, 'cells': 30562,
                 'representatives': 52469, 'births': 14212, 'point_births': 226, 'q3': 7830,
                 'q4': 5976, 'extras': 25, 'extra_tables': 25, 'extra_rank_calls': 82,
                 'regular_cells': 30480, 'equal_level_pairs': 120, 'unequal_raw_equal_pairs': 66,
                 'nonidentity_shells': 19, 'terminal_births': 6}
        need(all(result[k] == v for k, v in fixed.items()), f'{folder}.nonvacuity')
        need(result['extra_tables'] == result['extras'] and result['regular_cells'] + result['extra_rank_calls'] == result['cells'],
             f'{folder}.one_extraction')
        orders = result['per_k']
        need([r['K'] for r in orders] == list(range(1, 11)), f'{folder}.orders')
        for target, source in [('cells', 'A'), ('representatives', 'R'), ('births', 'births')]:
            need(result[target] == sum(r[source] for r in orders), f'{folder}.sum:{source}')
        need(orders[8]['R'] > 0 and orders[9]['R'] > 0, f'{folder}.highK')
        memory = result['memory']
        need(memory == {'logical_max': 152296, 'capacity_max': 173256, 'borrowed_ball_bytes_max': 605024,
             'owned_BallData_copies': 0, 'sizeof_ball_slot': 16, 'sizeof_contribution': 4,
             'sizeof_mask': 2, 'peak_RSS_measured': False}, f'{folder}.memory_scope')
        for name, cause in causes.items():
            rejection = read(f'{folder}/{name}.stdout')
            need(rejection['status'] == 'causal_rejection' and rejection['fault'] == '--' + name and
                 rejection['cause'] == cause and rejection['checks'] > 0, f'{folder}.cause:{name}')
    for name in cases:
        for extension in ('stdout', 'stderr'):
            need((BASE / 'o2_r1' / f'{name}.{extension}').read_bytes() ==
                 (BASE / 'san_root_r1' / f'{name}.{extension}').read_bytes(), f'O2_SAN_identity:{name}.{extension}')
    print(json.dumps({'status': 'passed', 'files': len(manifest['files']), 'commands': 22,
                      'state_faults_per_mode': 6, 'scope': 'bounded atlas differential, not geometry or FULL forests',
                      'gcp_used': False}, sort_keys=True))


if __name__ == '__main__':
    main()
