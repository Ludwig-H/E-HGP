#!/usr/bin/env python3
"""Rejeu Git + Python seulement. Aucun moteur, compilation, données ou réseau.
Usage: python [-O] check.py /chemin/du/depot
"""
import contextlib
import copy
import hashlib
import importlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
CAPTURE = json.loads((HERE / 'capture.json').read_text())
REPO = Path(sys.argv[1])


def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def git(*args):
    return subprocess.check_output(['git', '-C', str(REPO), *args])


def blob(pin, path):
    return git('show', pin + ':' + path)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def inventory(pin):
    names = git('ls-tree', '-r', '--name-only', pin, '--', 'morsehgp3D_v12/src/').decode().splitlines()
    return {n: sha(blob(pin, n)) for n in names}


def tree_hash(items):
    return sha(''.join('%s %s\n' % (items[n], n) for n in sorted(items)).encode())


def source_review():
    base, after, parent = (CAPTURE[k] for k in ('base', 'candidate', 'parent'))
    old, new = inventory(base), inventory(after)
    changed = [{'path': p, 'base': old.get(p), 'after': new.get(p)}
               for p in sorted(old.keys() | new.keys()) if old.get(p) != new.get(p)]
    outside = [x for x in changed if not x['path'].startswith('morsehgp3D_v12/src/catalogue/')]
    need([x['path'] for x in outside] == ['morsehgp3D_v12/src/io/io.hpp',
                                        'morsehgp3D_v12/src/io/sha256.cpp',
                                        'morsehgp3D_v12/src/sched/pool.cpp',
                                        'morsehgp3D_v12/src/sched/sched.hpp',
                                        'morsehgp3D_v12/src/sched/source_pins.json'], 'diff produit inattendu')
    delivered = inventory(CAPTURE['commit'])
    need([p for p in sorted(old.keys() | delivered.keys()) if old.get(p) != delivered.get(p)
          and '/src/catalogue/' not in p] == [x['path'] for x in outside[:2]], 'diff 02 inattendu')
    paths = CAPTURE['recipe_allowlist']
    patch = git('diff', parent, CAPTURE['commit'], '--', *paths)
    # Copie limitée aux fichiers affectés ; aucune copie ou modification de l'arbre vivant.
    with tempfile.TemporaryDirectory(prefix='t2dc-inverse-') as folder:
        root = Path(folder)
        for path in paths:
            out = root / path
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(blob(after, path))
        patchfile = root / 'inverse.patch'
        patchfile.write_bytes(patch)
        for flags in (['--reverse', '--check'], ['--reverse']):
            subprocess.run(['git', 'apply', *flags, str(patchfile)], cwd=root, check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        parent_names = set(git('ls-tree', '-r', '--name-only', parent).decode().splitlines())
        composed = dict(new)
        for path in paths:
            p = root / path
            if path in parent_names:
                need(p.read_bytes() == blob(parent, path), 'inverse incorrect: ' + path)
                if path in composed:
                    composed[path] = sha(p.read_bytes())
            else:
                need(not p.exists(), 'nouveau fichier non supprimé: ' + path)
                composed.pop(path, None)
    need(all(composed[p] == h for p, h in new.items() if '/src/catalogue/' not in p),
         'inverse a changé un produit hors catalogue')
    need(all(composed.get(p) == h for p, h in old.items() if '/src/catalogue/' in p),
         'catalogue reconstruit différent de 902')
    return {'base_files': len(old), 'after_files': len(new), 'base_tree': tree_hash(old),
            'after_tree': tree_hash(new), 'delivered_02_tree': tree_hash(delivered),
            'changed': changed, 'inverse_patch_sha256': sha(patch),
            'inverse_files': len(paths), 'same_pin_before_C_tree': tree_hash(composed),
            'inverse_application': 'ok ; postimages parent exactes ; hors catalogue inchangé',
            'qualification_compilation': False}


def probes(S, J, T):
    base = S.synthetic_report()
    result = {}

    def witness(name, edit):
        report = copy.deepcopy(base)
        edit(report)
        out = J.judge(report)
        need(out['verdict'] == 'adopte', name + ': comportement livré différent')
        result[name] = {'verdict': out['verdict'], 'mutant': out['stats'].get('mutant'),
                        'refused': out['refused'], 'rejected': out['rejected']}

    witness('nominal_officiel', lambda r: None)

    def failed(r, field, value):
        S.mutant_invariant(r)
        rows = r['steps']['mutant']['run']['rows']
        rows[1][field] = value
        if field == 'reason':
            rows[-1][field] = value

    witness('mutant_cpu_pour_commande_device', lambda r: failed(r, 'path', 'cpu'))
    witness('mutant_memory_budget_declare_invariant', lambda r: failed(r, 'reason', 'memory_budget'))
    witness('full_code_false', lambda r: r['steps']['ful1'][0]['run'].update(code=False))
    witness('full_liberation_pass_false',
            lambda r: r['steps']['ful1'][0]['run']['rows'][2].update({'pass': False}))
    witness('campagne_hors_cohorte_invalide', lambda r: r['steps']['campaign'].append(
        {'round': 10, 'frame': 'ng00', 'arm': 'apres', 'run': {}}))
    report = copy.deepcopy(base)
    for r in range(10, 21):
        e = copy.deepcopy(S.entry(base, 'campaign', round=0, frame='ng00', arm='apres'))
        e['round'] = r
        for row in e['run']['rows']:
            if row['phase'] == 'catalogue':
                row['wall_ns'] = 1000
                for key in row['diagnostics']:
                    if key.endswith('_ns'):
                        row['diagnostics'][key] = 0
                row['device']['transfer_ns'] = row['device']['publish_ns'] = 0
            if row['phase'] == 'sorties':
                row['outputs_ns'] = 0
        report['steps']['campaign'].append(e)

    def line(r):
        return next(x for x in T.campaign_lines(r) if x.startswith('| ng00 | apres |'))

    a, b = J.judge(base), J.judge(report)
    need(a == b and line(base) != line(report), 'témoin publication non causal')
    result['cohorte_juge_et_tableaux_divergente'] = {
        'before': line(base), 'after': line(report), 'extra_processes': 11,
        'verdict': b['verdict'], 'entire_judgment_unchanged': a == b}
    return result


def proposed(S, J, T):
    base = S.synthetic_report()
    need(J.judge(base)['verdict'] == 'adopte', 'positif perdu par la proposition')
    cases = {
        'vide': lambda r: r['steps'].update(campaign=[]),
        'manquante': lambda r: r['steps']['campaign'].pop(),
        'doublon': lambda r: r['steps']['campaign'].append(copy.deepcopy(r['steps']['campaign'][0])),
        'tour_etranger': lambda r: r['steps']['campaign'][0].update(round=10),
        'trame_etrangere': lambda r: r['steps']['campaign'][0].update(frame='autre'),
        'bras_etranger': lambda r: r['steps']['campaign'][0].update(arm='autre'),
        'tour_booleen': lambda r: r['steps']['campaign'][0].update(round=False),
        'tour_non_hashable': lambda r: r['steps']['campaign'][0].update(round=[]),
    }
    result = {}
    for name, edit in cases.items():
        r = copy.deepcopy(base)
        edit(r)
        r['verdict'] = {'verdict': 'adopte'}  # ancienne décision ne doit pas être republiée
        verdict = J.judge(r)
        rendered = T.tables(r)
        need(verdict['verdict'] == 'refuse' and '**refuse**' in rendered and '| ng00 |' not in rendered,
             'cohorte non refusée avant agrégation: ' + name)
        result[name] = {'verdict': verdict['verdict'], 'table_refuse': True}
    return {'nominal': 'adopte', 'negative': result}


def main():
    sources = source_review()
    with tempfile.TemporaryDirectory(prefix='t2dc-python-') as folder:
        root = Path(folder)
        for name, expected in CAPTURE['files'].items():
            data = blob(CAPTURE['commit'], 'morsehgp3D_v12/bench/' + name)
            need(sha(data) == expected, 'pin de source: ' + name)
            bench = root / 'morsehgp3D_v12/bench'
            bench.mkdir(parents=True, exist_ok=True)
            (bench / name).write_bytes(data)
        sys.path.insert(0, str(bench))
        S = importlib.import_module('g4_catalogue_flux_selftest')
        J = importlib.import_module('g4_catalogue_flux_judge')
        T = importlib.import_module('g4_catalogue_flux_tables')
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            code = S.selftest()
        need(code == 0, 'auto-test officiel')
        result = {'source_review': sources, 'official': {'code': code, 'stdout': stdout.getvalue().strip()},
                  'counter_json': probes(S, J, T)}
        for flags in (['--check'], []):
            subprocess.run(['git', 'apply', *flags, str(HERE / 'cohorte.patch')], cwd=root, check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        importlib.reload(J)
        importlib.reload(T)
        result['proposed_closed_cohort'] = proposed(S, J, T)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
