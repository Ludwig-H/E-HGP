#!/usr/bin/env python3
"""Recalcul des tableaux de CARTE_V11.md depuis les recus G4 de origin/main (lecture seule).

    python3 -B CARTE_V11_derive.py [--repo /workspaces/E-HGP/build/v11-claude-20261003] [--rev origin/main]

Lit, par `git archive` (aucune ecriture dans le depot), deux archives de resultats :
  Q   = receipts/qualification_performance_20261003/captures/paired/results/results.tar.gz (moteur c40f40798, 81 prises)
  AB7 = receipts/developpement_20261003/pipeline_g4/sessions/claudeab7/results.tar.gz (tranche 3, b87285378)
et imprime les medianes par etage, les parts, les accelerations W1/W48 et les reservations memoire.
Aucune construction, aucune execution native, aucun appel GCP.
"""
import argparse
import io
import json
import re
import statistics
import subprocess
import tarfile

V11 = 'morsehgp3D_v11/receipts/'
Q_ARCHIVE = V11 + 'qualification_performance_20261003/captures/paired/results/results.tar.gz'
AB7_ARCHIVE = V11 + 'developpement_20261003/pipeline_g4/sessions/claudeab7/results.tar.gz'
CAT_KEYS = ('prefix', 'single_pass', 'compact', 'sort', 'level_scan', 'assembly', 'allocation')
PHASES = ('classify', 'births', 'regular', 'publish', 'verticals')


def git_blob_tar(repo, rev, path):
    """Rend l'archive interne (tar.gz) d'un recu, extraite d'un git archive en memoire."""
    raw = subprocess.run(['git', '-C', repo, 'archive', rev, path], check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(raw)) as outer:
        data = outer.extractfile(path).read()
    return tarfile.open(fileobj=io.BytesIO(data))


def events(text):
    found = {}
    for line in text.splitlines():
        line = line.strip()
        if line.startswith('{'):
            event = json.loads(line)
            found[event.get('phase')] = event
    return found


def stages(full, dom):
    row = {'full': full['wall_ns'] / 1e6, 'index': full['index_ns'] / 1e6, 'domain': full['domain_ns'] / 1e6,
           'forest': full['forest_ns'] / 1e6, 'cpu_s': full['cpu_seconds'],
           'peak_mib': full['peak_reserved_bytes'] / 2**20, 'after_mib': full['reserved_after_bytes'] / 2**20}
    for key in CAT_KEYS:
        row[key] = dom[key + '_ns'] / 1e6
    row['domain_resid'] = row['domain'] - sum(row[k] for k in CAT_KEYS)
    phases = full.get('phases', {})
    for key in PHASES:
        row[key] = phases.get(key + '_ns', 0) / 1e6
    row['forest_resid'] = row['forest'] - sum(row[k] for k in PHASES)
    row['pop_mib'] = full.get('population_lookup_reserved_bytes', 0) / 2**20
    row['census_ws_mib'] = full.get('census_workspace_reserved_bytes', 0) / 2**20
    row['lookup_mib'] = full.get('lookup_reserved_bytes', 0) / 2**20
    row['regvert_mib'] = full.get('regular_vertical_reserved_bytes', 0) / 2**20
    row['arena_mib'] = dom['execution']['arena_capacity_bytes'] / 2**20
    row['k5_publish_ms'] = full['orders'][-1]['timings']['plateaus_ns'] / 1e6
    row['k5_verticals_ms'] = full['orders'][-1]['timings']['verticals_ns'] / 1e6
    return row


def show(title, rows_by_key):
    print('\n== ' + title)
    for key in sorted(rows_by_key):
        rows = rows_by_key[key]
        print('  %s : %d prise(s)' % (key, len(rows)))
        for name in rows[0]:
            values = [r[name] for r in rows]
            print('    %-14s mediane %10.3f   [%s]' % (name, statistics.median(values),
                                                     ', '.join('%.3f' % v for v in values)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', default='/workspaces/E-HGP/build/v11-claude-20261003')
    parser.add_argument('--rev', default='origin/main')
    args = parser.parse_args()

    with git_blob_tar(args.repo, args.rev, Q_ARCHIVE) as tar:
        paired = json.load(tar.extractfile('results/cmd/002_paired_full/files/full_paired.json'))
    q_rows = {}
    for run in paired['runs']:
        full = [e for e in run['events'] if e.get('phase') == 'full'][0]
        dom = [e for e in run['events'] if e.get('phase') == 'domain'][0]
        key = (run['build_variant'], run['case'], 'W%d' % run['workers'])
        q_rows.setdefault(key, []).append(stages(full, dom))
    show('Q : c40f40798 (variantes baseline/current2047/current16379), medianes independantes', q_rows)

    ab_rows = {}
    with git_blob_tar(args.repo, args.rev, AB7_ARCHIVE) as tar:
        for member in tar.getmembers():
            match = re.search(r't_(base|new)_lidar_(ng0\d)_w(\d+)_r(\d+)\.stdout$', member.name)
            if not match:
                continue
            found = events(tar.extractfile(member).read().decode())
            key = (match.group(1), match.group(2), 'W' + match.group(3))
            ab_rows.setdefault(key, []).append(stages(found['full'], found['domain']))
    show('AB7 : base a45daff3a / new = tranche 3 (b87285378), mode 16379', ab_rows)

    print('\n== AB7 new : prise mediane W48 (par FULL), parts et accelerations W1/W48')
    for case in ('ng00', 'ng01', 'ng02'):
        w48 = sorted(ab_rows[('new', case, 'W48')], key=lambda r: r['full'])
        mid, one = w48[len(w48) // 2], ab_rows[('new', case, 'W1')][0]
        print('  ' + case)
        for name in ('full', 'domain', 'prefix', 'single_pass', 'compact', 'sort', 'level_scan', 'assembly',
                     'domain_resid', 'forest', 'forest_resid', 'classify', 'births', 'regular', 'publish',
                     'verticals'):
            speed = one[name] / mid[name] if mid[name] > 0.05 else float('nan')
            print('    %-13s W1 %9.1f  W48 %7.1f (%5.1f %%)  S %5.1f' % (
                name, one[name], mid[name], 100 * mid[name] / mid['full'], speed))
        rest = mid['full'] - mid['single_pass'] - mid['regular']
        print('    hors passe unique et resolution (W48) : %.1f ms' % rest)
        print('    CPU W1 %.2f s, W48 %.2f s ; W1 requis pour 100 ms a S mesure : %.0f ms' % (
            one['cpu_s'], mid['cpu_s'], 100 * one['full'] / mid['full']))


if __name__ == '__main__':
    main()
