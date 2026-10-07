"""Invariant global J-KM2 (GEN_v2 § 12.4) rejoue par l'auditeur : cat(K) == restriction de cat(K + 2) aux boules
p + q_min <= K + 1 (entrees sans doublon). Compare les enregistrements (q, p, u, S*, I, U) et l'ordre relatif.
usage : python3 restrict_check.py EXE IN.u32le K [fils]"""
import hashlib, os, subprocess, sys, tempfile
exe, src, K = sys.argv[1], sys.argv[2], int(sys.argv[3])
th = sys.argv[4] if len(sys.argv) > 4 else '3'
def dump(k, path):
    r = subprocess.run([exe, src, '--k=%d' % k, '--threads=' + th, '--dump=' + path], capture_output=True, text=True)
    return r.returncode
with tempfile.TemporaryDirectory() as tmp:  # prevoir quelques centaines de Mo pour les dumps d'une trame entiere
    a, b = os.path.join(tmp, 'a.txt'), os.path.join(tmp, 'b.txt')
    if dump(K, a) or dump(K + 2, b):
        print('REFUS'); sys.exit(2)
    ha, hb = hashlib.sha256(), hashlib.sha256()
    na = nb = nkeep = 0
    for line in open(a):
        na += 1
        ha.update(line.split(' ', 1)[1].encode())   # sans le rang
    for line in open(b):
        nb += 1
        head = line.split('|', 1)[0].split()
        q, p = int(head[1]), int(head[2])
        if p + q <= K + 1:
            nkeep += 1
            hb.update(line.split(' ', 1)[1].encode())
    ok = ha.hexdigest() == hb.hexdigest() and na == nkeep
    print('%s K=%d : cat(K) %d boules ; cat(K+2) %d boules, restriction %d ; %s (sha %s / %s)' % (
        os.path.basename(src), K, na, nb, nkeep, 'EGALITE' if ok else 'ECART', ha.hexdigest()[:12], hb.hexdigest()[:12]))
    sys.exit(0 if ok else 1)
