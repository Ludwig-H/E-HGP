"""Cout des amas cospheriques (points entiers d'une sphere, interieur vide) : feuilles bloquees a m sites.
usage : python3 spheres_cout.py EXE DOSSIER_TRAVAIL"""
import json, os, struct, subprocess, sys, time
B, W = sys.argv[1], sys.argv[2]
os.makedirs(W, exist_ok=True)
def sphere(r2, off=1000):
    R = int(r2 ** 0.5) + 1
    return [(x + off, y + off, z + off) for x in range(-R, R + 1) for y in range(-R, R + 1) for z in range(-R, R + 1) if x * x + y * y + z * z == r2]
for r2 in (9, 26, 41, 89, 101, 314):
    P = sphere(r2)
    name = os.path.join(W, 'sph%d.u32le' % r2)
    with open(name, 'wb') as f:
        for p in P:
            f.write(struct.pack('<3I', *p))
    for k in (2, 5):
        if r2 == 314 and k == 5:
            continue
        t = time.time()
        r = subprocess.run([B, name, '--k=%d' % k, '--threads=1'], capture_output=True, text=True)
        js = json.loads(r.stdout.splitlines()[0])
        print('sphere R2=%d m=%d K=%d : code=%d status=%s raison=%s boules=%s feuilles_bloquees=%s m_max=%s quadruplets=%s juges=%s t=%.2fs' % (
            r2, len(P), k, r.returncode, js.get('status'), js.get('reason'), js.get('balls'), js.get('stalled_leaves'), js.get('max_m'), js.get('quad_tests'), js.get('judged'), time.time() - t))
