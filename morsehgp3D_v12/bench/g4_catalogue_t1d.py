#!/usr/bin/env python3
"""T1-d sur G4 : catalogue en flux (arene rapatriee par lots, fin d'etage par tranches de cles ; CONTRAT_CATALOGUE.md,
paragraphe 12), juge par une regle ecrite d'avance. Bibliotheque standard (Python 3.10 nu), aucun assert, aucune
commande GCP ; aucune donnee SemanticKITTI ecrite dans --out (lignes natives des sondes : empreintes, comptes, durees).
Les scenes entieres de plusieurs millions de sites se jouent par MES-B (microbancs/mes_b_scenes/pilote_b.py) ; ce
pilote montre que la trame de 60 000 sites ne paie rien et que la voie en flux rend le meme catalogue sur l'appareil.

Bras (Release, profil 21, MHGP12_ENABLE_CUDA=ON, sm_120) :
  avant      archive epinglee des sources de la base (main 27eca166b), sha256 verifie avant deballage
  avant_bis  le MEME binaire que avant, joue comme un bras distinct (A/A, condition de validite)
  apres      {src} (base + correctif T1-d)

Etapes (journal par etape dans <out>/logs/, rapport <out>/report.json reecrit apres chaque etape ; lignes natives des
sondes gardees telles quelles et relues par le juge, liees a leur commande) :
  1. environnement ; 2. constructions, empreintes des binaires ;
  3. portes rapides de apres (ctest -LE long), puis device_open et device_open_budget (vraie voie appareil) ;
  4. identite (apres) : voie appareil (3 passes) = voie CPU = F2 sur ng00-02 a K5 et K10 et uniformes a K5 ;
  5. voie en flux sur l'appareil reel (apres) : pour ng00-02 a K5 et K10, une prise sans budget (--tranches : octets
     de l'appareil gardes), puis des budgets de l'appareil de 1/2, 1/3, 1/4, 1/6, 1/8 et 1/32 de ces octets
     (--budget-appareil) : arene rapatriee, tranches, empreinte ;
  6. FUL1 (sonde FULL, voie appareil) : avant = apres sur les memes cas ; ng00-02 = session K ;
  7. campagne : K5, ng00, ng01, ng02, --tours tours ; dans chaque tour et pour chaque trame, un processus neuf par
     bras, ordre decale d'un bras par tour et a rebours un tour sur deux ; --passes passes a --fils fils ;
  8. juge (g4_catalogue_t1d_judge.py) et tableaux.

REGLE_T1D (ecrite le 8 octobre 2026, amendee a 08:27 UTC sur les comptes locaux du transit simule, avant toute
session ; RULE du juge) : par processus, temps de l'etage C = mediane
des passes 2 a P du wall_ns de la sonde du catalogue (voie appareil, a chaud, sans budget propre de l'appareil) ; par
tour et par trame, rapport de deux bras ; moyenne geometrique et IC 95 % par bootstrap sur les tours (10 000 tirages,
graine 20261008), bornes NON arrondies. << adopte >> si les empreintes sont identiques partout (etapes 4 a 6, portes
vertes) ET si la borne haute de l'IC du rapport apres / avant est au plus 1,01 sur CHACUNE de ng00, ng01, ng02 a K5
(le contrat principal ne paie rien a 1 % pres) ET si la voie en flux tient (RULE['flux'] : sous chaque budget de
l'appareil, F2 ou refus memory_budget ; au moins une prise identique avec au moins 2 tranches et un lot d'arene
rapatrie par trame et par K ; a K10, au moins une prise identique avec au moins 2 lots rapatries, l'arene en flux lot
par lot sur l'appareil reel) ; << rejete >> sinon ; << refuse >> si une prise
manque ou sort de sa commande, si un binaire change, si le GPU n'est pas isole, ou si l'A/A sort de [0,985 ; 1,015].

Usage :
  python3 g4_catalogue_t1d.py --src DEPOT --data DONNEES --work TRAVAIL --out SORTIE --avant-archive TAR
          --avant-sha256 HEX [--jobs 44] [--fils 48] [--tours 10] [--passes 10] [--nvcc NVCC] [--cmake CMAKE]
          [--ctest CTEST] [--essai]
  python3 g4_catalogue_t1d.py --selftest-judge
DONNEES : lidar_ng0{0,1,2}.u32le et .ids.u32le, uniform_u18_n{8000,16000,32000}.u32le et .ids.u32le. Codes : 0 rapport
ecrit (quel que soit le verdict) ou auto-test conforme ; 1 auto-test en echec ; 2 refus avant toute mesure.
"""
import argparse
import shutil
import sys
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import g4_catalogue_device as base  # noqa: E402
import g4_catalogue_flux as F  # noqa: E402
import g4_catalogue_flux_lecteur as L  # noqa: E402
import g4_catalogue_t1d_judge as T  # noqa: E402


def builds(s, args, nvcc, cmake):
    work, results, probes = Path(args.work), {}, {}
    problem = F.unpack_base(args.avant_archive, args.avant_sha256, work / 'src_avant')
    roots = {'avant': work / 'src_avant' if problem is None else None, 'apres': Path(args.src)}
    if problem:
        results['avant'] = {'ok': False, 'problem': problem}
    for arm, root in roots.items():
        if root is None:
            continue
        targets = [] if arm == 'apres' else ['mhgp12_catalogue_probe', 'mhgp12_full_probe']
        results[arm] = F.build_one(s, root, work / ('b_' + arm), cmake, nvcc, args.jobs, arm, None, targets)
        if results[arm]['ok']:
            probes[arm] = work / ('b_' + arm) / 'mhgp12_catalogue_probe'
    s.step('builds', results)
    s.step('binaries', {arm: base.sha256_file(p) for arm, p in probes.items() if p.is_file()})
    return probes


def slices(s, probe, threads):
    """Voie en flux sur l'appareil reel : prise sans budget, puis budgets de l'appareil en fractions de ses octets."""
    rows = []
    for case, k in T.SLICE_CASES:
        free = F.probe_run(s, 'flux_%s_k%d_libre' % (case, k), probe, case, T.spec_free(k, threads))
        rows.append({'case': case, 'k': k, 'budget': 0, 'run': free})
        s.step('slices', rows)
        state, _, parsed = L.read_catalogue(free, T.spec_free(k, threads))
        if state != 'ok':
            continue
        held = parsed['tranches'][-1]['device_bytes']
        for num, den in T.FRACTIONS:
            budget = held * num // den
            run = F.probe_run(s, 'flux_%s_k%d_%d_%d' % (case, k, num, den), probe, case,
                              T.spec_budget(k, threads, budget))
            rows.append({'case': case, 'k': k, 'budget': budget, 'run': run})
            s.step('slices', rows)


def campaign(s, args, probes):
    rows, arms = [], list(T.ARMS)
    for r in range(args.tours):
        order = arms[r % len(arms):] + arms[:r % len(arms)]
        if r % 2:
            order.reverse()
        for frame in T.FRAMES:
            for position, arm in enumerate(order):
                probe = probes['avant' if arm == 'avant_bis' else arm]
                run = F.probe_run(s, 'c_r%d_%s_%s' % (r, frame, arm), probe, frame,
                                  T.spec_campaign(args.fils, args.passes))
                rows.append({'round': r, 'frame': frame, 'arm': arm, 'position': position, 'run': run})
            s.step('campaign', rows)


def tables(report):
    verdict = report.get('verdict') or {}
    stats = verdict.get('stats') or {}
    lines = ['# T1-d : catalogue en flux sur G4', '', 'Verdict de REGLE_T1D : **%s**.' % verdict.get('verdict'), '']
    for kind, label in (('refused', 'refus'), ('rejected', 'rejet')):
        lines += ['- %s : %s' % (label, item) for item in verdict.get(kind) or []]
    lines += ['', '## Cout pour la trame (rapport apres/avant et A/A, IC 95 %, bornes non arrondies)', '',
              '| controle | ng00 | ng01 | ng02 | verdict |', '| --- | ---: | ---: | ---: | --- |']
    for name, lever in (stats.get('levers') or {}).items():
        cells = [(lever['frames'].get(f) or {}).get('affiche', '-') for f in T.FRAMES]
        lines.append('| %s | %s | %s |' % (name, ' | '.join(cells), lever['verdict']))
    lines += ['', '## Voie en flux sur l\'appareil (plus grand nombre de tranches d\'une prise identique)', '',
              '| cas | tranches |', '| --- | ---: |']
    lines += ['| %s | %s |' % (key, value) for key, value in sorted((stats.get('tranches') or {}).items())]
    return '\n'.join(lines) + '\n'


def main_run(args):
    problem = F.refused_before(args)
    if problem:
        print('refus : ' + problem)
        return 2
    Path(args.work).mkdir(parents=True, exist_ok=True)
    s = base.Session(Path(args.out))
    s.data = args.data
    s.report.update(schema='mhgp12_g4_catalogue_t1d_v1', rule=T.RULE,
                    options={'rounds': args.tours, 'passes': args.passes, 'threads': args.fils, 'jobs': args.jobs,
                             'essai': args.essai, 'avant_sha256': args.avant_sha256})
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
            F.gates(s, args, ctest)
            F.identity(s, probes['apres'], args.fils)
            slices(s, probes['apres'], args.fils)
            if 'avant' in probes:
                F.ful1(s, args)
                s.step('gpu_quiet_before', base.gpu_quiet(s, 'avant'))
                campaign(s, args, probes)
                s.step('gpu_quiet_after', base.gpu_quiet(s, 'apres'))
                s.step('binaries_after', {arm: base.sha256_file(p) for arm, p in probes.items() if p.is_file()})
    s.report['finished_utc'] = base.now()
    s.report['verdict'] = T.judge(s.report)
    if args.essai:
        s.report['verdict']['verdict'] = 'essai (' + s.report['verdict']['verdict'] + ')'
    s.save()
    (Path(args.out) / 'tableaux_t1d.md').write_text(tables(s.report), encoding='utf-8')
    print('verdict : %s' % s.report['verdict']['verdict'])
    return 0


def main():
    parser = argparse.ArgumentParser(description='T1-d sur G4 : catalogue en flux.')
    for name in ('--src', '--data', '--work', '--out', '--avant-archive', '--avant-sha256', '--nvcc', '--cmake',
                 '--ctest'):
        parser.add_argument(name)
    parser.add_argument('--jobs', type=int, default=44)
    parser.add_argument('--fils', type=int, default=48)
    parser.add_argument('--tours', type=int, default=10)
    parser.add_argument('--passes', type=int, default=10)
    parser.add_argument('--essai', action='store_true')
    parser.add_argument('--selftest-judge', action='store_true')
    args = parser.parse_args()
    if args.selftest_judge:
        import g4_catalogue_t1d_selftest as selftest
        return selftest.selftest()
    if not all((args.src, args.data, args.work, args.out, args.avant_archive, args.avant_sha256)):
        print('refus : --src, --data, --work, --out, --avant-archive et --avant-sha256 sont exiges')
        return 2
    if not args.essai and (args.tours < 10 or args.passes < 10 or args.fils != 48):
        print('refus : contrat de mesure (au moins 10 tours de 10 passes a 48 fils), sauf --essai')
        return 2
    args.v12set, args.delai = None, 0
    return main_run(args)


if __name__ == '__main__':
    sys.exit(main())
