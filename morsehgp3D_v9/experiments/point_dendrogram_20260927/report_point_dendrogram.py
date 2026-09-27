#!/usr/bin/env python3
"""Publish every row of the closed routed-point pilot after LIVE readback.

No fit/geometry or EOM runs here. A separate reader verifies provenance,
point payloads and scores. New output only; sources checked at closure.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import csv
import hashlib
import json
from pathlib import Path
import statistics

import post_audit_point_dendrogram as reader

METHODS = ('hgp_exclusive_point_routing', 'hgp_weighted_full_vote',
           'hgp_first_coverage', 'hdbscan_common', 'hdbscan_standard')
REGIMES = (('spherical', 2, 8), ('spherical', 8, 4), ('spherical', 16, 2),
           ('anisotropic', 8, 4), ('unbalanced', 8, 4))
KEYS = ('case', 'regime', 'communities', 'separation', 'seed', 'n', 'k',
        'min_cluster_size', 'exp_z', 'method')
GROUPS = ('regime', 'communities', 'separation', 'k', 'min_cluster_size', 'exp_z', 'method')
METRICS = ('ari_all', 'ari_inliers_noise_singletons', 'coverage', 'clusters',
           'noise_count', 'matched_macro_f1')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def flatten(row):
    flat = {key: row[key] for key in KEYS}
    flat.update({key: row['metrics'][key] for key in METRICS if key != 'matched_macro_f1'})
    flat['matched_macro_f1'] = row['extra']['matched_macro_f1']
    flat['origin'] = 'new_point_tree_selection' if row['method'] == METHODS[0] else 'inherited_unchanged'
    return flat


def aggregates(rows):
    groups = defaultdict(list)
    for row in rows:
        groups[tuple(row[key] for key in GROUPS)].append(row)
    result = []
    for key, values in sorted(groups.items()):
        row = dict(zip(GROUPS, key)); row['cases'] = len(values)
        for metric in METRICS:
            row[metric+'_mean'] = statistics.mean(value[metric] for value in values)
            row[metric+'_min'] = min(value[metric] for value in values)
            row[metric+'_max'] = max(value[metric] for value in values)
        result.append(row)
    need(len(result) == 90, 'exact 90 regime/parameter/method aggregates')
    return result


def tables(aggregate):
    index = {(row['regime'], row['communities'], row['separation'], row['min_cluster_size'],
              row['exp_z'], row['method']): row for row in aggregate}
    text = '# Diagnostic gaussien : dendrogramme de points après FULL\n\n'
    text += ('13 scènes entières connues de 1 200 points, K5 ; 52 nouvelles sélections, '
             '182 résultats hérités inchangés. Aucun nouveau fit HDBSCAN, calcul géométrique '
             'ou GCP. Ce n’est pas une évaluation tenue à l’écart du développement.\n\n'
             'Le profil principal préannoncé est m20/z1. Tous les seuils et exposants '
             'figurent ci-dessous ; aucun meilleur réglage choisi par scène. Les valeurs '
             'sont des ARI moyens tous points, bruit inclus. Trois graines par régime '
             'sphérique, deux par stress. CSV : détails par scène, ARI bruit en singletons, '
             'couverture, groupes, F1 macro apparié, minima et maxima.\n\n')
    for z in (1, 2):
        for minimum in (20, 50):
            text += f'## expZ={z}, seuil={minimum}\n\n'
            text += '| Régime | Routage exclusif | Vote pondéré | Première couverture | HDBSCAN commun |'
            if z == 1:
                text += ' HDBSCAN standard |'
            text += '\n|---|' + '---:|'*(5 if z == 1 else 4) + '\n'
            for regime, g, separation in REGIMES:
                label = f'{regime}, G{g}, δ{separation}'
                methods = METHODS if z == 1 else METHODS[:-1]
                values = [index[regime, g, separation, minimum, z, method]['ari_all_mean'] for method in methods]
                text += '| '+label+' | '+' | '.join(f'{value:.4f}' for value in values)+' |\n'
            text += '\n'
    text += ('## Interprétation et limites\n\n'
             'Le routage exclusif fixe une branche par point avant condensation et compte '
             'des points unitaires. Le vote pondéré condense des masses de facettes puis '
             'vote entre groupes sélectionnés. Le seuil a donc un sens différent ; les '
             'deux variantes ne sont pas des reproductions identiques. La première '
             'couverture reste une autre baseline, conservée sans modification.\n\n'
             'Les dates géométriques de l’arbre sont rationnelles. Les scores Sτ repris '
             'sont des arrondis binary64, relevés exactement en rationnels dyadiques '
             'pour le routage ; cela vaut aussi en z2. EOM reste binary64. Les racines '
             'sont exclues pour les deux méthodes comparées par EOM commun ; aucun '
             'remplissage du bruit. Changer z modifie le routage et λ de la nouvelle '
             'variante, mais seulement λ du HDBSCAN commun.\n\n'
             'Le nouveau min_cluster_size garantit la cardinalité des groupes ponctuels '
             'sélectionnés. Les pertes de qualité restent des pertes : la correction '
             'structurelle n’est pas une preuve de domination statistique. Le catalogue '
             'contributif Gabriel utilisé ici ne reproduit pas implicitement celui de '
             'HGP-old/HGP-Clusterer3D. K10, SIPU, généralisation statistique, croissance '
             'LiDAR et contrats GPU restent hors de ce pilote.\n')
    return text


def write_csv(path, rows):
    with path.open('x', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader(); writer.writerows(rows)


def publish(capture, output):
    need(not output.exists(), 'NEW publication directory required')
    source = Path(__file__).resolve(); source_sha = sha(source)
    audit_source = Path(reader.__file__).resolve(); audit_sha = sha(audit_source)
    receipt, raw_rows, pins = reader.validate_capture(capture)
    rows = [flatten(row) for row in sorted(raw_rows, key=lambda r: tuple(r[key] for key in KEYS))]
    aggregate = aggregates(rows)
    output.mkdir(parents=True, exist_ok=False)
    write_csv(output/'rows.csv', rows)
    write_csv(output/'aggregates.csv', aggregate)
    with (output/'TABLES.md').open('x') as stream:
        stream.write(tables(aggregate))
    summary = dict(schema='mhgp9_routed_point_gaussian_publication_v1', status='completed',
        capture=str(capture), capture_receipt_sha256=sha(capture/'receipt.json'),
        source=str(source), source_sha256=source_sha, audit_source=str(audit_source), audit_source_sha256=audit_sha,
        row_count=len(rows), aggregate_count=len(aggregate), new_selections=52, inherited_rows=182,
        plan=receipt['plan'], primary=receipt['primary'], scope=receipt['scope'],
        qualification=receipt['qualification'], qualification_sha256=receipt['qualification_sha256'],
        input_manifest=receipt['manifest'], input_manifest_sha256=receipt['manifest_sha256'],
        weights=receipt['weights'], EOM_arithmetic=receipt['EOM_arithmetic'],
        sources=receipt['sources_after'], elapsed_seconds=receipt['elapsed_seconds'],
        live_pins_checked=len(pins), live_before_and_after=True,
        fits_or_geometry_rerun=False, EOM_rerun=False, GCP_used=False, GPU_used=False,
        statistical_generalization_claimed=False, production_complexity_or_latency_claimed=False,
        files={name: sha(output/name) for name in ('rows.csv', 'aggregates.csv', 'TABLES.md')})
    reader.check_pins(pins)
    need(sha(source) == source_sha and sha(audit_source) == audit_sha, 'publication source closure')
    with (output/'receipt.json').open('x') as stream:
        json.dump(summary, stream, sort_keys=True, indent=2, allow_nan=False); stream.write('\n')
    print(json.dumps(dict(status='published', output=str(output), rows=len(rows), aggregates=len(aggregate))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    publish(args.capture.resolve(), args.output.resolve())
