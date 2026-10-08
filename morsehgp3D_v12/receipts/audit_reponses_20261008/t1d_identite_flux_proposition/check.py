#!/usr/bin/env python3
"""Reconstitution Git et tests JSON synthetiques seulement ; aucun processus natif."""
import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
PREFIX = 'morsehgp3D_v12/bench/'
NAMES = ('flux_niveaux_modifies', 'flux_table_modifiee', 'flux_table_ecarts', 'flux_complet_absent',
         'flux_preuve_refuse_apres_C', 'flux_prefixe_niveaux_faux', 'flux_prefixe_table_fausse',
         'flux_prefixe_table_ecarts', 'flux_ecarts_booleen', 'flux_libre_et_budgets_faux')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def child(tree, baseline):
    sys.path.insert(0, str(tree / PREFIX))
    import g4_catalogue_t1d_judge as T
    import g4_catalogue_t1d_selftest as S
    import g4_catalogue_flux_judge as J
    import g4_catalogue_flux_lecteur as L

    def module(source):
        ns = dict(__name__='audit_judge', __file__=str(tree / PREFIX / 'g4_catalogue_t1d_judge.py'))
        exec(compile(source, ns['__file__'], 'exec'), ns)
        return ns

    source = (tree / PREFIX / 'g4_catalogue_t1d_judge.py').read_text()
    old = module((baseline / PREFIX / 'g4_catalogue_t1d_judge.py').read_text())
    need({k: v for k, v in T.RULE.items() if k != 'flux'} ==
         {k: v for k, v in old['RULE'].items() if k != 'flux'}, 'regle numerique modifiee')
    need(T.spec_campaign(48, 10) == old['spec_campaign'](48, 10), 'commande chronometree modifiee')
    for spec in (T.spec_free(5, 48), T.spec_budget(5, 48, 7)):
        opts = L.catalogue_options(spec)
        need('--digest' in opts and '--digest-complet' in opts and '--tranches' in opts, 'commande flux')

    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        need(S.selftest() == 0, 'porte T1d : ' + captured.getvalue())
    cases = {n: (r, e) for n, r, e in S.cases() if n in NAMES}
    need(set(cases) == set(NAMES), 'injections absentes')

    def evaluate(report, fn):
        out = dict(refused=[], rejected=[], stats={})
        refs = J.check_identity(report['steps'], out, 48)
        fn(report['steps'], out, 48, refs)
        return 'refuse' if out['refused'] else 'rejete' if out['rejected'] else 'adopte'

    changed, projected = {}, {}
    for name, (report, expected) in cases.items():
        changed[name] = evaluate(report, T.check_slices)
        need(changed[name] == expected, name)
        # Projection vers les observations demandees par l'ancien protocole. Elle ne simule pas une sortie native :
        # sans cette option, les differences de niveaux/table ne sont simplement pas presentes dans le journal.
        r = copy.deepcopy(report)
        for entry in r['steps']['slices']:
            run = entry['run']
            run['options'].remove('--digest-complet')
            run['rows'] = [row for row in run['rows'] if row.get('phase') != 'digest_complet']
        projected[name] = evaluate(r, old['check_slices'])

    mutants = (
        ('sans_comparaison_niveaux', 'flux_niveaux_modifies',
         [("c['niveaux_sha256'] != reference['niveaux_sha256'] or\n            ", '')]),
        ('sans_comparaison_table', 'flux_table_modifiee',
         [("c['table_sha256'] != reference['table_sha256']", 'False')]),
        ('sans_ecarts', 'flux_table_ecarts',
         [("reference['table_ecarts'] != 0", 'False'), ("c['table_ecarts'] != 0 or ", '')]),
        ('sans_completude', 'flux_preuve_refuse_apres_C',
         [("len(parsed['complets']) != len(parsed['passes'])", 'False')]),
        ('sans_prefixes_refuses', 'flux_prefixe_niveaux_faux',
         [('if not check_complets(got,', "if state == 'ok' and not check_complets(got,")]),
        ('libre_son_propre_oracle', 'flux_libre_et_budgets_faux',
         [("check_complets(free, cpu['complets'][0],", "check_complets(free, free['complets'][0],")]),
    )
    causal = {}
    nominal = S.synthetic_report()
    for name, witness, edits in mutants:
        altered = source
        for before, after in edits:
            need(altered.count(before) == 1, 'motif mutant ' + name)
            altered = altered.replace(before, after)
        fn = module(altered)['check_slices']
        need(evaluate(nominal, fn) == 'adopte', 'mutant casse nominal ' + name)
        result = evaluate(cases[witness][0], fn)
        need(result == 'adopte', 'temoin non causal ' + name)
        causal[name] = dict(temoin=witness, mutant=result, proposition=changed[witness])
    return dict(porte=captured.getvalue().strip(), nouveaux_cas=changed,
                projection_ancien_protocole=projected, mutations_causales=causal,
                commande_cout_et_seuils_inchanges=True, qualification_native=False)


def main():
    if len(sys.argv) == 4 and sys.argv[1] == '--child':
        print(json.dumps(child(Path(sys.argv[2]), Path(sys.argv[3])), sort_keys=True, ensure_ascii=False))
        return
    repo = Path(sys.argv[1] if len(sys.argv) > 1 else '/workspaces/E-HGP')
    here = Path(__file__).resolve().parent
    capture = json.loads((here / 'capture.json').read_text())
    def blob(pin, path):
        return subprocess.check_output(['git', '-C', str(repo), 'show', pin + ':' + path])
    pre = capture['prerequisite']
    prerequisite = blob(pre['published_pin'], pre['path'])
    need(hashlib.sha256(prerequisite).hexdigest() == pre['sha256'], 'port_c31')
    with tempfile.TemporaryDirectory(prefix='audit-t1d-identite-') as tmp:
        root = Path(tmp)
        base = root / 'base'
        for path, digest in capture['sources'].items():
            content = blob(capture['pin'], path)
            need(hashlib.sha256(content).hexdigest() == digest, path)
            dest = base / path
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(content)
        prefile = root / 'port.patch'
        prefile.write_bytes(prerequisite)
        for option in ('--check', None):
            subprocess.run(['git', 'apply'] + ([option] if option else []) + [str(prefile)], cwd=base, check=True)
        before = subprocess.check_output([sys.executable, '-B', '-S',
                                         str(base / PREFIX / 'g4_catalogue_t1d_selftest.py')], text=True).strip()
        candidate = root / 'proposed'
        shutil.copytree(base, candidate)
        patch = here / 'proposition.patch'
        for option in ('--check', None):
            subprocess.run(['git', 'apply'] + ([option] if option else []) + [str(patch)], cwd=candidate, check=True)
        for path, digest in capture['patched'].items():
            need(hashlib.sha256((candidate / path).read_bytes()).hexdigest() == digest, 'postimage ' + path)
        outputs = []
        for flags in ([], ['-O']):
            outputs.append(subprocess.check_output([sys.executable, '-B', '-S', *flags, str(here / 'check.py'),
                                                   '--child', str(candidate), str(base)], text=True))
        need(outputs[0] == outputs[1], 'normal/O')
        result = json.loads(outputs[0])
        result.update(pin=capture['pin'], port_preexistant=before, normal_optimise_identiques=True)
    expected = here / 'results.json'
    if expected.exists():
        need(json.loads(expected.read_text()) == result, 'resultats differents')
    print(json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
