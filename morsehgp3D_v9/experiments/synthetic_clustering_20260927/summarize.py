#!/usr/bin/env python3
"""Compact, complete publication of the predeclared synthetic quality grid.

This formatter checks receipt/manifest structure and small source bindings. It
does not read label payloads, recompute metrics, rerun fits or rehash huge native
artifacts. A separately supplied post-audit is bound by the capture receipt SHA.
Without it the output explicitly remains a structural, non-counter-audited
summary. Existing captures and publications are never overwritten.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics

import plan

SCHEMA = 'mhgp9_synthetic_clustering_summary_v1'
CAPTURE_SCHEMA = 'mhgp9_variable_synthetic_quality_capture_v1'
AUDIT_SCHEMA = 'mhgp9_synthetic_clustering_post_audit_v1'
METHODS = tuple(plan.PLAN['quality']['methods'])
ROUTED, HDB = METHODS[0], 'hdbscan_common'
KEYS = ('case', 'k', 'min_cluster_size', 'exp_z', 'method')
GROUP_KEYS = ('axis', 'scenario', 'family', 'n', 'groups', 'separation',
              'noise_fraction', 'k', 'min_cluster_size', 'exp_z', 'method')
METRICS = ('ari_all', 'nmi_all', 'ari_true_inliers', 'ari_inliers_noise_singletons',
           'ari_classified', 'coverage', 'clusters', 'noise_count',
           'noise_precision', 'noise_recall', 'noise_f1')
EXTRAS = ('matched_macro_precision', 'matched_macro_recall', 'matched_macro_f1',
          'matched_micro_precision', 'matched_micro_recall', 'matched_micro_f1',
          'exact_classes', 'exact_class_fraction', 'matched_correct_points',
          'cluster_count_error', 'cluster_count_absolute_error')
AGG_METRICS = METRICS + EXTRAS + ('noise_fraction_predicted', 'selection_ms')
PAIR_METRICS = ('ari_all', 'ari_inliers_noise_singletons', 'coverage',
                'matched_macro_f1', 'clusters', 'noise_count',
                'noise_fraction_predicted', 'noise_precision', 'noise_recall', 'noise_f1')


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def check_pins(pins):
    for path, expected in pins.items():
        need(sha(path) == expected, 'summary input/source changed: '+path)


def expected_grid():
    quality = plan.PLAN['quality']
    return {(case['id'], k, minimum, z, method)
            for case in plan.PLAN['cases'] if case['phase'] == 'quality'
            for k in quality['k'] for minimum in quality['min_cluster_size']
            for method in quality['methods']
            for z in (quality['standard_exp_z'] if method == 'hdbscan_standard' else quality['exp_z'])}


def row_key(row):
    for name in ('k', 'min_cluster_size', 'exp_z'):
        need(type(row[name]) is int, 'integer selection parameter required')
    return tuple(row[key] for key in KEYS)


def numeric(value, name, *, nullable=False):
    if value is None and nullable:
        return
    need(type(value) in (int, float) and math.isfinite(value), 'finite metric: '+name)


def validate_inputs(receipt_path, audit_path=None):
    """Small structural bindings, NOT a replacement for post_audit.py."""
    receipt_path = Path(receipt_path).resolve()
    pins = {str(receipt_path): sha(receipt_path),
            str(Path(__file__).resolve()): sha(__file__),
            str(Path(plan.__file__).resolve()): sha(plan.__file__)}
    receipt = read(receipt_path)
    need(receipt.get('schema') == CAPTURE_SCHEMA and receipt.get('status') == 'completed',
         'completed synthetic quality capture required')
    need(receipt['plan'] == plan.PLAN, 'exact predeclared plan required')
    need(receipt.get('GCP_used') is False and receipt.get('GPU_used') is False and
         receipt.get('growth_executed') is False, 'CPU quality-only scope')
    need(type(receipt['workers']) is int and 1 <= receipt['workers'] <= 2, 'one or two scene workers')
    need(not receipt.get('closure_errors') and not receipt.get('cleanup_errors'), 'capture closure errors')
    need(receipt['sources_before'] and receipt['sources_before'] == receipt['sources_after'], 'capture source closure')
    plan_path = str(Path(plan.__file__).resolve())
    need(receipt['sources_after'].get(plan_path) == pins[plan_path], 'captured/current plan source binding')
    manifest_path = Path(receipt['manifest']).resolve()
    need(sha(manifest_path) == receipt['manifest_sha256'], 'manifest hash binding')
    pins[str(manifest_path)] = receipt['manifest_sha256']
    manifest = read(manifest_path)
    need(manifest['schema'] == 'mhgp9_synthetic_cluster_input_manifest_v1' and
         manifest['plan'] == plan.PLAN, 'prepared manifest plan/schema')
    need([case['id'] for case in manifest['cases']] == [case['id'] for case in plan.PLAN['cases']],
         'all 34 quality and 9 growth inputs, exact order')
    cases = {}
    for case, planned in zip(manifest['cases'], plan.PLAN['cases']):
        need(all(case.get(key) == value for key, value in planned.items()), 'case metadata differs from plan')
        need(case['n'] == planned['spec']['n'] and case['groups'] == planned['spec']['groups'], 'case dimensions')
        counts = case['true_counts']
        need(isinstance(counts, dict) and all(type(v) is int and v > 0 for v in counts.values()), 'positive class counts')
        allowed = {str(i) for i in range(1, case['groups']+1)}
        if case['spec']['noise_fraction']:
            allowed.add('-1')
        need(set(counts) == allowed and sum(counts.values()) == case['n'], 'whole input true counts')
        cases[case['id']] = case
    quality_ids = [case['id'] for case in plan.PLAN['cases'] if case['phase'] == 'quality']
    units = receipt['units']
    need(len(units) == 34 and [unit['case'] for unit in units] == quality_ids, '34 unique ordered units')
    grid = expected_grid()
    for unit in units:
        case = cases[unit['case']]
        need(unit.get('schema') == 'mhgp9_synthetic_clustering_unit_v1' and unit.get('status') == 'completed'
             and unit['n'] == case['n'] and unit['k'] == 5, 'completed bound unit')
        need(unit['sources_before'] and unit['sources_before'] == unit['sources_after'], 'unit source closure')
        need(len(unit['commands']) == 1 and unit['commands'][0]['returncode'] == 0, 'one successful native command per unit')
        actual = [row_key(row) for row in unit['rows']]
        need(len(actual) == 18 and set(actual) == {key for key in grid if key[0] == case['id']}, 'unit 18-row grid')
    rows = receipt['rows']
    need(rows == [row for unit in units for row in unit['rows']], 'receipt/unit rows differ')
    keys = [row_key(row) for row in rows]
    need(len(keys) == 612 and set(keys) == grid, 'exact unique 612-row grid')
    for row in rows:
        case = cases[row['case']]
        need(all(row.get(key) == case[key] for key in ('phase', 'axis', 'replicate', 'spec', 'n', 'groups')), 'row/case metadata')
        need(all(row.get(key) == case['spec'][key] for key in ('family', 'groups', 'separation', 'seed', 'noise_fraction')),
             'row/spec metadata')
        threshold = 'facet_mass_before_vote' if row['method'] == 'hgp_weighted_full_vote' else 'point_cardinality'
        need(row['threshold_semantics'] == threshold, 'threshold semantics')
        for name in METRICS:
            numeric(row['metrics'][name], name, nullable=name not in ('ari_all', 'nmi_all', 'coverage', 'clusters', 'noise_count'))
        for name in EXTRAS:
            numeric(row['extra'][name], name)
        for name in ('ari_all', 'ari_true_inliers', 'ari_inliers_noise_singletons', 'ari_classified'):
            value = row['metrics'][name]
            need(value is None or -1 <= value <= 1, 'ARI range')
        need(0 <= row['metrics']['coverage'] <= 1 and 0 <= row['extra']['matched_macro_f1'] <= 1, 'coverage/F1 range')
        for name in ('noise_count', 'clusters'):
            value = row['metrics'][name]
            need(type(value) is int and 0 <= value <= case['n'], 'integer count range')
        need(math.isclose(row['metrics']['coverage'], 1-row['metrics']['noise_count']/case['n'], abs_tol=1e-12), 'coverage/noise consistency')
        numeric(row['selection_ms'], 'selection_ms'); need(row['selection_ms'] >= 0, 'nonnegative selection time')
    audit_binding = None
    if audit_path is not None:
        audit_path = Path(audit_path).resolve(); audit = read(audit_path)
        need(audit.get('schema') == AUDIT_SCHEMA and audit.get('status') == 'passed', 'passed separate post-audit required')
        need(Path(audit['receipt']).resolve() == receipt_path and audit['receipt_sha256'] == pins[str(receipt_path)], 'post-audit capture binding')
        for name, expected in dict(rows=612, point_trees=68, point_selection_replays=136, common_condensed_replays=272,
                                   hdbscan_fits_checked=68, worker_commands=34, native_commands=34,
                                   fraction_ari_replays=612).items():
            need(audit.get(name) == expected, 'post-audit coverage: '+name)
        pins[str(audit_path)] = sha(audit_path)
        audit_binding = dict(path=str(audit_path), sha256=pins[str(audit_path)], schema=AUDIT_SCHEMA,
            status='passed', metrics_recomputed_by_this_formatter=False,
            full_live_rehash_by_this_formatter=False, fraction_ari_replays=612,
            NMI_recomputed=audit.get('NMI_recomputed'), Hungarian_solver_shared=audit.get('Hungarian_solver_shared'))
    check_pins(pins)
    return receipt, cases, pins, audit_binding


def flatten(row, case):
    fields = ('case', 'axis', 'family', 'n', 'groups', 'separation', 'noise_fraction',
              'seed', 'replicate', 'k', 'min_cluster_size', 'exp_z', 'method', 'threshold_semantics')
    result = {key: row[key] for key in fields}
    result['scenario'] = row['case'].rsplit('_s', 1)[0]
    sizes = [count for label, count in case['true_counts'].items() if label != '-1']
    result.update(true_class_min_size=min(sizes), true_class_max_size=max(sizes),
        true_noise_count=case['true_counts'].get('-1', 0),
        true_classes_below_m=sum(size < row['min_cluster_size'] for size in sizes),
        below_m_is_cardinality_constraint=row['threshold_semantics'] == 'point_cardinality',
        true_counts_json=json.dumps(case['true_counts'], sort_keys=True, separators=(',', ':')),
        labels_payload_sha256=row['labels_payload_sha256'], selection_ms=row['selection_ms'])
    result.update({key: row['metrics'][key] for key in METRICS})
    result.update({key: row['extra'][key] for key in EXTRAS})
    result['noise_fraction_predicted'] = result['noise_count']/result['n']
    return result


def aggregates(rows):
    groups = defaultdict(list)
    for row in rows:
        groups[tuple(row[key] for key in GROUP_KEYS)].append(row)
    output = []
    for key, values in sorted(groups.items()):
        need(len(values) == 2 and {row['seed'] for row in values} == set(plan.QUALITY_SEEDS), 'two prescribed seeds per aggregate')
        row = dict(zip(GROUP_KEYS, key)); row['replicates'] = 2
        row['true_classes_below_m_min'] = min(value['true_classes_below_m'] for value in values)
        row['true_classes_below_m_max'] = max(value['true_classes_below_m'] for value in values)
        for name in AGG_METRICS:
            observed = [value[name] for value in values if value[name] is not None]
            row[name+'_count'] = len(observed)
            row[name+'_mean'] = statistics.mean(observed) if observed else None
            row[name+'_min'] = min(observed) if observed else None
            row[name+'_max'] = max(observed) if observed else None
        output.append(row)
    need(len(output) == 306, '306 complete scenario/parameter/method aggregates')
    return output


def comparisons(rows):
    index = {row_key(row): row for row in rows}
    output = []
    for key, routed in sorted(index.items()):
        if routed['method'] != ROUTED:
            continue
        baseline = index[key[:-1]+(HDB,)]
        row = {name: routed[name] for name in ('case', 'axis', 'scenario', 'family', 'n', 'groups',
               'separation', 'noise_fraction', 'seed', 'replicate', 'k', 'min_cluster_size', 'exp_z',
               'true_class_min_size', 'true_classes_below_m')}
        row.update(method=ROUTED, baseline=HDB, delta_convention='routed_minus_hdbscan_common')
        for metric in PAIR_METRICS:
            a, b = routed[metric], baseline[metric]
            row['routed_'+metric] = a; row['hdbscan_'+metric] = b
            row['delta_'+metric] = a-b if a is not None and b is not None else None
        output.append(row)
    need(len(output) == 136, '136 paired same-m/same-z comparisons')
    return output


def size_diagnostics(receipt, cases):
    """Recorded size-axis work only; no geometry invocation or complexity fit."""
    output = []
    for unit in receipt['units']:
        case = cases[unit['case']]
        if case['phase'] != 'quality' or case['axis'] != 'size':
            continue
        need([part['exp_z'] for part in unit['units']] == [1, 2], 'two size diagnostic exponents')
        for part in unit['units']:
            routing = part['routing_statistics']; points = part['point_tree_statistics']
            row = dict(case=case['id'], seed=case['spec']['seed'], replicate=case['replicate'],
                n=case['n'], k=5, exp_z=part['exp_z'],
                cofaces=unit['native_statistics']['cofaces'], facets=unit['attachment_statistics']['facets'],
                incidences=routing['incidences'], source_nodes=routing['source_nodes'],
                virtual_nodes=routing['virtual_nodes'], point_internal_nodes=points['internal_nodes'],
                point_output_nodes=points['point_leaves']+points['internal_nodes'],
                full_geometry_nodes=part['full_tree_statistics']['full_nodes'],
                native_command_wall_ms=unit['commands'][0]['wall_ms'],
                native_chain_wall_ms=unit['native_times_ms']['native_chain_wall'],
                native_chain_reported_ms=unit['native_times_ms']['native_chain_reported'],
                native_tower_ms=unit['native_times_ms']['native_tower'],
                native_read_and_validate_ms=unit['native_read_and_validate_ms'],
                first_coverage_ms=unit['first_coverage_ms'],
                measure_and_naive_graph_ms=part['times_ms']['measure_and_naive_graph'],
                full_facet_tree_ms=part['times_ms']['full_facet_tree'],
                routing_ms=part['times_ms']['routing_reference'],
                point_tree_and_validate_ms=part['times_ms']['point_tree_and_validate'],
                unit_elapsed_ms=1000*unit['elapsed_seconds'])
            fits = {fit['min_cluster_size']: fit['wall_ms'] for fit in unit['hdbscan_fits']}
            for minimum in (20, 50):
                row[f'hdbscan_fit_m{minimum}_ms'] = fits[minimum]
                for method in METHODS:
                    selections = [selection['selection_ms'] for selection in unit['rows']
                                  if selection['method'] == method and selection['min_cluster_size'] == minimum
                                  and selection['exp_z'] == part['exp_z']]
                    row[f'{method}_m{minimum}_selection_ms'] = selections[0] if selections else None
            output.append(row)
    output.sort(key=lambda row: (row['seed'], row['exp_z'], row['n']))
    need(len(output) == 12, '12 size diagnostics: 3 sizes, 2 seeds, 2 exponents')
    grouped = defaultdict(list)
    for row in output:
        grouped[row['seed'], row['exp_z']].append(row)
    numeric_fields = [key for key in output[0] if key not in ('case', 'seed', 'replicate', 'n', 'k', 'exp_z')]
    for values in grouped.values():
        need([row['n'] for row in values] == [400, 800, 1600], 'whole size diagnostic sequence')
        for i, row in enumerate(values):
            row['previous_n'] = values[i-1]['n'] if i else None
            for key in numeric_fields:
                value = row[key]
                if value is not None:
                    numeric(value, key); need(value >= 0, 'nonnegative work/time')
                denominator = values[i-1][key] if i else None
                row[key+'_ratio_to_previous_n'] = (value/denominator
                    if value is not None and denominator is not None and denominator > 0 else None)
    return output


def readme(receipt, rows, grouped, paired, size_rows, audit_binding):
    primary = [row for row in rows if row['min_cluster_size'] == 20 and row['exp_z'] == 1]
    index = {(row['case'], row['method']): row for row in primary}
    primary_pairs = [row for row in paired if row['min_cluster_size'] == 20 and row['exp_z'] == 1]
    wins = sum(row['delta_ari_all'] > 0 for row in primary_pairs)
    losses = sum(row['delta_ari_all'] < 0 for row in primary_pairs)
    mean_delta = statistics.mean(row['delta_ari_all'] for row in primary_pairs)
    state = ('Contre-audit séparé lié par le hash du reçu de capture ; ce formateur ne le réexécute pas.'
             if audit_binding else 'Résumé structurel non contre-audité : métriques recopiées, non recalculées ici.')
    lines = ['# Capture synthétique : toutes les scènes et tous les réglages', '',
        '34 scènes complètes, 17 scénarios × deux nouvelles graines, K5 ; 612 scores nouveaux. '+state, '',
        '[Scores bruts](scores.csv), [306 agrégats](aggregates.csv), [136 comparaisons appariées](comparisons.csv), '
        '[diagnostic des tailles](size_diagnostics.csv), '
        '[provenance et hashes](receipt.json). Aucun label ponctuel ni gros objet natif n’est recopié.', '',
        '## Profil principal préannoncé : K5 / m20 / z1', '',
        f'Routage moins HDBSCAN commun : ΔARI moyen {mean_delta:+.6f} sur les 34 scènes ; '
        f'{wins} gains, {losses} pertes, {34-wins-losses} égalités exactes des scores stockés. '
        'Ces comptes descriptifs ne sont ni un test de significativité ni une domination générale. '
        'Aucun meilleur seuil ou exposant n’est choisi par scène.', '',
        'ARI tous points ci-dessous. Couverture, bruit et F1 apparié figurent pour chaque scène dans les CSV ; '
        'le signe des deltas est toujours routage moins HDBSCAN commun (un bruit plus élevé n’est pas un gain).', '',
        '| Scène (n / G / δ / bruit / graine dans l’ID) | Routage | Vote massique | Première couverture | HDB commun | HDB standard |',
        '|---|---:|---:|---:|---:|---:|']
    for case in plan.PLAN['cases']:
        if case['phase'] == 'quality':
            values = [index[case['id'], method]['ari_all'] for method in METHODS]
            lines.append('| '+case['id']+' | '+' | '.join(f'{value:.4f}' for value in values)+' |')
    lines += ['', '## Moyennes par scénario : deux graines, même profil principal', '',
        '| Axe / famille / n / G / δ / bruit | Routage | Vote massique | Première couverture | HDB commun | HDB standard |',
        '|---|---:|---:|---:|---:|---:|']
    group_index = {(row['scenario'], row['method']): row for row in grouped
                   if row['min_cluster_size'] == 20 and row['exp_z'] == 1}
    seen = set()
    for row in primary:
        scenario = row['scenario']
        if scenario in seen:
            continue
        seen.add(scenario)
        label = f"{row['axis']} / {row['family']} / {row['n']} / {row['groups']} / {row['separation']} / {row['noise_fraction']}"
        values = [group_index[scenario, method]['ari_all_mean'] for method in METHODS]
        lines.append('| '+label+' | '+' | '.join(f'{value:.4f}' for value in values)+' |')
    lines += ['', '## Classes sous le seuil : conservées, jamais exclues', '',
        'Les effectifs viennent de `true_counts` du manifeste. Le seuil du vote massique porte sur les facettes, '
        'pas sur la cardinalité des groupes après vote. Les autres méthodes ont un seuil ponctuel. '
        'La condition de taille est nécessaire à la restitution exacte d’une classe, jamais suffisante.', '',
        '| Scène | Effectifs vrais (−1 : bruit injecté) | Classes <20 | Classes <50 |', '|---|---|---:|---:|']
    for case in plan.PLAN['cases']:
        if case['phase'] != 'quality':
            continue
        row = index[case['id'], ROUTED]
        counts = json.loads(row['true_counts_json']); sizes = [count for label, count in counts.items() if label != '-1']
        lines.append(f"| {case['id']} | {row['true_counts_json']} | {sum(s < 20 for s in sizes)} | {sum(s < 50 for s in sizes)} |")
    lines += ['', '## Taille 400 / 800 / 1600 : diagnostic distinct des scores', '',
        'Même famille sphérique/G8/δ4, grille commune et flux par composante. Deux graines, '
        'pas deux répétitions chronométriques. Les deux exposants et tous les temps de phase disponibles '
        'sont conservés dans `size_diagnostics.csv`, avec rapports au doublement pour chaque graine. '
        'Les compteurs et temps natifs/communs sont répétés sur les lignes z1/z2 : ne pas les additionner.', '',
        '| Graine / z / n | Cofaces | Facettes | Incidences | Nœuds source routage | Nœuds virtuels cumulés | Internes ponctuels | Routage ms |',
        '|---|---:|---:|---:|---:|---:|---:|---:|']
    for row in size_rows:
        lines.append(f"| {row['seed']} / {row['exp_z']} / {row['n']} | {row['cofaces']} | {row['facets']} | "
            f"{row['incidences']} | {row['source_nodes']} | {row['virtual_nodes']} | {row['point_internal_nodes']} | {row['routing_ms']:.3f} |")
    lines += ['', 'Ce sont des compteurs de sorties et du prototype de projection, pas un inventaire complet '
        'du travail géométrique. Les nœuds virtuels sont cumulés par point, pas le nombre de nœuds d’un '
        'seul arbre matérialisé. Des rapports inférieurs à4 ne prouveraient aucune borne sous-quadratique. '
        'Aucun 8k/16k/32k n’a été exécuté ici.', '', '## Sens des scores, coût et limites', '',
        'Le F1 macro apparié utilise un appariement hongrois un-à-un maximisant le nombre de points correctement '
        'appariés, pas le F1 lui-même. Les classes de bruit vrai sont exclues de cet appariement mais restent '
        'une contamination des groupes prédits. ARI tous points conserve −1 comme label ; '
        '`ari_inliers_noise_singletons` exclut le bruit vrai et traite chaque abstention prédite comme singleton. '
        'Les cellules vides des CSV signifient métrique non applicable, jamais zéro.', '',
        'Les labels vrais décrivent le mécanisme générateur, pas nécessairement des modes de densité identifiables. '
        'Deux graines nouvelles confirment ces scénarios préannoncés, pas toutes les distributions. '
        'm50 et z2 sont conservés intégralement dans les CSV, y compris les cas défavorables ; '
        'HDBSCAN standard n’a que z1. z modifie les poids et le routage HGP, ainsi que λ ; '
        'pour HDBSCAN commun il ne modifie que λ. Le catalogue Gabriel courant n’est pas une reproduction '
        'implicite du catalogue historique d’ordre-Voronoï.', '',
        f"Durée de capture : {receipt['elapsed_seconds']:.3f} s ; {receipt['workers']} workers de scènes. "
        'Les chronos concernent une chaîne CPU/Python/export de diagnostic partagée, pas une performance '
        'de production ni le temps du seul moteur FULL. `selection_ms` n’inclut ni toute la géométrie ni '
        'la préparation ; zéro pour HDBSCAN standard désigne une sélection déjà incluse dans son fit, '
        'pas un algorithme gratuit. Les sommes de durées concurrentes ne sont pas une durée murale.', '',
        'Neuf entrées de croissance 8k/16k/32k sont préparées mais NON exécutées par cette capture. '
        'Aucune borne sous-quadratique, qualification GPU/GCP ou latence de production ne découle de ce rapport. '
        'Le formateur ne lance aucun fit, calcul géométrique, routage ou EOM.', '']
    return '\n'.join(lines)


def write_csv(path, rows):
    with Path(path).open('x', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader(); writer.writerows(rows)


def publish(receipt_path, output, audit_path=None):
    output = Path(output).absolute()
    need(not output.exists(), 'NEW summary directory required')
    receipt_path = Path(receipt_path).resolve()
    receipt, cases, pins, audit_binding = validate_inputs(receipt_path, audit_path)
    rows = [flatten(row, cases[row['case']]) for row in sorted(receipt['rows'], key=row_key)]
    grouped = aggregates(rows); paired = comparisons(rows); size_rows = size_diagnostics(receipt, cases)
    document = readme(receipt, rows, grouped, paired, size_rows, audit_binding)
    output.mkdir(parents=True, exist_ok=False)
    write_csv(output/'scores.csv', rows); write_csv(output/'aggregates.csv', grouped)
    write_csv(output/'comparisons.csv', paired)
    write_csv(output/'size_diagnostics.csv', size_rows)
    with (output/'README.md').open('x') as stream:
        stream.write(document)
    check_pins(pins)
    summary = dict(schema=SCHEMA, status='completed', capture_receipt=str(receipt_path),
        capture_receipt_sha256=pins[str(receipt_path)], manifest=receipt['manifest'],
        manifest_sha256=receipt['manifest_sha256'], summary_inputs_sha256=pins,
        structural_validation_only=audit_binding is None, post_audit=audit_binding,
        rows=len(rows), aggregates=len(grouped), comparisons=len(paired), size_diagnostics=len(size_rows), quality_cases=34,
        growth_inputs_prepared=9, growth_executed=False, primary=plan.PLAN['quality']['primary'],
        sources_closed_in_capture=receipt['sources_after'], workers=receipt['workers'],
        elapsed_seconds=receipt['elapsed_seconds'], timing_scope=receipt['timing_scope'],
        metrics_recomputed=False, full_live_artifact_rehash=False, labels_copied=False,
        fits_or_geometry_rerun=False, GCP_used=False, GPU_used=False,
        statistical_dominance_claimed=False, production_performance_claimed=False,
        files={name: sha(output/name) for name in ('scores.csv', 'aggregates.csv', 'comparisons.csv', 'size_diagnostics.csv', 'README.md')})
    with (output/'receipt.json').open('x') as stream:
        json.dump(summary, stream, indent=2, sort_keys=True, allow_nan=False); stream.write('\n')
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--audit', type=Path)
    arguments = parser.parse_args()
    result = publish(arguments.receipt, arguments.output, arguments.audit)
    print(json.dumps({key: result[key] for key in ('status', 'rows', 'aggregates', 'comparisons', 'structural_validation_only')}))
