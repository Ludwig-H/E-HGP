"""Mesure de mise a l'echelle : catalogue et tour sur les entrees de scale_inputs.py, par K.

Pour chaque (entree, K) : compteurs deterministes du catalogue (mhgp10_catalogue) et de la tour (mhgp10_tower
--no-points), temps mural et CPU (/usr/bin/time -v, sinon getrusage), RSS maximale. Puis, `report` : exposant de
chaque doublement, log2(x(2n) / x(n)), par regime (synthetique `space` / `density` ; LiDAR quart -> moitie -> trame,
avec les effectifs reels des secteurs : exposant = log(x_parent / x_enfant) / log(n_parent / n_enfant)).

  python3 scale_run.py run --build <build> --data <dossier scale_inputs> --out mesures.csv --k 5,10 --threads 4 \
      [--timeout 300] [--budget 1300]
Entrees traitees par taille croissante. --timeout : delai de chaque appel de binaire (statut `timeout`) ; --budget :
au-dela de ce temps total, les entrees restantes ne sont pas lancees (statut `skipped_budget`, ecrit).
  python3 scale_run.py report --csv mesures.csv
Les compteurs sont deterministes (independants de la charge) ; les temps ne valent que sur un hote calme (G4).
"""
import argparse
import csv
import json
import math
import os
import subprocess
import sys
import time

COLS = ('file', 'kind', 'family', 'regime', 'factor', 'frame', 'sector', 'sites', 'k', 'threads', 'status',
        'balls', 'cat_nodes', 'cat_leaves', 'cat_sum_m', 'cat_judged', 'cat_quad_tests', 'cat_triple_tests',
        'tower_nodes_kmax', 'tower_steps_kmax', 'tower_nodes_all', 'tower_steps_all', 'catalogue_s', 'tower_s',
        'wall_s', 'cpu_s', 'max_rss_kb')


TIMEOUT = None  # delai par appel de binaire (s), fixe par --timeout


def run_json(cmd):
    """Execute cmd sous /usr/bin/time (RSS maximale propre a la commande) et rend (code, json, mur, cpu, rss).
    Code -9 si le delai TIMEOUT est depasse."""
    timing = cmd[0] + '.time.%d' % os.getpid()
    full = ['/usr/bin/time', '-f', '%e %U %S %M', '-o', timing] + cmd if os.path.exists('/usr/bin/time') else cmd
    t0 = time.time()
    try:
        r = subprocess.run(full, capture_output=True, text=True, timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        if os.path.exists(timing):
            os.remove(timing)
        return -9, None, time.time() - t0, None, None
    wall, cpu, rss = time.time() - t0, None, None
    if full is not cmd and os.path.exists(timing):
        with open(timing) as fh:
            fields = fh.read().split()
        os.remove(timing)
        if len(fields) >= 4:
            wall, cpu, rss = float(fields[-4]), float(fields[-3]) + float(fields[-2]), int(fields[-1])
    data = None
    for line in r.stdout.splitlines():
        line = line.strip()
        if line.startswith('{'):
            data = json.loads(line)
    return r.returncode, data, wall, cpu, rss


def measure(build, path, k, threads):
    row = dict(k=k, threads=threads)
    code, cat, _, _, _ = run_json([os.path.join(build, 'mhgp10_catalogue'), path, '--k=%d' % k,
                                   '--threads=%d' % threads])
    if code == -9:
        row['status'] = 'catalogue_timeout'
        return row
    if code != 0 or cat is None:
        row['status'] = 'catalogue_refused_%d' % code
        return row
    row.update(balls=cat['balls'], cat_nodes=cat.get('nodes'), cat_leaves=cat.get('leaves'),
               cat_sum_m=cat.get('sum_m'), cat_judged=cat.get('judged'), cat_quad_tests=cat.get('quad_tests'),
               cat_triple_tests=cat.get('triple_tests'))
    code, tw, wall, cpu, rss = run_json([os.path.join(build, 'mhgp10_tower'), path, '--k=%d' % k,
                                         '--threads=%d' % threads, '--no-points'])
    if code == -9:
        row['status'] = 'tower_timeout'
        return row
    if code != 0 or tw is None:
        row['status'] = 'tower_refused_%d' % code
        return row
    orders = tw.get('orders', [])
    row.update(status='ok', catalogue_s=tw.get('catalogue_s'), tower_s=tw.get('tower_s'), wall_s=round(wall, 3),
               cpu_s=round(cpu, 3), max_rss_kb=rss,
               tower_nodes_kmax=orders[-1]['nodes'] if orders else '', tower_steps_kmax=orders[-1]['steps'] if orders else '',
               tower_nodes_all=sum(o['nodes'] for o in orders), tower_steps_all=sum(o['steps'] for o in orders))
    return row


def infer_manifest(data):
    """Manifeste reconstruit depuis les noms (le protocole G4 ne transporte que des .u32le) :
    syn_<famille>_<space|density>_x<f>.u32le et lidar<trame>_<secteur>.u32le."""
    entries = []
    for name in sorted(os.listdir(data)):
        if not name.endswith('.u32le'):
            continue
        sites = os.path.getsize(os.path.join(data, name)) // 12
        stem = name[:-len('.u32le')]
        if stem.startswith('syn_'):
            fam, regime, f = stem[4:].rsplit('_', 2)
            entries.append(dict(file=name, kind='synthetic', family=fam, regime=regime, factor=int(f[1:]),
                                sites=sites))
        elif stem.startswith('lidar'):
            frame, sector = stem[5:7], stem[8:]
            entries.append(dict(file=name, kind='lidar', frame=frame, sector=sector, sites=sites))
    return dict(entries=entries)


def cmd_run(args):
    path = os.path.join(args.data, 'MANIFEST.json')
    manifest = json.load(open(path)) if os.path.exists(path) else infer_manifest(args.data)
    ks = [int(x) for x in args.k.split(',')]
    entries = sorted(manifest['entries'], key=lambda e: (e['sites'], e['file']))
    if args.only:
        entries = [e for e in entries if any(tok in e['file'] for tok in args.only.split(','))]
    global TIMEOUT
    TIMEOUT = args.timeout
    t_start = time.time()
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        for e in entries:
            for k in ks:
                if args.budget and time.time() - t_start > args.budget:
                    row = dict(k=k, threads=args.threads, status='skipped_budget')
                else:
                    row = measure(args.build, os.path.join(args.data, e['file']), k, args.threads)
                row.update(file=e['file'], kind=e['kind'], family=e.get('family', ''), regime=e.get('regime', ''),
                           factor=e.get('factor', ''), frame=e.get('frame', ''), sector=e.get('sector', ''),
                           sites=e['sites'])
                w.writerow({c: row.get(c, '') for c in COLS})
                h.flush()
                print('%-40s K%-2d %s balls=%s tower_s=%s wall=%s' % (e['file'], k, row.get('status'),
                                                                     row.get('balls'), row.get('tower_s'),
                                                                     row.get('wall_s')), flush=True)
    return 0


METRICS = ('balls', 'cat_judged', 'tower_nodes_all', 'tower_steps_all', 'catalogue_s', 'tower_s', 'cpu_s',
           'max_rss_kb')


def expo(a, b, na, nb):
    try:
        a, b = float(a), float(b)
        if a <= 0 or b <= 0:
            return None
        return math.log(b / a) / math.log(nb / na)
    except (TypeError, ValueError):
        return None


def cmd_report(args):
    rows = [r for r in csv.DictReader(open(args.csv)) if r['status'] == 'ok']
    out = []
    by = {}
    for r in rows:
        by[(r['file'], r['k'])] = r
    ks = sorted({r['k'] for r in rows}, key=int)
    print('Exposants par doublement (log2 du rapport rapporte au rapport des effectifs)')
    for k in ks:
        print('== K%s' % k)
        fams = sorted({(r['family'], r['regime']) for r in rows if r['kind'] == 'synthetic'})
        for fam, reg in fams:
            line = []
            facs = sorted({int(r['factor']) for r in rows if r['family'] == fam and r['regime'] == reg and r['k'] == k})
            for f1, f2 in zip(facs, facs[1:]):
                a = by.get(('syn_%s_%s_x%d.u32le' % (fam, reg, f1), k))
                b = by.get(('syn_%s_%s_x%d.u32le' % (fam, reg, f2), k))
                if not a or not b:
                    continue
                e = {m: expo(a[m], b[m], float(a['sites']), float(b['sites'])) for m in METRICS}
                line.append('x%d->x%d ' % (f1, f2) + ' '.join('%s=%s' % (m, '%.2f' % v if v is not None else '-')
                                                             for m, v in e.items()))
                out.append(dict(k=k, family=fam, regime=reg, step='x%d->x%d' % (f1, f2), **e))
            print('  %-10s %-8s %s' % (fam, reg, ' | '.join(line)))
        for frame in sorted({r['frame'] for r in rows if r['kind'] == 'lidar'}):
            full = by.get(('lidar%s_full.u32le' % frame, k))
            pairs = [('half_x_neg', 'quarter_x_neg_y_neg'), ('half_x_neg', 'quarter_x_neg_y_nonneg'),
                     ('half_x_nonneg', 'quarter_x_nonneg_y_neg'), ('half_x_nonneg', 'quarter_x_nonneg_y_nonneg'),
                     ('full', 'half_x_neg'), ('full', 'half_x_nonneg')]
            for parent, child in pairs:
                a = by.get(('lidar%s_%s.u32le' % (frame, child), k))
                b = by.get(('lidar%s_%s.u32le' % (frame, parent), k))
                if not a or not b:
                    continue
                e = {m: expo(a[m], b[m], float(a['sites']), float(b['sites'])) for m in METRICS}
                print('  lidar%s %-26s %s' % (frame, '%s<-%s' % (parent, child),
                                              ' '.join('%s=%s' % (m, '%.2f' % v if v is not None else '-')
                                                       for m, v in e.items())))
                out.append(dict(k=k, family='lidar' + frame, regime='sector', step='%s<-%s' % (parent, child), **e))
            if full:
                print('  lidar%s full : sites=%s balls=%s tower_s=%s cpu_s=%s rss=%s' % (
                    frame, full['sites'], full['balls'], full['tower_s'], full['cpu_s'], full['max_rss_kb']))
    if args.json:
        json.dump(out, open(args.json, 'w'), indent=1)
    return 0


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest='cmd', required=True)
    r = sub.add_parser('run')
    r.add_argument('--build', required=True)
    r.add_argument('--data', required=True)
    r.add_argument('--out', required=True)
    r.add_argument('--k', default='5,10')
    r.add_argument('--threads', type=int, default=4)
    r.add_argument('--only', default='')
    r.add_argument('--timeout', type=float, default=None)
    r.add_argument('--budget', type=float, default=None)
    p = sub.add_parser('report')
    p.add_argument('--csv', required=True)
    p.add_argument('--json', default='')
    args = ap.parse_args()
    return cmd_run(args) if args.cmd == 'run' else cmd_report(args)


if __name__ == '__main__':
    sys.exit(main())
