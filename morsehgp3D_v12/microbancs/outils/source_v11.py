#!/usr/bin/env python3
"""Deballe l'archive epinglee des sources de la v11 gelee et construit sa bibliotheque, pour les microbancs.

Le paquet d'une session G4 de la v12 ne contient que morsehgp3D_v12/ ; les microbancs qui lient la v11 gelee
(vidages, temoins) recoivent ses sources par une archive transmise avec les donnees (--data) et epinglee ici par son
SHA-256. L'archive est faite sur le codespace par :

  git archive --format=tar.gz ac081a06f morsehgp3D_v11/CMakeLists.txt morsehgp3D_v11/cmake morsehgp3D_v11/src \
      morsehgp3D_v11/bench morsehgp3D_v11/tests morsehgp3D_v11/reference morsehgp3D_v11/tools morsehgp3D_v11/cli

Usage : source_v11.py --archive A --sha256 H --dest D [--build B --jobs N] --report R
  - verifie l'empreinte avant toute lecture du contenu ; refuse tout membre qui n'est ni fichier ni dossier, tout
    chemin absolu ou avec '..', tout chemin hors de morsehgp3D_v11/ ;
  - avec --build : construit la bibliotheque libmhgp11.a (Release, profil 21, tous les modules) dans B.
Bibliotheque standard seule (Python 3.10 nu de la VM). Codes : 0 conforme ; 2 usage ou archive refusee ; 3 echec de
construction.
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import time

ROOT = 'morsehgp3D_v11'


def sha256_of(path):
  digest = hashlib.sha256()
  with open(path, 'rb') as handle:
    for block in iter(lambda: handle.read(1 << 20), b''):
      digest.update(block)
  return digest.hexdigest()


def safe_members(archive):
  members = []
  for member in archive.getmembers():
    name = member.name
    parts = name.split('/')
    if name.startswith('/') or '..' in parts or parts[0] != ROOT:
      raise ValueError('chemin refuse : %s' % name)
    if not (member.isfile() or member.isdir()):
      raise ValueError('membre refuse (ni fichier ni dossier) : %s' % name)
    members.append(member)
  return members


def run(command, log_path, timeout):
  started = time.time()
  with open(log_path, 'w', encoding='utf-8') as log:
    try:
      done = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=timeout, check=False)
      code = done.returncode
    except subprocess.TimeoutExpired:
      code = -1
  return code, round(time.time() - started, 2)


def main(argv):
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  parser.add_argument('--archive', required=True)
  parser.add_argument('--sha256', required=True)
  parser.add_argument('--dest', required=True)
  parser.add_argument('--build')
  parser.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) - 2))
  parser.add_argument('--report', required=True)
  try:
    args = parser.parse_args(argv[1:])
  except SystemExit:
    return 2
  report = {'schema': 'ehgp.v12.source_v11.v1', 'expected_sha256': args.sha256}
  try:
    actual = sha256_of(args.archive)
    report['archive_sha256'] = actual
    if actual != args.sha256:
      raise ValueError('empreinte de l\'archive differente de l\'epingle')
    os.makedirs(args.dest, exist_ok=True)
    with tarfile.open(args.archive, 'r:gz') as archive:
      members = safe_members(archive)
      for member in members:
        target = os.path.join(args.dest, member.name)
        if member.isdir():
          os.makedirs(target, exist_ok=True)
          continue
        os.makedirs(os.path.dirname(target), exist_ok=True)
        source = archive.extractfile(member)
        with open(target, 'wb') as out:
          shutil.copyfileobj(source, out)
        os.chmod(target, 0o755 if member.mode & 0o111 else 0o644)
      report['files'] = sum(1 for member in members if member.isfile())
  except (OSError, ValueError, tarfile.TarError) as error:
    report['refusal'] = str(error)
    with open(args.report, 'w', encoding='utf-8') as out:
      json.dump(report, out, indent=1, sort_keys=True)
    print('source_v11_refus : %s' % error, file=sys.stderr)
    return 2
  code = 0
  if args.build:
    cmake = shutil.which('cmake') or 'cmake'
    source = os.path.join(args.dest, ROOT)
    os.makedirs(args.build, exist_ok=True)
    configure = [cmake, '-S', source, '-B', args.build, '-DCMAKE_BUILD_TYPE=Release', '-DMHGP11_COORD_BITS=21']
    report['configure'] = run(configure, os.path.join(args.build, 'configure.log'), 900)
    if report['configure'][0] == 0:
      build = [cmake, '--build', args.build, '-j', str(args.jobs), '--target', 'mhgp11']
      report['build'] = run(build, os.path.join(args.build, 'build.log'), 1800)
    library = os.path.join(args.build, 'libmhgp11.a')
    if os.path.isfile(library):
      report['libmhgp11_sha256'] = sha256_of(library)
    else:
      code = 3
  with open(args.report, 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=1, sort_keys=True)
  print('source_v11_%s fichiers=%d' % ('ok' if code == 0 else 'echec', report.get('files', 0)))
  return code


if __name__ == '__main__':
  sys.exit(main(sys.argv))
