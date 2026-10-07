"""Bords de l'entree de mhgp10_catalogue (HEAD afb081774) : troncature, options, K, domaine, doublons, feuille trop
petite. usage : python3 bords_cli.py EXE DOSSIER_TRAVAIL"""
import json, os, struct, subprocess, sys, time
B, W = sys.argv[1], sys.argv[2]
os.makedirs(W, exist_ok=True)
def write(name, pts, extra=b''):
    with open(os.path.join(W, name), 'wb') as f:
        for p in pts:
            f.write(struct.pack('<3I', *p))
        f.write(extra)
def run(name, *args, timeout=20):
    t = time.time()
    try:
        r = subprocess.run([B, os.path.join(W, name)] + list(args), capture_output=True, text=True, timeout=timeout)
        out = r.stdout.strip().splitlines()
        try:
            js = json.loads(out[0]) if out else {}
        except ValueError:
            js = {}
        return r.returncode, js, time.time() - t, r.stderr.strip().replace('\n', ' | ')[:100]
    except subprocess.TimeoutExpired:
        return 'DELAI', {}, time.time() - t, ''
for extra in (1, 4, 8, 11):
    write('trunc%d.u32le' % extra, [(0, 0, 0), (10, 0, 0)], b'\x01' * extra)
    rc, js, dt, err = run('trunc%d.u32le' % extra, '--k=2')
    print('fichier de 2 points + %d octets : code=%s status=%s n=%s boules=%s' % (extra, rc, js.get('status'), js.get('n'), js.get('balls')))
write('vide.u32le', [])
print('fichier vide :', run('vide.u32le', '--k=2')[:2])
print('fichier absent :', run('nexistepas.u32le', '--k=2')[:2])
L = 262143
corners = [(x, y, z) for x in (0, L) for y in (0, L) for z in (0, L)]
write('cube.u32le', corners)
for leaf in (0, 8, 6, 4, 2):
    rc, js, dt, err = run('cube.u32le', '--k=5', '--leaf=%d' % leaf, '--threads=1', timeout=15)
    print('8 coins u18, K=5, --leaf=%d : code=%s boules=%s noeuds=%s feuilles=%s t=%.2fs' % (leaf, rc, js.get('balls'), js.get('nodes'), js.get('leaves'), dt))
for k in (0, 12, 13):
    rc, js, dt, err = run('cube.u32le', '--k=%d' % k)
    print('--k=%d : code=%s status=%s raison=%s boules=%s' % (k, rc, js.get('status'), js.get('reason'), js.get('balls')))
write('hors.u32le', [(0, 0, 0), (262144, 0, 0)])
print('coordonnee 262144 :', run('hors.u32le', '--k=2')[:2])
write('dup.u32le', [(0, 0, 0), (0, 0, 0), (0, 0, 0), (10, 0, 0), (0, 10, 0), (7, 7, 3)])
for k in (1, 2, 3, 5):
    rc, js, dt, err = run('dup.u32le', '--k=%d' % k)
    print('doublons, K=%d : code=%s n=%s sites=%s boules=%s ponderees=%s' % (k, rc, js.get('n'), js.get('sites'), js.get('balls'), js.get('weighted')))
write('un.u32le', [(5, 5, 5)])
rc, js, dt, err = run('un.u32le', '--k=3')
print('n=1 : code=%s status=%s boules=%s' % (rc, js.get('status'), js.get('balls')))
write('deux.u32le', [(0, 0, 0), (1, 0, 0)])
rc, js, dt, err = run('deux.u32le', '--k=1')
print('n=2 : code=%s status=%s boules=%s noeuds=%s' % (rc, js.get('status'), js.get('balls'), js.get('nodes')))
for opt in ('--k=abc', '--k=2x', '--threads=x', '--leaf=-1'):
    rc, js, dt, err = run('deux.u32le', opt)
    print('%s : code=%s status=%s stderr=%s' % (opt, rc, js.get('status'), err))
