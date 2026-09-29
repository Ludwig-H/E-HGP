"""Campagne de TEST preenregistree du banc v10 (une seule execution par preenregistrement, EVAL_v2 D11).

Lit le preenregistrement (JSON), verifie tout ce qu'il epingle AVANT de generer une seule scene de test :
  - sha256 du binaire mhgp10_cluster ;
  - sha256 des scripts du banc (scenes, methods, metrics, run_campaign, run_test, decide) ;
  - versions de numpy, scipy, scikit-learn ;
  - sha256 du manifeste du plan (liste canonique des specifications de scenes, graines de l'espace `test`).
Puis execute chaque methode figee sur chaque scene. Un refus (exception, code non nul, delai) vaut ARI_s = 0 et
est compte (colonne `refused`) : aucune scene ni aucune methode n'est jamais omise (EVAL_v2 D8).

Les methodes de la tour d'une scene partagent un seul appel du binaire (un catalogue a l'ordre maximal, puis chaque
(entree, K) et chaque tete) ; si cet appel est refuse, chaque methode retombe sur son appel separe, et un refus y est
compte comme ailleurs. `--resume` reprend une execution interrompue : les scenes deja completes dans results.csv sont
gardees telles quelles, les autres sont calculees (une scene n'est jamais calculee deux fois).

  python3 run_test.py --prereg prereg/PREREG_V10_CLUSTER_<date>.json --build <build> --out <dossier> --jobs 4
Codes : 0 campagne complete ; 2 refus avant calcul (epingle violee) ; 3 campagne incomplete.
"""
import argparse
import csv
import hashlib
import json
import math
import os
import platform
import subprocess
import sys
import tempfile
import time
import traceback
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

SCRIPTS = ('scenes.py', 'methods.py', 'metrics.py', 'run_campaign.py', 'run_test.py', 'decide.py')
COLUMNS = ('unit', 'family', 'level', 'noise', 'n', 'seed', 'points', 'duplicates', 'zhat', 'method', 'ari_s',
           'ari_nc', 'ami_nc', 'coverage', 'clusters', 'refused', 'reason', 'seconds', 'shared_seconds')


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def plan_manifest(prereg):
    p = prereg['plan']
    specs = run_campaign.plan(p['split'], p['sizes'], p['replicates'], tuple(p['noises']))
    specs = [s for s in specs if s['family'] in p['families'] and s['level'] in p['levels']]
    text = json.dumps(specs, sort_keys=True, separators=(',', ':'))
    return specs, hashlib.sha256(text.encode()).hexdigest()


def environment():
    import scipy
    import sklearn
    return dict(numpy=np.__version__, scipy=scipy.__version__, sklearn=sklearn.__version__,
                python=platform.python_version())


def check_pins(prereg, build):
    errors = []
    exe = os.path.join(build, 'mhgp10_cluster')
    if not os.path.isfile(exe) or sha256_file(exe) != prereg['pins']['mhgp10_cluster_sha256']:
        errors.append('binaire mhgp10_cluster different de l epingle')
    if any(m['kind'] == 'mreach' for m in prereg['methods']):
        exe = os.path.join(build, 'mhgp10_mreach_cluster')
        if not os.path.isfile(exe) or sha256_file(exe) != prereg['pins'].get('mhgp10_mreach_cluster_sha256'):
            errors.append('binaire mhgp10_mreach_cluster different de l epingle')
    for name in SCRIPTS:
        want = prereg['pins']['scripts_sha256'].get(name)
        if want is None or sha256_file(os.path.join(HERE, name)) != want:
            errors.append('script %s different de l epingle' % name)
    env = environment()
    for key in ('numpy', 'scipy', 'sklearn'):
        if env[key] != prereg['pins']['versions'][key]:
            errors.append('%s %s au lieu de %s' % (key, env[key], prereg['pins']['versions'][key]))
    _, digest = plan_manifest(prereg)
    if digest != prereg['plan']['manifest_sha256']:
        errors.append('manifeste du plan different de l epingle')
    return errors


def mcs_of(value, n):
    return int(round(math.sqrt(n))) if value == 'sqrt' else int(value)


def apply_fill(G, labels, fill, k):
    if fill == 'none':
        return labels
    if fill == 'full':
        return methods.fill_noise(G, labels)
    if fill.startswith('b'):
        return methods.bounded_fill(G, labels, max(int(k), 5), float(fill[1:]))
    raise ValueError('politique de bruit inconnue ' + fill)


def tower_key(m, n, zh):
    z = zh if m['z'] == 'zhat' else float(m['z'])
    return ('tower', m.get('entry', 'core'), int(m['k']), mcs_of(m['mcs'], n), z, m['selection'])


def tower_batch(prereg_methods, G, n, zh, build, cache):
    """Toutes les methodes de la tour d'une scene en un seul appel ; rend sa duree. Si le binaire refuse, rien n'est
    mis en cache et chaque methode retombe sur son appel separe dans run_method."""
    keys = [tower_key(m, n, zh) for m in prereg_methods if m['kind'] == 'tower']
    if len(keys) < 2:
        return 0.0
    configs = sorted({(k[3], k[4], k[5], False) for k in keys})
    t0 = time.time()
    try:
        labels = methods.tower_labels_batch(build, G, [k[2] for k in keys], sorted({k[1] for k in keys}), configs,
                                            threads=1)
    except Exception:
        return round(time.time() - t0, 3)
    for k in keys:
        cache[k] = labels[(k[1], k[2], configs.index((k[3], k[4], k[5], False)))]
    return round(time.time() - t0, 3)


def mreach_key(m, n, zh):
    z = zh if m['z'] == 'zhat' else float(m['z'])
    return ('mreach', int(m['k']), int(m['alpha']), m['entry'], mcs_of(m['mcs'], n), z, m['selection'])


def mreach_batch(prereg_methods, G, n, zh, build, cache):
    """Methodes « meme tete » sur la hierarchie d'HDBSCAN : un appel par (K, alpha, entree), toutes les tetes a la
    fois ; si l'appel est refuse, rien n'est mis en cache et chaque methode retombe sur son appel separe."""
    groups = {}
    for m in prereg_methods:
        if m['kind'] == 'mreach':
            key = mreach_key(m, n, zh)
            groups.setdefault(key[1:4], set()).add((key[4], key[5], key[6], False))
    for (k, alpha, entry), cfgs in groups.items():
        cfgs = sorted(cfgs)
        try:
            labels = methods.mreach_labels(build, G, k, alpha, entry, cfgs, threads=1)
        except Exception:
            continue
        for (mcs, z, sel, _), lab in zip(cfgs, labels):
            cache[('mreach', k, alpha, entry, mcs, z, sel)] = lab


def run_method(m, G, n, zh, build, cache):
    """Etiquettes brutes d'une methode (avant remplissage), avec cache par construction partagee."""
    mcs = mcs_of(m['mcs'], n) if 'mcs' in m else None
    if m['kind'] == 'tower':
        key = tower_key(m, n, zh)
        if key not in cache:
            cache[key] = methods.tower_labels(build, G, key[2], key[3], key[4], key[5], threads=1, entry=key[1])
        return cache[key]
    if m['kind'] == 'mreach':
        key = mreach_key(m, n, zh)
        if key not in cache:
            cache[key] = methods.mreach_labels(build, G, key[1], key[2], key[3], [(key[4], key[5], key[6], False)],
                                               threads=1)[0]
        return cache[key]
    if m['kind'] == 'sklearn':
        key = ('sk', m['min_samples'], mcs, m['selection'], m['alpha'])
        if key not in cache:
            cache[key] = methods.hdbscan_labels(G, int(m['min_samples']), mcs, m['selection'], float(m['alpha']))
        return cache[key]
    if m['kind'] == 'sklearn_default':
        if 'default' not in cache:
            from sklearn.cluster import HDBSCAN
            cache['default'] = HDBSCAN(copy=True).fit(np.asarray(G, dtype=np.float64)).labels_.astype(np.int64)
        return cache['default']
    raise ValueError('methode inconnue ' + m['kind'])


def unit_name(spec):
    return '%s_n%d_%s_nu%g_s%d' % (spec['family'], spec['n'], spec['level'], spec['noise_fraction'], spec['seed'])


def run_unit(spec, prereg, build):
    unit = unit_name(spec)
    base = dict(unit=unit, family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=spec['n'],
                seed=spec['seed'])
    out = []
    try:
        P, L, _ = scenes.generate(spec)
        G, T, dups, _ = scenes.quantize18(P, L)
        n = len(G)
        zh = methods.zhat(G)
    except Exception:
        for m in prereg['methods']:
            out.append(dict(base, method=m['name'], ari_s=0.0, ari_nc=0.0, ami_nc=0.0, coverage=0.0, clusters=0,
                            refused=1, reason='generation: ' + traceback.format_exc(limit=1).strip()[-200:],
                            seconds=0.0))
        return out
    base.update(points=n, duplicates=dups, zhat=round(zh, 4))
    cache = {}
    shared = tower_batch(prereg['methods'], G, n, zh, build, cache)
    t_mr = time.time()
    mreach_batch(prereg['methods'], G, n, zh, build, cache)
    base.update(shared_seconds=round(shared + time.time() - t_mr, 3))
    for m in prereg['methods']:
        t0 = time.time()
        try:
            raw = run_method(m, G, n, zh, build, cache)
            k = m.get('k', m.get('min_samples', 5))
            lab = apply_fill(G, raw, m['fill'], k)
            s = metrics.scores(T, lab)
            out.append(dict(base, method=m['name'], ari_s=round(s['ari_s'], 6), ari_nc=round(s['ari_nc'], 6),
                            ami_nc=round(s['ami_nc'], 6), coverage=round(s['coverage'], 4),
                            clusters=s['clusters'], refused=0, reason='', seconds=round(time.time() - t0, 3)))
        except Exception:
            out.append(dict(base, method=m['name'], ari_s=0.0, ari_nc=0.0, ami_nc=0.0, coverage=0.0, clusters=0,
                            refused=1, reason=traceback.format_exc(limit=1).strip()[-200:],
                            seconds=round(time.time() - t0, 3)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--prereg', required=True)
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--resume', action='store_true')
    args = ap.parse_args()
    with open(args.prereg) as f:
        prereg = json.load(f)
    errors = check_pins(prereg, args.build)
    if errors:
        for e in errors:
            print('REFUS', e, flush=True)
        return 2
    specs, digest = plan_manifest(prereg)
    path = os.path.join(args.out, 'results.csv')
    segment = dict(started_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), jobs=args.jobs,
                   host=platform.node(), cpus=os.cpu_count(), environment=environment())
    kept = set()
    if args.resume and os.path.isfile(path):
        info = json.load(open(os.path.join(args.out, 'run.json')))
        if info['prereg_sha256'] != sha256_file(args.prereg) or info['plan_sha256'] != digest:
            print('REFUS reprise : preenregistrement ou plan different', flush=True)
            return 2
        rows = list(csv.DictReader(open(path)))
        names = {m['name'] for m in prereg['methods']}
        by_unit = {}
        for r in rows:
            by_unit.setdefault(r['unit'], set()).add(r['method'])
        kept = {u for u, ms in by_unit.items() if ms == names}
        with open(path, 'w', newline='') as h:  # scenes incompletes retirees, recalculees ci-dessous
            w = csv.DictWriter(h, fieldnames=COLUMNS)
            w.writeheader()
            for r in rows:
                if r['unit'] in kept:
                    w.writerow({c: r.get(c, '') for c in COLUMNS})
        segment.update(kept_scenes=len(kept))
        info.setdefault('segments', []).append(segment)
    else:
        os.makedirs(args.out, exist_ok=False)
        info = dict(prereg=os.path.basename(args.prereg), prereg_sha256=sha256_file(args.prereg), plan_sha256=digest,
                    scenes=len(specs), segments=[segment])
        with open(path, 'w', newline='') as h:
            csv.DictWriter(h, fieldnames=COLUMNS).writeheader()
    with open(os.path.join(args.out, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    todo = [s for s in specs if unit_name(s) not in kept]
    done, failed = len(specs) - len(todo), 0
    with open(path, 'a', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLUMNS)
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, prereg, args.build): s for s in todo}
            for fu in as_completed(futs):
                done += 1
                try:
                    rows = fu.result()
                except Exception:
                    failed += 1
                    s = futs[fu]
                    rows = [dict(unit=unit_name(s), family=s['family'], level=s['level'], noise=s['noise_fraction'],
                                 n=s['n'], seed=s['seed'], method=m['name'], ari_s=0.0, ari_nc=0.0, ami_nc=0.0, coverage=0.0,
                                 clusters=0, refused=1, reason='worker: ' + traceback.format_exc(limit=1)[-200:],
                                 seconds=0.0) for m in prereg['methods']]
                for r in rows:
                    w.writerow({c: r.get(c, '') for c in COLUMNS})
                h.flush()
                print('%d/%d' % (done, len(specs)), flush=True)
    segment.update(finished_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), worker_failures=failed,
                   computed_scenes=len(todo))
    with open(os.path.join(args.out, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    return 0 if done == len(specs) else 3


if __name__ == '__main__':
    sys.exit(main())
