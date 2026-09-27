#!/usr/bin/env python3
"""Publish only the completed FULL-weighted pilot plus its passed post-audit.

No fit, geometry, EOM or score computation is rerun. LIVE hashes are checked
before and after publication. Raw clouds and native/condensed arrays remain
private. A failed attempt is never promoted, and the output must be NEW.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics

HERE = Path(__file__).resolve().parent
SCHEMA = 'mhgp9_weighted_full_gaussian_pilot_v2'
AUDIT_SCHEMA = 'mhgp9_weighted_post_capture_score_audit_v1'
QUALIFICATION_SCHEMA = 'mhgp9_weighted_full_attachment_qualification_v1'
BASELINE_SHA = '8f9aba99396b6341f7a6872956e66f50a7776cc1535d9a3f9137907d2945157a'
MANIFEST_SHA = '1cab6404055aebf0dfa31b76f16f8a4c84e4acf9678b2b910dabd9f367f46c67'
METHODS = ('hgp_weighted_full_vote', 'hgp_first_coverage', 'hdbscan_common')
REGIMES = (('spherical', 2, 8), ('spherical', 8, 4), ('spherical', 16, 2),
           ('anisotropic', 8, 4), ('unbalanced', 8, 4))
CASES = tuple(f'{regime}_g{g}_d{d}_s{s}' for regime, g, d in REGIMES
              for s in ((1, 2, 3) if regime == 'spherical' else (1, 2)))
KEYS = ('case', 'regime', 'communities', 'separation', 'seed', 'n', 'k',
        'min_cluster_size', 'exp_z', 'method')
GROUP_KEYS = ('regime', 'communities', 'separation', 'k', 'min_cluster_size', 'exp_z', 'method')
METRICS = ('ari_all', 'ari_true_inliers', 'ari_inliers_noise_singletons', 'ari_classified',
           'nmi_all', 'coverage', 'clusters', 'noise_count')
EXTRA = ('truth_classes', 'predicted_clusters', 'cluster_count_error', 'cluster_count_absolute_error',
         'exact_classes', 'exact_class_fraction', 'matched_correct_points', 'matched_macro_precision',
         'matched_macro_recall', 'matched_macro_f1', 'matched_micro_precision', 'matched_micro_recall',
         'matched_micro_f1', 'eligible_classes', 'classes_below_min_cluster_size')


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write('\n')


def map_digest(mapping):
    return hashlib.sha256(json.dumps(mapping, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def add_pins(pins, additions):
    for path, digest in additions.items():
        need(isinstance(digest, str) and len(digest) == 64 and
             all(c in '0123456789abcdef' for c in digest), 'SHA256 format')
        path = str(Path(path).resolve())
        need(path not in pins or pins[path] == digest, 'conflicting pin: ' + path)
        pins[path] = digest


def check_pins(pins):
    for path, digest in pins.items():
        need(sha(path) == digest, 'LIVE pin changed: ' + path)


def validate_grid(receipt):
    need(receipt['schema'] == SCHEMA and receipt['status'] == 'completed',
         'requires a completed R2 capture; failures remain failures')
    need(receipt['plan'] == dict(cases=list(CASES), k=[5, 10], sizes=[20, 50], exp_z=[1, 2]),
         'fixed 13-case pilot plan')
    expected = {(c, k, m, z, method) for c in CASES for k in (5, 10) for m in (20, 50)
                for z in (1, 2) for method in METHODS}
    expected.update((c, k, m, 1, 'hdbscan_standard') for c in CASES for k in (5, 10) for m in (20, 50))
    key = lambda row: tuple(row[field] for field in ('case', 'k', 'min_cluster_size', 'exp_z', 'method'))
    need(len(receipt['rows']) == 364 and {key(row) for row in receipt['rows']} == expected,
         'exact 364-row grid, including K10/m50')
    for row in receipt['rows']:
        regime, g, d = row['regime'], row['communities'], row['separation']
        need(row['case'].rsplit('_s', 1)[0] == f'{regime}_g{g}_d{d:g}' and row['n'] == 1200,
             'row scene metadata')
    need(len(receipt['commands']) == 26 and
         {(c['case'], c['k']) for c in receipt['commands']} == {(c, k) for c in CASES for k in (5, 10)},
         'exact 26-command grid')
    need(receipt['root_policy'] == 'excluded_for_both' and receipt['approximate_postprocessing'] is True
         and receipt['selected_subset_after_previous_scores'] is True, 'declared prototype scope')
    need(all(receipt[key] is False for key in ('GCP_used', 'GPU_used', 'engine_modified')),
         'local experiment scope')


def closed_sources(receipt, pins):
    need(receipt['sources_before'] == receipt['sources_after'] and receipt['sources_before'], 'source closure')
    add_pins(pins, receipt['sources_after'])


def command_binding(directory, command, stdout, stderr, pins):
    need(command['returncode'] == 0 and not command.get('timed_out'), 'successful recorded command')
    path = directory / ('command.json' if 'case' in command else command['name'] + '.command.json')
    need(read(path) == command, 'command JSON differs from receipt')
    add_pins(pins, {str(path): sha(path), str(directory / stdout): command['stdout_sha256'],
                    str(directory / stderr): command['stderr_sha256']})


def qualify_metadata(path, receipt, pins):
    qualification = read(path)
    need(qualification['schema'] == QUALIFICATION_SCHEMA and qualification['status'] == 'passed',
         'passed FULL attachment qualification required')
    need(qualification['native_binary_sha256'] == receipt['native_binary_sha256'] and
         Path(qualification['native_binary']).resolve() == Path(receipt['native_binary']).resolve(),
         'same qualified native executable')
    closed_sources(qualification, pins)
    add_pins(pins, {qualification['build_receipt']: qualification['build_receipt_sha256']})
    build = read(qualification['build_receipt'])
    need(build['status'] == 'completed' and build['pins_before'] == build['pins_after'], 'closed native build')
    need(build['binary_sha256'] == receipt['native_binary_sha256'], 'native build executable binding')
    add_pins(pins, build['pins_after'])
    folder = Path(path).parent
    files = {p.name for p in folder.iterdir() if p.is_file() and p.name != 'receipt.json'}
    need(files == set(qualification['artifacts']), 'qualification artifact inventory')
    for name, digest in qualification['artifacts'].items():
        need(Path(name).name == name, 'qualification artifact basename')
        add_pins(pins, {str(folder / name): digest})
    need(len({c['name'] for c in qualification['commands']}) == len(qualification['commands']),
         'unique qualification commands')
    for command in qualification['commands']:
        name = command['name']
        command_binding(folder, command, name + '.stdout', name + '.stderr', pins)
    return dict(private_receipt=str(Path(path).resolve()), receipt_sha256=receipt['qualification_sha256'],
                schema=qualification['schema'], status=qualification['status'], summary=qualification['summary'],
                scope=qualification['scope'], build_receipt=qualification['build_receipt'],
                build_receipt_sha256=qualification['build_receipt_sha256'],
                source_count=len(qualification['sources_after']),
                source_map_sha256=map_digest(qualification['sources_after']),
                commands=qualification['commands'],
                publication_check='LIVE hashes and command bindings; no new geometry/oracle execution')


def validated_inputs(capture, post_path):
    receipt_path = capture / 'receipt.json'
    receipt, post = read(receipt_path), read(post_path)
    validate_grid(receipt)
    receipt_sha = sha(receipt_path)
    need(post['schema'] == AUDIT_SCHEMA and post['status'] == 'passed', 'passed post-capture audit required')
    need(Path(post['capture']).resolve() == capture and post['receipt_sha256'] == receipt_sha,
         'post-audit bound to this exact capture')
    need((post['weighted_rows'], post['comparator_rows'], post['total_rows'], post['measures']) == (104, 260, 364, 52),
         'complete post-audit scope')
    need(post['audit_source_sha256'] == sha(HERE / 'post_audit.py'), 'post-audit source pin')
    pins = {}
    add_pins(pins, {str(receipt_path): receipt_sha, str(post_path): sha(post_path),
                   str(HERE / 'post_audit.py'): post['audit_source_sha256'], str(Path(__file__).resolve()): sha(__file__)})
    add_pins(pins, post['pins_sha256'])
    closed_sources(receipt, pins)
    add_pins(pins, receipt['input_hashes'])
    add_pins(pins, receipt['artifacts'])
    for path_key, hash_key in (('native_binary', 'native_binary_sha256'), ('qualification', 'qualification_sha256'),
                              ('baseline_receipt', 'baseline_receipt_sha256'), ('manifest', 'manifest_sha256')):
        add_pins(pins, {receipt[path_key]: receipt[hash_key]})
    need(receipt['baseline_receipt_sha256'] == BASELINE_SHA and receipt['manifest_sha256'] == MANIFEST_SHA,
         'frozen comparator receipt and input manifest')
    baseline = read(receipt['baseline_receipt'])
    need(baseline['status'] == 'completed' and not baseline.get('failures'), 'completed inherited baseline')
    closed_sources(baseline, pins)
    manifest = {row['id']: row for row in read(receipt['manifest'])['cases']}
    for row in receipt['rows']:
        need(all(row[key] == manifest[row['case']][key] for key in
                 ('regime', 'communities', 'separation', 'seed', 'n')), 'manifest scene/seed binding')
    copied_fields = (*KEYS, 'metrics', 'extra')
    key = lambda row: tuple(row[field] for field in ('case', 'k', 'min_cluster_size', 'exp_z', 'method'))
    inherited = {key(row): {field: row[field] for field in copied_fields} for row in baseline['rows']}
    for row in receipt['rows']:
        if row['method'] != METHODS[0]:
            need(row == inherited[key(row)], 'copied comparator row changed')
    intent = capture / 'intent.json'
    initial = read(intent)
    for name in ('schema', 'plan', 'sources_before', 'native_binary_sha256', 'qualification_sha256',
                 'baseline_receipt_sha256', 'manifest_sha256'):
        need(initial[name] == receipt[name], 'capture intent binding: ' + name)
    add_pins(pins, {str(intent): sha(intent)})
    for command in receipt['commands']:
        case, k = command['case'], command['k']
        directory = capture / f'{case}_k{k}'
        expected = [receipt['native_binary'], '--input', manifest[case]['points_u32le'], '--k', str(k), '--workers', '1']
        need(command['argv'] == expected, 'native exact command binding')
        command_binding(directory, command, 'native.json', 'native.stderr', pins)
        intent = directory / 'intent.json'
        need(read(intent) == dict(argv=expected, binary_sha256=receipt['native_binary_sha256'],
                                 input_sha256=receipt['input_hashes'][manifest[case]['points_u32le']]),
             'native command intent binding')
        add_pins(pins, {str(intent): sha(intent)})
    qualification = qualify_metadata(receipt['qualification'], receipt, pins)
    check_pins(pins)
    return receipt, post, qualification, pins


def flatten(row):
    flat = {key: row[key] for key in KEYS}
    flat.update({key: row['metrics'][key] for key in METRICS})
    flat.update({key: row['extra'][key] for key in EXTRA})
    flat['threshold_unit'] = 'facet_mass_before_point_vote' if row['method'] == METHODS[0] else 'points'
    flat['inherited_comparator'] = row['method'] != METHODS[0]
    for key in ('near_threshold_nodes', 'vote_ties', 'naive_gabriel_components',
                'final_point_clusters_below_mass_threshold'):
        flat[key] = row.get(key)
    flat['final_point_sizes_json'] = json.dumps(row['final_point_sizes'], separators=(',', ':')) if 'final_point_sizes' in row else None
    for key in ('model', 'naive_mass_and_graph', 'full_attachment_tree', 'selection', 'vote'):
        flat[key + '_ms'] = row.get('times_ms', {}).get(key)
    for key, value in flat.items():
        need(not isinstance(value, float) or math.isfinite(value), 'finite published field: ' + key)
    return flat


def aggregates(rows):
    groups = {}
    for row in rows:
        groups.setdefault(tuple(row[key] for key in GROUP_KEYS), []).append(row)
    result = []
    measured = (*METRICS, *EXTRA, 'near_threshold_nodes', 'vote_ties', 'naive_gabriel_components',
                'final_point_clusters_below_mass_threshold')
    for key, group in sorted(groups.items()):
        seeds = {row['seed'] for row in group}
        need(len(seeds) == len(group) == (3 if key[0] == 'spherical' else 2),
             'complete seed group')
        aggregate = dict(zip(GROUP_KEYS, key), repetitions=len(group))
        for metric in measured:
            values = [row[metric] for row in group if row[metric] is not None]
            aggregate[metric + '_defined'] = len(values)
            aggregate[metric + '_mean'] = statistics.mean(values) if values else None
            aggregate[metric + '_sd'] = statistics.stdev(values) if len(values) > 1 else (0.0 if values else None)
        result.append(aggregate)
    need(len(result) == 140, 'all 140 method/parameter/regime aggregates')
    return result


def tables(rows):
    lookup = {tuple(row[key] for key in GROUP_KEYS): row for row in rows}
    titles = {'spherical': 'Sphérique', 'anisotropic': 'Anisotrope', 'unbalanced': 'Déséquilibré'}
    text = ['# Pilote pondéré FULL — tableaux prédéfinis', '',
            '1 200 points par scène ; K=5, seuil=20. Moyenne ± écart-type entre graines : '
            '3 sphériques, 2 anisotropes/déséquilibrées. Ce ne sont pas des intervalles de confiance.', '',
            'Le seuil porte sur la masse des facettes avant vote pour HGP pondéré, '
            'sur les points pour les comparateurs. Aucun filtre de taille n’est appliqué après le vote. '
            'La racine est exclue de l’EOM commun. HDBSCAN standard conserve sa voie propre, uniquement en z1.', '',
            'Les cinq régimes ont été choisis après la campagne précédente : diagnostic connu, '
            'pas test tenu à l’écart ni preuve de supériorité. Les 364 lignes, dont K10 et seuil50, '
            'restent dans [rows.csv](rows.csv) ; les 140 agrégats dans [aggregates.csv](aggregates.csv).']
    for z in (1, 2):
        methods = (*METHODS, 'hdbscan_standard') if z == 1 else METHODS
        headings = ['HGP pondéré FULL', 'Première couverture', 'HDBSCAN EOM commun']
        if z == 1:
            headings.append('HDBSCAN standard z1')
        text.extend(['', f'## expZ={z}', '',
                     'Chaque cellule : **ARI moyenne ± écart-type** ; F1 macro apparié ; couverture ; nombre de groupes.', '',
                     '| Régime | ' + ' | '.join(headings) + ' |',
                     '|---|' + '---:|' * len(methods)])
        for regime, g, d in REGIMES:
            cells = []
            for method in methods:
                row = lookup[(regime, g, d, 5, 20, z, method)]
                cells.append(f"{row['ari_all_mean']:.4f} ± {row['ari_all_sd']:.4f} ; "
                             f"{row['matched_macro_f1_mean']:.4f} ; {100*row['coverage_mean']:.1f}% ; {row['clusters_mean']:.2f}")
            text.append(f'| {titles[regime]} G{g}, δ{d} | ' + ' | '.join(cells) + ' |')
    text.extend(['', '## Portée de la preuve', '',
                 'Le contre-audit recalcule les ARI par contingences entières/Fraction, '
                 'les votes, les masses et les scores par classe. Le solveur Hungarian SciPy reste partagé ; '
                 'le NMI n’est pas recalculé. Les métriques non définies restent vides dans le CSV ; '
                 'chaque agrégat publie son nombre de valeurs définies.', '',
                 'Les masses, rayons de sélection et votes sont un post-traitement flottant non certifié. '
                 'Les avertissements près des seuils et les égalités de votes sont conservés dans le CSV. '
                 'Le vote final ne fournit pas à lui seul une famille emboîtée de partitions de points. '
                 'Les références privées, hashes LIVE, commandes et qualifications sont liés dans [receipt.json](receipt.json).', ''])
    return '\n'.join(text)


def write_csv(path, rows):
    keys = list(rows[0])
    need(all(list(row) == keys for row in rows), 'uniform CSV columns')
    with path.open('x', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=keys, lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)


def publish(capture, post_path, output):
    need(not output.exists(), 'NEW publication directory required')
    receipt, post, qualification, pins = validated_inputs(capture, post_path)
    flat = [flatten(row) for row in sorted(receipt['rows'], key=lambda row: tuple(row[key] for key in KEYS))]
    aggregate = aggregates(flat)
    output.mkdir(parents=True, exist_ok=False)
    write_csv(output / 'rows.csv', flat)
    write_csv(output / 'aggregates.csv', aggregate)
    with (output / 'TABLES.md').open('x') as stream:
        stream.write(tables(aggregate))
    summary = dict(schema='mhgp9_weighted_full_publication_v1', status='completed',
        private_capture=str(capture), private_receipt=str(capture / 'receipt.json'),
        receipt_sha256=sha(capture / 'receipt.json'), source_sha256=sha(__file__),
        row_count=len(flat), aggregate_count=len(aggregate), command_count=len(receipt['commands']),
        plan=receipt['plan'], scope=receipt['scope'], root_policy=receipt['root_policy'],
        approximate_postprocessing=True, selected_subset_after_previous_scores=True, note=receipt['note'],
        elapsed_seconds=receipt['elapsed_seconds'], native_binary=receipt['native_binary'],
        native_binary_sha256=receipt['native_binary_sha256'], sources=receipt['sources_after'],
        commands=receipt['commands'], baseline_receipt=receipt['baseline_receipt'], baseline_receipt_sha256=BASELINE_SHA,
        input_manifest=receipt['manifest'], input_manifest_sha256=MANIFEST_SHA,
        qualification=qualification,
        post_audit=dict(private_receipt=str(post_path), receipt_sha256=sha(post_path),
                        **{key: value for key, value in post.items() if key not in
                           ('capture', 'receipt_sha256', 'pins_sha256', 'exact_ARI')}),
        live_pins_checked=len(pins), live_pin_map_sha256=map_digest(pins),
        live_before_and_after=True, raw_clouds_or_native_arrays_published=False,
        fits_or_geometry_rerun=False, GCP_used=False, GPU_used=False,
        files={name: sha(output / name) for name in ('rows.csv', 'aggregates.csv', 'TABLES.md')})
    check_pins(pins)
    save(output / 'receipt.json', summary)
    print(json.dumps(dict(status='published', rows=len(flat), aggregates=len(aggregate),
                          output=str(output), GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', required=True, type=Path)
    parser.add_argument('--post-audit', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    publish(args.capture.resolve(), args.post_audit.resolve(), args.output.resolve())
