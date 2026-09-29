"""Execution d'une campagne de test preenregistree du banc v10 sur la VM G4, en une ou plusieurs sessions.

La VM n'a ni pip ni numpy, et son glibc (2.35) ne charge pas les binaires du codespace : la session televerse, comme
donnees, deux conteneurs `.u32le` (en-tete u64 = longueur, archive tar.gz, bourrage a un multiple de 4) :
  - l'environnement Python portable (CPython de python-build-standalone + numpy, scipy, scikit-learn aux versions
    epinglees) ;
  - les binaires epingles (mhgp10_cluster, mhgp10_mreach_cluster, lies statiquement) ;
et, facultativement, la liste des scenes deja calculees par les sessions precedentes (un nom de scene par ligne).

Etage 1 (python3 systeme) : decode et extrait dans --work, puis relance ce script sous le Python portable.
Etage 2 : verifie TOUTES les epingles du preenregistrement (run_test.check_pins), prend le plan epingle, retire les
scenes deja faites, calcule les autres par taille decroissante (run_test.run_unit, rien n'est reimplemente) tant que
le budget de temps le permet, et ecrit results.csv (colonnes de run_test) scene par scene, puis run.json.
Une scene n'est jamais calculee deux fois : la fusion des sessions (merge_sessions.py) le verifie.

  python3 lot_runner.py --prereg PREREG.json --env ENV.u32le --bins BINS.u32le [--done DONE.u32le]
                        --out DIR --work DIR --jobs 46 --budget 1150
Codes : 0 conforme (meme si toutes les scenes ne sont pas faites) ; 2 refus (epingle violee, conteneur invalide).
"""
import argparse
import io
import json
import os
import platform
import struct
import sys
import tarfile
import time


def read_container(path):
    with open(path, 'rb') as f:
        raw = f.read()
    if len(raw) < 8:
        raise ValueError('conteneur trop court : ' + path)
    (length,) = struct.unpack('<Q', raw[:8])
    if length > len(raw) - 8 or any(raw[8 + length:]):
        raise ValueError('conteneur incoherent : ' + path)
    return raw[8:8 + length]


def extract(path, dest):
    os.makedirs(dest, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(read_container(path)), mode='r:gz') as t:
        for m in t.getmembers():
            if m.name.startswith('/') or '..' in m.name.split('/'):
                raise ValueError('chemin refuse dans l archive : ' + m.name)
        t.extractall(dest)


def stage1(args):
    work = os.path.abspath(args.work)
    extract(args.env, os.path.join(work, 'pyenv'))
    extract(args.bins, os.path.join(work, 'bins'))
    python = os.path.join(work, 'pyenv', 'python', 'bin', 'python3')
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1', PYTHONNOUSERSITE='1')
    env.pop('PYTHONPATH', None)
    argv = [python, os.path.abspath(__file__), '--stage2'] + sys.argv[1:]
    os.execve(python, argv, env)


def stage2(args):
    here = os.path.dirname(os.path.abspath(__file__))
    sys.path.insert(0, os.path.join(here, '..', 'synthetic'))
    import csv
    from concurrent.futures import FIRST_COMPLETED, ProcessPoolExecutor, wait

    import run_test

    work = os.path.abspath(args.work)
    build = os.path.join(work, 'bins')
    with open(args.prereg) as f:
        prereg = json.load(f)
    errors = run_test.check_pins(prereg, build)
    if errors:
        for e in errors:
            print('REFUS', e, flush=True)
        return 2
    specs, digest = run_test.plan_manifest(prereg)
    done = set()
    if args.done:
        done = {line.strip() for line in read_container(args.done).decode().splitlines() if line.strip()}
    known = {run_test.unit_name(s) for s in specs}
    if not done <= known:
        print('REFUS liste des scenes faites hors du plan', flush=True)
        return 2
    todo = [s for s in specs if run_test.unit_name(s) not in done]
    todo.sort(key=lambda s: -s['n'])  # les plus grosses d'abord : la fin du budget se remplit de petites scenes
    os.makedirs(args.out, exist_ok=True)
    t0 = time.time()
    info = dict(prereg=os.path.basename(args.prereg), prereg_sha256=run_test.sha256_file(args.prereg),
                plan_sha256=digest, scenes=len(specs), already_done=len(done), todo=len(todo), jobs=args.jobs,
                budget_seconds=args.budget, host=platform.node(), cpus=os.cpu_count(),
                environment=run_test.environment(), started_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()))
    with open(os.path.join(args.out, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    computed = failed = 0
    with open(os.path.join(args.out, 'results.csv'), 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=run_test.COLUMNS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            pending = {}
            it = iter(todo)
            exhausted = False
            while True:
                while not exhausted and len(pending) < args.jobs and time.time() - t0 < args.budget:
                    s = next(it, None)
                    if s is None:
                        exhausted = True
                        break
                    pending[pool.submit(run_test.run_unit, s, prereg, build)] = s
                if not pending:
                    break
                finished, _ = wait(pending, return_when=FIRST_COMPLETED)
                for fu in finished:
                    s = pending.pop(fu)
                    try:
                        rows = fu.result()
                    except Exception as e:  # compte comme refus de toutes les methodes (regle D8)
                        failed += 1
                        rows = [dict(unit=run_test.unit_name(s), family=s['family'], level=s['level'],
                                     noise=s['noise_fraction'], n=s['n'], seed=s['seed'], method=m['name'], ari_s=0.0,
                                     ari_nc=0.0, ami_nc=0.0, coverage=0.0, clusters=0, refused=1,
                                     reason='worker: ' + repr(e)[-200:], seconds=0.0) for m in prereg['methods']]
                    for r in rows:
                        w.writerow({c: r.get(c, '') for c in run_test.COLUMNS})
                    h.flush()
                    computed += 1
                    print('%d/%d en %.0f s' % (computed, len(todo), time.time() - t0), flush=True)
    info.update(finished_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), computed=computed,
                worker_failures=failed, remaining=len(todo) - computed)
    with open(os.path.join(args.out, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    print('FIN : %d scenes calculees, %d restantes' % (computed, len(todo) - computed), flush=True)
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--stage2', action='store_true')
    ap.add_argument('--prereg', required=True)
    ap.add_argument('--env', required=True)
    ap.add_argument('--bins', required=True)
    ap.add_argument('--done', default=None)
    ap.add_argument('--out', required=True)
    ap.add_argument('--work', required=True)
    ap.add_argument('--jobs', type=int, default=46)
    ap.add_argument('--budget', type=float, default=1150.0)
    args = ap.parse_args()
    if not args.stage2:
        try:
            stage1(args)
        except (OSError, ValueError, tarfile.TarError) as e:
            print('REFUS', e, flush=True)
            return 2
        return 2  # execve ne revient pas
    return stage2(args)


if __name__ == '__main__':
    sys.exit(main())
