"""Le dump par defaut de mhgp10_tower est-il identique, octet pour octet, entre HEAD (afb081774) et l'arbre integre
R2 (865f5e6) sur les 73 entrees de la porte d'oracle de HEAD ? Si oui, le verdict du juge R2 (a temoins) sur le
binaire R2 vaut pour la foret publiee par HEAD sur ces entrees.
Usage : python3 head_egale_r2.py BUILD_HEAD BUILD_R2 SRC_HEAD_V10
"""
import json
import os
import random
import subprocess
import sys
import tempfile

head, r2, src = sys.argv[1], sys.argv[2], sys.argv[3]
sys.path.insert(0, os.path.join(src, 'reference'))
sys.path.insert(0, os.path.join(src, 'tests', 'oracle'))
from test_catalogue_oracle import clouds  # noqa: E402

rnd = random.Random(20260929)
cases = [(P[:12], K) for P in clouds(24, rnd) for K in (1, 3, 5)]
cases.append(([(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)], 4))
same = diff = 0
with tempfile.TemporaryDirectory() as tmp:
    inp = os.path.join(tmp, 'in.u32le')
    for P, K in cases:
        with open(inp, 'wb') as f:
            for p in P:
                for v in p:
                    f.write(int(v).to_bytes(4, 'little'))
        out = []
        for b in (head, r2):
            d = os.path.join(tmp, 'd.txt')
            subprocess.run([os.path.join(b, 'mhgp10_tower'), inp, '--k=%d' % K, '--threads=2', '--dump=' + d],
                           capture_output=True, text=True)
            out.append(open(d, 'rb').read())
            os.remove(d)
        if out[0] == out[1]:
            same += 1
        else:
            diff += 1
print(json.dumps({'entrees': len(cases), 'dumps_identiques_HEAD_R2': same, 'differents': diff}))
sys.exit(0 if diff == 0 else 1)
