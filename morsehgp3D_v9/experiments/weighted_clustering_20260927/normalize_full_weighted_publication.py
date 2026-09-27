#!/usr/bin/env python3
"""Derive a NEW publication by removing Markdown trailing ASCII spaces/tabs.

The sealed parent and formatter stay unchanged. CSV bytes, line endings and
all other Markdown bytes are preserved. Only metadata/LIVE pins are replayed;
no aggregates, scores, fits, EOM, geometry or arithmetic audit are recomputed.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path
import re

import report_full_weighted_parallel as original

common = original.formatting
need, sha, read = common.need, common.sha, common.read
HERE = Path(__file__).resolve()
FORMATTER = HERE.with_name('report_full_weighted_parallel.py')
FORMATTER_SHA = '0a0b2ecffd9a63da9ac3e07f5ac50127090acb0dbf9ce846d784400eec685185'
SCHEMA = 'mhgp9_weighted_full_parallel_publication_normalized_v2'
PAYLOADS = ('rows.csv', 'aggregates.csv', 'TABLES.md')
POLICY = 'remove_only_ASCII_space_and_tab_before_LF_CRLF_or_EOF'


def normalize(data):
    """Do not decode/re-encode, strip blank lines or change line endings."""
    data.decode('utf-8')  # Reject non-Markdown payloads, preserve original bytes.
    return re.sub(rb'[ \t]+(?=\r?\n|\Z)', b'', data)


def inventory(folder):
    need(folder.is_dir() and not folder.is_symlink(), 'regular publication directory')
    need({p.name for p in folder.iterdir()} == {*PAYLOADS, 'receipt.json'},
         'exact four-file publication inventory')
    need(all((folder / name).is_file() and not (folder / name).is_symlink()
             for name in (*PAYLOADS, 'receipt.json')), 'regular publication files only')


def validate_metadata(parent, receipt, post, qualification, pins, orchestration):
    """Compare published metadata with the existing independent LIVE reader."""
    same = ('plan', 'scope', 'root_policy', 'note', 'elapsed_seconds',
            'native_binary', 'native_binary_sha256', 'commands', 'worker_commands',
            'reuse_verification', 'worker_environment', 'baseline_receipt')
    need(all(parent[key] == receipt[key] for key in same), 'parent capture metadata')
    need(parent['sources'] == receipt['sources_after'] and
         parent['input_manifest'] == receipt['manifest'] and
         parent['baseline_receipt_sha256'] == common.BASELINE_SHA and
         parent['input_manifest_sha256'] == common.MANIFEST_SHA, 'parent sources and inputs')
    need(parent['qualification'] == qualification and parent['orchestration'] == orchestration,
         'parent qualification and orchestration')
    projected = {key: value for key, value in post.items()
                 if key not in ('capture', 'receipt_sha256', 'pins_sha256', 'exact_ARI')}
    actual = {key: value for key, value in parent['post_audit'].items()
              if key not in ('private_receipt', 'receipt_sha256')}
    need(actual == projected, 'parent post-audit metadata')
    need(parent['live_pins_checked'] == len(pins) and
         parent['live_pin_map_sha256'] == common.map_digest(pins), 'parent LIVE pin inventory')
    need((parent['row_count'], parent['aggregate_count'], parent['command_count']) == (364, 140, 26),
         'complete parent publication')
    need(parent['approximate_postprocessing'] is True and
         parent['selected_subset_after_previous_scores'] is True and
         parent['live_before_and_after'] is True, 'parent declared scope')
    need(all(parent[key] is False for key in (
        'original_failure_promoted', 'raw_clouds_or_native_arrays_published',
        'fits_or_geometry_rerun', 'EOM_or_metrics_rerun', 'serial_speedup_claimed',
        'memory_bound_claimed', 'GCP_used', 'GPU_used')), 'parent scope flags')


def validated_parent(folder, expected_receipt_sha):
    inventory(folder)
    need(sha(folder / 'receipt.json') == expected_receipt_sha, 'sealed parent receipt SHA256')
    parent = read(folder / 'receipt.json')
    need(parent['schema'] == original.PUBLICATION_SCHEMA and parent['status'] == 'completed',
         'completed original parallel publication required')
    need(parent['source_sha256'] == FORMATTER_SHA == sha(FORMATTER) and
         parent['formatter_source_sha256'] == original.FORMAT_SHA, 'frozen original formatter')
    need(set(parent['files']) == set(PAYLOADS), 'parent payload inventory')
    pins = {str(folder / name): parent['files'][name] for name in PAYLOADS}
    common.add_pins(pins, {str(folder / 'receipt.json'): expected_receipt_sha})
    common.check_pins(pins)
    capture = Path(parent['private_capture']).resolve()
    post_path = Path(parent['post_audit']['private_receipt']).resolve()
    need(Path(parent['private_receipt']).resolve() == capture / 'receipt.json' and
         parent['receipt_sha256'] == sha(capture / 'receipt.json') and
         parent['post_audit']['receipt_sha256'] == sha(post_path), 'parent evidence binding')
    receipt, post, qualification, live, orchestration = original.validated_inputs(capture, post_path)
    validate_metadata(parent, receipt, post, qualification, live, orchestration)
    for name, count in (('rows.csv', 364), ('aggregates.csv', 140)):
        with (folder / name).open(newline='') as stream:
            need(len(list(csv.DictReader(stream))) == count, 'parent CSV row count')
    common.add_pins(pins, live)
    common.add_pins(pins, {str(HERE): sha(HERE), str(FORMATTER): FORMATTER_SHA})
    common.check_pins(pins)
    return parent, pins


def transformation_proof(source, target):
    csv_equal = {name: (source / name).read_bytes() == (target / name).read_bytes()
                 for name in ('rows.csv', 'aggregates.csv')}
    need(all(csv_equal.values()), 'CSV bytes must remain unchanged')
    before, after = (source / 'TABLES.md').read_bytes(), (target / 'TABLES.md').read_bytes()
    need(after == normalize(before), 'only the declared Markdown normalization is allowed')
    need(normalize(after) == after, 'normalization idempotence')
    # The exact byte transformation above is stronger than this redundant digest.
    without_whitespace = lambda b: hashlib.sha256(re.sub(rb'\s', b'', b)).hexdigest()
    need(without_whitespace(before) == without_whitespace(after), 'non-whitespace text identity')
    return dict(csv_byte_identical=csv_equal, markdown_exact_normalization=True,
                markdown_non_whitespace_sha256=without_whitespace(after),
                removed_bytes=len(before) - len(after),
                changed_lines=sum(a != b for a, b in zip(before.split(b'\n'), after.split(b'\n'))),
                line_endings_preserved=True, idempotent=True)


def derived_receipt(parent, source, parent_sha, output, pins):
    result = dict(parent)
    result.update(schema=SCHEMA, source_sha256=sha(HERE),
                  files={name: sha(output / name) for name in PAYLOADS})
    result['derivation'] = dict(
        parent_publication=str(source), parent_receipt_sha256=parent_sha,
        parent_schema=parent['schema'], parent_formatter_source_sha256=parent['source_sha256'],
        parent_files=parent['files'], normalization_source=str(HERE),
        normalization_source_sha256=sha(HERE), policy=POLICY,
        proof=transformation_proof(source, output),
        live_pins_checked=len(pins), live_pin_map_sha256=common.map_digest(pins),
        parent_live_fields_preserved=True, live_before_and_after=True,
        aggregates_recomputed=False, arithmetic_audit_recomputed=False,
        scores_fits_EOM_or_geometry_rerun=False)
    return result


def publish(source, parent_sha, output):
    need(not output.exists() and not output.is_symlink(), 'NEW output required')
    parent, pins = validated_parent(source, parent_sha)
    output.mkdir(parents=True, exist_ok=False)
    for name in PAYLOADS:
        data = (source / name).read_bytes()
        with (output / name).open('xb') as stream:
            stream.write(normalize(data) if name == 'TABLES.md' else data)
    result = derived_receipt(parent, source, parent_sha, output, pins)
    common.check_pins(pins)
    common.save(output / 'receipt.json', result)
    return result


def readback(output):
    inventory(output)
    result = read(output / 'receipt.json')
    need(result['schema'] == SCHEMA and result['status'] == 'completed', 'normalized publication schema')
    source = Path(result['derivation']['parent_publication']).resolve()
    parent_sha = result['derivation']['parent_receipt_sha256']
    parent, pins = validated_parent(source, parent_sha)
    need(result == derived_receipt(parent, source, parent_sha, output, pins),
         'exact derived metadata and hashes')
    common.check_pins(pins)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--source-receipt-sha256')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--readback', type=Path)
    args = parser.parse_args()
    if args.readback:
        need(not any((args.source, args.source_receipt_sha256, args.output)), 'readback arguments only')
        result = readback(args.readback.resolve())
    else:
        need(all((args.source, args.source_receipt_sha256, args.output)), 'source, source hash and output required')
        result = publish(args.source.resolve(), args.source_receipt_sha256, args.output.resolve())
    print(common.json.dumps(dict(status='passed', schema=result['schema'],
                                 proof=result['derivation']['proof']), sort_keys=True))
