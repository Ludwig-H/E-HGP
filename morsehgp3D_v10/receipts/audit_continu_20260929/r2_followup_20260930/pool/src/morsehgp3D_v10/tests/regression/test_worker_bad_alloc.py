"""Regression (29 septembre 2026, audit independant § 2, audit continu pool_head § 1) : une allocation qui echoue
dans un ouvrier du pool ferme l'appel proprement. Chaque executable qui passe par le pool (mhgp10_catalogue,
mhgp10_tower, mhgp10_cluster, temoin mhgp10_mreach_cluster) rend alors le statut resource_exhausted, raison
memory_budget, code 2, sans publier de sortie (ni dump, ni etiquettes, ni arbre). Avant la correction : std::terminate,
SIGABRT.

L'echec est injecte par libmhgp10_fault_preload.so (LD_PRELOAD, hors produit), toutes formes de new remplacees
(scalaire, tableau, nothrow, alignees) :
- la N-ieme allocation faite hors du fil principal echoue (MHGP10_FAULT_OFF_MAIN) ; dans ces executables, seuls les
  ouvriers du pool allouent hors du fil principal, dans les tranches des points d'entree ;
- la N-ieme allocation alignee, tous fils confondus, echoue (MHGP10_FAULT_ALIGNED ; verificateur du tour 1 : le
  std::vector<Local> alignas(64) de build_catalogue, alloue par l'appelant, n'etait injecte par aucune porte). La
  bibliotheque ecrit a la sortie le nombre d'allocations alignees vues ; N croit jusqu'a depasser la derniere.
Sans injection declenchee, la sortie doit etre identique a celle d'une execution sans bibliotheque. Temoin positif :
chaque executable passe (code 0) sans injection. Planchers : au moins une injection hors du fil principal par
executable, et au moins une injection alignee pour ceux qui construisent un catalogue (le temoin mreach n'en construit
pas : zero allocation alignee attendue et imprimee) ; sinon la porte serait verte par vacuite. Python nu (ni numpy ni
scipy). Sans label fast : elle exige mhgp10_mreach_cluster et la bibliotheque de prechargement, que les plans de la VM
ne construisent pas forcement.

  python3 test_worker_bad_alloc.py <dossier de build>   -> code 0 si conforme, 1 desaccord, 3 plancher
"""
import hashlib
import os
import random
import re
import struct
import subprocess
import sys
import tempfile

STATUS = '"status":"resource_exhausted","reason":"memory_budget"'
MARK = 'mhgp10_fault_injected'
MARK_ALIGNED = 'mhgp10_fault_injected_aligned'
SEEN = re.compile(r'^mhgp10_fault_aligned_seen (\d+)$', re.M)
# injections alignees exigees : une par construction de catalogue (std::vector<Local> alignas(64))
ALIGNED_FLOOR = {'mhgp10_catalogue': 1, 'mhgp10_tower': 1, 'mhgp10_cluster': 1, 'mhgp10_mreach_cluster': 0}


def digest(paths):
    h = hashlib.sha256()
    for p in paths:
        with open(p, 'rb') as f:
            h.update(f.read())
    return h.hexdigest()


def clean(paths):
    for p in paths:
        if os.path.exists(p):
            os.remove(p)


def injected_run(argv, outs, base_env, lib, variable, n, reference):
    """Une execution sous injection : (ok, verdict, injection declenchee, code, stderr)."""
    clean(outs)
    env = dict(base_env)
    env['LD_PRELOAD'] = lib
    env[variable] = str(n)
    # build sous ASan : la bibliotheque prechargee precede le runtime, ce qui est voulu ici
    env['ASAN_OPTIONS'] = ':'.join(x for x in (env.get('ASAN_OPTIONS', ''), 'verify_asan_link_order=0') if x)
    r = subprocess.run(argv, capture_output=True, text=True, env=env)
    hit = MARK in r.stderr
    if r.returncode < 0:
        ok = False
        verdict = 'signal %d' % -r.returncode
    elif hit:
        ok = r.returncode == 2 and STATUS in r.stdout and '"status":"ok"' not in r.stdout and \
            not any(os.path.exists(p) for p in outs)
        verdict = 'refus memory_budget' if ok else 'ECHEC refus'
    else:
        ok = r.returncode == 0 and all(os.path.exists(p) for p in outs) and digest(outs) == reference
        verdict = 'sans injection, sortie identique' if ok else 'ECHEC sans injection'
    return ok, verdict, hit, r.returncode, r.stderr


def main():
    build = sys.argv[1]
    lib = os.path.join(build, 'libmhgp10_fault_preload.so')
    rng = random.Random(20260929)
    seen = set()
    while len(seen) < 2000:
        seen.add((rng.randrange(4096), rng.randrange(4096), rng.randrange(4096)))
    failures = 0
    floor = 0
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, 'nuage.u32le')
        with open(src, 'wb') as f:
            f.write(b''.join(struct.pack('<3I', *p) for p in sorted(seen)))
        dump, out, tree = (os.path.join(tmp, x) for x in ('dump.txt', 'labels.i32le', 'tree.txt'))
        exe = lambda name: os.path.join(build, name)  # noqa: E731
        cases = [
            ('mhgp10_catalogue', [exe('mhgp10_catalogue'), src, '--k=4', '--threads=4', '--dump=' + dump], [dump]),
            ('mhgp10_tower', [exe('mhgp10_tower'), src, '--k=4', '--threads=4', '--dump=' + dump], [dump]),
            ('mhgp10_cluster', [exe('mhgp10_cluster'), src, out, '--k=4', '--mcs=10', '--threads=4', '--tree=' + tree],
             [out, tree]),
            ('mhgp10_mreach_cluster', [exe('mhgp10_mreach_cluster'), src, out, '--k=4', '--mcs=10', '--threads=4'],
             [out]),
        ]
        base_env = dict(os.environ)
        base_env.pop('MHGP10_FAULT_OFF_MAIN', None)
        base_env.pop('MHGP10_FAULT_ALIGNED', None)
        for name, argv, outs in cases:
            clean(outs)
            r = subprocess.run(argv, capture_output=True, text=True, env=base_env)
            ok = r.returncode == 0 and all(os.path.exists(p) for p in outs)
            reference = digest(outs) if ok else None
            print('%s sans injection code=%d %s' % (name, r.returncode, 'ok' if ok else 'ECHEC'))
            failures += not ok
            injected = 0
            for n in (0, 1, 7, 50, 400, 3000, 6000, 12000, 14000, 16000, 18000, 20000, 100000):
                for attempt in range(5 if n == 0 else 1):
                    ok, verdict, hit, code, _ = injected_run(argv, outs, base_env, lib, 'MHGP10_FAULT_OFF_MAIN', n,
                                                             reference)
                    print('%s N=%d essai %d code=%d %s' % (name, n, attempt, code, verdict))
                    failures += not ok
                    injected += hit
                    if hit or not ok:
                        break
            if injected == 0:
                print('%s PLANCHER aucune injection declenchee' % name)
                floor += 1
            # formes alignees : la N-ieme allocation alignee echoue, N = 0, 1, ... jusqu'a la premiere execution sans
            # injection (au-dela de la derniere allocation alignee) ; chaque execution doit ecrire son decompte (sinon
            # la bibliotheque n'observe pas les formes alignees : plancher)
            aligned_hits, aligned_seen = 0, []
            for n in range(8):
                ok, verdict, hit, code, err = injected_run(argv, outs, base_env, lib, 'MHGP10_FAULT_ALIGNED', n,
                                                           reference)
                counts = SEEN.findall(err)
                aligned_seen.append(int(counts[-1]) if counts else None)
                if hit and MARK_ALIGNED not in err:
                    ok = False
                    verdict += ', injection non alignee'
                print('%s alignee N=%d code=%d vues=%s %s' % (name, n, code, counts[-1] if counts else 'ABSENT',
                                                              verdict))
                failures += not ok
                aligned_hits += hit
                if not hit or not ok:
                    break
            print('%s formes alignees : %d injection(s), allocations alignees vues par execution %s' %
                  (name, aligned_hits, aligned_seen))
            if aligned_hits < ALIGNED_FLOOR[name] or None in aligned_seen:
                print('%s PLANCHER injections alignees %d, exigees %d' % (name, aligned_hits, ALIGNED_FLOOR[name]))
                floor += 1
    if failures:
        print('ECHECS %d' % failures)
        return 1
    if floor:
        return 3
    print('worker_bad_alloc_ok')
    return 0


if __name__ == '__main__':
    sys.exit(main())
