#!/usr/bin/env python3
"""Git et Python synthetique uniquement ; aucune commande de moteur."""
import copy
import hashlib
import importlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
REPO = Path(sys.argv[1])
BASE = '02b735d6bc7d0eeb0053303e9d9711db96a985df'
POOL = '5b3362bbdba50a5910033808daf0011c22378069'
NOW = '5f5c0c83fcb7df842996f584e3870bfbf3017d89'
PREFIX = 'morsehgp3D_v12/'
PATCH = PREFIX + 'receipts/audit_reponses_20261008/t2dc_integration/cohorte.patch'
NAMES = ['g4_catalogue_' + n + '.py' for n in
         ('flux', 'flux_judge', 'flux_tables', 'flux_selftest', 'flux_lecteur', 'device', 'judge', 'schema')]


def need(ok, why):
    if not ok:
        raise ValueError(why)


def blob(commit, path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', commit + ':' + path])


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    result = {'base': BASE, 'livraison': NOW, 'execution_native': False, 'sources': {}}
    patch = (REPO / PATCH).read_bytes()
    need(sha(patch) == '4380e99cdf3481f1e9ed02291585f07483ef24226c17391622aa31f137dd30d1', 'proposition differente')
    result['patch_sha256'] = sha(patch)
    with tempfile.TemporaryDirectory(prefix='audit-t2dc-cohorte-') as folder:
        root = Path(folder)
        bench = root / PREFIX / 'bench'
        bench.mkdir(parents=True)
        for name in NAMES:
            path = PREFIX + 'bench/' + name
            raw = blob(NOW, path)
            result['sources'][path] = sha(raw)
            (bench / name).write_bytes(raw)
        targets = ('g4_catalogue_flux_judge.py', 'g4_catalogue_flux_tables.py')
        for name in targets:
            (bench / name).write_bytes(blob(BASE, PREFIX + 'bench/' + name))
        for flags in (['--check'], []):
            subprocess.run(['git', 'apply', *flags, '-'], input=patch, cwd=root, check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        need(all((bench / name).read_bytes() == blob(NOW, PREFIX + 'bench/' + name)
                 for name in targets), 'postimages du patch non identiques')
        result['postimages_exactes'] = list(targets)
        outputs = []
        for opt in ([], ['-O']):
            run = subprocess.run([sys.executable, '-B', *opt, str(bench / NAMES[0]), '--selftest-judge'],
                                 capture_output=True, text=True, timeout=60)
            need(run.returncode == 0 and not run.stderr and run.stdout.strip() ==
                 'juge_g4_t2dc_ok injections=39 admission=25 cohorte=8',
                 'porte officielle: ' + repr((run.returncode, run.stdout, run.stderr)))
            outputs.append({'code': run.returncode, 'stdout': run.stdout.strip()})
        need(outputs[0] == outputs[1], 'normal et -O differents')
        result['porte_normal_et_O'] = outputs[0]
        sys.path.insert(0, str(bench))
        S = importlib.import_module('g4_catalogue_flux_selftest')
        J = importlib.import_module('g4_catalogue_flux_judge')
        T = importlib.import_module('g4_catalogue_flux_tables')
        need(J.judge(S.synthetic_report())['verdict'] == 'adopte', 'nominal perdu')
        result['nominal'] = 'adopte'
        cases = S.cohort_cases()
        result['temoins'] = {}
        for name, report in cases:
            rendered = T.tables(copy.deepcopy(report))
            need(J.judge(copy.deepcopy(report))['verdict'] == 'refuse' and
                 '**refuse**' in rendered and '| ng00 |' not in rendered, 'negatif perdu')
            result['temoins'][name] = {'livre': 'refuse ; aucune aggregation'}
        # Meme fixture complete, seules les deux sources du juge/publication reviennent a 02.
        for name in targets:
            (bench / name).write_bytes(blob(BASE, PREFIX + 'bench/' + name))
        importlib.reload(J)
        importlib.reload(T)
        need(J.judge(S.synthetic_report())['verdict'] == 'adopte', 'temoin causal nominal')
        for name, report in cases:
            old = {}
            try:
                old['verdict'] = J.judge(copy.deepcopy(report))['verdict']
            except TypeError:
                old['exception'] = 'TypeError'
            old['agregation_ng00'] = '| ng00 |' in T.tables(copy.deepcopy(report))
            need(old.get('verdict') != 'refuse' or old['agregation_ng00'], 'temoin non causal')
            result['temoins'][name]['avant'] = old
    reader = PREFIX + 'bench/g4_catalogue_flux_lecteur.py'
    need(blob(BASE, reader) == blob(NOW, reader), 'lecteur modifie')
    result['lecteur_inchange'] = True
    pool_review = {}
    for suffix in ('pool.cpp', 'sched.hpp'):
        path = PREFIX + 'src/sched/' + suffix
        before, after = blob(POOL, path), blob(NOW, path)
        # Retrait des lignes ENTIEREMENT commentaires seulement ; le reste est compare octet pour octet.
        def body(raw):
            return b''.join(line for line in raw.splitlines(keepends=True)
                            if not line.lstrip().startswith(b'//'))
        need(body(before) == body(after), 'corps C++ modifie')
        pool_review[path] = dict(avant_sha256=sha(before), apres_sha256=sha(after),
                                 hors_lignes_commentaires_sha256=sha(body(after)))
    result['pool'] = pool_review
    for path in ('tests/catalogue/tests.cmake', 'tests/sched/README.md'):
        result['sources'][PREFIX + path] = sha(blob(NOW, PREFIX + path))
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
