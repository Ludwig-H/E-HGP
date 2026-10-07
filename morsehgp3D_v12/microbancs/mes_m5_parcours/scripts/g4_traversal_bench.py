#!/usr/bin/env python3
"""MES-M5 sur G4 : parcours des boites du catalogue en largeur sur le GPU (microbanc hors produit de la v12).

Une seule entree pour la session : construit, vide le parcours de la v11, verifie l'identite (hote, puis appareil),
tue les mutants, passe Compute Sanitizer, mesure le parcours GPU et la meme etape de la v11 sur la meme machine, juge,
puis ecrit un rapport JSON. Bibliotheque standard seulement (Python 3.10 nu de la VM) ; aucune commande GCP.

Dossiers : --out recoit le rapport, les journaux et les resultats legers (a rapatrier) ; --work recoit constructions,
vidages et fixtures (derives de SemanticKITTI pour les vidages : JAMAIS rapatries). --work ne peut pas etre dans --out.

Etapes (journal par etape dans <out>/logs/) :
  1. environnement (nvcc, cmake, compilateur, nvidia-smi, uptime, processus GPU) ;
  2. bibliotheque v11 gelee (--v11-lib, ou construite depuis --repo comme MES-M2), microbanc (hote + CUDA sm_120) ;
     empreintes des binaires, des sources du microbanc, des en-tetes de MES-M2 et des sources v11 du parcours ;
  3. vidages du parcours v11 des trames (--data) en parallele, decoupe pour Compute Sanitizer, fixtures synthetiques
     (reference v11 recontrolee par l'oracle Python pour u21, oracle pour u32) ;
  4. identite hote (warp simule) de tous les vidages, noeuds compris, tous les mutants, portes unitaires ;
  5. Compute Sanitizer (memcheck, racecheck, synccheck) sur la decoupe et deux fixtures u32 ; identite appareil des
     fixtures et mutants sur l'appareil ;
  6. 1 + P tours : par tour et par cas (ordre tournant), un processus de chrono v11 (frontiere + passe unique, W fils,
     passes chaudes) puis un processus du banc GPU (echauffement, R passes, profil, verification, sans mutant) ; le
     premier tour est jete (MESURE.md § 5) ;
  7. juge (regle ecrite d'avance, README.md § 6) et rapport <out>/report.json.

Usage :
  python3 g4_traversal_bench.py --out OUT --work WORK --data DONNEES [--repo DEPOT] [--v11-lib LIB]
          [--frames ng00,ng01,ng02] [--configs 5:16,5:24,10:24] [--decide 5:24,10:24] [--processes 5]
          [--reps 15] [--warmup 3] [--v11-workers 48] [--v11-passes 10] [--jobs N] [--no-cuda] [--nvcc NVCC]
          [--cmake CMAKE] [--crop 4000] [--skip-sanitizer]
  python3 g4_traversal_bench.py --selftest-judge     (auto-test du juge par injections, sans outil ni donnee)
Codes : 0 rapport ecrit (quel que soit le verdict) ou auto-test conforme ; 2 refus avant toute mesure (arguments, depot
ou microbanc MES-M2 introuvable, --work dans --out) ; 1 auto-test du juge en echec.
"""

import argparse
import importlib.util
import json
import math
import os
import random
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True  # aucun __pycache__ dans les sources (paquet de session)
HERE = Path(__file__).resolve().parent.parent  # racine du microbanc (mes_m5_parcours)
THRESHOLD = 0.25  # regle d'adoption : parcours GPU, transferts compris, <= 1/4 de la frontiere + passe unique v11
BOOTSTRAP = 10000
SEED = 20261007
MUTANTS = ['temoin_perdu', 'repere_enfant', 'compactage_instable', 'bissection_decalee', 'ex_aequo_inverses',
           'axe_dernier_maximum']
# Mutants que les trames LiDAR doivent tuer (chacun sur au moins un cas reel) ; repere_enfant est equivalent au
# profil u21 (voie native partout, s <= 22) : il doit etre tue par la fixture u32 de la coquille.
REAL_KILLS = ['temoin_perdu', 'compactage_instable', 'bissection_decalee', 'ex_aequo_inverses', 'axe_dernier_maximum']
FIXTURE_KILLS = {'coquille48_u32_k5_l24': ['repere_enfant']}
SANITIZER_FIXTURES = ['coquille48_u32_k5_l24', 'uniforme_u32_k3_l8']


def load_m2(m2_dir):
  """Outils de session de MES-M2 (Session, empreintes, environnement, nvcc, construction de la v11) : reutilises."""
  path = Path(m2_dir) / 'scripts' / 'g4_leaf_bench.py'
  if not path.is_file():
    return None
  spec = importlib.util.spec_from_file_location('mhgp12_m2_session', str(path))
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


# ------------------------------------------------------------------------------------------------------------- juge
def finite_positive(values):
  return bool(values) and all(isinstance(v, (int, float)) and math.isfinite(v) and v > 0 for v in values)


def median(v):
  v = sorted(v)
  n = len(v)
  if n == 0:
    return float('nan')
  return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def bootstrap_ci(logs, rng):
  boot = []
  for _ in range(BOOTSTRAP):
    sample = [logs[rng.randrange(len(logs))] for _ in logs]
    boot.append(sum(sample) / len(sample))
  boot.sort()
  return math.exp(boot[int(0.025 * BOOTSTRAP)]), math.exp(boot[int(0.975 * BOOTSTRAP) - 1])


def judge(evidence):
  """Regle ecrite d'avance (README.md § 6). evidence : dictionnaire pur (aucune E/S).

  Cles : cases (noms attendus), deciding (noms qui decident), processes (tours exiges), refusals (refus du banc),
  host_identity {cas: bool}, device_identity {cas: bool}, fixtures_identity {nom: bool}, unit_ok, fixtures_ok,
  host_kills {mutant: [cas qui le tuent]}, device_kills {mutant: [fixtures qui le tuent]}, sanitizer {outil: code},
  isolation, rounds {cas: [{'v11_ms': x, 'gpu_ms': y, 'gpu_resident_ms': z, 'timed_allocations': a,
  'gpu_identity': b}]}.
  Rend (verdict, raisons de refus, raisons de rejet, statistiques par cas)."""
  refused = list(evidence.get('refusals', []))
  rejected = []
  cases = list(evidence.get('cases', []))
  deciding = list(evidence.get('deciding', []))
  need = int(evidence.get('processes', 0))
  if not cases or not deciding or need < 1:
    refused.append('manifeste vide : aucun cas, aucun cas qui decide ou aucun processus exige')
  for name in deciding:
    if name not in cases:
      refused.append('%s : cas qui decide hors du manifeste' % name)
  if not evidence.get('isolation', False):
    refused.append('isolation GPU non certifiee')
  if not evidence.get('unit_ok', False):
    refused.append('portes unitaires en echec ou absentes')
  if not evidence.get('fixtures_ok', False):
    refused.append('fixtures ou controles de l oracle en echec ou absents')
  sanitizer = evidence.get('sanitizer', {})
  for tool in ('memcheck', 'racecheck', 'synccheck'):
    if sanitizer.get(tool) != 0:
      refused.append('compute-sanitizer %s : %s' % (tool, sanitizer.get(tool, 'non joue')))
  host = evidence.get('host_identity', {})
  device = evidence.get('device_identity', {})
  for name in cases:
    if name not in host:
      refused.append('%s : identite hote absente' % name)
    elif host[name] is not True:
      rejected.append('%s : identite hote en defaut' % name)
  for name, ok in sorted(evidence.get('fixtures_identity', {}).items()):
    if ok is not True:
      rejected.append('fixture %s : identite en defaut' % name)
  if not evidence.get('fixtures_identity'):
    refused.append('identite des fixtures absente')
  kills = evidence.get('host_kills', {})
  for m in MUTANTS:
    if not kills.get(m):
      refused.append('mutant %s jamais tue sur l hote' % m)
  dkills = evidence.get('device_kills', {})
  for fx, ms in FIXTURE_KILLS.items():
    for m in ms:
      if fx not in kills.get(m, []):
        refused.append('mutant %s non tue par la fixture %s sur l hote' % (m, fx))
      if fx not in dkills.get(m, []):
        refused.append('mutant %s non tue par la fixture %s sur l appareil' % (m, fx))
  rng = random.Random(SEED)
  stats = {}
  rounds = evidence.get('rounds', {})
  for name in cases:
    runs = rounds.get(name, [])
    decides = name in deciding
    valid, gpu_ok = [], True
    for r in runs:
      if not finite_positive([r.get('v11_ms'), r.get('gpu_ms'), r.get('gpu_resident_ms')]):
        refused.append('%s : duree non finie ou non positive' % name)
        continue
      if r.get('timed_allocations', 1) != 0:
        refused.append('%s : reservation pendant les passes chronometrees' % name)
        continue
      if r.get('gpu_identity') is not True:
        gpu_ok = False
      valid.append(r)
    if name not in device:
      refused.append('%s : identite appareil absente' % name)
    elif device[name] is not True or not gpu_ok:
      rejected.append('%s : identite appareil en defaut' % name)
    if len(valid) < need:
      refused.append('%s : %d tours valides sur %d exiges' % (name, len(valid), need))
      continue
    logs = [math.log(r['gpu_ms'] / r['v11_ms']) for r in valid]
    lo, hi = bootstrap_ci(logs, rng)
    gm = math.exp(sum(logs) / len(logs))
    res_logs = [math.log(r['gpu_resident_ms'] / r['v11_ms']) for r in valid]
    stats[name] = {'ratio_gm': gm, 'ci95': [lo, hi], 'rounds': len(valid), 'deciding': decides,
                   'ratios': [math.exp(x) for x in logs], 'resident_ratio_gm': math.exp(sum(res_logs) / len(res_logs)),
                   'gpu_ms_median': median([r['gpu_ms'] for r in valid]),
                   'v11_ms_median': median([r['v11_ms'] for r in valid])}
    if not (math.isfinite(hi) and hi > 0):
      refused.append('%s : intervalle non fini' % name)
    elif decides and hi > THRESHOLD:
      rejected.append('%s : borne haute %.4f > 1/4' % (name, hi))
  if refused:
    verdict = 'refuse'
  elif rejected:
    verdict = 'rejete'
  else:
    verdict = 'adopte'
  return verdict, sorted(set(refused)), rejected, stats


def selftest_judge():
  """Injections : le juge doit rendre le verdict attendu dans chaque scenario (sans outil, sans donnee)."""
  cases = ['ng00_k5_l16', 'ng00_k5_l24', 'ng00_k10_l24']
  deciding = ['ng00_k5_l24', 'ng00_k10_l24']

  def base():
    return {'cases': list(cases), 'deciding': list(deciding), 'processes': 5, 'refusals': [], 'isolation': True,
            'unit_ok': True, 'fixtures_ok': True, 'sanitizer': {'memcheck': 0, 'racecheck': 0, 'synccheck': 0},
            'host_identity': {c: True for c in cases}, 'device_identity': {c: True for c in cases},
            'fixtures_identity': {'coquille48_u32_k5_l24': True},
            'host_kills': {m: (['coquille48_u32_k5_l24'] if m == 'repere_enfant' else ['ng00_k5_l24'])
                           for m in MUTANTS},
            'device_kills': {'repere_enfant': ['coquille48_u32_k5_l24']},
            'rounds': {c: [{'v11_ms': 60.0, 'gpu_ms': 6.0 + 0.1 * i, 'gpu_resident_ms': 5.0, 'timed_allocations': 0,
                            'gpu_identity': True} for i in range(5)] for c in cases}}

  scenarios = []

  def expect(label, evidence, verdict):
    got = judge(evidence)[0]
    scenarios.append({'scenario': label, 'expected': verdict, 'got': got, 'ok': got == verdict})

  expect('valide', base(), 'adopte')
  e = base()
  e['host_identity']['ng00_k5_l16'] = False
  expect('identite hote en defaut sur un cas publie', e, 'rejete')
  e = base()
  e['rounds']['ng00_k10_l24'][2]['gpu_identity'] = False
  expect('identite appareil en defaut dans un tour', e, 'rejete')
  e = base()
  del e['rounds']['ng00_k5_l24'][4]
  expect('tour manquant sur un cas qui decide', e, 'refuse')
  e = base()
  e['refusals'] = ['ng00_k5_l24_p2 : resultat perime (vidage different de la commande)']
  expect('resultat perime', e, 'refuse')
  e = base()
  e['deciding'] = []
  expect('aucun cas qui decide', e, 'refuse')
  e = base()
  e['rounds']['ng00_k5_l24'][1]['gpu_ms'] = float('nan')
  expect('duree NaN', e, 'refuse')
  e = base()
  e['rounds']['ng00_k5_l24'][1]['v11_ms'] = 0.0
  expect('duree nulle', e, 'refuse')
  e = base()
  for r in e['rounds']['ng00_k10_l24']:
    r['gpu_ms'] = 20.0
  expect('borne haute au-dessus de 1/4', e, 'rejete')
  e = base()
  e['host_kills']['temoin_perdu'] = []
  expect('mutant jamais tue', e, 'refuse')
  e = base()
  e['device_kills'] = {}
  expect('mutant du repere non tue sur l appareil', e, 'refuse')
  e = base()
  e['rounds']['ng00_k5_l16'][0]['timed_allocations'] = 3
  expect('reservation pendant le chrono', e, 'refuse')
  e = base()
  e['sanitizer']['racecheck'] = 9
  expect('sanitizer en echec', e, 'refuse')
  e = base()
  e['isolation'] = False
  expect('isolation GPU non certifiee', e, 'refuse')
  e = base()
  del e['host_identity']['ng00_k5_l16']
  expect('identite hote absente', e, 'refuse')
  e = base()
  for r in e['rounds']['ng00_k5_l16']:
    r['gpu_ms'] = 30.0
  expect('cas publie (non decisif) au-dessus du seuil', e, 'adopte')
  ok = all(s['ok'] for s in scenarios)
  print(json.dumps({'selftest_judge': 'conforme' if ok else 'ECHEC', 'scenarios': scenarios}, indent=1))
  return 0 if ok else 1


# --------------------------------------------------------------------------------------------------- execution
def parse_configs(text):
  out = []
  for c in text.split(','):
    k, leaf = c.split(':')
    out.append((int(k), int(leaf)))
  return out


def case_name(frame, k, leaf):
  return '%s_k%d_l%d' % (frame, k, leaf)


def read_json(path):
  try:
    with open(path, encoding='utf-8') as f:
      return json.load(f)
  except (OSError, ValueError):
    return None


def last_json_line(text):
  out = None
  for line in text.splitlines():
    line = line.strip()
    if line.startswith('{'):
      try:
        out = json.loads(line)
      except ValueError:
        pass
  return out


def run_parallel(s, commands, jobs, timeout):
  """commands : [(nom, argv)] ; rend {nom: (code, stdout)} ; un processus par commande, au plus jobs a la fois."""
  pending, running, results = list(commands), [], {}
  while pending or running:
    while pending and len(running) < jobs:
      name, argv = pending.pop(0)
      log = open(s.logs / (name + '.log'), 'wb')
      log.write(('$ ' + ' '.join(str(a) for a in argv) + '\n').encode())
      p = subprocess.Popen([str(a) for a in argv], stdout=subprocess.PIPE, stderr=log)
      running.append((name, p, log, time.time()))
    still = []
    for name, p, log, t0 in running:
      if p.poll() is None and time.time() - t0 < timeout:
        still.append((name, p, log, t0))
        continue
      if p.poll() is None:
        p.kill()
        p.wait()
      stdout = p.stdout.read().decode(errors='replace')
      log.write(b'\n--- stdout ---\n' + stdout.encode())
      log.close()
      results[name] = (p.returncode, stdout)
      s.steps.append({'step': name, 'code': p.returncode, 'seconds': round(time.time() - t0, 3)})
    running = still
    time.sleep(0.1)
  return results


def main():
  ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  ap.add_argument('--out')
  ap.add_argument('--work')
  ap.add_argument('--data', help='dossier des trames lidar_ngXX.u32le / .ids.u32le (hors depot)')
  ap.add_argument('--repo', help='racine contenant morsehgp3D_v11 (sources gelees)')
  ap.add_argument('--v11-lib', help='libmhgp11.a deja construite (profil u21) ; sinon construite depuis --repo')
  ap.add_argument('--m2', help='dossier du microbanc MES-M2 (par defaut le dossier frere mes_m2_feuille)')
  ap.add_argument('--frames', default='ng00,ng01,ng02')
  ap.add_argument('--configs', default='5:16,5:24,10:24')
  ap.add_argument('--extra', default='uniform_u18_n8000,uniform_u18_n16000,uniform_u18_n32000',
                  help='entrees publiees en plus (<data>/<nom>.u32le), tailles d interet de CLAUDE.md ; absentes : sautees')
  ap.add_argument('--extra-configs', default='5:24')
  ap.add_argument('--decide', default='5:24,10:24')
  ap.add_argument('--processes', type=int, default=5)
  ap.add_argument('--reps', type=int, default=15)
  ap.add_argument('--warmup', type=int, default=3)
  ap.add_argument('--v11-workers', type=int, default=48)
  ap.add_argument('--v11-passes', type=int, default=10)
  ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 8) - 2))
  ap.add_argument('--crop', type=int, default=4000)
  ap.add_argument('--no-cuda', action='store_true', help='outils hote seulement (essai local sans GPU)')
  ap.add_argument('--skip-sanitizer', action='store_true')
  ap.add_argument('--nvcc')
  ap.add_argument('--cmake', default=shutil.which('cmake') or 'cmake')
  ap.add_argument('--selftest-judge', action='store_true')
  args = ap.parse_args()
  if args.selftest_judge:
    return selftest_judge()
  if not args.out or not args.work or not args.data:
    print('--out, --work et --data requis', file=sys.stderr)
    return 2
  out, work = Path(args.out).resolve(), Path(args.work).resolve()
  if work == out or out in work.parents:
    print('--work ne peut pas etre dans --out (les vidages ne sont jamais rapatries)', file=sys.stderr)
    return 2
  m2_dir = Path(args.m2).resolve() if args.m2 else HERE.parent / 'mes_m2_feuille'
  m2 = load_m2(m2_dir)
  if m2 is None or not (m2_dir / 'include' / 'mhgp12' / 'leaf' / 'simt.hpp').is_file():
    print('microbanc MES-M2 introuvable : --m2', file=sys.stderr)
    return 2
  repo = Path(args.repo).resolve() if args.repo else m2.find_repo(HERE)
  if repo is None or not (repo / 'morsehgp3D_v11' / 'src' / 'catalogue' / 'boxes.cpp').is_file():
    print('depot introuvable : --repo (racine contenant morsehgp3D_v11)', file=sys.stderr)
    return 2
  frames = [f for f in args.frames.split(',') if f]
  configs, decide = parse_configs(args.configs), parse_configs(args.decide)
  data = Path(args.data)
  # Entrees : trames (lidar_<f>) qui decident selon --decide, puis entrees publiees (--extra) si presentes.
  inputs = {f: (data / ('lidar_%s.u32le' % f), data / ('lidar_%s.ids.u32le' % f)) for f in frames}
  extras = [e for e in args.extra.split(',') if e]
  extra_configs = parse_configs(args.extra_configs) if extras else []
  present_extras = [e for e in extras if (data / (e + '.u32le')).is_file() and (data / (e + '.ids.u32le')).is_file()]
  for e in present_extras:
    inputs[e] = (data / (e + '.u32le'), data / (e + '.ids.u32le'))
  frame_of = {case_name(f, k, l): (f, k, l) for f in frames for k, l in configs}
  frame_of.update({case_name(e, k, l): (e, k, l) for e in present_extras for k, l in extra_configs})
  cases = list(frame_of)
  deciding = [case_name(f, k, l) for f in frames for k, l in configs if (k, l) in decide]
  jobs = max(1, args.jobs)

  s = m2.Session(out)
  work.mkdir(parents=True, exist_ok=True)
  report = {'bench': 'MES-M5 parcours des boites en largeur sur GPU (v12, hors produit)', 'started_utc': m2.now(),
            'frame': {'phase': 'exploration_v12_hors_registre', 'backend': 'cuda_g4 (microbanc)',
                      'quantification': 'quantized_u21_input_only', 'public_status': 'not_claimed'},
            'rule': {'threshold': THRESHOLD, 'metric': 'total : copie du nuage + tous les niveaux + rapatriement des '
                     'feuilles, contre frontiere + passe unique de la v11 (W fils, passes chaudes), meme machine',
                     'statistic': 'par cas : rapport des medianes par tour (GPU / v11), moyenne geometrique, IC 95 % '
                     'bootstrap sur les tours (10 000 tirages, graine 20261007)',
                     'adopt': 'identite (hote, appareil, fixtures) partout, mutants tues, sanitizer propre, et borne '
                     'haute <= 1/4 sur chaque cas qui decide', 'deciding': deciding, 'processes_required': args.processes},
            'args': vars(args), 'repo': str(repo), 'microbench_dir': str(HERE), 'm2_dir': str(m2_dir),
            'cases': cases, 'extras_absent': [e for e in extras if e not in present_extras]}
  nvcc = None if args.no_cuda else m2.find_nvcc(args.nvcc)
  report['environment'] = m2.environment(s, nvcc, args.cmake)
  gpu_apps = report['environment'].get('gpu_apps')
  isolation = isinstance(gpu_apps, str) and gpu_apps.strip() == ''
  report['gpu_isolation'] = isolation
  report['data'] = {}
  for name, pair in inputs.items():
    for p in pair:
      report['data'][p.name] = m2.sha256_file(p) if p.is_file() else None
      if not p.is_file():
        s.refusals.append('entree absente : ' + p.name)

  # 2. Constructions.
  v11_lib = Path(args.v11_lib).resolve() if args.v11_lib else m2.build_v11(s, repo, work, args.cmake, jobs)
  if v11_lib is None or not Path(v11_lib).is_file():
    s.refusals.append('bibliotheque v11 absente ou construction en echec')
    v11_lib = None
  bdir = work / 'b_m5'
  cmd = [args.cmake, '-S', HERE, '-B', bdir, '-DCMAKE_BUILD_TYPE=Release', '-DMHGP12_M2_DIR=' + str(m2_dir),
         '-DMHGP11_SOURCE_DIR=' + str(repo / 'morsehgp3D_v11')]
  if v11_lib:
    cmd.append('-DMHGP11_LIBRARY=' + str(v11_lib))
  if nvcc:
    cmd += ['-DMHGP12_PARCOURS_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc]
  code, _, _ = s.run('m5_configure', cmd, 900)
  if code == 0:
    code, _, _ = s.run('m5_build', [args.cmake, '--build', bdir, '-j', str(jobs)], 3600)
  if code != 0:
    s.refusals.append('construction du microbanc en echec')
  tools = {name: bdir / name for name in ('mhgp12_traversal_dump', 'mhgp12_traversal_identity',
                                          'mhgp12_v11_traversal_timing', 'mhgp12_traversal_bench')}
  hashes = {}
  for name, p in tools.items():
    if p.is_file():
      hashes['bin/' + name] = m2.sha256_file(p)
    elif name != 'mhgp12_traversal_bench' or nvcc:
      s.refusals.append('binaire absent : ' + name)
  if v11_lib:
    hashes['bin/libmhgp11.a'] = m2.sha256_file(v11_lib)
  for p in sorted(HERE.rglob('*')):
    if p.is_file() and p.suffix in ('.hpp', '.cpp', '.cu', '.py', '.txt', '.md') and '__pycache__' not in p.parts:
      hashes['m5/' + str(p.relative_to(HERE))] = m2.sha256_file(p)
  for rel in ('include/mhgp12/leaf/simt.hpp', 'include/mhgp12/leaf/dump_format.hpp', 'scripts/g4_leaf_bench.py'):
    hashes['m2/' + rel] = m2.sha256_file(m2_dir / rel)
  for rel in ('src/catalogue/boxes.cpp', 'src/catalogue/internal.hpp', 'src/catalogue/adaptive_frontier.cpp',
              'src/catalogue/single_pass.cpp', 'src/catalogue/frontier_dispatch.hpp'):
    p = repo / 'morsehgp3D_v11' / rel
    hashes['v11/' + rel] = m2.sha256_file(p) if p.is_file() else None
  report['hashes'] = hashes

  # 3. Vidages, decoupe, fixtures (dans --work).
  dumps = work / 'dumps'
  dumps.mkdir(exist_ok=True)
  for old in dumps.glob('*.bin'):
    old.unlink()
  dump_tool = tools['mhgp12_traversal_dump']
  cmds = []
  for name, (frame, k, leaf) in frame_of.items():
    xyz, ids = inputs[frame]
    cmds.append(('dump_' + name, [dump_tool, xyz, ids, k, leaf, dumps / (name + '.bin')]))
  crop_name = '%s_k5_l24_crop%d' % (frames[0], args.crop)
  cmds.append(('dump_' + crop_name, [dump_tool, inputs[frames[0]][0], inputs[frames[0]][1], 5, 24,
                                     dumps / (crop_name + '.bin'), '--crop', args.crop]))
  made = run_parallel(s, cmds, jobs, 1800) if dump_tool.is_file() else {}
  report['dumps'] = {}
  for name, (code, stdout) in sorted(made.items()):
    report['dumps'][name[5:]] = {'code': code, 'summary': last_json_line(stdout)}
  case_dumps = {}
  for name in cases + [crop_name]:
    p = dumps / (name + '.bin')
    if p.is_file() and made.get('dump_' + name, (1, ''))[0] == 0:
      case_dumps[name] = p
      report['dumps'].setdefault(name, {})['sha256'] = m2.sha256_file(p)
    else:
      s.refusals.append('vidage absent ou en echec : ' + name)
  fixtures = work / 'fixtures'
  code, stdout, _ = s.run('fixtures', [sys.executable, '-S', HERE / 'fixtures' / 'fixtures.py', '--out', fixtures,
                                       '--dump-tool', dump_tool], 1800)
  manifest = read_json(fixtures / 'fixtures.json')
  report['fixtures'] = manifest
  fixtures_ok = code == 0 and manifest is not None and all(e.get('present') and e.get('controls_ok')
                                                            for e in manifest.get('fixtures', []))
  fixture_dumps = {}
  if manifest:
    for e in manifest.get('fixtures', []):
      p = fixtures / (e['name'] + '.bin')
      if e.get('present') and p.is_file():
        fixture_dumps[e['name']] = p

  # 4. Identite hote : tous les vidages, noeuds, mutants, portes unitaires.
  host_identity, host_kills, fixtures_identity = {}, {m: [] for m in MUTANTS}, {}
  unit_ok = False
  identity_tool = tools['mhgp12_traversal_identity']
  if identity_tool.is_file():
    target = out / 'identity_host.json'
    if target.exists():
      target.unlink()
    all_dumps = [case_dumps[c] for c in cases + [crop_name] if c in case_dumps] + list(fixture_dumps.values())
    code, _, _ = s.run('identity_host', [identity_tool, '--mutants', 'all', '--nodes', '--unit', '--threads',
                                         min(jobs, 64), '--json', target] + all_dumps, 7200)
    lines = []
    if target.is_file():
      lines = [json.loads(l) for l in target.read_text().splitlines() if l.strip()]
    report['identity_host'] = {'code': code, 'results': lines}
    by_path = {}
    for line in lines:
      if line.get('phase') == 'unit':
        unit_ok = line.get('ok') is True
      elif line.get('phase') == 'identity':
        by_path[line.get('dump')] = line
    for name, p in list(case_dumps.items()) + list(fixture_dumps.items()):
      line = by_path.get(str(p))
      if line is None:
        continue
      ok = line.get('identity') is True and (line.get('nodes_equal') is True or line.get('status') != 0)
      if name in fixture_dumps:
        fixtures_identity[name] = ok
      elif name in cases:
        host_identity[name] = ok
      elif name == crop_name and not ok:
        s.refusals.append('identite hote en defaut sur la decoupe ' + crop_name)
      for m, v in line.get('mutants', {}).items():
        if v.get('killed') is True and m in host_kills:
          host_kills[m].append(name)
    for m in REAL_KILLS:
      if not any(c in deciding for c in host_kills[m]):
        s.refusals.append('mutant %s tue par aucune trame' % m)
  else:
    s.refusals.append('outil d identite hote absent')

  # 5. Sanitizer et identite appareil des fixtures (mutants sur l'appareil).
  bench = tools['mhgp12_traversal_bench']
  sanitizer_codes, device_kills = {}, {}
  if nvcc and bench.is_file():
    targets = [case_dumps[crop_name]] if crop_name in case_dumps else []
    targets += [fixture_dumps[f] for f in SANITIZER_FIXTURES if f in fixture_dumps]
    tool = Path(nvcc).parent / 'compute-sanitizer'
    sanitizer = tool if tool.is_file() else shutil.which('compute-sanitizer')
    if not args.skip_sanitizer and sanitizer and targets:
      report['sanitizer'] = {}
      for t in ('memcheck', 'racecheck', 'synccheck'):
        argv = [sanitizer, '--tool', t, '--error-exitcode', '9', bench]
        for p in targets:
          argv += ['--dump', p]
        argv += ['--reps', '1', '--warmup', '0', '--no-profile', '--mutants', 'none', '--json',
                 out / ('sanitizer_%s.json' % t)]
        code, o2, e2 = s.run('sanitizer_' + t, argv, 3600)
        sanitizer_codes[t] = code
        report['sanitizer'][t] = {'code': code, 'tail': (o2 + e2)[-600:]}
    target = out / 'device_fixtures.json'
    if target.exists():
      target.unlink()
    argv = [bench]
    for p in fixture_dumps.values():
      argv += ['--dump', p]
    argv += ['--reps', '1', '--warmup', '0', '--no-profile', '--mutants', 'all', '--json', target]
    code, _, _ = s.run('device_fixtures', argv, 1800)
    result = read_json(target)
    report['device_fixtures'] = {'code': code, 'result': result}
    if result:
      for c in result.get('cases', []):
        name = Path(c.get('dump', '')).stem
        if name in fixtures_identity:
          fixtures_identity[name] = fixtures_identity[name] and c.get('identity') is True
        for m, v in c.get('mutants', {}).items():
          if v.get('killed') is True:
            device_kills.setdefault(m, []).append(name)
    for name in fixture_dumps:
      if result is None or name not in [Path(c.get('dump', '')).stem for c in result.get('cases', [])]:
        fixtures_identity[name] = False
  elif not args.no_cuda:
    s.refusals.append('banc CUDA absent (nvcc introuvable ou construction en echec)')

  # 6. Tours : chrono v11 puis banc GPU, cas en ordre tournant ; le tour 0 est jete.
  rounds, device_identity = {c: [] for c in cases}, {}
  runs_dir = out / 'runs'
  runs_dir.mkdir(exist_ok=True)
  timing = tools['mhgp12_v11_traversal_timing']
  present = [c for c in cases if c in case_dumps]
  for p in range(args.processes + 1):
    order = present[p % len(present):] + present[:p % len(present)] if present else []
    for name in order:
      frame, k, leaf = frame_of[name]
      v11_target = runs_dir / ('%s_p%d_v11.json' % (name, p))
      gpu_target = runs_dir / ('%s_p%d_gpu.json' % (name, p))
      for t in (v11_target, gpu_target):
        if t.exists():
          t.unlink()
      entry = {'round': p}
      if timing.is_file():
        code, stdout, _ = s.run('v11_%s_p%d' % (name, p),
                                [timing, inputs[frame][0], inputs[frame][1], k, leaf, args.v11_workers,
                                 args.v11_passes, '--walk-reps', 0 if p else 3], 1800)
        v = last_json_line(stdout)
        if code == 0 and v and v.get('kmax') == k and v.get('leaf_size') == leaf:
          v11_target.write_text(json.dumps(v))
          warm = v.get('traversal_ns', [])[1:] or v.get('traversal_ns', [])
          entry['v11_ms'] = median(warm) / 1e6 if warm else float('nan')
          summary = report['dumps'].get(name, {}).get('summary') or {}
          entry['v11_ledger_ok'] = (v.get('catalogue_nodes') == summary.get('nodes') and
                                    v.get('catalogue_filter_tests') == summary.get('filter_tests'))
          if not entry['v11_ledger_ok']:
            s.refusals.append('%s tour %d : grand livre du chrono v11 different du vidage' % (name, p))
        else:
          s.refusals.append('%s tour %d : chrono v11 en echec ou incoherent (code %d)' % (name, p, code))
      if nvcc and bench.is_file():
        code, _, _ = s.run('gpu_%s_p%d' % (name, p), [bench, '--dump', case_dumps[name], '--reps', args.reps,
                                                       '--warmup', args.warmup, '--mutants', 'none', '--json',
                                                       gpu_target], 1800)
        g = read_json(gpu_target) if gpu_target.is_file() else None
        c = (g or {}).get('cases', [{}])[0] if g else {}
        same = c.get('dump') == str(case_dumps[name]) and c.get('kmax') == k and c.get('leaf_size') == leaf
        if code in (0, 1) and g and same and len(c.get('total_ms', [])) == args.reps:
          entry['gpu_ms'] = c['median_ms']['total']
          entry['gpu_resident_ms'] = c['median_ms']['resident']
          entry['gpu_kernels_ms'] = c.get('profile', {}).get('kernels_ms')
          entry['timed_allocations'] = c.get('timed_allocations')
          entry['gpu_identity'] = c.get('identity') is True
          device_identity[name] = device_identity.get(name, True) and entry['gpu_identity']
        else:
          s.refusals.append('%s tour %d : banc GPU en echec, perime ou incomplet (code %d)' % (name, p, code))
      if p > 0:
        rounds[name].append(entry)
  report['rounds'] = rounds
  for name in cases:
    if name not in device_identity and nvcc:
      device_identity[name] = False

  # Binaires inchanges depuis le debut de la session (lien execution / binaire).
  for name, p in tools.items():
    key = 'bin/' + name
    if key in hashes and (not p.is_file() or m2.sha256_file(p) != hashes[key]):
      s.refusals.append('binaire modifie pendant la session : ' + name)
  evidence = {'cases': cases, 'deciding': deciding, 'processes': args.processes, 'refusals': list(s.refusals),
              'isolation': isolation, 'unit_ok': unit_ok, 'fixtures_ok': fixtures_ok,
              'sanitizer': sanitizer_codes if not args.skip_sanitizer else {},
              'host_identity': host_identity, 'device_identity': device_identity,
              'fixtures_identity': fixtures_identity, 'host_kills': host_kills, 'device_kills': device_kills,
              'rounds': rounds}
  verdict, refused, rejected, stats = judge(evidence)
  report['evidence'] = {k: v for k, v in evidence.items() if k != 'rounds'}
  report['verdict'] = verdict
  report['refused'] = refused
  report['rejected'] = rejected
  report['stats'] = stats
  report['steps'] = s.steps
  report['finished_utc'] = m2.now()
  (out / 'report.json').write_text(json.dumps(report, indent=1, sort_keys=True))
  print(json.dumps({'report': str(out / 'report.json'), 'verdict': verdict, 'refused': refused[:10],
                    'rejected': rejected[:10],
                    'ratios': {n: round(v['ratio_gm'], 4) for n, v in stats.items()}}))
  return 0


if __name__ == '__main__':
  sys.exit(main())
