#!/usr/bin/env python3
"""Pilote apparie de la sonde FULL (bench/full_probe.cpp) : des bras qui ne different QUE par des options de la sonde
(meme binaire, memes donnees, meme configuration de Session), joues en processus alternes, et un juge ecrit d'avance.

Usage : pilote_apparie.py --src SRC --travail DOSSIER --donnees DOSSIER --sortie DOSSIER
                          --bras NOM=OPTIONS [--bras NOM=OPTIONS ...] --reference NOM [--aa NOM]
                          [--trames ng00,ng01,ng02] [--k 5] [--voie appareil|cpu] [--fils 48] [--tours 10]
                          [--passes 10] [--archive-v12set TAR] [--session-tours 2] [--jobs 44] [--delai 900]
                          [--essai --sonde BINAIRE]
        pilote_apparie.py auto-test
OPTIONS : options de la sonde separees par des espaces, dans une liste fermee : --cache=OCTETS, --sequentiel,
--recouvert ; vide : la voie par defaut (Session recouverte, sans cache). NOM : [a-z][a-z0-9_]* (au plus 24).
--donnees contient lidar_<trame>.u32le et lidar_<trame>.ids.u32le. --essai : sonde existante, voie CPU, minima
relaches, verdict « essai » (le jugement calcule reste publie, jamais comme mesure).

Etapes : environnement et GPU vide avant et apres (voie appareil) ; construction Release au profil 21 de
mhgp12_full_probe (CUDA pour la voie appareil ; journal, empreinte SHA-256, extrait du CMakeCache) ; puis
  1. identite : par trame et par bras, un processus de deux passes avec --digest ; une seule empreinte FUL1 par trame,
     tous bras et toutes passes confondus ;
  2. campagne decisive : par trame, --tours tours ; dans chaque tour, un processus neuf par bras, ordre decale d'un
     bras par tour, sans inversion ; chaque processus joue --passes passes sans empreinte ;
  3. information (si --archive-v12set) : par tour de Session (--session-tours), un processus par bras, ordre decale,
     qui enchaine les trames v12set deux fois (le second passage fait foi) ; aucun verdict.
Chaque sortie est lue par le lecteur strict partage microbancs/outils/lecteur_full.py, avec l'attendu DERIVE DE SA
PLACE (trame, bras, configuration, schema : « sequentiel » si le bras porte --sequentiel, « recouvert » sinon), jamais
de metadonnees relues ; le juge re-hache et relit chaque journal de la campagne (le rejeu brut fait autorite).

REGLE_APPARIEE (ecrite le 8 octobre 2026, avant toute mesure G4 de ce pilote ; ses parametres sont publies dans le
rapport) : par processus, mur chaud = mediane des passes 2 a P de wall_ns ; par tour, rapport bras / reference ;
moyenne geometrique des rapports des tours et IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261008),
par trame decisive. Un bras est ADOPTE si l'empreinte FUL1 est identique (identite) ET si la borne haute de son IC est
sous 1 sur CHACUNE des trames decisives ; REJETE sinon. La campagne est REFUSEE (aucun bras adopte ni rejete) si une
prise manque ou est illisible, si un journal manque ou a change, si un tour n'a pas tous ses bras, si le nombre de
tours est sous --tours, si l'identite manque, si l'environnement est incomplet ou le GPU occupe (voie appareil), ou si
la moyenne geometrique du bras A/A (--aa, memes options que la reference) sort de [0,985 ; 1,015] sur une trame
decisive. Le bras A/A n'est jamais adopte.

Sorties : <sortie>/rapport_apparie.json, <sortie>/tableaux_apparie.md, <sortie>/journaux/, <sortie>/construction.log.
Codes : 0 rendu (quel que soit le verdict) ; 1 auto-test en echec ; 2 usage ; 3 construction impossible.
Python 3.10 nu, aucun assert (tient sous -O).
"""
import argparse
import json
import math
import os
import random
import re
import shutil
import statistics
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'outils'))
import banc_full as bf  # noqa: E402
import lecteur_full as lf  # noqa: E402

REGLE_APPARIEE = dict(statistique='par processus : mediane des passes 2..P du mur (wall_ns) ; par tour : rapport bras '
                                  '/ reference ; moyenne geometrique et IC 95 % par bootstrap sur les tours',
                      adoption="empreinte FUL1 identique (identite) et borne haute de l'IC sous 1 sur chacune des "
                               "trames decisives",
                      refus='prise manquante ou illisible, journal manquant ou change, tour incomplet, tours sous le '
                            'minimum, identite absente, environnement incomplet ou GPU occupe, A/A hors fenetre',
                      bootstrap=10000, graine=20261008, seuil_borne_haute=1.0, fenetre_aa=0.015, passes_min=6,
                      tours_min=10, ecrite='8 octobre 2026, avant toute mesure G4 de ce pilote')
NAME = re.compile(r'[a-z][a-z0-9_]{0,23}')
OPTION = re.compile(r'--cache=(0|[1-9][0-9]{0,19})|--sequentiel|--recouvert')


def parse_arm(text):
    """'NOM=OPTIONS' -> (nom, [options]) ; None si le nom ou une option sort de la liste fermee."""
    name, sep, rest = text.partition('=')
    options = rest.split()
    if not sep or not NAME.fullmatch(name) or any(not OPTION.fullmatch(o) for o in options) or \
            len(set(options)) != len(options) or ('--sequentiel' in options and '--recouvert' in options) or \
            sum(o.startswith('--cache=') for o in options) > 1:
        return None
    return name, options


def schema_of(options):
    return 'sequentiel' if '--sequentiel' in options else 'recouvert'


def expected(arm_options, voie, k, fils, passes, frames, digest):
    """Attendu du lecteur partage, derive de la place de la prise (jamais de metadonnees relues)."""
    return dict(voie=voie, k=k, fils=fils, passes=passes, empreinte=digest, trames=list(frames),
                budget_appareil='partage', bits=21, schema=schema_of(arm_options))


def probe_argv(probe, frames, voie, k, fils, passes, digest, options):
    argv = [probe] + ['--trame=%s,%s,%s' % (xyz, ids, label) for xyz, ids, label in frames]
    argv += ['--k=%d' % k, '--threads=%d' % fils, '--passes=%d' % passes] + (['--digest'] if digest else [])
    return argv + (['--device'] if voie == 'appareil' else []) + list(options)


def median(values):
    return statistics.median(values)


def bootstrap_gm(logs, rng, draws):
    """Moyenne geometrique et IC 95 % par bootstrap sur les tours."""
    gm = math.exp(sum(logs) / len(logs))
    boot = sorted(sum(logs[rng.randrange(len(logs))] for _ in logs) / len(logs) for _ in range(draws))
    return gm, math.exp(boot[int(0.025 * draws)]), math.exp(boot[int(0.975 * draws) - 1])


def warm_wall(passes):
    """Mur chaud d'un processus : mediane des passes 2 a P (la premiere, a froid, est ecartee)."""
    return median([p['wall_ns'] for p in passes[1:]])


def take_summary(state):
    """Resume publie d'une prise conforme : mur chaud, etages, ressources (medianes des passes chaudes)."""
    warm = state['passes'][1:]
    return dict(mur_chaud_ns=warm_wall(state['passes']),
                etapes_ns={s: median([p['etapes_ns'][s] for p in warm]) for s in warm[0]['etapes_ns']},
                cpu_ns=median([p['cpu_ns'] for p in warm]), pic_octets=max(p['pic_octets'] for p in warm))


def file_sha(path):
    try:
        return bf.sha256_file(path)
    except OSError:
        return None


def play(argv, attendu, journal, delay):
    """Une prise : processus borne, sortie brute dans journal, lecture stricte ; rend la prise publiee."""
    code, out, _err, secs = bf.run(argv, delay, journal, journal + '.err')
    state = lf.parse_output(code, out, attendu)
    take = dict(journal=os.path.basename(journal), journal_sha256=file_sha(journal), code=code, etat=state['etat'],
                raison=state['raison'], secondes=round(secs, 1))
    if state['etat'] == 'ok':
        take.update(take_summary(state) if attendu['passes'] >= 2 else {},
                    empreintes=sorted({p['full_sha256'] for p in state['passes']}) if attendu['empreinte'] else [])
    return take


# ---- Juge -----------------------------------------------------------------------------------------------------------
def reread(report, folder, refusals):
    """Relit les preuves d'identite et les campagnes ; recalcule tous les champs utilises par les tableaux."""
    cfg = report['parametres']
    arms = dict(report['regle_bras'])

    def one(take, path, attendu, label):
        actual_sha = file_sha(path)
        if type(take) is not dict or actual_sha is None or take.get('journal') != os.path.basename(path) or \
                actual_sha != take.get('journal_sha256'):
            refusals.append('journal manquant ou change : ' + label)
            return
        if type(take.get('code')) is not int or take['code'] != 0:
            refusals.append('code de processus non conforme : ' + label)
            return
        try:
            with open(path, encoding='utf-8') as handle:
                text = handle.read()
        except (OSError, UnicodeError):
            refusals.append('journal illisible : ' + label)
            return
        state = lf.parse_output(take['code'], text, attendu)
        if state['etat'] != 'ok' or take.get('etat') != 'ok':
            refusals.append('prise relue non conforme : ' + label)
            return
        summary = take_summary(state)
        summary['empreintes'] = sorted({p['full_sha256'] for p in state['passes']}) if attendu['empreinte'] else []
        # JSON canonique preserve aussi les types : False n'est pas le nombre 0, 0.0 n'est pas 0.
        dumped = lambda x: json.dumps(x, sort_keys=True, allow_nan=False)
        if any(dumped(take.get(key)) != dumped(value) for key, value in summary.items()):
            refusals.append('resume different du journal : ' + label)

    for label in cfg['trames']:
        frame = report['trames'].get(label)
        if frame is None:
            refusals.append("trame d'identite inconnue : " + label)
            continue
        for arm in arms:
            path = os.path.join(folder, 'journaux', 'identite', '%s_%s.jsonl' % (label, arm))
            attendu = expected(arms[arm], cfg['voie'], cfg['k'], cfg['fils'], 2, [(label, frame['sites'])], True)
            one(report.get('identite', {}).get(label, {}).get(arm), path, attendu, 'identite %s %s' % (label, arm))
    for label, rounds in report['campagne'].items():
        frame = report['trames'].get(label)
        if frame is None:
            refusals.append('trame de campagne inconnue : %s' % label)
            continue
        for t, row in enumerate(rounds):
            for arm, take in row.items():
                if arm not in arms:
                    refusals.append('bras inconnu dans la campagne : %s' % arm)
                    continue
                path = os.path.join(folder, 'journaux', 'campagne', label, '%s_t%02d.jsonl' % (arm, t))
                attendu = expected(arms[arm], cfg['voie'], cfg['k'], cfg['fils'], cfg['passes'],
                                   [(label, frame['sites'])], False)
                one(take, path, attendu, 'campagne %s %s tour %d' % (label, arm, t))


def validate_campaign(report, raw_replay):
    """Fermer la configuration et les cohortes avant toute statistique ou relecture."""
    if type(report) is not dict or type(report.get('parametres')) is not dict or \
            type(report.get('regle_bras')) is not dict:
        return 'configuration absente ou mal formee'
    cfg, arms = report['parametres'], report['regle_bras']
    if len(arms) < 2 or any(type(name) is not str or type(opts) is not list or
            any(type(o) is not str for o in opts) or parse_arm(name + '=' + ' '.join(opts)) != (name, opts)
            for name, opts in arms.items()):
        return 'bras mal formes'
    ref, aa = cfg.get('reference'), cfg.get('aa')
    if type(ref) is not str or ref not in arms or (aa is not None and
            (type(aa) is not str or aa not in arms or aa == ref or arms[aa] != arms[ref])):
        return 'reference ou controle A/A incoherent'
    trial = cfg.get('essai', False)
    if type(trial) is not bool or cfg.get('voie') not in ('cpu', 'appareil') or \
            any(type(cfg.get(k)) is not int for k in ('k', 'fils', 'passes', 'tours')) or \
            not 1 <= cfg['k'] <= 12 or cfg['fils'] < 1 or \
            cfg['passes'] < (2 if trial else REGLE_APPARIEE['passes_min']) or \
            cfg['tours'] < (1 if trial else REGLE_APPARIEE['tours_min']):
        return 'regime ou minima de campagne invalides'
    labels = cfg.get('trames')
    if type(labels) is not list or not labels or any(type(x) is not str or not x for x in labels) or \
            len(set(labels)) != len(labels):
        return 'cohorte decisive vide ou mal formee'
    for key in ('identite', 'campagne'):
        if type(report.get(key)) is not dict or set(report[key]) != set(labels):
            return 'cohorte ' + key + ' differente de la configuration'
    if raw_replay and (type(report.get('trames')) is not dict or set(report['trames']) != set(labels) or
            any(type(x) is not dict or type(x.get('sites')) is not int or not 0 < x['sites'] < 2**32
                for x in report['trames'].values())):
        return 'metadonnees de trames invalides'
    if any(type(tours) is not list or len(tours) != cfg['tours'] for tours in report['campagne'].values()):
        return 'nombre de tours different de la configuration'
    return ''


def judge(report, folder=None):
    """Jugement par REGLE_APPARIEE ; folder=None (auto-test seulement) saute le rejeu brut."""
    invalid = validate_campaign(report, folder is not None)
    if invalid:
        return dict(verdict='refuse', refus=[invalid], cas={})
    provenance = report.get('provenance')
    if folder is not None and (type(provenance) is not dict or
            type(provenance.get('sonde_sha256')) is not str or
            re.fullmatch(r'[0-9a-f]{64}', provenance['sonde_sha256']) is None or
            provenance.get('sonde_fin_sha256') != provenance['sonde_sha256']):
        return dict(verdict='refuse', refus=['sonde absente, modifiee ou fermeture non prouvee'], cas={})

    cfg, rule = report['parametres'], REGLE_APPARIEE
    arms, reference, aa = dict(report['regle_bras']), cfg['reference'], cfg.get('aa')
    refusals, cases = [], {}
    if cfg['voie'] == 'appareil' and not cfg.get('essai'):
        for when in ('avant', 'apres'):
            if not bf.environment_ok(report.get('environnement', {}).get(when, {})):
                refusals.append('environnement %s incomplet ou GPU occupe' % when)
    decisive = cfg['trames']
    identity = report.get('identite', {})
    for label in decisive:
        takes = identity.get(label, {})
        digests = set()
        if set(takes) != set(arms) or any(t.get('etat') != 'ok' for t in takes.values()):
            refusals.append('identite absente ou non conforme : %s' % label)
            continue
        for t in takes.values():
            digests.update(t['empreintes'])
        if len(digests) != 1:
            refusals.append('empreintes FUL1 differentes : %s (%d)' % (label, len(digests)))
    camp = report.get('campagne', {})
    for label in decisive:
        rounds = camp.get(label)
        if type(rounds) is not list:  # nombre de tours deja ferme par validate_campaign
            refusals.append('tours manquants : %s' % label)
            continue
        for t, row in enumerate(rounds):
            if type(row) is not dict or set(row) != set(arms) or \
                    any(type(x) is not dict or x.get('etat') != 'ok' or not x.get('mur_chaud_ns')
                        for x in row.values()):
                refusals.append('tour incomplet ou prise manquante : %s tour %d' % (label, t))
    if folder is not None:
        reread(report, folder, refusals)
    if refusals:
        return dict(verdict='refuse', refus=refusals, cas={})
    rng_seed = rule['graine']
    for arm in sorted(arms):
        if arm == reference:
            continue
        per_frame = {}
        for label in decisive:
            logs = [math.log(row[arm]['mur_chaud_ns'] / row[reference]['mur_chaud_ns']) for row in camp[label]]
            gm, lo, hi = bootstrap_gm(logs, random.Random(rng_seed), rule['bootstrap'])
            per_frame[label] = dict(rapport=gm, ic95=[lo, hi], tours=len(logs))
        cases[arm] = dict(trames=per_frame)
    if aa is not None:
        off = [label for label, v in cases[aa]['trames'].items() if abs(v['rapport'] - 1) > rule['fenetre_aa']]
        if off:
            return dict(verdict='refuse', refus=['A/A hors de la fenetre de +/- %.1f %% : %s' % (
                100 * rule['fenetre_aa'], ', '.join(off))], cas=cases)
    for arm, case in cases.items():
        if arm == aa:
            case['verdict'] = 'controle A/A'
            continue
        case['verdict'] = 'adopte' if all(v['ic95'][1] < rule['seuil_borne_haute'] for v in case['trames'].values()) \
            else 'rejete'
    return dict(verdict='juge', refus=[], cas=cases)


def synthetic_report(effects, missing=False, digest_split=False, aa_effect=1.0, noise=0.004, seed=1, short=False,
                     swing=0.0):
    """Rapport synthetique pour l'auto-test du juge : effet multiplicatif par bras, bruit log-normal ; swing non nul :
    sans bruit, les bras de effects alternent exp(+swing) et exp(-swing) d'un tour a l'autre (IC a cheval sur 1)."""
    rng = random.Random(seed)
    arms = {'ref': [], 'aa': [], **{name: ['--cache=1'] for name in effects}}
    labels = ('ng00', 'ng01', 'ng02')
    cfg = dict(reference='ref', aa='aa', trames=list(labels), tours=10, voie='cpu', k=5, fils=3, passes=10)
    identity = {label: {arm: dict(etat='ok', empreintes=['ab' * 32]) for arm in arms} for label in labels}
    if digest_split:
        identity['ng01']['aa']['empreintes'] = ['cd' * 32]
    effect = dict(effects, ref=1.0, aa=aa_effect)
    def wall(arm, t):
        if swing:
            return 1e8 * effect[arm] * (math.exp(swing if t % 2 else -swing) if arm in effects else 1.0)
        return 1e8 * effect[arm] * math.exp(rng.gauss(0, noise))
    camp = {label: [{arm: dict(etat='ok', mur_chaud_ns=wall(arm, t)) for arm in arms} for t in range(10)]
            for label in labels}
    if missing:
        del camp['ng02'][4]['ref']
    if short:
        del camp['ng01'][-1]
    return dict(parametres=cfg, regle_bras=arms, identite=identity, campagne=camp)


def self_test():
    """Le juge sur des campagnes synthetiques : adopte, trois rejets (dont un IC a cheval sur 1, qui separe la borne
    haute de la borne basse) et quatre refus ; tout autre resultat : code 1."""
    errors = []
    cases = [('adopte', synthetic_report({'levier': 0.9}), 'juge', {'levier': 'adopte'}),
             ('rejete', synthetic_report({'levier': 1.0}), 'juge', {'levier': 'rejete'}),
             ('ic_a_cheval', synthetic_report({'levier': 1.0}, swing=0.02), 'juge', {'levier': 'rejete'}),
             ('lent', synthetic_report({'levier': 1.1}), 'juge', {'levier': 'rejete'}),
             ('manque', synthetic_report({'levier': 0.9}, missing=True), 'refuse', {}),
             ('tours_courts', synthetic_report({'levier': 0.9}, short=True), 'refuse', {}),
             ('empreinte', synthetic_report({'levier': 0.9}, digest_split=True), 'refuse', {}),
             ('aa_hors', synthetic_report({'levier': 0.9}, aa_effect=1.03), 'refuse', None)]
    straddle = judge(cases[2][1])['cas']['levier']['trames']
    if not all(v['ic95'][0] < 1 < v['ic95'][1] for v in straddle.values()):
        errors.append('ic_a_cheval : un IC ne contient pas 1 (%s)' % straddle)
    for name, report, verdict, wanted in cases:
        result = judge(report)
        got = {arm: c.get('verdict') for arm, c in result['cas'].items() if arm != 'aa'}
        if result['verdict'] != verdict or (wanted is not None and verdict == 'juge' and got != wanted):
            errors.append('%s : %s %s' % (name, result['verdict'], got))
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('pilote_apparie_auto_test_ok cas=%d' % len(cases))
    return 0


# ---- Campagne --------------------------------------------------------------------------------------------------------
def rotation(names, t):
    """Ordre des bras au tour t : decale d'un bras par tour, sans inversion."""
    return [names[(t + i) % len(names)] for i in range(len(names))]


def frame_files(folder, label):
    stem = os.path.join(folder, 'lidar_' + label)
    xyz, ids = stem + '.u32le', stem + '.ids.u32le'
    if not (os.path.isfile(xyz) and os.path.isfile(ids)) or os.path.getsize(xyz) % 12:
        return None
    return dict(xyz=xyz, ids=ids, sites=os.path.getsize(xyz) // 12)


def session_frames(archive, folder):
    """Trames v12set de l'archive (manifeste du paquet, dans son ordre) ; None si refusee."""
    manifest = bf.unpack(archive, folder)
    if manifest is None:
        return None
    out = []
    for case in manifest.get('cases', []):
        name = case.get('name') if type(case) is dict else None
        if type(name) is not str or not 0 < len(name) <= 23 or '/' in name:
            return None
        xyz = os.path.join(folder, name + '.u32le')
        if not os.path.isfile(xyz) or not os.path.isfile(os.path.join(folder, name + '.ids.u32le')) or \
                os.path.getsize(xyz) % 12:
            return None
        out.append((xyz, os.path.join(folder, name + '.ids.u32le'), name, os.path.getsize(xyz) // 12))
    return out or None


def play_session_info(args, probe, arms, frames, out_dir):
    """Information : Sessions enchainant les trames v12set deux fois, un processus par bras et par tour."""
    info = {arm: {} for arm in arms}
    names = sorted(arms)
    n = len(frames)
    for r in range(args.session_tours):
        for arm in rotation(names, r):
            journal = os.path.join(out_dir, 'journaux', 'session', '%s_r%d.jsonl' % (arm, r))
            argv = probe_argv(probe, [(x, i, label) for x, i, label, _s in frames], args.voie, args.k, args.fils,
                              2 * n, False, arms[arm])
            code, out, _err, _secs = bf.run(argv, args.delai, journal, journal + '.err')
            state = lf.parse_output(code, out, expected(arms[arm], args.voie, args.k, args.fils, 2 * n,
                                                        [(label, s) for _x, _i, label, s in frames], False))
            if state['etat'] != 'ok':
                info[arm].setdefault('_refus', []).append('tour %d : %s %s' % (r, state['etat'], state['raison']))
                continue
            for p in state['passes'][n:]:
                info[arm].setdefault(p['trame'], []).append(p['wall_ns'])
    summary = {}
    for arm in names:
        walls = {label: median(v) for label, v in info[arm].items() if label != '_refus'}
        summary[arm] = dict(trames=walls, refus=info[arm].get('_refus', []),
                            mediane_ns=median(list(walls.values())) if walls else None,
                            maximum_ns=max(walls.values()) if walls else None)
    return summary


def tables(report, verdict):
    cfg = report['parametres']
    lines = ['# Pilote apparie : %s' % ', '.join('%s = %s' % (a, ' '.join(o) or '(defaut)')
                                                 for a, o in sorted(report['regle_bras'].items())), '',
             'Reference : `%s` ; A/A : `%s` ; voie %s, K%d, %d fils, %d tours x %d passes. Jugement : **%s**%s.' % (
                 cfg['reference'], cfg.get('aa'), cfg['voie'], cfg['k'], cfg['fils'], cfg['tours'], cfg['passes'],
                 verdict['verdict'], (' (' + ' ; '.join(verdict['refus']) + ')') if verdict['refus'] else ''), '',
             '| bras | trame | rapport a la reference (IC 95 %) | verdict |', '| --- | --- | --- | --- |']
    for arm, case in sorted(verdict['cas'].items()):
        for label, v in sorted(case['trames'].items()):
            lines.append('| `%s` | %s | %.3f (%.3f-%.3f) | %s |' % (arm, label, v['rapport'], v['ic95'][0],
                                                                    v['ic95'][1], case.get('verdict', '')))
    lines += ['', 'Murs chauds medians des prises (ms) et etages (mediane des medianes de processus) :', '',
              '| trame | bras | mur | P | C | G | queue ou T+M+V+R | CPU par passe | pic (Mio) |',
              '| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |']
    for label, rounds in sorted(report.get('campagne', {}).items()):
        for arm in sorted(report['regle_bras']):
            takes = [row[arm] for row in rounds if row.get(arm, {}).get('etat') == 'ok']
            if not takes:
                continue
            med = lambda f: median([f(x) for x in takes]) / 1e6  # noqa: E731
            lines.append('| %s | `%s` | %.1f | %.1f | %.1f | %.1f | %.1f | %.1f | %.0f |' % (
                label, arm, med(lambda x: x['mur_chaud_ns']), med(lambda x: x['etapes_ns']['P']),
                med(lambda x: x['etapes_ns']['C']), med(lambda x: x['etapes_ns']['G']),
                med(lambda x: x['etapes_ns']['TMVR']), med(lambda x: x['cpu_ns']),
                max(x['pic_octets'] for x in takes) / 2 ** 20))
    info = report.get('session_v12set')
    if info:
        lines += ['', 'Session v12set (information, second passage, mediane des processus) :', '',
                  '| bras | mediane (ms) | maximum (ms) | trames | refus |', '| --- | ---: | ---: | ---: | --- |']
        for arm, s in sorted(info.items()):
            lines.append('| `%s` | %s | %s | %d | %s |' % (
                arm, '-' if s['mediane_ns'] is None else '%.1f' % (s['mediane_ns'] / 1e6),
                '-' if s['maximum_ns'] is None else '%.1f' % (s['maximum_ns'] / 1e6), len(s['trames']),
                ' ; '.join(s['refus']) or '-'))
    return '\n'.join(lines) + '\n'


def main(argv):
    if argv[1:] == ['auto-test']:
        return self_test()
    parser = argparse.ArgumentParser()
    for name in ('--src', '--travail', '--sonde', '--archive-v12set', '--aa'):
        parser.add_argument(name)
    for name in ('--donnees', '--sortie', '--reference'):
        parser.add_argument(name, required=True)
    parser.add_argument('--bras', action='append', default=[])
    parser.add_argument('--trames', default='ng00,ng01,ng02')
    parser.add_argument('--voie', default='appareil', choices=('appareil', 'cpu'))
    for name, default in (('--k', 5), ('--fils', 48), ('--tours', 10), ('--passes', 10), ('--session-tours', 2),
                          ('--jobs', 44), ('--delai', 900)):
        parser.add_argument(name, type=int, default=default)
    parser.add_argument('--essai', action='store_true')
    args = parser.parse_args(argv[1:])
    parsed = [parse_arm(b) for b in args.bras]
    arms = dict(p for p in parsed if p is not None)
    labels = args.trames.split(',')
    if None in parsed or len(arms) != len(parsed) or len(arms) < 2 or args.reference not in arms or \
            (args.aa is not None and (args.aa not in arms or args.aa == args.reference or
                                      arms[args.aa] != arms[args.reference])) or \
            args.essai != bool(args.sonde) or (not args.essai and not (args.src and args.travail)) or \
            not 1 <= args.k <= 12 or args.fils < 1 or args.session_tours < 1 or \
            (not args.essai and (args.tours < REGLE_APPARIEE['tours_min'] or
                                 args.passes < REGLE_APPARIEE['passes_min'])) or args.passes < 2 or \
            len(set(labels)) != len(labels):
        print('pilote_apparie : usage refuse', file=sys.stderr)
        return 2
    if args.essai:
        args.voie = 'cpu'
    frames = {label: frame_files(args.donnees, label) for label in labels}
    if any(f is None for f in frames.values()):
        print('pilote_apparie : trame absente ou mal formee', file=sys.stderr)
        return 2
    out_dir = os.path.abspath(args.sortie)
    for sub in ('identite', 'campagne', 'session'):
        os.makedirs(os.path.join(out_dir, 'journaux', sub), exist_ok=True)
    with_gpu = args.voie == 'appareil'
    nvcc = shutil.which('nvcc') or '/usr/local/cuda/bin/nvcc'
    report = dict(mesure='pilote_apparie', regle=REGLE_APPARIEE, regle_bras=arms,
                  parametres=dict(reference=args.reference, aa=args.aa, trames=labels, tours=args.tours,
                                  passes=args.passes, voie=args.voie, k=args.k, fils=args.fils, essai=args.essai,
                                  argv=argv[1:]),
                  trames={label: dict(sites=f['sites']) for label, f in frames.items()},
                  environnement=dict(avant=bf.environment(nvcc, with_gpu)))
    if args.essai:
        probe, cmake = args.sonde, None
    else:
        probe = build_probe(args, nvcc, os.path.join(out_dir, 'construction.log'), with_gpu)
        if probe is None:
            return 3
        cmake = bf.cmake_extract(args.travail)
    report['provenance'] = dict(sonde_sha256=bf.sha256_file(probe), pilote_sha256=bf.sha256_file(__file__),
                                lecteur_sha256=bf.sha256_file(lf.__file__), cmake=cmake)
    t_start = time.monotonic()
    names = sorted(arms)
    report['identite'] = {}
    for label, f in frames.items():
        report['identite'][label] = {}
        for arm in names:
            journal = os.path.join(out_dir, 'journaux', 'identite', '%s_%s.jsonl' % (label, arm))
            argv_take = probe_argv(probe, [(f['xyz'], f['ids'], label)], args.voie, args.k, args.fils, 2, True,
                                   arms[arm])
            report['identite'][label][arm] = play(argv_take, expected(arms[arm], args.voie, args.k, args.fils, 2,
                                                                      [(label, f['sites'])], True), journal,
                                                  args.delai)
    report['campagne'], report['ordres'] = {}, {}
    for label, f in frames.items():
        folder = os.path.join(out_dir, 'journaux', 'campagne', label)
        os.makedirs(folder, exist_ok=True)
        rounds, orders = [], []
        for t in range(args.tours):
            row = {}
            orders.append(rotation(names, t))
            for arm in orders[-1]:
                journal = os.path.join(folder, '%s_t%02d.jsonl' % (arm, t))
                argv_take = probe_argv(probe, [(f['xyz'], f['ids'], label)], args.voie, args.k, args.fils,
                                       args.passes, False, arms[arm])
                row[arm] = play(argv_take, expected(arms[arm], args.voie, args.k, args.fils, args.passes,
                                                    [(label, f['sites'])], False), journal, args.delai)
            rounds.append(row)
        report['campagne'][label] = rounds
        report['ordres'][label] = orders
    report['duree_campagne_s'] = round(time.monotonic() - t_start, 1)
    report['environnement']['apres'] = bf.environment(nvcc, with_gpu)
    if args.archive_v12set:
        frames_v = session_frames(args.archive_v12set, os.path.join(args.travail or out_dir, 'v12set'))
        report['session_v12set'] = None if frames_v is None else play_session_info(args, probe, arms, frames_v,
                                                                                    out_dir)
    report['provenance']['sonde_fin_sha256'] = file_sha(probe)
    verdict = judge(report, out_dir)
    if args.essai:
        verdict = dict(verdict, verdict_calcule=verdict['verdict'], verdict='essai')
    report['jugement'] = verdict
    report['duree_s'] = round(time.monotonic() - t_start, 1)
    with open(os.path.join(out_dir, 'rapport_apparie.json'), 'w', encoding='utf-8') as handle:
        json.dump(report, handle, indent=1, sort_keys=True)
    with open(os.path.join(out_dir, 'tableaux_apparie.md'), 'w', encoding='utf-8') as handle:
        handle.write(tables(report, verdict))
    print('pilote_apparie_verdict %s refus=%d' % (verdict['verdict'], len(verdict['refus'])))
    return 0


def build_probe(args, nvcc, log_path, with_gpu):
    """Sonde Release au profil 21 : avec CUDA pour la voie appareil (banc_full.build), sinon sans."""
    if with_gpu:
        return bf.build(args.src, args.travail, nvcc, args.jobs, log_path)
    os.makedirs(args.travail, exist_ok=True)
    configure = ['cmake', '-S', os.path.join(args.src, 'morsehgp3D_v12'), '-B', args.travail,
                 '-DCMAKE_BUILD_TYPE=Release', '-DMHGP12_COORD_BITS=21']
    for argv in (configure, ['cmake', '--build', args.travail, '-j', str(args.jobs), '--target', 'mhgp12_full_probe']):
        code, out, err, _s = bf.run(argv, 3600)
        with open(log_path, 'a', encoding='utf-8') as log:
            log.write('$ %s\n%s%s' % (' '.join(argv), out, err))
        if code != 0:
            return None
    probe = os.path.join(args.travail, 'mhgp12_full_probe')
    return probe if os.path.isfile(probe) else None


if __name__ == '__main__':
    sys.exit(main(sys.argv))
