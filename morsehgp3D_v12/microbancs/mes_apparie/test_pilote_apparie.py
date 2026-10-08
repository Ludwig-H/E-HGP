#!/usr/bin/env python3
"""Porte du pilote apparie sur une sonde simulee, sans moteur ni donnees reelles. Python 3.10 nu, aucun assert (tient
sous -O).

  auto-test      le juge sur ses campagnes synthetiques (adopte, rejete deux fois, quatre refus) ;
  usage          option hors de la liste fermee, bras A/A d'options differentes de la reference, reference absente :
                 code 2 avant toute prise ;
  campagne       campagne d'essai complete (voie CPU, deux passes : la premiere, froide et plus lente, est ecartee) :
                 reference, A/A et un bras --cache plus rapide (x 0,9) et un bras --sequentiel plus lent (x 1,1) ; le
                 jugement calcule adopte le premier (rapport 0,9 exactement), rejette le second, publie l'A/A a 1 ;
                 verdict publie « essai » ; schema sequentiel lu pour le seul bras --sequentiel ; ordre des bras
                 decale d'un bras par tour (chaque position une fois par bras en quatre tours) ; les tableaux portent
                 les quatre bras ; la Session v12set d'information porte ses deux trames par bras ;
  schema_croise  une sonde qui ignore --sequentiel : les prises de ce bras sont illisibles, la campagne est refusee ;
  journal_change un journal de campagne modifie apres coup : le rejeu brut du juge refuse la campagne ;
  empreinte      une sonde dont l'empreinte depend des options : identite refusee.
Codes : 0 conforme ; 1 ecart.
"""
import io
import json
import os
import stat
import sys
import tarfile
import tempfile
from contextlib import redirect_stderr, redirect_stdout

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pilote_apparie as pa  # noqa: E402

FAKE = r'''#!/usr/bin/env python3
import json, os, sys
MODE = %r
frames = [a.split('=', 1)[1].split(',') for a in sys.argv[1:] if a.startswith('--trame=')]
args = dict(a.split('=', 1) for a in sys.argv[1:] if '=' in a and not a.startswith('--trame='))
passes, k, fils = int(args['--passes']), int(args['--k']), int(args['--threads'])
digest = '--digest' in sys.argv
sequential = '--sequentiel' in sys.argv and MODE != 'schema_croise'
factor = 0.9 if '--cache' in args else (1.1 if '--sequentiel' in sys.argv else 1.0)
for i in range(passes):
    xyz, _ids, label = frames[i %% len(frames)]
    sites = os.path.getsize(xyz) // 12
    wall = int(1_000_000 * factor) + (300_000 if i < len(frames) else 0)
    row = dict(phase='full', trame=label, voie='cpu', status='ok', coord_bits=21, kmax=k, threads=fils, sites=sites,
               wall_ns=wall, c_ns=dict(parcours=1, feuilles=1, emission=1, fin_etage=1, transferts=0, publication=0),
               hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=9, cpu_ns=3, rss_max_octets=9,
               appareil_octets=0, epinglee_octets=0, pic_appareil_octets=0)
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
    if digest:
        row['full_sha256'] = ('cd' if MODE == 'empreinte' and '--cache' in args else 'ab') * 32
    row['pass'] = i
    print(json.dumps(row))
    print(json.dumps({'phase': 'liberation', 'pass': i, 'liberation_ns': 1}))
print(json.dumps(dict(phase='exit', status='ok', reason='none')))
'''

ARMS = ['--bras', 'ref=', '--bras', 'aa=', '--bras', 'cache=--cache=1048576', '--bras', 'seq=--sequentiel']


def write_frame(folder, name, sites):
    with open(os.path.join(folder, name + '.u32le'), 'wb') as out:
        out.write(b'\0' * (12 * sites))
    with open(os.path.join(folder, name + '.ids.u32le'), 'wb') as out:
        out.write(b'\0' * (4 * sites))


def prepare(folder):
    data = os.path.join(folder, 'donnees')
    os.makedirs(data)
    for i, label in enumerate(('ng00', 'ng01', 'ng02')):
        write_frame(data, 'lidar_' + label, 100 + i)
    session = os.path.join(folder, 'v12set')
    os.makedirs(session)
    names = ['kitti_ng_00_000123', 'kitti_ng_08_000456']
    for i, name in enumerate(names):
        write_frame(session, name, 50 + i)
    with open(os.path.join(session, 'bundle_manifest.json'), 'w', encoding='utf-8') as out:
        json.dump(dict(cases=[dict(name=n) for n in names]), out)
    with tarfile.open(os.path.join(folder, 'v12set.tar'), 'w') as tar:
        for member in sorted(os.listdir(session)):
            tar.add(os.path.join(session, member), arcname=member)
    return data


def campaign(folder, mode, arms=ARMS, extra=()):
    probe = os.path.join(folder, 'sonde_%s.py' % mode)
    with open(probe, 'w', encoding='utf-8') as out:
        out.write(FAKE % mode)
    os.chmod(probe, os.stat(probe).st_mode | stat.S_IXUSR)
    out_dir = os.path.join(folder, 'sortie_' + mode)
    argv = ['pilote_apparie.py', '--essai', '--sonde', probe, '--donnees', os.path.join(folder, 'donnees'),
            '--sortie', out_dir, '--reference', 'ref', '--aa', 'aa', '--fils', '3', '--tours', '4', '--passes', '2',
            '--archive-v12set', os.path.join(folder, 'v12set.tar'), '--session-tours', '1'] + list(arms) + list(extra)
    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        code = pa.main(argv)
    try:
        with open(os.path.join(out_dir, 'rapport_apparie.json'), encoding='utf-8') as handle:
            report = json.load(handle)
        with open(os.path.join(out_dir, 'tableaux_apparie.md'), encoding='utf-8') as handle:
            report['_tableaux'] = handle.read()
        return code, report, out_dir
    except (OSError, ValueError):
        return code, None, out_dir


def main():
    errors = []
    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        if pa.self_test() != 0:
            errors.append('auto-test du juge en echec')
    with tempfile.TemporaryDirectory() as folder:
        prepare(folder)
        for name, arms in (('option_inconnue', ['--bras', 'ref=', '--bras', 'aa=', '--bras', 'x=--leaf=4']),
                           ('aa_different', ['--bras', 'ref=', '--bras', 'aa=--cache=1']),
                           ('reference_absente', ['--bras', 'aa=', '--bras', 'x='])):
            code, report, _out = campaign(folder, 'usage_' + name, arms)
            if code != 2 or report is not None:
                errors.append('usage %s : code %s' % (name, code))
        code, report, out_dir = campaign(folder, 'ok')
        judged = (report or {}).get('jugement', {})
        cases = {arm: c.get('verdict') for arm, c in judged.get('cas', {}).items()}
        aa = judged.get('cas', {}).get('aa', {}).get('trames', {})
        if code != 0 or report is None or judged.get('verdict') != 'essai' or judged.get('verdict_calcule') != 'juge' \
                or cases != {'aa': 'controle A/A', 'cache': 'adopte', 'seq': 'rejete'} or \
                any(abs(v['rapport'] - 1) > 1e-12 for v in aa.values()) or \
                any(abs(v['rapport'] - 0.9) > 1e-6 for v in judged['cas']['cache']['trames'].values()):
            errors.append('campagne : code %s, jugement %s' % (code, judged))
        else:
            if any('`%s`' % arm not in report['_tableaux'] for arm in ('ref', 'aa', 'cache', 'seq')):
                errors.append('campagne : bras absents des tableaux')
            # Ordre decale d'un bras par tour, sans inversion : chaque bras occupe chaque position une fois en 4 tours.
            for label, orders in report.get('ordres', {}).items():
                if len(orders) != 4 or any(sorted(o) != ['aa', 'cache', 'ref', 'seq'] for o in orders) or \
                        any(sorted(o[p] for o in orders) != ['aa', 'cache', 'ref', 'seq'] for p in range(4)):
                    errors.append('campagne : ordre des bras %s %s' % (label, orders))
            if sorted(report.get('ordres', {})) != ['ng00', 'ng01', 'ng02']:
                errors.append('campagne : ordres absents')
            session = report.get('session_v12set') or {}
            if sorted(session) != ['aa', 'cache', 'ref', 'seq'] or any(len(s['trames']) != 2 or s['refus']
                                                                       for s in session.values()):
                errors.append('campagne : Session v12set %s' % session)
            # Journal de campagne modifie apres coup : le rejeu brut refuse.
            path = os.path.join(out_dir, 'journaux', 'campagne', 'ng01', 'cache_t02.jsonl')
            with open(path, 'a', encoding='utf-8') as handle:
                handle.write('\n')
            if pa.judge(report, out_dir)['verdict'] != 'refuse':
                errors.append('journal_change : rejeu brut admis')
        code, report, _out = campaign(folder, 'schema_croise')
        judged = (report or {}).get('jugement', {})
        if code != 0 or judged.get('verdict_calcule') != 'refuse' or \
                not any('seq' in r for r in judged.get('refus', [])):
            errors.append('schema_croise : %s' % judged)
        code, report, _out = campaign(folder, 'empreinte')
        judged = (report or {}).get('jugement', {})
        if code != 0 or judged.get('verdict_calcule') != 'refuse' or \
                not any('empreintes FUL1 differentes' in r for r in judged.get('refus', [])):
            errors.append('empreinte : %s' % judged)
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('test_pilote_apparie_ok auto_test=1 usage=3 campagne=1 journal_change=1 schema_croise=1 empreinte=1')
    return 0


if __name__ == '__main__':
    sys.exit(main())
