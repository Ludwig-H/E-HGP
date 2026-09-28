"""Campagne de TEST preenregistree du banc v10 (une seule execution par preenregistrement, EVAL_v2 D11).

Lit le preenregistrement (JSON), verifie tout ce qu'il epingle AVANT de generer une seule scene de test :
  - sha256 du binaire mhgp10_cluster ;
  - sha256 des scripts du banc (scenes, methods, metrics, run_campaign, run_test, decide) ;
  - versions de numpy, scipy, scikit-learn ;
  - sha256 du manifeste du plan (liste canonique des specifications de scenes, graines de l'espace `test`).
Puis execute chaque methode figee sur chaque scene. Un refus (exception, code non nul, delai) vaut ARI_s = 0 et
est compte (colonne `refused`) : aucune scene ni aucune methode n'est jamais omise (EVAL_v2 D8).

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
           'ari_nc', 'ami_nc', 'coverage', 'clusters', 'refused', 'reason', 'seconds')


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


def run_method(m, G, n, zh, build, cache):
    """Etiquettes brutes d'une methode (avant remplissage), avec cache par construction partagee."""
    mcs = mcs_of(m['mcs'], n) if 'mcs' in m else None
    if m['kind'] == 'tower':
        z = zh if m['z'] == 'zhat' else float(m['z'])
        key = ('tower', m['k'], mcs, z, m['selection'])
        if key not in cache:
            cache[key] = methods.tower_labels(build, G, int(m['k']), mcs, z, m['selection'], threads=1)
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


def run_unit(spec, prereg, build):
    unit = '%s_n%d_%s_nu%g_s%d' % (spec['family'], spec['n'], spec['level'], spec['noise_fraction'], spec['seed'])
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
    args = ap.parse_args()
    with open(args.prereg) as f:
        prereg = json.load(f)
    errors = check_pins(prereg, args.build)
    if errors:
        for e in errors:
            print('REFUS', e, flush=True)
        return 2
    specs, digest = plan_manifest(prereg)
    os.makedirs(args.out, exist_ok=False)
    info = dict(prereg=os.path.basename(args.prereg), prereg_sha256=sha256_file(args.prereg), plan_sha256=digest,
                scenes=len(specs), environment=environment(), host=platform.node(), cpus=os.cpu_count(),
                started_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), jobs=args.jobs)
    path = os.path.join(args.out, 'results.csv')
    done, failed = 0, 0
    with open(path, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLUMNS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, prereg, args.build): s for s in specs}
            for fu in as_completed(futs):
                done += 1
                try:
                    rows = fu.result()
                except Exception:
                    failed += 1
                    s = futs[fu]
                    rows = [dict(unit='%s_n%d_%s_nu%g_s%d' % (s['family'], s['n'], s['level'], s['noise_fraction'],
                                                              s['seed']),
                                 family=s['family'], level=s['level'], noise=s['noise_fraction'], n=s['n'],
                                 seed=s['seed'], method=m['name'], ari_s=0.0, ari_nc=0.0, ami_nc=0.0, coverage=0.0,
                                 clusters=0, refused=1, reason='worker: ' + traceback.format_exc(limit=1)[-200:],
                                 seconds=0.0) for m in prereg['methods']]
                for r in rows:
                    w.writerow({c: r.get(c, '') for c in COLUMNS})
                h.flush()
                print('%d/%d' % (done, len(specs)), flush=True)
    info.update(finished_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), worker_failures=failed)
    with open(os.path.join(args.out, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    return 0 if done == len(specs) else 3


if __name__ == '__main__':
    sys.exit(main())
