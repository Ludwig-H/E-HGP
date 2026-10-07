#!/usr/bin/env python3
"""Recopie dans les resultats d'une session les sorties legeres d'un microbanc, sans ses vidages.

Les microbancs ecrivent vidages et constructions (dizaines de Mo a plusieurs Go, derives de SemanticKITTI) dans le
dossier de construction de la VM ; seuls leurs rapports, journaux et comptes doivent etre rapatries, sous le plafond
des resultats de la session.

Usage : publier.py --from SRC --to DEST [--exclude-dir NOM]... [--exclude-suffix .bin]...
                   [--max-file-bytes N] [--max-bytes N] --report R
Copie les fichiers reguliers de SRC (liens ignores), sauf ceux d'un dossier exclu ou d'un suffixe exclu, dans l'ordre
des chemins ; un fichier plus gros que --max-file-bytes est saute et compte ; la copie s'arrete avant de depasser
--max-bytes. Bibliotheque standard seule. Codes : 0 conforme ; 2 usage ; 3 plafond atteint (rapport ecrit, copie
partielle signalee).
"""

import argparse
import json
import os
import shutil
import sys


def main(argv):
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  parser.add_argument('--from', dest='source', required=True)
  parser.add_argument('--to', dest='dest', required=True)
  parser.add_argument('--exclude-dir', action='append', default=[])
  parser.add_argument('--exclude-suffix', action='append', default=[])
  parser.add_argument('--max-file-bytes', type=int, default=8 << 20)
  parser.add_argument('--max-bytes', type=int, default=40 << 20)
  parser.add_argument('--report', required=True)
  try:
    args = parser.parse_args(argv[1:])
  except SystemExit:
    return 2
  if not os.path.isdir(args.source):
    print('publier : dossier source absent : %s' % args.source, file=sys.stderr)
    return 2
  copied, skipped, total, code = [], [], 0, 0
  for root, dirs, files in os.walk(args.source):
    dirs[:] = sorted(d for d in dirs if d not in args.exclude_dir)
    for name in sorted(files):
      path = os.path.join(root, name)
      relative = os.path.relpath(path, args.source)
      if os.path.islink(path) or not os.path.isfile(path):
        continue
      if any(name.endswith(suffix) for suffix in args.exclude_suffix):
        continue
      size = os.path.getsize(path)
      if size > args.max_file_bytes:
        skipped.append({'path': relative, 'bytes': size, 'reason': 'max_file_bytes'})
        continue
      if total + size > args.max_bytes:
        skipped.append({'path': relative, 'bytes': size, 'reason': 'max_bytes'})
        code = 3
        continue
      target = os.path.join(args.dest, relative)
      os.makedirs(os.path.dirname(target), exist_ok=True)
      shutil.copyfile(path, target)
      copied.append(relative)
      total += size
  report = {'schema': 'ehgp.v12.publier.v1', 'copied': len(copied), 'bytes': total, 'skipped': skipped,
            'excluded_dirs': args.exclude_dir, 'excluded_suffixes': args.exclude_suffix}
  os.makedirs(os.path.dirname(os.path.abspath(args.report)), exist_ok=True)
  with open(args.report, 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=1, sort_keys=True)
  print('publier_%s fichiers=%d octets=%d sautes=%d' % ('ok' if code == 0 else 'plafond', len(copied), total,
                                                        len(skipped)))
  return code


if __name__ == '__main__':
  sys.exit(main(sys.argv))
