#!/usr/bin/env python3
"""MES-C : la tour FULL de la v12 sur les petits nuages (100 a 10 000 sites), regime (c) de la decision D7, sur G4.

Paquet g4_small (159 nuages, docs/DONNEES.md) voyageant en une seule archive tar (--archive, deballee sous --deballage
apres controle de chaque membre) : morceaux d'objets et de contexte SemanticKITTI, boules K-NN et bouts de scenes de la
v11 (groupe « reel »), familles synthetiques saines (uniform, clusters8, slab) et difficiles (lattice : cosphericite
massive ; line : colineaire ; sphere : quasi-sphere arrondie).

Etapes : environnement et GPU vide avant et apres ; construction Release au profil 21 avec CUDA de mhgp12_full_probe
(journal, empreinte SHA-256, extrait du CMakeCache) ; puis
  1. pour chaque configuration (voie, K, fils) : UNE Session (un processus) qui enchaine les nuages reels et
     synthetiques sains, --tours fois dans le meme ordre (passe p = nuage p modulo n), avec --digest ; valeur chaude
     d'un nuage = mediane de ses passes des tours 2 et suivants ;
  2. chaque nuage difficile seul, voies CPU et appareil a 48 fils (ou --fils-difficiles), --tours-difficiles passes,
     delai propre --delai-cas : un refus, un echec ou une expiration y est un resultat publie.
Ordre : K5 en entier (Sessions, puis nuages difficiles), puis K10 : les criteres ne dependent que de K5.
Delai global --delai-global : une etape dont la prevision depasse le temps restant n'est pas lancee (non jouee).
Lecture stricte de chaque sortie par le lecteur partage microbancs/outils/lecteur_full.py (trame et sites attendus a
chaque passe, budget de l'appareil separe). Empreinte FUL1 identique sur toutes les passes d'un nuage, et entre les
voies a K egal ; sinon controle manquant.

Ajustements : par configuration et par groupe (reel ; chaque famille synthetique), moindres carres t = a + b n sur les
valeurs chaudes (a : cout fixe, b : cout par site) ; jamais une droite qui melange des groupes ou des nombres de fils
(CST-0238).

Verdicts ecrits d'avance (objectifs du regime (c), MESURE.md) :
  C1 : voie CPU, K5, 48 fils, groupe reel : ordonnee a l'origine de la droite des moindres carres (cout fixe a chaud
       extrapole a zero site, descriptif) au plus 2 ms ;
  C2 : meme configuration : pente des moindres carres au plus 3 727,2 ns par site, rapport de la mediane des temps
       (241,3 ms) a la mediane des sites (64 740) des trames v12set de la session K, voie appareil. C2 juge une pente,
       pas un plafond par nuage : les points et les residus sont publies a cote ;
  C3 : la cohorte complete des nuages difficiles, a K5, voies CPU et appareil, 48 fils (une ligne jouee par couple
       nuage et voie, ni absente ni doublee) : aucun refus, echec ni expiration ; cohorte incomplete : non evalue
       (correctif de l'auditeur Codex, receipts/audit_reponses_20261008/mes_c_livraison).
Verdict d'ensemble : « tenu » si C1, C2 et C3 sont tenus ; « non tenu » si l'un ne l'est pas ; « refuse » si un
controle manque (sortie illisible, empreinte instable, environnement incomplet ou GPU occupe, Session en echec) ou si
un critere n'est pas evalue.

Usage : pilote_c.py --src SRC --travail DOSSIER --archive TAR --deballage DOSSIER --sortie DOSSIER
                    [--voies cpu,appareil] [--k 5,10] [--fils 1,4,48] [--tours 3] [--tours-difficiles 2]
                    [--fils-difficiles 48] [--delai-global 1800] [--delai-cas 120] [--jobs 44]
                    [--essai --sonde BINAIRE]   essai local : sonde existante, voie CPU, verdict « essai »
Sorties : <sortie>/rapport_c.json, <sortie>/tableaux_c.md, <sortie>/brut/, <sortie>/construction.log.
Codes : 0 rendu (quel que soit le verdict) ; 2 usage ou donnees ; 3 construction impossible. Python 3.10 nu, aucun
assert (tient sous -O).
"""
import argparse
import json
import os
import shutil
import statistics
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'outils'))
import banc_full as bf  # noqa: E402
import lecteur_full as lf  # noqa: E402

GIB = 1 << 30
REAL = ('boule_knn', 'bout_v11', 'objet_reel', 'objet_reel_contexte')
SOUND = ('uniform', 'clusters8', 'slab')
HARD = ('lattice', 'line', 'sphere')
MAIN_REGIME_NS_PER_SITE = 241.3e6 / 64740  # session K, mediane de v12set, voie appareil, K5
FIXED_LIMIT_NS = 2e6


def group_of(case):
    """Groupe d'un nuage du manifeste : 'reel', une famille synthetique, ou None (inconnu : refus)."""
    if case.get('subfamily') in REAL:
        return 'reel'
    family = (case.get('provenance') or {}).get('family')
    return family if family in SOUND + HARD else None


def clouds_of(manifest, folder):
    """Nuages du paquet : liste de dict(nom, etiquette, sites, groupe, xyz, ids) ; None si un cas est mal forme."""
    out, labels = [], set()
    for case in manifest.get('cases', []):
        group, sites = group_of(case), case.get('count')
        if group is None or type(sites) is not int or sites <= 0 or '/' in case['coordinates'] or \
                '/' in case['point_ids']:
            return None
        label = 'c%03d' % len(out)  # etiquette courte et unique (au plus 23 octets)
        labels.add(label)
        out.append(dict(nom=case['name'], etiquette=label, sites=sites, groupe=group,
                        xyz=os.path.join(folder, case['coordinates']), ids=os.path.join(folder, case['point_ids'])))
    return out or None


def fit(points):
    """Moindres carres t = a + b n ; rend (a, b) en ns et ns par site, ou None sous deux tailles distinctes."""
    xs = [x for x, _ in points]
    if len(set(xs)) < 2:
        return None
    mx, my = statistics.fmean(xs), statistics.fmean(y for _, y in points)
    b = sum((x - mx) * (y - my) for x, y in points) / sum((x - mx) ** 2 for x in xs)
    return my - b * mx, b


def probe_argv(probe, clouds, voie, k, fils, passes, budget):
    argv = [probe] + ['--trame=%s,%s,%s' % (c['xyz'], c['ids'], c['etiquette']) for c in clouds]
    argv += ['--k=%d' % k, '--threads=%d' % fils, '--passes=%d' % passes, '--digest', '--budget=%d' % budget]
    if voie == 'appareil':
        argv += ['--device', '--budget-appareil=%d' % budget]
    return argv


def expected(clouds, voie, k, fils, passes):
    return dict(voie=voie, k=k, fils=fils, passes=passes, empreinte=True, budget_appareil='separe', bits=21,
                trames=[(c['etiquette'], c['sites']) for c in clouds])


def session_values(passes, clouds):
    """Par nuage : premiere passe, mediane chaude (tours 2 et suivants), empreintes."""
    n, out = len(clouds), {}
    for i, c in enumerate(clouds):
        mine = passes[i::n]
        warm = [p['wall_ns'] for p in mine[1:]]
        out[c['nom']] = dict(sites=c['sites'], groupe=c['groupe'], premiere_ns=mine[0]['wall_ns'] if mine else None,
                             chaud_ns=statistics.median(warm) if warm else None,
                             cpu_ns=statistics.median(p['cpu_ns'] for p in mine[1:]) if warm else None,
                             empreintes=sorted(set(p['full_sha256'] for p in mine)))
    return out


def fits_of(values):
    """Droites par groupe (reel, chaque famille synthetique) sur les valeurs chaudes."""
    groups = {}
    for v in values.values():
        if v['chaud_ns'] is not None:
            groups.setdefault(v['groupe'], []).append((v['sites'], v['chaud_ns']))
    out = {}
    for group, points in sorted(groups.items()):
        line = fit(points)
        out[group] = None if line is None else dict(fixe_ns=line[0], par_site_ns=line[1], nuages=len(points))
    return out


def verdicts(configs, hard, hard_names):
    out = {}
    ref = configs.get('cpu:5:48')
    line = ref and ref.get('droites', {}).get('reel')
    out['C1'] = dict(etat='non evalue' if not line else 'tenu' if line['fixe_ns'] <= FIXED_LIMIT_NS else 'non tenu',
                     detail='cout fixe %.3f ms' % (line['fixe_ns'] / 1e6) if line else 'configuration cpu:5:48 absente')
    out['C2'] = dict(etat='non evalue' if not line else 'tenu' if line['par_site_ns'] <= MAIN_REGIME_NS_PER_SITE
                     else 'non tenu', detail='%.3f us par site (limite %.3f)' % (
                         line['par_site_ns'] / 1e3, MAIN_REGIME_NS_PER_SITE / 1e3) if line else '')
    # C3 porte sur tous les nuages difficiles, les deux voies, K5 et 48 fils.
    # Un sous-ensemble joue ne remplace jamais cette cohorte.
    k5 = [h for h in hard if h['k'] == 5]
    wanted = {(name, voie) for name in hard_names for voie in ('cpu', 'appareil')}
    keys = [(h['nom'], h['voie']) for h in k5]
    complete = bool(wanted) and len(keys) == len(wanted) and set(keys) == wanted and all(
        h['fils'] == 48 and h['etat'] != 'non_joue' for h in k5)
    bad = ['%s %s : %s (%s)' % (h['nom'], h['voie'], h['etat'], h['raison']) for h in k5 if h['etat'] != 'ok']
    out['C3'] = dict(etat='non evalue' if not complete else 'non tenu' if bad else 'tenu',
                     detail=['cohorte C3 incomplete ou hors configuration'] if not complete else
                     bad or ['%d executions difficiles a K5 calculees' % len(k5)])
    return out


def overall(essai, controls, crit):
    """Verdict d'ensemble : essai ; refuse si un controle manque ou un critere n'est pas evalue ; tenu si C1 a C3
    sont tenus ; non tenu sinon."""
    if essai:
        return 'essai'
    if controls or any(crit[c]['etat'] == 'non evalue' for c in ('C1', 'C2', 'C3')):
        return 'refuse'
    return 'tenu' if all(crit[c]['etat'] == 'tenu' for c in ('C1', 'C2', 'C3')) else 'non tenu'


def play_session(probe, session, voie, k, f, tours, budget, remaining, raw, controls):
    """Une Session qui enchaine les nuages sains, tours fois ; rend l'entree de la configuration."""
    key = '%s:%d:%d' % (voie, k, f)
    passes = len(session) * tours
    code, out, _err, secs = bf.run(probe_argv(probe, session, voie, k, f, passes, budget), remaining,
                                   os.path.join(raw, 'session_%s.jsonl' % key.replace(':', '_')))
    state = lf.parse_output(code, out, expected(session, voie, k, f, passes))
    entry = dict(etat=state['etat'], raison=state['raison'], secondes=round(secs, 1))
    if state['etat'] == 'ok':
        values = session_values(state['passes'], session)
        entry.update(valeurs=values, droites=fits_of(values))
        for name, v in values.items():
            if len(v['empreintes']) != 1:
                controls.append('%s %s : %d empreintes FUL1' % (key, name, len(v['empreintes'])))
    else:
        controls.append('Session %s : %s (%s)' % (key, state['etat'], state['raison']))
    return entry


def play_hard(probe, c, voie, k, args, budget, remaining, raw, controls):
    """Un nuage difficile seul ; un refus, un echec ou une expiration est un resultat publie."""
    row = dict(nom=c['nom'], groupe=c['groupe'], sites=c['sites'], k=k, voie=voie, fils=args.fils_difficiles,
               etat='non_joue', raison='', passes=[])
    if remaining < 30:
        row['raison'] = 'delai'
        return row
    tag = '%s_k%d_%s' % (c['nom'], k, voie)
    code, out, _err, secs = bf.run(probe_argv(probe, [c], voie, k, args.fils_difficiles, args.tours_difficiles, budget),
                                   min(remaining, args.delai_cas), os.path.join(raw, tag + '.jsonl'))
    state = lf.parse_output(code, out, expected([c], voie, k, args.fils_difficiles, args.tours_difficiles))
    row.update(etat=state['etat'], raison=state['raison'], secondes=round(secs, 1),
               passes=[dict(mur_ns=p['wall_ns'], full_sha256=p['full_sha256']) for p in state['passes']])
    if state['etat'] == 'illisible':
        controls.append('%s : %s' % (tag, state['raison']))
    return row


def main(argv):
    parser = argparse.ArgumentParser()
    for name in ('--src', '--travail', '--sonde'):
        parser.add_argument(name)
    for name in ('--archive', '--deballage', '--sortie'):
        parser.add_argument(name, required=True)
    parser.add_argument('--voies', default='cpu,appareil')
    parser.add_argument('--k', default='5,10')
    parser.add_argument('--fils', default='1,4,48')
    parser.add_argument('--fils-difficiles', type=int, default=48)
    parser.add_argument('--tours', type=int, default=3)
    parser.add_argument('--tours-difficiles', type=int, default=2)
    parser.add_argument('--delai-global', type=float, default=1800.0)
    parser.add_argument('--delai-cas', type=float, default=120.0)
    parser.add_argument('--budget-gio', type=float, default=64.0)
    parser.add_argument('--jobs', type=int, default=44)
    parser.add_argument('--essai', action='store_true')
    args = parser.parse_args(argv[1:])
    t_start = time.monotonic()
    try:
        voies = args.voies.split(',')
        ks = [int(x) for x in args.k.split(',')]
        fils = [int(x) for x in args.fils.split(',')]
    except ValueError:
        return 2
    if any(v not in lf.VOIES for v in voies) or args.tours < 2 or args.tours_difficiles < 1 or \
            args.essai != bool(args.sonde) or (not args.essai and not (args.src and args.travail)):
        print('pilote_c : usage refuse', file=sys.stderr)
        return 2
    if args.essai:
        voies = ['cpu']
    manifest = bf.unpack(args.archive, args.deballage)
    clouds = clouds_of(manifest, args.deballage) if manifest else None
    if clouds is None:
        print('pilote_c : archive ou manifeste refuses', file=sys.stderr)
        return 2
    raw = os.path.join(args.sortie, 'brut')
    os.makedirs(raw, exist_ok=True)
    nvcc = shutil.which('nvcc') or '/usr/local/cuda/bin/nvcc'
    with_gpu = not args.essai
    report = dict(mesure='MES-C', regime='c', environnement=dict(avant=bf.environment(nvcc, with_gpu)), controles=[])
    if args.essai:
        probe, cmake = args.sonde, None
    else:
        probe = bf.build(args.src, args.travail, nvcc, args.jobs, os.path.join(args.sortie, 'construction.log'))
        if probe is None:
            return 3
        cmake = bf.cmake_extract(args.travail)
    report['provenance'] = dict(sonde_sha256=bf.sha256_file(probe), pilote_sha256=bf.sha256_file(__file__),
                                lecteur_sha256=bf.sha256_file(lf.__file__), archive_sha256=bf.sha256_file(args.archive),
                                cmake=cmake)
    budget = int(args.budget_gio * GIB)
    session = [c for c in clouds if c['groupe'] not in HARD]
    hard_clouds = [c for c in clouds if c['groupe'] in HARD]
    configs, controls, hard = {}, report['controles'], []
    for k in ks:  # K5 en entier (Sessions puis nuages difficiles) avant K10 : C1 a C3 ne dependent que de K5
        for voie in voies:
            for f in fils:
                key = '%s:%d:%d' % (voie, k, f)
                remaining = args.delai_global - (time.monotonic() - t_start)
                if remaining < 60:
                    configs[key] = dict(etat='non_joue', raison='delai')
                    continue
                configs[key] = play_session(probe, session, voie, k, f, args.tours, budget, remaining, raw, controls)
        for voie in voies:
            for c in hard_clouds:
                remaining = args.delai_global - (time.monotonic() - t_start)
                hard.append(play_hard(probe, c, voie, k, args, budget, remaining, raw, controls))
    # Identite entre voies a K egal (Session et nuages difficiles).
    for k in ks:
        seen = {}
        for voie in voies:
            for f in fils:
                for name, v in configs.get('%s:%d:%d' % (voie, k, f), {}).get('valeurs', {}).items():
                    seen.setdefault(name, set()).update(v['empreintes'])
        for h in hard:
            if h['k'] == k:
                seen.setdefault(h['nom'], set()).update(p['full_sha256'] for p in h['passes'])
        controls += ['K%d %s : empreintes differentes entre voies ou fils' % (k, n)
                     for n, s in seen.items() if len(s) > 1]
    report['environnement']['apres'] = bf.environment(nvcc, with_gpu)
    if with_gpu:
        for when in ('avant', 'apres'):
            if not bf.environment_ok(report['environnement'][when]):
                controls.append('environnement %s incomplet ou GPU occupe' % when)
    criteria = verdicts(configs, hard, [c['nom'] for c in hard_clouds])
    report.update(configurations=configs, difficiles=hard, criteres=criteria,
                  parametres=dict(argv=argv[1:], tours=args.tours, budget_octets=budget),
                  duree_s=round(time.monotonic() - t_start, 1))
    report['verdict'] = overall(args.essai, controls, report['criteres'])
    with open(os.path.join(args.sortie, 'rapport_c.json'), 'w', encoding='utf-8') as handle:
        json.dump(report, handle, indent=1, sort_keys=True)
    with open(os.path.join(args.sortie, 'tableaux_c.md'), 'w', encoding='utf-8') as handle:
        handle.write(tables(report))
    print('mes_c verdict=%s configurations=%d difficiles=%d controles=%d' % (
        report['verdict'], len(configs), len(hard), len(controls)))
    return 0


def tables(report):
    lines = ['# MES-C : petits nuages, tour FULL de la v12', '',
             'Verdict d\'ensemble : **%s**.' % report['verdict'], '']
    for k, v in report['criteres'].items():
        detail = v['detail'] if isinstance(v['detail'], str) else ' ; '.join(v['detail'])
        lines.append('- %s : %s — %s' % (k, v['etat'], detail))
    lines += ['', 'Droites t = a + b n sur les valeurs chaudes (a en ms, b en us par site) :', '',
              '| configuration (voie:K:fils) | etat | groupe | nuages | a (ms) | b (us/site) |',
              '| --- | --- | --- | ---: | ---: | ---: |']
    for key, entry in report['configurations'].items():
        for group, line in sorted(entry.get('droites', {}).items()):
            if line:
                lines.append('| %s | %s | %s | %d | %.3f | %.3f |' % (key, entry['etat'], group, line['nuages'],
                                                                      line['fixe_ns'] / 1e6, line['par_site_ns'] / 1e3))
        if not entry.get('droites'):
            lines.append('| %s | %s (%s) | — | — | — | — |' % (key, entry['etat'], entry.get('raison', '')))
    lines += ['', 'Familles difficiles (chaque nuage seul) :', '',
              '| nuage | sites | K | voie | etat | mur (ms, derniere passe) |',
              '| --- | ---: | ---: | --- | --- | ---: |']
    for h in report['difficiles']:
        last = h['passes'][-1]['mur_ns'] / 1e6 if h['passes'] else None
        lines.append('| `%s` | %d | %d | %s | %s%s | %s |' % (h['nom'], h['sites'], h['k'], h['voie'], h['etat'],
                                                             (' (%s)' % h['raison']) if h['raison'] else '',
                                                             '%.2f' % last if last is not None else '—'))
    lines += ['', 'Controles manquants : %d.' % len(report['controles'])] + ['- ' + c for c in report['controles']]
    return '\n'.join(lines) + '\n'


if __name__ == '__main__':
    sys.exit(main(sys.argv))
