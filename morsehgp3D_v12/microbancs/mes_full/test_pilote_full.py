#!/usr/bin/env python3
"""Porte du pilote MES-FULL sur une sonde simulee, sans moteur ni donnees reelles. Python 3.10 nu, aucun assert
(tient sous -O).

  essai          campagne complete en mode essai (voie CPU) : trois trames ng, deux trames v12set en Session (passe p
                 = trame p modulo n), empreintes identiques : verdict « essai », aucun refus, provenance publiee
                 (empreintes de la sonde, du pilote et du lecteur), temps CPU median par trame ;
  sites_faux     une sonde qui annonce un site de trop sur une trame : chaque processus de cette trame est refuse par
                 le lecteur partage (sites attendus = taille du fichier / 12) ;
  empreinte      une sonde dont l'empreinte FUL1 change d'une passe a l'autre : refus d'empreinte ;
  sequentiel     --sequentiel : le pilote passe le drapeau a la sonde et lit le schema sequentiel (verdict « essai ») ;
  schema_croise  une sonde qui ignore le drapeau et publie le schema sequentiel quand le pilote attend le schema
                 recouvert (voie par defaut) : chacun des dix processus est refuse comme illisible, et les controles
                 d'empreinte, sans aucune empreinte lue, manquent.
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
import pilote_full  # noqa: E402

FAKE = r'''#!/usr/bin/env python3
import json, os, sys
MODE = %r
frames = [a.split('=', 1)[1].split(',') for a in sys.argv[1:] if a.startswith('--trame=')]
args = dict(a.split('=', 1) for a in sys.argv[1:] if '=' in a and not a.startswith('--trame='))
passes, k, fils = int(args['--passes']), int(args['--k']), int(args['--threads'])
device = '--device' in sys.argv
sequential = '--sequentiel' in sys.argv or MODE == 'schema_croise'
if device:
    print(json.dumps(dict(phase='open', status='ok', reason='none', wall_ns=7, budget_appareil='partage')))
for i in range(passes):
    xyz, _ids, label = frames[i %% len(frames)]
    sites = os.path.getsize(xyz) // 12 + (1 if MODE == 'sites_faux' and label == 'ng01' else 0)
    sha = ('%%02x' %% (i if MODE == 'empreinte' else 0)) * 32
    row = dict(phase='full', trame=label, voie='device' if device else 'cpu', status='ok', coord_bits=21, kmax=k,
               threads=fils, sites=sites, wall_ns=2000 + i,
               c_ns=dict(parcours=1, feuilles=1, emission=1, fin_etage=1, transferts=0, publication=0),
               hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=9,
               cpu_ns=3 + i, rss_max_octets=9, appareil_octets=0, epinglee_octets=0, pic_appareil_octets=0,
               full_sha256=sha)
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
    print(json.dumps(row))
    print(json.dumps({'phase': 'liberation', 'pass': i, 'liberation_ns': 1}))
print(json.dumps(dict(phase='exit', status='ok', reason='none')))
'''


def write_frame(folder, name, sites):
    with open(os.path.join(folder, name + '.u32le'), 'wb') as out:
        out.write(b'\0' * (12 * sites))
    with open(os.path.join(folder, name + '.ids.u32le'), 'wb') as out:
        out.write(b'\0' * (4 * sites))


def campaign(folder, mode, extra=()):
    probe = os.path.join(folder, 'sonde_%s.py' % mode)
    with open(probe, 'w', encoding='utf-8') as out:
        out.write(FAKE % mode)
    os.chmod(probe, os.stat(probe).st_mode | stat.S_IXUSR)
    data = os.path.join(folder, 'donnees')
    if not os.path.isdir(data):
        os.makedirs(data)
        for i, f in enumerate(pilote_full.FRAMES):
            write_frame(data, 'lidar_' + f, 100 + i)
        frames = os.path.join(folder, 'v12set')
        os.makedirs(frames)
        names = ['seq00_000123_sans_sol', 'seq08_000456_sans_sol']
        for i, n in enumerate(names):
            write_frame(frames, n, 50 + i)
        with open(os.path.join(frames, 'bundle_manifest.json'), 'w', encoding='utf-8') as out:
            json.dump(dict(cases=[dict(name=n) for n in names]), out)
        with tarfile.open(os.path.join(folder, 'v12set.tar'), 'w') as tar:
            for member in sorted(os.listdir(frames)):
                tar.add(os.path.join(frames, member), arcname=member)
    out_dir = os.path.join(folder, 'sortie_' + mode)
    argv = ['pilote_full.py', '--essai', '--sonde', probe, '--src', folder,
            '--travail', os.path.join(folder, 'w_' + mode), '--donnees', data, '--sortie', out_dir,
            '--archive-v12set', os.path.join(folder, 'v12set.tar'), '--fils', '3', '--processus', '1', '--passes', '2']
    argv += list(extra)
    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        code = pilote_full.main(argv)
    try:
        with open(os.path.join(out_dir, 'rapport_full.json'), encoding='utf-8') as handle:
            return code, json.load(handle)
    except (OSError, ValueError):
        return code, None


def main():
    errors = []
    with tempfile.TemporaryDirectory() as folder:
        code, report = campaign(folder, 'ok')
        if code != 0 or report is None or report['verdict'] != 'essai' or report['refus'] or \
                set(report.get('provenance', {})) != {'sonde_sha256', 'pilote_sha256', 'lecteur_sha256', 'cmake'} or \
                report['statistiques']['k5_appareil']['ng00']['cpu_ns'] != 4:  # passe chaude 1 : cpu_ns = 3 + 1
            errors.append('essai : code %s, rapport %s' % (code, report and (report['verdict'], report['refus'])))
        code, report = campaign(folder, 'sites_faux')
        refused = [r for r in (report or {}).get('refus', []) if 'ng01' in r and 'illisible' in r]
        if code != 0 or report is None or len(refused) != 3:
            errors.append('sites_faux : refus %s' % (report or {}).get('refus'))
        code, report = campaign(folder, 'empreinte')
        if code != 0 or report is None or not any('empreintes FUL1 differentes' in r for r in report['refus']):
            errors.append('empreinte : refus %s' % (report or {}).get('refus'))
        code, report = campaign(folder, 'ok', ['--sequentiel'])
        if code != 0 or report is None or report['verdict'] != 'essai' or report['refus']:
            errors.append('sequentiel : code %s, rapport %s' % (code, report and (report['verdict'], report['refus'])))
        code, report = campaign(folder, 'schema_croise')
        refused = (report or {}).get('refus', [])
        unreadable = [r for r in refused if ' : illisible : ' in r]
        if code != 0 or report is None or len(unreadable) != 10 or \
                any(r not in unreadable and not r.endswith(': 0 empreintes FUL1 differentes') for r in refused):
            errors.append('schema_croise : refus %s' % refused)
        code, report = campaign(folder, 'ok')
        if code != 0 or report is None or report['verdict'] != 'essai' or report['refus']:
            errors.append('retour au schema recouvert apres --sequentiel : %s' % (report and report['refus']))
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('test_pilote_full_ok essai=1 sites_faux=1 empreinte=1 sequentiel=1 schema_croise=1')
    return 0


if __name__ == '__main__':
    sys.exit(main())
