#!/usr/bin/env python3
"""Porte de MES-B (lecteur, verdicts et pilote), sans sonde ni donnees reelles. Python 3.10 nu, aucun assert (tient
sous -O).

  lecture        une sortie appareil conforme est admise ; dix-sept mutations du schema et les cinq corruptions de la
                 contrelecture de l'auditeur (cle repetee, booleen, ouverture, 2^64, NaN) sont refusees comme sorties
                 illisibles (controle manquant), jamais comme resultats ;
  issues         refus de la sonde (code 2, ressources ou degenerescence) publie comme resultat, avec ses passes deja
                 jouees ; invariant viole (codes 2 et 3), signal et expiration publies comme echecs du cas ;
  verdicts       B1 a B4 aux seuils ecrits d'avance (2 s par million, refus sous 10 millions, pente 1,1, 10 s par
                 million a K10), « non evalue » sans donnee ; un echec compte a toute taille, un refus a K5 seulement
                 sous 10 millions (au-dela tolere), tout refus a K10 ;
  empreintes     une empreinte differente d'une voie a l'autre est un controle manquant ;
  etiquettes     un meme nom long repete rend des etiquettes distinctes d'au plus 23 octets (boucle sans fin corrigee) ;
  pilote         le pilote complet sur une sonde simulee : verdict d'essai, un cas non joue faute de delai, empreinte
                 calculee seulement sous le seuil de sites.
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
import pilote_b  # noqa: E402

CASE = dict(nom='scene', k=5, voie='appareil', passes=2, fils=48, empreinte=True)
SITES = 2_000_000
LABEL = 'scene'
SHA = 'ab' * 32


def full_row(i, wall=4_000_000_000):
    return dict(phase='full', pass_=i, trame=LABEL, voie='device', status='ok', coord_bits=21, kmax=5, threads=48,
                sites=SITES, wall_ns=wall,
                etapes_ns=dict(P=1, C=wall // 4, G=wall // 4, raccord=1, TMVR=wall // 4, T=wall // 8, M=1, V=1, R=1),
                c_ns={k: 1 for k in pilote_b.C_KEYS}, g_ns=dict(tables=1, resolution=wall // 8),
                hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=10, cpu_ns=5, rss_max_octets=10,
                appareil_octets=7, epinglee_octets=3, pic_appareil_octets=8, full_sha256=SHA)


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
    state = pilote_b.parse_output(0, device_output(), CASE, SITES, LABEL)
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
    }
    for name, mutate in mutations.items():
        state = pilote_b.parse_output(0, device_output(mutate=mutate), CASE, SITES, LABEL)
        if state['etat'] != 'illisible':
            errors.append('lecture : mutation %s rendue %s' % (name, state['etat']))
    text = device_output().replace('{"phase": "liberation", "pass": 1, "liberation_ns": 3}\n', '')
    if pilote_b.parse_output(0, text, CASE, SITES, LABEL)['etat'] != 'illisible':
        errors.append('lecture : liberation absente admise')
    shared = dict(phase='open', status='ok', reason='none', wall_ns=5, budget_appareil='partage')
    if pilote_b.parse_output(0, device_output(open_row=shared), CASE, SITES, LABEL)['etat'] != 'illisible':
        errors.append('lecture : budget de l\'appareil partage admis')
    cpu_case = dict(CASE, voie='cpu')
    text = device_output().replace('"voie": "device"', '"voie": "cpu"').split('\n', 1)[1]
    if pilote_b.parse_output(0, text, cpu_case, SITES, LABEL)['etat'] != 'illisible':
        errors.append('lecture : memoire de l\'appareil admise sur la voie CPU')
    # Corruptions de la contrelecture de l'auditeur Codex (mes_b_prelecture) : cle repetee, booleen comme rang de
    # liberation, ouverture ok avec une raison, entier hors de u64, constante non finie.
    good = device_output()
    corrupt = {
        'cle_repetee': good.replace('"cpu_ns": 5,', '"cpu_ns": 5, "cpu_ns": 5,', 1),
        'liberation_booleenne': good.replace('{"phase": "liberation", "pass": 0,',
                                             '{"phase": "liberation", "pass": false,'),
        'ouverture_raison': good.replace('"reason": "none", "wall_ns": 5', '"reason": "device_fault", "wall_ns": 5'),
        'entier_2_64': good.replace('"pic_octets": 10,', '"pic_octets": 18446744073709551616,', 1),
        'constante_nan': good.replace('"cpu_ns": 5,', '"cpu_ns": NaN,', 1),
    }
    for name, text in corrupt.items():
        if text == good or pilote_b.parse_output(0, text, CASE, SITES, LABEL)['etat'] != 'illisible':
            errors.append('lecture : corruption %s admise' % name)


def check_outcomes(errors):
    refused = device_output(passes=1, end=dict(phase='exit', status='resource_exhausted', reason='memory_budget'))
    state = pilote_b.parse_output(2, refused, CASE, SITES, LABEL)
    if state['etat'] != 'refus' or state['raison'] != 'resource_exhausted/memory_budget' or len(state['passes']) != 1:
        errors.append('issues : refus de ressources mal lu (%s)' % state)
    degenerate = device_output(passes=0, end=dict(phase='exit', status='unsupported_degeneracy', reason='wide_leaf'))
    if pilote_b.parse_output(2, degenerate, CASE, SITES, LABEL)['etat'] != 'refus':
        errors.append('issues : refus de degenerescence mal lu')
    broken = device_output(passes=0, end=dict(phase='exit', status='invariant_violated', reason='tower_invariant'))
    for code in (3, 2):
        if pilote_b.parse_output(code, broken, CASE, SITES, LABEL)['etat'] != 'echec':
            errors.append('issues : invariant viole (code %d) non publie comme echec' % code)
    for code, label in ((-9, 'signal'), ('expire', 'expiration')):
        if pilote_b.parse_output(code, '', CASE, SITES, LABEL)['etat'] != 'echec':
            errors.append('issues : %s non publie comme echec' % label)
    if pilote_b.parse_output(0, device_output(passes=1), CASE, SITES, LABEL)['etat'] != 'echec':
        errors.append('issues : passes manquantes avec sortie ok admises')


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


def check_labels(errors):
    used, name = set(), 'boreas_202011261358_f4500_n50_sans_sol'
    labels = [pilote_b.label_of(name, used) for _ in range(12)]
    if len(set(labels)) != 12 or any(len(x) > 23 for x in labels):
        errors.append('etiquettes : %s' % labels)


FAKE = r'''#!/usr/bin/env python3
import json, sys
args = dict(a.split('=', 1) for a in sys.argv[1:] if '=' in a)
label = args['--trame'].split(',')[2]
passes, k, fils = int(args['--passes']), int(args['--k']), int(args['--threads'])
digest = '--digest' in sys.argv
sites = 1000 if 'petite' in label else 5000
for i in range(passes):
    row = dict(phase='full', trame=label, voie='cpu', status='ok', coord_bits=21, kmax=k, threads=fils,
               sites=sites, wall_ns=2000, etapes_ns=dict(P=1, C=1, G=4, raccord=1, TMVR=4, T=1, M=1, V=1, R=1),
               c_ns=dict(parcours=1, feuilles=1, emission=1, fin_etage=1, transferts=0, publication=0),
               g_ns=dict(tables=1, resolution=1), hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=9,
               cpu_ns=3, rss_max_octets=9, appareil_octets=0, epinglee_octets=0, pic_appareil_octets=0)
    row['pass'] = i
    if digest:
        row['full_sha256'] = '0f' * 32
    print(json.dumps(row))
    print(json.dumps(dict(phase='liberation', liberation_ns=1, **{'pass': i})))
print(json.dumps(dict(phase='exit', status='ok', reason='none')))
'''


def check_pilot(errors):
    with tempfile.TemporaryDirectory() as folder:
        probe = os.path.join(folder, 'sonde.py')
        with open(probe, 'w', encoding='utf-8') as out:
            out.write(FAKE)
        os.chmod(probe, os.stat(probe).st_mode | stat.S_IXUSR)
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
        sortie = os.path.join(folder, 'sortie')
        argv = ['pilote_b.py', '--essai', '--sonde', probe, '--donnees', data, '--sortie', sortie,
                '--cas', 'petite:5:appareil:2,enorme:5:appareil:1,grande:5:cpu:1', '--fils', '3',
                '--delai-global', '120', '--empreinte-max-sites', '2000']
        text = io.StringIO()
        with redirect_stdout(text), redirect_stderr(io.StringIO()):
            code = pilote_b.main(argv)
        try:
            with open(os.path.join(sortie, 'rapport_b.json'), encoding='utf-8') as handle:
                report = json.load(handle)
        except (OSError, ValueError):
            errors.append('pilote : rapport absent (code %s)' % code)
            return
        states = [(r['nom'], r['etat'], len(r['passes']), r['empreinte']) for r in report['cas']]
        expected = [('petite', 'ok', 2, True), ('enorme', 'non_joue', 0, False), ('grande', 'ok', 1, False)]
        if code != 0 or report['verdict'] != 'essai' or states != expected or report['controles']:
            errors.append('pilote : code %s, verdict %s, cas %s, controles %s' % (code, report['verdict'], states,
                                                                                report['controles']))
        if 'petite:K5' not in report['empreintes'] or 'grande:K5' in report['empreintes']:
            errors.append('pilote : empreintes %s' % sorted(report['empreintes']))
        with redirect_stderr(io.StringIO()):
            bad = pilote_b.main(['pilote_b.py', '--essai', '--sonde', probe, '--donnees', data, '--sortie', sortie,
                                 '--cas', 'absente:5:cpu:1'])
        if bad != 2:
            errors.append('pilote : cas absent du manifeste admis (code %s)' % bad)


def main():
    errors = []
    for check in (check_reading, check_outcomes, check_verdicts, check_digests, check_labels, check_pilot):
        check(errors)
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('test_pilote_b_ok lecture=23 issues=7 verdicts=8 empreintes=3 etiquettes=12 pilote=2')
    return 0


if __name__ == '__main__':
    sys.exit(main())
