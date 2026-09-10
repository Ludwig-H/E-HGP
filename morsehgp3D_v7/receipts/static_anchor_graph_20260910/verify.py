#!/usr/bin/env python3
"""Read-only verifier for the published finite static-anchor graph evidence."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

BASE = Path(__file__).resolve().parent
CAPTURE = BASE / 'capture'
PRIVATE_MANIFEST_SHA = '9a88419ad2ae42ae1ba91de6d4f7bd5fd5dfe7bb944e0a37c1c517b4f49ca0f5'
MUTANTS = {
    'drop_vertex_activation': 'no_spurious_or_missing_graph_component',
    'drop_unary_contribution': 'dated_D_reconstructs_exact_point_cover',
    'count_keys_not_roots': 'simultaneous_graph_event_matches_dynamic_anchor_model',
    'binary_same_level': 'simultaneous_graph_event_matches_dynamic_anchor_model',
    'vertical_open': 'upper_birth_samekey_lower_anchor_is_active_closed',
    'admit_global_key_without_K': 'active_facet_has_active_static_terminal',
    'consult_partial_calendar': 'complete_catalogue_has_weak_terminal',
}


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def physical(logical: str) -> Path:
    return CAPTURE / (logical + '.source' if logical == 'README.md' else logical)


def rebase(match: re.Match[str]) -> str:
    label, target = match.groups()
    if target.startswith('../../morsehgp3D_v7/'):
        target = '../../' + target[len('../../morsehgp3D_v7/'):]
    else:
        target = 'capture/' + target
    return '[' + label + '](' + target + ')'


def main() -> None:
    need(len(sys.argv) == 1, 'no_unknown_arguments')
    manifest = json.loads((BASE / 'MANIFEST.json').read_text())
    files = {str(p.relative_to(BASE)) for p in BASE.rglob('*')
             if p.is_file() and p != BASE / 'MANIFEST.json'}
    need(set(manifest) == files, 'publication_file_set')
    for relative, digest in manifest.items():
        path = BASE / relative
        need(path.resolve().is_relative_to(BASE), 'manifest_stays_inside_packet')
        data = path.read_bytes()
        need(sha(data) == digest, 'publication_hash:' + relative)
        need(not data.startswith(b'\x7fELF'), 'no_ELF')
    private_data = (CAPTURE / 'MANIFEST.json').read_bytes()
    need(sha(private_data) == PRIVATE_MANIFEST_SHA, 'original_private_manifest_pin')
    private = json.loads(private_data)
    need(len(private) == 75, 'private_manifest_file_count')
    expected_capture = {physical(name).relative_to(CAPTURE).as_posix() for name in private}
    expected_capture.add('MANIFEST.json')
    actual_capture = {p.relative_to(CAPTURE).as_posix() for p in CAPTURE.rglob('*') if p.is_file()}
    need(actual_capture == expected_capture, 'exact_capture_set_with_document_name_mapping')
    for relative, digest in private.items():
        need(sha(physical(relative).read_bytes()) == digest, 'unchanged_private_capture:' + relative)
    pins = json.loads((BASE / 'source_pins.json').read_text())
    old_pins = json.loads((CAPTURE / 'source_pins.json').read_text())
    need(pins['original_private_manifest_sha256'] == PRIVATE_MANIFEST_SHA,
         'publication_provenance_manifest')
    need(pins['original_private_file_count'] == 75 and
         pins['copied_sources_and_captures_byte_identical'] is True and
         pins['engine_is_not_executed_by_this_oracle'] is True, 'bounded_scope_and_copy_declaration')
    need(pins['original_audit_sources'] == old_pins['originals'] and
         pins['private_model_sha256'] == old_pins['private_model_sha256'] and
         pins['mechanical_model_change'] == old_pins['mechanical_change'], 'audit_provenance_pins')
    for original, digest in old_pins['originals'].items():
        name = Path(original).name
        copied = CAPTURE / name if name == 'meb_rational_oracle_20260905.py' else CAPTURE / 'geometry' / name
        data = copied.read_bytes()
        if name == 'plateau_model.py':
            need(sha(data) == old_pins['private_model_sha256'], 'private_model_pin')
            needle = b'2 <= len(points) <= 10'
            need(data.count(needle) == 1, 'unique_finite_guard_delta')
            data = data.replace(needle, b'2 <= len(points) <= 7')
        need(sha(data) == digest, 'original_audit_source_or_declared_guard_delta:' + name)
    expected_note = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', rebase, physical('README.md').read_text())
    need((BASE / 'NOTE.md').read_text() == expected_note, 'note_only_rebases_markdown_links')
    nominal = None
    commands = 0
    for directory in ['runs_normal', 'runs_optimized']:
        root = CAPTURE / directory
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
                            pins['original_private_directory'] + '/graph_oracle.py']
            if mutant != 'nominal':
                expected_cmd += ['--mutant', mutant]
            need(row['command'] == expected_cmd, 'closed_historical_command_shape')
            if mutant == 'nominal':
                need(row['returncode'] == 0 and not stderr, 'nominal_exit_and_stderr')
                if nominal is None:
                    nominal = stdout
                need(stdout == nominal, 'all_nominal_runs_identical')
                report = json.loads(stdout)
                need(report['production_Gamma_queries'] == 0 and len(report['fixtures']) == 7,
                     'bounded_geometry_only_production')
                need(report['status'] == 'passed_relative_bounded' and report['public_status'] == 'not_claimed',
                     'no_product_promotion')
                need(report['totals'] == dict(birth_events=112, changed_target_blocks=2,
                     facet_cut_checks=5943, growth_events=3, inert_events=16, merge_events=55,
                     order_cuts=648, prelot_target_checks=203, vertical_birth_anchor_checks=78,
                     vertical_facet_checks=5047), 'exact_nonvacuity_counters')
            else:
                need(row['returncode'] == 1 and not stdout, 'mutant_exact_reject_exit')
                need(stderr.decode().strip() == MUTANTS[mutant], 'mutant_exact_reject_reason')
            commands += 1
    print(json.dumps(dict(status='verified_relative_bounded_oracle', commands=commands,
                         mutants=len(MUTANTS), manifest_files=len(manifest),
                         unchanged_capture_files=len(private), public_status='not_claimed'), sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
