#!/usr/bin/env python3
"""Contre-rejeu JSON du juge G capture ; aucune execution native du moteur."""
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
REL = 'morsehgp3D_v12/tests/tower/g_determinism.py'
OLD = 'morsehgp3D_v12/receipts/audit_reponses_20261007/g_juge_proposition/'
PRIOR = 'morsehgp3D_v12/receipts/audit_t2g_prepublication_20261007/check.py'


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def git_bytes(pin, path):
    return subprocess.check_output(['git', 'show', pin + ':' + path], cwd=ROOT)


def main():
    meta = json.loads((HERE / 'sources.json').read_text())
    files = [HERE / 'check.py', HERE / 'sources.json', HERE / 'exit_order_type.patch']
    files += [HERE / s['snapshot'] for s in meta['sources'].values()]
    before = {p.name: sha(p.read_bytes()) for p in files}
    for source in meta['sources'].values():
        need(before[source['snapshot']] == source['sha256'], 'capture modifiee')
    deps = {p: git_bytes(meta['main_pin'], p) for p in meta['pinned_dependencies']}
    need({p: sha(b) for p, b in deps.items()} == meta['pinned_dependencies'], 'dependance hors pin')
    prior = types.ModuleType('prior_factory')
    prior.__file__ = str(HERE / 'factory_not_executed.py')
    exec(compile(deps[PRIOR], PRIOR, 'exec'), prior.__dict__)
    header = deps['morsehgp3D_v12/bench/tower_export.hpp'].decode()
    names = re.findall(r'"([a-z_0-9]+)"', header.split('kCounterNames[kScalarCounters] = {', 1)[1].split('};', 1)[0])
    need(tuple(names) == prior.OBJECT + prior.WORK, 'compteurs differents du schema C++')
    good = [prior.order(k, 10) for k in range(1, 6)]
    cases = [('temoin_cinq_ordres', prior.payload(good), 0),
             ('seul_k1', prior.payload(good[:1]), 2),
             ('k3_absent', prior.payload([r for r in good if r['k'] != 3]), 2),
             ('k2_duplique_k3_absent', prior.payload([good[0], good[1], good[1], good[3], good[4]]), 2),
             ('digest_non_hexadecimal', prior.payload(good, digest='x' * 64), 2),
             ('digest_liste', prior.payload(good, digest=[]), 2)]
    cmake = (HERE / 'tests.cmake.snapshot').read_text()
    pinned = re.search(r'set\(tower_scale_8000 "([^\"]+)"\)', cmake).group(1)
    totals = dict(re.findall(r'(naissances|cellules|representants|cibles_cellule)=(\d+)', pinned))
    prefix = re.search(r'empreinte=([0-9a-f]+)', pinned).group(1)
    truncated = prior.order(1, 8000)
    truncated['objet'].update(births=int(totals['naissances']), cells=int(totals['cellules']),
                              representatives=int(totals['representants']))
    truncated['travail']['cell_stops'] = int(totals['cibles_cellule'])
    cases.append(('k1_tronque_totaux_wrapper', prior.payload([truncated], sites=8000,
                  digest=prefix + '0' * (64 - len(prefix))), 2))
    complete = prior.payload(good)
    complete[0]['diagnostics'] = dict.fromkeys(('count_ns', 'fill_ns', 'tables_ns', 'resolve_ns',
                                               'workspace_bytes', 'table_bytes', 'peak_bytes'), 0)
    complete[0]['diagnostics']['order_ns'] = [0] * 5
    cases.append(('format_complet_tower_probe', complete, 0))
    cases.append(('sites_dedup_deux_sur_dix', prior.payload([prior.order(k, 2) for k in (1, 2)], sites=2), 0))
    cases.append(('fils_dupliques', prior.payload(good), 2))
    cases.append(('fils_texte_ignore', prior.payload(good), 2))
    typed = prior.payload(good)
    typed[1]['objet']['births'] = True
    cases.append(('compteur_booleen', typed, 2))
    wrong = prior.payload(good)
    wrong[0]['kmax'] = 4
    cases.append(('k_stage_different', wrong, 2))
    cases.append(('tour_g_absent', prior.payload(good)[1:], 2))
    cases.append(('digest_suffixe_different', prior.payload(good), 1))
    final = dict(phase='exit', status='ok', reason='none', order=0)
    python = [sys.executable, '-B', '-S'] + (['-O'] if sys.flags.optimize else [])
    results, causal, residual, fixed_results = [], [], [], []
    with tempfile.TemporaryDirectory(prefix='audit-g-integration-json-') as tmp:
        folder = Path(tmp)
        old_meta = json.loads(deps[OLD + 'sources.json'])
        original = git_bytes(old_meta['pin'], REL)
        need(sha(original) == old_meta['sources_sha256'][REL], 'ancienne base differente')
        proposal = folder / REL
        proposal.parent.mkdir(parents=True)
        proposal.write_bytes(original)
        patch = folder / 'proposal.patch'
        patch.write_bytes(deps[OLD + 'proposition.patch'])
        need(sha(patch.read_bytes()) == old_meta['patch_sha256'], 'ancien patch different')
        subprocess.run(['git', 'apply', str(patch)], cwd=folder, check=True)
        need(sha(proposal.read_bytes()) == old_meta['proposed_judge_sha256'], 'proposition f7a differente')
        stub = folder / 'json_probe.py'
        stub.write_text('#!' + sys.executable + '\nimport json,pathlib,sys\n'
                        "d=json.loads(pathlib.Path(__file__).with_suffix('.json').read_text())\n"
                        "w=int(next(x[10:] for x in sys.argv if x.startswith('--threads=')))\n"
                        "for row in d['rows']:\n"
                        " if row.get('phase')=='tour_g':\n  row['threads']=w\n  row['pass']=row.pop('pass_index')\n"
                        " if d['vary'] and w!=1 and row.get('phase')=='digest': row['resolution_sha256']='0'*63+'1'\n"
                        " print(json.dumps(row))\n")
        stub.chmod(0o700)

        def run(name, rows, judge, expected, threads='1,48', sites=10, vary=False):
            stub.with_suffix('.json').write_text(json.dumps(dict(rows=rows, vary=vary)))
            args = [str(stub), name, '--uniform=%d,20261007,18' % sites, '--k=5', '--threads=' + threads]
            proc = subprocess.run(python + [str(judge)] + args, capture_output=True, text=True, timeout=10)
            need(proc.returncode == expected, (name, proc.returncode, expected, proc.stdout, proc.stderr))
            return dict(cas=name, code=proc.returncode, stdout=proc.stdout.strip(), stderr=proc.stderr,
                        json_sha256=sha(json.dumps(rows, sort_keys=True).encode()))

        integrated = HERE / 'g_determinism.snapshot.py'
        fixed = folder / 'fixed' / REL
        fixed.parent.mkdir(parents=True)
        fixed.write_bytes(integrated.read_bytes())
        need(sha((HERE / 'exit_order_type.patch').read_bytes()) == meta['exit_type_patch_sha256'],
             'patch du type different')
        subprocess.run(['git', 'apply', '--check', str(HERE / 'exit_order_type.patch')],
                       cwd=folder / 'fixed', check=True)
        subprocess.run(['git', 'apply', str(HERE / 'exit_order_type.patch')],
                       cwd=folder / 'fixed', check=True)
        need(sha(fixed.read_bytes()) == meta['exit_type_proposed_sha256'], 'correction de type differente')
        for name, rows, expected in cases:
            threads = '1,1' if name == 'fils_dupliques' else '1,x,48' if name == 'fils_texte_ignore' else '1,48'
            sites = 8000 if name == 'k1_tronque_totaux_wrapper' else 10
            results.append(run(name, rows + [copy.deepcopy(final)], integrated, expected,
                               threads=threads, sites=sites, vary=name == 'digest_suffixe_different'))
            checked = run(name, rows + [copy.deepcopy(final)], fixed, expected, threads=threads,
                          sites=sites, vary=name == 'digest_suffixe_different')
            need(checked == results[-1], 'correction de type change un temoin historique')
            fixed_results.append(dict(cas=name, code=checked['code'], output_identical=True))
        causal.append(run('f7a_ancien_format_sans_exit', complete, proposal, 0))
        causal.append(run('f7a_format_producteur_avec_exit', complete + [copy.deepcopy(final)], proposal, 2))
        causal.append(run('integration_sans_exit', complete, integrated, 2))
        for name, value in [('exit_order_booleen', False), ('exit_order_flottant', 0.0)]:
            altered = dict(final, order=value)
            residual.append(dict(cas=name, integrated=run(name, complete + [altered], integrated, 0),
                                 proposed=run(name, complete + [altered], fixed, 2)))
    need(before == {p.name: sha(p.read_bytes()) for p in files}, 'capture changee pendant le rejeu')
    print(json.dumps(dict(scope='JSON uniquement ; aucune execution native HGP',
                          source_sha256=meta['sources'][REL]['sha256'],
                          proposed_f7a_sha256=old_meta['proposed_judge_sha256'],
                          cases=results, proposal_erratum=causal, residual_type_validation=residual,
                          exit_type_proposal_cases=fixed_results,
                          ctest_printed_digest_characters=len(prefix), frozen_files_sha256=before),
                     ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
