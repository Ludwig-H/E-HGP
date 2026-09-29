"""Mesure de mise a l'echelle : catalogue et tour sur les entrees de scale_inputs.py, par K.

Pour chaque (entree, K) : compteurs deterministes du catalogue (mhgp10_catalogue) et de la tour (mhgp10_tower
--no-points), temps mural et CPU (/usr/bin/time -v, sinon getrusage), RSS maximale. Puis, `report` : exposant de
chaque doublement, log2(x(2n) / x(n)), par regime (synthetique `space` / `density` ; LiDAR quart -> moitie -> trame,
avec les effectifs reels des secteurs : exposant = log(x_parent / x_enfant) / log(n_parent / n_enfant)).

  python3 scale_run.py run --build <build> --data <dossier scale_inputs> --out mesures.csv --k 5,10 --threads 4 \
      [--timeout 300] [--budget 1300] [--calls appels.jsonl]
Entrees traitees par taille croissante. --timeout : delai de chaque appel de binaire (statut `timeout`) ; --budget :
au-dela de ce temps total, les entrees restantes ne sont pas lancees (statut `skipped_budget`, ecrit).
  python3 scale_run.py report --csv mesures.csv
Les compteurs de l'objet (boules, noeuds, catalogue) sont deterministes ; les pas de descente `tower_steps_*` dependent
de l'ordre d'arrivee des fils au-dela d'un fil (memo partage, src/tower/tower.hpp) ; les temps ne valent que sur un
hote calme (G4).

Chaque appel de binaire tourne dans son propre groupe de processus (nouvelle session) : au delai, a une exception ou
a SIGTERM/SIGHUP/SIGINT, tout le groupe (le calcul sous /usr/bin/time compris) recoit SIGKILL et est recolte avant la
suite ; aucun calcul declare arrete ne partage la machine avec les mesures suivantes (audit du 29 septembre 2026).
Le JSON natif de chaque appel (stdout brut, stderr, code, mur, CPU, RSS) est conserve, une ligne par appel, dans
--calls (defaut : <out>.calls.jsonl). Le compteur `balls` de l'appel catalogue doit egaler celui de l'appel tour,
sinon statut `balls_mismatch`. Codes de `run` : 0 (delais, refus et budget sont des statuts) ; 3 invariant viole
(`balls_mismatch`, ou groupe d'un appel encore vivant apres SIGKILL) ; 128 + signal apres SIGTERM/SIGHUP/SIGINT.
Limite : seul SIGKILL de scale_run lui-meme (sans SIGTERM prealable) laisserait l'appel en cours hors de portee.
"""
import argparse
import csv
import json
import math
import os
import signal
import subprocess
import sys
import time

COLS = ('file', 'kind', 'family', 'regime', 'factor', 'frame', 'sector', 'sites', 'k', 'threads', 'status',
        'balls', 'cat_nodes', 'cat_leaves', 'cat_sum_m', 'cat_judged', 'cat_quad_tests', 'cat_triple_tests',
        'tower_nodes_kmax', 'tower_steps_kmax', 'tower_nodes_all', 'tower_steps_all', 'catalogue_s', 'tower_s',
        'wall_s', 'cpu_s', 'max_rss_kb')


TIMEOUT = None  # delai par appel de binaire (s), fixe par --timeout
GRACE = 10.0  # delai (s) accorde a la fermeture du groupe d'un appel apres SIGKILL
CALL_KEYS = ('call', 'argv', 'code', 'timed_out', 'wall_s', 'cpu_s', 'max_rss_kb', 'stdout', 'stderr')


class GroupNotClosed(RuntimeError):
    """Un membre du groupe d'un appel survit a SIGKILL au-dela de GRACE : les mesures suivantes seraient faussees."""


class Terminated(BaseException):
    """SIGTERM, SIGHUP ou SIGINT pendant `run` : l'appel en cours est tue et recolte, puis sortie 128 + signal."""

    def __init__(self, signum):
        super().__init__(signum)
        self.signum = signum


_LAUNCH = dict(active=False, pending=[])  # signaux differes pendant le lancement d'un appel


def terminate(signum, frame):
    """Gestionnaire de `run`. Un signal recu pendant le lancement d'un appel est differe jusqu'a ce que l'appel soit
    sous garde : leve entre fork et la garde, il laisserait l'appel vivant hors de portee."""
    if _LAUNCH['active']:
        _LAUNCH['pending'].append(signum)
        return
    raise Terminated(signum)


_SUBREAPER = None


def become_subreaper():
    """Linux : ce processus devient le « subreaper » de ses descendants. Le calcul, lance sous /usr/bin/time, est un
    petit-enfant : tue avec son groupe, il revient ici et y est recolte. Ailleurs, init le recolte et close_group
    attend que le groupe soit vide."""
    global _SUBREAPER
    if _SUBREAPER is None:
        _SUBREAPER = False
        if sys.platform.startswith('linux'):
            try:
                import ctypes
                _SUBREAPER = ctypes.CDLL(None, use_errno=True).prctl(36, 1, 0, 0, 0) == 0  # PR_SET_CHILD_SUBREAPER
            except (OSError, AttributeError):
                _SUBREAPER = False
    return _SUBREAPER


def close_group(proc):
    """Tue le groupe de processus d'un appel (SIGKILL), recolte l'enfant direct puis les descendants revenus ici, et ne
    rend qu'une fois le groupe vide ; rend (stdout, stderr) deja emis. GroupNotClosed si un membre survit a GRACE.
    SIGINT/SIGTERM/SIGHUP sont differes pendant la fermeture : ils agissent juste apres, groupe ferme."""
    held = {signal.SIGINT, signal.SIGTERM, signal.SIGHUP}
    old = signal.pthread_sigmask(signal.SIG_BLOCK, held) if hasattr(signal, 'pthread_sigmask') else None
    try:
        if proc.returncode is None:  # enfant direct pas encore recolte : son groupe existe encore
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        try:
            out, err = proc.communicate(timeout=GRACE)
        except subprocess.TimeoutExpired:
            raise GroupNotClosed('appel %s : tubes encore ouverts %.0f s apres SIGKILL' % (proc.args, GRACE))
        deadline = time.monotonic() + GRACE
        while True:
            try:
                reaped = os.waitpid(-proc.pid, os.WNOHANG)[0]
            except ChildProcessError:
                reaped = 0
            try:
                os.killpg(proc.pid, 0)
            except ProcessLookupError:
                return out or '', err or ''
            except PermissionError:
                pass
            if time.monotonic() > deadline:
                raise GroupNotClosed('appel %s : groupe %d vivant %.0f s apres SIGKILL' % (proc.args, proc.pid, GRACE))
            if not reaped:
                time.sleep(0.005)
    finally:
        if old is not None:
            signal.pthread_sigmask(signal.SIG_SETMASK, old)


def run_json(cmd):
    """Execute cmd sous /usr/bin/time (RSS maximale propre a la commande), dans son propre groupe de processus, et rend
    l'enregistrement de l'appel : argv, code (-9 si le delai TIMEOUT est depasse), timed_out, json (derniere ligne
    JSON de stdout), stdout et stderr bruts (JSON natif conserve tel quel), wall_s, cpu_s, max_rss_kb. Au delai comme a
    toute exception (signal compris), tout le groupe est tue et recolte avant de rendre la main."""
    become_subreaper()
    timing = cmd[0] + '.time.%d' % os.getpid()
    full = ['/usr/bin/time', '-f', '%e %U %S %M', '-o', timing] + cmd if os.path.exists('/usr/bin/time') else cmd
    rec = dict(argv=list(cmd), code=None, timed_out=False, json=None, stdout='', stderr='', wall_s=None, cpu_s=None,
               max_rss_kb=None)
    t0 = time.time()
    _LAUNCH['active'] = True
    try:
        proc = subprocess.Popen(full, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                start_new_session=True)
    except BaseException:
        _LAUNCH['active'] = False
        raise
    try:
        _LAUNCH['active'] = False
        if _LAUNCH['pending']:
            raise Terminated(_LAUNCH['pending'].pop(0))
        out, err = proc.communicate(timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        try:
            out, err = close_group(proc)
        finally:
            if os.path.exists(timing):
                os.remove(timing)
        rec.update(code=-9, timed_out=True, wall_s=time.time() - t0, stdout=out, stderr=err)
        return rec
    except BaseException:
        try:
            close_group(proc)
        finally:
            if os.path.exists(timing):
                os.remove(timing)
        raise
    wall, cpu, rss = time.time() - t0, None, None
    if full is not cmd and os.path.exists(timing):
        with open(timing) as fh:
            fields = fh.read().split()
        os.remove(timing)
        if len(fields) >= 4:
            wall, cpu, rss = float(fields[-4]), float(fields[-3]) + float(fields[-2]), int(fields[-1])
    data = None
    for line in out.splitlines():
        line = line.strip()
        if line.startswith('{'):
            data = json.loads(line)
    rec.update(code=proc.returncode, json=data, stdout=out, stderr=err, wall_s=wall, cpu_s=cpu, max_rss_kb=rss)
    return rec


def measure(build, path, k, threads):
    """Mesure un couple (entree, K) : rend la ligne du CSV et les enregistrements des appels (JSON natifs)."""
    row = dict(k=k, threads=threads)
    cat_call = dict(run_json([os.path.join(build, 'mhgp10_catalogue'), path, '--k=%d' % k, '--threads=%d' % threads]),
                    call='catalogue')
    calls = [cat_call]
    code, cat = cat_call['code'], cat_call['json']
    if cat_call['timed_out']:
        row['status'] = 'catalogue_timeout'
        return row, calls
    if code != 0 or cat is None:
        row['status'] = 'catalogue_refused_%d' % code
        return row, calls
    row.update(balls=cat['balls'], cat_nodes=cat.get('nodes'), cat_leaves=cat.get('leaves'),
               cat_sum_m=cat.get('sum_m'), cat_judged=cat.get('judged'), cat_quad_tests=cat.get('quad_tests'),
               cat_triple_tests=cat.get('triple_tests'))
    tw_call = dict(run_json([os.path.join(build, 'mhgp10_tower'), path, '--k=%d' % k, '--threads=%d' % threads,
                             '--no-points']), call='tower')
    calls.append(tw_call)
    code, tw, wall, cpu, rss = (tw_call['code'], tw_call['json'], tw_call['wall_s'], tw_call['cpu_s'],
                                tw_call['max_rss_kb'])
    if tw_call['timed_out']:
        row['status'] = 'tower_timeout'
        return row, calls
    if code != 0 or tw is None:
        row['status'] = 'tower_refused_%d' % code
        return row, calls
    if tw.get('balls') != cat['balls']:  # deux appels, une seule entree : le catalogue doit etre le meme
        row['status'] = 'balls_mismatch'
        return row, calls
    orders = tw.get('orders', [])
    row.update(status='ok', catalogue_s=tw.get('catalogue_s'), tower_s=tw.get('tower_s'), wall_s=round(wall, 3),
               cpu_s=round(cpu, 3) if cpu is not None else '', max_rss_kb=rss,
               tower_nodes_kmax=orders[-1]['nodes'] if orders else '', tower_steps_kmax=orders[-1]['steps'] if orders else '',
               tower_nodes_all=sum(o['nodes'] for o in orders), tower_steps_all=sum(o['steps'] for o in orders))
    return row, calls


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
    for sig in (signal.SIGTERM, signal.SIGHUP, signal.SIGINT):  # une disposition ignoree (nohup, &) est respectee
        if signal.getsignal(sig) in (signal.SIG_DFL, signal.default_int_handler):
            signal.signal(sig, terminate)
    code = 0
    t_start = time.time()
    with open(args.out, 'w', newline='') as h, open(args.calls or args.out + '.calls.jsonl', 'w') as hc:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        for e in entries:
            for k in ks:
                calls = []
                if args.budget and time.time() - t_start > args.budget:
                    row = dict(k=k, threads=args.threads, status='skipped_budget')
                else:
                    try:
                        row, calls = measure(args.build, os.path.join(args.data, e['file']), k, args.threads)
                    except GroupNotClosed as err:
                        print('INVARIANT %s : mesure arretee' % err, flush=True)
                        return 3
                for c in calls:
                    hc.write(json.dumps(dict({key: c[key] for key in CALL_KEYS}, file=e['file'], k=k,
                                             threads=args.threads), sort_keys=True) + '\n')
                hc.flush()
                row.update(file=e['file'], kind=e['kind'], family=e.get('family', ''), regime=e.get('regime', ''),
                           factor=e.get('factor', ''), frame=e.get('frame', ''), sector=e.get('sector', ''),
                           sites=e['sites'])
                w.writerow({c: row.get(c, '') for c in COLS})
                h.flush()
                print('%-40s K%-2d %s balls=%s tower_s=%s wall=%s' % (e['file'], k, row.get('status'),
                                                                     row.get('balls'), row.get('tower_s'),
                                                                     row.get('wall_s')), flush=True)
                if row.get('status') == 'balls_mismatch':
                    code = 3
    return code


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
    r.add_argument('--calls', default='')
    p = sub.add_parser('report')
    p.add_argument('--csv', required=True)
    p.add_argument('--json', default='')
    args = ap.parse_args()
    if args.cmd != 'run':
        return cmd_report(args)
    try:
        return cmd_run(args)
    except Terminated as stop:
        print('INTERROMPU par le signal %d : appel en cours tue et recolte' % stop.signum, file=sys.stderr, flush=True)
        return 128 + stop.signum


if __name__ == '__main__':
    sys.exit(main())
