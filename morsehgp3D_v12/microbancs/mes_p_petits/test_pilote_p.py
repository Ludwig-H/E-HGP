#!/usr/bin/env python3
"""Porte de MES-P (lecteur et pilote), sans sonde ni donnees reelles. Python 3.10 nu, aucun assert (tient sous -O).

  fils_separes   CST-0238 (temoin de l'auditeur Codex) : deux nuages a 1, 4 et 48 fils, pentes 100, 60 et 10 µs par
                 site ; le lecteur rend trois droites, jamais leur reunion (56,67 µs par site) ;
  cohorte        un nuage expire a 1 fil : la cohorte commune garde les deux autres, un ecarte a 4 et a 48 fils ;
  selection_vide le pilote refuse une selection vide (code 2) avant toute construction ;
  delai          une prise expiree tue tout son groupe de processus et n'a aucune valeur chaude.
Codes : 0 conforme ; 1 ecart.
"""
import io
import json
import os
import stat
import sys
import tempfile
import time
from contextlib import redirect_stderr, redirect_stdout

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import analyse_p  # noqa: E402
import pilote_p  # noqa: E402

SLOPES = {1: 100e-6, 4: 60e-6, 48: 10e-6}


def take(name, sites, threads, chaud, code=0):
    return dict(nuage=name, k=5, fils=threads, code=code, sites=sites, chaud=chaud, passes=[], froid=chaud)


def analyse(takes):
    with tempfile.TemporaryDirectory() as folder:
        path = os.path.join(folder, 'mes_p.json')
        with open(path, 'w', encoding='utf-8') as out:
            json.dump(dict(mesure='MES-P', prises=takes), out)
        text = io.StringIO()
        with redirect_stdout(text):
            code = analyse_p.main(['analyse_p.py', path])
    return code, text.getvalue()


def check_threads(errors):
    takes = [take('knn_%d' % n, n, f, 1e-3 + b * n) for f, b in SLOPES.items() for n in (100, 200)]
    code, text = analyse(takes)
    for f, b in SLOPES.items():
        if 'K = 5, %d fils) : cout fixe 1.00 ms, %.2f µs par site.' % (f, b * 1e6) not in text:
            errors.append('fils_separes : droite de %d fils absente' % f)
    if code != 0 or '56.67' in text:
        errors.append('fils_separes : code %s ou pente reunie publiee' % code)


def check_cohort(errors):
    takes = [take('knn_%d' % n, n, f, 1e-3 + b * n) for f, b in SLOPES.items() for n in (100, 200)]
    takes += [take('knn_300', 300, f, 1e-3 + b * 300) for f, b in SLOPES.items() if f != 1]
    takes.append(take('knn_300', 300, 1, None, code='expire'))
    code, text = analyse(takes)
    rows = [line for line in text.splitlines() if line.startswith('| ') and ' fils | 2 | ' in line]
    wanted = ['| 1 fils | 2 | 0 | 1.00 | 100.00 |', '| 4 fils | 2 | 1 | 1.00 | 60.00 |',
              '| 48 fils | 2 | 1 | 1.00 | 10.00 |']
    if code != 0 or rows != wanted or '| knn_300 | 5 | 1 | 300 | expire | - |' not in text:
        errors.append('cohorte : %s' % rows)


def check_empty(errors):
    with tempfile.TemporaryDirectory() as folder:
        data = os.path.join(folder, 'donnees')
        os.makedirs(data)
        with open(os.path.join(data, 'bundle_manifest.json'), 'w', encoding='utf-8') as out:
            json.dump(dict(cases=[dict(name='synth_a'), dict(name='synth_b')]), out)
        argv = ['pilote_p.py', '--v11-build', os.path.join(folder, 'absent'), '--donnees', data, '--sortie',
                os.path.join(folder, 'sortie'), '--exclure', 'synth_']
        with redirect_stderr(io.StringIO()):
            code = pilote_p.main(argv)
        if code != 2 or os.path.exists(os.path.join(folder, 'sortie')):
            errors.append('selection_vide : code %s' % code)


def check_delay(errors):
    with tempfile.TemporaryDirectory() as folder:
        fake = os.path.join(folder, 'sonde')
        pidfile = os.path.join(folder, 'enfant.pid')
        with open(fake, 'w', encoding='utf-8') as out:
            out.write('#!/bin/sh\nsleep 30 &\necho $! > "%s"\necho \'{"phase":"pass","wall_ns":1000}\'\n'
                      'echo \'{"phase":"pass","wall_ns":1000}\'\nsleep 30\n' % pidfile)
        os.chmod(fake, stat.S_IRWXU)
        raw = os.path.join(folder, 'brut')
        os.makedirs(raw)
        entry = pilote_p.run_cloud(fake, folder, raw, 'nuage', 5, 1, 2, 1)
        child = int(open(pidfile, encoding='utf-8').read())
        alive = True
        for _ in range(40):
            try:
                with open('/proc/%d/stat' % child, encoding='utf-8') as handle:
                    alive = handle.read().rsplit(')', 1)[1].split()[0] not in ('Z', 'X')
            except OSError:
                alive = False
            if not alive:
                break
            time.sleep(0.05)
        if entry['code'] != 'expire' or entry['chaud'] is not None or len(entry['passes']) != 2 or alive:
            errors.append('delai : %s, enfant vivant %s' % (entry, alive))


def main():
    errors = []
    for check in (check_threads, check_cohort, check_empty, check_delay):
        check(errors)
    for error in errors:
        print(error, file=sys.stderr)
    print(json.dumps(dict(porte='mes_p', cas=4, ecarts=len(errors))))
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
