#!/usr/bin/env python3
"""MES-B : la tour FULL de la v12 sur des scenes LiDAR reelles ENTIERES, regime (b) de la decision D7, sur G4.

Captations telles quelles (consigne de l'utilisateur du 8 octobre) : scenes entieres des paquets de bench/data
(dalles IGN, scans ETH3D, placettes FOR-instance, fenetres de trames consecutives de Boreas), sans decoupe ni
sous-echantillonnage ; seul ecart, declare : les retours d'une meme position au millimetre sont un seul site (variante
`.distinct` du paquet, decision D8).

Etapes : environnement (nvcc, cmake, nvidia-smi, GPU vide avant et apres, hote) ; construction Release au profil 21
avec CUDA de mhgp12_full_probe, journal de construction, empreinte SHA-256 de la sonde et extrait du CMakeCache ;
lecture du manifeste du paquet ; puis les cas, dans l'ordre donne, un processus neuf chacun :
  --cas NOM:K:VOIE:PASSES[,...]   VOIE = appareil | cpu ; NOM = un cas du manifeste du paquet
avec --digest (empreinte FUL1 hors du mur) jusqu'a --empreinte-max-sites sites (l'empreinte hache plusieurs Go par
million de sites sur un seul fil : au-dela, elle couterait des minutes par passe), budget de l'hote --budget-gio,
budget propre de l'appareil --budget-appareil-gio (voie appareil), --fils fils ; pic de memoire de l'appareil
echantillonne par nvidia-smi pendant le cas. Delai global --delai-global (secondes depuis le lancement du pilote) :
un cas dont la prevision depasse le temps restant n'est pas lance (« non joue : delai ») ; chaque cas a son propre
delai (trois fois sa prevision, au plus le temps restant). Prevision : 20 s, plus le debit par site et par passe du
dernier cas joue du meme (K, voie) d'au moins un million de sites et sans empreinte (sinon un debit par defaut
prudent), fois les sites et les passes, fois 1,3.
Un refus de la sonde (code 2 : memory_budget, index_overflow_u32, ...) est un RESULTAT publie (point de rupture) ;
une mort par signal, une expiration ou un invariant viole est un ECHEC du cas, publie ; une sortie hors schema ou un
appareil indisponible font manquer un controle.

Lecture stricte de chaque sortie par le lecteur partage microbancs/outils/lecteur_full.py (schema exact, entiers u64
non booleens, sequence open/full/liberation/sortie, etages inclus dans le mur, memoire par etage coherente avec
pic_octets, mur non nul) ; sites = compte du manifeste ; budget de l'appareil « separe » attendu sur la voie appareil.
Controles (refus d'ensemble si l'un manque) : environnement complet et GPU vide avant et apres (voie appareil) ; aucune
sortie illisible ; empreinte FUL1 identique sur toutes les passes d'un meme (scene, K) et entre les deux voies, pour
les cas qui la calculent.

Verdicts ecrits d'avance (objectifs du regime (b), MESURE.md, hypotheses du 7 octobre ; mur chaud = derniere passe
jouee si au moins deux, sinon la premiere, marquee froide) :
  B1 : K5, voie appareil : chaque scene lancee aboutit et tient au plus 2 s par million de sites ; un echec compte a
       toute taille, un refus seulement sous 10 millions de sites (au-dela, refus tolere par les objectifs, publie) ;
  B2 : K5, voie appareil : aucune scene de moins de 10 millions de sites refusee ni en echec ;
  B3 : K5, voie appareil : exposant au plus 1,1 sur chaque serie emboitee de captations entieres (--series) dont tous
       les membres sont joues : pente des moindres carres de log(mur) contre log(sites) ;
  B4 : K10 : chaque scene lancee aboutit et tient au plus 10 s par million de sites.
Chaque critere est « tenu », « non tenu » ou « non evalue » (aucune donnee). Verdict d'ensemble : « tenu » si B1 et B2
sont evalues et tous les criteres evalues tenus ; « non tenu » sinon ; « refuse » si un controle manque.

Usage : pilote_b.py --src SRC --travail DOSSIER --donnees DOSSIER --sortie DOSSIER --cas LISTE
                    [--series S1;S2 (membres separes par des virgules)] [--fils 48] [--budget-gio 160]
                    [--budget-appareil-gio 88] [--delai-global 2000] [--empreinte-max-sites 3000000] [--jobs 44]
                    [--essai --sonde BINAIRE]   essai local : sonde existante, voie CPU partout, pas d'environnement
                                                GPU ni de construction ; verdict marque « essai », jamais publie
Sorties : <sortie>/rapport_b.json, <sortie>/tableaux_b.md, <sortie>/brut/ (sorties de chaque cas),
<sortie>/construction.log. Codes : 0 rendu (quel que soit le verdict) ; 2 usage ou donnees ; 3 construction
impossible. Python 3.10 nu, aucun assert (tient sous -O).
"""
import argparse
import hashlib
import json
import math
import os
import platform
import re
import shutil
import signal
import subprocess
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'outils'))
import lecteur_full as lf  # noqa: E402  lecteur strict partage avec MES-FULL

GIB = 1 << 30
# Debits par defaut prudents (secondes par million de sites et par passe, hors du mur compris), avant toute mesure.
DEFAULT_RATE = {(5, 'appareil'): 15.0, (5, 'cpu'): 40.0, (10, 'appareil'): 80.0, (10, 'cpu'): 160.0}
CMAKE_KEYS = re.compile(r'^(CMAKE_BUILD_TYPE|CMAKE_CXX_COMPILER|CMAKE_CUDA_COMPILER|CMAKE_CUDA_ARCHITECTURES|'
                        r'CMAKE_CXX_FLAGS|CMAKE_CXX_FLAGS_RELEASE|CMAKE_CUDA_FLAGS|CMAKE_CUDA_FLAGS_RELEASE|'
                        r'MHGP12_[A-Z0-9_]+)(:[A-Z]+)?=')
BITS_EXPECTED = 21


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 22), b''):
            digest.update(block)
    return digest.hexdigest()


def run(argv, delay, raw_out=None, raw_err=None):
    """Lance argv dans son propre groupe ; rend (code, sortie, erreur, secondes) ; code 'expire' apres le delai."""
    t0 = time.monotonic()
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
    for path, data in ((raw_out, out), (raw_err, err)):
        if path is not None:
            with open(path, 'wb') as handle:
                handle.write(data)
    return code, out.decode('utf-8', 'replace'), err.decode('utf-8', 'replace'), time.monotonic() - t0


def environment(nvcc, with_gpu):
    env = {}
    commands = [('cmake', ['cmake', '--version'])]
    if with_gpu:
        commands += [('nvcc', [nvcc, '--version']),
                     ('gpu', ['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,memory.total',
                              '--format=csv,noheader']),
                     ('gpu_apps', ['nvidia-smi', '--query-compute-apps=pid', '--format=csv,noheader'])]
    for name, argv in commands:
        try:
            code, out, _err, _s = run(argv, 60)
            env[name] = out.strip() if code == 0 else None
        except OSError:
            env[name] = None
    env['noyau'] = platform.release()
    env['fils_hote'] = os.cpu_count()
    try:
        with open('/proc/meminfo', encoding='ascii') as handle:
            env['memoire_hote'] = next((line.split(':', 1)[1].strip() for line in handle
                                        if line.startswith('MemTotal:')), None)
    except OSError:
        env['memoire_hote'] = None
    return env


def build(src, folder, nvcc, jobs, log_path):
    os.makedirs(folder, exist_ok=True)
    configure = ['cmake', '-S', os.path.join(src, 'morsehgp3D_v12'), '-B', folder, '-DCMAKE_BUILD_TYPE=Release',
                 '-DMHGP12_COORD_BITS=%d' % BITS_EXPECTED, '-DMHGP12_ENABLE_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc]
    for argv in (configure, ['cmake', '--build', folder, '-j', str(jobs), '--target', 'mhgp12_full_probe']):
        code, out, err, _s = run(argv, 3600)
        with open(log_path, 'a', encoding='utf-8') as log:
            log.write('$ %s\n%s%s' % (' '.join(argv), out, err))
        if code != 0:
            return None
    probe = os.path.join(folder, 'mhgp12_full_probe')
    return probe if os.path.isfile(probe) else None


def cmake_extract(folder):
    lines = []
    try:
        with open(os.path.join(folder, 'CMakeCache.txt'), encoding='utf-8', errors='replace') as handle:
            lines = [line.rstrip('\n') for line in handle if CMAKE_KEYS.match(line)]
    except OSError:
        return None
    return lines


def load_manifest(folder):
    """Rend {nom: (xyz, ids, sites)} des cas du paquet ; None si le manifeste manque ou est mal forme."""
    try:
        with open(os.path.join(folder, 'bundle_manifest.json'), encoding='utf-8') as handle:
            manifest = json.load(handle)
        cases = {}
        for case in manifest['cases']:
            entry = case['distinct'] if case.get('bundled') == 'distinct' else case
            xyz, ids = entry['coordinates'], entry['point_ids']
            sites = entry.get('count', case.get('count'))
            if type(sites) is not int or sites <= 0 or '/' in xyz or '/' in ids:
                return None
            cases[case['name']] = (os.path.join(folder, xyz), os.path.join(folder, ids), sites)
    except (OSError, ValueError, KeyError, TypeError):
        return None
    return cases


def parse_cases(text):
    """'NOM:K:VOIE:PASSES,...' -> liste de dict ; None si une entree est mal formee ou repetee."""
    cases, seen = [], set()
    for item in text.split(','):
        parts = item.split(':')
        if len(parts) != 4 or parts[2] not in lf.VOIES or not parts[1].isdigit() or not parts[3].isdigit():
            return None
        k, passes = int(parts[1]), int(parts[3])
        if not 1 <= k <= 12 or not 1 <= passes <= 20 or (parts[0], k, parts[2]) in seen:
            return None
        seen.add((parts[0], k, parts[2]))
        cases.append(dict(nom=parts[0], k=k, voie=parts[2], passes=passes))
    return cases or None


def label_of(name, used):
    """Etiquette de trame de la sonde (au plus 23 octets), unique dans la campagne : la fin du nom, prefixee d'un
    rang si elle est deja prise."""
    label, i = name[-23:], 1
    while label in used:
        prefix = '%d_' % i
        label = prefix + name[-(23 - len(prefix)):]
        i += 1
    used.add(label)
    return label


def expected(case, label, sites):
    """Ce que le lecteur partage doit trouver dans la sortie d'un cas : une trame, budget de l'appareil separe."""
    return dict(voie=case['voie'], k=case['k'], fils=case['fils'], passes=case['passes'], empreinte=case['empreinte'],
                trames=[(label, sites)], budget_appareil='separe', bits=BITS_EXPECTED)


class GpuSampler:
    """Pic de memory.used (Mio) pendant un cas, par nvidia-smi -lms ; None sans nvidia-smi."""

    def __init__(self, enabled):
        self.proc = None
        if enabled:
            try:
                self.proc = subprocess.Popen(['nvidia-smi', '--query-gpu=memory.used', '--format=csv,noheader,nounits',
                                              '-lms', '250'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                             start_new_session=True)
            except OSError:
                self.proc = None

    def stop(self):
        if self.proc is None:
            return None
        try:
            os.killpg(self.proc.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        out, _ = self.proc.communicate()
        values = [int(v) for v in out.decode('ascii', 'replace').split() if v.isdigit()]
        return max(values) if values else None


def warm_pass(passes):
    """Passe chaude : la derniere jouee si au moins deux, sinon la premiere (froide)."""
    if not passes:
        return None, None
    return (passes[-1], 'chaude') if len(passes) >= 2 else (passes[0], 'froide')


def slope(points):
    """Pente des moindres carres de log(y) contre log(x) ; None sous deux points distincts ou si une valeur n'est pas
    strictement positive (garde : la lecture refuse deja les murs nuls)."""
    if any(x <= 0 or y <= 0 for x, y in points):
        return None
    xs = [math.log(x) for x, _ in points]
    ys = [math.log(y) for _, y in points]
    if len(set(xs)) < 2:
        return None
    mx, my = sum(xs) / len(xs), sum(ys) / len(ys)
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)


def verdicts(results, series):
    """Criteres B1 a B4 sur les resultats ; rend {critere: dict(etat, detail)}."""
    def played(k, voie):
        return [r for r in results if r['k'] == k and r['voie'] == voie and r['etat'] != 'non_joue']
    out = {}
    k5 = played(5, 'appareil')
    rows = [(r['nom'], warm_pass(r['passes'])[0]['wall_ns'] / 1e9 / (r['sites'] / 1e6))
            for r in k5 if r['etat'] == 'ok']
    bad = ['%s : %.3f s par million' % (n, v) for n, v in rows if v > 2.0]
    bad += ['%s : %s (%s)' % (r['nom'], r['etat'], r['raison']) for r in k5
            if r['etat'] == 'echec' or (r['etat'] == 'refus' and r['sites'] < 10_000_000)]
    tolerated = ['%s : refus tolere au-dela de 10 millions de sites (%s)' % (r['nom'], r['raison']) for r in k5
                 if r['etat'] == 'refus' and r['sites'] >= 10_000_000]
    out['B1'] = dict(etat='non evalue' if not k5 else 'non tenu' if bad else 'tenu',
                     detail=(bad or (['%d scenes, maximum %.3f s par million' % (len(rows), max(v for _, v in rows))]
                                     if rows else [])) + tolerated)
    small = [r for r in k5 if r['sites'] < 10_000_000]
    failed = [r for r in small if r['etat'] != 'ok']
    out['B2'] = dict(etat='non evalue' if not small else 'non tenu' if failed else 'tenu',
                     detail=['%s : %s (%s)' % (r['nom'], r['etat'], r['raison']) for r in failed] or
                     ['%d scenes jouees sous 10 millions de sites' % len(small)] if small else [])
    fits, bad3 = [], []
    for members in series:
        found = {r['nom']: r for r in k5 if r['nom'] in members and r['passes'] and r['etat'] == 'ok'}
        if len(found) != len(members):
            continue
        s = slope([(found[m]['sites'], warm_pass(found[m]['passes'])[0]['wall_ns']) for m in members])
        if s is None:
            continue
        fits.append('%s : pente %.3f' % (' < '.join(members), s))
        if s > 1.1:
            bad3.append(fits[-1])
    out['B3'] = dict(etat='non evalue' if not fits else 'non tenu' if bad3 else 'tenu', detail=fits)
    k10 = played(10, 'appareil') + played(10, 'cpu')
    rows10 = [(r['nom'], r['voie'], warm_pass(r['passes'])[0]['wall_ns'] / 1e9 / (r['sites'] / 1e6))
              for r in k10 if r['etat'] == 'ok']
    bad4 = ['%s (%s) : %.3f s par million' % row for row in rows10 if row[2] > 10.0]
    bad4 += ['%s (%s) : %s (%s)' % (r['nom'], r['voie'], r['etat'], r['raison']) for r in k10 if r['etat'] != 'ok']
    out['B4'] = dict(etat='non evalue' if not k10 else 'non tenu' if bad4 else 'tenu',
                     detail=bad4 or ['%s (%s) : %.3f s par million' % row for row in rows10])
    return out


def digest_controls(results):
    """Empreintes FUL1 : identiques sur les passes d'un (scene, K) et entre les voies ; rend (table, ecarts)."""
    table, errors = {}, []
    for r in results:
        for p in r['passes']:
            if 'full_sha256' in p:
                table.setdefault((r['nom'], r['k']), {}).setdefault(r['voie'], set()).add(p['full_sha256'])
    for (name, k), by_voie in sorted(table.items()):
        everything = set().union(*by_voie.values())
        if len(everything) != 1:
            errors.append('%s K%d : %d empreintes (%s)' % (name, k, len(everything), sorted(by_voie)))
    return {'%s:K%d' % key: sorted(set().union(*v.values())) for key, v in table.items()}, errors


def gib(value):
    return '—' if value is None else '%.1f' % (value / GIB)


def tables(report):
    lines = ['# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12', '',
             'Verdict d\'ensemble : **%s**.' % report['verdict'], '']
    for name, v in report['criteres'].items():
        lines.append('- %s : %s%s' % (name, v['etat'], (' — ' + ' ; '.join(v['detail'])) if v['detail'] else ''))
    lines += ['', '| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | '
              'CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | '
              'Ko par site | FUL1 |',
              '| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: '
              '| --- |']
    for r in report['cas']:
        first = r['passes'][0] if r['passes'] else None
        warm, kind = warm_pass(r['passes'])
        smi = r.get('pic_nvidia_smi_mio')
        lines.append('| `%s` | %s | %d | %s | %s%s | %d | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |' % (
            r['nom'], format(r['sites'], ',').replace(',', ' '), r['k'], r['voie'], r['etat'],
            (' (%s)' % r['raison']) if r['raison'] else '', len(r['passes']),
            '%.2f' % (first['wall_ns'] / 1e9) if first else '—',
            ('%.2f' % (warm['wall_ns'] / 1e9) + ('' if kind == 'chaude' else ' (froide)')) if warm else '—',
            '%.3f' % (warm['wall_ns'] / 1e9 / (r['sites'] / 1e6)) if warm else '—',
            '%.1f' % (warm['cpu_ns'] / 1e9) if warm else '—',
            gib(warm['pic_octets']) if warm else '—', gib(max(p['rss_max_octets'] for p in r['passes']))
            if r['passes'] else '—', gib(warm['appareil_octets']) if warm and r['voie'] == 'appareil' else '—',
            '%.1f' % (smi / 1024) if smi is not None else '—',
            '%.1f' % (warm['pic_octets'] / r['sites'] / 1e3) if warm else '—',
            ('`%s`' % warm['full_sha256'][:12]) if warm and 'full_sha256' in warm else '—'))
    lines += ['', 'Memoire du budget de l\'hote par etage, passe chaude (Ko par site : en usage a la fin de '
              'l\'etage / pic pendant l\'etage) :', '',
              '| Scene | K | voie | ' + ' | '.join(lf.MEM_STAGES) + ' |',
              '| --- | ---: | --- |' + ' ---: |' * len(lf.MEM_STAGES)]
    for r in report['cas']:
        warm, _kind = warm_pass(r['passes'])
        if warm is None:
            continue
        mem = warm['memoire_octets']
        lines.append('| `%s` | %d | %s | %s |' % (r['nom'], r['k'], r['voie'], ' | '.join(
            '%.2f / %.2f' % (mem[k][0] / r['sites'] / 1e3, mem[k][1] / r['sites'] / 1e3) for k in lf.MEM_STAGES)))
    lines += ['', 'Etages de la passe chaude (secondes) :', '',
              '| Scene | K | voie | P | C | dont transferts | G | T | M | V | R | validation | empreinte '
              '| liberation |',
              '| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |']
    for r in report['cas']:
        warm, _kind = warm_pass(r['passes'])
        if warm is None:
            continue
        st = warm['etapes_ns']
        lines.append('| `%s` | %d | %s | %s |' % (r['nom'], r['k'], r['voie'], ' | '.join('%.2f' % (v / 1e9) for v in (
            st['P'], st['C'], warm['c_ns']['transferts'], st['G'], st['T'], st['M'], st['V'], st['R'],
            warm['hors_mur_ns']['validation'], warm['hors_mur_ns']['empreinte'], warm['liberation_ns']))))
    return '\n'.join(lines) + '\n'


def main(argv):
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src')
    parser.add_argument('--travail')
    parser.add_argument('--donnees', required=True)
    parser.add_argument('--sortie', required=True)
    parser.add_argument('--cas', required=True)
    parser.add_argument('--series', default='')
    parser.add_argument('--fils', type=int, default=48)
    parser.add_argument('--budget-gio', type=float, default=160.0)
    parser.add_argument('--budget-appareil-gio', type=float, default=88.0)
    parser.add_argument('--delai-global', type=float, default=2000.0)
    parser.add_argument('--empreinte-max-sites', type=int, default=3_000_000)
    parser.add_argument('--jobs', type=int, default=44)
    parser.add_argument('--nvcc', default=None)
    parser.add_argument('--essai', action='store_true')
    parser.add_argument('--sonde')
    args = parser.parse_args(argv[1:])
    t_start = time.monotonic()
    cases = parse_cases(args.cas)
    series = [s.split(',') for s in args.series.split(';') if s]
    manifest = load_manifest(args.donnees)
    if cases is None or manifest is None or any(c['nom'] not in manifest for c in cases) or \
            any(m not in manifest for s in series for m in s) or args.fils < 1 or \
            (args.essai != bool(args.sonde)) or (not args.essai and not (args.src and args.travail)):
        print('pilote_b : usage, cas ou manifeste refuses', file=sys.stderr)
        return 2
    out_dir = args.sortie
    raw_dir = os.path.join(out_dir, 'brut')
    os.makedirs(raw_dir, exist_ok=True)
    for c in cases:
        c['fils'] = args.fils
        c['empreinte'] = manifest[c['nom']][2] <= args.empreinte_max_sites
        if args.essai:
            c['voie'] = 'cpu'
    with_gpu = not args.essai
    if args.nvcc is None:
        args.nvcc = shutil.which('nvcc') or '/usr/local/cuda/bin/nvcc'
    env_before = environment(args.nvcc, with_gpu)
    provenance = {}
    if args.essai:
        probe = args.sonde
    else:
        log_path = os.path.join(out_dir, 'construction.log')
        probe = build(args.src, args.travail, args.nvcc, args.jobs, log_path)
        if probe is None:
            print('pilote_b : construction impossible (voir construction.log)', file=sys.stderr)
            return 3
        provenance['cmake'] = cmake_extract(args.travail)
    provenance['sonde_sha256'] = sha256_file(probe)
    provenance['pilote_sha256'] = sha256_file(os.path.abspath(__file__))
    provenance['manifeste_sha256'] = sha256_file(os.path.join(args.donnees, 'bundle_manifest.json'))
    used, results, rates = set(), [], {}
    budget = int(args.budget_gio * GIB)
    device_budget = int(args.budget_appareil_gio * GIB)
    for c in cases:
        xyz, ids, sites = manifest[c['nom']]
        label = label_of(c['nom'], used)
        rate = rates.get((c['k'], c['voie']), DEFAULT_RATE.get((c['k'], c['voie']), 200.0))
        forecast = 20.0 + rate * sites / 1e6 * c['passes'] * 1.3
        remaining = args.delai_global - (time.monotonic() - t_start)
        entry = dict(c, sites=sites, etiquette=label, prevision_s=round(forecast, 1), etat='non_joue',
                     raison='', passes=[])
        if forecast > remaining:
            entry['raison'] = 'delai : prevision %.0f s, reste %.0f s' % (forecast, remaining)
            results.append(entry)
            continue
        cmd = [probe, '--trame=%s,%s,%s' % (xyz, ids, label), '--k=%d' % c['k'], '--threads=%d' % args.fils,
               '--passes=%d' % c['passes'], '--budget=%d' % budget] + (['--digest'] if c['empreinte'] else [])
        if c['voie'] == 'appareil':
            cmd += ['--device', '--budget-appareil=%d' % device_budget]
        tag = '%s_k%d_%s' % (c['nom'], c['k'], c['voie'])
        sampler = GpuSampler(with_gpu and c['voie'] == 'appareil')
        code, out, _err, seconds = run(cmd, min(remaining, max(60.0, 3 * forecast)),
                                       os.path.join(raw_dir, tag + '.jsonl'), os.path.join(raw_dir, tag + '.err'))
        smi = sampler.stop()
        parsed = lf.parse_output(code, out, expected(c, label, sites))
        entry.update(parsed, code=code, secondes=round(seconds, 1), pic_nvidia_smi_mio=smi)
        results.append(entry)
        if parsed['passes'] and sites >= 1_000_000 and not c['empreinte']:
            rates[(c['k'], c['voie'])] = seconds / len(parsed['passes']) / (sites / 1e6)
    env_after = environment(args.nvcc, with_gpu)
    digests, digest_errors = digest_controls(results)
    controls = []
    if with_gpu:
        for when, env in (('avant', env_before), ('apres', env_after)):
            if any(env.get(k) is None for k in ('cmake', 'nvcc', 'gpu')) or env.get('gpu_apps') != '':
                controls.append('environnement %s incomplet ou GPU occupe' % when)
    controls += ['sortie de %s K%d %s : %s' % (r['nom'], r['k'], r['voie'], r['raison'])
                 for r in results if r['etat'] == 'illisible']
    controls += digest_errors
    criteria = verdicts(results, series)
    if args.essai:
        verdict = 'essai'
    elif controls:
        verdict = 'refuse'
    elif all(criteria[b]['etat'] != 'non evalue' for b in ('B1', 'B2')) and \
            all(v['etat'] in ('tenu', 'non evalue') for v in criteria.values()):
        verdict = 'tenu'
    else:
        verdict = 'non tenu'
    report = dict(mesure='MES-B', regime='b', verdict=verdict, criteres=criteria, controles=controls,
                  environnement=dict(avant=env_before, apres=env_after), provenance=provenance,
                  parametres=dict(fils=args.fils, budget_octets=budget, budget_appareil_octets=device_budget,
                                  delai_global_s=args.delai_global, series=series, argv=argv[1:]),
                  empreintes=digests, cas=results, duree_s=round(time.monotonic() - t_start, 1))
    with open(os.path.join(out_dir, 'rapport_b.json'), 'w', encoding='utf-8') as handle:
        json.dump(report, handle, indent=1, sort_keys=True)
    with open(os.path.join(out_dir, 'tableaux_b.md'), 'w', encoding='utf-8') as handle:
        handle.write(tables(report))
    print('mes_b verdict=%s cas=%d joues=%d refus=%d echecs=%d controles=%d' % (
        verdict, len(results), sum(r['etat'] != 'non_joue' for r in results),
        sum(r['etat'] == 'refus' for r in results), sum(r['etat'] == 'echec' for r in results), len(controls)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
