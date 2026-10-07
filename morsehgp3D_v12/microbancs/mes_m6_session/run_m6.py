#!/usr/bin/env python3
"""MES-M6 sur G4 : compile mes_m6_session_cost.cu et le joue dans un processus neuf par mode d'attente de l'hote.

Usage : run_m6.py --work W --out O [--nvcc NVCC] [--reps N] [--processes P]
        run_m6.py --rejuger O [--reps N] [--processes P]
Chaque mode (spin, yield, blocking) est joue P fois, chaque fois dans un processus neuf (le mode d'attente se fixe
avant l'ouverture du contexte) ; les lignes JSON du banc vont dans O/m6_<mode>_<i>.jsonl, l'environnement (nvcc,
nvidia-smi, uptime), les codes, les empreintes et le controle de chaque prise dans O/m6_report.json.

Le pilote ne rend aucun verdict d'adoption (M6 est un banc publie), mais son succes exige ses preuves (CST-0018,
complement de pilote, recu audit_session_t1_20261007/m6) : sorties fraiches (fichiers m6_* d'un passage anterieur
effaces avant tout), binaire efface, recompile, present et rehache apres les prises, source et pilote rehaches ; GPU
isole (nvidia-smi lisible, aucun processus de calcul) au debut, avant et apres chaque prise ; chaque prise de code 0
rend exactement les 65 mesures attendues (une ligne JSON chacune, aucune en double ni en trop, ligne device du mode
joue et du meme appareil pour toutes les prises, effectifs n fixes par --reps comme dans le banc, nombres finis
positifs, p05 <= p50 <= p95 <= max). --rejuger relit un dossier publie avec les memes regles. Rapport v2 : schema
strict (champs obligatoires types, empreintes SHA-256 du binaire et des deux sources, releves d'isolation coherents
avec leur code et leurs processus, aucun refus publie, prises declarees conformes sans probleme, medianes publiees
egales a celles des prises ; recu audit_juges_emst_20261007/juges). Rapport v1 (historique) : relu avec ses limites
declarees non rejouables (ni empreintes, ni isolation par prise, ni effectif publie). Bibliotheque standard seule
(Python 3.10 nu, aucun assert). Codes : 0 conforme (mes_m6_ok) ; 2 usage ; 3 compilation, execution, preuve ou
sortie en echec (mes_m6_echec, raisons dans le rapport et sur stderr).
"""

import argparse
import hashlib
import json
import math
import os
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'mes_m6_session_cost.cu'
DRIVER = Path(__file__).resolve()
SCHEMA = 'ehgp.v12.mes_m6.v2'
MODES = ('spin', 'yield', 'blocking')
GPU_APPS = ['nvidia-smi', '--query-compute-apps=pid,process_name,used_memory', '--format=csv,noheader']
BIG = 256 << 20
SIZES = (4 << 10, 64 << 10, 720 << 10, 4 << 20, 64 << 20, BIG)
ONCE = ('context_open', 'two_streams', 'first_launch', 'pinned_alloc_256mib', 'pool_alloc_cold_256mib',
        'graph_of_ten_prepare')
QUANTILES = ('p05_us', 'p50_us', 'p95_us', 'max_us')
DEVICE_FIELDS = ('cc', 'device', 'global_mib', 'mes', 'name', 'sm', 'sync')
EXTRA_FIELDS = ('bytes', 'host', 'dir', 'bytes_rw')
SUMMARY = ('context_open', 'first_launch', 'empty_kernel_launch_sync', 'graph_of_ten_sync', 'touch_256mib')
SHA256_HEX = re.compile(r'[0-9a-f]{64}\Z')
SOURCES = ('mes_m6_session_cost.cu', 'run_m6.py')
# Champs obligatoires d'un rapport v2 et leur type (schema strict de la relecture).
REPORT_V2_FIELDS = (('date_utc', str), ('cleared', list), ('nvcc', list), ('gpu', str), ('gpu_apps', str),
                    ('uptime_since', str), ('runs', list), ('refusals', list), ('verdict', str))


def find_nvcc(explicit):
  candidates = [explicit, shutil.which('nvcc')]
  if os.environ.get('CUDA_HOME'):
    candidates.append(os.path.join(os.environ['CUDA_HOME'], 'bin', 'nvcc'))
  candidates += ['/usr/local/cuda/bin/nvcc']
  candidates += sorted(str(path) for path in Path('/usr/local').glob('cuda-12*/bin/nvcc'))
  for candidate in candidates:
    if candidate and os.path.isfile(candidate) and os.access(candidate, os.X_OK):
      return candidate
  return None


def capture(command, timeout):
  try:
    done = subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)
    return done.returncode, done.stdout, done.stderr
  except (OSError, subprocess.TimeoutExpired) as error:
    return -1, '', str(error)


def sha256(path):
  try:
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
      for block in iter(lambda: handle.read(1 << 20), b''):
        digest.update(block)
    return digest.hexdigest()
  except OSError:
    return None


def is_int(value):
  return isinstance(value, int) and not isinstance(value, bool)


def finite(value):
  return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value) and value >= 0


# ---------------------------------------------------------------- contenu d'une prise
def expected_rows(reps):
  """Les 65 mesures d'une prise de mes_m6_session_cost.cu : cle -> (effectif n, genre). Cle : (name, bytes, host,
  dir, bytes_rw), None pour un champ absent ; genre : 'device', 'us' (usage unique) ou 'quantiles'."""
  rows = {(name, None, None, None, None): (1, 'us') for name in ONCE}
  rows[('device', None, None, None, None)] = (None, 'device')
  for name, n in (('pool_alloc_free_256mib', min(reps, 1000)), ('empty_kernel_launch_sync', reps),
                  ('ten_launches_sync', reps), ('graph_of_ten_sync', reps)):
    rows[(name + '_first', None, None, None, None)] = (1, 'us')
    rows[(name, None, None, None, None)] = (n, 'quantiles')
  for size in SIZES:
    n = max(10, reps // 100) if size >= 64 << 20 else max(10, reps // 10)
    for host in ('pinned', 'pageable'):
      for direction in ('h2d', 'd2h'):
        rows[('copy_first', size, host, direction, None)] = (1, 'us')
        rows[('copy', size, host, direction, None)] = (n, 'quantiles')
  rows[('touch_256mib_first', None, None, None, 2 * BIG)] = (1, 'us')
  rows[('touch_256mib', None, None, None, 2 * BIG)] = (max(10, reps // 100), 'quantiles')
  return rows


def check_row(row, n, kind, mode):
  """Probleme d'une ligne de cle attendue (None : conforme)."""
  extra = [field for field in EXTRA_FIELDS if field in row]
  if kind == 'device':
    if sorted(row) != list(DEVICE_FIELDS):
      return 'champs de la ligne device'
    if row['sync'] != mode:
      return 'ligne device du mode %r dans une prise %s' % (row['sync'], mode)
    if not isinstance(row['device'], str) or not row['device'] or not is_int(row['sm']) or row['sm'] <= 0 or \
        not isinstance(row['cc'], str) or not re.fullmatch(r'[0-9]+\.[0-9]+', row['cc']) or \
        not is_int(row['global_mib']) or row['global_mib'] <= 0:
      return 'ligne device illisible'
    return None
  fields = ['mes', 'name', 'n'] + (['us'] if kind == 'us' else list(QUANTILES)) + extra
  if sorted(row) != sorted(fields):
    return 'champs %s' % ','.join(sorted(row))
  if not is_int(row['n']) or row['n'] != n:
    return 'effectif %r, attendu %d' % (row['n'], n)
  if kind == 'us':
    return None if finite(row['us']) else 'duree non finie ou negative'
  values = [row[q] for q in QUANTILES]
  if not all(finite(value) for value in values):
    return 'quantile non fini ou negatif'
  if values != sorted(values):
    return 'quantiles non ordonnes (p05 <= p50 <= p95 <= max)'
  return None


def check_take(text, mode, reps):
  """Problemes d'une prise (liste vide : conforme) : exactement les 65 mesures attendues, une ligne JSON chacune."""
  if not text:
    return ['prise vide']
  if not text.endswith('\n'):
    return ['prise tronquee (derniere ligne sans fin de ligne)']
  expected = expected_rows(reps)
  seen, problems = set(), []
  for number, line in enumerate(text[:-1].split('\n'), 1):
    try:
      row = json.loads(line)
    except ValueError:
      problems.append('ligne %d illisible' % number)
      continue
    if not isinstance(row, dict) or row.get('mes') != 'M6':
      problems.append('ligne %d hors schema' % number)
      continue
    key = (row.get('name'),) + tuple(row.get(field) for field in EXTRA_FIELDS)
    if not all(part is None or isinstance(part, (str, int)) for part in key):
      problems.append('ligne %d hors schema' % number)
      continue
    if key in seen:
      problems.append('ligne %d en double : %s' % (number, key[0]))
      continue
    seen.add(key)
    if key not in expected:
      problems.append('ligne %d inattendue : %s' % (number, key[0]))
      continue
    n, kind = expected[key]
    problem = check_row(row, n, kind, mode)
    if problem:
      problems.append('%s (ligne %d) : %s' % (key[0], number, problem))
  missing = sorted({key[0] for key in expected if key not in seen})
  if missing:
    problems.append('mesures absentes : %s' % ', '.join(missing[:6]))
  return problems


def device_of(text):
  """Identite de l'appareil d'une prise conforme : (device, sm, cc, global_mib)."""
  for line in text.splitlines():
    row = json.loads(line)
    if row.get('name') == 'device':
      return row['device'], row['sm'], row['cc'], row['global_mib']
  return None


def summary(takes):
  """Mediane entre processus, par mode, des cinq mesures resumees (p50 ou usage unique, microsecondes)."""
  result = {}
  for mode in MODES:
    values = {name: [] for name in SUMMARY}
    for (take_mode, _), text in sorted(takes.items()):
      if take_mode != mode:
        continue
      for line in text.splitlines():
        row = json.loads(line)
        if row.get('name') in values:
          values[row['name']].append(row.get('p50_us', row.get('us')))
    result[mode] = {name: statistics.median(v) for name, v in values.items() if v}
  return result


def gpu_quiet(label, settle=0):
  """Isolation du GPU : nvidia-smi lisible (code 0) et aucun processus de calcul. Rend (certifiee, releve). settle :
  releves supplementaires, a 0,5 s d'intervalle, accordes juste apres la fin d'une prise (le contexte du processus
  joint peut encore etre liste) ; seul le releve est repete, jamais la prise."""
  attempts = 0
  while True:
    code, out, err = capture(GPU_APPS, 60)
    attempts += 1
    quiet = code == 0 and out.strip() == ''
    if quiet or attempts > settle:
      break
    time.sleep(0.5)
  return quiet, {'label': label, 'code': code, 'processes': out.strip()[-400:], 'stderr': err.strip()[-200:],
                 'quiet': quiet, 'attempts': attempts}


def clear_outputs(out):
  """Efface les sorties d'un passage anterieur (m6_*.jsonl, m6_report.json). Rend (noms effaces, probleme)."""
  removed = []
  try:
    for path in sorted(out.iterdir()):
      if path.name == 'm6_report.json' or (path.name.startswith('m6_') and path.name.endswith('.jsonl')):
        path.unlink()
        removed.append(path.name)
  except OSError as error:
    return removed, 'sortie anterieure non effacable : %s' % error
  return removed, None


def write_report(out, report):
  temporary = out / 'm6_report.json.tmp'
  temporary.write_text(json.dumps(report, indent=1, sort_keys=True), encoding='utf-8')
  os.replace(temporary, out / 'm6_report.json')


# ---------------------------------------------------------------- passage
def run(args):
  work, out = Path(args.work), Path(args.out)
  try:
    work.mkdir(parents=True, exist_ok=True)
    out.mkdir(parents=True, exist_ok=True)
  except OSError as error:
    print('run_m6 : dossier impossible : %s' % error, file=sys.stderr)
    return 2
  refusals = []
  report = {'schema': SCHEMA, 'date_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), 'reps': args.reps,
            'processes': args.processes}
  report['cleared'], problem = clear_outputs(out)
  if problem:
    refusals.append(problem)
  sources = {'run_m6.py': sha256(DRIVER), 'mes_m6_session_cost.cu': sha256(SOURCE)}
  report['sources_sha256'] = sources
  if None in sources.values():
    refusals.append('source illisible')
  nvcc = find_nvcc(args.nvcc)
  report['nvcc'] = nvcc and capture([nvcc, '--version'], 60)[1].strip().splitlines()[-1:]
  report['gpu'] = capture(['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,clocks.sm,clocks.max.sm,'
                           'temperature.gpu,power.draw,memory.used,memory.total', '--format=csv,noheader'], 60)[1].strip()
  quiet, report['isolation_start'] = gpu_quiet('debut')
  report['gpu_apps'] = report['isolation_start']['processes']
  if not quiet:
    refusals.append('GPU non isole au debut (nvidia-smi illisible ou processus de calcul)')
  report['uptime_since'] = capture(['uptime', '-s'], 30)[1].strip()
  binary = work / 'mes_m6'
  try:
    if binary.exists() or binary.is_symlink():
      binary.unlink()
  except OSError as error:
    refusals.append('binaire anterieur non effacable : %s' % error)
  if nvcc is None:
    report['refusal'] = 'nvcc introuvable'
    refusals.append('nvcc introuvable')
  else:
    compile_code, _, compile_err = capture([nvcc, '-std=c++20', '-O3', '-arch=sm_120', '-Xcompiler',
                                            '-Wall,-Wextra,-Werror', '-o', str(binary), str(SOURCE)], 600)
    report['compile'] = {'code': compile_code, 'stderr': compile_err[-2000:]}
    report['binary_sha256'] = sha256(binary) if binary.is_file() else None
    if compile_code != 0:
      refusals.append('compilation en echec (code %d)' % compile_code)
    elif report['binary_sha256'] is None:
      refusals.append('binaire absent apres la compilation')
  runs, takes = [], {}
  if report.get('binary_sha256') and not any(r.startswith('compilation') for r in refusals):
    for index in range(args.processes):
      for mode in MODES:
        before, before_detail = gpu_quiet('avant_%s_%d' % (mode, index))
        started = time.time()
        run_code, run_out, run_err = capture([str(binary), '--sync=' + mode, '--reps=' + str(args.reps)], 900)
        seconds = round(time.time() - started, 2)
        after, after_detail = gpu_quiet('apres_%s_%d' % (mode, index), settle=2)
        data = run_out.encode('utf-8')
        (out / ('m6_%s_%d.jsonl' % (mode, index))).write_bytes(data)
        problems = (['code %d' % run_code] if run_code != 0 else []) + check_take(run_out, mode, args.reps)
        if not before or not after:
          problems.append('GPU non isole %s la prise' % ('avant' if not before else 'apres'))
        runs.append({'mode': mode, 'process': index, 'code': run_code, 'seconds': seconds,
                     'stderr': run_err[-1000:], 'lines': run_out.count('\n'), 'sha256': hashlib.sha256(data).hexdigest(),
                     'isolation_before': before_detail, 'isolation_after': after_detail, 'problems': problems[:12],
                     'conform': not problems})
        if problems:
          refusals.append('prise %s %d : %s' % (mode, index, '; '.join(problems[:3])))
        else:
          takes[(mode, index)] = run_out
    if sha256(binary) != report['binary_sha256']:
      refusals.append('binaire modifie ou retire pendant les prises')
  if {'run_m6.py': sha256(DRIVER), 'mes_m6_session_cost.cu': sha256(SOURCE)} != sources:
    refusals.append('source ou pilote modifie pendant le passage')
  devices = {device_of(text) for text in takes.values()}
  if len(devices) > 1:
    refusals.append('appareil different entre prises')
  report['runs'] = runs
  if not refusals:
    report['summary_median_us'] = summary(takes)
  report['refusals'] = refusals
  report['verdict'] = 'mes_m6_ok' if not refusals else 'mes_m6_echec'
  write_report(out, report)
  for line in refusals[:8]:
    print('run_m6 : ' + line, file=sys.stderr)
  print('%s processus=%d' % (report['verdict'], len(runs)))
  return 0 if not refusals else 3


# ---------------------------------------------------------------- relecture d'un dossier publie
def is_sha256(value):
  return isinstance(value, str) and SHA256_HEX.match(value) is not None


def isolation_problem(record, label):
  """Releve d'isolation d'un rapport v2 : champs types, quiet vrai si et seulement si nvidia-smi a rendu le code 0 et
  aucune ligne de processus (comme gpu_quiet), et quiet vrai. Rend None ou la raison."""
  if not isinstance(record, dict) or not is_int(record.get('code')) or not isinstance(record.get('processes'), str) \
      or not isinstance(record.get('quiet'), bool) or not is_int(record.get('attempts')) or record['attempts'] < 1:
    return 'isolation %s : releve absent ou illisible' % label
  if record['quiet'] != (record['code'] == 0 and record['processes'].strip() == ''):
    return 'isolation %s : quiet=%r contredit par le code %d et les processus %r' % (
        label, record['quiet'], record['code'], record['processes'][:40])
  if not record['quiet']:
    return 'isolation %s : GPU non isole' % label
  return None


def report_v2_problems(report):
  """Schema strict d'un rapport v2 (CST-0018, recu audit_juges_emst_20261007/juges) : champs obligatoires types,
  provenance (empreintes SHA-256 du binaire et des deux sources), compilation de code 0, isolation du debut coherente,
  et aucun refus publie : une relecture ne peut pas etablir apres coup ce que le passage a refuse."""
  problems = []
  for key, kind in REPORT_V2_FIELDS:
    if not isinstance(report.get(key), kind):
      problems.append('champ %s absent ou illisible' % key)
  if not is_sha256(report.get('binary_sha256')):
    problems.append('empreinte du binaire absente ou illisible')
  sources = report.get('sources_sha256')
  if not isinstance(sources, dict) or sorted(sources) != sorted(SOURCES) or \
      not all(is_sha256(v) for v in sources.values()):
    problems.append('empreintes des sources absentes, incompletes ou illisibles')
  compiled = report.get('compile')
  if not isinstance(compiled, dict) or compiled.get('code') != 0 or not isinstance(compiled.get('stderr'), str):
    problems.append('compilation absente ou en echec')
  why = isolation_problem(report.get('isolation_start'), 'du debut')
  if why is not None:
    problems.append(why)
  refusals = report.get('refusals')
  if isinstance(refusals, list) and refusals:
    problems.append('refus publies dans le rapport : ' + '; '.join(str(r) for r in refusals[:3]))
  return problems


def run_v2_problems(r):
  """Entree v2 d'une prise : duree, sortie d'erreur, empreinte, deux releves d'isolation coherents, et ni probleme
  publie ni prise declaree non conforme."""
  found = []
  seconds = r.get('seconds')
  if not isinstance(seconds, (int, float)) or isinstance(seconds, bool) or not math.isfinite(seconds) or seconds < 0 \
      or not isinstance(r.get('stderr'), str) or not is_sha256(r.get('sha256')):
    found.append('entree de la prise illisible (duree, sortie d erreur ou empreinte)')
  for side, label in (('isolation_before', 'avant la prise'), ('isolation_after', 'apres la prise')):
    why = isolation_problem(r.get(side), label)
    if why is not None:
      found.append(why)
  if r.get('problems') != [] or r.get('conform') is not True:
    found.append('prise declaree non conforme ou problemes publies')
  return found


def rejudge(folder, args):
  """Relit un dossier publie (rapport et prises) avec les regles du passage. Rend 0 si tout est conforme."""
  problems, not_replayable, takes, lines = [], [], {}, 0
  try:
    report = json.loads((folder / 'm6_report.json').read_text(encoding='utf-8'))
  except (OSError, ValueError) as error:
    report, problems = None, ['rapport illisible : %s' % error]
  if report is not None and not isinstance(report, dict):
    report, problems = None, ['rapport hors schema']
  v2 = report is not None and report.get('schema') == SCHEMA
  reps, processes = (report.get('reps'), report.get('processes')) if v2 else (args.reps, args.processes)
  if report is not None and not v2:
    not_replayable.append('rapport %s : effectif et nombre de processus non publies, pris de la commande (--reps %d, '
                          '--processes %d)' % (report.get('schema'), reps, processes))
    not_replayable.append('empreintes du binaire, des sources et des prises absentes : rattachement par le paquet de '
                          'session seulement')
    not_replayable.append("isolation : sortie de nvidia-smi au debut sans son code, aucune prise encadree")
  if report is not None and (not is_int(reps) or not is_int(processes) or not 10 <= reps <= 100000 or
                             not 1 <= processes <= 20):
    problems.append('effectif ou nombre de processus illisible')
    report = None
  if report is not None:
    compiled = report.get('compile')
    if not isinstance(compiled, dict) or compiled.get('code') != 0:
      problems.append('compilation absente ou en echec')
    if report.get('gpu_apps') != '':
      problems.append('GPU occupe ou non releve au debut')
    if v2:
      problems += report_v2_problems(report)
    runs = report.get('runs')
    runs = runs if isinstance(runs, list) and all(isinstance(r, dict) for r in runs) else []
    keys = [(r.get('mode'), r.get('process')) for r in runs]
    expected = {(mode, index) for index in range(processes) for mode in MODES}
    if len(keys) != len(expected) or set(keys) != expected:
      problems.append('prises manquantes, en double ou en trop (%d pour %d)' % (len(keys), len(expected)))
    for r in runs:
      mode, index = r.get('mode'), r.get('process')
      if mode not in MODES or not is_int(index):
        continue
      try:
        data = (folder / ('m6_%s_%d.jsonl' % (mode, index))).read_bytes()
        text = data.decode('utf-8')
      except (OSError, UnicodeDecodeError) as error:
        problems.append('prise %s %d illisible : %s' % (mode, index, error))
        continue
      found = (['code %r' % r.get('code')] if r.get('code') != 0 else []) + check_take(text, mode, reps)
      if r.get('lines') != text.count('\n'):
        found.append('nombre de lignes different du rapport')
      if v2:
        if r.get('sha256') != hashlib.sha256(data).hexdigest():
          found.append('empreinte differente du rapport')
        found += run_v2_problems(r)
      if found:
        problems.append('prise %s %d : %s' % (mode, index, '; '.join(found[:3])))
      else:
        takes[(mode, index)] = text
        lines += text.count('\n')
    if len({device_of(text) for text in takes.values()}) > 1:
      problems.append('appareil different entre prises')
    if v2 and not problems and report.get('summary_median_us') != summary(takes):
      problems.append('medianes publiees differentes de celles des prises')
  verdict = 'mes_m6_ok' if not problems else 'mes_m6_echec'
  if v2 and report is not None and report.get('verdict') != verdict:
    problems.append('verdict publie %r, rejuge %s' % (report.get('verdict'), verdict))
    verdict = 'mes_m6_echec'
  print(json.dumps({'rejuge': folder.name, 'schema': report.get('schema') if report else None, 'verdict': verdict,
                    'prises': len(takes), 'lignes': lines, 'refus': problems[:20], 'non_rejouable': not_replayable,
                    'medianes_us': summary(takes) if not problems else None}, ensure_ascii=False, sort_keys=True))
  return 0 if not problems else 3


def main(argv):
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  parser.add_argument('--work')
  parser.add_argument('--out')
  parser.add_argument('--nvcc')
  parser.add_argument('--reps', type=int, default=2000)
  parser.add_argument('--processes', type=int, default=3)
  parser.add_argument('--rejuger', metavar='DOSSIER')
  try:
    args = parser.parse_args(argv[1:])
  except SystemExit:
    return 2
  if not 10 <= args.reps <= 100000 or not 1 <= args.processes <= 20:
    print('run_m6 : --reps ou --processes hors bornes', file=sys.stderr)
    return 2
  if args.rejuger:
    if args.work or args.out or args.nvcc:
      print('run_m6 : --rejuger exclut --work, --out et --nvcc', file=sys.stderr)
      return 2
    return rejudge(Path(args.rejuger), args)
  if not args.work or not args.out:
    print('run_m6 : --work et --out exiges (ou --rejuger)', file=sys.stderr)
    return 2
  return run(args)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
