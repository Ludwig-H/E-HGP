"""Verdict des deux portes d'oracle de HEAD pour un binaire mutant, sans rejouer tout Gamma_k.

Le juge est deterministe : sur une entree ou le dump du mutant est IDENTIQUE octet pour octet a celui de HEAD (dont la
porte complete est verte, rejouee dans cet audit), son verdict est celui de HEAD. On ne rejoue donc le vrai juge
(check() des portes) que sur les entrees dont le dump differe. La verification de translation (auto-coherence de deux
executions du meme binaire) est rejouee telle quelle.
Usage : python3 oracles_rapides.py BUILD_HEAD BUILD_MUTANT SRC_V10
"""
import json
import os
import random
import subprocess
import sys
import tempfile

head, mut, src = sys.argv[1], sys.argv[2], sys.argv[3]
sys.path.insert(0, os.path.join(src, 'reference'))
sys.path.insert(0, os.path.join(src, 'tests', 'oracle'))
import test_catalogue_oracle as C  # noqa: E402
import test_tower_oracle as T  # noqa: E402


def write(P, path):
    with open(path, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))


def dump(build, exe, inp, K, tmp, tag):
    d = os.path.join(tmp, tag)
    if os.path.exists(d):
        os.remove(d)
    r = subprocess.run([os.path.join(build, exe), inp, '--k=%d' % K, '--threads=2', '--dump=' + d],
                       capture_output=True, text=True)
    return (r.returncode, open(d, 'rb').read() if os.path.exists(d) else b'')


res = {'catalogue': {'entrees': 0, 'dumps_identiques': 0, 'rejuges': 0, 'ecarts': []},
       'tour': {'entrees': 0, 'dumps_identiques': 0, 'rejuges': 0, 'ecarts': []}}
with tempfile.TemporaryDirectory() as tmp:
    inp = os.path.join(tmp, 'in.u32le')
    rnd = random.Random(20260928)
    for t, P in enumerate(C.clouds(40, rnd)):
        for K in (1, 2, 3, 5):
            write(P, inp)
            res['catalogue']['entrees'] += 1
            if dump(head, 'mhgp10_catalogue', inp, K, tmp, 'h') == dump(mut, 'mhgp10_catalogue', inp, K, tmp, 'm'):
                res['catalogue']['dumps_identiques'] += 1
            else:
                res['catalogue']['rejuges'] += 1
                err = C.check(os.path.join(mut, 'mhgp10_catalogue'), P, K, tmp)
                if err:
                    res['catalogue']['ecarts'].append('nuage %d K=%d : %s' % (t, K, err[:120]))
    err = C.translation_check(os.path.join(mut, 'mhgp10_catalogue'), rnd, tmp)
    res['catalogue']['translation'] = err or 'conforme'
    rnd = random.Random(20260929)
    cases = [(P[:12], K) for P in C.clouds(24, rnd) for K in (1, 3, 5)]
    cases.append(([(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)], 4))
    for i, (P, K) in enumerate(cases):
        write(P, inp)
        res['tour']['entrees'] += 1
        if dump(head, 'mhgp10_tower', inp, K, tmp, 'h') == dump(mut, 'mhgp10_tower', inp, K, tmp, 'm'):
            res['tour']['dumps_identiques'] += 1
        else:
            res['tour']['rejuges'] += 1
            err, _ = T.check(os.path.join(mut, 'mhgp10_tower'), P, K, tmp)
            if err:
                res['tour']['ecarts'].append('cas %d K=%d : %s' % (i, K, err[:120]))
ok = not res['catalogue']['ecarts'] and not res['tour']['ecarts'] and res['catalogue']['translation'] == 'conforme'
res['verdict_portes_oracle'] = 'VERTES (mutant survivant aux deux oracles)' if ok else 'ROUGE (mutant tue par un oracle)'
print(json.dumps(res, indent=1, sort_keys=True))
sys.exit(0 if ok else 1)
