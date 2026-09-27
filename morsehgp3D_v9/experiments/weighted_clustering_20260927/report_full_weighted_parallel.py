#!/usr/bin/env python3
"""Publish a completed parallel pilot, never the interrupted serial attempt.

Explicit reuse of the pinned serial formatter's pure CSV/aggregate/table and
qualification helpers. The parallel reader validates orchestration, reuse,
command streams and live pins; its passed arithmetic receipt is required.
No saved receipt is rewritten or coerced to another schema. No score, EOM,
fit or geometry is rerun. New output only; before/after LIVE pin checks.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import json

import report_full_weighted as formatting
import post_audit_parallel as reader

HERE = Path(__file__).resolve().parent
SCHEMA = 'mhgp9_weighted_full_gaussian_parallel_pilot_v1'
AUDIT_SCHEMA = 'mhgp9_weighted_parallel_post_capture_score_audit_v1'
PUBLICATION_SCHEMA = 'mhgp9_weighted_full_parallel_publication_v1'
FORMAT_SOURCE = HERE / 'report_full_weighted.py'
FORMAT_SHA = 'b7bfba2b1364beb022a74698f6f468d6c3d4433abefb08467cdeaa5d9cbf8e9b'
AUDIT_SOURCE = HERE / 'post_audit_parallel.py'
ARITHMETIC_SOURCE = HERE / 'post_audit.py'
ARITHMETIC_SHA = '95cece7c7b74d839495485d8bad018abffb246458f2d4a800e68c479387e6e1d'
need, sha, read = formatting.need, formatting.sha, formatting.read


def validate_grid(receipt):
    """Additional formatter guard, without modifying the parallel receipt."""
    need(receipt['schema'] == SCHEMA and receipt['status'] == 'completed',
         'completed parallel capture required, never a promoted failure')
    plan = dict(cases=list(formatting.CASES), k=[5, 10], sizes=[20, 50], exp_z=[1, 2])
    need(receipt['plan'] == plan, 'fixed parallel pilot plan')
    expected = {(c, k, m, z, method) for c in formatting.CASES for k in (5, 10)
                for m in (20, 50) for z in (1, 2) for method in formatting.METHODS}
    expected.update((c, k, m, 1, 'hdbscan_standard') for c in formatting.CASES
                    for k in (5, 10) for m in (20, 50))
    keys = [tuple(row[name] for name in ('case', 'k', 'min_cluster_size', 'exp_z', 'method'))
            for row in receipt['rows']]
    need(len(keys) == 364 and set(keys) == expected, 'exact unique 364-row grid')
    commands = [(row['case'], row['k']) for row in receipt['commands']]
    need(len(commands) == 26 and set(commands) == {(c, k) for c in formatting.CASES for k in (5, 10)},
         'exact unique 26-native-command grid')


def validate_post(post, capture, receipt_digest, reader_pins, orchestration):
    """Bind a passed arithmetic audit to freshly replayed provenance."""
    need(post['schema'] == AUDIT_SCHEMA and post['status'] == 'passed', 'passed parallel post-audit required')
    need(Path(post['capture']).resolve() == capture and post['receipt_sha256'] == receipt_digest,
         'post-audit exact capture binding')
    need((post['weighted_rows'], post['comparator_rows'], post['total_rows'], post['measures']) ==
         (104, 260, 364, 52), 'whole parallel arithmetic audit')
    need(post['audit_source_sha256'] == sha(AUDIT_SOURCE) and
         post['arithmetic_source_sha256'] == ARITHMETIC_SHA, 'post-audit executable source binding')
    need(post['pins_sha256'] == reader_pins and post['pins_checked'] == len(reader_pins),
         'post-audit pin inventory equals current reader replay')
    need(post['orchestration'] == orchestration, 'post-audit orchestration equals current reader replay')
    need(post['Hungarian_solver_shared'] is True and post['NMI_recomputed'] is False and
         post['geometry_or_EOM_rerun'] is False and post['GCP_used'] is False, 'honest post-audit scope')


def validated_inputs(capture, post_path):
    need(sha(FORMAT_SOURCE) == FORMAT_SHA and sha(ARITHMETIC_SOURCE) == ARITHMETIC_SHA,
         'pinned serial formatting/arithmetic sources')
    receipt, _, _, reader_pins, orchestration = reader.validate_capture(capture)
    validate_grid(receipt)
    receipt_path = capture / 'receipt.json'
    receipt_digest = sha(receipt_path)
    post = read(post_path)
    validate_post(post, capture, receipt_digest, reader_pins, orchestration)
    pins = {}
    formatting.add_pins(pins, reader_pins)
    formatting.add_pins(pins, {
        str(post_path): sha(post_path), str(receipt_path): receipt_digest,
        str(FORMAT_SOURCE): FORMAT_SHA, str(ARITHMETIC_SOURCE): ARITHMETIC_SHA,
        str(AUDIT_SOURCE): post['audit_source_sha256'], str(Path(__file__).resolve()): sha(__file__),
    })
    qualification = formatting.qualify_metadata(receipt['qualification'], receipt, pins)
    formatting.check_pins(pins)
    return receipt, post, qualification, pins, orchestration


def flatten(row):
    flat = formatting.flatten(row)
    # This describes the case/K unit, not a new fit of an inherited comparator.
    flat['unit_origin'] = 'reused_interrupted_serial_unit' if 'reused_from' in row else 'new_parallel_worker'
    return flat


def tables(aggregate, orchestration):
    text = formatting.tables(aggregate)
    heading = '## expZ=2\n\n'
    need(text.count(heading) == 1, 'unique z2 interpretation location')
    text = text.replace(heading, heading +
        'Passer de z1 à z2 modifie chez **HGP pondéré** les scores de facettes Sτ, '
        'leurs masses et λ. **HDBSCAN EOM commun** conserve ses masses ponctuelles '
        'unitaires et change λ seulement ; il en va de même pour la projection '
        'première couverture figée. La règle EOM est commune, mais les mesures '
        'sont différentes : cette comparaison n’est pas une ablation pure de λ.\n\n', 1)
    text += '\n## Exécution et reprise\n\n'
    text += (f"{orchestration['reused_units']} unités complètes reprises, "
             f"{orchestration['newly_executed_units']} unités exécutées dans de nouveaux workers ; "
             f"maximum observé de {orchestration['maximum_observed_workers']} workers simultanés. "
             'Une unité est une scène entière et un ordre K, avec les quatre sélections pondérées. '
             'Les comparateurs restent hérités de la campagne précédente, sans nouveau fit.\n\n')
    if orchestration['original_failure_preserved']:
        text += ('Le reçu séquentiel interrompu reste **failed** et est conservé par hash. '
                 'Seules ses unités complètes, avec les six payloads et quatorze lignes liés, sont reprises. ')
        if orchestration['original_source_closure_missing']:
            text += ('Sa clôture originale des sources était absente ; la nouvelle vérification LIVE '
                     'ne la recrée pas rétroactivement. ')
        text += '\n\n'
    text += ('Les durées mélangent unités héritées et exécutions sous concurrence. '
             'Elles ne mesurent ni un gain série/parallèle, ni une borne mémoire. '
             'La compression gzip a changé, pas les valeurs JSON attendues. '
             'Les métadonnées de reprise et les commandes des workers restent dans le reçu publié.\n')
    return text


def publish(capture, post_path, output):
    need(not output.exists(), 'NEW publication directory required')
    receipt, post, qualification, pins, orchestration = validated_inputs(capture, post_path)
    rows = [flatten(row) for row in sorted(receipt['rows'],
            key=lambda row: tuple(row[key] for key in formatting.KEYS))]
    aggregate = formatting.aggregates(rows)
    output.mkdir(parents=True, exist_ok=False)
    formatting.write_csv(output / 'rows.csv', rows)
    formatting.write_csv(output / 'aggregates.csv', aggregate)
    with (output / 'TABLES.md').open('x') as stream:
        stream.write(tables(aggregate, orchestration))
    summary = dict(
        schema=PUBLICATION_SCHEMA, status='completed', private_capture=str(capture),
        private_receipt=str(capture / 'receipt.json'), receipt_sha256=sha(capture / 'receipt.json'),
        source_sha256=sha(__file__), formatter_source_sha256=FORMAT_SHA,
        row_count=len(rows), aggregate_count=len(aggregate), command_count=len(receipt['commands']),
        plan=receipt['plan'], scope=receipt['scope'], root_policy=receipt['root_policy'],
        approximate_postprocessing=True, selected_subset_after_previous_scores=True, note=receipt['note'],
        elapsed_seconds=receipt['elapsed_seconds'], native_binary=receipt['native_binary'],
        native_binary_sha256=receipt['native_binary_sha256'], sources=receipt['sources_after'],
        commands=receipt['commands'], worker_commands=receipt['worker_commands'],
        orchestration=orchestration, reuse_verification=receipt['reuse_verification'],
        original_failure_promoted=False, worker_environment=receipt['worker_environment'],
        baseline_receipt=receipt['baseline_receipt'], baseline_receipt_sha256=formatting.BASELINE_SHA,
        input_manifest=receipt['manifest'], input_manifest_sha256=formatting.MANIFEST_SHA,
        qualification=qualification,
        post_audit=dict(private_receipt=str(post_path), receipt_sha256=sha(post_path),
            **{key: value for key, value in post.items()
               if key not in ('capture', 'receipt_sha256', 'pins_sha256', 'exact_ARI')}),
        live_pins_checked=len(pins), live_pin_map_sha256=formatting.map_digest(pins),
        live_before_and_after=True, raw_clouds_or_native_arrays_published=False,
        fits_or_geometry_rerun=False, EOM_or_metrics_rerun=False,
        serial_speedup_claimed=False, memory_bound_claimed=False, GCP_used=False, GPU_used=False,
        files={name: sha(output / name) for name in ('rows.csv', 'aggregates.csv', 'TABLES.md')})
    formatting.check_pins(pins)
    formatting.save(output / 'receipt.json', summary)
    print(json.dumps(dict(status='published', rows=len(rows), aggregates=len(aggregate),
                         output=str(output), GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', required=True, type=Path)
    parser.add_argument('--post-audit', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    publish(args.capture.resolve(), args.post_audit.resolve(), args.output.resolve())
