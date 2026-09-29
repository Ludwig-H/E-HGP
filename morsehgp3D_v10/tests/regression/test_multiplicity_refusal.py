"""Regression (29 septembre 2026, audit IMP-16) : une entree a positions dupliquees (multiplicites) est refusee par la
tour sous sa propre raison, multiplicity_unsupported (statut unsupported_degeneracy), et non plus sous
shell_quotient_budget. Temoin positif : le meme nuage sans le doublon passe (code 0), la porte n'est donc pas verte
par vacuite. Python nu (ni numpy ni scipy) : la porte tourne aussi sur la VM G4 (label fast).

  python3 test_multiplicity_refusal.py <dossier de build>   -> code 0 si conforme, 1 sinon
"""
import json
import os
import random
import struct
import subprocess
import sys
import tempfile


def run(build, exe, src, extra):
    r = subprocess.run([os.path.join(build, exe), src] + extra, capture_output=True, text=True)
    lines = [x for x in r.stdout.splitlines() if x.startswith('{')]
    try:
        last = json.loads(lines[-1]) if lines else {}
    except ValueError:
        last = {}
    return r.returncode, last


def main():
    build = sys.argv[1]
    rng = random.Random(20260929)
    distinct = []
    while len(distinct) < 40:
        p = (rng.randrange(1000), rng.randrange(1000), rng.randrange(1000))
        if p not in distinct:
            distinct.append(p)
    doubled = list(distinct)
    doubled[7] = doubled[3]  # un doublon de position : un site de multiplicite 2
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        for name, cloud, want in (('distinct', distinct, None), ('doublon', doubled, 'multiplicity_unsupported')):
            src = os.path.join(tmp, name)
            with open(src, 'wb') as f:
                f.write(b''.join(struct.pack('<3I', *p) for p in cloud))
            for exe, extra in (('mhgp10_tower', ['--k=5']),
                               ('mhgp10_cluster', [os.path.join(tmp, 'out'), '--k=5', '--mcs=5'])):
                code, js = run(build, exe, src, extra)
                if want is None:
                    ok = code == 0
                else:
                    ok = code == 2 and js.get('reason') == want and js.get('status') == 'unsupported_degeneracy'
                print('%s %s code=%d raison=%s %s' % (exe, name, code, js.get('reason', '-'), 'ok' if ok else 'ECHEC'))
                failures += not ok
    print('multiplicity_refusal_ok' if not failures else 'ECHECS %d' % failures)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
