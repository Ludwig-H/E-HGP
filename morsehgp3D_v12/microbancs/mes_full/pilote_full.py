#!/usr/bin/env python3
"""MES-FULL : la tour FULL de la v12 en Session residente sur G4, frontiere du mur de bench/full_probe.cpp (proposee par
l'auditeur Codex le 8 octobre, receipts/audit_reponses_20261008/frontiere_full_proposee).

Mesure du contrat (FULL K1..5 en memoire, verticales comprises, 100 ms sur G4 sur les trames SemanticKITTI sans sol,
plusieurs sequences, mediane et maximum), publiee telle quelle : aucune regle d'adoption, un verdict de contrat
« tenu » ou « non tenu » ecrit d'avance, et un refus si une preuve manque.

Etapes : environnement (nvcc, nvidia-smi, GPU isole avant et apres) ; construction Release au profil 21 avec
MHGP12_ENABLE_CUDA=ON de mhgp12_full_probe ; deballage de l'archive des 37 trames v12set (membres controles) ; puis,
toutes passes avec --digest (empreinte FUL1 hors du mur) :
  A. ng00, ng01, ng02 a K5, voie appareil : --processus processus par trame, --passes passes, ordre tournant ;
  B. les memes a K10, voie appareil : 3 processus x 5 passes ;
  C. bras CPU identifie : les memes a K5, voie CPU, 3 processus x 5 passes ;
  D. Session qui enchaine les 37 trames v12set a K5, voie appareil : --processus processus, deux tours chacun (le second
     fait foi), ordre des trames tourne d'un processus a l'autre.
Controles (refus si l'un manque) : chaque processus rend 0 et toutes ses passes, lues par le lecteur strict partage
microbancs/outils/lecteur_full.py (schema exact, trame et sites attendus a chaque passe, budget de l'appareil
« partage », memoire par etage, mur non nul) ; une empreinte par trame et par K, identique sur toutes les passes, tous
les processus et les deux voies (appareil = CPU) ; environnement complet et GPU connu vide avant et apres (chaine vide,
jamais absente ; contre-lecteur de l'auditeur, receipts/audit_reponses_20261008/mes_full_contrelecture).
Provenance publiee : journal de construction, empreinte SHA-256 de la sonde, extrait du CMakeCache ; temps CPU par
passe (cpu_ns) en mediane par trame.
Statistiques (K5) : par trame, mediane des passes chaudes (passes 2..P d'un processus ; second tour en D) et maximum
des medianes par processus ; sur les trames : mediane et maximum de ces valeurs ; etages en mediane.
Verdict du contrat, ecrit d'avance : « tenu » si, sur ng00-02 ET sur les 37 trames v12set, la mediane et le maximum
(sur les trames, des maximums des medianes par processus) sont au plus 100 ms ; « non tenu » sinon ; « refuse » si un
controle manque.

Usage : pilote_full.py --src SRC --travail DOSSIER --donnees DOSSIER --sortie DOSSIER --archive-v12set TAR
                       [--fils 48] [--processus 5] [--passes 10] [--jobs 44] [--delai 900]
                       [--essai --sonde BINAIRE]   essai local de la logique : sonde existante, voie CPU partout, minima
                                                  releves, deux trames v12set ; verdict marque « essai », jamais publie
Sorties : <sortie>/rapport_full.json, <sortie>/tableaux_full.md, sorties brutes sous <sortie>/brut/.
Codes : 0 rendu (quel que soit le verdict) ; 2 usage ou donnees ; 3 construction impossible. Python 3.10 nu.
"""
import argparse
import hashlib
import json
import os
import shutil
import signal
import statistics
import subprocess
import sys
import tarfile
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'outils'))
import lecteur_full as lf  # noqa: E402  lecteur strict partage avec MES-B

BUDGET_NS = 100_000_000
FRAMES = ('ng00', 'ng01', 'ng02')
STAGES = ('P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R')
# Voie jouee : la Session recouverte, voie par defaut de la sonde depuis l'adoption de T2-d-A (schema "recouvert") ;
# --sequentiel joue l'ancienne voie (resolve_tower puis build_forests) avec son schema. Fixe une fois par main.
MODE = dict(schema='recouvert', flags=[])


def run(argv, delay, raw_path=None):
    proc = subprocess.Popen([str(a) for a in argv], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=True)
    try:
        out, err = proc.communicate(timeout=delay)
        code = proc.returncode
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        out, err = proc.communicate()
        code = 'expire'
    if raw_path is not None:
        with open(raw_path, 'wb') as handle:
            handle.write(out)
    return code, out.decode('utf-8', 'replace'), err.decode('utf-8', 'replace')


def environment(nvcc):
    env = {}
    for name, argv in (('nvcc', [nvcc, '--version'] if nvcc else None), ('cmake', ['cmake', '--version']),
                       ('gpu', ['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,memory.total',
                                '--format=csv,noheader']),
                       ('gpu_apps', ['nvidia-smi', '--query-compute-apps=pid', '--format=csv,noheader'])):
        if argv is None:
            env[name] = None
            continue
        try:
            code, out, _err = run(argv, 60)
            env[name] = out.strip() if code == 0 else None
        except OSError:
            env[name] = None
    return env


def build(src, folder, nvcc, jobs):
    os.makedirs(folder, exist_ok=True)
    configure = ['cmake', '-S', os.path.join(src, 'morsehgp3D_v12'), '-B', folder, '-DCMAKE_BUILD_TYPE=Release',
                 '-DMHGP12_COORD_BITS=21', '-DMHGP12_ENABLE_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc]
    for argv in (configure, ['cmake', '--build', folder, '-j', str(jobs), '--target', 'mhgp12_full_probe']):
        code, out, err = run(argv, 3600)
        with open(os.path.join(folder, 'construction.log'), 'a', encoding='utf-8') as log:
            log.write(out + err)
        if code != 0:
            return None
    probe = os.path.join(folder, 'mhgp12_full_probe')
    return probe if os.path.isfile(probe) else None


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 22), b''):
            digest.update(block)
    return digest.hexdigest()


CMAKE_KEYS = ('CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_CUDA_COMPILER', 'CMAKE_CUDA_ARCHITECTURES',
              'CMAKE_CXX_FLAGS', 'CMAKE_CUDA_FLAGS', 'MHGP12_')


def cmake_extract(folder):
    """Lignes du CMakeCache qui fixent compilateurs, options et profil ; None si le cache manque."""
    try:
        with open(os.path.join(folder, 'CMakeCache.txt'), encoding='utf-8', errors='replace') as handle:
            return [line.rstrip('\n') for line in handle if line.startswith(CMAKE_KEYS)]
    except OSError:
        return None


def unpack(archive, folder):
    """Archive tar plate (fichiers simples a nom simple) ; rend les trames du manifeste, ou None si refusee."""
    os.makedirs(folder, exist_ok=True)
    try:
        with tarfile.open(archive) as tar:
            members = tar.getmembers()
            for m in members:
                if not m.isfile() or '/' in m.name or m.name in ('', '.', '..') or m.name.startswith('.'):
                    return None
            for m in members:
                with open(os.path.join(folder, m.name), 'wb') as out:
                    out.write(tar.extractfile(m).read())
        manifest = json.load(open(os.path.join(folder, 'bundle_manifest.json'), encoding='utf-8'))
        names = [case['name'] for case in manifest['cases']]
    except (OSError, ValueError, KeyError, TypeError, tarfile.TarError):
        return None
    for name in names:
        for suffix in ('.u32le', '.ids.u32le'):
            if not os.path.isfile(os.path.join(folder, name + suffix)):
                return None
    return names


def frame_sites(path):
    """Sites d'une trame : taille du fichier de coordonnees u32le (trois mots par site, positions distinctes)."""
    size = os.path.getsize(path)
    return size // 12 if size % 12 == 0 else None


def parse_process(code, text, passes, kmax, device, threads, frames):
    """Passes 'full' d'un processus lues par le lecteur partage ; rend (passes, refus) ; refus non vide si un controle
    manque ou si le processus n'aboutit pas (frames : [(etiquette, sites)], la passe p joue la trame p modulo n)."""
    expected = dict(voie='appareil' if device else 'cpu', k=kmax, fils=threads, passes=passes, empreinte=True,
                    trames=frames, budget_appareil='partage', bits=21, schema=MODE['schema'])
    state = lf.parse_output(code, text, expected)
    if state['etat'] != 'ok':
        return [], '%s : %s' % (state['etat'], state['raison'])
    return state['passes'], ''


def campaign_frames(probe, data, frames, kmax, processes, passes, threads, device, delay, raw_dir, tag):
    """Un processus par (trame, repetition), ordre tournant ; rend {trame: [[passes d'un processus], ...]}, refus."""
    result, refusals = {f: [] for f in frames}, []
    for rep in range(processes):
        for f in frames[rep % len(frames):] + frames[:rep % len(frames)]:
            argv = [probe, '--trame=%s,%s,%s' % (os.path.join(data, 'lidar_%s.u32le' % f),
                                                os.path.join(data, 'lidar_%s.ids.u32le' % f), f),
                    '--k=%d' % kmax, '--threads=%d' % threads, '--passes=%d' % passes, '--digest'] + MODE['flags']
            if device:
                argv.append('--device')
            code, out, _err = run(argv, delay, os.path.join(raw_dir, '%s_%s_r%d.jsonl' % (tag, f, rep)))
            sites = frame_sites(os.path.join(data, 'lidar_%s.u32le' % f))
            full, refusal = parse_process(code, out, passes, kmax, device, threads, [(f, sites)])
            if refusal:
                refusals.append('%s %s r%d : %s' % (tag, f, rep, refusal))
            else:
                result[f].append(full)
    return result, refusals


def campaign_sequence(probe, folder, names, processes, threads, delay, raw_dir, device):
    """Session qui enchaine les trames v12set, deux tours ; rend {trame: [[passes du second tour], ...]}, refus."""
    result, refusals = {n: [] for n in names}, []
    for rep in range(processes):
        order = names[rep % len(names):] + names[:rep % len(names)]
        argv = [probe] + ['--trame=%s,%s,%s' % (os.path.join(folder, n + '.u32le'),
                                               os.path.join(folder, n + '.ids.u32le'), n[-23:]) for n in order]
        argv += ['--k=5', '--threads=%d' % threads, '--passes=%d' % (2 * len(order)), '--digest'] + MODE['flags']
        if device:
            argv.append('--device')
        code, out, _err = run(argv, delay, os.path.join(raw_dir, 'v12set_r%d.jsonl' % rep))
        frames = [(n[-23:], frame_sites(os.path.join(folder, n + '.u32le'))) for n in order]
        full, refusal = parse_process(code, out, 2 * len(order), 5, device, threads, frames)
        if refusal:
            refusals.append('v12set r%d : %s' % (rep, refusal))
            continue
        for i, n in enumerate(order):
            first, second = full[i], full[len(order) + i]
            result[n].append([first, second])
    return result, refusals


def frame_stats(runs, warm_from):
    """runs : liste de listes de passes (une par processus) ; chaud = passes a partir de warm_from."""
    medians = [statistics.median(p['wall_ns'] for p in r[warm_from:]) for r in runs if len(r) > warm_from]
    warm = [p for r in runs for p in r[warm_from:]]
    if not medians:
        return None
    return dict(mediane_ns=statistics.median(p['wall_ns'] for p in warm), max_medianes_ns=max(medians),
                max_ns=max(p['wall_ns'] for p in warm), premiere_ns=statistics.median(r[0]['wall_ns'] for r in runs),
                etapes_ns={s: statistics.median(p['etapes_ns'][s] for p in warm) for s in warm[0]['etapes_ns']},
                c_ns={k: statistics.median(p['c_ns'][k] for p in warm) for k in warm[0]['c_ns']},
                cpu_ns=statistics.median(p['cpu_ns'] for p in warm) if all(type(p['cpu_ns']) is int for p in warm)
                else None,
                pic_octets=max(p['pic_octets'] for p in warm), sites=warm[0]['sites'], valeurs=len(warm))


def digests(groups, refusals, label):
    """Une empreinte par trame sur toutes les passes et tous les processus de tous les groupes donnes."""
    out = {}
    for frame in set(f for g in groups for f in g):
        seen = set(p['full_sha256'] for g in groups for r in g.get(frame, []) for p in r)
        if len(seen) != 1:
            refusals.append('%s %s : %d empreintes FUL1 differentes' % (label, frame, len(seen)))
        else:
            out[frame] = seen.pop()
    return out


def contract(stats):
    if not stats or any(v is None for v in stats.values()):
        return None
    values = [v['max_medianes_ns'] for v in stats.values()]
    medians = [v['mediane_ns'] for v in stats.values()]
    return dict(mediane_ns=statistics.median(medians), maximum_ns=max(values), trames=len(stats),
                tenu=statistics.median(medians) <= BUDGET_NS and max(values) <= BUDGET_NS)


def table(title, stats):
    """Tableau d'un bras ; colonnes d'etages selon le schema joue (recouvert : G jusqu'au dernier calcul de G, puis la
    queue ; sequentiel : G, puis T, M, V, R)."""
    tail = ('queue',) if MODE['schema'] == 'recouvert' else ('T', 'M', 'V', 'R')
    head = ('| trame | sites | chaud mediane (ms) | max des medianes par processus | max | 1re passe | P | C | '
            'transferts | G | ' + ' | '.join(tail) + ' | pic (Mo) |')
    columns = 11 + len(tail)
    lines = ['## ' + title, '', head, '| --- ' + '| ---: ' * columns + '|']
    for frame, v in sorted(stats.items()):
        if v is None:
            lines.append('| %s |' % frame + ' - |' * columns)
            continue
        e = v['etapes_ns']
        rest = (e['TMVR'],) if MODE['schema'] == 'recouvert' else (e['T'], e['M'], e['V'], e['R'])
        values = (v['mediane_ns'], v['max_medianes_ns'], v['max_ns'], v['premiere_ns'], e['P'], e['C'],
                  v['c_ns'].get('transferts', 0), e['G']) + rest
        lines.append('| %s | %d | ' % (frame, v['sites']) + ' | '.join('%.1f' % (x / 1e6) for x in values) +
                     ' | %.0f |' % (v['pic_octets'] / 1e6))
    return lines + ['']


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ('--src', '--travail', '--donnees', '--sortie', '--archive-v12set'):
        parser.add_argument(name, required=True)
    for name, default in (('--fils', 48), ('--processus', 5), ('--passes', 10), ('--jobs', 44), ('--delai', 900)):
        parser.add_argument(name, type=int, default=default)
    parser.add_argument('--essai', action='store_true')
    parser.add_argument('--sequentiel', action='store_true')
    parser.add_argument('--sonde')
    try:
        args = parser.parse_args(argv[1:])
    except SystemExit:
        return 2
    if args.essai != (args.sonde is not None) or args.fils < 1 or \
            (not args.essai and (args.processus < 5 or args.passes < 10)) or args.processus < 1 or args.passes < 2:
        return 2
    device = not args.essai
    MODE.update(dict(schema='sequentiel', flags=['--sequentiel']) if args.sequentiel else
                dict(schema='recouvert', flags=[]))
    raw_dir = os.path.join(args.sortie, 'brut')
    os.makedirs(raw_dir, exist_ok=True)
    nvcc = shutil.which('nvcc') or ('/usr/local/cuda/bin/nvcc' if os.path.isfile('/usr/local/cuda/bin/nvcc') else None)
    report = dict(mesure='MES-FULL', budget_ns=BUDGET_NS, options=vars(args), refus=[])
    report['environnement_avant'] = environment(nvcc)
    names = unpack(args.archive_v12set, os.path.join(args.travail, 'v12set'))
    if names is None or any(not os.path.isfile(os.path.join(args.donnees, 'lidar_%s.u32le' % f)) for f in FRAMES):
        return 2
    if args.essai:
        names = names[:2]
        probe = args.sonde if os.path.isfile(args.sonde) else None
    else:
        probe = build(args.src, os.path.join(args.travail, 'b21cuda'), nvcc, args.jobs) if nvcc else None
        log = os.path.join(args.travail, 'b21cuda', 'construction.log')
        if os.path.isfile(log):
            shutil.copyfile(log, os.path.join(args.sortie, 'construction.log'))
    if probe is None:
        return 3
    report['provenance'] = dict(sonde_sha256=sha256_file(probe), pilote_sha256=sha256_file(os.path.abspath(__file__)),
                                lecteur_sha256=sha256_file(lf.__file__),
                                cmake=None if args.essai else cmake_extract(os.path.join(args.travail, 'b21cuda')))
    t0 = time.time()
    small = (min(3, args.processus), min(5, args.passes))
    a, ra = campaign_frames(probe, args.donnees, FRAMES, 5, args.processus, args.passes, args.fils, device, args.delai,
                            raw_dir, 'k5')
    b, rb = campaign_frames(probe, args.donnees, FRAMES, 10, small[0], small[1], args.fils, device, args.delai, raw_dir,
                            'k10')
    c, rc = campaign_frames(probe, args.donnees, FRAMES, 5, small[0], small[1], args.fils, False, args.delai, raw_dir,
                            'cpu')
    d, rd = campaign_sequence(probe, os.path.join(args.travail, 'v12set'), names, args.processus, args.fils,
                              2 * args.delai, raw_dir, device)
    report['refus'] = ra + rb + rc + rd
    report['environnement_apres'] = environment(nvcc)
    if not args.essai:
        for when in ('avant', 'apres'):
            env = report['environnement_' + when]
            if env.get('gpu_apps') != '' or any(env.get(k) is None for k in ('nvcc', 'cmake', 'gpu')):
                report['refus'].append('environnement %s incomplet ou GPU non connu vide' % when)
    report['empreintes'] = dict(k5=digests([a, c], report['refus'], 'k5 appareil = cpu'),
                                k10=digests([b], report['refus'], 'k10'),
                                v12set=digests([d], report['refus'], 'v12set'))
    stats_a = {f: frame_stats(a[f], 1) for f in FRAMES}
    stats_b = {f: frame_stats(b[f], 1) for f in FRAMES}
    stats_c = {f: frame_stats(c[f], 1) for f in FRAMES}
    stats_d = {n: frame_stats(d[n], 1) for n in names}
    report['statistiques'] = dict(k5_appareil=stats_a, k10_appareil=stats_b, k5_cpu=stats_c, v12set_k5_appareil=stats_d)
    report['contrat'] = dict(ng00_02=contract(stats_a), v12set=contract(stats_d))
    if args.essai:
        report['verdict'] = 'essai'
    elif report['refus'] or report['contrat']['ng00_02'] is None or report['contrat']['v12set'] is None:
        report['verdict'] = 'refuse'
    else:
        report['verdict'] = 'tenu' if report['contrat']['ng00_02']['tenu'] and report['contrat']['v12set']['tenu'] \
            else 'non tenu'
    report['duree_s'] = round(time.time() - t0, 1)
    with open(os.path.join(args.sortie, 'rapport_full.json'), 'w', encoding='utf-8') as out:
        json.dump(report, out, indent=1, sort_keys=True)
    lines = ['# MES-FULL : tour FULL v12 en Session residente sur G4', '', 'Verdict du contrat (100 ms, K5) : **%s**.'
             % report['verdict'], '']
    for key, label in (('ng00_02', 'ng00-02'), ('v12set', '37 trames v12set')):
        ct = report['contrat'][key]
        if ct:
            lines.append('- %s : mediane %.1f ms, maximum %.1f ms sur %d trames.' % (label, ct['mediane_ns'] / 1e6,
                                                                                  ct['maximum_ns'] / 1e6, ct['trames']))
    lines += [''] + table('K5, voie appareil (ng00-02)', stats_a) + table('K10, voie appareil', stats_b)
    lines += table('K5, voie CPU (bras identifie)', stats_c)
    lines += table('K5, voie appareil, Session sur v12set (second tour)', stats_d)
    lines += ['Refus : %d.' % len(report['refus'])] + ['- ' + r for r in report['refus']]
    with open(os.path.join(args.sortie, 'tableaux_full.md'), 'w', encoding='utf-8') as out:
        out.write('\n'.join(lines) + '\n')
    print('mes_full verdict=%s refus=%d' % (report['verdict'], len(report['refus'])))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
