#!/usr/bin/env python3
"""Juge de la session G4 de la tranche T1-b (voie appareil du catalogue), regle ecrite d'avance (docstring de
g4_catalogue_device.py, CONTRAT_CATALOGUE.md, paragraphe 10). Fonction pure du rapport : trois verdicts, << adopte >>,
<< rejete >>, << refuse >> ; auto-test par injections (--selftest-judge du pilote, porte CTest
mhgp12_catalogue_g4_judge). Bibliotheque standard, Python 3.10 nu, aucun assert.

Attendus graves : empreintes catalogue_digest de la voie CPU mesurees par la session F2 (receipts/g4_t1f_20261007,
nom de trame << probe >>, une par cas, identiques aux feuilles 16 et 24) et medianes chaudes publiees par F2 (comparaison
seulement, aucune decision).
"""
import copy
import statistics
import sys

BUDGET_NS = 45_000_000  # etage C a K5, borne haute du budget de 35 a 45 ms (CONTRAT_CATALOGUE.md, paragraphe 7)

CONTRACT = {
    'frames': ['ng00', 'ng01', 'ng02'],
    'identity_cases': [['ng00', 5, 24], ['ng00', 10, 24], ['ng01', 5, 24], ['ng01', 10, 24], ['ng02', 5, 24],
                       ['ng02', 10, 24], ['u8000', 5, 24], ['u16000', 5, 24], ['u32000', 5, 24]],
    'identity_passes': 3,
    'f2_cases': [['ng00', 5, 16], ['ng00', 5, 24], ['ng00', 10, 24], ['ng01', 5, 16], ['ng01', 5, 24],
                 ['ng01', 10, 24], ['ng02', 5, 16], ['ng02', 5, 24], ['ng02', 10, 24], ['u8000', 5, 24],
                 ['u16000', 5, 24], ['u32000', 5, 24]],
    'cpu_passes': 10,
    'processes': 5,
    'passes': 10,
    'threads': 48,
    'k10_processes': 3,
    'k10_passes': 5,
    'mutants': ['feuille_non_resolue_admise_sans_rejeu', 'fin_sans_departage_exact', 'un_fil_par_feuille'],
}

F2_DIGESTS = {
    'ng00:5': '698efd3d46e2d9c24363dca26eb76d293d78ddb46e4594d1fcb16eef2f1dd977',
    'ng00:10': '0f92e4f1187d88a0d5bcd94b5814aad4f40a6dc76ed1e598bfffac441639eaf7',
    'ng01:5': '12e7684173191378e43dcf306b87259111fbea427c5381829a447a563ce4d309',
    'ng01:10': '2c0018b203c65feee260535b6ce6d904a75276c674daa45e27d217a01db445c3',
    'ng02:5': 'd0578a530bd4247b873fbb99efac3038f2b9eb3f9c52da3a2de9e44821046c7c',
    'ng02:10': 'aea71fe1eea83fd15a7d70dd6fd27d7c18bf320ed40e75b00b2d893cecefeca7',
    'u8000:5': 'b38e265772e407cc49d2efc5c51d6f4d5ddb94d3397bddeebac6c6b9da8c2230',
    'u16000:5': '3838851969c0ca2511fd84265127e53b32f89e5fffaa942c3b5402c18ee345cc',
    'u32000:5': '2fe412a5066e83d9ca223bc4ade93f45dcd7eaee6a6a2017d47db750d7e12561',
}
F2_WARM_MS = {'ng00:5:16': 465, 'ng00:5:24': 452, 'ng00:10:24': 1750, 'ng01:5:16': 374, 'ng01:5:24': 373,
              'ng01:10:24': 1380, 'ng02:5:16': 468, 'ng02:5:24': 470, 'ng02:10:24': 1699, 'u8000:5:24': 158,
              'u16000:5:24': 327, 'u32000:5:24': 671}


def is_int(x):
    return isinstance(x, int) and not isinstance(x, bool)


def run_ok(run, passes):
    """Une sonde jouee en entier : passes attendues, toutes conformes, durees entieres, empreinte par passe."""
    if not isinstance(run, dict) or run.get('unreadable', 1) != 0:
        return False
    rows = run.get('passes') or []
    if len(rows) != passes or any(r.get('status') != 'ok' or not is_int(r.get('wall_ns')) for r in rows):
        return False
    return True


def code_class(code, timeout=False):
    """0 conforme ; 3 invariant (defaut du produit) ; tout le reste (refus, delai, signal) : preuve absente."""
    if timeout or code is None:
        return 'absent'
    return {0: 'ok', 3: 'defaut'}.get(code, 'absent')


def check_identity(steps, out):
    entries = {'%s:%d' % (e.get('case'), e.get('k')): e for e in steps.get('identity') or []}
    for case, k, _ in CONTRACT['identity_cases']:
        key = '%s:%d' % (case, k)
        e = entries.get(key)
        if e is None:
            out['refused'].append('identite : cas absent ' + key)
            continue
        classes = [code_class(e.get('cpu_code')), code_class(e.get('device_code'))]
        if 'absent' in classes:
            out['refused'].append('identite : sonde en echec ' + key)
            continue
        if 'defaut' in classes:
            out['rejected'].append('identite : invariant viole ' + key)
            continue
        cpu, dev = e.get('cpu') or {}, e.get('device') or {}
        if not run_ok(cpu, 1) or not run_ok(dev, CONTRACT['identity_passes']) or len(cpu.get('digests') or []) != 1 \
                or len(dev.get('digests') or []) != CONTRACT['identity_passes']:
            out['refused'].append('identite : sortie incomplete ' + key)
            continue
        ref = cpu['passes'][0]
        same = all(d == cpu['digests'][0] for d in dev['digests']) and all(
            p.get(f) == ref.get(f) for p in dev['passes'] for f in ('balls', 'incidences', 'levels', 'ledger'))
        if not same:
            out['rejected'].append('identite en defaut ' + key)
        if not physical_ok(cpu, dev):
            out['rejected'].append('identite : diagnostics physiques ou reprises mal comptes ' + key)
        if F2_DIGESTS.get(key) and cpu['digests'][0] != F2_DIGESTS[key]:
            out['rejected'].append('voie CPU differente de F2 ' + key)


# Diagnostics physiques comptes comme la voie CPU (paliers, etendue, reecritures) ; voie hybride : reprises sur l'hote
# par cause (plus de 32 sites, etendue au-dela de 16) et reecritures par lieu (appareil, reprise de l'hote).
PHYSICAL = ('leaves_narrow', 'leaves_medium', 'leaves_wide', 'leaves_exact', 'leaves_virtual_warp', 'leaves_rewritten',
            'max_leaf_span')
HYBRID = ('replayed_leaves', 'replayed_wide', 'replayed_span', 'rewritten_device', 'rewritten_host')


def physical_ok(cpu, dev):
    cd, dd, dv = cpu.get('diagnostics') or {}, dev.get('diagnostics') or {}, dev.get('device') or {}
    if not all(is_int(cd.get(f)) and cd.get(f) == dd.get(f) for f in PHYSICAL):
        return False
    if not all(is_int(dv.get(f)) for f in HYBRID):
        return False
    return dv['rewritten_device'] + dv['rewritten_host'] == dd['leaves_rewritten'] and \
        dv['replayed_wide'] == cd['leaves_virtual_warp'] and dv['replayed_span'] == cd['leaves_exact'] and \
        max(dv['replayed_wide'], dv['replayed_span']) <= dv['replayed_leaves'] <= dv['replayed_wide'] + dv['replayed_span']


def check_cpu_timing(steps, out):
    entries = {'%s:%d:%d' % (e.get('case'), e.get('k'), e.get('leaf')): e for e in steps.get('cpu_timing') or []}
    table = {}
    for case, k, leaf in CONTRACT['f2_cases']:
        key = '%s:%d:%d' % (case, k, leaf)
        e = entries.get(key)
        if e is None or code_class(e.get('code')) != 'ok' or not run_ok(e.get('run'), CONTRACT['cpu_passes']):
            out['refused'].append('voie CPU : mesure absente ou incomplete ' + key)
            continue
        digests = set(e['run'].get('digests') or [])
        if len(digests) != 1 or (F2_DIGESTS.get('%s:%d' % (case, k)) not in digests):
            out['rejected'].append('voie CPU : empreinte differente de F2 ' + key)
        warm = [p['wall_ns'] for p in e['run']['passes'][1:]]
        median_ms = statistics.median(warm) / 1e6
        table[key] = {'median_ms': round(median_ms, 3), 'max_ms': round(max(warm) / 1e6, 3),
                      'f2_median_ms': F2_WARM_MS[key], 'ratio_to_f2': round(median_ms / F2_WARM_MS[key], 4),
                      'stages': stage_table(e['run']['passes'][1:])}
    out['stats']['cpu'] = table


# Etapes publiees (CST-0235) : parcours, feuilles, emission, fin d'etage, transferts, publication, total, non ventile.
STAGES = {'parcours': ('traversal_ns',), 'feuilles': ('count_ns',), 'emission': ('fill_ns',),
          'fin_etage': ('levels_ns', 'sort_ns', 'assemble_ns', 'table_ns'), 'transferts': ('transfer_ns',),
          'publication': ('publish_ns',)}


def stage_table(rows):
    """Mediane et maximum de chaque etape sur les passes chaudes donnees (sommes calculees dans chaque passe) ;
    'incoherent' si une passe publie des etapes dont la somme depasse sa duree totale (etapes non disjointes)."""
    table = {}
    for name, keys in list(STAGES.items()) + [('total', None), ('non_ventile', None)]:
        values = []
        for r in rows:
            st = r.get('stages') or {}
            if name == 'total':
                values.append(r['wall_ns'])
            elif name == 'non_ventile':
                parts = [st.get(k) for ks in STAGES.values() for k in ks]
                if all(is_int(v) for v in parts):
                    values.append(r['wall_ns'] - sum(parts))
                    if sum(parts) > r['wall_ns']:
                        table['incoherent'] = True
            else:
                parts = [st.get(k) for k in keys]
                if all(is_int(v) for v in parts):
                    values.append(sum(parts))
        if values:
            table[name] = {'median_ms': round(statistics.median(values) / 1e6, 3), 'max_ms': round(max(values) / 1e6, 3)}
    return table


def frame_stats(runs, frame, processes, passes):
    mine = [r for r in runs if r.get('frame') == frame]
    if len(mine) < processes:
        return None
    warm, medians, cold, warm_rows = [], [], [], []
    for r in mine:
        if code_class(r.get('code')) != 'ok' or not run_ok(r.get('run'), passes):
            return None
        times = [p['wall_ns'] for p in r['run']['passes']]
        warm += times[1:]
        warm_rows += r['run']['passes'][1:]
        medians.append(statistics.median(times[1:]))
        cold.append(times[0])
    return {'processes': len(mine), 'warm_values': len(warm), 'median_ms': round(statistics.median(warm) / 1e6, 3),
            'max_process_median_ms': round(max(medians) / 1e6, 3), 'max_ms': round(max(warm) / 1e6, 3),
            'first_pass_median_ms': round(statistics.median(cold) / 1e6, 3), 'stages': stage_table(warm_rows),
            'held': statistics.median(warm) <= BUDGET_NS and max(medians) <= BUDGET_NS}


def check_device_timing(steps, out):
    budget = True
    for k, processes, passes in ((5, CONTRACT['processes'], CONTRACT['passes']),
                                 (10, CONTRACT['k10_processes'], CONTRACT['k10_passes'])):
        runs = steps.get('device_timing_k%d' % k) or []
        stats = {}
        for frame in CONTRACT['frames']:
            st = frame_stats(runs, frame, processes, passes)
            if st is None:
                out['refused'].append('temps appareil absents ou incomplets K%d %s' % (k, frame))
                continue
            stats[frame] = st
            if st['stages'].get('incoherent'):
                out['refused'].append('etapes non disjointes (somme au-dela du total) K%d %s' % (k, frame))
            if k == 5:
                budget = budget and st['held']
        out['stats']['device_k%d' % k] = stats
    if steps.get('gpu_quiet_before') is not True:
        out['refused'].append('GPU non isole avant les temps')
    return budget


def mutant_killed(m, reference):
    if m.get('critere') == 'temps':
        run = m.get('run') or {}
        if m.get('timeout'):
            return True  # budget perdu au-dela du delai
        rows = run.get('passes') or []
        if code_class(m.get('code')) != 'ok' or len(rows) < 2:
            return None
        return statistics.median([p['wall_ns'] for p in rows[1:]]) > BUDGET_NS
    unit = code_class(m.get('unit_code'), m.get('unit_timeout', False))
    ng00 = code_class(m.get('ng00_code'))
    if unit == 'absent' and ng00 == 'absent':
        return None
    differs = m.get('ng00_digest') is not None and reference is not None and m.get('ng00_digest') != reference
    return m.get('unit_code') in (1, 3) or ng00 == 'defaut' or differs


def check_mutants(report, out):
    if (report.get('options') or {}).get('skip_mutants'):
        out['refused'].append('mutants sautes')
        return
    entries = {m.get('id'): m for m in (report.get('steps') or {}).get('mutants') or []}
    reference = reference_digest(report)
    table = {}
    for mid in CONTRACT['mutants']:
        m = entries.get(mid)
        if m is None or not m.get('applied') or not (m.get('build') or {}).get('ok'):
            out['refused'].append('mutant non juge ' + str(mid))
            continue
        killed = mutant_killed(m, reference)
        table[mid] = killed
        if killed is None:
            out['refused'].append('mutant sans preuve ' + mid)
        elif not killed:
            out['rejected'].append('mutant survivant ' + mid)
    out['stats']['mutants'] = table


def reference_digest(report):
    for e in (report.get('steps') or {}).get('identity') or []:
        if e.get('case') == 'ng00' and e.get('k') == 5:
            digests = (e.get('cpu') or {}).get('digests') or []
            if digests:
                return digests[0]
    return F2_DIGESTS['ng00:5']


def judge(report):
    out = {'refused': [], 'rejected': [], 'stats': {}}
    steps = report.get('steps') or {}
    env = steps.get('environment') or {}
    if not env.get('nvcc') or not env.get('gpu'):
        out['refused'].append('outil absent (nvcc ou GPU)')
    opts = report.get('options') or {}
    if opts.get('processes', 0) < CONTRACT['processes'] or opts.get('passes', 0) < CONTRACT['passes'] or \
            opts.get('threads') != CONTRACT['threads']:
        out['refused'].append('contrat de mesure non respecte')
    if not (steps.get('build') or {}).get('ok'):
        out['refused'].append('construction en echec ou absente')
    gates = steps.get('gates') or {}
    if not is_int(gates.get('code')) or gates.get('timeout'):
        out['refused'].append('portes rapides non jouees')
    elif gates.get('code') != 0:
        out['rejected'].append('portes rapides en echec (code %s)' % gates.get('code'))
    if gates.get('device_open_code') != 0:
        out['rejected' if is_int(gates.get('device_open_code')) else 'refused'].append('device_open en echec')
    elif 'voie appareil jouee' not in (gates.get('device_open_stdout') or ''):
        out['refused'].append('device_open sans voie appareil jouee')
    check_identity(steps, out)
    check_cpu_timing(steps, out)
    budget = check_device_timing(steps, out)
    check_mutants(report, out)
    if out['refused']:
        verdict = 'refuse'
    elif out['rejected'] or not budget:
        verdict = 'rejete'
        if not budget:
            out['rejected'].append('budget de l etage C non tenu a K5')
    else:
        verdict = 'adopte'
    out['verdict'] = verdict
    return out


# ---------------------------------------------------------------------------------------------------- auto-test
FAKE_DIAG = {'leaves_narrow': 90, 'leaves_medium': 1, 'leaves_wide': 0, 'leaves_exact': 1, 'leaves_virtual_warp': 2,
             'leaves_rewritten': 7, 'max_leaf_span': 17}
FAKE_DEVICE = {'replayed_leaves': 3, 'replayed_wide': 2, 'replayed_span': 1, 'rewritten_device': 5,
               'rewritten_host': 2}


def fake_run(passes, wall_ns, digest, ledger=None):
    stages = {'traversal_ns': 2_000_000, 'count_ns': 8_000_000, 'fill_ns': 3_000_000, 'levels_ns': 1_000_000,
              'sort_ns': 2_000_000, 'assemble_ns': 2_000_000, 'table_ns': 1_000_000, 'transfer_ns': 6_000_000,
              'publish_ns': 1_000_000}
    rows = [{'status': 'ok', 'reason': 'none', 'wall_ns': wall_ns, 'balls': 10, 'incidences': 40, 'levels': 9,
             'ledger': ledger or {'nodes': 5}, 'stages': dict(stages)} for _ in range(passes)]
    return {'passes': rows, 'digests': [digest] * passes, 'unreadable': 0, 'diagnostics': dict(FAKE_DIAG),
            'device': dict(FAKE_DEVICE)}


def good_report():
    steps = {'environment': {'nvcc': 'nvcc 12.9', 'gpu': 'RTX PRO 6000'}, 'build': {'ok': True},
             'gates': {'code': 0, 'timeout': False, 'device_open_code': 0,
                       'device_open_stdout': 'device_open : voie appareil jouee sur 9 temoins'},
             'gpu_quiet_before': True, 'gpu_quiet_after': True}
    identity = []
    for case, k, leaf in CONTRACT['identity_cases']:
        d = F2_DIGESTS['%s:%d' % (case, k)]
        identity.append({'case': case, 'k': k, 'leaf': leaf, 'cpu_code': 0, 'device_code': 0,
                         'cpu': fake_run(1, 400_000_000, d), 'device': fake_run(3, 30_000_000, d)})
    steps['identity'] = identity
    steps['cpu_timing'] = [{'case': c, 'k': k, 'leaf': f, 'code': 0, 'run': fake_run(10, 200_000_000,
                                                                                      F2_DIGESTS['%s:%d' % (c, k)])}
                           for c, k, f in CONTRACT['f2_cases']]
    for k, processes, passes, t in ((5, 5, 10, 38_000_000), (10, 3, 5, 150_000_000)):
        steps['device_timing_k%d' % k] = [{'frame': f, 'process': p, 'code': 0, 'run': fake_run(passes, t, 'x')}
                                          for p in range(processes) for f in CONTRACT['frames']]
    steps['mutants'] = [
        {'id': 'feuille_non_resolue_admise_sans_rejeu', 'critere': 'identite', 'applied': True, 'build': {'ok': True},
         'unit_code': 1, 'ng00_code': 0, 'ng00_digest': 'autre'},
        {'id': 'fin_sans_departage_exact', 'critere': 'identite', 'applied': True, 'build': {'ok': True},
         'unit_code': 1, 'ng00_code': 0, 'ng00_digest': F2_DIGESTS['ng00:5']},
        {'id': 'un_fil_par_feuille', 'critere': 'temps', 'applied': True, 'build': {'ok': True}, 'code': 0,
         'run': fake_run(4, 300_000_000, F2_DIGESTS['ng00:5'])}]
    return {'options': {'processes': 5, 'passes': 10, 'threads': 48, 'skip_mutants': False}, 'steps': steps}


def injections():
    """(nom, transformation du rapport, verdict attendu)."""
    def slow_ng01(r):
        for run in r['steps']['device_timing_k5']:
            if run['frame'] == 'ng01':
                for p in run['run']['passes']:
                    p['wall_ns'] = 46_000_000

    def one_slow_process(r):
        for p in r['steps']['device_timing_k5'][4]['run']['passes']:
            p['wall_ns'] = 60_000_000

    def digest_u16000(r):
        r['steps']['identity'][7]['device']['digests'][1] = 'ecart'

    def ledger_ng02(r):
        r['steps']['identity'][4]['device']['passes'][2]['ledger'] = {'nodes': 6}

    def missing_case(r):
        del r['steps']['identity'][5]

    def survivor(r):
        r['steps']['mutants'][2]['run'] = fake_run(4, 30_000_000, 'x')

    def cpu_changed(r):
        r['steps']['identity'][0]['cpu']['digests'][0] = 'autre'
        r['steps']['identity'][0]['device']['digests'] = ['autre'] * 3

    def gates_red(r):
        r['steps']['gates']['code'] = 8

    def no_gpu_path(r):
        r['steps']['gates']['device_open_stdout'] = 'device_open : appareil indisponible'

    def busy_gpu(r):
        r['steps']['gpu_quiet_before'] = False

    def four_processes(r):
        r['options']['processes'] = 4

    def skipped(r):
        r['options']['skip_mutants'] = True

    def crashed_probe(r):
        r['steps']['identity'][2]['device_code'] = None

    def invariant(r):
        r['steps']['identity'][3]['device_code'] = 3

    def incomplete_passes(r):
        r['steps']['device_timing_k5'][0]['run']['passes'].pop()

    def rewrites_lost(r):
        r['steps']['identity'][1]['device']['device']['rewritten_device'] = 0

    def stages_overlap(r):
        r['steps']['device_timing_k5'][3]['run']['passes'][4]['stages']['transfer_ns'] = 30_000_000

    return [('conforme', lambda r: None, 'adopte'), ('budget_ng01', slow_ng01, 'rejete'),
            ('processus_lent', one_slow_process, 'rejete'), ('empreinte_u16000', digest_u16000, 'rejete'),
            ('grand_livre_ng02', ledger_ng02, 'rejete'), ('cas_absent', missing_case, 'refuse'),
            ('mutant_survivant', survivor, 'rejete'), ('voie_cpu_changee', cpu_changed, 'rejete'),
            ('portes_rouges', gates_red, 'rejete'), ('sans_voie_appareil', no_gpu_path, 'refuse'),
            ('gpu_occupe', busy_gpu, 'refuse'), ('quatre_processus', four_processes, 'refuse'),
            ('mutants_sautes', skipped, 'refuse'), ('sonde_tuee', crashed_probe, 'refuse'),
            ('invariant_appareil', invariant, 'rejete'), ('passe_manquante', incomplete_passes, 'refuse'),
            ('reecritures_perdues', rewrites_lost, 'rejete'), ('etapes_recouvertes', stages_overlap, 'refuse')]


def selftest():
    failures = 0
    cases = injections()
    for name, change, expected in cases:
        report = copy.deepcopy(good_report())
        change(report)
        got = judge(report)['verdict']
        ok = got == expected
        failures += 0 if ok else 1
        print('%s juge=%s attendu=%s %s' % (name, got, expected, 'ok' if ok else 'ECART'))
    if failures:
        print('juge_g4_t1b_ecarts %d' % failures)
        return 1
    print('juge_g4_t1b_ok injections=%d' % len(cases))
    return 0


if __name__ == '__main__':
    sys.exit(selftest())
