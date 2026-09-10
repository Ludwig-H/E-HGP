#!/usr/bin/env python3
"""Portable evidence checks; no compilation, subprocess, VM, or binary execution."""
from __future__ import annotations

import argparse
import difflib
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

ROOT = Path(__file__).resolve().parent
HEADER = 'morsehgp3D_v7/src/forest/full_ball_tower.hpp'
MEB_HEADER = 'morsehgp3D_v7/src/forest/anchor_meb.hpp'
EXPECTED_COUNTS = {
    'meb_o2': 3, 'meb_san': 3, 'meb_mutants': 6, 'full_gate_r3': 18,
    'mono_o2': 9, 'mono_san': 9, 'mono_mutant': 4, 'mono_extended': 9,
    'mono_monotone_o2': 6, 'mono_monotone_san': 6, 'mono_monotone_mutant': 4,
    'work_o2': 3, 'work_san': 3, 'worker_pure': 4, 'baseline_8k_failed': 8,
}
SAN_ENV = {'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
           'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}


def need(ok: bool, why: str) -> None:
    if not ok:
        raise ValueError(why)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path: Path):
    return json.loads(path.read_text())


def safe_name(name: str) -> bool:
    path = PurePosixPath(name)
    return bool(name) and not path.is_absolute() and all(
        part not in ('', '.', '..') for part in name.split('/')) and '\\' not in name


def stream(group: str, name: str, kind: str = 'stdout') -> str:
    return (ROOT / 'captures' / group / (name + '.' + kind)).read_text()


def records(group: str):
    directory = ROOT / 'captures' / group
    if group in ('full_gate_r3', 'baseline_8k_failed'):
        return read_json(directory / 'commands.json')
    return read_json(directory / 'receipt.json')['commands']


def expected_code(group: str, name: str) -> int:
    if group == 'baseline_8k_failed' and name == 'n8000':
        return 143
    if 'argument' in name:
        return 2
    if 'selftest' in name:
        if group in ('meb_mutants', 'mono_monotone_mutant'):
            return 1
        if group == 'mono_mutant' and name == 'counters_selftest':
            return 1
        if group == 'full_gate_r3' and name.startswith('selftest_mutant_'):
            return 1
    return 0


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--extract', nargs=2, metavar=('VIEW', 'NEW_DIRECTORY'))
    args = parser.parse_args()
    manifest = read_json(ROOT / 'manifest.json')
    need(isinstance(manifest, dict) and all(safe_name(name) for name in manifest),
         'safe manifest paths')
    paths = list(ROOT.rglob('*'))
    need(not any(path.is_symlink() for path in paths), 'no symlinks')
    observed = {path.relative_to(ROOT).as_posix(): sha(path) for path in paths
                if path.is_file() and path != ROOT / 'manifest.json'}
    need(observed == manifest, 'exact inventory and content hashes')
    for path in paths:
        if path.is_file():
            data = path.read_bytes()
            need(not data.startswith(b'\x7fELF') and b'\0' not in data,
                 'text-only packet, no ELF')
    index = read_json(ROOT / 'capture_index.json')
    need(index['schema'] == 'v7-ball-local-publication-v1' and
         index['GCP_used'] is False and index['CUDA_executed'] is False and
         index['public_status'] == 'not_claimed', 'bounded CPU-only scope')
    need(set(index['groups']) == set(EXPECTED_COUNTS), 'closed capture selection')

    origins = read_json(ROOT / 'source_origins.json')
    objects = {path.name for path in (ROOT / 'objects').iterdir()}
    need(objects == set(origins), 'source object inventory')
    for name in objects:
        need(re.fullmatch('[0-9a-f]{64}', name) is not None and
             sha(ROOT / 'objects' / name) == name and origins[name], 'source object SHA')
    views = read_json(ROOT / 'views.json')

    def view(name: str, active: tuple[str, ...] = ()) -> dict[str, str]:
        need(name in views and name not in active, 'view identity and acyclic refs')
        item = views[name]
        result = view(item['extends'], (*active, name)) if 'extends' in item else {}
        for path, value in item['files'].items():
            need(safe_name(path) and value in objects, 'view source reference')
            result[path] = value
        return result

    for name in views:
        need(view(name), 'nonempty materializable view')

    def source(name: str, path: str = HEADER) -> str:
        return (ROOT / 'objects' / view(name)[path]).read_text()

    missing = []
    commands_checked = 0
    command_raw_pins = 0
    source_pins_checked = 0
    for group, count in EXPECTED_COUNTS.items():
        directory = ROOT / 'captures' / group
        receipt = read_json(directory / ('result.json' if group == 'full_gate_r3' else 'receipt.json'))
        rows = records(group)
        need(len(rows) == count and len({row['name'] for row in rows}) == count,
             'exact distinct command count: ' + group)
        for row in rows:
            name = row['name']
            need('/' not in name and safe_name(name), 'safe command identity')
            code = row.get('returncode', row.get('exit_code'))
            expected = row.get('expected', row.get('expected_returncode', 0))
            need(code == expected_code(group, name), 'observed exit: ' + group + '/' + name)
            if group == 'baseline_8k_failed':
                need(row['closed'] is True and expected == 0, 'closed original failed probe')
            else:
                need(code == expected, 'original expected exit: ' + group + '/' + name)
            start = row.get('start_ns', row.get('started_ns'))
            end = row.get('end_ns', row.get('finished_ns', row.get('ended_ns')))
            need(isinstance(start, int) and isinstance(end, int) and end >= start,
                 'command chronology')
            for kind in ('stdout', 'stderr'):
                path = directory / (name + '.' + kind)
                need(path.is_file(), 'raw command stream exists')
                if group == 'baseline_8k_failed':
                    need(kind + '_sha256' not in row, 'do not invent initial probe stream hashes')
                else:
                    need(sha(path) == row[kind + '_sha256'], 'original stream pin')
                    command_raw_pins += 1
            if group == 'full_gate_r3' and name in ('selftest_san', 'argument_san'):
                need(row['environment_overrides'] == SAN_ENV, 'gate sanitizers including leaks')
            commands_checked += 1
        if group in ('meb_san', 'mono_san', 'mono_extended', 'mono_monotone_san', 'work_san'):
            need(receipt['sanitizer_env'] == SAN_ENV, 'recorded sanitizer environment')
        for field in ('sources', 'sources_before', 'sources_after', 'source_hashes'):
            for name, value in receipt.get(field, {}).items():
                if value not in objects:
                    missing.append({'group': group, 'field': field, 'name': name, 'sha256': value})
                else:
                    source_pins_checked += 1
        if 'recorder_sha256' in receipt:
            need(receipt['recorder_sha256'] in objects, 'retained exact recorder')
        if group.startswith('work_'):
            need(receipt['antidrift'] is True and
                 receipt['sources_before'] == receipt['sources_after'], 'active source antidrift')
            need({name: value for name, value in receipt['sources_before'].items()
                  if name.startswith('morsehgp3D_v7/')} == view('work_active'),
                 'active source view equals original pins')
    unavailable = index['known_unavailable_source_pin']
    need(missing == [{key: unavailable[key] for key in ('group', 'field', 'name', 'sha256')}],
         'only declared initial signedness pin unavailable')
    need(commands_checked == 95 and command_raw_pins == 174, 'nonvacuous command inventory')

    baseline = read_json(ROOT / 'captures/baseline_8k_failed/receipt.json')
    need(baseline['status'] == 'failed' and baseline['all_processes_closed'] is True and
         baseline['error'] == 'RuntimeError: unexpected exit: n8000', 'failed is not completed')
    need(read_json(ROOT / 'captures/baseline_8k_failed/sources_before.json') ==
         view('baseline_8k_failed'), 'original failed probe source pins')
    failed_row = next(row for row in records('baseline_8k_failed') if row['name'] == 'n8000')
    need(not stream('baseline_8k_failed', 'n8000'), 'no terminal n8000 output')
    baseline_seconds = (failed_row['ended_ns'] - failed_row['started_ns']) / 1e9
    need(575 < baseline_seconds < 576, 'failed original interval')
    meb = json.loads(stream('meb_o2', 'selftest'))
    need(stream('meb_o2', 'selftest') == stream('meb_san', 'selftest') and
         meb['status'] == 'pass' and meb['checks'] == 11752 and meb['comparisons'] == 605 and
         meb['extra_shells'] == 197 and all(value > 0 for value in meb['accepted_supports']),
         'MEB exact local nonvacuity and sanitizer agreement')
    full = json.loads(stream('full_gate_r3', 'selftest_o2'))
    need(full['status'] == 'passed' and full['checks'] == 130734 and full['clouds'] == 24 and
         full['orders'] == 100 and full['cuts'] == 2136 and full['vertical_checks'] == 35462 and
         full['growth_snapshots'] == 4 and full['extra_blocks'] == 122 and
         full['same_radius_steps'] == 2 and full['rejections'] == 8 and
         full['authority'] == 'bounded_independent_Gram_Gamma_not_WSPD_completeness',
         'independent bounded full gate nonvacuity')
    for group, name in (('full_gate_r3', 'selftest_san'), ('mono_extended', 'o2_selftest'),
                        ('mono_extended', 'san_selftest'),
                        ('mono_monotone_o2', 'oracle_selftest'),
                        ('mono_monotone_san', 'oracle_selftest')):
        need(stream(group, name) == stream('full_gate_r3', 'selftest_o2'),
             'same final oracle output across nominal variants')
    for group in ('mono_o2', 'mono_san'):
        need(stream(group, 'baseline_selftest') == stream(group, 'candidate_selftest'),
             'initial baseline/candidate oracle differential')
    need(stream('mono_mutant', 'oracle_selftest') == stream('mono_o2', 'baseline_selftest') and
         stream('mono_mutant', 'counters_selftest', 'stderr').strip() == 'mono.physical_unitary_work',
         'unitary mutant changes work, not geometry')
    need(stream('mono_monotone_mutant', 'comb_selftest', 'stderr').strip() == 'birth_initial_cut',
         'future activation causal rejection')
    need(stream('work_o2', 'selftest') == stream('work_san', 'selftest'), 'active work O2/SAN agreement')
    work_lines = [json.loads(line) for line in stream('work_o2', 'selftest').splitlines()]
    need(len(work_lines) == 4, 'three combs and one active work result')
    for row, merges, old_edges in zip(work_lines[:3], (256, 512, 1024), (32896, 131328, 524800)):
        need(row == {'merges': merges, 'nodes': 2 * merges + 1,
                     'baseline_edges': old_edges, 'activated_edges': 2 * merges,
                     'find_steps': 4 * merges - 2, 'path_writes': 2 * merges}, 'physical comb scaling')
    need(work_lines[-1] == {'status': 'passed', 'clouds': 24, 'declared_support_checks': 270,
                           'extra_MEB_calls': 36, 'extra_MEB_supports': 42, 'singleton_lots': 290,
                           'grouped_lots': 120, 'lot_dsu_slots': 356, 'gcp_used': False},
         'active direct-support/singleton work counters')
    for group in ('mono_monotone_o2', 'mono_monotone_san'):
        need([json.loads(line) for line in stream(group, 'comb_selftest').splitlines()] == work_lines[:3],
             'private and active physical comb agreement')
    for name in ('full_ball_worker_v7', 'full_probe_worker_v7'):
        raw = stream('worker_pure', 'selftest_' + name + '_normal')
        need(raw == stream('worker_pure', 'selftest_' + name + '_optimized'), 'worker normal/-O agreement')
        info = json.loads(raw)
        need(info['status'] == 'passed' and info['GCP_used'] is False and
             info['subprocess_invoked'] is False, 'pure worker tests only')
    need(json.loads(stream('worker_pure', 'selftest_full_ball_worker_v7_normal'))['checks'] == 386,
         'nonvacuous worker ball gate')

    def mutation(parent: str, child: str, old: str, new: str, path: str = HEADER) -> None:
        original = source(parent, path)
        need(original.count(old) == 1 and source(child, path) == original.replace(old, new),
             'exact sole source mutation: ' + child)

    mutation('meb_nominal', 'meb_shell_is_support', 'result.selected_shell_count = shell;',
             'result.selected_shell_count = q;', MEB_HEADER)
    mutation('meb_nominal', 'meb_accept_outside', 'if (power > 0) return false;',
             'if (power > 0) continue;', MEB_HEADER)
    mutation('meb_nominal', 'meb_reject_extra_shell', 'if (!anchor_meb_detail::charge(work.materializations)) {',
             'if (shell != q) return false;\n    if (!anchor_meb_detail::charge(work.materializations)) {', MEB_HEADER)
    mutation('full_baseline', 'full_drop_growth',
             'if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));',
             'if (action.parents.size() != 1) batch.actions.push_back(std::move(action));')
    mutation('full_baseline', 'full_drop_inert_anchor', 'anchors[blocks[b].ball] = targets[b];',
             'if (blocks[b].roots.size() == 1 && blocks[b].contribution == 0 && !blocks[b].interior) continue;\n'
             '      anchors[blocks[b].ball] = targets[b];')
    mutation('full_baseline', 'full_wrong_vertical_cut',
             'return full_coverage_root_at(tower.orders[k - 2].forest, upper.lower_nodes[root], cut, closed);',
             'return full_coverage_root_at(tower.orders[k - 2].forest, upper.lower_nodes[root],\n'
             '      tower.orders[k - 2].forest.nodes().back().level, true);')
    mutation('full_baseline', 'full_strict_radius_only', 'require(cmp <= 0, "full_ball_radius_increased");',
             'require(cmp < 0, "full_ball_radius_increased");')
    mutation('mono_candidate', 'mono_mutant', 'if (blocks.size() == 1) {',
             'if (blocks.size() == 1 && false) {')
    mutation('mono_monotone', 'mono_monotone_future_mutant',
             'full_coverage_detail::admitted(source.levels[cursor], cut, closed)', 'true')
    result = read_json(ROOT / 'captures/full_gate_r3/result.json')
    need(result['source_hashes'] == view('full_baseline') and
         read_json(ROOT / 'captures/full_gate_r3/sources.json') == view('full_baseline') and
         result['GCP_used'] is False and result['status'] == 'passed', 'full original source pins and scope')
    for name, info in result['mutants'].items():
        need(view('full_' + name)[HEADER] == info['header_sha256'] and
             stream('full_gate_r3', 'selftest_mutant_' + name, 'stderr').strip() == info['diagnostic'],
             'full original mutant source/diagnostic pin')
    for summary_name, prefix, modes in (
        ('summary.json', 'mono_', ('o2', 'san', 'mutant', 'extended')),
        ('monotone_summary.json', 'mono_monotone_', ('o2', 'san', 'mutant'))):
        summary = read_json(ROOT / 'metadata' / summary_name)
        need(summary['status'] == 'passed', 'original bounded summary')
        for mode in modes:
            need(summary['receipts'][mode] == sha(ROOT / 'captures' / (prefix + mode) / 'receipt.json'),
                 'original mono summary receipt pins')
    for first, second, patch_name in (('full_baseline', 'mono_candidate', 'candidate.patch'),
                                      ('mono_candidate', 'mono_monotone', 'monotone.patch'),
                                      ('full_baseline', 'mono_monotone', 'combined.patch')):
        patch = ''.join(difflib.unified_diff(source(first).splitlines(keepends=True),
                                            source(second).splitlines(keepends=True),
                                            fromfile='a/' + HEADER, tofile='b/' + HEADER))
        need(patch == (ROOT / 'metadata' / patch_name).read_text(), 'exact physical patch ' + patch_name)
    need(view('work_active')[HEADER] == view('mono_monotone')[HEADER], 'active combined variant identity')

    if args.extract:
        name, destination = args.extract
        files = view(name)
        target = Path(destination).absolute()
        need(not target.exists(), 'extraction requires a new directory')
        target.mkdir(parents=True, exist_ok=False)
        for name, value in files.items():
            path = target / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes((ROOT / 'objects' / value).read_bytes())
    print(json.dumps({'status': 'passed', 'files': len(manifest), 'source_objects': len(objects),
                      'views': len(views), 'commands': commands_checked,
                      'original_command_stream_pins': command_raw_pins,
                      'available_original_source_pins': source_pins_checked,
                      'declared_unavailable_initial_pin': len(missing),
                      'baseline_8k_status': 'failed', 'baseline_8k_exit': 143,
                      'baseline_8k_elapsed_s': baseline_seconds, 'full_checks': full['checks'],
                      'CUDA_executed': False, 'GCP_used': False, 'binary_executed': False,
                      'public_status': 'not_claimed'}, sort_keys=True))


if __name__ == '__main__':
    main()
