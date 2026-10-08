#!/usr/bin/env python3
"""T2-d-C sur G4 : transferts et publication du catalogue sur l'appareil (CONTRAT_CATALOGUE.md, paragraphe 11),
juges par une regle ecrite d'avance. Bibliotheque standard (Python 3.10 nu), aucun assert, aucune commande GCP ;
aucune donnee SemanticKITTI ecrite dans --out (lignes natives des sondes : empreintes, comptes, durees).

Bras (sondes construites ici : Release, profil 21, MHGP12_ENABLE_CUDA=ON, sm_120) ; chaque bras d'ablation est {src}
avec UNE substitution exacte d'une constante par levier (SUBSTITUTIONS), et porte le nom de ce qu'il retire :
  avant                   archive epinglee des sources de la base (main 902041f66), sha256 verifie avant deballage
  avant_bis               le MEME binaire que avant, joue comme un bras distinct (A/A, condition de validite)
  apres                   {src} (base + correctif T2-d-C) : produit complet, portes rapides
  sans_anticipation       kAnticipateOutputs = false : sorties reservees a leur prise par la fin d'etage, sans
                          reservation anticipee ni premier toucher (comme la base)
  sans_double_tampon      kStagingSlots = 1 : tranches du flux en serie (copie, puis consommation)
  repli_cles_entieres     kChainWindow = 2^62 : bornes des chaines lues sur TOUTES les cles (8 n octets par chaine) ;
                          garde la compaction des positions et le rapatriement de l'ordre et des cles des seules
                          chaines ; ce n'est PAS l'ancien repli (verdicts, ordre et cles des n boules, 16 n octets)
  flux_et_repli_selectif  les trois substitutions : reste le flux de sortie (tranches en serie, niveaux materialises
                          depuis la tranche, sans tampon de mots) et le repli selectif ; sur ng00 et ng01 (aucune
                          chaine retriee) il isole le flux, sur ng02 le flux et le repli selectif ensemble

Etapes (journal par etape dans <out>/logs/, rapport <out>/report.json reecrit apres chaque etape ; lignes natives des
sondes gardees telles quelles, le juge les relit et les lie a la commande, g4_catalogue_flux_lecteur.py) :
  1. environnement (nvcc, cmake, GPU, processus de calcul, noyau Linux, charge) ;
  2. constructions ; empreintes des binaires ;
  3. portes rapides de apres (ctest -LE long), puis device_open et device_open_budget de apres hors CTest (la vraie
     voie appareil contre la voie CPU, et ses refus sous budget serre de l'hote et de l'appareil) ;
  4. identite (apres) : voie appareil, 3 passes, = voie CPU (MHGP12DP, niveaux, table, grand livre, comptes,
     diagnostics physiques, reprises) sur ng00, ng01, ng02 a K5 et K10 et uniformes de 8 000, 16 000, 32 000 a K5 ;
     voie CPU = F2 ; chaque bras construit : MHGP12DP de F2 sur ng00-02 a K5 (voie appareil, 2 passes) ;
  5. FUL1 (sonde FULL, voie appareil, 2 passes) : avant = apres sur les memes cas ; ng00-02 = session K ;
  6. campagne decisive : K5, ng00, ng01, ng02, --tours tours ; dans chaque tour et pour chaque trame, un processus
     neuf par bras, ordre decale d'un bras par tour et a rebours un tour sur deux ; --passes passes a --fils fils ;
  7. mutant appareil flux_sans_attente_appareil (tranche consommee sans attendre son evenement), sonde --device
     --digest --passes=2 (sans --digest-complet : la table d'un catalogue faux pourrait etre lue hors bornes) ;
  8. informations (ne decident rien ; sautees au-dela de --delai secondes) : K10 (avant, apres), cache de blocs de
     4 Gio (apres avec et sans), mur FULL (avant, apres), trames v12set (avant, apres, un tour) ;
  9. juge (g4_catalogue_flux_judge.py) et rapport.

REGLE_T2D_C (ecrite le 8 octobre 2026 avant toute session, completee le meme jour apres la contre-lecture
d'admission de l'auditeur Codex, toujours avant toute session ; RULE du juge) : par processus, temps de l'etage C =
mediane des passes 2 a P du wall_ns de la sonde du catalogue (voie appareil, a chaud) ; par tour et par trame,
rapport de deux bras ; moyenne geometrique des rapports des tours et IC 95 % par bootstrap sur les tours (10 000
tirages, graine 20261008), bornes NON arrondies. << adopte >> si les empreintes sont identiques partout (etapes 4 et
5, mutant tue, portes vertes) ET si la borne haute de l'IC du rapport apres / avant est sous 1 sur CHACUNE de ng00,
ng01, ng02 a K5 ; << rejete >> sinon ; << refuse >> si une prise manque ou sort de sa commande (processus en echec,
sortie hors schema ou liee a d'autres parametres, champ absent, passes absentes, moins de --tours tours par trame et
par bras, construction ou porte non jouee, GPU non isole, binaire change pendant la campagne, etapes d'une passe dont
la somme depasse le total), si le mutant n'a pas ete compare (RULE['mutant']), ou si le controle A/A sort de sa
fenetre (moyenne geometrique non arrondie du rapport avant_bis / avant hors de [0,985 ; 1,015] sur une trame : la
session ne mesure pas a 1,5 % pres). Leviers publies, chacun par son bras, meme statistique : flux (avant ->
flux_et_repli_selectif, decide sur ng00 et ng01), double_tampon (sans_double_tampon -> apres), sorties_anticipees
(sans_anticipation -> apres), fenetres_du_repli (repli_cles_entieres -> apres, decide sur ng02 seul) ; un levier
rejete se retire par la substitution de son bras ; l'ancien repli n'a pas de bras (il ne se mesure que dans le lot).

Usage :
  python3 g4_catalogue_flux.py --src DEPOT --data DONNEES --work TRAVAIL --out SORTIE --avant-archive TAR
          --avant-sha256 HEX [--v12set TAR] [--jobs 44] [--fils 48] [--tours 10] [--passes 10] [--delai 1700]
          [--nvcc NVCC] [--cmake CMAKE] [--ctest CTEST] [--essai]
  python3 g4_catalogue_flux.py --selftest-judge          (auto-test du juge par injections, sans outil ni donnee)
  python3 g4_catalogue_flux.py --check-substitutions RACINE_V12   (motifs des bras et du mutant presents une fois)
DONNEES : lidar_ng0{0,1,2}.u32le et .ids.u32le, uniform_u18_n{8000,16000,32000}.u32le et .ids.u32le. --essai :
essai local, contrat de mesure non exige, verdict marque << essai >>. Codes : 0 rapport ecrit (quel que soit le
verdict) ou auto-test conforme ; 1 auto-test en echec ; 2 refus avant toute mesure.
"""
import argparse
import json
import shutil
import sys
import tarfile
import time
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import g4_catalogue_device as base  # noqa: E402
import g4_catalogue_flux_judge as J  # noqa: E402
import g4_catalogue_flux_lecteur as L  # noqa: E402
import g4_catalogue_flux_tables as T  # noqa: E402

DATA = {'ng00': 'lidar_ng00', 'ng01': 'lidar_ng01', 'ng02': 'lidar_ng02', 'u8000': 'uniform_u18_n8000',
        'u16000': 'uniform_u18_n16000', 'u32000': 'uniform_u18_n32000'}
SUBSTITUTIONS = {
    'kAnticipateOutputs': ('src/catalogue/finish_outputs.hpp', 'inline constexpr bool kAnticipateOutputs = true;',
                           'inline constexpr bool kAnticipateOutputs = false;'),
    'kStagingSlots': ('src/catalogue/transfer_meter.hpp', 'inline constexpr u64 kStagingSlots = 2;',
                      'inline constexpr u64 kStagingSlots = 1;'),
    'kChainWindow': ('src/catalogue/finish_repair.hpp', 'inline constexpr u64 kChainWindow = 64;',
                     'inline constexpr u64 kChainWindow = u64{1} << 62;'),
    'mutant': ('src/catalogue/device_cuda.cu',
               '  Outcome wait(u64 c) noexcept { return check(cudaEventSynchronize(slot_events[c])); }',
               '  Outcome wait(u64) noexcept { return {}; }'),
}
ARM_SUBSTITUTIONS = {'sans_anticipation': ['kAnticipateOutputs'], 'sans_double_tampon': ['kStagingSlots'],
                     'repli_cles_entieres': ['kChainWindow'],
                     'flux_et_repli_selectif': ['kAnticipateOutputs', 'kStagingSlots', 'kChainWindow']}
CACHE_BYTES = 4 << 30


def substitute(root, names):
    """Substitutions exactes sur une copie (chaque motif present une seule fois) ; rend None ou le motif en defaut."""
    for name in names:
        relative, find, replace = SUBSTITUTIONS[name]
        path = Path(root) / relative
        text = path.read_text(encoding='utf-8')
        if text.count(find) != 1:
            return name
        path.write_text(text.replace(find, replace), encoding='utf-8')
    return None


def check_substitutions(root):
    for name, (relative, find, _) in SUBSTITUTIONS.items():
        if (Path(root) / relative).read_text(encoding='utf-8').count(find) != 1:
            print('substitutions_ecart : %s' % name)
            return 1
    print('substitutions_ok bras=%d mutant=1' % len(ARM_SUBSTITUTIONS))
    return 0


def unpack_base(archive, expected, folder):
    """Archive tar.gz des sources de la base, sha256 verifie, membres sous morsehgp3D_v12/ seulement."""
    if base.sha256_file(archive) != expected:
        return 'empreinte de l\'archive differente'
    try:
        with tarfile.open(archive, 'r:gz') as tar:
            members = tar.getmembers()
            for m in members:
                parts = Path(m.name).parts
                if m.issym() or m.islnk() or not parts or parts[0] != 'morsehgp3D_v12' or '..' in parts or \
                        Path(m.name).is_absolute():
                    return 'membre refuse : ' + m.name
            tar.extractall(folder, members=members)
    except (OSError, tarfile.TarError) as error:
        return 'archive illisible : %s' % error
    return None


def native(text):
    """Lignes natives d'une sonde (objets JSON, cles uniques, constantes finies) et nombre de lignes illisibles."""
    rows, bad = [], 0
    if not text.isascii():
        return rows, 1
    for line in text.splitlines():
        if not line.strip():
            continue
        try:
            obj = json.loads(line, object_pairs_hook=base.unique_object, parse_constant=base.reject_constant)
        except (ValueError, RecursionError):
            bad += 1
            continue
        if isinstance(obj, dict):
            rows.append(obj)
        else:
            bad += 1
    return rows, bad


def captured(r, options):
    rows, bad = native(r['stdout'])
    return {'code': r['code'], 'timeout': r['timeout'], 'seconds': r['seconds'], 'bad_lines': bad,
            'options': options, 'rows': rows}


def probe_run(s, name, probe, case, spec, timeout=600):
    """Sonde du catalogue sur une trame ; options produites par le lecteur (une seule source avec le juge)."""
    stem = Path(s.data) / DATA[case]
    options = L.catalogue_options(spec)
    r = s.run(name, [probe, str(stem) + '.u32le', str(stem) + '.ids.u32le'] + options, timeout)
    return captured(r, options)


def full_run(s, name, probe, case, k, threads, passes, timeout=900):
    stem = Path(s.data) / DATA[case]
    options = L.full_options(k, threads, passes)
    r = s.run(name, [probe, '--trame=%s.u32le,%s.ids.u32le,%s' % (stem, stem, case)] + options, timeout)
    return captured(r, options)


def build_one(s, root, bdir, cmake, nvcc, jobs, name, modules, targets):
    configure = [cmake, '-S', Path(root) / 'morsehgp3D_v12', '-B', bdir, '-DCMAKE_BUILD_TYPE=Release',
                 '-DMHGP12_COORD_BITS=21', '-DMHGP12_ENABLE_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc]
    if modules:
        configure.append('-DMHGP12_MODULES=' + modules)
    c = s.run(name + '_configure', configure, 900)
    if c['code'] != 0:
        return {'ok': False, 'configure': c['code']}
    cmd = [cmake, '--build', bdir, '-j', str(jobs)]
    for t in targets:
        cmd += ['--target', t]
    b = s.run(name + '_build', cmd, 2400)
    return {'ok': b['code'] == 0, 'configure': 0, 'build': b['code'], 'seconds': c['seconds'] + b['seconds']}


def copy_source(src, target):
    shutil.rmtree(target, ignore_errors=True)
    shutil.copytree(Path(src) / 'morsehgp3D_v12', Path(target) / 'morsehgp3D_v12',
                    ignore=shutil.ignore_patterns('__pycache__', '.git'))


def builds(s, args, nvcc, cmake):
    work = Path(args.work)
    roots, results, probes = {}, {}, {}
    problem = unpack_base(args.avant_archive, args.avant_sha256, work / 'src_avant')
    roots['avant'] = work / 'src_avant' if problem is None else None
    results['avant'] = {'ok': False, 'problem': problem} if problem else None
    roots['apres'] = Path(args.src)
    for arm, names in ARM_SUBSTITUTIONS.items():
        copy_source(args.src, work / ('src_' + arm))
        bad = substitute(work / ('src_' + arm) / 'morsehgp3D_v12', names)
        roots[arm] = work / ('src_' + arm) if bad is None else None
        results[arm] = {'ok': False, 'problem': 'motif ' + bad} if bad else None
    for arm, root in roots.items():
        if root is None:
            continue
        full = arm in ('apres', 'avant')
        targets = [] if arm == 'apres' else ['mhgp12_catalogue_probe'] + (['mhgp12_full_probe'] if full else [])
        results[arm] = build_one(s, root, work / ('b_' + arm), cmake, nvcc, args.jobs, arm,
                                 None if full else 'catalogue', targets)
        if results[arm]['ok']:
            probes[arm] = work / ('b_' + arm) / 'mhgp12_catalogue_probe'
    s.step('builds', results)
    s.step('binaries', {arm: base.sha256_file(p) for arm, p in probes.items() if p.is_file()})
    return probes


def gates(s, args, ctest):
    bdir = Path(args.work) / 'b_apres'
    g = s.run('ctest', [ctest, '--no-tests=error', '-LE', 'long', '-j', str(args.jobs), '--output-on-failure'], 2400,
              cwd=bdir)
    out = {'code': g['code'], 'timeout': g['timeout'], 'tail': g['stdout'][-1500:]}
    for group in ('device_open', 'device_open_budget'):
        u = s.run(group, [bdir / 'mhgp12_catalogue_device_unit', group], 1200)
        out[group] = {'code': u['code'], 'timeout': u['timeout'], 'stdout': u['stdout'][-4000:]}
    s.step('gates', out)


def identity(s, probe, threads):
    cases = []
    for case, k in J.IDENTITY_CASES:
        cpu = probe_run(s, 'id_%s_k%d_cpu' % (case, k), probe, case, L.catalogue_spec('cpu', k, threads, 1, True, True))
        dev = probe_run(s, 'id_%s_k%d_dev' % (case, k), probe, case,
                        L.catalogue_spec('device', k, threads, 3, True, True, True))
        cases.append({'case': case, 'k': k, 'cpu': cpu, 'device': dev})
        s.step('identity', cases)


def arms_identity(s, probes, threads):
    rows = []
    for arm in J.BUILT_ARMS:
        for frame in J.FRAMES:
            run = probe_run(s, 'bras_%s_%s' % (arm, frame), probes[arm], frame,
                            L.catalogue_spec('device', 5, threads, 2, digest=True))
            rows.append({'arm': arm, 'case': frame, 'run': run})
            s.step('arms_identity', rows)


def ful1(s, args):
    rows = []
    for arm in ('avant', 'apres'):
        probe = Path(args.work) / ('b_' + arm) / 'mhgp12_full_probe'
        for case, k in J.FUL1_CASES:
            run = full_run(s, 'ful1_%s_%s_k%d' % (arm, case, k), probe, case, k, args.fils, 2)
            rows.append({'arm': arm, 'case': case, 'k': k, 'run': run})
            s.step('ful1', rows)


def campaign(s, args, probes):
    rows, arms = [], list(J.ARMS)
    for r in range(args.tours):
        order = arms[r % len(arms):] + arms[:r % len(arms)]
        if r % 2:
            order.reverse()
        for frame in J.FRAMES:
            for position, arm in enumerate(order):
                probe = probes['avant' if arm == 'avant_bis' else arm]
                spec = L.catalogue_spec('device', 5, args.fils, args.passes, sorties=arm not in J.HISTORICAL)
                run = probe_run(s, 'c_r%d_%s_%s' % (r, frame, arm), probe, frame, spec)
                rows.append({'round': r, 'frame': frame, 'arm': arm, 'position': position, 'run': run})
            s.step('campaign', rows)


def mutant(s, args, nvcc, cmake):
    work = Path(args.work) / 'mutant'
    copy_source(args.src, work)
    out = {'id': 'flux_sans_attente_appareil', 'applied': substitute(work / 'morsehgp3D_v12', ['mutant']) is None}
    if out['applied']:
        built = build_one(s, work, work / 'b', cmake, nvcc, args.jobs, 'mutant', 'catalogue',
                          ['mhgp12_catalogue_probe'])
        out['built'] = built['ok']
        if built['ok']:
            out['run'] = probe_run(s, 'mutant_ng00', work / 'b' / 'mhgp12_catalogue_probe', 'ng00',
                                   L.catalogue_spec('device', 5, args.fils, 2, digest=True))
    s.step('mutant', out)


def informations(s, args, probes, deadline):
    """Mesures publiees sans decider ; chaque processus n'est lance qu'avant l'echeance (sinon << non joue >>)."""
    info, skipped = {}, []

    def allowed(name):
        if time.time() < deadline:
            return True
        skipped.append(name)
        return False
    for key, k, arms, rounds, passes in (('k10', 10, ('avant', 'apres'), 3, 5),
                                         ('cache', 5, ('apres', 'apres_cache'), 3, args.passes)):
        rows = []
        for r in range(rounds):
            for frame in J.FRAMES:
                for arm in (arms if r % 2 == 0 else tuple(reversed(arms))):
                    name = 'i_%s_r%d_%s_%s' % (key, r, frame, arm)
                    if not allowed(name):
                        continue
                    spec = L.catalogue_spec('device', k, args.fils, passes, sorties=arm not in J.HISTORICAL,
                                            cache=CACHE_BYTES if arm == 'apres_cache' else 0)
                    run = probe_run(s, name, probes['apres' if arm == 'apres_cache' else arm], frame, spec)
                    rows.append({'round': r, 'frame': frame, 'arm': arm, 'k': k, 'run': run})
        info[key] = rows
        s.step('informations', dict(info, non_joues=skipped))
    rows = []
    for r in range(3):
        for frame in J.FRAMES:
            for arm in ('avant', 'apres') if r % 2 == 0 else ('apres', 'avant'):
                name = 'i_full_r%d_%s_%s' % (r, frame, arm)
                if allowed(name):
                    probe = Path(args.work) / ('b_' + arm) / 'mhgp12_full_probe'
                    rows.append({'round': r, 'frame': frame, 'arm': arm,
                                 'run': full_run(s, name, probe, frame, 5, args.fils, args.passes)})
    info['full'] = rows
    s.step('informations', dict(info, non_joues=skipped))
    if args.v12set:
        info['v12set'] = v12set(s, args, probes, allowed)
    s.step('informations', dict(info, non_joues=skipped))


def v12set(s, args, probes, allowed):
    folder = Path(args.work) / 'v12set'
    names = unpack_v12set(args.v12set, folder)
    if names is None:
        return {'refus': 'archive v12set illisible'}
    rows = []
    for i, name in enumerate(names):
        for arm in ('avant', 'apres') if i % 2 == 0 else ('apres', 'avant'):
            label = 'i_v12set_%s_%s' % (name, arm)
            if not allowed(label):
                continue
            spec = L.catalogue_spec('device', 5, args.fils, 6, sorties=arm not in J.HISTORICAL)
            stem = folder / name
            options = L.catalogue_options(spec)
            r = s.run(label, [probes[arm], str(stem) + '.u32le', str(stem) + '.ids.u32le'] + options, 600)
            rows.append({'frame': name, 'arm': arm, 'run': captured(r, options)})
    return rows


def unpack_v12set(archive, folder):
    """Archive tar plate (fichiers simples) et son manifeste ; rend les noms des trames, ou None."""
    folder.mkdir(parents=True, exist_ok=True)
    try:
        with tarfile.open(archive) as tar:
            members = tar.getmembers()
            if any(not m.isfile() or '/' in m.name or m.name.startswith('.') for m in members):
                return None
            tar.extractall(folder, members=members)
        manifest = json.loads((folder / 'bundle_manifest.json').read_text(encoding='utf-8'))
        return [case['name'] for case in manifest['cases']]
    except (OSError, ValueError, KeyError, TypeError, tarfile.TarError):
        return None


def refused_before(args):
    for folder in (args.src, args.data):
        if not Path(folder).is_dir():
            return 'dossier absent : %s' % folder
    out, work = Path(args.out).resolve(), Path(args.work).resolve()
    if out == work or out in work.parents or work in out.parents:
        return '--work et --out doivent etre disjoints'
    for stem in DATA.values():
        for suffix in ('.u32le', '.ids.u32le'):
            if not (Path(args.data) / (stem + suffix)).is_file():
                return 'donnee absente : %s%s' % (stem, suffix)
    return None


def main_run(args):
    start = time.time()
    problem = refused_before(args)
    if problem:
        print('refus : ' + problem)
        return 2
    Path(args.work).mkdir(parents=True, exist_ok=True)
    s = base.Session(Path(args.out))
    s.data = args.data
    s.report.update(schema='mhgp12_g4_catalogue_flux_v2', rule=J.RULE,
                    options={'rounds': args.tours, 'passes': args.passes, 'threads': args.fils, 'jobs': args.jobs,
                             'essai': args.essai, 'avant_sha256': args.avant_sha256, 'delai': args.delai})
    s.report.pop('contract', None)
    s.report.pop('budget_ns', None)
    nvcc = base.find_tool(args.nvcc, 'nvcc', ['/usr/local/cuda/bin/nvcc'])
    cmake = args.cmake or shutil.which('cmake') or 'cmake'
    ctest = args.ctest or shutil.which('ctest') or 'ctest'
    env = base.environment(s, nvcc, cmake)
    env['kernel'] = s.run('env_kernel', ['uname', '-r'], 60)['stdout'].strip()
    s.step('environment', env)
    if nvcc is not None:
        probes = builds(s, args, nvcc, cmake)
        if 'apres' in probes:
            gates(s, args, ctest)
            identity(s, probes['apres'], args.fils)
            if all(arm in probes for arm in J.BUILT_ARMS):
                arms_identity(s, probes, args.fils)
                ful1(s, args)
                s.step('gpu_quiet_before', base.gpu_quiet(s, 'avant'))
                campaign(s, args, probes)
                s.step('gpu_quiet_after', base.gpu_quiet(s, 'apres'))
                s.step('binaries_after', {arm: base.sha256_file(p) for arm, p in probes.items() if p.is_file()})
            mutant(s, args, nvcc, cmake)
            if all(arm in probes for arm in J.BUILT_ARMS):
                informations(s, args, probes, start + args.delai)
    s.report['finished_utc'] = base.now()
    s.report['verdict'] = J.judge(s.report)
    if args.essai:
        s.report['verdict']['verdict'] = 'essai (' + s.report['verdict']['verdict'] + ')'
    s.save()
    (Path(args.out) / 'tableaux_t2dc.md').write_text(T.tables(s.report), encoding='utf-8')
    print('verdict : %s' % s.report['verdict']['verdict'])
    return 0


def main():
    parser = argparse.ArgumentParser(description='T2-d-C sur G4 : transferts et publication du catalogue.')
    for name in ('--src', '--data', '--work', '--out', '--avant-archive', '--avant-sha256', '--v12set', '--nvcc',
                 '--cmake', '--ctest', '--check-substitutions'):
        parser.add_argument(name)
    parser.add_argument('--jobs', type=int, default=44)
    parser.add_argument('--fils', type=int, default=48)
    parser.add_argument('--tours', type=int, default=10)
    parser.add_argument('--passes', type=int, default=10)
    parser.add_argument('--delai', type=int, default=1700)
    parser.add_argument('--essai', action='store_true')
    parser.add_argument('--selftest-judge', action='store_true')
    args = parser.parse_args()
    if args.selftest_judge:
        import g4_catalogue_flux_selftest as selftest
        return selftest.selftest()
    if args.check_substitutions:
        return check_substitutions(args.check_substitutions)
    if not all((args.src, args.data, args.work, args.out, args.avant_archive, args.avant_sha256)):
        print('refus : --src, --data, --work, --out, --avant-archive et --avant-sha256 sont exiges')
        return 2
    if not args.essai and (args.tours < 10 or args.passes < 10 or args.fils != 48):
        print('refus : contrat de mesure (au moins 10 tours de 10 passes a 48 fils), sauf --essai')
        return 2
    return main_run(args)


if __name__ == '__main__':
    sys.exit(main())
