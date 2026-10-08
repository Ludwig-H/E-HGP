#!/usr/bin/env python3
"""Porte de MES-B (verdicts et pilote), sans sonde ni donnees reelles ; la lecture stricte des sorties est celle du
lecteur partage, eprouve par microbancs/outils/test_lecteur_full.py. Python 3.10 nu, aucun assert (tient sous -O).

  verdicts       B1 a B4 aux seuils ecrits d'avance (2 s par million, refus sous 10 millions, pente 1,1, 10 s par
                 million a K10), « non evalue » sans donnee ; un echec compte a toute taille, un refus a K5 seulement
                 sous 10 millions (au-dela tolere), tout refus a K10 ;
  empreintes     une empreinte differente d'une voie a l'autre est un controle manquant ;
  attendu        le pilote demande au lecteur partage la trame, les sites, K, les fils, l'empreinte et un budget de
                 l'appareil separe ;
  etiquettes     un meme nom long repete rend des etiquettes distinctes d'au plus 23 octets (boucle sans fin corrigee) ;
  pilote         le pilote complet sur une sonde simulee : verdict d'essai, un cas non joue faute de delai, empreinte
                 calculee seulement sous le seuil de sites ; --sequentiel transmis a la sonde et son schema lu ; une
                 sonde au schema sequentiel quand le schema recouvert (defaut) est attendu : sorties illisibles,
                 controles manquants ; retour au schema recouvert apres une campagne --sequentiel.
Codes : 0 conforme ; 1 ecart.
"""
import io
import json
import os
import stat
import sys
import tempfile
from contextlib import redirect_stderr, redirect_stdout

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'outils'))
import lecteur_full as lf  # noqa: E402
import pilote_b  # noqa: E402

SITES = 2_000_000
LABEL = 'scene'
SHA = 'ab' * 32


def full_row(i, wall=4_000_000_000):
    """Ligne "full" du schema recouvert (voie par defaut), K = 5."""
    g, queue = wall // 2, wall // 10
    return dict(phase='full', pass_=i, trame=LABEL, voie='device', status='ok', etapes_schema='recouvert',
                coord_bits=21, kmax=5, threads=48, sites=SITES, wall_ns=wall,
                etapes_ns=dict(P=1, C=wall // 4, G=g, raccord=0, TMVR=queue),
                fenetres_ns=dict(G=5 * g, foret=4 * g, foret_apres_g=queue, T=g, M=g // 2, V=g // 4, R=g),
                c_ns={k: 1 for k in lf.C_KEYS}, g_ns=dict(ouverture=g // 10, tables=g // 20),
                hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=10, cpu_ns=5, rss_max_octets=10,
                appareil_octets=7, epinglee_octets=3, pic_appareil_octets=8, full_sha256=SHA,
                memoire_octets=dict(P=[1, 2], C=[4, 10], tour=[8, 9]),
                recouvrement=dict(tour_ns=g + queue + 1, ouverture_ns=g // 10, fin_g_ns=g, fin_ns=g + queue,
                                  queue_ns=queue, noyau_reprises=18, noyau_arrets=13, admis_octets=99),
                fins_par_ordre_ns=[[g, g + 1, g + 2, 0, g + 3]] + [[g - 10 * k, g, g + 1, g + 2, g + 3]
                                                                   for k in range(1, 5)])


def result(name, sites, wall_s, k=5, voie='appareil', etat='ok', passes=2):
    rows = [dict(full_row(i, int(wall_s * 1e9)), full_sha256=SHA) for i in range(passes)]
    return dict(nom=name, k=k, voie=voie, sites=sites, etat=etat, raison='' if etat == 'ok' else 'x', passes=rows)


def check_verdicts(errors):
    series = [['a', 'b', 'c']]
    linear = [result('a', 1_000_000, 1.5), result('b', 2_000_000, 3.0), result('c', 4_000_000, 6.0)]
    v = pilote_b.verdicts(linear, series)
    if [v[b]['etat'] for b in ('B1', 'B2', 'B3', 'B4')] != ['tenu', 'tenu', 'tenu', 'non evalue']:
        errors.append('verdicts : cas lineaire %s' % v)
    steep = [result('a', 1_000_000, 1.0), result('b', 2_000_000, 2.6), result('c', 4_000_000, 7.0)]
    v = pilote_b.verdicts(steep, series)
    if v['B3']['etat'] != 'non tenu' or v['B1']['etat'] != 'tenu':
        errors.append('verdicts : pente 1,4 %s' % v['B3'])
    slow = linear + [result('d', 3_000_000, 6.3), result('e', 9_000_000, 0, etat='refus', passes=0),
                     result('f', 1_000_000, 10.5, k=10)]
    v = pilote_b.verdicts(slow, series)
    if v['B1']['etat'] != 'non tenu' or v['B2']['etat'] != 'non tenu' or v['B4']['etat'] != 'non tenu':
        errors.append('verdicts : seuils %s' % v)
    over = pilote_b.verdicts(linear + [result('d', 3_000_000, 6.3)], series)
    if over['B1']['etat'] != 'non tenu' or over['B2']['etat'] != 'tenu':
        errors.append('verdicts : 2,1 s par million admis par B1 (%s)' % over['B1'])
    big = linear + [result('g', 12_000_000, 0, etat='refus', passes=0)]
    v = pilote_b.verdicts(big, series)
    if v['B2']['etat'] != 'tenu' or v['B1']['etat'] != 'tenu' or not any('tolere' in d for d in v['B1']['detail']):
        errors.append('verdicts : refus au-dela de 10 millions mal compte (%s, %s)' % (v['B1'], v['B2']))
    crash = linear + [result('h', 12_000_000, 0, etat='echec', passes=0)]
    if pilote_b.verdicts(crash, series)['B1']['etat'] != 'non tenu':
        errors.append('verdicts : echec au-dela de 10 millions ignore par B1')
    k10_refused = linear + [result('i', 1_000_000, 0, k=10, etat='refus', passes=0)]
    if pilote_b.verdicts(k10_refused, series)['B4']['etat'] != 'non tenu':
        errors.append('verdicts : refus a K10 ignore par B4')
    if pilote_b.slope([(1_000_000, 0.0), (2_000_000, 1.0)]) is not None:
        errors.append('verdicts : pente calculee sur un mur nul')
    partial = [result('a', 1_000_000, 1.5), result('b', 2_000_000, 3.0)]
    if pilote_b.verdicts(partial, series)['B3']['etat'] != 'non evalue':
        errors.append('verdicts : serie incomplete evaluee')


def check_digests(errors):
    rows = [result('a', 1_000_000, 1.0), result('a', 1_000_000, 3.0, voie='cpu')]
    for p in rows[1]['passes']:
        p['full_sha256'] = 'cd' * 32  # chaque voie constante, les deux voies differentes
    _table, bad = pilote_b.digest_controls(rows)
    if len(bad) != 1:
        errors.append('empreintes : ecart entre voies non signale (%s)' % bad)
    rows = [result('a', 1_000_000, 1.0)]
    rows[0]['passes'][1]['full_sha256'] = 'cd' * 32
    if len(pilote_b.digest_controls(rows)[1]) != 1:
        errors.append('empreintes : ecart entre passes non signale')
    _table, bad = pilote_b.digest_controls([result('a', 1_000_000, 1.0), result('a', 1_000_000, 3.0, voie='cpu')])
    if bad:
        errors.append('empreintes : ecart invente %s' % bad)


def check_expected(errors):
    """Le pilote demande au lecteur un budget de l'appareil separe, le schema recouvert (voie par defaut) et la trame,
    les sites et K du cas."""
    case = dict(nom='x', k=10, voie='appareil', passes=2, fils=48, empreinte=False)
    want = dict(voie='appareil', k=10, fils=48, passes=2, empreinte=False, trames=[('lbl', 123)],
                budget_appareil='separe', bits=21, schema='recouvert')
    if pilote_b.expected(case, 'lbl', 123) != want:
        errors.append('attendu : %s' % pilote_b.expected(case, 'lbl', 123))


def check_labels(errors):
    used, name = set(), 'boreas_202011261358_f4500_n50_sans_sol'
    labels = [pilote_b.label_of(name, used) for _ in range(12)]
    if len(set(labels)) != 12 or any(len(x) > 23 for x in labels):
        errors.append('etiquettes : %s' % labels)


FAKE = r'''#!/usr/bin/env python3
import json, sys
MODE = %r
args = dict(a.split('=', 1) for a in sys.argv[1:] if '=' in a)
label = args['--trame'].split(',')[2]
passes, k, fils = int(args['--passes']), int(args['--k']), int(args['--threads'])
digest = '--digest' in sys.argv
sequential = '--sequentiel' in sys.argv or MODE == 'schema_croise'
sites = 1000 if 'petite' in label else 5000
for i in range(passes):
    row = dict(phase='full', trame=label, voie='cpu', status='ok', coord_bits=21, kmax=k, threads=fils,
               sites=sites, wall_ns=2000,
               c_ns=dict(parcours=1, feuilles=1, emission=1, fin_etage=1, transferts=0, publication=0),
               hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=9,
               cpu_ns=3, rss_max_octets=9, appareil_octets=0, epinglee_octets=0, pic_appareil_octets=0)
    if sequential:
        row.update(etapes_ns=dict(P=1, C=1, G=4, raccord=1, TMVR=4, T=1, M=1, V=1, R=1),
                   g_ns=dict(tables=1, resolution=1),
                   memoire_octets=dict(P=[1, 2], C=[3, 9], G=[4, 5], raccord=[4, 4], TMVR=[6, 7]))
    else:
        row.update(etapes_schema='recouvert', etapes_ns=dict(P=1, C=1, G=4, raccord=0, TMVR=2),
                   fenetres_ns=dict(G=8, foret=6, foret_apres_g=2, T=1, M=1, V=1, R=1),
                   g_ns=dict(ouverture=2, tables=1), memoire_octets=dict(P=[1, 2], C=[3, 9], tour=[4, 5]),
                   recouvrement=dict(tour_ns=7, ouverture_ns=2, fin_g_ns=4, fin_ns=6, queue_ns=2, noyau_reprises=3,
                                     noyau_arrets=1, admis_octets=9),
                   fins_par_ordre_ns=[[4, 5, 6, 0, 6]] + [[3, 4, 5, 5, 6]] * (k - 1))
    row['pass'] = i
    if digest:
        row['full_sha256'] = '0f' * 32
    print(json.dumps(row))
    print(json.dumps(dict(phase='liberation', liberation_ns=1, **{'pass': i})))
print(json.dumps(dict(phase='exit', status='ok', reason='none')))
'''


def play(folder, data, mode, extra=()):
    """Campagne d'essai du pilote sur la sonde simulee `mode` ; rend (code, rapport ou None)."""
    probe = os.path.join(folder, 'sonde_%s.py' % mode)
    with open(probe, 'w', encoding='utf-8') as out:
        out.write(FAKE % mode)
    os.chmod(probe, os.stat(probe).st_mode | stat.S_IXUSR)
    sortie = os.path.join(folder, 'sortie_%s%s' % (mode, ''.join(extra)))
    argv = ['pilote_b.py', '--essai', '--sonde', probe, '--donnees', data, '--sortie', sortie,
            '--cas', 'petite:5:appareil:2,enorme:5:appareil:1,grande:5:cpu:1', '--fils', '3',
            '--delai-global', '120', '--empreinte-max-sites', '2000'] + list(extra)
    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        code = pilote_b.main(argv)
    try:
        with open(os.path.join(sortie, 'rapport_b.json'), encoding='utf-8') as handle:
            report = json.load(handle)
        with open(os.path.join(sortie, 'tableaux_b.md'), encoding='utf-8') as handle:
            report['tableaux'] = handle.read()
        return code, report
    except (OSError, ValueError):
        return code, None


def check_pilot(errors):
    with tempfile.TemporaryDirectory() as folder:
        data = os.path.join(folder, 'donnees')
        os.makedirs(data)
        cases = []
        for name, count in (('petite', 1000), ('grande', 5000), ('enorme', 50_000_000)):
            for suffix in ('.u32le', '.ids.u32le'):
                with open(os.path.join(data, name + suffix), 'wb') as out:
                    out.write(b'\0' * 4)
            cases.append(dict(name=name, coordinates=name + '.u32le', point_ids=name + '.ids.u32le', count=count))
        with open(os.path.join(data, 'bundle_manifest.json'), 'w', encoding='utf-8') as out:
            json.dump(dict(cases=cases), out)
        expected = [('petite', 'ok', 2, True), ('enorme', 'non_joue', 0, False), ('grande', 'ok', 1, False)]
        heads = {'recouvert': ('| P | C | tour |', '| G | queue | validation |'),
                 'sequentiel': ('| P | C | G | raccord | TMVR |', '| G | T | M | V | R | validation |')}
        for mode, extra, schema in (('ok', (), 'recouvert'), ('ok', ('--sequentiel',), 'sequentiel'),
                                    ('ok', (), 'recouvert')):
            code, report = play(folder, data, mode, extra)
            if report is None:
                errors.append('pilote %s : rapport absent (code %s)' % (' '.join(extra), code))
                return
            states = [(r['nom'], r['etat'], len(r['passes']), r['empreinte']) for r in report['cas']]
            if code != 0 or report['verdict'] != 'essai' or states != expected or report['controles'] or \
                    report['parametres']['schema'] != schema:
                errors.append('pilote %s : code %s, verdict %s, cas %s, controles %s' % (
                    ' '.join(extra), code, report['verdict'], states, report['controles']))
            if 'petite:K5' not in report['empreintes'] or 'grande:K5' in report['empreintes']:
                errors.append('pilote %s : empreintes %s' % (' '.join(extra), sorted(report['empreintes'])))
            if any(head not in report['tableaux'] for head in heads[schema]):
                errors.append('pilote %s : colonnes des tableaux hors schema %s' % (' '.join(extra), schema))
        code, report = play(folder, data, 'schema_croise')
        unreadable = [r for r in (report or {}).get('cas', []) if r['etat'] == 'illisible']
        if code != 0 or report is None or len(unreadable) != 2 or len(report['controles']) < 2:
            errors.append('pilote schema_croise : cas %s, controles %s' % (
                [(r['nom'], r['etat']) for r in (report or {}).get('cas', [])], (report or {}).get('controles')))
        probe = os.path.join(folder, 'sonde_ok.py')
        with redirect_stderr(io.StringIO()):
            bad = pilote_b.main(['pilote_b.py', '--essai', '--sonde', probe, '--donnees', data, '--sortie',
                                 os.path.join(folder, 'sortie_absente'), '--cas', 'absente:5:cpu:1'])
        if bad != 2:
            errors.append('pilote : cas absent du manifeste admis (code %s)' % bad)


def main():
    errors = []
    for check in (check_verdicts, check_digests, check_expected, check_labels, check_pilot):
        check(errors)
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('test_pilote_b_ok verdicts=9 empreintes=3 attendu=1 etiquettes=12 pilote=5')
    return 0


if __name__ == '__main__':
    sys.exit(main())
