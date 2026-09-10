#!/usr/bin/env python3
"""Read-only finite-proof reader, with a separate one-shot private seal mode."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
MUTANTS = {
    'drop_vertex_activation': 'no_spurious_or_missing_graph_component',
    'drop_unary_contribution': 'dated_D_reconstructs_exact_point_cover',
    'count_keys_not_roots': 'simultaneous_graph_event_matches_dynamic_anchor_model',
    'binary_same_level': 'simultaneous_graph_event_matches_dynamic_anchor_model',
    'vertical_open': 'upper_birth_samekey_lower_anchor_is_active_closed',
    'admit_global_key_without_K': 'active_facet_has_active_static_terminal',
    'consult_partial_calendar': 'complete_catalogue_has_weak_terminal',
}


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def relevant():
    files = [BASE / name for name in ['README.md', 'freeze.py', 'graph_oracle.py', 'qualify.py',
             'verify.py', 'source_pins.json', 'meb_rational_oracle_20260905.py']]
    for directory in ['geometry', 'runs_normal', 'runs_optimized']:
        files.extend(p for p in (BASE / directory).rglob('*') if p.is_file())
    return sorted(files)


def main():
    if sys.argv[1:] == ['--seal']:
        path = BASE / 'MANIFEST.json'
        need(not path.exists(), 'no_overwrite_seal')
        payload = {str(p.relative_to(BASE)): sha(p.read_bytes()) for p in relevant()}
        path.write_text(json.dumps(payload, indent=2, sort_keys=True) + '\n')
    else:
        need(len(sys.argv) == 1, 'no_unknown_arguments')
    manifest = json.loads((BASE / 'MANIFEST.json').read_text())
    need(set(manifest) == {str(p.relative_to(BASE)) for p in relevant()}, 'manifest_file_set')
    for name, digest in manifest.items():
        data = (BASE / name).read_bytes()
        need(sha(data) == digest, 'file_hash:' + name)
        need(not data.startswith(b'\x7fELF'), 'no_ELF')
    nominal = None
    commands = 0
    for directory in ['runs_normal', 'runs_optimized']:
        root = BASE / directory
        records = json.loads((root / 'commands.json').read_text())
        need(len(records) == 16, 'sixteen_closed_commands')
        expected_names = {prefix + (name or 'nominal') for prefix in ['normal_', 'opt_']
                          for name in ['', *MUTANTS]}
        need({row['name'] for row in records} == expected_names, 'exact_command_set')
        for row in records:
            name = row['name']
            stdout = (root / (name + '.stdout')).read_bytes()
            stderr = (root / (name + '.stderr')).read_bytes()
            need(sha(stdout) == row['stdout_sha256'] and sha(stderr) == row['stderr_sha256'], 'raw_output_hash')
            mutant = name.split('_', 1)[1]
            expected_cmd = [row['command'][0], '-B', *(['-O'] if name.startswith('opt_') else []),
                            str(BASE / 'graph_oracle.py')]
            if mutant != 'nominal':
                expected_cmd += ['--mutant', mutant]
            need(row['command'] == expected_cmd, 'closed_command_shape')
            if mutant == 'nominal':
                need(row['returncode'] == 0 and not stderr, 'nominal_exit_and_stderr')
                if nominal is None:
                    nominal = stdout
                need(stdout == nominal, 'all_nominal_runs_identical')
                report = json.loads(stdout)
                need(report['production_Gamma_queries'] == 0 and len(report['fixtures']) == 7,
                     'bounded_geometry_only_production')
                need(report['totals'] == dict(birth_events=112, changed_target_blocks=2,
                     facet_cut_checks=5943, growth_events=3, inert_events=16, merge_events=55,
                     order_cuts=648, prelot_target_checks=203, vertical_birth_anchor_checks=78,
                     vertical_facet_checks=5047), 'exact_nonvacuity_counters')
            else:
                need(row['returncode'] == 1 and not stdout, 'mutant_exact_reject_exit')
                need(stderr.decode().strip() == MUTANTS[mutant], 'mutant_exact_reject_reason')
            commands += 1
    print(json.dumps(dict(status='verified_relative_bounded', commands=commands, mutants=len(MUTANTS),
                         manifest_files=len(manifest), public_status='not_claimed'), sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
