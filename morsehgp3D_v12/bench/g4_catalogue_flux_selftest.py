#!/usr/bin/env python3
"""Auto-test du juge de la session G4 T2-d-C (g4_catalogue_flux_judge.py) par injections dans un rapport synthetique
fait de lignes NATIVES des sondes (memes cles et meme sequence que bench/catalogue_probe.cpp et bench/full_probe.cpp).
Un cas par defaut releve par l'auditeur Codex (receipts/audit_reponses_20261008/t2d_c_admission) en plus des cas
d'origine : passes hors commande, indices, raisons, champs absents, booleens, options, mutant sans comparaison ou sans
cause, borne arrondie, A/A. Porte CTest mhgp12_catalogue_g4_flux_judge (ligne juge_g4_t2dc_ok). Bibliotheque
standard, Python 3.10 nu, aucun assert.
"""
import copy
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import g4_catalogue_flux_judge as J  # noqa: E402
import g4_catalogue_flux_lecteur as L  # noqa: E402

BUDGET_LINE = 'device_open_budget : refus sous budget serre joues sur l\'appareil (10 appels)'
COUNTS = {'ng00': (39885, 1306696, 6097121, 1085776), 'ng01': (35551, 1200000, 5600000, 990000),
          'ng02': (45845, 1407885, 6514697, 1099582)}


def counts_of(case, k):
    if case in COUNTS:
        sites, balls, incidences, levels = COUNTS[case]
    else:
        sites = int(case[1:])
        balls, incidences, levels = 3 * sites, 9 * sites, 7
    return {'sites': sites, 'balls': balls * k // 5, 'incidences': incidences * k // 5, 'levels': levels * k // 5}


def catalogue_row(spec, p, wall, counts):
    diag = {k: 1 for k in L.DIAG_KEYS}
    diag.update(traversal_ns=wall // 10, count_ns=wall // 5, fill_ns=wall // 20)
    device = {k: 1 for k in L.DEVICE_KEYS}
    device.update(rewritten_host=0, transfer_ns=wall // 10, publish_ns=wall // 50)
    row = {'phase': 'catalogue', 'path': spec['path'], 'pass': p, 'status': 'ok', 'reason': 'none',
           'coord_bits': L.COORD_BITS, 'kmax': spec['k'], 'leaf': spec['leaf'], 'threads': spec['threads'],
           'wall_ns': wall, 'ledger': {k: 2 for k in L.LEDGER_KEYS}, 'diagnostics': diag, 'device': device}
    row.update(counts)
    return row


def catalogue_run(spec, walls, counts, digest='0' * 64, full=None):
    """Execution native conforme de la sonde du catalogue (code 0)."""
    rows = [{'phase': 'open', 'status': 'ok', 'reason': 'none', 'open_ns': 1000}] if spec['path'] == 'device' else []
    for p, wall in enumerate(walls):
        rows.append(catalogue_row(spec, p, wall, counts))
        if spec['sorties']:
            rows.append({'phase': 'sorties', 'pass': p, 'outputs_ns': wall // 20, 'outputs_bytes': 7,
                         'stream_chunks': 3})
        if spec['digest']:
            rows.append({'phase': 'digest', 'catalogue_sha256': digest})
        if spec['complet']:
            rows.append(dict({'phase': 'digest_complet', 'table_ecarts': 0},
                             **(full or {'niveaux_sha256': 'a' * 64, 'table_sha256': 'b' * 64})))
    rows.append({'phase': 'exit', 'status': 'ok', 'reason': 'none'})
    return {'code': 0, 'timeout': False, 'seconds': 1.0, 'bad_lines': 0, 'options': L.catalogue_options(spec),
            'rows': rows}


def full_run(case, k, threads, passes, digest):
    rows = [{'phase': 'open', 'status': 'ok', 'reason': 'none', 'wall_ns': 5, 'budget_appareil': 'partage'}]
    for p in range(passes):
        row = {'phase': 'full', 'pass': p, 'trame': case, 'voie': 'device', 'status': 'ok', 'coord_bits': L.COORD_BITS,
               'kmax': k, 'threads': threads, 'sites': counts_of(case, k)['sites'], 'wall_ns': 900,
               'pic_octets': 8, 'cpu_ns': None,
               'rss_max_octets': 9, 'appareil_octets': 1, 'epinglee_octets': 1, 'pic_appareil_octets': 0,
               'memoire_octets': {s: [1, 8] for s in L.MEM_STAGES}, 'full_sha256': digest}
        row.update({name: {x: 1 for x in keys} for name, keys in L.FULL_BLOCKS.items()})
        row['etapes_ns'].update(G=10, TMVR=10)
        rows += [row, {'phase': 'liberation', 'pass': p, 'liberation_ns': 3}]
    rows.append({'phase': 'exit', 'status': 'ok', 'reason': 'none'})
    return {'code': 0, 'timeout': False, 'seconds': 1.0, 'bad_lines': 0, 'options': L.full_options(k, threads, passes),
            'rows': rows}


def synthetic_report(ratio=0.8, factors=None, constant=False):
    """Rapport complet et coherent : apres = ratio * avant, ablations entre les deux ; constant : memes temps a chaque
    tour (bootstrap degenere, IC = le rapport exact)."""
    rounds, passes, threads = 10, 10, 48
    factor = {'avant': 1.0, 'avant_bis': 1.0, 'apres': ratio, 'flux_et_repli_selectif': (1 + ratio) / 2,
              'sans_anticipation': (1 + 2 * ratio) / 3, 'sans_double_tampon': (1 + 3 * ratio) / 4,
              'repli_cles_entieres': (1 + 4 * ratio) / 5}
    factor.update(factors or {})
    identity = []
    for case, k in J.IDENTITY_CASES:
        key, counts = '%s:%d' % (case, k), counts_of(case, k)
        identity.append({'case': case, 'k': k,
                         'cpu': catalogue_run(L.catalogue_spec('cpu', k, threads, 1, True, True), [10], counts,
                                              J.F2_DIGESTS[key]),
                         'device': catalogue_run(L.catalogue_spec('device', k, threads, 3, True, True, True),
                                                 [10, 10, 10], counts, J.F2_DIGESTS[key])})
    arms_identity = [{'arm': a, 'case': f, 'run': catalogue_run(L.catalogue_spec('device', 5, threads, 2, True),
                                                                [10, 10], counts_of(f, 5), J.F2_DIGESTS[f + ':5'])}
                     for a in J.BUILT_ARMS for f in J.FRAMES]
    ful1 = [{'arm': a, 'case': c, 'k': k,
             'run': full_run(c, k, threads, 2, J.FUL1_SESSION_K.get('%s:%d' % (c, k), 'c' * 64))}
            for a in ('avant', 'apres') for c, k in J.FUL1_CASES]
    campaign, seed = [], 7
    for r in range(rounds):
        for frame in J.FRAMES:
            for arm in J.ARMS:
                seed = (seed * 1103515245 + 12345) % (1 << 31)
                noise = 1.0 if constant else 1 + 0.01 * seed / (1 << 31)
                wall = int(35e6 * noise * factor[arm]) if not constant else int(100000000 * factor[arm])
                spec = L.catalogue_spec('device', 5, threads, passes, sorties=arm not in J.HISTORICAL)
                campaign.append({'round': r, 'frame': frame, 'arm': arm, 'position': 0,
                                 'run': catalogue_run(spec, [wall] * passes, counts_of(frame, 5))})
    shas = {a: 'd' * 64 for a in J.BUILT_ARMS}
    unit = {'code': 0, 'timeout': False}
    steps = {'environment': {'nvcc': 'nvcc 12.9', 'gpu': 'RTX PRO 6000'},
             'builds': {a: {'ok': True} for a in J.BUILT_ARMS},
             'gates': {'code': 0, 'timeout': False, 'device_open': dict(unit, stdout=L.DEVICE_OPEN_LINE + '\n'),
                       'device_open_budget': dict(unit, stdout=BUDGET_LINE + '\n')},
             'gpu_quiet_before': True, 'gpu_quiet_after': True, 'identity': identity, 'arms_identity': arms_identity,
             'ful1': ful1, 'campaign': campaign, 'binaries': dict(shas), 'binaries_after': dict(shas),
             'mutant': {'applied': True, 'built': True,
                        'run': catalogue_run(L.catalogue_spec('device', 5, threads, 2, digest=True), [10, 10],
                                             counts_of('ng00', 5), 'e' * 64)}}
    return {'options': {'rounds': rounds, 'passes': passes, 'threads': threads}, 'steps': steps}


def entry(report, step, **fields):
    for e in report['steps'][step]:
        if all(e.get(k) == v for k, v in fields.items()):
            return e
    return None


def rows_of(report, frame='ng00', arm='apres', r=3):
    return entry(report, 'campaign', round=r, frame=frame, arm=arm)['run']['rows']


def set_rows(report, rows, frame='ng00', arm='apres', r=3):
    entry(report, 'campaign', round=r, frame=frame, arm=arm)['run']['rows'] = rows


def foreign_output(report):
    """Sortie CPU, profil 18, K10, un fil, indices 0 repetes, raison memory_budget, sans ligne open : rendue pour une
    commande appareil, profil 21, K5, 48 fils (prelecture d'admission, defaut 1)."""
    rows = rows_of(report)
    for row in rows:
        if row.get('phase') == 'catalogue':
            row.update(path='cpu', coord_bits=18, kmax=10, threads=1, reason='memory_budget')
            row['pass'] = 0
    set_rows(report, rows[1:])


def foreign_field(field, value):
    """Un seul champ de la deuxieme passe hors de la commande (ligne open gardee)."""
    def edit(report):
        rows_of(report)[3][field] = value
    return edit


def missing_diagnostic(report):
    del rows_of(report)[3]['diagnostics']['traversal_ns']


def drop_sorties(report):
    rows = rows_of(report)
    set_rows(report, [row for row in rows if not (row.get('phase') == 'sorties' and row.get('pass') == 4)])


def boolean_ledger(report):
    rows_of(report)[1]['ledger']['emitted'] = True


def wrong_options(report):
    entry(report, 'campaign', round=3, frame='ng00', arm='apres')['run']['options'][2] = '--threads=1'


def repeated_index(report):
    rows = rows_of(report)
    rows[3]['pass'] = 0


def no_exit(report):
    set_rows(report, rows_of(report)[:-1])


def digest_first(report):
    rows = entry(report, 'identity', case='ng01', k=5)['device']['rows']
    rows[1], rows[3] = rows[3], rows[1]


def other_counts(report):
    for row in rows_of(report, 'ng02', 'sans_anticipation'):
        if row.get('phase') == 'catalogue':
            row['balls'] += 1


def mutant_rows(report, code, rows):
    run = report['steps']['mutant']['run']
    run['code'], run['rows'] = code, rows


OPEN = {'phase': 'open', 'status': 'ok', 'reason': 'none', 'open_ns': 1000}


def mutant_empty(report):
    mutant_rows(report, 0, [dict(OPEN), {'phase': 'exit', 'status': 'ok', 'reason': 'none'}])


def mutant_invariant(report):
    failed = {k: v for k, v in rows_of(report)[1].items() if k in L.CAT_BASE_KEYS}
    failed.update(threads=48, kmax=5, status='invariant_violated', reason='catalogue_invariant')
    failed['pass'] = 0
    mutant_rows(report, 3, [dict(OPEN), failed,
                            {'phase': 'exit', 'status': 'invariant_violated', 'reason': 'catalogue_invariant'}])


def mutant_no_cause(report):
    mutant_rows(report, 3, [dict(OPEN), {'phase': 'exit', 'status': 'ok', 'reason': 'none'}])


def mutant_signal(report):
    mutant_rows(report, -11, [])


def mutant_refusal(report):
    mutant_rows(report, 2, [dict(OPEN), {'phase': 'exit', 'status': 'resource_exhausted', 'reason': 'memory_budget'}])


def mutant_survivor(report):
    run = report['steps']['mutant']['run']
    for row in run['rows']:
        if row.get('phase') == 'digest':
            row['catalogue_sha256'] = J.F2_DIGESTS['ng00:5']


def ful1_options(report):
    entry(report, 'ful1', arm='apres', case='ng01', k=5)['run']['options'][1] = '--threads=1'


def budget_marker(report):
    report['steps']['gates']['device_open_budget']['stdout'] = 'device_open_budget : appareil indisponible\n'


def slower_frame(report):
    for e in report['steps']['campaign']:
        if e['frame'] == 'ng02' and e['arm'] == 'apres':
            for row in e['run']['rows']:
                if row.get('phase') == 'catalogue':
                    row['wall_ns'] = int(row['wall_ns'] * 1.3)


def set_digest(run, value):
    for row in run['rows']:
        if row.get('phase') == 'digest':
            row['catalogue_sha256'] = value


def cases():
    """(nom, rapport, verdict attendu) ; les cas marques d'un * sont ceux de la prelecture d'admission."""
    out = [('conforme', synthetic_report(), 'adopte'), ('apres_plus_lent', synthetic_report(ratio=1.03), 'rejete'),
           ('une_trame_plus_lente', synthetic_report(), 'rejete')]
    slower_frame(out[-1][1])
    edits = (
        ('prise_manquante', 'refuse', lambda r: r['steps']['campaign'].remove(
            entry(r, 'campaign', round=3, frame='ng01', arm='apres'))),
        ('identite_rompue', 'rejete', lambda r: entry(r, 'identity', case='ng00', k=5)['device']['rows'][3].update(
            catalogue_sha256='f' * 64)),
        ('niveaux_differents', 'rejete', lambda r: entry(r, 'identity', case='ng01', k=5)['device']['rows'][4].update(
            niveaux_sha256='f' * 64)),
        ('binaire_change', 'refuse', lambda r: r['steps']['binaries_after'].update(apres='e' * 64)),
        ('etapes_recouvertes', 'refuse', lambda r: rows_of(r)[3]['device'].update(publish_ns=10 ** 9)),
        ('mutant_survivant', 'rejete', mutant_survivor),
        ('ful1_differente', 'rejete', lambda r: [set_digest_full(e) for e in r['steps']['ful1']
                                                 if e['arm'] == 'apres' and e['case'] == 'u8000']),
        ('portes_en_echec', 'rejete', lambda r: r['steps']['gates'].update(code=8)),
        ('gpu_non_isole', 'refuse', lambda r: r['steps'].update(gpu_quiet_after=False)),
        ('bras_different', 'rejete', lambda r: set_digest(entry(r, 'arms_identity', arm='sans_double_tampon',
                                                                case='ng02')['run'], '1' * 64)),
        ('contrat_de_mesure', 'refuse', lambda r: r['options'].update(rounds=5)),
        ('*passes_hors_commande', 'refuse', foreign_output),
        ('*voie_cpu', 'refuse', foreign_field('path', 'cpu')),
        ('*profil_18', 'refuse', foreign_field('coord_bits', 18)),
        ('*k10', 'refuse', foreign_field('kmax', 10)),
        ('*un_fil', 'refuse', foreign_field('threads', 1)),
        ('*feuille_16', 'refuse', foreign_field('leaf', 16)),
        ('*raison_sous_statut_ok', 'refuse', foreign_field('reason', 'memory_budget')),
        ('*diagnostic_absent', 'refuse', missing_diagnostic),
        ('*indice_repete', 'refuse', repeated_index),
        ('*sorties_absente', 'refuse', drop_sorties),
        ('*booleen_pour_entier', 'refuse', boolean_ledger),
        ('*options_hors_commande', 'refuse', wrong_options),
        ('*exit_absente', 'refuse', no_exit),
        ('*sequence_rompue', 'refuse', digest_first),
        ('*comptes_d_une_autre_entree', 'refuse', other_counts),
        ('*mutant_sans_comparaison', 'refuse', mutant_empty),
        ('*mutant_code3_cause_lue', 'adopte', mutant_invariant),
        ('*mutant_code3_sans_cause', 'refuse', mutant_no_cause),
        ('*mutant_signal', 'adopte', mutant_signal),
        ('*mutant_refus_de_ressources', 'refuse', mutant_refusal),
        ('*ful1_hors_commande', 'refuse', ful1_options),
        ('*budget_appareil_non_joue', 'refuse', budget_marker),
    )
    for name, expected, edit in edits:
        report = synthetic_report()
        edit(report)
        out.append((name, report, expected))
    out += [('*borne_haute_0_99996_non_arrondie', synthetic_report(constant=True, factors={
                'apres': 0.99996, 'flux_et_repli_selectif': 1.0, 'sans_anticipation': 1.0,
                'sans_double_tampon': 1.0, 'repli_cles_entieres': 1.0}), 'adopte'),
            ('*aa_hors_fenetre', synthetic_report(factors={'avant_bis': 1.02}), 'refuse'),
            ('*aa_dans_la_fenetre', synthetic_report(factors={'avant_bis': 1.01}), 'adopte')]
    return out


def cohort_cases():
    """Cohorte de campagne (contre-lecture de l'auditeur, receipts/audit_reponses_20261008/t2dc_integration) : vide,
    prise manquante, doublon, tour, trame ou bras etranger, tour booleen ou non hachable ; une ancienne decision
    « adopte » stockee dans le rapport ne doit pas etre republiee par les tableaux."""
    edits = {
        'vide': lambda r: r['steps'].update(campaign=[]),
        'manquante': lambda r: r['steps']['campaign'].pop(),
        'doublon': lambda r: r['steps']['campaign'].append(copy.deepcopy(r['steps']['campaign'][0])),
        'tour_etranger': lambda r: r['steps']['campaign'][0].update(round=10),
        'trame_etrangere': lambda r: r['steps']['campaign'][0].update(frame='autre'),
        'bras_etranger': lambda r: r['steps']['campaign'][0].update(arm='autre'),
        'tour_booleen': lambda r: r['steps']['campaign'][0].update(round=False),
        'tour_non_hashable': lambda r: r['steps']['campaign'][0].update(round=[]),
    }
    out = []
    for name, edit in edits.items():
        report = synthetic_report()
        edit(report)
        report['verdict'] = {'verdict': 'adopte'}
        out.append(('*cohorte_' + name, report))
    return out


def set_digest_full(e):
    for row in e['run']['rows']:
        if row.get('phase') == 'full':
            row['full_sha256'] = '0' * 64


def selftest():
    failures = []
    all_cases = cases()
    for name, report, expected in all_cases:
        got = J.judge(copy.deepcopy(report))
        if got['verdict'] != expected:
            failures.append('%s : %s au lieu de %s (%s)' % (name, got['verdict'], expected,
                                                           (got['refused'] + got['rejected'])[:2]))
    import g4_catalogue_flux_tables as T  # noqa: E402  tableaux : meme cohorte que le juge
    for name, report in cohort_cases():
        got = J.judge(copy.deepcopy(report))
        rendered = T.tables(copy.deepcopy(report))
        if got['verdict'] != 'refuse' or '**refuse**' not in rendered or '| ng00 |' in rendered:
            failures.append('%s : verdict %s ou tableau agrege publie' % (name, got['verdict']))
    good = J.judge(all_cases[0][1])
    levers = good['stats']['levers']
    if levers['double_tampon']['verdict'] != 'adopte' or levers['A/A']['verdict'] != 'valide' or \
            good['stats'].get('mutant') != 'tue (empreinte)':
        failures.append('leviers ou mutant : verdicts inattendus')
    if failures:
        print('juge_g4_t2dc_ecart ' + ' ; '.join(failures))
        return 1
    print('juge_g4_t2dc_ok injections=%d admission=%d cohorte=%d' % (
        len(all_cases), sum(1 for c in all_cases if c[0].startswith('*')), len(cohort_cases())))
    return 0


if __name__ == '__main__':
    sys.exit(selftest())
