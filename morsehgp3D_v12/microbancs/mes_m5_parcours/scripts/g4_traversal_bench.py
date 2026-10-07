#!/usr/bin/env python3
"""MES-M5 sur G4 : parcours des boites du catalogue en largeur sur le GPU (microbanc hors produit de la v12).

Une seule entree pour la session : construit, vide le parcours de la v11, verifie l'identite (hote, puis appareil),
tue les mutants, passe Compute Sanitizer, mesure le parcours GPU et la meme etape de la v11 sur la meme machine, juge,
puis ecrit un rapport JSON. Bibliotheque standard seulement (Python 3.10 nu de la VM) ; aucune commande GCP.

Dossiers : --out recoit le rapport, les journaux et les resultats legers (a rapatrier) ; --work recoit constructions,
vidages et fixtures (derives de SemanticKITTI pour les vidages : JAMAIS rapatries). --work ne peut pas etre dans --out.

Etapes (journal par etape dans <out>/logs/) :
  1. environnement (nvcc, cmake, compilateur, nvidia-smi, uptime, processus GPU) ; jeton de session (--nonce des
     outils) ; ecarts au contrat ecrit d'avance (publies, et refus de l'adoption) ;
  2. bibliotheque v11 gelee (--v11-lib, ou construite depuis --repo comme MES-M2), microbanc (hote + CUDA sm_120) ;
     empreintes des binaires, des sources du microbanc, des en-tetes de MES-M2, des sources v11 du parcours et des
     dependances effectivement compilees (fichiers .d) ;
  3. porte du lecteur strict MHGP12TR (mhgp12_traversal_format_selftest, CST-0223) ;
  4. vidages du parcours v11 des trames (--data) en parallele (cibles effacees avant), chacun rattache a son cas par
     son en-tete (profil, K, feuille, sites, statut, grand livre) ; decoupe pour Compute Sanitizer ; fixtures
     synthetiques (dossier efface avant ; reference v11 recontrolee par l'oracle Python pour u21, oracle pour u32) ;
  5. identite hote (warp simule) de tous les vidages, noeuds compris, tous les mutants, portes unitaires ;
  6. Compute Sanitizer (memcheck, racecheck, synccheck) sur la decoupe et deux fixtures u32 ; identite appareil des
     fixtures et mutants sur l'appareil ;
  7. 1 + P tours : par tour et par cas (ordre tournant), un processus de chrono v11 (frontiere + passe unique, W fils,
     passes chaudes) puis un processus du banc GPU (echauffement, R passes, profil, verification, sans mutant) ; le
     premier tour est jete (MESURE.md § 5) ; isolation du GPU relevee avant et apres les tours ;
  8. binaires, sources, dependances compilees et vidages rehaches ; juge (regle ecrite d'avance, README.md § 6) et
     rapport <out>/report.json.

Juge (README.md § 6, regle inchangee ; preuves exigees par CST-0018 de l'auditeur Codex, sur le modele de MES-M2
durci) : « adopte » seulement si TOUTES les preuves sont presentes et fraiches : contrat complet (les neuf cas ng00..02
a K5/16, K5/24, K10/24, les six qui decident, au moins cinq tours, 15 passes GPU apres 3 d'echauffement, dix passes v11
a 48 fils, decoupe de 4 000 sites, CUDA et sanitizers joues) ; outils d'identite hote et appareil de code 0, avec une
ligne valide par cas et par fixture, les six fixtures exigees ; chaque tour de chaque cas valide : chrono v11 aux dix
passes exigees (passes 2 a 10), banc GPU aux 15 durees brutes dont les medianes RECALCULEES egalent les medianes
declarees, aucune reservation chronometree ; sanitizers de code 0 avec leur prise sur les trois cibles ; mutants tues ;
portes unitaires et porte du lecteur conformes ; isolation du GPU au debut, avant et apres les tours ; chaque prise
fraiche (cible effacee avant, jeton de la session) et rattachee a son vidage ; binaires, sources, dependances et
vidages rehaches egaux. Une preuve manquante, perimee ou incoherente rend « refuse », jamais « adopte » ni « rejete ».
« rejete » : identite en defaut (code 1 coherent) ou borne haute > 1/4 sur un cas qui decide. Statistique inchangee :
memes tirages ; les medianes recalculees sont celles que le banc a imprimees (a 6 chiffres significatifs pres).

Usage :
  python3 g4_traversal_bench.py --out OUT --work WORK --data DONNEES [--repo DEPOT] [--v11-lib LIB]
          [--frames ng00,ng01,ng02] [--configs 5:16,5:24,10:24] [--decide 5:24,10:24] [--processes 5]
          [--reps 15] [--warmup 3] [--v11-workers 48] [--v11-passes 10] [--jobs N] [--no-cuda] [--nvcc NVCC]
          [--cmake CMAKE] [--crop 4000] [--skip-sanitizer]
  python3 g4_traversal_bench.py --selftest-judge     (auto-test du juge par injections, sans outil ni donnee)
  python3 g4_traversal_bench.py --rejudge DOSSIER [--expect-published]
      rejuge les sorties publiees d'une session (report.json, runs/, identity_host.json, device_fixtures.json,
      sanitizer_*.json) par le juge durci. Sans jeton dans le rapport (session anterieure au durcissement), la
      fraicheur par jeton, la porte du lecteur, l'isolation avant et apres les tours et le rehachage final des sources
      n'existent pas : ils sont declares non rejouables, jamais supposes.
Les options qui s'ecartent du contrat restent permises pour l'exploration : le rapport publie les mesures, le verdict
est « refuse ».
Codes : 0 rapport ecrit (quel que soit le verdict), auto-test conforme, ou rejugement fait (avec --expect-published :
verdict et statistiques egaux aux publies) ; 2 refus avant toute mesure (arguments, depot ou microbanc MES-M2
introuvable, --work dans --out, dossier a rejuger illisible) ; 1 auto-test en echec ou rejugement different du publie.
"""

import argparse
import importlib.util
import json
import math
import os
import random
import re
import shutil
import struct
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
# Mutants que les trames LiDAR doivent tuer (chacun sur au moins un cas qui decide) ; repere_enfant est equivalent au
# profil u21 (voie native partout, s <= 22) : il doit etre tue par la fixture u32 de la coquille.
REAL_KILLS = ['temoin_perdu', 'compactage_instable', 'bissection_decalee', 'ex_aequo_inverses', 'axe_dernier_maximum']
FIXTURE_KILLS = {'coquille48_u32_k5_l24': ['repere_enfant']}
SANITIZER_FIXTURES = ['coquille48_u32_k5_l24', 'uniforme_u32_k3_l8']
SANITIZERS = ('memcheck', 'racecheck', 'synccheck')
# Contrat ecrit avant la mesure (README.md § 6) : grille, cas qui decident, tours, passes, decoupe, fixtures.
CONTRACT = {'frames': ('ng00', 'ng01', 'ng02'), 'configs': ((5, 16), (5, 24), (10, 24)), 'decide': ((5, 24), (10, 24)),
            'processes_min': 5, 'reps': 15, 'warmup': 3, 'v11_passes': 10, 'v11_workers': 48, 'crop': 4000}
FIXTURES = ('coquille24_k2_l16', 'coquille48_k5_l24', 'coquille48_k5_l8_m8', 'coquille48_u32_k5_l24', 'bord_u32_k2_l5',
            'uniforme_u32_k3_l8')
PROFILE_BITS = 21  # profil des vidages de la v11 (bibliotheque u21)
MEDIAN_RTOL = 2e-5  # le banc imprime ses durees et medianes a 6 chiffres significatifs
REQUIRED_BINARIES = ('mhgp12_traversal_dump', 'mhgp12_traversal_identity', 'mhgp12_v11_traversal_timing',
                     'mhgp12_traversal_format_selftest')
HEADER = struct.Struct('<8s16Q15Q')  # en-tete MHGP12TR v1 (format.hpp, 256 octets)
HEADER_FIELDS = ('version', 'producer', 'coord_bits', 'kmax', 'leaf_size', 'max_leaf', 'n_sites', 'n_nodes',
                 'n_leaves', 'n_leaf_sites', 'status', 'nodes', 'leaves', 'filter_tests', 'max_depth', 'max_leaf_seen')
FORMAT_GATE_MIN = {'valid': 4, 'invalid': 15}  # cas de la porte du lecteur (host/format_selftest.cpp)


def load_m2(m2_dir):
  """Outils de session de MES-M2 (Session, empreintes, environnement, nvcc, isolation, construction de la v11)."""
  path = Path(m2_dir) / 'scripts' / 'g4_leaf_bench.py'
  if not path.is_file():
    return None
  spec = importlib.util.spec_from_file_location('mhgp12_m2_session', str(path))
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


# ------------------------------------------------------------------------------------------------------------- juge
def is_int(x):
  return isinstance(x, int) and not isinstance(x, bool)


def is_real(x):
  return isinstance(x, (int, float)) and not isinstance(x, bool) and math.isfinite(x)


def finite_positive(values):
  return bool(values) and all(is_real(v) and v > 0 for v in values)


def median(v):
  v = sorted(v)
  n = len(v)
  if n == 0:
    return float('nan')
  return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def same_median(declared, computed):
  """Mediane declaree par le banc egale a la mediane recalculee depuis ses durees brutes (impression a 6 chiffres)."""
  return is_real(declared) and is_real(computed) and \
      abs(declared - computed) <= MEDIAN_RTOL * max(abs(declared), abs(computed))


def bootstrap_ci(logs, rng):
  boot = []
  for _ in range(BOOTSTRAP):
    sample = [logs[rng.randrange(len(logs))] for _ in logs]
    boot.append(sum(sample) / len(sample))
  boot.sort()
  return math.exp(boot[int(0.025 * BOOTSTRAP)]), math.exp(boot[int(0.975 * BOOTSTRAP) - 1])


def case_name(frame, k, leaf):
  return '%s_k%d_l%d' % (frame, k, leaf)


def contract_cases():
  return [case_name(f, k, l) for f in CONTRACT['frames'] for k, l in CONTRACT['configs']]


def contract_deciding():
  return [case_name(f, k, l) for f in CONTRACT['frames'] for k, l in CONTRACT['configs'] if (k, l) in CONTRACT['decide']]


def round_values(r, reps, passes):
  """Entree d'un tour (durees BRUTES) : valeurs recalculees, ou (None, raison). v11 : mediane des passes 2..N de
  frontiere + passe unique ; GPU : medianes de total et resident recalculees et egales aux medianes declarees."""
  if not isinstance(r, dict):
    return None, 'entree illisible'
  ns = r.get('v11_traversal_ns')
  if r.get('v11_passes') != passes or not isinstance(ns, list) or len(ns) != passes or passes < 2:
    return None, 'chrono v11 : %s passe(s) declaree(s), %s duree(s), %s exigees' % (
        r.get('v11_passes'), len(ns) if isinstance(ns, list) else 0, passes)
  if not all(is_int(v) and v > 0 for v in ns):
    return None, 'chrono v11 : duree non entiere ou non positive'
  if r.get('v11_ledger_ok') is not True:
    return None, 'grand livre du chrono v11 different du vidage'
  total, resident = r.get('gpu_total_ms'), r.get('gpu_resident_ms')
  for label, values in (('total', total), ('resident', resident)):
    if not isinstance(values, list) or len(values) != reps or not finite_positive(values):
      return None, 'banc GPU : durees %s absentes, non finies, non positives ou en nombre faux (%s sur %s)' % (
          label, len(values) if isinstance(values, list) else 0, reps)
  declared = r.get('gpu_median_ms') if isinstance(r.get('gpu_median_ms'), dict) else {}
  gpu_ms, resident_ms = median(total), median(resident)
  if not same_median(declared.get('total'), gpu_ms) or not same_median(declared.get('resident'), resident_ms):
    return None, 'mediane declaree contraire aux durees brutes (declaree %r, recalculee %r)' % (
        declared.get('total'), gpu_ms)
  if r.get('timed_allocations') != 0:
    return None, 'reservation pendant les passes chronometrees'
  if not isinstance(r.get('gpu_identity'), bool):
    return None, 'identite du banc illisible'
  return {'round': r.get('round'), 'v11_ms': median(ns[1:]) / 1e6, 'gpu_ms': gpu_ms, 'gpu_resident_ms': resident_ms,
          'gpu_identity': r['gpu_identity']}, None


def tool_code(code, failures, label, refused):
  """Code d'un outil d'identite : 0 sans defaut, 1 coherent avec un defaut constate ; tout le reste refuse."""
  if code == 0 and failures:
    refused.append('%s : code 0 malgre une identite en defaut (incoherent)' % label)
  elif code == 1 and not failures:
    refused.append('%s : code 1 sans identite en defaut (incoherent)' % label)
  elif code not in (0, 1):
    refused.append('%s : code %r (refus ou echec de l outil, jamais une preuve)' % (label, code))


def judge(evidence):
  """Regle ecrite d'avance (README.md § 6). evidence : dictionnaire pur (aucune E/S).

  Cles : cases (noms joues, dans l'ordre des tirages), deciding, processes (tours exiges), reps, v11_passes, refusals
  (refus du collecteur : fraicheur, jeton, provenance, contrat), isolation, unit_ok, fixtures_ok, fixture_names,
  sanitizer {outil: code}, sanitizer_proof {outil: bool}, host_code, host_identity {cas: bool}, fixtures_host
  {fixture: bool}, device_fixtures_code, fixtures_device {fixture: bool}, host_kills {mutant: [noms]}, device_kills
  {mutant: [fixtures]}, rounds {cas: [entrees aux durees brutes, voir round_values]}.
  Rend (verdict, raisons de refus, raisons de rejet, statistiques par cas)."""
  refused = list(evidence.get('refusals', []))
  rejected = []
  cases = list(evidence.get('cases', []))
  deciding = list(evidence.get('deciding', []))
  need, reps, passes = evidence.get('processes'), evidence.get('reps'), evidence.get('v11_passes')
  # 1. Contrat : grille complete, cas qui decident, tours, passes.
  missing = [c for c in contract_cases() if c not in cases]
  if missing:
    refused.append('cas du contrat absents : %s' % ', '.join(missing))
  if sorted(deciding) != sorted(contract_deciding()) or any(c not in cases for c in deciding):
    refused.append('cas qui decident %s : le contrat fixe %s' % (','.join(deciding), ','.join(contract_deciding())))
  if len(set(cases)) != len(cases):
    refused.append('cas repetes dans le manifeste')
  if not is_int(need) or need < CONTRACT['processes_min']:
    refused.append('%r tours exiges : le contrat en exige au moins %d' % (need, CONTRACT['processes_min']))
  if reps != CONTRACT['reps'] or passes != CONTRACT['v11_passes']:
    refused.append('passes GPU %r et v11 %r : le contrat fixe %d et %d' % (reps, passes, CONTRACT['reps'],
                                                                          CONTRACT['v11_passes']))
  # 2. Preuves globales.
  if evidence.get('isolation') is not True:
    refused.append('isolation GPU non certifiee')
  if evidence.get('unit_ok') is not True:
    refused.append('portes unitaires en echec ou absentes')
  if evidence.get('fixtures_ok') is not True:
    refused.append('fixtures ou controles de l oracle en echec ou absents')
  if sorted(evidence.get('fixture_names', [])) != sorted(FIXTURES):
    refused.append('fixtures %s : le contrat exige %s' % (','.join(evidence.get('fixture_names', [])),
                                                          ','.join(FIXTURES)))
  sanitizer, proof = evidence.get('sanitizer', {}), evidence.get('sanitizer_proof', {})
  for tool in SANITIZERS:
    if sanitizer.get(tool) != 0:
      refused.append('compute-sanitizer %s : %s' % (tool, sanitizer.get(tool, 'non joue')))
    elif proof.get(tool) is not True:
      refused.append('compute-sanitizer %s : prise absente, perimee ou incomplete' % tool)
  # 3. Identite hote (cas et fixtures) et code de l'outil.
  host, fx_host = evidence.get('host_identity', {}), evidence.get('fixtures_host', {})
  tool_code(evidence.get('host_code'), [n for n, ok in list(host.items()) + list(fx_host.items()) if ok is False],
            'outil d identite hote', refused)
  for name in cases:
    if name not in host:
      refused.append('%s : identite hote absente' % name)
    elif host[name] is not True:
      rejected.append('%s : identite hote en defaut' % name)
  for name in FIXTURES:
    if name not in fx_host:
      refused.append('fixture %s : identite hote absente' % name)
    elif fx_host[name] is not True:
      rejected.append('fixture %s : identite hote en defaut' % name)
  # 4. Identite appareil des fixtures et code de l'outil.
  fx_device = evidence.get('fixtures_device', {})
  tool_code(evidence.get('device_fixtures_code'), [n for n, ok in fx_device.items() if ok is False],
            'banc des fixtures sur l appareil', refused)
  for name in FIXTURES:
    if name not in fx_device:
      refused.append('fixture %s : identite appareil absente' % name)
    elif fx_device[name] is not True:
      rejected.append('fixture %s : identite appareil en defaut' % name)
  # 5. Mutants.
  kills, dkills = evidence.get('host_kills', {}), evidence.get('device_kills', {})
  for m in MUTANTS:
    if not kills.get(m):
      refused.append('mutant %s jamais tue sur l hote' % m)
  for m in REAL_KILLS:
    if not any(c in deciding for c in kills.get(m, [])):
      refused.append('mutant %s tue par aucune trame qui decide' % m)
  for fx, ms in FIXTURE_KILLS.items():
    for m in ms:
      if fx not in kills.get(m, []):
        refused.append('mutant %s non tue par la fixture %s sur l hote' % (m, fx))
      if fx not in dkills.get(m, []):
        refused.append('mutant %s non tue par la fixture %s sur l appareil' % (m, fx))
  # 6. Tours : durees brutes validees, medianes recalculees ; statistique inchangee (memes tirages, meme ordre).
  rng = random.Random(SEED)
  stats = {}
  rounds = evidence.get('rounds', {})
  for name in cases:
    decides = name in deciding
    valid, gpu_ok, seen = [], True, set()
    for r in rounds.get(name, []):
      take, why = round_values(r, reps, passes)
      if take is None:
        refused.append('%s tour %s : %s' % (name, r.get('round') if isinstance(r, dict) else '?', why))
        continue
      if take['round'] in seen:
        refused.append('%s : tour %s repete' % (name, take['round']))
        continue
      seen.add(take['round'])
      gpu_ok = gpu_ok and take['gpu_identity']
      valid.append(take)
    if not gpu_ok:
      rejected.append('%s : identite appareil en defaut' % name)
    if not is_int(need) or len(valid) < need:
      refused.append('%s : %d tours valides sur %s exiges' % (name, len(valid), need))
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


def synthetic_round(p, gpu=6.0, v11_ns=60000000, passes=10, reps=15):
  return {'round': p, 'v11_passes': passes, 'v11_traversal_ns': [v11_ns] * passes, 'v11_ledger_ok': True,
          'gpu_total_ms': [gpu] * reps, 'gpu_resident_ms': [5.0] * reps, 'gpu_median_ms': {'total': gpu, 'resident': 5.0},
          'timed_allocations': 0, 'gpu_identity': True}


def selftest_judge():
  """Injections : le juge doit rendre le verdict attendu dans chaque scenario (sans outil, sans donnee)."""
  cases = contract_cases()
  deciding = contract_deciding()

  def base():
    return {'cases': list(cases), 'deciding': list(deciding), 'processes': 5, 'reps': 15, 'v11_passes': 10,
            'refusals': [], 'isolation': True, 'unit_ok': True, 'fixtures_ok': True, 'fixture_names': list(FIXTURES),
            'sanitizer': {t: 0 for t in SANITIZERS}, 'sanitizer_proof': {t: True for t in SANITIZERS},
            'host_code': 0, 'host_identity': {c: True for c in cases}, 'fixtures_host': {f: True for f in FIXTURES},
            'device_fixtures_code': 0, 'fixtures_device': {f: True for f in FIXTURES},
            'host_kills': {m: (['coquille48_u32_k5_l24'] if m == 'repere_enfant' else ['ng00_k5_l24'])
                           for m in MUTANTS},
            'device_kills': {'repere_enfant': ['coquille48_u32_k5_l24']},
            'rounds': {c: [synthetic_round(i + 1, gpu=6.0 + 0.1 * i) for i in range(5)] for c in cases}}

  scenarios = []

  def expect(label, evidence, verdict):
    got = judge(evidence)[0]
    scenarios.append({'scenario': label, 'expected': verdict, 'got': got, 'ok': got == verdict})

  expect('valide', base(), 'adopte')
  e = base()
  e['host_identity']['ng00_k5_l16'] = False
  e['host_code'] = 1
  expect('identite hote en defaut sur un cas publie (code 1 coherent)', e, 'rejete')
  e = base()
  e['rounds']['ng00_k10_l24'][2]['gpu_identity'] = False
  expect('identite appareil en defaut dans un tour', e, 'rejete')
  e = base()
  del e['rounds']['ng00_k5_l24'][4]
  expect('tour manquant sur un cas qui decide', e, 'refuse')
  e = base()
  e['refusals'] = ['ng00_k5_l24 tour 2 : prise perimee (jeton different)']
  expect('resultat perime', e, 'refuse')
  e = base()
  e['deciding'] = []
  expect('aucun cas qui decide', e, 'refuse')
  e = base()
  e['rounds']['ng00_k5_l24'][1]['gpu_total_ms'][3] = float('nan')
  expect('duree NaN', e, 'refuse')
  e = base()
  e['rounds']['ng00_k5_l24'][1]['v11_traversal_ns'][4] = 0
  expect('duree nulle', e, 'refuse')
  e = base()
  for i, r in enumerate(e['rounds']['ng00_k10_l24']):
    e['rounds']['ng00_k10_l24'][i] = synthetic_round(r['round'], gpu=20.0)
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
  for i, r in enumerate(e['rounds']['ng00_k5_l16']):
    e['rounds']['ng00_k5_l16'][i] = synthetic_round(r['round'], gpu=30.0)
  expect('cas publie (non decisif) au-dessus du seuil', e, 'adopte')
  # Injections de l'auditeur Codex (recu audit_b_m5_20261007/juge_format), au niveau du juge.
  e = base()
  e.update(cases=['ng00_k5_l24'], deciding=['ng00_k5_l24'], processes=1)
  e['rounds'] = {'ng00_k5_l24': [synthetic_round(1)]}
  expect('auditeur : une trame, un cas K5/24, un processus', e, 'refuse')
  e = base()
  e['fixture_names'] = ['coquille48_u32_k5_l24']
  for d in (e['fixtures_host'], e['fixtures_device']):
    for f in FIXTURES:
      if f != 'coquille48_u32_k5_l24':
        del d[f]
  expect('auditeur : une seule des six fixtures', e, 'refuse')
  e = base()
  e['host_code'] = 2
  expect('auditeur : outil d identite hote de code 2', e, 'refuse')
  e = base()
  e['device_fixtures_code'] = 7
  expect('auditeur : banc des fixtures de code 7', e, 'refuse')
  e = base()
  for c in cases:
    e['rounds'][c] = [synthetic_round(i + 1, passes=1) for i in range(5)]
  expect('auditeur : une seule passe v11 (froide)', e, 'refuse')
  e = base()
  for c in cases:
    for r in e['rounds'][c]:
      r['gpu_total_ms'] = [60.0] * 15
      r['gpu_median_ms']['total'] = 6.0
  expect('auditeur : mediane GPU declaree contraire aux durees brutes', e, 'refuse')
  e = base()
  for c in cases:
    e['rounds'][c] = [synthetic_round(i + 1, gpu=60.0) for i in range(5)]
  expect('auditeur (controle) : banc lent et honnete', e, 'rejete')
  # Autres preuves exigees.
  e = base()
  e['sanitizer_proof']['synccheck'] = False
  expect('sanitizer de code 0 sans prise', e, 'refuse')
  e = base()
  e['host_identity']['ng01_k5_l24'] = False
  expect('outil d identite hote de code 0 malgre un defaut', e, 'refuse')
  e = base()
  del e['fixtures_device']['bord_u32_k2_l5']
  expect('fixture absente sur l appareil', e, 'refuse')
  e = base()
  e['rounds']['ng02_k5_l24'][3]['gpu_total_ms'] = [6.0] * 14
  expect('passes GPU en nombre faux', e, 'refuse')
  e = base()
  e['host_kills']['axe_dernier_maximum'] = ['coquille48_k5_l24', 'ng00_k5_l16']
  expect('mutant LiDAR tue seulement hors des cas qui decident', e, 'refuse')
  e = base()
  e['reps'] = 10
  for c in cases:
    e['rounds'][c] = [dict(synthetic_round(i + 1, reps=10)) for i in range(5)]
  expect('passes GPU hors contrat', e, 'refuse')
  e = base()
  e['rounds']['ng01_k10_l24'][0]['v11_ledger_ok'] = False
  expect('grand livre du chrono v11 different du vidage', e, 'refuse')
  e = base()
  e['rounds']['ng00_k5_l24'][4]['round'] = 1
  expect('tour compte deux fois', e, 'refuse')
  ok = all(s['ok'] for s in scenarios)
  print(json.dumps({'selftest_judge': 'conforme' if ok else 'ECHEC', 'scenarios': scenarios}, indent=1))
  return 0 if ok else 1


# ------------------------------------------------------------------------------------------- preuves et prises
def parse_configs(text):
  out = []
  for c in text.split(','):
    if not c:
      continue
    k, leaf = c.split(':')
    out.append((int(k), int(leaf)))
  return out


def read_json(path):
  try:
    with open(path, encoding='utf-8') as f:
      return json.load(f)
  except (OSError, ValueError):
    return None


def jsonl(text):
  out = []
  for line in text.splitlines():
    line = line.strip()
    if line.startswith('{'):
      try:
        out.append(json.loads(line))
      except ValueError:
        pass
  return out


def last_json_line(text):
  rows = jsonl(text)
  return rows[-1] if rows else None


def dump_identity(path):
  """Identite d'un vidage MHGP12TR lue en une passe : sha256, taille, champs de l'en-tete, FNV-1a finale. None si
  illisible."""
  import hashlib
  h = hashlib.sha256()
  size, head, tail = 0, b'', b''
  try:
    with open(path, 'rb') as f:
      for block in iter(lambda: f.read(1 << 20), b''):
        if len(head) < HEADER.size:
          head += block[:HEADER.size - len(head)]
        tail = (tail + block)[-8:]
        size += len(block)
        h.update(block)
  except OSError:
    return None
  if len(head) < HEADER.size or len(tail) < 8:
    return None
  values = HEADER.unpack(head)
  out = {'sha256': h.hexdigest(), 'octets': size, 'magic': values[0].decode('ascii', 'replace'),
         'fnv1a': '%016x' % int.from_bytes(tail, 'little')}
  out.update(dict(zip(HEADER_FIELDS, values[1:1 + len(HEADER_FIELDS)])))
  return out


def identity_line(line, ident, nonce):
  """Ligne d'identite hote d'un vidage : (identite bool, None), ou (None, raison) si la ligne n'est pas une preuve."""
  if not isinstance(line, dict):
    return None, 'ligne illisible'
  if nonce is not None and line.get('nonce') != nonce:
    return None, 'jeton absent ou different (ligne perimee)'
  for key, ref in (('kmax', 'kmax'), ('leaf_size', 'leaf_size'), ('coord_bits', 'coord_bits'), ('sites', 'n_sites')):
    if line.get(key) != ident.get(ref):
      return None, '%s different du vidage' % key
  identity, mutants = line.get('identity'), line.get('mutants')
  if not isinstance(identity, bool):
    return None, 'identite illisible'
  if not isinstance(mutants, dict) or sorted(mutants) != sorted(MUTANTS) or \
          not all(isinstance(v, dict) and isinstance(v.get('killed'), bool) for v in mutants.values()):
    return None, 'mutants absents ou illisibles'
  if line.get('reference_status') != ident.get('status'):
    return None, 'statut de reference different du vidage'
  if identity:
    if line.get('status') != line.get('reference_status') or line.get('ledger') != line.get('reference_ledger'):
      return None, 'identite declaree avec statut ou grand livre differents'
    if line.get('reference_status') == 0 and not (line.get('nodes_checked') is True and line.get('nodes_equal') is True):
      return None, 'identite declaree sans comparaison des noeuds'
  return identity, None


def bench_case(c, path, ident):
  """Cas d'une prise du banc CUDA rattache a son vidage : (identite bool, None) ou (None, raison)."""
  if not isinstance(c, dict) or c.get('dump') != str(path):
    return None, 'prise d un autre vidage (chemin)'
  for key, ref in (('kmax', 'kmax'), ('leaf_size', 'leaf_size'), ('coord_bits', 'coord_bits'), ('sites', 'n_sites')):
    if c.get(key) != ident.get(ref):
      return None, '%s different du vidage' % key
  if not isinstance(c.get('identity'), bool):
    return None, 'identite illisible'
  return c['identity'], None


def bench_json(g, nonce, reps, warmup, paths):
  """Prise du banc CUDA : fichier de ce banc, jeton de la session, repetitions de la commande, exactement les vidages
  demandes, dans l'ordre. Rend (cas, None) ou (None, raison)."""
  if not isinstance(g, dict) or g.get('bench') != 'mhgp12_traversal_bench':
    return None, 'prise absente, illisible ou d un autre banc'
  if nonce is not None and g.get('nonce') != nonce:
    return None, 'jeton absent ou different (prise perimee)'
  if g.get('reps') != reps or g.get('warmup') != warmup:
    return None, 'repetitions ou echauffement differents de la commande'
  cases = g.get('cases')
  if not isinstance(cases, list) or [c.get('dump') if isinstance(c, dict) else None for c in cases] != \
          [str(p) for p in paths]:
    return None, 'vidages de la prise differents de la commande'
  if not isinstance(g.get('identity'), bool):
    return None, 'identite globale illisible'
  return cases, None


def v11_take(v, k, leaf, workers, ident):
  """Chrono v11 d'un tour (sortie standard du processus) : (valeurs brutes, None) ou (None, raison)."""
  if not isinstance(v, dict) or v.get('phase') != 'v11_traversal_timing':
    return None, 'sortie absente ou illisible'
  if v.get('kmax') != k or v.get('leaf_size') != leaf or v.get('workers') != workers:
    return None, 'K, feuille ou fils differents de la commande'
  if v.get('sites') != ident.get('n_sites'):
    return None, 'sites differents du vidage'
  ns, prefix, single = v.get('traversal_ns'), v.get('prefix_ns'), v.get('single_pass_ns')
  if not all(isinstance(x, list) for x in (ns, prefix, single)) or not len(ns) == len(prefix) == len(single) or \
          any(a != b + c for a, b, c in zip(ns, prefix, single)):
    return None, 'durees incoherentes (frontiere + passe unique)'
  return {'v11_passes': v.get('passes'), 'v11_traversal_ns': ns,
          'v11_ledger_ok': v.get('catalogue_nodes') == ident.get('nodes') and
          v.get('catalogue_filter_tests') == ident.get('filter_tests')}, None


def gpu_take(g, path, ident, nonce, reps, warmup):
  """Prise du banc GPU d'un tour : (valeurs brutes, None) ou (None, raison)."""
  cases, why = bench_json(g, nonce, reps, warmup, [path])
  if cases is None:
    return None, why
  identity, why = bench_case(cases[0], path, ident)
  if identity is None:
    return None, why
  c = cases[0]
  med = c.get('median_ms') if isinstance(c.get('median_ms'), dict) else {}
  profile = c.get('profile') if isinstance(c.get('profile'), dict) else {}
  return {'gpu_total_ms': c.get('total_ms'), 'gpu_resident_ms': c.get('resident_ms'),
          'gpu_median_ms': {'total': med.get('total'), 'resident': med.get('resident')},
          'timed_allocations': c.get('timed_allocations'), 'gpu_identity': identity,
          'gpu_kernels_ms': profile.get('kernels_ms')}, None


def format_gate(code, out):
  """Porte du lecteur strict : code 0, chaque cas a son attendu, assez de valides admis et d'invalides refuses."""
  rows = [r for r in jsonl(out) if 'cas' in r]
  valid = [r for r in rows if r.get('attendu') is True]
  invalid = [r for r in rows if r.get('attendu') is False]
  ok = code == 0 and rows and all(r.get('admis') == r.get('attendu') for r in rows) and \
      len(valid) >= FORMAT_GATE_MIN['valid'] and len(invalid) >= FORMAT_GATE_MIN['invalid']
  return bool(ok), {'code': code, 'cases': len(rows), 'valid_admitted': sum(r.get('admis') is True for r in valid),
                    'invalid_refused': sum(r.get('admis') is False for r in invalid), 'conform': bool(ok)}


def fixtures_manifest(manifest, nonce):
  """Manifeste des fixtures : les six exigees, presentes, controles conformes, jeton de la session."""
  if not isinstance(manifest, dict) or not isinstance(manifest.get('fixtures'), list):
    return False, [], 'manifeste absent ou illisible'
  entries = [e for e in manifest['fixtures'] if isinstance(e, dict)]
  names = [e.get('name') for e in entries]
  if nonce is not None and manifest.get('nonce') != nonce:
    return False, names, 'jeton absent ou different (manifeste perime)'
  if sorted(names) != sorted(FIXTURES):
    return False, names, 'fixtures %s, attendues %s' % (','.join(str(n) for n in names), ','.join(FIXTURES))
  if not all(e.get('present') is True and e.get('controls_ok') is True for e in entries):
    return False, names, 'fixture absente ou controle de l oracle en echec'
  return True, names, None


def contract_deviations(frames, configs, decide, processes, reps, warmup, v11_passes, v11_workers, crop,
                        skip_sanitizer, no_cuda):
  out = []
  if list(frames) != list(CONTRACT['frames']):
    out.append('trames %s : le contrat fixe %s' % (','.join(frames), ','.join(CONTRACT['frames'])))
  if sorted(set(configs)) != sorted(CONTRACT['configs']):
    out.append('configurations %s : le contrat fixe 5:16,5:24,10:24' % ','.join('%d:%d' % c for c in configs))
  if sorted(set(decide)) != sorted(CONTRACT['decide']):
    out.append('cas qui decident %s : le contrat fixe 5:24,10:24' % ','.join('%d:%d' % c for c in decide))
  if processes < CONTRACT['processes_min']:
    out.append('%d tours : le contrat en exige au moins %d' % (processes, CONTRACT['processes_min']))
  for label, got, want in (('passes GPU', reps, 'reps'), ('echauffement GPU', warmup, 'warmup'),
                           ('passes v11', v11_passes, 'v11_passes'), ('fils v11', v11_workers, 'v11_workers'),
                           ('decoupe', crop, 'crop')):
    if got != CONTRACT[want]:
      out.append('%s %r : le contrat fixe %r' % (label, got, CONTRACT[want]))
  if skip_sanitizer:
    out.append('sanitizers non joues (--skip-sanitizer)')
  if no_cuda:
    out.append('banc CUDA non joue (--no-cuda)')
  return out


# --------------------------------------------------------------------------------------------------- execution
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


def clear(target):
  """Efface une cible avant sa commande : aucune prise perimee ne survit. Faux si l'effacement echoue."""
  try:
    if target.is_dir() and not target.is_symlink():
      shutil.rmtree(target)
    elif target.exists() or target.is_symlink():
      target.unlink()
    return True
  except OSError:
    return False


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
  ap.add_argument('--rejudge', help='dossier de sorties publiees d une session a rejuger')
  ap.add_argument('--expect-published', action='store_true')
  args = ap.parse_args()
  if args.selftest_judge:
    return selftest_judge()
  if args.rejudge:
    return rejudge(Path(args.rejudge), args.expect_published)
  if not args.out or not args.work or not args.data:
    print('--out, --work et --data requis', file=sys.stderr)
    return 2
  try:
    frames = [f for f in args.frames.split(',') if f]
    configs, decide = parse_configs(args.configs), parse_configs(args.decide)
    extras = [e for e in args.extra.split(',') if e]
    extra_configs = parse_configs(args.extra_configs) if extras else []
  except ValueError:
    print('--configs, --decide, --extra-configs : K:feuille separes par des virgules', file=sys.stderr)
    return 2
  if not frames or not configs or args.processes < 1 or args.reps < 1 or args.warmup < 0 or args.v11_passes < 2 or \
          args.v11_workers < 1 or args.crop < 1:
    print('arguments invalides', file=sys.stderr)
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
  data = Path(args.data)
  # Entrees : trames (lidar_<f>) qui decident selon --decide, puis entrees publiees (--extra) si presentes.
  inputs = {f: (data / ('lidar_%s.u32le' % f), data / ('lidar_%s.ids.u32le' % f)) for f in frames}
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
  nonce = 'm5-%s-%s' % (time.strftime('%Y%m%dT%H%M%SZ', time.gmtime()), os.urandom(6).hex())
  deviations = contract_deviations(frames, configs, decide, args.processes, args.reps, args.warmup, args.v11_passes,
                                   args.v11_workers, args.crop, args.skip_sanitizer, args.no_cuda)
  for reason in deviations:
    s.refuse('hors contrat : ' + reason)
  report = {'bench': 'MES-M5 parcours des boites en largeur sur GPU (v12, hors produit)', 'started_utc': m2.now(),
            'nonce': nonce,
            'frame': {'phase': 'exploration_v12_hors_registre', 'backend': 'cuda_g4 (microbanc)',
                      'quantification': 'quantized_u21_input_only', 'public_status': 'not_claimed'},
            'rule': {'threshold': THRESHOLD, 'metric': 'total : copie du nuage + tous les niveaux + rapatriement des '
                     'feuilles, contre frontiere + passe unique de la v11 (W fils, passes chaudes), meme machine',
                     'statistic': 'par cas : rapport des medianes par tour (GPU / v11), moyenne geometrique, IC 95 % '
                     'bootstrap sur les tours (10 000 tirages, graine 20261007) ; medianes recalculees depuis les '
                     'durees brutes', 'adopt': 'identite (hote, appareil, fixtures) partout, mutants tues, sanitizer '
                     'propre, toutes les preuves presentes et fraiches (CST-0018), et borne haute <= 1/4 sur chaque '
                     'cas qui decide', 'deciding': deciding, 'processes_required': args.processes,
                     'contract': {'frames': list(CONTRACT['frames']),
                                  'configs': ['%d:%d' % c for c in CONTRACT['configs']],
                                  'decide': ['%d:%d' % c for c in CONTRACT['decide']],
                                  'processes_min': CONTRACT['processes_min'], 'reps': CONTRACT['reps'],
                                  'warmup': CONTRACT['warmup'], 'v11_passes': CONTRACT['v11_passes'],
                                  'v11_workers': CONTRACT['v11_workers'], 'crop': CONTRACT['crop'],
                                  'fixtures': list(FIXTURES), 'sanitizers': list(SANITIZERS)}},
            'contract_deviations': deviations, 'args': vars(args), 'repo': str(repo), 'microbench_dir': str(HERE),
            'm2_dir': str(m2_dir), 'cases': cases, 'extras_absent': [e for e in extras if e not in present_extras]}
  nvcc = None if args.no_cuda else m2.find_nvcc(args.nvcc)
  report['environment'] = m2.environment(s, nvcc, args.cmake)
  gpu_apps = report['environment'].get('gpu_apps')
  isolation_start = isinstance(gpu_apps, str) and gpu_apps.strip() == ''
  report['data'] = {}
  for name, pair in inputs.items():
    for p in pair:
      report['data'][p.name] = m2.sha256_file(p) if p.is_file() else None
      if not p.is_file():
        s.refuse('entree absente : ' + p.name)

  # 2. Constructions et empreintes (rehachees en fin de session).
  v11_lib = Path(args.v11_lib).resolve() if args.v11_lib else m2.build_v11(s, repo, work, args.cmake, jobs)
  if v11_lib is None or not Path(v11_lib).is_file():
    s.refuse('bibliotheque v11 absente ou construction en echec')
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
    s.refuse('construction du microbanc en echec')
  tools = {name: bdir / name for name in REQUIRED_BINARIES + ('mhgp12_traversal_bench',)}

  def binary_hashes():
    found = {'bin/' + n: m2.sha256_file(p) for n, p in tools.items() if p.is_file()}
    if v11_lib and Path(v11_lib).is_file():
      found['bin/libmhgp11.a'] = m2.sha256_file(v11_lib)
    return found

  out_dir = s.out.resolve()

  def source_hashes():
    found = {}
    for p in sorted(HERE.rglob('*')):
      if p.is_file() and p.suffix in ('.hpp', '.cpp', '.cu', '.py', '.txt', '.md') and '__pycache__' not in p.parts \
              and out_dir not in p.resolve().parents and work not in p.resolve().parents:
        found['m5/' + str(p.relative_to(HERE))] = m2.sha256_file(p)
    for rel in ('include/mhgp12/leaf/simt.hpp', 'include/mhgp12/leaf/dump_format.hpp', 'scripts/g4_leaf_bench.py'):
      found['m2/' + rel] = m2.sha256_file(m2_dir / rel) if (m2_dir / rel).is_file() else None
    for rel in ('src/catalogue/boxes.cpp', 'src/catalogue/internal.hpp', 'src/catalogue/adaptive_frontier.cpp',
                'src/catalogue/single_pass.cpp', 'src/catalogue/frontier_dispatch.hpp'):
      p = repo / 'morsehgp3D_v11' / rel
      found['v11/' + rel] = m2.sha256_file(p) if p.is_file() else None
    return found

  binaries, sources = binary_hashes(), source_hashes()
  hashes = dict(binaries)
  hashes.update(sources)
  report['hashes'] = hashes
  dep_roots = [HERE, m2_dir, repo / 'morsehgp3D_v11']
  dependencies = m2.compiled_dependencies(bdir, dep_roots) if bdir.is_dir() else {}
  report['compiled_dependencies'] = {'roots': ['0: microbanc M5', '1: microbanc M2', '2: morsehgp3D_v11'],
                                     'files': dependencies}
  for name in REQUIRED_BINARIES:
    if 'bin/' + name not in binaries:
      s.refuse('binaire absent ou non hache : ' + name)
  if nvcc and 'bin/mhgp12_traversal_bench' not in binaries:
    s.refuse('binaire absent ou non hache : mhgp12_traversal_bench')

  # 3. Porte du lecteur strict (CST-0223), binaire de cette session.
  format_ok = False
  selftest = tools['mhgp12_traversal_format_selftest']
  if selftest.is_file():
    folder = work / 'format_selftest'
    if clear(folder):
      folder.mkdir(parents=True)
      code, stdout, _ = s.run('format_selftest', [selftest, folder], 600)
      format_ok, report['format_selftest'] = format_gate(code, stdout)
      clear(folder)
  if not format_ok:
    s.refuse('porte du lecteur strict MHGP12TR absente ou en echec')

  # 4. Vidages (cibles effacees avant, code 0 exige, en-tete rattache au cas), decoupe, fixtures.
  dumps = work / 'dumps'
  dumps.mkdir(exist_ok=True)
  for old in dumps.glob('*.bin'):
    if not clear(old):
      s.refuse('vidage perime non effacable : ' + old.name)
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
  case_dumps, idents = {}, {}
  for name in cases + [crop_name]:
    p = dumps / (name + '.bin')
    frame, k, leaf = frame_of[name] if name in frame_of else (frames[0], 5, 24)
    xyz = inputs[frame][0]
    frame_sites = xyz.stat().st_size // 12 if xyz.is_file() else None
    sites = min(args.crop, frame_sites) if name == crop_name and frame_sites is not None else frame_sites
    ident = dump_identity(p) if p.is_file() and made.get('dump_' + name, (1, ''))[0] == 0 else None
    summary = report['dumps'].get(name, {}).get('summary') or {}
    if ident is None:
      s.refuse('vidage absent ou en echec : ' + name)
      continue
    if ident['magic'] != 'MHGP12TR' or ident['version'] != 1 or ident['coord_bits'] != PROFILE_BITS or \
            ident['kmax'] != k or ident['leaf_size'] != leaf or ident['status'] != 0 or ident['n_sites'] != sites:
      s.refuse('vidage %s : en-tete (profil, K, feuille, statut, sites) different du cas' % name)
      continue
    if summary.get('nodes') != ident['nodes'] or summary.get('filter_tests') != ident['filter_tests']:
      s.refuse('vidage %s : sortie de l outil differente de l en-tete' % name)
      continue
    case_dumps[name] = p
    idents[name] = ident
    report['dumps'][name]['sha256'] = ident['sha256']
  fixtures_dir = work / 'fixtures'
  fixture_dumps, fixtures_ok, fixture_names = {}, False, []
  if clear(fixtures_dir) and dump_tool.is_file():
    code, stdout, _ = s.run('fixtures', [sys.executable, '-S', HERE / 'fixtures' / 'fixtures.py', '--out', fixtures_dir,
                                         '--dump-tool', dump_tool, '--nonce', nonce], 1800)
    manifest = read_json(fixtures_dir / 'fixtures.json')
    report['fixtures'] = manifest
    complete, fixture_names, why = fixtures_manifest(manifest, nonce)
    fixtures_ok = code == 0 and complete
    if not fixtures_ok:
      s.refuse('fixtures : code %s, %s' % (code, why or 'manifeste conforme mais outil en echec'))
    for e in (manifest or {}).get('fixtures', []) if complete else []:
      p = fixtures_dir / (e['name'] + '.bin')
      ident = dump_identity(p) if p.is_file() else None
      if ident is None or ident['sha256'] != e.get('dump_sha256') or ident['kmax'] != e.get('kmax') or \
              ident['leaf_size'] != e.get('leaf') or ident['coord_bits'] != e.get('bits') or \
              ident['n_sites'] != e.get('sites'):
        s.refuse('fixture %s : vidage absent ou different de son manifeste' % e['name'])
        continue
      fixture_dumps[e['name']] = p
      idents[e['name']] = ident
  else:
    s.refuse('fixtures non jouees (dossier perime non effacable ou outil de vidage absent)')
  report['dump_identity'] = {name: {k: v for k, v in ident.items() if k != 'magic'} for name, ident in idents.items()}

  # 5. Identite hote : tous les vidages, noeuds, mutants, portes unitaires ; une ligne fraiche par vidage.
  host_identity, fixtures_host, host_kills = {}, {}, {m: [] for m in MUTANTS}
  unit_ok, host_code = False, None
  identity_tool = tools['mhgp12_traversal_identity']
  target = out / 'identity_host.json'
  all_dumps = [(c, case_dumps[c]) for c in cases + [crop_name] if c in case_dumps] + list(fixture_dumps.items())
  if identity_tool.is_file() and clear(target):
    host_code, _, _ = s.run('identity_host', [identity_tool, '--mutants', 'all', '--nodes', '--unit', '--threads',
                                              min(jobs, 64), '--json', target, '--nonce', nonce] +
                            [p for _, p in all_dumps], 7200)
    lines = jsonl(target.read_text()) if target.is_file() else []
    report['identity_host'] = {'code': host_code, 'results': lines}
    units = [l for l in lines if l.get('phase') == 'unit']
    unit_ok = len(units) == 1 and units[0].get('nonce') == nonce and units[0].get('ok') is True and \
        units[0].get('emit_tasks_u64') is True
    by_path = {}
    for line in lines:
      if line.get('phase') == 'identity':
        by_path.setdefault(line.get('dump'), []).append(line)
    for name, p in all_dumps:
      found = by_path.get(str(p), [])
      if len(found) != 1:
        s.refuse('identite hote : %d ligne(s) pour %s' % (len(found), name))
        continue
      ok, why = identity_line(found[0], idents[name], nonce)
      if ok is None:
        s.refuse('identite hote %s : %s' % (name, why))
        continue
      if name in fixture_dumps:
        fixtures_host[name] = ok
      elif name in cases:
        host_identity[name] = ok
      elif not ok:
        s.refuse('identite hote en defaut sur la decoupe ' + crop_name)
      for m, v in found[0]['mutants'].items():
        if v['killed'] is True:
          host_kills[m].append(name)
  else:
    s.refuse('outil d identite hote absent ou cible perimee non effacable')

  # 6. Sanitizer et identite appareil des fixtures (mutants sur l'appareil) ; prises fraiches.
  bench = tools['mhgp12_traversal_bench']
  sanitizer_codes, sanitizer_proof, device_kills, fixtures_device, device_code = {}, {}, {}, {}, None
  report['sanitizer'] = {}
  if nvcc and bench.is_file():
    targets = [case_dumps[crop_name]] if crop_name in case_dumps else []
    targets += [fixture_dumps[f] for f in SANITIZER_FIXTURES if f in fixture_dumps]
    tool = Path(nvcc).parent / 'compute-sanitizer'
    sanitizer = tool if tool.is_file() else shutil.which('compute-sanitizer')
    if args.skip_sanitizer:
      pass
    elif not sanitizer:
      s.refuse('compute-sanitizer introuvable')
    elif len(targets) != 1 + len(SANITIZER_FIXTURES):
      s.refuse('compute-sanitizer : cibles incompletes (decoupe et fixtures u32 exigees)')
    else:
      for t in SANITIZERS:
        json_target = out / ('sanitizer_%s.json' % t)
        if not clear(json_target):
          s.refuse('sanitizer %s : prise perimee non effacable' % t)
          continue
        argv = [sanitizer, '--tool', t, '--error-exitcode', '9', bench]
        for p in targets:
          argv += ['--dump', p]
        argv += ['--reps', '1', '--warmup', '0', '--no-profile', '--mutants', 'none', '--json', json_target,
                 '--nonce', nonce]
        code, o2, e2 = s.run('sanitizer_' + t, argv, 3600)
        sanitizer_codes[t] = code
        taken, why = bench_json(read_json(json_target), nonce, 1, 0, targets)
        names = [crop_name] + [f for f in SANITIZER_FIXTURES]
        proof = taken is not None and all(bench_case(c, p, idents[n])[0] is True
                                          for c, p, n in zip(taken, targets, names))
        sanitizer_proof[t] = proof
        report['sanitizer'][t] = {'code': code, 'tail': (o2 + e2)[-600:], 'proof': proof, 'reason': why}
    target = out / 'device_fixtures.json'
    if clear(target) and fixture_dumps:
      argv = [bench]
      for p in fixture_dumps.values():
        argv += ['--dump', p]
      argv += ['--reps', '1', '--warmup', '0', '--no-profile', '--mutants', 'all', '--json', target, '--nonce', nonce]
      device_code, _, _ = s.run('device_fixtures', argv, 1800)
      result = read_json(target)
      report['device_fixtures'] = {'code': device_code, 'result': result}
      taken, why = bench_json(result, nonce, 1, 0, list(fixture_dumps.values()))
      if taken is None:
        s.refuse('fixtures sur l appareil : %s' % why)
      else:
        for c, (name, p) in zip(taken, fixture_dumps.items()):
          ok, why = bench_case(c, p, idents[name])
          mutants = c.get('mutants') if isinstance(c.get('mutants'), dict) else {}
          if ok is None or sorted(mutants) != sorted(MUTANTS):
            s.refuse('fixture %s sur l appareil : %s' % (name, why or 'mutants absents'))
            continue
          fixtures_device[name] = ok
          for m, v in mutants.items():
            if isinstance(v, dict) and v.get('killed') is True:
              device_kills.setdefault(m, []).append(name)
    else:
      s.refuse('fixtures sur l appareil non jouees')
  elif not args.no_cuda:
    s.refuse('banc CUDA absent (nvcc introuvable ou construction en echec)')

  # 7. Tours : chrono v11 puis banc GPU, cas en ordre tournant ; le tour 0 est jete ; prises fraiches et rattachees.
  rounds = {c: [] for c in cases}
  runs_dir = out / 'runs'
  runs_dir.mkdir(exist_ok=True)
  timing = tools['mhgp12_v11_traversal_timing']
  present = [c for c in cases if c in case_dumps]
  isolation_before = m2.gpu_quiet(s, 'avant_tours') if nvcc else (False, {'quiet': False, 'reason': 'sans CUDA'})
  for p in range(args.processes + 1):
    order = present[p % len(present):] + present[:p % len(present)] if present else []
    for name in order:
      frame, k, leaf = frame_of[name]
      v11_target = runs_dir / ('%s_p%d_v11.json' % (name, p))
      gpu_target = runs_dir / ('%s_p%d_gpu.json' % (name, p))
      if not clear(v11_target) or not clear(gpu_target):
        s.refuse('%s tour %d : prise perimee non effacable' % (name, p))
        continue
      entry = {'round': p}
      v11 = None
      if timing.is_file():
        code, stdout, _ = s.run('v11_%s_p%d' % (name, p),
                                [timing, inputs[frame][0], inputs[frame][1], k, leaf, args.v11_workers,
                                 args.v11_passes, '--walk-reps', 0 if p else 3], 1800)
        v = last_json_line(stdout)
        if v is not None:
          v11_target.write_text(json.dumps(v))
        v11, why = v11_take(v, k, leaf, args.v11_workers, idents[name]) if code == 0 else (None, 'code %d' % code)
        if v11 is None:
          s.refuse('%s tour %d : chrono v11 en echec ou incoherent (%s)' % (name, p, why))
        elif v11['v11_ledger_ok'] is not True:
          s.refuse('%s tour %d : grand livre du chrono v11 different du vidage' % (name, p))
      gpu = None
      if nvcc and bench.is_file():
        code, _, _ = s.run('gpu_%s_p%d' % (name, p), [bench, '--dump', case_dumps[name], '--reps', args.reps,
                                                       '--warmup', args.warmup, '--mutants', 'none', '--json',
                                                       gpu_target, '--nonce', nonce], 1800)
        gpu, why = gpu_take(read_json(gpu_target), case_dumps[name], idents[name], nonce, args.reps, args.warmup) \
            if code in (0, 1) else (None, 'code %d' % code)
        if gpu is not None and (code == 0) != (gpu['gpu_identity'] is True):
          gpu, why = None, 'code %d et identite discordants' % code
        if gpu is None:
          s.refuse('%s tour %d : banc GPU en echec, perime ou incomplet (%s)' % (name, p, why))
      if v11 is not None and gpu is not None:
        entry.update(v11)
        entry.update(gpu)
        if p > 0:
          rounds[name].append(entry)
  isolation_after = m2.gpu_quiet(s, 'apres_tours') if nvcc else (False, {'quiet': False, 'reason': 'sans CUDA'})
  isolation = isolation_start and isolation_before[0] and isolation_after[0]
  report['gpu_isolation'] = isolation
  report['gpu_isolation_detail'] = {'start': isolation_start, 'before_rounds': isolation_before[1],
                                    'after_rounds': isolation_after[1]}
  report['rounds'] = rounds

  # 8. Fin de session : binaires, sources, dependances compilees et vidages inchanges depuis leur empreinte.
  end_binaries, end_sources = binary_hashes(), source_hashes()
  if end_binaries != binaries:
    s.refuse('binaire modifie pendant la session')
  if end_sources != sources:
    s.refuse('sources modifiees pendant la session')
  if bdir.is_dir() and m2.compiled_dependencies(bdir, dep_roots) != dependencies:
    s.refuse('dependance compilee modifiee pendant la session')
  changed = [n for n, p in list(case_dumps.items()) + list(fixture_dumps.items())
             if (m2.sha256_file(p) if p.is_file() else None) != idents[n]['sha256']]
  if changed:
    s.refuse('vidages modifies pendant la session : ' + ', '.join(changed))
  report['hashes_end'] = {'binaries_equal': end_binaries == binaries, 'sources_equal': end_sources == sources,
                          'dumps_changed': changed}
  evidence = {'cases': cases, 'deciding': deciding, 'processes': args.processes, 'reps': args.reps,
              'v11_passes': args.v11_passes, 'refusals': list(s.refusals), 'isolation': isolation, 'unit_ok': unit_ok,
              'fixtures_ok': fixtures_ok, 'fixture_names': fixture_names,
              'sanitizer': sanitizer_codes if not args.skip_sanitizer else {}, 'sanitizer_proof': sanitizer_proof,
              'host_code': host_code, 'host_identity': host_identity, 'fixtures_host': fixtures_host,
              'device_fixtures_code': device_code, 'fixtures_device': fixtures_device, 'host_kills': host_kills,
              'device_kills': device_kills, 'rounds': rounds}
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


# ---------------------------------------------------------------------------------------------------- rejugement
def evidence_from_outputs(folder, nonce=None):
  """Preuves d'une session reconstituees depuis ses sorties publiees, par les memes validateurs que main.

  nonce : jeton exige dans chaque prise ; par defaut celui du rapport (None pour une session anterieure au jeton).
  Rend (evidence, report, notes) ; notes : ce qui n'est pas rejouable depuis ces sorties, declare."""
  report = read_json(folder / 'report.json')
  if not isinstance(report, dict) or 'args' not in report or 'cases' not in report:
    return None, None, ['report.json absent ou illisible']
  nonce = report.get('nonce') if nonce is None else nonce
  args, rule = report['args'], report['rule']
  notes, refusals = [], list((report.get('evidence') or {}).get('refusals', []))
  if nonce is None:
    notes.append('jeton de session : absent (sorties anterieures au durcissement) ; fraicheur attestee par l effacement '
                 'des cibles avant chaque commande dans le script de la session, non rejouable')
  frames = [f for f in args['frames'].split(',') if f]
  configs, decide = parse_configs(args['configs']), parse_configs(args['decide'])
  for reason in contract_deviations(frames, configs, decide, args['processes'], args['reps'], args['warmup'],
                                    args['v11_passes'], args['v11_workers'], args['crop'], args['skip_sanitizer'],
                                    args['no_cuda']):
    refusals.append('hors contrat : ' + reason)
  steps = {}
  for step in report.get('steps', []):
    steps.setdefault(step.get('step'), step.get('code'))
  cases = list(report['cases'])
  deciding = list(rule['deciding'])
  work = Path(args['work'])
  crop_name = '%s_k5_l24_crop%d' % (frames[0], args['crop'])
  frame_of = {}
  for name in cases:
    found = re.fullmatch(r'(.+)_k(\d+)_l(\d+)', name)
    if found is None:
      refusals.append('%s : nom de cas illisible' % name)
      continue
    frame_of[name] = (found.group(1), int(found.group(2)), int(found.group(3)))
  idents, paths = {}, {}
  for name in cases + [crop_name]:
    entry = (report.get('dumps') or {}).get(name) or {}
    summary = entry.get('summary') or {}
    if steps.get('dump_' + name) != 0 or entry.get('code') != 0 or not entry.get('sha256'):
      refusals.append('vidage absent ou en echec : ' + name)
      continue
    idents[name] = {'kmax': summary.get('kmax'), 'leaf_size': summary.get('leaf_size'),
                    'coord_bits': summary.get('coord_bits'), 'n_sites': summary.get('sites'),
                    'status': summary.get('status'), 'nodes': summary.get('nodes'),
                    'filter_tests': summary.get('filter_tests')}
    paths[name] = work / 'dumps' / (name + '.bin')
    want = frame_of.get(name, (frames[0], 5, 24))
    if idents[name]['coord_bits'] != PROFILE_BITS or idents[name]['kmax'] != want[1] or \
            idents[name]['leaf_size'] != want[2] or idents[name]['status'] != 0:
      refusals.append('vidage %s : sortie de l outil differente du cas' % name)
  manifest = report.get('fixtures')
  complete, fixture_names, why = fixtures_manifest(manifest, nonce)
  fixtures_ok = steps.get('fixtures') == 0 and complete
  if not fixtures_ok:
    refusals.append('fixtures : %s' % (why or 'outil en echec'))
  for e in (manifest or {}).get('fixtures', []) if complete else []:
    check = e.get('oracle_check') or {}
    idents[e['name']] = {'kmax': e.get('kmax'), 'leaf_size': e.get('leaf'), 'coord_bits': e.get('bits'),
                         'n_sites': e.get('sites'), 'status': check.get('status')}
    paths[e['name']] = work / 'fixtures' / (e['name'] + '.bin')
  # Identite hote : JSON brut, egal a celui du rapport.
  lines = []
  raw = folder / 'identity_host.json'
  if raw.is_file():
    lines = jsonl(raw.read_text())
  host_code = (report.get('identity_host') or {}).get('code')
  if lines != (report.get('identity_host') or {}).get('results'):
    refusals.append('identite hote : JSON brut different du rapport')
  units = [l for l in lines if l.get('phase') == 'unit']
  unit_ok = len(units) == 1 and units[0].get('ok') is True and (nonce is None or units[0].get('nonce') == nonce)
  if nonce is not None and units and units[0].get('emit_tasks_u64') is not True:
    unit_ok = False
  if nonce is None:
    notes.append('portes unitaires : ok global de la ligne publiee ; le temoin CST-0222 (emit_tasks_u64) n existait '
                 'pas')
  by_path = {}
  for line in lines:
    if line.get('phase') == 'identity':
      by_path.setdefault(line.get('dump'), []).append(line)
  host_identity, fixtures_host, host_kills = {}, {}, {m: [] for m in MUTANTS}
  for name in cases + [crop_name] + list(fixture_names if complete else []):
    if name not in paths:
      continue
    found = by_path.get(str(paths[name]), [])
    if len(found) != 1:
      refusals.append('identite hote : %d ligne(s) pour %s' % (len(found), name))
      continue
    ok, why = identity_line(found[0], idents[name], nonce)
    if ok is None:
      refusals.append('identite hote %s : %s' % (name, why))
      continue
    if name in FIXTURES:
      fixtures_host[name] = ok
    elif name in cases:
      host_identity[name] = ok
    elif not ok:
      refusals.append('identite hote en defaut sur la decoupe ' + crop_name)
    for m, v in found[0]['mutants'].items():
      if v['killed'] is True:
        host_kills[m].append(name)
  # Sanitizers : codes et prises brutes sur les trois cibles.
  sanitizer, sanitizer_proof = {}, {}
  targets = [crop_name] + list(SANITIZER_FIXTURES)
  for t in SANITIZERS:
    entry = (report.get('sanitizer') or {}).get(t)
    if not isinstance(entry, dict):
      continue
    sanitizer[t] = entry.get('code')
    if steps.get('sanitizer_' + t) != entry.get('code'):
      refusals.append('sanitizer %s : code du rapport different de l etape' % t)
    if not all(n in paths for n in targets):
      sanitizer_proof[t] = False
      continue
    taken, _ = bench_json(read_json(folder / ('sanitizer_%s.json' % t)), nonce, 1, 0, [paths[n] for n in targets])
    sanitizer_proof[t] = taken is not None and all(bench_case(c, paths[n], idents[n])[0] is True
                                                   for c, n in zip(taken, targets))
  # Fixtures sur l'appareil : JSON brut, egal a celui du rapport.
  device = report.get('device_fixtures') or {}
  device_code, result = device.get('code'), read_json(folder / 'device_fixtures.json')
  if result != device.get('result'):
    refusals.append('fixtures sur l appareil : JSON brut different du rapport')
  fixtures_device, device_kills = {}, {}
  order = [n for n in FIXTURES if n in paths]
  if complete and manifest:
    order = [e['name'] for e in manifest['fixtures'] if e['name'] in paths]
  taken, why = bench_json(result, nonce, 1, 0, [paths[n] for n in order])
  if taken is None:
    refusals.append('fixtures sur l appareil : %s' % why)
  else:
    for c, name in zip(taken, order):
      ok, why = bench_case(c, paths[name], idents[name])
      mutants = c.get('mutants') if isinstance(c.get('mutants'), dict) else {}
      if ok is None or sorted(mutants) != sorted(MUTANTS):
        refusals.append('fixture %s sur l appareil : %s' % (name, why or 'mutants absents'))
        continue
      fixtures_device[name] = ok
      for m, v in mutants.items():
        if isinstance(v, dict) and v.get('killed') is True:
          device_kills.setdefault(m, []).append(name)
  # Tours : prises brutes de runs/, rattachees au vidage et a leur commande.
  rounds = {c: [] for c in cases}
  for name in cases:
    if name not in idents or name not in frame_of:
      continue
    frame, k, leaf = frame_of[name]
    for p in range(1, args['processes'] + 1):
      v_code, g_code = steps.get('v11_%s_p%d' % (name, p)), steps.get('gpu_%s_p%d' % (name, p))
      v11, why = v11_take(read_json(folder / 'runs' / ('%s_p%d_v11.json' % (name, p))), k, leaf, args['v11_workers'],
                          idents[name]) if v_code == 0 else (None, 'code %r' % v_code)
      if v11 is None:
        refusals.append('%s tour %d : chrono v11 (%s)' % (name, p, why))
        continue
      gpu, why = gpu_take(read_json(folder / 'runs' / ('%s_p%d_gpu.json' % (name, p))), paths[name], idents[name],
                          nonce, args['reps'], args['warmup']) if g_code in (0, 1) else (None, 'code %r' % g_code)
      if gpu is not None and (g_code == 0) != (gpu['gpu_identity'] is True):
        gpu, why = None, 'code %r et identite discordants' % g_code
      if gpu is None:
        refusals.append('%s tour %d : banc GPU (%s)' % (name, p, why))
        continue
      entry = {'round': p}
      entry.update(v11)
      entry.update(gpu)
      rounds[name].append(entry)
  detail = report.get('gpu_isolation_detail')
  isolation = report.get('gpu_isolation') is True
  if isinstance(detail, dict):
    isolation = isolation and detail.get('start') is True and (detail.get('before_rounds') or {}).get('quiet') is True \
        and (detail.get('after_rounds') or {}).get('quiet') is True
  else:
    notes.append('isolation du GPU : releve de debut de session seulement (aucun releve avant et apres les tours)')
  if 'format_selftest' not in report:
    notes.append('porte du lecteur strict (CST-0223) : absente de cette session, non rejouable ici ; les vidages '
                 'reels de la session C, refaits a l identique, sont admis par le lecteur durci (RAPPORT)')
  if 'hashes_end' not in report:
    notes.append('rehachage final : binaires seulement (aucun refus publie) ; sources et dependances non rehachees')
  evidence = {'cases': cases, 'deciding': deciding, 'processes': args['processes'], 'reps': args['reps'],
              'v11_passes': args['v11_passes'], 'refusals': refusals, 'isolation': isolation, 'unit_ok': unit_ok,
              'fixtures_ok': fixtures_ok, 'fixture_names': fixture_names,
              'sanitizer': sanitizer if not args['skip_sanitizer'] else {}, 'sanitizer_proof': sanitizer_proof,
              'host_code': host_code, 'host_identity': host_identity, 'fixtures_host': fixtures_host,
              'device_fixtures_code': device_code, 'fixtures_device': fixtures_device, 'host_kills': host_kills,
              'device_kills': device_kills, 'rounds': rounds}
  return evidence, report, notes


def same_stats(mine, published):
  """Statistiques egales a l'octet (Python < 3.12, celui de la VM) ou a 1e-12 relatif (sum() compensee depuis 3.12)."""
  exact = sys.version_info < (3, 12)

  def equal(a, b):
    if isinstance(a, dict) and isinstance(b, dict):
      return sorted(a) == sorted(b) and all(equal(a[k], b[k]) for k in a)
    if isinstance(a, list) and isinstance(b, list):
      return len(a) == len(b) and all(equal(x, y) for x, y in zip(a, b))
    if is_real(a) and is_real(b) and not exact:
      return abs(a - b) <= 1e-12 * max(1.0, abs(b))
    return a == b and type(a) is type(b)
  return equal(mine, published)


def rejudge(folder, expect_published):
  evidence, report, notes = evidence_from_outputs(folder)
  if evidence is None:
    print(json.dumps({'rejudge': str(folder), 'error': notes}))
    return 2
  verdict, refused, rejected, stats = judge(evidence)
  identical = verdict == report.get('verdict') and same_stats(stats, report.get('stats'))
  rounds = sum(len(v) for v in evidence['rounds'].values())
  print(json.dumps({'rejudge': str(folder), 'verdict': verdict, 'published_verdict': report.get('verdict'),
                    'stats_identical': same_stats(stats, report.get('stats')), 'identical': identical,
                    'python': sys.version.split()[0], 'refused': refused, 'rejected': rejected,
                    'cases': len(evidence['cases']), 'valid_rounds': rounds,
                    'fixtures_host': len(evidence['fixtures_host']), 'fixtures_device': len(evidence['fixtures_device']),
                    'ratios': {n: [round(v['ratio_gm'], 4), round(v['ci95'][1], 4)] for n, v in stats.items()},
                    'not_replayable': notes}, ensure_ascii=False, indent=1))
  return 0 if (identical or not expect_published) else 1


if __name__ == '__main__':
  sys.exit(main())
