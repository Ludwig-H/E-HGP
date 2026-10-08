#!/usr/bin/env python3
"""Auto-test du juge de la session G4 T1-d (g4_catalogue_t1d_judge.py) par injections dans un rapport synthetique de
lignes natives (memes generateurs que l'auto-test T2-d-C). Porte CTest mhgp12_catalogue_g4_t1d_judge (ligne
juge_g4_t1d_ok). Bibliotheque standard, Python 3.10 nu, aucun assert.
"""
import copy
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import g4_catalogue_flux_judge as J  # noqa: E402
import g4_catalogue_flux_lecteur as L  # noqa: E402
import g4_catalogue_flux_selftest as S  # noqa: E402
import g4_catalogue_t1d_judge as T  # noqa: E402

HELD = 8 << 30  # octets de l'appareil gardes par la voie complete (synthetique)


def tranches_rows(run, slices, streamed):
    """Ajoute la ligne << tranches >> apres chaque ligne catalogue (et sa ligne sorties s'il y en a)."""
    rows, out = run['rows'], []
    for row in rows:
        out.append(row)
        if row.get('phase') == 'catalogue':
            out.append({'phase': 'tranches', 'pass': row['pass'], 'finish_slices': slices,
                        'arena_streamed': streamed, 'device_bytes': HELD, 'device_peak': HELD})
    run['rows'] = out
    return run


def slices_step(threads, best=12):
    entries = []
    for case, k in T.SLICE_CASES:
        counts, digest = S.counts_of(case, k), J.F2_DIGESTS['%s:%d' % (case, k)]
        free = tranches_rows(S.catalogue_run(T.spec_free(k, threads), [10, 10], counts, digest), 0, 0)
        entries.append({'case': case, 'k': k, 'budget': 0, 'run': free})
        for i, (num, den) in enumerate(T.FRACTIONS):
            budget = HELD * num // den
            run = S.catalogue_run(T.spec_budget(k, threads, budget), [10, 10], counts, digest)
            entries.append({'case': case, 'k': k, 'budget': budget,
                            'run': tranches_rows(run, 2 + (best - 2) * i // (len(T.FRACTIONS) - 1), 1 + i)})
    return entries


def synthetic_report(ratio=0.999, aa=1.0, constant=False):
    base = S.synthetic_report(constant=constant)
    rounds, passes, threads = 10, 10, 48
    campaign, seed = [], 11
    factor = {'avant': 1.0, 'avant_bis': aa, 'apres': ratio}
    for r in range(rounds):
        for frame in J.FRAMES:
            for arm in T.ARMS:
                seed = (seed * 1103515245 + 12345) % (1 << 31)
                noise = 1.0 if constant else 1 + 0.002 * seed / (1 << 31)
                wall = int(100000000 * noise * factor[arm])
                campaign.append({'round': r, 'frame': frame, 'arm': arm, 'position': 0,
                                 'run': S.catalogue_run(T.spec_campaign(threads, passes), [wall] * passes,
                                                        S.counts_of(frame, 5))})
    steps = base['steps']
    shas = {a: 'd' * 64 for a in T.BUILT_ARMS}
    steps.update(campaign=campaign, slices=slices_step(threads), builds={a: {'ok': True} for a in T.BUILT_ARMS},
                 binaries=dict(shas), binaries_after=dict(shas))
    for key in ('arms_identity', 'mutant'):
        steps.pop(key, None)
    return base


def entry(report, step, **fields):
    return S.entry(report, step, **fields)


def slice_entry(report, case, k, index):
    budget = 0 if index < 0 else HELD * T.FRACTIONS[index][0] // T.FRACTIONS[index][1]
    return entry(report, 'slices', case=case, k=k, budget=budget)


def refuse_all(report, case, k):
    """Chaque prise sous budget refusee a sa premiere passe, comme la sonde : ligne catalogue en echec, puis exit."""
    for i in range(len(T.FRACTIONS)):
        e = slice_entry(report, case, k, i)
        first = {key: v for key, v in e['run']['rows'][1].items() if key in L.CAT_BASE_KEYS}
        first.update(status='resource_exhausted', reason='memory_budget')
        e['run']['code'] = 2
        e['run']['rows'] = [e['run']['rows'][0], first,
                            {'phase': 'exit', 'status': 'resource_exhausted', 'reason': 'memory_budget'}]


def edit_digest(report, case, k, index):
    for row in slice_entry(report, case, k, index)['run']['rows']:
        if row.get('phase') == 'digest':
            row['catalogue_sha256'] = '1' * 64


def cases():
    out = [('conforme', synthetic_report(), 'adopte'), ('apres_paie', synthetic_report(ratio=1.02), 'rejete'),
           ('borne_1_00999', synthetic_report(ratio=1.00999, constant=True), 'adopte'),
           ('borne_1_0101', synthetic_report(ratio=1.0101, constant=True), 'rejete'),
           ('aa_hors_fenetre', synthetic_report(aa=1.02), 'refuse')]
    edits = (
        ('flux_en_defaut', 'rejete', lambda r: edit_digest(r, 'ng01', 5, 1)),
        ('flux_tout_refuse', 'rejete', lambda r: refuse_all(r, 'ng02', 10)),
        ('flux_une_seule_tranche', 'rejete', lambda r: [
            row.update(finish_slices=1) for i in range(len(T.FRACTIONS))
            for row in slice_entry(r, 'ng01', 5, i)['run']['rows'] if row.get('phase') == 'tranches']),
        ('flux_k10_sans_lots_rapatries', 'rejete', lambda r: [
            row.update(arena_streamed=1) for case in T.FRAMES for i in range(len(T.FRACTIONS))
            for row in slice_entry(r, case, 10, i)['run']['rows'] if row.get('phase') == 'tranches']),
        ('flux_prise_manquante', 'refuse', lambda r: r['steps']['slices'].remove(slice_entry(r, 'ng00', 5, 2))),
        ('flux_budget_hors_commande', 'refuse', lambda r: slice_entry(r, 'ng01', 10, 0)['run']['options'].__setitem__(
            -1, '--budget-appareil=12345')),
        ('identite_rompue', 'rejete', lambda r: entry(r, 'identity', case='ng00', k=5)['device']['rows'][3].update(
            catalogue_sha256='f' * 64)),
        ('prise_manquante', 'refuse', lambda r: r['steps']['campaign'].remove(
            entry(r, 'campaign', round=2, frame='ng01', arm='apres'))),
        ('binaire_change', 'refuse', lambda r: r['steps']['binaries_after'].update(apres='e' * 64)),
        ('gpu_non_isole', 'refuse', lambda r: r['steps'].update(gpu_quiet_before=False)),
    )
    for name, expected, edit in edits:
        report = synthetic_report()
        edit(report)
        out.append((name, report, expected))
    return out


def selftest():
    failures = []
    all_cases = cases()
    for name, report, expected in all_cases:
        got = T.judge(copy.deepcopy(report))
        if got['verdict'] != expected:
            failures.append('%s : %s au lieu de %s (%s)' % (name, got['verdict'], expected,
                                                           (got['refused'] + got['rejected'])[:2]))
    if not T.ratio_check() or L.catalogue_options(T.spec_budget(5, 48, 7))[-1] != '--budget-appareil=7':
        failures.append('regle ou options')
    if failures:
        print('juge_g4_t1d_ecart ' + ' ; '.join(failures))
        return 1
    print('juge_g4_t1d_ok injections=%d' % len(all_cases))
    return 0


if __name__ == '__main__':
    sys.exit(selftest())
