#!/usr/bin/env python3
"""Porte du lecteur partage des sorties de la sonde FULL (lecteur_full.py, MES-B et MES-FULL), sans sonde ni donnees
reelles. Python 3.10 nu, aucun assert (tient sous -O).

  lecture        une sortie appareil conforme est admise ; vingt-trois mutations du schema (dont cinq de la
                 memoire par etage : etage absent, pic incoherent avec pic_octets, usage au-dela du pic, booleen, pic
                 d'un etage sous l'usage a la fin du precedent ;
                 et le mur nul de la contrelecture de livraison de MES-B, refuse avant toute statistique) et les cinq
                 corruptions de la prelecture de l'auditeur (cle repetee, booleen, ouverture, 2^64, NaN) sont
                 refusees comme sorties illisibles (controle manquant), jamais comme resultats ;
  issues         refus de la sonde (code 2, ressources ou degenerescence) publie comme resultat, avec ses passes deja
                 jouees ; invariant viole (codes 2 et 3), signal et expiration publies comme echecs du cas ;
  recouvert      schema de la Session recouverte (voie par defaut de la sonde depuis T2-d-A) : sortie conforme admise,
                 vingt-cinq incoherences refusees (schema, raccord, partition, fenetres, ouverture, memoire dont le pic
                 de la tour sous l'usage a la fin de C, recouvrement, fins par ordre, et les neuf corruptions
                 d'horloges de l'auditeur : tour hors du mur, ouvertures differentes, fins de G avant l'ouverture,
                 noyau avant G, M avant le noyau, R avant M, V avant M, V avant M de l'ordre inferieur, maximum des
                 G faux), et aucun melange des deux schemas ;
  session        plusieurs trames en alternance (passe p = trame p modulo n, comme la Session de MES-FULL) : admises
                 dans l'ordre, refusees permutees ; budget de l'appareil attendu « partage » (MES-FULL) ou « separe »
                 (MES-B), l'autre refuse ; empreinte absente quand elle n'est pas demandee, refusee si elle l'est.
Codes : 0 conforme ; 1 ecart.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lecteur_full as lf  # noqa: E402

SITES = 2_000_000
LABEL = 'scene'
SHA = 'ab' * 32
ATTENDU = dict(voie='appareil', k=5, fils=48, passes=2, empreinte=True, trames=[(LABEL, SITES)],
               budget_appareil='separe')


def full_row(i, wall=4_000_000_000):
    return dict(phase='full', pass_=i, trame=LABEL, voie='device', status='ok', coord_bits=21, kmax=5, threads=48,
                sites=SITES, wall_ns=wall,
                etapes_ns=dict(P=1, C=wall // 4, G=wall // 4, raccord=1, TMVR=wall // 4, T=wall // 8, M=1, V=1, R=1),
                c_ns={k: 1 for k in lf.C_KEYS}, g_ns=dict(tables=1, resolution=wall // 8),
                hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=10, cpu_ns=5, rss_max_octets=10,
                appareil_octets=7, epinglee_octets=3, pic_appareil_octets=8, full_sha256=SHA,
                memoire_octets=dict(P=[1, 2], C=[4, 10], G=[6, 7], raccord=[6, 6], TMVR=[8, 9]))


def dump(rows):
    lines = []
    for row in rows:
        row = dict(row)
        if 'pass_' in row:
            row = {('pass' if k == 'pass_' else k): v for k, v in row.items()}
        lines.append(json.dumps(row, ensure_ascii=False))
    return '\n'.join(lines) + '\n'


def device_output(passes=2, end=None, open_row=None, mutate=None):
    rows = [open_row or dict(phase='open', status='ok', reason='none', wall_ns=5, budget_appareil='separe')]
    for i in range(passes):
        row = full_row(i)
        if mutate is not None and i == passes - 1:
            mutate(row)
        rows += [row, dict(phase='liberation', pass_=i, liberation_ns=3)]
    rows.append(end or dict(phase='exit', status='ok', reason='none'))
    return dump(rows)


def check_reading(errors):
    state = lf.parse_output(0, device_output(), ATTENDU)
    if state['etat'] != 'ok' or len(state['passes']) != 2:
        errors.append('lecture : sortie conforme refusee (%s)' % state['raison'])
    mutations = {
        'cle_en_trop': lambda r: r.update(extra=1),
        'cle_absente': lambda r: r.pop('cpu_ns'),
        'booleen': lambda r: r.update(pic_octets=True),
        'negatif': lambda r: r.update(cpu_ns=-1),
        'sites': lambda r: r.update(sites=SITES - 1),
        'voie': lambda r: r.update(voie='cpu'),
        'fils': lambda r: r.update(threads=47),
        'profil': lambda r: r.update(coord_bits=24),
        'trame': lambda r: r.update(trame='autre'),
        'empreinte': lambda r: r.update(full_sha256='AB' * 32),
        'mur': lambda r: r['etapes_ns'].update(P=r['wall_ns']),
        'tmvr': lambda r: r['etapes_ns'].update(T=r['etapes_ns']['TMVR'] + 1),
        'g': lambda r: r['g_ns'].update(tables=r['etapes_ns']['G']),
        'ascii': lambda r: r.update(trame='scène'),
        'memoire_etage_absent': lambda r: r['memoire_octets'].pop('G'),
        'memoire_pic_incoherent': lambda r: r['memoire_octets'].update(C=[4, 9]),
        'memoire_usage_sup_pic': lambda r: r['memoire_octets'].update(TMVR=[9, 8]),
        'memoire_booleen': lambda r: r['memoire_octets'].update(P=[True, 2]),
        'memoire_pic_sous_usage_precedent': lambda r: r['memoire_octets'].update(C=[8, 10]),
        'mur_nul': lambda r: (r.update(wall_ns=0), r['etapes_ns'].update({k: 0 for k in r['etapes_ns']}),
                              r['g_ns'].update(tables=0, resolution=0)),
    }
    for name, mutate in mutations.items():
        state = lf.parse_output(0, device_output(mutate=mutate), ATTENDU)
        if state['etat'] != 'illisible':
            errors.append('lecture : mutation %s rendue %s' % (name, state['etat']))
    text = device_output().replace('{"phase": "liberation", "pass": 1, "liberation_ns": 3}\n', '')
    if lf.parse_output(0, text, ATTENDU)['etat'] != 'illisible':
        errors.append('lecture : liberation absente admise')
    shared = dict(phase='open', status='ok', reason='none', wall_ns=5, budget_appareil='partage')
    if lf.parse_output(0, device_output(open_row=shared), ATTENDU)['etat'] != 'illisible':
        errors.append('lecture : budget de l\'appareil partage admis')
    cpu_case = dict(ATTENDU, voie='cpu')
    text = device_output().replace('"voie": "device"', '"voie": "cpu"').split('\n', 1)[1]
    if lf.parse_output(0, text, cpu_case)['etat'] != 'illisible':
        errors.append('lecture : memoire de l\'appareil admise sur la voie CPU')
    # Corruptions de la contrelecture de l'auditeur Codex (mes_b_prelecture) : cle repetee, booleen comme rang de
    # liberation, ouverture ok avec une raison, entier hors de u64, constante non finie.
    good = device_output()
    corrupt = {
        'cle_repetee': good.replace('"cpu_ns": 5,', '"cpu_ns": 5, "cpu_ns": 5,', 1),
        'liberation_booleenne': good.replace('{"phase": "liberation", "pass": 0,',
                                             '{"phase": "liberation", "pass": false,'),
        'ouverture_raison': good.replace('"reason": "none", "wall_ns": 5', '"reason": "device_fault", "wall_ns": 5'),
        'entier_2_64': good.replace('"cpu_ns": 5,', '"cpu_ns": 18446744073709551616,', 1),
        'constante_nan': good.replace('"cpu_ns": 5,', '"cpu_ns": NaN,', 1),
    }
    for name, text in corrupt.items():
        if text == good or lf.parse_output(0, text, ATTENDU)['etat'] != 'illisible':
            errors.append('lecture : corruption %s admise' % name)


def check_outcomes(errors):
    refused = device_output(passes=1, end=dict(phase='exit', status='resource_exhausted', reason='memory_budget'))
    state = lf.parse_output(2, refused, ATTENDU)
    if state['etat'] != 'refus' or state['raison'] != 'resource_exhausted/memory_budget' or len(state['passes']) != 1:
        errors.append('issues : refus de ressources mal lu (%s)' % state)
    degenerate = device_output(passes=0, end=dict(phase='exit', status='unsupported_degeneracy', reason='wide_leaf'))
    if lf.parse_output(2, degenerate, ATTENDU)['etat'] != 'refus':
        errors.append('issues : refus de degenerescence mal lu')
    broken = device_output(passes=0, end=dict(phase='exit', status='invariant_violated', reason='tower_invariant'))
    for code in (3, 2):
        if lf.parse_output(code, broken, ATTENDU)['etat'] != 'echec':
            errors.append('issues : invariant viole (code %d) non publie comme echec' % code)
    for code, label in ((-9, 'signal'), ('expire', 'expiration')):
        if lf.parse_output(code, '', ATTENDU)['etat'] != 'echec':
            errors.append('issues : %s non publie comme echec' % label)
    if lf.parse_output(0, device_output(passes=1), ATTENDU)['etat'] != 'echec':
        errors.append('issues : passes manquantes avec sortie ok admises')


def check_session(errors):
    """Trames en alternance (Session de MES-FULL), budget partage, empreinte non demandee."""
    def row(i, label, sites):
        r = full_row(i)
        r.update(trame=label, sites=sites)
        return r
    frames = [('a', 1000), ('b', 2000)]
    attendu = dict(ATTENDU, passes=4, trames=frames, budget_appareil='partage')
    def output(order, budget='partage', digest=True):
        rows = [dict(phase='open', status='ok', reason='none', wall_ns=5, budget_appareil=budget)]
        for i, (label, sites) in enumerate(order):
            r = row(i, label, sites)
            if not digest:
                r.pop('full_sha256')
            rows += [r, dict(phase='liberation', pass_=i, liberation_ns=3)]
        rows.append(dict(phase='exit', status='ok', reason='none'))
        return dump(rows)
    good = output(frames * 2)
    if lf.parse_output(0, good, attendu)['etat'] != 'ok':
        errors.append('session : alternance conforme refusee (%s)' % lf.parse_output(0, good, attendu)['raison'])
    if lf.parse_output(0, output([frames[1], frames[0]] * 2), attendu)['etat'] != 'illisible':
        errors.append('session : trames permutees admises')
    if lf.parse_output(0, output(frames * 2, budget='separe'), attendu)['etat'] != 'illisible':
        errors.append('session : budget separe admis quand le partage est attendu')
    plain = dict(attendu, empreinte=False)
    if lf.parse_output(0, output(frames * 2, digest=False), plain)['etat'] != 'ok':
        errors.append('session : sortie sans empreinte refusee quand elle n\'est pas demandee')
    if lf.parse_output(0, output(frames * 2, digest=True), plain)['etat'] != 'illisible':
        errors.append('session : empreinte presente admise quand elle n\'est pas demandee')
    if lf.parse_output(0, output(frames * 2, digest=False), attendu)['etat'] != 'illisible':
        errors.append('session : empreinte absente admise quand elle est demandee')


def overlapped_row(i, wall=4_000_000_000):
    """Ligne "full" du schema recouvert (voie par defaut de la sonde depuis T2-d-A), K = 5."""
    g, queue = wall // 2, wall // 10
    return dict(phase='full', pass_=i, trame=LABEL, voie='device', status='ok', etapes_schema='recouvert',
                coord_bits=21, kmax=5, threads=48, sites=SITES, wall_ns=wall,
                etapes_ns=dict(P=1, C=wall // 4, G=g, raccord=0, TMVR=queue),
                fenetres_ns=dict(G=5 * g, foret=4 * g, foret_apres_g=queue, T=g, M=g // 2, V=g // 4, R=g),
                c_ns={k: 1 for k in lf.C_KEYS}, g_ns=dict(ouverture=g // 10, tables=g // 20),
                hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=12, cpu_ns=5, rss_max_octets=10,
                appareil_octets=7, epinglee_octets=3, pic_appareil_octets=8, full_sha256=SHA,
                memoire_octets=dict(P=[1, 2], C=[4, 10], tour=[8, 12]),
                recouvrement=dict(tour_ns=g + queue + 1, ouverture_ns=g // 10, fin_g_ns=g, fin_ns=g + queue,
                                  queue_ns=queue, noyau_reprises=18, noyau_arrets=13, admis_octets=99),
                fins_par_ordre_ns=[[g, g + 1, g + 2, 0, g + 3]] + [[g - 10 * k, g, g + 1, g + 2, g + 3]
                                                                   for k in range(1, 5)])


def check_overlapped(errors):
    """Schema recouvert : sortie conforme admise, coherences du recouvrement exigees, schemas non melanges."""
    attendu = dict(ATTENDU, schema='recouvert')
    def output(mutate=None, row_maker=overlapped_row):
        rows = [dict(phase='open', status='ok', reason='none', wall_ns=5, budget_appareil='separe')]
        for i in range(2):
            row = row_maker(i)
            if mutate is not None and i == 1:
                mutate(row)
            rows += [row, dict(phase='liberation', pass_=i, liberation_ns=3)]
        rows.append(dict(phase='exit', status='ok', reason='none'))
        return dump(rows)
    state = lf.parse_output(0, output(), attendu)
    if state['etat'] != 'ok':
        errors.append('recouvert : sortie conforme refusee (%s)' % state['raison'])
    mutations = {
        'schema_absent': lambda r: r.pop('etapes_schema'),
        'raccord_non_nul': lambda r: r['etapes_ns'].update(raccord=1),
        'partition_hors_mur': lambda r: r['etapes_ns'].update(P=r['wall_ns']),
        'fenetres_hors_foret': lambda r: r['fenetres_ns'].update(T=r['fenetres_ns']['foret']),
        'ouverture_hors_g': lambda r: r['g_ns'].update(ouverture=r['etapes_ns']['G'] + 1),
        'memoire_tour_absente': lambda r: r['memoire_octets'].pop('tour'),
        'memoire_pic_incoherent': lambda r: r['memoire_octets'].update(tour=[8, 11]),
        'memoire_pic_sous_usage_precedent': lambda r: r['memoire_octets'].update(P=[11, 11]),
        'queue_incoherente': lambda r: r['recouvrement'].update(queue_ns=r['recouvrement']['queue_ns'] + 1),
        'tmvr_hors_queue': lambda r: r['etapes_ns'].update(TMVR=r['etapes_ns']['TMVR'] + 1),
        'fin_g_incoherente': lambda r: r['recouvrement'].update(fin_g_ns=r['recouvrement']['fin_g_ns'] + 1),
        'arrets_sup_reprises': lambda r: r['recouvrement'].update(noyau_arrets=19),
        'admission_nulle': lambda r: r['recouvrement'].update(admis_octets=0),
        'fins_ordres_manquants': lambda r: r['fins_par_ordre_ns'].pop(),
        'verticales_ordre_1': lambda r: r['fins_par_ordre_ns'][0].__setitem__(3, 1),
        'fin_g_ordre_tardive': lambda r: r['fins_par_ordre_ns'][2].__setitem__(0, r['recouvrement']['fin_g_ns'] + 1),
        # Gardes des horloges et dependances (auditeur, lf_recouvert_gardes) : P + C + tour dans le mur, deux
        # ouvertures egales, ouverture <= G <= noyau <= M <= R par ordre, V(k) apres M(k) et M(k-1), maximum des G.
        'tour_hors_du_mur': lambda r: r['recouvrement'].update(tour_ns=r['wall_ns']),
        'ouvertures_differentes': lambda r: r['recouvrement'].update(ouverture_ns=0),
        'fins_g_avant_ouverture': lambda r: [e.__setitem__(0, 0) for e in r['fins_par_ordre_ns']],
        'noyau_avant_g': lambda r: r['fins_par_ordre_ns'][0].__setitem__(1, 0),
        'm_avant_noyau': lambda r: r['fins_par_ordre_ns'][0].__setitem__(2, 0),
        'r_avant_m': lambda r: r['fins_par_ordre_ns'][0].__setitem__(4, 0),
        'v_avant_m': lambda r: r['fins_par_ordre_ns'][1].__setitem__(3, 0),
        # M(3) avance d'une unite (toujours avant R(3) et V(3)) ; V(4) = M(4) < M(3) : seule la dependance a l'ordre
        # inferieur est violee.
        'v_avant_m_ordre_inferieur': lambda r: (r['fins_par_ordre_ns'][2].__setitem__(2, r['fins_par_ordre_ns'][2][3]),
                                               r['fins_par_ordre_ns'][3].__setitem__(3, r['fins_par_ordre_ns'][3][2])),
        'maximum_g_faux': lambda r: [e.__setitem__(0, r['recouvrement']['ouverture_ns'])
                                     for e in r['fins_par_ordre_ns']],
    }
    for name, mutate in mutations.items():
        state = lf.parse_output(0, output(mutate), attendu)
        if state['etat'] != 'illisible':
            errors.append('recouvert : mutation %s rendue %s' % (name, state['etat']))
    if lf.parse_output(0, output(row_maker=full_row), attendu)['etat'] != 'illisible':
        errors.append('recouvert : ligne sequentielle admise quand le schema recouvert est attendu')
    if lf.parse_output(0, output(), ATTENDU)['etat'] != 'illisible':
        errors.append('recouvert : ligne recouverte admise quand le schema sequentiel est attendu')


def main():
    errors = []
    for check in (check_reading, check_outcomes, check_session, check_overlapped):
        check(errors)
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('test_lecteur_full_ok lecture=29 issues=7 session=6 recouvert=28')
    return 0


if __name__ == '__main__':
    sys.exit(main())
