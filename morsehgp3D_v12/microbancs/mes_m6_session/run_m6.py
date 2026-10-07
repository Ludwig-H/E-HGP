#!/usr/bin/env python3
"""MES-M6 sur G4 : compile mes_m6_session_cost.cu et le joue dans un processus neuf par mode d'attente de l'hote.

Usage : run_m6.py --work W --out O [--nvcc NVCC] [--reps N] [--processes P]
Chaque mode (spin, yield, blocking) est joue P fois, chaque fois dans un processus neuf (le mode d'attente se fixe
avant l'ouverture du contexte) ; les lignes JSON du banc vont dans O/m6_<mode>_<i>.jsonl, l'environnement (nvcc,
nvidia-smi, uptime) et les codes dans O/m6_report.json. Bibliotheque standard seule. Codes : 0 conforme ; 2 usage ;
3 compilation ou execution en echec.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'mes_m6_session_cost.cu'
MODES = ('spin', 'yield', 'blocking')


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


def main(argv):
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  parser.add_argument('--work', required=True)
  parser.add_argument('--out', required=True)
  parser.add_argument('--nvcc')
  parser.add_argument('--reps', type=int, default=2000)
  parser.add_argument('--processes', type=int, default=3)
  try:
    args = parser.parse_args(argv[1:])
  except SystemExit:
    return 2
  if not 10 <= args.reps <= 100000 or not 1 <= args.processes <= 20:
    print('run_m6 : --reps ou --processes hors bornes', file=sys.stderr)
    return 2
  work, out = Path(args.work), Path(args.out)
  work.mkdir(parents=True, exist_ok=True)
  out.mkdir(parents=True, exist_ok=True)
  report = {'schema': 'ehgp.v12.mes_m6.v1', 'date_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}
  nvcc = find_nvcc(args.nvcc)
  report['nvcc'] = nvcc and capture([nvcc, '--version'], 60)[1].strip().splitlines()[-1:]
  report['gpu'] = capture(['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,clocks.sm,clocks.max.sm,'
                           'temperature.gpu,power.draw,memory.used,memory.total', '--format=csv,noheader'], 60)[1].strip()
  report['gpu_apps'] = capture(['nvidia-smi', '--query-compute-apps=pid,process_name,used_memory',
                                '--format=csv,noheader'], 60)[1].strip()
  report['uptime_since'] = capture(['uptime', '-s'], 30)[1].strip()
  code = 0
  binary = work / 'mes_m6'
  if nvcc is None:
    report['refusal'] = 'nvcc introuvable'
    code = 3
  else:
    compile_code, _, compile_err = capture([nvcc, '-std=c++20', '-O3', '-arch=sm_120', '-Xcompiler',
                                            '-Wall,-Wextra,-Werror', '-o', str(binary), str(SOURCE)], 600)
    report['compile'] = {'code': compile_code, 'stderr': compile_err[-2000:]}
    if compile_code != 0:
      code = 3
  runs = []
  if code == 0:
    for index in range(args.processes):
      for mode in MODES:
        started = time.time()
        run_code, run_out, run_err = capture([str(binary), '--sync=' + mode, '--reps=' + str(args.reps)], 900)
        (out / ('m6_%s_%d.jsonl' % (mode, index))).write_text(run_out, encoding='utf-8')
        runs.append({'mode': mode, 'process': index, 'code': run_code, 'seconds': round(time.time() - started, 2),
                     'stderr': run_err[-1000:], 'lines': run_out.count('\n')})
        if run_code != 0:
          code = 3
  report['runs'] = runs
  (out / 'm6_report.json').write_text(json.dumps(report, indent=1, sort_keys=True), encoding='utf-8')
  print('mes_m6_%s processus=%d' % ('ok' if code == 0 else 'echec', len(runs)))
  return code


if __name__ == '__main__':
  sys.exit(main(sys.argv))
