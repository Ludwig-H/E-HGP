"""Orchestrateur de l'oracle differentiel du journal FULL date.

  python3 -B run_oracle.py --bridge build/bridge --out <dir> [--valid 320] [--invalid 320]

Ecrit corpus/<name>.json, bridge_out/<name>.json, puis imprime un rapport JSON
deterministe (sans horodatage ni chemin absolu) sur stdout. Le meme rapport
doit sortir octet pour octet sous `python3 -B` et `python3 -B -O`.
Aucun assert. Sortie code 0 si aucune divergence, 1 sinon, 2 sur refus.
"""

import argparse
import json
import os
import subprocess
import sys

import generator
import journal_model
from geometry_journals import geometry_corpus


class Refusal(Exception):
    pass


def refuse(condition, reason):
    if not condition:
        raise Refusal(reason)


def dump(path, data):
    with open(path, 'w') as handle:
        json.dump(data, handle, sort_keys=True, separators=(',', ':'))
        handle.write('\n')


def run_bridge(bridge, journal_path, out_path):
    result = subprocess.run([bridge, journal_path], capture_output=True, text=True)
    with open(out_path, 'w') as handle:
        handle.write(result.stdout)
    refuse(result.returncode in (0, 2, 3), 'bridge_signal_or_unknown_code')
    parsed = json.loads(result.stdout) if result.stdout.strip() else {}
    return result.returncode, parsed


def canonical_cut(cut):
    """Forme comparable d'une coupe rendue par le pont ou le modele."""
    return dict(level=cut['level'], closed=cut['closed'], origin=cut['origin'],
                nodes=[dict(id=row['id'], root=row['root'], live=row['live'],
                            read=dict(status=row['read']['status'], reason=row['read']['reason'],
                                      values=list(row['read']['values']))) for row in cut['nodes']])


def compare_valid(product, expected):
    """Divergences entre la sortie produit et le modele sur un journal valide."""
    divergences = []
    if product.get('bank', {}).get('status') != 'ok':
        divergences.append(dict(kind='bank_refused_valid_journal', product=product.get('bank')))
        return divergences
    if product.get('build', {}).get('status') != 'ok':
        divergences.append(dict(kind='build_refused_valid_journal', product=product.get('build')))
        return divergences
    if product['build'].get('order') != expected['build']['order']:
        divergences.append(dict(kind='order_mismatch', product=product['build'], expected=expected['build']))
    if product['nodes'] != expected['nodes']:
        divergences.append(dict(kind='nodes_mismatch', product=product['nodes'], expected=expected['nodes']))
    if product['contributions'] != expected['contributions']:
        divergences.append(dict(kind='contributions_mismatch', product=product['contributions'],
                                expected=expected['contributions']))
    if len(product['cuts']) != len(expected['cuts']):
        divergences.append(dict(kind='cut_count_mismatch', product=len(product['cuts']),
                                expected=len(expected['cuts'])))
        return divergences
    for p_cut, e_cut in zip(product['cuts'], expected['cuts']):
        p_c, e_c = canonical_cut(p_cut), canonical_cut(e_cut)
        if p_c != e_c:
            details = []
            for p_row, e_row in zip(p_c['nodes'], e_c['nodes']):
                if p_row != e_row:
                    details.append(dict(product=p_row, expected=e_row))
            divergences.append(dict(kind='cut_mismatch', level=e_c['level'], closed=e_c['closed'],
                                    origin=e_c['origin'], rows=details[:5], row_count=len(details)))
    return divergences


def evaluate_valid(bridge, journal, corpus_dir, out_dir, stats):
    path = os.path.join(corpus_dir, journal['name'] + '.json')
    dump(path, journal)
    code, product = run_bridge(bridge, path, os.path.join(out_dir, journal['name'] + '.json'))
    expected = journal_model.evaluate(journal)
    refuse(expected['build']['status'] == 'ok', 'generator_produced_invalid_journal:' + journal['name'] +
           ':' + ','.join(expected['build'].get('reasons', [])))
    divergences = [] if code == 0 else [dict(kind='bridge_code', code=code, product=product)]
    if code == 0:
        divergences = compare_valid(product, expected)
    stats['valid_journals'] += 1
    stats['nodes'] += len(expected['nodes'])
    stats['contributions'] += len(expected['contributions'])
    stats['cuts'] += len(expected['cuts'])
    stats['node_cut_evaluations'] += sum(len(cut['nodes']) for cut in expected['cuts'])
    stats['live_reads_ok'] += sum(1 for cut in expected['cuts'] for row in cut['nodes']
                                  if row['read']['status'] == 'ok')
    stats['max_parents'] = max([stats['max_parents']] + [len(n['parents']) for n in expected['nodes']])
    chain = 0
    for node in expected['nodes']:
        length, current = 0, node['id']
        while expected['nodes'][current]['successor'] is not None:
            current = expected['nodes'][current]['successor']
            length += 1
        chain = max(chain, length)
    stats['max_successor_chain'] = max(stats['max_successor_chain'], chain)
    stats['journals_with_chain_ge3'] += 1 if chain >= 3 else 0
    stats['k1_journals'] += 1 if journal['order'] == 1 else 0
    stats['continuations'] += sum(1 for b in journal['batches'] for a in b['actions'] if len(a['parents']) == 1)
    stats['births'] += sum(1 for b in journal['batches'] for a in b['actions'] if not a['parents'])
    stats['multifusions'] += sum(1 for b in journal['batches'] for a in b['actions'] if len(a['parents']) >= 2)
    stats['mask16_populations'] += sum(1 for r in journal['populations'] if len(r['shell']) == 16)
    stats['multi_limb_levels'] += sum(1 for b in journal['batches'] if int(b['level'][0]) >= (1 << 64))
    stats['nonreduced_levels'] += sum(1 for b in journal['batches']
                                      if journal_model.parse_level(b['level']) is not None and
                                      int(b['level'][1]) != journal_model.parse_level(b['level']).denominator)
    profile = journal.get('profile', 'geometry')
    stats['profiles'][profile] = stats['profiles'].get(profile, 0) + 1
    return divergences


def evaluate_invalid(bridge, journal, corpus_dir, out_dir, stats, classes):
    path = os.path.join(corpus_dir, journal['name'] + '.json')
    dump(path, journal)
    code, product = run_bridge(bridge, path, os.path.join(out_dir, journal['name'] + '.json'))
    expected = journal_model.evaluate(journal)
    refuse(expected['build']['status'] != 'ok', 'mutation_left_journal_valid:' + journal['name'])
    reasons = expected['build']['reasons']
    divergences = []
    stats['invalid_journals'] += 1
    if code != 0:
        divergences.append(dict(kind='bridge_code_on_invalid', code=code, product=product))
        return divergences, None
    bank_ok = product['bank']['status'] == 'ok'
    if bool(expected.get('bank_codes')) == bank_ok:
        divergences.append(dict(kind='bank_verdict_mismatch', product=product['bank'],
                                expected_codes=expected.get('bank_codes')))
    if expected.get('bank_codes') and product['bank']['reason'] != journal_model.BANK_INVALID:
        divergences.append(dict(kind='bank_reason_mismatch', product=product['bank']))
    if product['build']['status'] != 'invalid_input':
        divergences.append(dict(kind='product_accepted_invalid_journal', product=product['build'],
                                expected_reasons=reasons))
        return divergences, None
    if not product['build']['empty']:
        divergences.append(dict(kind='refusal_left_nonempty_forest', product=product['build']))
    reason = product['build']['reason']
    if reason not in reasons:
        divergences.append(dict(kind='refusal_reason_outside_model_set', product=reason, expected_reasons=reasons))
    # une mutation de banque se voit au niveau banque (coverage_invalid_population)
    # et rend la banque nulle : le certificat refuse alors coverage_invalid_domain
    expected_build = (journal_model.INVALID_DOMAIN if journal['expected_reason'] == journal_model.BANK_INVALID
                      else journal['expected_reason'])
    primary_match = reason == expected_build
    if journal['expected_reason'] == journal_model.BANK_INVALID:
        primary_match = primary_match and product['bank']['reason'] == journal_model.BANK_INVALID
    record = classes.setdefault(journal['mutation'], dict(count=0, product_reasons={}, primary_expected=
                                                          journal['expected_reason'], primary_matches=0))
    record['count'] += 1
    observed = reason if journal['expected_reason'] != journal_model.BANK_INVALID else \
        product['bank']['reason'] + '+' + reason
    record['product_reasons'][observed] = record['product_reasons'].get(observed, 0) + 1
    record['primary_matches'] += 1 if primary_match else 0
    if not primary_match:
        return divergences, dict(journal=journal['name'], mutation=journal['mutation'],
                                 product=reason, primary=expected_build, model_set=reasons)
    return divergences, None


def evaluate_geometry(bridge, journal, expected_geometry, corpus_dir, out_dir, stats):
    divergences = evaluate_valid(bridge, journal, corpus_dir, out_dir, stats)
    with open(os.path.join(out_dir, journal['name'] + '.json')) as handle:
        product = json.load(handle)
    if product.get('build', {}).get('status') != 'ok':
        return divergences
    by_level = {}
    for cut in product['cuts']:
        by_level[(tuple(cut['level']), cut['closed'])] = cut
    for exp in expected_geometry['cuts']:
        key = (tuple(exp['level']), exp['closed'])
        refuse(key in by_level, 'geometry_cut_missing_in_bridge_output')
        cut = by_level[key]
        live = {row['id']: row['read']['values'] for row in cut['nodes']
                if row['live'] and row['id'] is not None and row['id'] < len(product['nodes'])}
        product_multiset = sorted(sorted(v) for v in live.values())
        if product_multiset != exp['gamma']:
            divergences.append(dict(kind='gamma_coverage_multiset_mismatch', level=exp['level'],
                                    closed=exp['closed'], product=product_multiset, gamma=exp['gamma']))
        product_by_id = {str(i): v for i, v in live.items()}
        if product_by_id != exp['model_read']:
            divergences.append(dict(kind='model_read_by_id_mismatch', level=exp['level'], closed=exp['closed'],
                                    product=product_by_id, model=exp['model_read']))
        stats['geometry_cuts'] += 1
        stats['geometry_components'] += len(exp['gamma'])
    stats['geometry_journals'] += 1
    return divergences


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bridge', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--valid', type=int, default=320)
    parser.add_argument('--invalid', type=int, default=320)
    args = parser.parse_args()
    corpus_dir = os.path.join(args.out, 'corpus')
    out_dir = os.path.join(args.out, 'bridge_out')
    os.makedirs(corpus_dir, exist_ok=True)
    os.makedirs(out_dir, exist_ok=True)
    stats = dict(valid_journals=0, invalid_journals=0, nodes=0, contributions=0, cuts=0,
                 node_cut_evaluations=0, live_reads_ok=0, max_parents=0, max_successor_chain=0,
                 journals_with_chain_ge3=0, k1_journals=0, continuations=0, births=0, multifusions=0,
                 mask16_populations=0, multi_limb_levels=0, nonreduced_levels=0, profiles={},
                 geometry_journals=0, geometry_cuts=0, geometry_components=0)
    divergences = []
    classes = {}
    primary_mismatches = []
    valid_journals = []
    for seed in range(args.valid):
        journal = generator.make_valid(seed)
        valid_journals.append(journal)
        for d in evaluate_valid(args.bridge, journal, corpus_dir, out_dir, stats):
            divergences.append(dict(journal=journal['name'], **d))
    for seed in range(args.invalid):
        base = valid_journals[seed % len(valid_journals)]
        journal = generator.make_invalid(seed, base)
        found, mismatch = evaluate_invalid(args.bridge, journal, corpus_dir, out_dir, stats, classes)
        for d in found:
            divergences.append(dict(journal=journal['name'], **d))
        if mismatch:
            primary_mismatches.append(mismatch)
    geometry_divergences = []
    for journal, expected in geometry_corpus():
        for d in evaluate_geometry(args.bridge, journal, expected, corpus_dir, out_dir, stats):
            geometry_divergences.append(dict(journal=journal['name'], **d))
    reason_classes = {}
    for mutation, record in classes.items():
        for reason, count in record['product_reasons'].items():
            reason_classes[reason] = reason_classes.get(reason, 0) + count
    report = dict(
        schema='oracle_differentiel_full_dated_coverage_v1',
        product_header='morsehgp3D_v7/src/forest/full_coverage_certificate.hpp',
        stats=stats,
        rejection_classes_by_mutation=classes,
        rejection_reasons_covered=reason_classes,
        distinct_product_reasons=len(reason_classes),
        primary_reason_mismatches=primary_mismatches,
        divergences=divergences,
        geometry_divergences=geometry_divergences,
        verdict='no_divergence' if not divergences and not geometry_divergences else 'divergences_found',
    )
    print(json.dumps(report, sort_keys=True, separators=(',', ':')))
    return 1 if divergences or geometry_divergences else 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Refusal as error:
        print(json.dumps(dict(refused=str(error)), sort_keys=True))
        sys.exit(2)
