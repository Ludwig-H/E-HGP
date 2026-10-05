#!/usr/bin/env python3
"""Mesure appariee de la regle de L2 (docs/SORTIES.md, paragraphe 11), preparee pour la session G4 de la tranche S7.

    python3 sorties_g4.py --cli <mhgp11> [--data-dir <dossier>] [--frames=ng00,ng01,ng02] [--k=5] [--fils=1,48]
            [--prises=3] [--k10=ng00] [--work=<dossier>] [--out=<fichier.json>] [--commit=<sha>]

Memes entree, profil et K pour les deux sorties : --sortie=full (forets 1 a K de build_full, masque 16379) contre
--sortie=supports. Jusqu'a L2b, supports construisait l'arbre d'ordre K seul par build_order, au masque 7035 = 16379
sans verticales paralleles 128, reemploi vertical 1024 ni ordres concurrents 8192, que build_order refuse. La regle a
decide livrer_L2b : depuis, supports tire l'ordre K de FULL au masque 16379 (build_order_full, journal des graines sur
l'ordre K), et la mesure, refaite sur G4, decrit le cout de cette voie. Pour chaque trame
(lidar_<trame>.u32le et lidar_<trame>.ids.u32le du dossier de donnees, variable MHGP11_DATA_DIR par defaut) et chaque
W de --fils : une passe a froid (premier appel de la configuration, entree fraichement ouverte ; ce n'est pas un cache
vide, faute de privilege pour le vider), puis --prises passes a chaud, en alternant full et supports. Chaque appel
publie son dossier dans --work, qui est retire apres la lecture de ses empreintes.

Releve par appel, depuis la ligne d'etat (jamais depuis le manifeste, qui n'a ni temps ni fils) : etages cloud, index,
domain, tree, attach, output, write et total (ns), pics par etage, comptes ; sha256 du fichier de donnees et du
manifeste. Controles : code 0 et ligne de succes ; pour une meme (trame, sortie), fichier et manifeste identiques a
l'octet entre toutes les prises et tous les W (sinon la mesure est INVALIDE : defaut a corriger avant toute decision,
paragraphe 11) ; tree_k_sha256 egal entre full et supports.

Statistique et regle (paragraphe 11) : par trame et par sortie, mediane des prises a chaud de l'etage tree a W48 ;
build_order reste la voie par defaut si, sur au moins deux des trois trames ng00, ng01, ng02, la mediane de supports
ne depasse pas 1,1 fois celle de full. La regle n'est evaluee que si W48 a ete mesure sur les trois trames, avec au
moins trois prises et des sorties identiques ; sinon "decision" vaut "non_evaluee" avec la raison. W1 et K10 sont
descriptifs. Les etages attach, output et write de supports sont publies a part, comme ceux de full.

Sortie : un document JSON (schema ehgp.v11.sorties_g4.v1) sur --out ou sur la sortie standard, pret pour un recu :
provenance (commit declare, sha256 du binaire, machine, heure UTC de debut et de fin), configurations, appels,
medianes, ratios et decision. Aucun temps n'est une conclusion hors de G4 : un essai local (W1, W4) sert a valider le
script, jamais la regle. Codes : 0 mesure valide (decision evaluee ou non) ; 1 mesure invalide (sorties differentes
ou appel en echec) ; 2 refus d'usage. Python 3.10 nu, bibliotheque standard seule, aucun assert.
"""
import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import time

SCHEMA = 'ehgp.v11.sorties_g4.v1'
STAGES = ('cloud', 'index', 'domain', 'tree', 'attach', 'output', 'write', 'total')
OUTPUTS = (('full', 'full.mhgp11ful1', 16379), ('supports', 'supports.mhgp11sp', 16379))  # supports : L2b
RULE_FRAMES = ('ng00', 'ng01', 'ng02')
RULE_WORKERS = 48
RULE_RATIO = (11, 10)  # T_supports <= 1,1 T_full, en entiers


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def utc_now():
    return time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())


def median(values):
    ordered = sorted(values)
    middle = len(ordered) // 2
    return ordered[middle] if len(ordered) % 2 else (ordered[middle - 1] + ordered[middle]) // 2


def run_one(args, frame, output, k, workers, phase, prise):
    """Un appel du CLI ; rend l'enregistrement de l'appel (ok, etages, pics, comptes, empreintes)."""
    points = os.path.join(args.data_dir, 'lidar_%s.u32le' % frame)
    ids = os.path.join(args.data_dir, 'lidar_%s.ids.u32le' % frame)
    folder = tempfile.mkdtemp(prefix='g4', dir=args.work)
    directory = os.path.join(folder, 'D')
    name = dict((o, f) for o, f, _ in OUTPUTS)[output]
    argv = [args.cli, '--sortie=' + output, '--points=' + points, '--ids=' + ids, '--dossier=' + directory,
            '--k=%d' % k, '--fils=%d' % workers]
    record = dict(frame=frame, output=output, k=k, workers=workers, phase=phase, prise=prise, ok=False)
    started = time.monotonic_ns()
    try:
        done = subprocess.run(argv, capture_output=True, text=True, timeout=args.timeout)
    except subprocess.TimeoutExpired:
        record['error'] = 'delai depasse'
        shutil.rmtree(folder, ignore_errors=True)
        return record
    record['wall_ns'] = time.monotonic_ns() - started
    lines = done.stdout.splitlines()
    try:
        line = json.loads(lines[0]) if len(lines) == 1 else {}
    except ValueError:
        line = {}
    if done.returncode != 0 or line.get('status') != 'ok' or line.get('workers') != workers:
        record['error'] = 'code %d, ligne %r, erreur %r' % (done.returncode, done.stdout[-300:], done.stderr[-300:])
        shutil.rmtree(folder, ignore_errors=True)
        return record
    try:
        with open(os.path.join(directory, 'manifeste.json'), 'rb') as handle:
            raw = handle.read()
        manifest = json.loads(raw)
        record.update(file_sha256=sha256_file(os.path.join(directory, name)),
                      manifest_sha256=hashlib.sha256(raw).hexdigest(),
                      file_bytes=os.path.getsize(os.path.join(directory, name)),
                      tree_k_sha256=manifest['tree_k_sha256'])
    except (OSError, ValueError, KeyError) as error:
        record['error'] = 'dossier illisible : %s' % error
        shutil.rmtree(folder, ignore_errors=True)
        return record
    shutil.rmtree(folder, ignore_errors=True)
    record.update(ok=True, sites=line.get('sites'), stages_ns=dict((s, line['stages_ns'][s]) for s in STAGES),
                  peaks_bytes=line.get('peaks_bytes'), counts=line.get('counts'))
    return record


def measure(args, frame, k, workers_list, prises, calls):
    """Une trame a l'ordre k : pour chaque W, une passe a froid puis `prises` passes a chaud, full et supports
    alternes."""
    for workers in workers_list:
        for output, _name, _mask in OUTPUTS:
            calls.append(run_one(args, frame, output, k, workers, 'froid', 0))
        for prise in range(1, prises + 1):
            for output, _name, _mask in OUTPUTS:
                calls.append(run_one(args, frame, output, k, workers, 'chaud', prise))
            sys.stderr.write('sorties_g4 : %s K%d W%d prise %d/%d (%s)\n' % (frame, k, workers, prise, prises,
                                                                           utc_now()))
            sys.stderr.flush()


def identity(calls):
    """Pour chaque (trame, K, sortie) : une seule empreinte de fichier et de manifeste ; tree_k_sha256 commun aux deux
    sorties. Rend la liste des defauts."""
    defects, seen, trees = [], {}, {}
    for c in calls:
        if not c['ok']:
            defects.append('%s %s K%d W%d %s %d : %s' % (c['frame'], c['output'], c['k'], c['workers'], c['phase'],
                                                       c['prise'], c.get('error')))
            continue
        key = (c['frame'], c['k'], c['output'])
        pair = (c['file_sha256'], c['manifest_sha256'])
        if seen.setdefault(key, pair) != pair:
            defects.append('%s %s K%d : sorties differentes entre prises ou W (W%d %s %d)'
                           % (c['frame'], c['output'], c['k'], c['workers'], c['phase'], c['prise']))
        tree = trees.setdefault((c['frame'], c['k']), c['tree_k_sha256'])
        if tree != c['tree_k_sha256']:
            defects.append('%s K%d : tree_k_sha256 different entre full et supports' % (c['frame'], c['k']))
    return defects


def summarize(calls, k):
    """Medianes a chaud par (trame, sortie, W) de chaque etage, a l'ordre k ; ratios supports / full de tree."""
    table = {}
    for c in calls:
        if c['ok'] and c['phase'] == 'chaud' and c['k'] == k:
            table.setdefault((c['frame'], c['workers'], c['output']), []).append(c)
    medians = []
    for (frame, workers, output), group in sorted(table.items()):
        medians.append(dict(frame=frame, workers=workers, output=output, prises=len(group),
                            median_ns=dict((s, median([c['stages_ns'][s] for c in group])) for s in STAGES),
                            peak_bytes_max=dict((s, max(c['peaks_bytes'][s] for c in group))
                                                for s in group[0]['peaks_bytes'])))
    ratios = []
    index = dict(((m['frame'], m['workers'], m['output']), m) for m in medians)
    for (frame, workers, output), m in sorted(index.items()):
        if output != 'supports' or (frame, workers, 'full') not in index:
            continue
        full = index[(frame, workers, 'full')]['median_ns']['tree']
        sup = m['median_ns']['tree']
        ratios.append(dict(frame=frame, workers=workers, tree_full_ns=full, tree_supports_ns=sup,
                           ratio_permille=(1000 * sup + full // 2) // full if full else None,
                           within_rule=sup * RULE_RATIO[1] <= RULE_RATIO[0] * full))
    return medians, ratios


def decide(ratios, defects, prises, k):
    """Regle de L2, a W48 sur ng00, ng01, ng02 ; non evaluee hors de ses conditions."""
    if defects:
        return dict(decision='non_evaluee', reason='mesure invalide : %d defaut(s)' % len(defects))
    if k != 5:
        return dict(decision='non_evaluee', reason='la regle porte sur K = 5')
    at48 = dict((r['frame'], r) for r in ratios if r['workers'] == RULE_WORKERS)
    missing = [f for f in RULE_FRAMES if f not in at48]
    if missing:
        return dict(decision='non_evaluee', reason='W48 absent pour %s' % ','.join(missing))
    if prises < 3:
        return dict(decision='non_evaluee', reason='moins de trois prises (%d)' % prises)
    held = [f for f in RULE_FRAMES if at48[f]['within_rule']]
    keep = len(held) >= 2
    return dict(decision='build_order_par_defaut' if keep else 'livrer_L2b', frames_within_rule=held,
                rule='mediane tree(supports) <= 1,1 mediane tree(full) a W48 sur au moins deux des trois trames')


def main():
    parser = argparse.ArgumentParser(description='Mesure appariee FULL contre supports (L2b : 16379), regle de L2.')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--data-dir', default=os.environ.get('MHGP11_DATA_DIR', ''))
    parser.add_argument('--frames', default='ng00,ng01,ng02')
    parser.add_argument('--k', type=int, default=5)
    parser.add_argument('--fils', default='1,48')
    parser.add_argument('--prises', type=int, default=3)
    parser.add_argument('--k10', default='', help='trame mesuree une fois a K = 10 (descriptif), au plus grand W')
    parser.add_argument('--work', default='')
    parser.add_argument('--out', default='')
    parser.add_argument('--commit', default='')
    parser.add_argument('--timeout', type=int, default=3600)
    args = parser.parse_args()
    frames = [f for f in args.frames.split(',') if f]
    try:
        workers = [int(w) for w in args.fils.split(',')]
    except ValueError:
        workers = []
    if (not os.path.isfile(args.cli) or not os.path.isdir(args.data_dir) or not frames or not workers or
            args.prises < 1 or any(w < 1 or w > 256 for w in workers)):
        print('usage : --cli <mhgp11 existant>, --data-dir (ou MHGP11_DATA_DIR), --frames, --fils, --prises >= 1')
        return 2
    for frame in frames + ([args.k10] if args.k10 else []):
        for suffix in ('.u32le', '.ids.u32le'):
            if not os.path.isfile(os.path.join(args.data_dir, 'lidar_%s%s' % (frame, suffix))):
                print('usage : trame %s absente du dossier de donnees' % frame)
                return 2
    own_work = not args.work
    args.work = args.work or tempfile.mkdtemp(prefix='mhgp11-sorties-g4-')
    os.makedirs(args.work, exist_ok=True)
    begin = utc_now()
    calls = []
    try:
        for frame in frames:
            measure(args, frame, args.k, workers, args.prises, calls)
        if args.k10:
            measure(args, args.k10, 10, [max(workers)], 1, calls)
    finally:
        if own_work:
            shutil.rmtree(args.work, ignore_errors=True)
    defects = identity(calls)
    medians, ratios = summarize(calls, args.k)
    medians10, _ = summarize(calls, 10) if args.k10 else ([], [])
    document = dict(
        schema=SCHEMA, phase='exploration_v11_hors_registre', backend='cpu_reference', public_status='not_claimed',
        rule='docs/SORTIES.md, paragraphe 11', engine_masks=dict((o, m) for o, _f, m in OUTPUTS),
        provenance=dict(commit=args.commit or None, cli_sha256=sha256_file(args.cli), host=platform.node(),
                        machine=platform.machine(), cpu_count=os.cpu_count(), python=platform.python_version(),
                        begin_utc=begin, end_utc=utc_now()),
        configuration=dict(frames=frames, k=args.k, workers=workers, prises=args.prises, k10=args.k10 or None,
                           cold='premier appel de la configuration, cache non vide'),
        calls=calls, defects=defects, medians=medians, medians_k10=medians10, ratios=ratios,
        decision=decide(ratios, defects, args.prises, args.k))
    text = json.dumps(document, indent=1, sort_keys=False) + '\n'
    if args.out:
        with open(args.out, 'w') as handle:
            handle.write(text)
    else:
        sys.stdout.write(text)
    sys.stderr.write('sorties_g4 : appels=%d defauts=%d decision=%s\n'
                     % (len(calls), len(defects), document['decision']['decision']))
    return 1 if defects else 0


if __name__ == '__main__':
    sys.exit(main())
