#!/usr/bin/env python3
"""MES-C3: frozen cohort/statistics, explicit overlapped-schema adapter; no engine.
Usage: check.py DEPOT RETOUR_C3 PLAN_JSON RAPPORT_C2
"""
import ast
from collections import Counter
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import statistics as st
import subprocess
import sys
import tempfile
import types

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
C = json.loads((HERE / 'capture.json').read_text())


def sha(b):
    return hashlib.sha256(b).hexdigest()


def need(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    repo, raw, plan_path, c2_path = map(Path, sys.argv[1:5])
    frozen = HERE.parent / 'mes_c_contrelecture'
    need(sha((frozen / 'reader.py').read_bytes()) == C['reader_sha256'], 'frozen reader')
    need(sha((frozen / 'capture.json').read_bytes()) == C['base_capture_sha256'], 'base cohort')
    sp = importlib.util.spec_from_file_location('frozen_mes_c', frozen / 'reader.py')
    audit = importlib.util.module_from_spec(sp)
    sp.loader.exec_module(audit)
    expected = audit.loads((frozen / 'capture.json').read_text())
    c2_bytes = (HERE.parent / 'session_c2_admission/capture.json').read_bytes()
    need(sha(c2_bytes) == C['c2_capture_sha256'], 'C2 capture')
    c2_capture = audit.loads(c2_bytes.decode())
    need(C['configuration'] == c2_capture['configuration'] and
         C['command_options'] == c2_capture['command_options'], 'cohort changed from C2')
    need(sha(plan_path.read_bytes()) == C['plan_sha256'], 'plan pin')
    argv = audit.loads(plan_path.read_text())['commands'][0]['argv'][2:]
    options = {k: None if '{' in v else v for k, v in zip(argv[::2], argv[1::2])}
    need(options == C['command_options'] and '--sequentiel' not in options, 'commanded schema/cohort')
    expected.update(commit=C['source_commit'], sources=C['sources'], configuration=C['configuration'],
                    command_options=C['command_options'])
    old_lf, reasons = audit.load_sources(repo, expected)
    source = lambda rev, p: subprocess.check_output(
        ['git', '-C', str(repo), 'show', rev + ':morsehgp3D_v12/' + p])
    pilot_path = 'microbancs/mes_c_petits/pilote_c.py'
    funcs = lambda b: {n.name: ast.dump(n, include_attributes=False) for n in ast.parse(b).body
                      if isinstance(n, ast.FunctionDef)}
    before = funcs(source(c2_capture['source_commit'], pilot_path))
    after = funcs(source(C['source_commit'], pilot_path))
    preserved = ['clouds_of', 'play_hard', 'session_values', 'overall', 'group_of', 'verdicts',
                 'play_session', 'tables', 'fit', 'fits_of']
    need(all(before[k] == after[k] for k in preserved), 'statistical/return-code policy changed')
    patch = HERE.parent / 'lf_recouvert_gardes/proposition.patch'
    need(sha(patch.read_bytes()) == C['lf_patch_sha256'], 'clock guards patch')
    with tempfile.TemporaryDirectory(prefix='audit-c3-reader-') as tmp:
        lf_path = Path(tmp) / 'morsehgp3D_v12/microbancs/outils/lecteur_full.py'
        lf_path.parent.mkdir(parents=True)
        lf_path.write_bytes(source(C['source_commit'], 'microbancs/outils/lecteur_full.py'))
        for check in (True, False):
            subprocess.run(['git', 'apply'] + (['--check'] if check else []) + [str(patch)],
                           cwd=tmp, check=True, capture_output=True)
        candidate = lf_path.read_bytes()
    need(sha(candidate) == C['lf_candidate_sha256'], 'clock guards postimage')
    lf = types.ModuleType('strengthened_lf')
    exec(compile(candidate, 'strengthened_lf', 'exec'), lf.__dict__)
    # Only schema and memory chronology change; frozen cohort, statistics and criteria remain intact.
    sequential_expected = audit.expected
    audit.expected = lambda spec, capture: dict(sequential_expected(spec, capture), schema='recouvert')
    audit.STAGES = ('P', 'C', 'tour')
    files = {str(p.relative_to(raw)): p.read_bytes() for p in sorted(raw.rglob('*')) if p.is_file()}
    need({n: sha(b) for n, b in files.items()} == C['returned_hashes'], 'returned inventory/pins')
    report = audit.loads(files['rapport_c.json'].decode())
    need(report['parametres']['schema'] == 'recouvert', 'declared schema')
    raws = {Path(n).name: b.decode('ascii') for n, b in files.items() if n.startswith('brut/')}
    old = audit.review(report, raws, expected, old_lf, reasons)
    admitted = audit.review(report, raws, expected, lf, reasons)
    need(old == admitted, 'legacy/strengthened reader delta')
    need(not admitted['differences'] and not admitted['controles'] and admitted['cohorte_complete'],
         'admission/coverage/report mismatch')
    need(sha(c2_path.read_bytes()) == C['c2_report_sha256'], 'C2 report')
    previous = audit.loads(c2_path.read_text())
    table, resources, comparison = {}, {}, {}
    matched = 0
    for key, value in admitted['configurations'].items():
        vals = value['valeurs']
        groups = {}
        old_values = previous['configurations'][key]['valeurs']
        need(set(vals) == set(old_values), 'comparison cloud cohort')
        for n, v in vals.items():
            need(all(v[k] == old_values[n][k] for k in ('sites', 'groupe', 'empreintes')), 'object/metadata delta')
            matched += 1
        for group in ('tous', 'reel', 'reel_le150'):
            selected = [(n, v) for n, v in vals.items() if group == 'tous' or
                        v['groupe'] == 'reel' and (group != 'reel_le150' or v['sites'] <= 150)]
            groups[group] = dict(nuages=len(selected), mediane_ns=st.median(v['chaud_ns'] for n, v in selected),
                                 maximum_ns=max(v['chaud_ns'] for n, v in selected))
            comparison.setdefault(key, {})[group] = dict(nuages=len(selected),
                avant_mediane_ns=st.median(old_values[n]['chaud_ns'] for n, v in selected),
                apres_mediane_ns=groups[group]['mediane_ns'],
                mediane_ratios_apres_avant=st.median(v['chaud_ns'] / old_values[n]['chaud_ns'] for n, v in selected))
        table[key] = dict(groupes=groups, droites=value['droites'])
        rows = [audit.loads(s) for s in raws['session_' + key.replace(':', '_') + '.jsonl'].splitlines()]
        full = [r for r in rows if r['phase'] == 'full']
        warm = full[147:]
        need(len(full) == 441 and len(warm) == 294, 'regular temperatures')
        resources[key] = dict(passes_chaudes=len(warm), cpu_chaud_total_ns=sum(x['cpu_ns'] for x in warm),
            pic_budget_hote_max=max(x['pic_octets'] for x in warm),
            pic_budget_appareil_max=max(x['pic_appareil_octets'] for x in warm),
            capacite_appareil_max=max(x['appareil_octets'] for x in warm), epinglee_max=max(x['epinglee_octets'] for x in warm),
            rss_processus_max=max(x['rss_max_octets'] for x in full))
    # Small adapter guards on a real admitted stream, not a new native invocation.
    spec = next(s for s in audit.specifications(expected) if s['key'] == 'cpu:5:4')
    rows = [audit.loads(s) for s in raws[spec['file']].splitlines()]
    pos = next(i for i, r in enumerate(rows) if r['phase'] == 'full')
    mutations = {
        'schema_sequentiel': lambda r: r.update(etapes_schema='sequentiel'),
        'tour_hors_mur': lambda r: r['recouvrement'].update(tour_ns=r['wall_ns'] + 1),
        'continuite_memoire_C_tour': lambda r: r['memoire_octets'].update(tour=[0, 0]),
        'identite_trame': lambda r: r.update(trame='etrangere'),
    }
    killed = {}
    for name, change in mutations.items():
        altered = copy.deepcopy(rows)
        change(altered[pos])
        text = '\n'.join(json.dumps(r) for r in altered) + '\n'
        try:
            state = audit.strict_process(0, text, spec, expected, lf, reasons)
            rejected = state['etat'] == 'illisible'
        except (audit.Invalid, KeyError, TypeError, ValueError):
            rejected = True
        need(rejected, 'adapter counterexample survived: ' + name)
        killed[name] = 'refuse'
    hard = []
    for e in report['difficiles']:
        x = {k: e[k] for k in ('nom', 'voie', 'k', 'fils', 'sites', 'etat', 'raison')}
        if e['etat'] == 'ok':
            x['chaud_ns'] = e['passes'][1]['mur_ns']
        hard.append(x)
    result = dict(source_commit=C['source_commit'], schema='recouvert', cache_default_octets=8 << 30,
        criteres=admitted['criteres'], verdict=admitted['verdict'], controles=admitted['controles'],
        differences=admitted['differences'], processus_prevus=admitted['processus_prevus'],
        processus_joues=admitted['processus_joues'], etats=dict(Counter(admitted['etats'].values())),
        cohorte_complete=admitted['cohorte_complete'], passes_completes=admitted['passes_completes'],
        passes_chaudes=admitted['passes_chaudes'], configurations=table, ressources=resources,
        difficiles=hard, comparaison_descriptive_C2=comparison, empreintes_C2_comparees=matched,
        codes=admitted['codes'], fonctions_pilote_preservees=preserved, contreflux=killed, native_calls=0)
    for n, b in files.items():
        need((raw / n).read_bytes() == b, 'returned evidence mutated')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
