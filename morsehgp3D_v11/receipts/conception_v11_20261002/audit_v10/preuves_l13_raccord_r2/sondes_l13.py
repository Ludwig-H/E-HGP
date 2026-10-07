#!/usr/bin/env python3
"""Sondes L13 : memes appels sur la v10 PUBLIEE (origin/main, code = commit jetable 5a06844) et sur l'arbre final du
raccord R2 (865f5e6, hors depot). Chaque appel : code de sortie, signal eventuel, premiere ligne, etat des fichiers.
Aucune ecriture hors du dossier de travail. Usage : sondes_l13.py <build_tete> <build_final> <travail>"""
import hashlib, os, random, shutil, struct, subprocess, sys, resource

def sha(p):
    if not os.path.exists(p):
        return 'ABSENT'
    with open(p, 'rb') as f:
        d = f.read()
    return f'{len(d)}o:{hashlib.sha256(d).hexdigest()[:12]}'

def cloud(path, pts):
    with open(path, 'wb') as f:
        for p in pts:
            f.write(struct.pack('<3I', *p))

def run(argv, cwd, as_kib=None, stdout_to=None, timeout=60):
    def pre():
        if as_kib:
            resource.setrlimit(resource.RLIMIT_AS, (as_kib * 1024, as_kib * 1024))
    try:
        out = open(stdout_to, 'wb') if stdout_to else subprocess.PIPE
        r = subprocess.run(argv, cwd=cwd, stdout=out, stderr=subprocess.PIPE, timeout=timeout, preexec_fn=pre)
        if stdout_to:
            out.close()
            so = ''
        else:
            so = r.stdout.decode('utf-8', 'replace')
        se = r.stderr.decode('utf-8', 'replace')
        first = (so.strip().splitlines() or se.strip().splitlines() or [''])[0][:150]
        return r.returncode, first
    except subprocess.TimeoutExpired:
        return 'DELAI', ''

def main():
    head, final, work = map(os.path.abspath, sys.argv[1:4])
    rng = random.Random(20261002)
    p40 = [(rng.randrange(1 << 18), rng.randrange(1 << 18), rng.randrange(1 << 18)) for _ in range(40)]
    five = [(0, 0, 0), (10, 0, 0), (0, 10, 0), (0, 0, 10), (7, 7, 7)]
    rows = []
    def cas(nom, f):
        res = {}
        for tag, b in (('tete_publiee', head), ('raccord_final', final)):
            d = os.path.join(work, nom.replace(' ', '_').replace('/', '_')[:60], tag)
            shutil.rmtree(d, ignore_errors=True)
            os.makedirs(d)
            cloud(os.path.join(d, 'p40.u32le'), p40)
            cloud(os.path.join(d, 'five.u32le'), five)
            cloud(os.path.join(d, 'one.u32le'), five[:1])
            with open(os.path.join(d, 'tronque.u32le'), 'wb') as fh:   # 40 points + 5 octets parasites
                for p in p40:
                    fh.write(struct.pack('<3I', *p))
                fh.write(b'\x01\x02\x03\x04\x05')
            with open(os.path.join(d, 'precieux.txt'), 'w') as fh:
                fh.write('precieux\n')
            res[tag] = f(b, d)
        rows.append((nom, res['tete_publiee'], res['raccord_final']))
    T = lambda b: os.path.join(b, 'mhgp10_tower')
    C = lambda b: os.path.join(b, 'mhgp10_cluster')
    G = lambda b: os.path.join(b, 'mhgp10_catalogue')
    def simple(exe, args, watch=(), **kw):
        def f(b, d):
            before = {w: sha(os.path.join(d, w)) for w in watch}
            code, first = run([exe(b)] + args, d, **kw)
            after = {w: sha(os.path.join(d, w)) for w in watch}
            ch = ' '.join(f'{w}:{"intact" if before[w] == after[w] else before[w] + "->" + after[w]}' for w in watch)
            return f'code={code} | {first} | {ch}'.strip(' |')
        return f
    cas('tower --k=abc (entier mal forme)', simple(T, ['p40.u32le', '--k=abc', '--threads=1']))
    cas('tower --k=2abc (prefixe numerique)', simple(T, ['p40.u32le', '--k=2abc', '--threads=1']))
    cas('tower --k=0', simple(T, ['p40.u32le', '--k=0', '--threads=1']))
    cas('tower option inconnue', simple(T, ['p40.u32le', '--k=2', '--inconnue', '--threads=1']))
    cas('tower entree absente', simple(T, ['absent.u32le', '--k=2', '--threads=1']))
    cas('tower entree tronquee (+5 octets)', simple(T, ['tronque.u32le', '--k=2', '--threads=1']))
    cas('tower --dump=ENTREE en succes', simple(T, ['five.u32le', '--k=2', '--threads=1', '--dump=five.u32le'], watch=('five.u32le',)))
    cas('tower --dump=precieux, refus k=13', simple(T, ['five.u32le', '--k=13', '--threads=1', '--dump=precieux.txt'], watch=('precieux.txt',)))
    cas('tower --dump= (vide)', simple(T, ['five.u32le', '--k=2', '--threads=1', '--dump=']))
    cas('tower --threads=-1 (RLIMIT_AS 1 Gio)', simple(T, ['p40.u32le', '--k=2', '--threads=-1'], as_kib=1 << 20))
    cas('tower --threads=1024 (RLIMIT_AS 1 Gio)', simple(T, ['p40.u32le', '--k=2', '--threads=1024'], as_kib=1 << 20))
    cas('tower sortie standard /dev/full avec dump', simple(T, ['p40.u32le', '--k=2', '--threads=1', '--dump=precieux.txt'], watch=('precieux.txt',), stdout_to='/dev/full'))
    cas('catalogue --k=10 --leaf=11 (feuille K+1)', simple(G, ['p40.u32le', '--k=10', '--leaf=11', '--threads=1']))
    cas('catalogue --k=10 --leaf=13 (feuille K+3)', simple(G, ['p40.u32le', '--k=10', '--leaf=13', '--threads=1']))
    for z in ('nan', '-1', '0', '17', 'abc', '1e'):
        cas(f'cluster --z={z}', simple(C, ['p40.u32le', 'out.i32le', '--k=2', '--mcs=2', f'--z={z}', '--threads=1'], watch=('out.i32le',)))
    cas('cluster --mcs=0', simple(C, ['p40.u32le', 'out.i32le', '--k=2', '--mcs=0', '--threads=1'], watch=('out.i32le',)))
    cas('cluster --mcs=-1', simple(C, ['p40.u32le', 'out.i32le', '--k=2', '--mcs=-1', '--threads=1'], watch=('out.i32le',)))
    cas('cluster --k=1 --mcs=1', simple(C, ['p40.u32le', 'out.i32le', '--k=1', '--mcs=1', '--threads=1'], watch=('out.i32le',)))
    cas('cluster un point --k=1 --mcs=2', simple(C, ['one.u32le', 'out.i32le', '--k=1', '--mcs=2', '--threads=1'], watch=('out.i32le',)))
    cas('cluster collision etiquettes/--tree', simple(C, ['p40.u32le', 'out.i32le', '--k=2', '--mcs=2', '--tree=out.i32le', '--threads=1'], watch=('out.i32le',)))
    cas('cluster SORTIE = ENTREE', simple(C, ['p40.u32le', 'p40.u32le', '--k=2', '--mcs=2', '--threads=1'], watch=('p40.u32le',)))
    cas('cluster option prise pour la sortie', simple(C, ['p40.u32le', '--k=5', '--mcs=5', '--threads=1'], watch=('--k=5',)))
    cas('cluster --entry=cover5 sur 5 sites (K+E > sites)', simple(C, ['five.u32le', 'out.i32le', '--k=2', '--mcs=2', '--entry=cover5', '--threads=1'], watch=('out.i32le',)))
    cas('cluster --allow-single --mcs=40 --k=5 --z=1 (regle de la racine)', lambda b, d: (lambda r: r + ' | bruit=' + str(sum(1 for i in range(0, os.path.getsize(os.path.join(d, 'out.i32le')), 4) if struct.unpack('<i', open(os.path.join(d, 'out.i32le'), 'rb').read()[i:i + 4])[0] == -1) if os.path.exists(os.path.join(d, 'out.i32le')) else 'ABSENT'))(simple(C, ['p40.u32le', 'out.i32le', '--k=5', '--mcs=40', '--z=1', '--allow-single', '--threads=1'])(b, d)))
    w = max(len(r[0]) for r in rows)
    for nom, a, b in rows:
        print(f'## {nom}')
        print(f'   tete publiee  : {a}')
        print(f'   raccord final : {b}')
    print(f'FIN sondes={len(rows)}')

if __name__ == '__main__':
    main()
