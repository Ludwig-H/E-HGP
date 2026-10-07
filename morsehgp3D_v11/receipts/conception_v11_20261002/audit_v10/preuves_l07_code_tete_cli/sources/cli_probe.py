"""Audit L07 : batterie de frontieres du CLI mhgp10_cluster (binaire construit de afb081774, sources inchangees a
52687f8e5). Chaque cas : argv, code (negatif = signal), sorties standard, fichiers produits. Aucune assertion : la
batterie CONSTATE. Limite d'adresse 6 Go par processus (garde de l'audit sur machine partagee).

  python3 -B cli_probe.py BUILD_DIR WORK_DIR > cli_probe.jsonl
"""
import itertools
import json
import os
import random
import resource
import shutil
import struct
import subprocess
import sys


def limit():
    resource.setrlimit(resource.RLIMIT_AS, (6 << 30, 6 << 30))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def write(path, pts, extra=b''):
    with open(path, 'wb') as f:
        f.write(b''.join(struct.pack('<3I', *p) for p in pts) + extra)


def main():
    build, work = sys.argv[1], sys.argv[2]
    exe = os.path.join(build, 'mhgp10_cluster')
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    rng = random.Random(20261002)
    g40 = []
    while len(g40) < 40:
        p = (rng.randrange(1000), rng.randrange(1000), rng.randrange(1000))
        if p not in g40:
            g40.append(p)
    sphere = sorted({tuple(10 + s * v for s, v in zip(sg, perm))
                     for base in ((3, 0, 0), (2, 2, 1)) for perm in set(itertools.permutations(base))
                     for sg in itertools.product((1, -1), repeat=3)})
    inputs = {
        'g40': (g40, b''), 'g3': (g40[:3], b''), 'g1': (g40[:1], b''), 'dup': (g40[:39] + [g40[3]], b''),
        'ood': (g40[:39] + [(262144, 5, 5)], b''), 'trunc': (g40, b'\x01\x02\x03\x04\x05'), 'empty': ([], b''),
        'cube27': ([(10 * x, 10 * y, 10 * z) for x in range(3) for y in range(3) for z in range(3)], b''),
        'sphere30': (sphere + [(10, 10, 10), (40, 40, 40), (41, 43, 47), (45, 40, 42), (48, 47, 41), (43, 49, 45)], b''),
    }
    for name, (pts, extra) in inputs.items():
        write(os.path.join(work, name + '.u32le'), pts, extra)
    with open(os.path.join(work, 'cfg_ok'), 'w') as f:
        f.write('5 1.0 eom 0\n5 1.0 leaf 0\n')
    with open(os.path.join(work, 'cfg_empty'), 'w') as f:
        pass
    with open(os.path.join(work, 'cfg_bad2'), 'w') as f:
        f.write('5 1.0 eom 0\n5 abc eom 0\n7 1.0 leaf 0\n')
    with open(os.path.join(work, 'cfg_sel'), 'w') as f:
        f.write('5 1.0 feuilles 0\n')
    with open(os.path.join(work, 'cfg_neg'), 'w') as f:
        f.write('-1 nan eom 7\n')
    I = lambda n: os.path.join(work, n + '.u32le')  # noqa: E731
    C = lambda n: os.path.join(work, n)  # noqa: E731
    cases = [
        ('A01 sans argument', []),
        ('A02 entree absente', [C('absent.u32le'), 'OUT']),
        ('A03 options avant les chemins', ['--k=2', I('g40'), 'OUT']),
        ('A04 temoin conforme', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--threads=1']),
        ('A05 --k=abc', [I('g40'), 'OUT', '--k=abc', '--mcs=5', '--threads=1']),
        ('A06 --k=3x (suffixe)', [I('g40'), 'OUT', '--k=3x', '--mcs=5', '--threads=1']),
        ('A07 --k= (vide)', [I('g40'), 'OUT', '--k=', '--mcs=5', '--threads=1']),
        ('A08 --k=0', [I('g40'), 'OUT', '--k=0', '--mcs=5', '--threads=1']),
        ('A09 --k=-1', [I('g40'), 'OUT', '--k=-1', '--mcs=5', '--threads=1']),
        ('A10 --k=11 (au-dela de kMaxOrder=10)', [I('g40'), 'OUT', '--k=11', '--mcs=5', '--threads=1']),
        ('A11 --k=12', [I('g40'), 'OUT', '--k=12', '--mcs=5', '--threads=1']),
        ('A12 --k=13', [I('g40'), 'OUT', '--k=13', '--mcs=5', '--threads=1']),
        ('A13 --k=99999999999 (hors int)', [I('g40'), 'OUT', '--k=99999999999', '--mcs=5', '--threads=1']),
        ('A14 --mcs=0', [I('g40'), 'OUT', '--k=2', '--mcs=0', '--threads=1']),
        ('A15 --mcs=1', [I('g40'), 'OUT', '--k=2', '--mcs=1', '--threads=1']),
        ('A16 --mcs=-1 (repli 2^64-1)', [I('g40'), 'OUT', '--k=2', '--mcs=-1', '--threads=1']),
        ('A17 --mcs=1e3 (lu 1)', [I('g40'), 'OUT', '--k=2', '--mcs=1e3', '--threads=1']),
        ('A18 --mcs=abc', [I('g40'), 'OUT', '--k=2', '--mcs=abc', '--threads=1']),
        ('A19 --z=nan', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--z=nan', '--threads=1']),
        ('A20 --z=inf', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--z=inf', '--threads=1']),
        ('A21 --z=-1', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--z=-1', '--threads=1']),
        ('A22 --z=0', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--z=0', '--threads=1']),
        ('A23 --z=1,5 (virgule, lu 1)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--z=1,5', '--threads=1']),
        ('A24 --z=abc', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--z=abc', '--threads=1']),
        ('A25 --z=400 (lambda sous-deborde)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--z=400', '--threads=1']),
        ('A26 --selection=EOM', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--selection=EOM', '--threads=1']),
        ('A27 --threads=4294967297 (repli a 1)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--threads=4294967297']),
        ('A28 --threads=-1 (repli a 2^32-1)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--threads=-1']),
        ('A29 --threads=abc', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--threads=abc']),
        ('A30 --entry=cover9', [I('g40'), 'OUT', '--k=5', '--mcs=5', '--entry=cover9', '--threads=1']),
        ('A31 --entry=cover0', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--entry=cover0', '--threads=1']),
        ('A32 --entry=core,core (doublon)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--entry=core,core', '--threads=1']),
        ('A33 --entry= (vide)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--entry=', '--threads=1']),
        ('A34 --entry=Cover', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--entry=Cover', '--threads=1']),
        ('A35 --cover-extra=-3 (ignore)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--entry=cover', '--cover-extra=-3', '--threads=1']),
        ('A36 --cover-extra=2 avec core (sans effet)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--cover-extra=2', '--threads=1']),
        ('A37 --label=vote avec core (sans effet)', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--label=vote', '--threads=1']),
        ('A38 --label=vote avec cover', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--entry=cover', '--label=vote', '--threads=1']),
        ('A39 --label=foo', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--label=foo', '--threads=1']),
        ('A40 --k-list= (vide, retombe sur --k)', [I('g40'), 'OUT', '--k-list=', '--mcs=5', '--threads=1']),
        ('A41 --k-list=2,,3', [I('g40'), 'OUT', '--k-list=2,,3', '--mcs=5', '--threads=1']),
        ('A42 --k-list=0,2', [I('g40'), 'OUT', '--k-list=0,2', '--mcs=5', '--threads=1']),
        ('A43 --k-list=3,3 (doublon)', [I('g40'), 'OUT', '--k-list=3,3', '--mcs=5', '--threads=1']),
        ('A44 --k-list=3 seul (pas multi)', [I('g40'), 'OUT', '--k-list=3', '--mcs=5', '--threads=1']),
        ('A45 --k=5 puis --k-list=2 (--k ignore)', [I('g40'), 'OUT', '--k=5', '--k-list=2', '--mcs=5', '--threads=1']),
        ('A46 --configs absent', [I('g40'), 'OUT', '--k=2', '--configs=' + C('absent'), '--threads=1']),
        ('A47 --configs vide', [I('g40'), 'OUT', '--k=2', '--configs=' + C('cfg_empty'), '--threads=1']),
        ('A48 --configs ligne 2 malformee', [I('g40'), 'OUT', '--k=2', '--configs=' + C('cfg_bad2'), '--threads=1']),
        ('A49 --configs selection inconnue (lue eom)', [I('g40'), 'OUT', '--k=2', '--configs=' + C('cfg_sel'), '--threads=1']),
        ('A50 --configs mcs=-1 z=nan single=7', [I('g40'), 'OUT', '--k=2', '--configs=' + C('cfg_neg'), '--threads=1']),
        ('A51 --configs et --mcs (ignore)', [I('g40'), 'OUT', '--k=2', '--mcs=7', '--configs=' + C('cfg_ok'), '--threads=1']),
        ('A52 option inconnue', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--inconnue=1']),
        ('A53 option repetee (derniere gagne)', [I('g40'), 'OUT', '--k=2', '--k=3', '--mcs=5', '--threads=1']),
        ('A54 --allow-single', [I('g40'), 'OUT', '--k=2', '--mcs=50', '--allow-single', '--threads=1']),
        ('B01 fichier vide', [I('empty'), 'OUT', '--k=2', '--mcs=5', '--threads=1']),
        ('B02 5 octets en trop (tronque)', [I('trunc'), 'OUT', '--k=2', '--mcs=5', '--threads=1']),
        ('B03 un point, K=1', [I('g1'), 'OUT', '--k=1', '--mcs=5', '--threads=1']),
        ('B04 un point, K=1, --allow-single', [I('g1'), 'OUT', '--k=1', '--mcs=5', '--allow-single', '--threads=1']),
        ('B05 trois points, K=5', [I('g3'), 'OUT', '--k=5', '--mcs=5', '--threads=1']),
        ('B06 trois points, K=3', [I('g3'), 'OUT', '--k=3', '--mcs=2', '--threads=1']),
        ('B07 coordonnee 2^18', [I('ood'), 'OUT', '--k=2', '--mcs=5', '--threads=1']),
        ('B08 position dupliquee', [I('dup'), 'OUT', '--k=2', '--mcs=5', '--threads=1']),
        ('B09 grille 3x3x3, K=1..5', [I('cube27'), 'OUT', '--k-list=1,2,3,4,5', '--mcs=3', '--threads=1']),
        ('B10 30 cospheriques, K=1,2,5', [I('sphere30'), 'OUT', '--k-list=1,2,5', '--mcs=3', '--threads=1']),
        ('B11 trois points, K=3, mcs=5, --allow-single', [I('g3'), 'OUT', '--k=3', '--mcs=5', '--allow-single', '--threads=1']),
        ('C01 sortie dans un dossier absent', [I('g40'), os.path.join('absent_dir', 'OUT'), '--k=2', '--mcs=5', '--threads=1']),
        ('C02 --tree egal a la sortie', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--tree=OUT', '--threads=1']),
        ('C03 --tree dans un dossier absent', [I('g40'), 'OUT', '--k=2', '--mcs=5', '--tree=absent_dir/T', '--threads=1']),
        ('C04 groupe k-list+entrees+configs+vote+tree', [I('g40'), 'OUT', '--k-list=2,3', '--entry=core,cover,cover1',
                                                      '--configs=' + C('cfg_ok'), '--label=vote', '--tree=T', '--threads=1']),
        ('C05 --configs seul (une ligne)', [I('g40'), 'OUT', '--k=2', '--configs=' + C('cfg_sel'), '--tree=T', '--threads=1']),
    ]
    env = dict(os.environ, LC_ALL='C')
    for name, args in cases:
        d = os.path.join(work, 'run_' + name.split()[0])
        os.makedirs(d)
        try:
            r = subprocess.run([exe] + args, cwd=d, capture_output=True, text=True, timeout=120, preexec_fn=limit, env=env)
            rc, out, err = r.returncode, r.stdout, r.stderr
        except subprocess.TimeoutExpired:
            rc, out, err = 'timeout', '', ''
        files = sorted((f, os.path.getsize(os.path.join(d, f))) for f in os.listdir(d))
        print(json.dumps(dict(cas=name, argv=[a.replace(work + '/', '') for a in args], code=rc, stdout=out[-400:],
                              stderr=err[-300:], fichiers=files), ensure_ascii=False))
    # C06 : sortie egale a l'entree
    d = os.path.join(work, 'run_C06')
    os.makedirs(d)
    src = os.path.join(d, 'io.u32le')
    write(src, g40)
    before = os.path.getsize(src)
    r = subprocess.run([exe, src, src, '--k=2', '--mcs=5', '--threads=1'], cwd=d, capture_output=True, text=True,
                       timeout=120, preexec_fn=limit, env=env)
    print(json.dumps(dict(cas='C06 sortie egale a l entree', code=r.returncode, stdout=r.stdout[-300:],
                          taille_avant=before, taille_apres=os.path.getsize(src)), ensure_ascii=False))


if __name__ == '__main__':
    main()
